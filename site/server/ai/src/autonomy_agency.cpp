//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#ifdef  HAS_BEEN_REFACTORED

#include "autonomy_agency.h"
#include <algorithm>

AutonomyAgency::AutonomyAgency(Config cfg,
                               std::shared_ptr<IRawCollector> raw_collector,
                               std::shared_ptr<ICooker> cooker,
                               std::shared_ptr<IPlanner> planner,
                               std::shared_ptr<IRenderer> renderer,
                               std::shared_ptr<ICommandInjector> injector,
                               std::shared_ptr<ITelemetrySink> telemetry,
                               std::shared_ptr<IAtomicPhaseGate> atomic_gate)
  : cfg_(std::move(cfg))
  , raw_collector_(std::move(raw_collector))
  , cooker_(std::move(cooker))
  , planner_(std::move(planner))
  , renderer_(std::move(renderer))
  , injector_(std::move(injector))
  , telemetry_(std::move(telemetry))
  , atomic_gate_(std::move(atomic_gate))
{}

AutonomyAgency::~AutonomyAgency()
{
    request_stop();
    join();
}

void AutonomyAgency::pre_init_load_dsl_files(const std::vector<std::string>& /*lisp_paths*/)
{
    // Stub: real ECL init/load belongs in an EclPlanner implementation.
    // Keep AA host ignorant of ECL specifics beyond "planner exists".
}

void AutonomyAgency::start()
{
    bool expected = false;
    if (!started_.compare_exchange_strong(expected, true))
        return;

    stop_.store(false);
    worker_ = std::thread(&AutonomyAgency::thread_main, this);
    pump(PumpReason::ManualKick);
}

void AutonomyAgency::request_stop()
{
    stop_.store(true);
    pump(PumpReason::Shutdown);
}

void AutonomyAgency::join()
{
    if (worker_.joinable())
        worker_.join();
}

void AutonomyAgency::pump(PumpReason why)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pump_q_.push_back(why);
    }
    cv_.notify_one();
}

void AutonomyAgency::notify_player_command_activity()
{
    player_cmd_flag_.store(true);
    pump(PumpReason::PlayerCommandObserved);
}

void AutonomyAgency::thread_main()
{
    while (!stop_.load())
    {
        PumpReason reason = PumpReason::UiPoll;

        {
            std::unique_lock<std::mutex> lk(mtx_);
            if (pump_q_.empty())
            {
                cv_.wait_for(lk, cfg_.default_wait, [&]{
                    return stop_.load() || !pump_q_.empty();
                });
            }

            if (stop_.load())
                break;

            if (!pump_q_.empty())
            {
                reason = pump_q_.front();
                pump_q_.pop_front();
            }
        }

        run_one_cycle(reason);
    }
}

void AutonomyAgency::run_one_cycle(PumpReason /*reason*/)
{
    // If combat is in progress, we do the tighter internal loop.
    // Combat loop itself still does Raw→Cook→Calc→Render→Telem per round.
    bool player_observed = player_cmd_flag_.exchange(false);
    combat_loop_if_needed(player_observed);

    // Normal single pass (pachinko)
    RawInputs raw = gather_raw();
    CookedInputs cooked = cook_inputs(raw);

    if (cooked.game_done)
        return;

    Plan plan = calculate_plan(cooked);
    render(cooked, plan);
    telemeter(cooked, plan);
}

RawInputs AutonomyAgency::gather_raw()
{
    const bool player_observed = player_cmd_flag_.exchange(false);
    const uint64_t cid = ++cycle_id_;

    if (!raw_collector_)
        return RawInputs{};

    return raw_collector_->gather_raw(player_observed, cid);
}

CookedInputs AutonomyAgency::cook_inputs(const RawInputs& raw)
{
    if (!cooker_)
    {
        CookedInputs cooked;
        cooked.session_id = raw.snap.session_id;
        cooked.turn_number = raw.snap.turn_number;
        cooked.phase = raw.snap.phase;
        cooked.aa_is_active_player = raw.snap.aa_is_active_player;
        cooked.game_done = raw.snap.game_done;
        cooked.in_combat = raw.snap.in_combat;
        cooked.movement_atomic_now = (raw.snap.phase == GameSnapshot::Phase::Movement);
        cooked.combat_atomic_now = (raw.snap.phase == GameSnapshot::Phase::Combat);
        cooked.contested_hexes_sorted = raw.snap.contested_hexes;
        cooked.monotonic_cycle_id = raw.monotonic_cycle_id;
        return cooked;
    }

    return cooker_->cook(raw, slate_);
}

Plan AutonomyAgency::calculate_plan(const CookedInputs& cooked)
{
    if (!planner_)
        return Plan{};

    Plan plan = planner_->decide(cooked, slate_);

    if (plan.commands.size() > cfg_.max_commands_per_cycle)
        plan.commands.resize(cfg_.max_commands_per_cycle);

    // Update slate if planner returned a patch.
    if (plan.new_slate_json.has_value())
        slate_.opaque_json = *plan.new_slate_json;

    // Track last seen
    slate_.last_session_id = cooked.session_id;
    slate_.last_turn_seen  = cooked.turn_number;

    return plan;
}

void AutonomyAgency::render(const CookedInputs& cooked, const Plan& plan)
{
    if (!renderer_ || !injector_)
        return;

    renderer_->render_and_inject(cooked, plan, *injector_, atomic_gate_.get());
}

void AutonomyAgency::telemeter(const CookedInputs& cooked, const Plan& plan)
{
    if (!telemetry_)
        return;

    for (auto ev : plan.telemetry)
    {
        if (ev.session_id == 0) ev.session_id = cooked.session_id;
        if (ev.turn_number == 0) ev.turn_number = cooked.turn_number;
        telemetry_->publish(ev);
    }
}

void AutonomyAgency::combat_loop_if_needed(bool player_command_observed)
{
    // First, peek at current state (Raw→Cook).
    const uint64_t cid = ++cycle_id_;
    RawInputs raw0 = raw_collector_ ? raw_collector_->gather_raw(player_command_observed, cid) : RawInputs{};
    CookedInputs cooked0 = cook_inputs(raw0);

    if (!cooked0.in_combat && cooked0.phase != GameSnapshot::Phase::Combat)
        return;

    // Tight loop while combat persists.
    for (int round = 0; round < cfg_.combat_round_cap && !stop_.load(); ++round)
    {
        const uint64_t rid = ++cycle_id_;
        RawInputs raw = raw_collector_ ? raw_collector_->gather_raw(false, rid) : RawInputs{};
        CookedInputs cooked = cook_inputs(raw);

        if (!cooked.in_combat && cooked.phase != GameSnapshot::Phase::Combat)
            break;

        Plan plan = calculate_plan(cooked);
        render(cooked, plan);
        telemeter(cooked, plan);

        std::this_thread::sleep_for(cfg_.combat_yield_every);
    }
}

void AutonomyAgency::publish_info(const CookedInputs& cooked, const std::string& msg)
{
    if (!telemetry_)
        return;

    TelemetryEvent ev;
    ev.kind = TelemetryEvent::Kind::Info;
    ev.text = msg;
    ev.session_id = cooked.session_id;
    ev.turn_number = cooked.turn_number;
    telemetry_->publish(ev);
}

#endif // HAS_BEEN_REFACTORED
