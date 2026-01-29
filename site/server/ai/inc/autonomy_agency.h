//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_AUTONOMY_AGENCY_H__
#define __KH_AUTONOMY_AGENCY_H__


#if HAS_BEEN_REFACTORED

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <chrono>

// ----------------------------
// Host-facing primitives
// ----------------------------

struct TelemetryEvent
{
    enum class Kind : uint8_t { Info, Decision, Warning, Error, Combat };
    Kind kind = Kind::Info;
    std::string text;
    uint32_t session_id = 0;
    uint32_t turn_number = 0;
};

struct InjectedCommand
{
    std::string text;   // EXACT user-grammar command line
    std::string tag;    // for logging/debug (optional)
};

struct ICommandInjector
{
    virtual ~ICommandInjector() = default;
    virtual void inject(const InjectedCommand& cmd) = 0;
};

struct ITelemetrySink
{
    virtual ~ITelemetrySink() = default;
    virtual void publish(const TelemetryEvent& ev) = 0;
};

// Optional but recommended: host provides "atomic phase" gate.
// If you already have turn locks / phase locks, map them here.
struct IAtomicPhaseGate
{
    virtual ~IAtomicPhaseGate() = default;

    // Acquire exclusive gate for atomic phases (movement/combat resolution).
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

// ----------------------------
// Raw/Cooked boundary
// ----------------------------

struct RawInputs
{
    GameSnapshot snap;

    // Raw DB rows / metrics / session tables etc (opaque placeholder).
    // Replace with typed results later.
    std::string db_blob_json;

    // Raw event indicators (UI tick / player activity).
    bool player_command_observed = false;

    // Timing
    uint32_t monotonic_cycle_id = 0;
};

// ----------------------------
// AA private state ("Slate")
// ----------------------------

struct Slate
{
    uint32_t last_session_id = 0;
    uint32_t last_turn_seen = 0;

    // Your “memory”: plan fragments, combat intent, economic goals, etc.
    // Keep opaque for now; you can refactor later.
    std::string opaque_json = "{}";
};

// ----------------------------
// Planner output ("Plan")
// ----------------------------

struct Plan
{
    // Commands to inject this cycle/round
    std::vector<InjectedCommand> commands;

    // Telemetry to emit
    std::vector<TelemetryEvent> telemetry;

    // Optional: slate patch (or just mutate slate in-place in the planner)
    // Here we keep it simple: planner may return an updated opaque json.
    std::optional<std::string> new_slate_json;
};

// ----------------------------
// Interfaces: collection, cooking, planning, rendering
// ----------------------------

struct IRawCollector
{
    virtual ~IRawCollector() = default;
    virtual RawInputs gather_raw(bool player_command_observed,
                                 uint32_t monotonic_cycle_id) = 0;
};

struct ICooker
{
    virtual ~ICooker() = default;
    virtual CookedInputs cook(const RawInputs& raw, const Slate& slate) = 0;
};

struct IPlanner
{
    virtual ~IPlanner() = default;

    // ECL boundary later: planner reads Cooked only.
    virtual Plan decide(const CookedInputs& cooked, const Slate& slate) = 0;
};

struct IRenderer
{
    virtual ~IRenderer() = default;

    // Renderer may:
    // - filter/sequence commands
    // - respect atomic phase policies
    // - chunk combat sequence (order/commit/assign)
    virtual void render_and_inject(const CookedInputs& cooked,
                                   const Plan& plan,
                                   ICommandInjector& injector,
                                   IAtomicPhaseGate* gate_or_null) = 0;
};

// ----------------------------
// AutonomyAgency
// ----------------------------

class AutonomyAgency
{
public:
    struct Config
    {
        std::chrono::milliseconds default_wait = std::chrono::milliseconds(3000);
        size_t max_commands_per_cycle = 16;

        // Safety: how long a combat internal loop may spin before yielding
        std::chrono::milliseconds combat_yield_every = std::chrono::milliseconds(50);
        int combat_round_cap = 64;
    };

    enum class PumpReason : uint8_t
    {
        UiPoll,
        PlayerCommandObserved,
        ServerStateChanged,
        ManualKick,
        Shutdown
    };

    AutonomyAgency(Config cfg,
                   std::shared_ptr<IRawCollector> raw_collector,
                   std::shared_ptr<ICooker> cooker,
                   std::shared_ptr<IPlanner> planner,
                   std::shared_ptr<IRenderer> renderer,
                   std::shared_ptr<ICommandInjector> injector,
                   std::shared_ptr<ITelemetrySink> telemetry,
                   std::shared_ptr<IAtomicPhaseGate> atomic_gate = nullptr);

    ~AutonomyAgency();

    // ECL/DSL pre-init hook (stubbed here; real work in planner impl)
    void pre_init_load_dsl_files(const std::vector<std::string>& lisp_paths);

    void start();
    void request_stop();
    void join();

    void pump(PumpReason why);
    void notify_player_command_activity();

private:
    void thread_main();
    void run_one_cycle(PumpReason reason);

    // MSS stages
    RawInputs gather_raw();
    CookedInputs cook_inputs(const RawInputs& raw);
    Plan calculate_plan(const CookedInputs& cooked);
    void render(const CookedInputs& cooked, const Plan& plan);
    void telemeter(const CookedInputs& cooked, const Plan& plan);
    void wait_or_block();

    // Combat internal loop: repeats Raw→Cook→Calc→Render→Telem while combat persists
    void combat_loop_if_needed(bool player_command_observed);

    void publish_info(const CookedInputs& cooked, const std::string& msg);

private:
    Config cfg_;

    std::shared_ptr<IRawCollector> raw_collector_;
    std::shared_ptr<ICooker> cooker_;
    std::shared_ptr<IPlanner> planner_;
    std::shared_ptr<IRenderer> renderer_;
    std::shared_ptr<ICommandInjector> injector_;
    std::shared_ptr<ITelemetrySink> telemetry_;
    std::shared_ptr<IAtomicPhaseGate> atomic_gate_;

    std::atomic<bool> started_{false};
    std::atomic<bool> stop_{false};

    std::thread worker_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<PumpReason> pump_q_;

    // edge-trigger: “player command observed since last cycle”
    std::atomic<bool> player_cmd_flag_{false};

    // AA state (Mealy state)
    Slate slate_;

    // monotonic cycle id
    std::atomic<uint32_t> cycle_id_{0};
};

#endif // HAS_BEEN_REFACTORED

#endif
