///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "ai_command_injector.h"
#include "ai_db_mutex.h"
#include "autonomy_agency.h"
#include "configr.h"
#include "db.h"
#include "ecl_bridge.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "statemachine.h"

#include <chrono>
#include <set>

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------

AutonomyAgency& AutonomyAgency::instance()
{
    static AutonomyAgency singleton;
    return singleton;
}

// ----------------------------------------------------------------------------
// Construction / Destruction
// ----------------------------------------------------------------------------

AutonomyAgency::AutonomyAgency()
    : m_game_id(0), m_aa_player('\0'), m_cycle_counter(0), m_running(false),
      m_stop_requested(false), m_pumped(false), m_task_done(false),
      m_ecl_initialized(false)
{
}

AutonomyAgency::~AutonomyAgency()
{
    stop();
    join();
    shutdown_ecl();
}

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

void AutonomyAgency::configure(int game_id, char aa_player)
{
    m_game_id = game_id;
    m_aa_player = aa_player;
    m_slate.game_id = game_id;
    m_slate.aa_player = aa_player;
}

// ----------------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------------

void AutonomyAgency::start()
{
    bool already_running = m_running.exchange(true);
    if (already_running)
    {
        return;
    }

    m_stop_requested.store(false);

    // Initialize ECL if not already done
    if (!m_ecl_initialized)
    {
        init_ecl();
    }

    m_worker = std::thread(&AutonomyAgency::thread_main, this);
}

void AutonomyAgency::stop()
{
    m_stop_requested.store(true);

    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_pumped = true;
    }
    m_cv.notify_one();
}

void AutonomyAgency::join()
{
    if (m_worker.joinable())
    {
        m_worker.join();
    }
    m_running.store(false);
}

void AutonomyAgency::pump()
{
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_pumped = true;
    }
    m_cv.notify_one();
}

// ----------------------------------------------------------------------------
// Task Synchronization
// ----------------------------------------------------------------------------

void AutonomyAgency::wait_for_task_completion()
{
    std::unique_lock<std::mutex> lk(m_task_mtx);
    m_task_done = false;
    m_task_cv.wait(lk,
                   [this] { return m_task_done || m_stop_requested.load(); });
}

void AutonomyAgency::notify_task_complete()
{
    {
        std::lock_guard<std::mutex> lk(m_task_mtx);
        m_task_done = true;
    }
    m_task_cv.notify_one();
}

// ----------------------------------------------------------------------------
// Thread Entry
// ----------------------------------------------------------------------------

void AutonomyAgency::thread_main()
{
    MySqlThreadGuard mysql_guard;

    // Register this thread with ECL runtime
    // Required for threads not created by ECL itself
    ecl_import_current_thread(ECL_NIL, ECL_NIL);

    while (!m_stop_requested.load())
    {
        // Wait for pump or timeout
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            bool signaled = m_cv.wait_for(
                lk, std::chrono::milliseconds(AI_THREAD_PUMP_DELAY),
                [this] { return m_pumped || m_stop_requested.load(); });

            if (m_stop_requested.load())
            {
                break;
            }

            m_pumped = false;
        }

        run_cycle();
    }

    // Release ECL thread before exit
    ecl_release_current_thread();
}

// ----------------------------------------------------------------------------
// Run One Cycle
// ----------------------------------------------------------------------------

void AutonomyAgency::run_cycle()
{
    ++m_cycle_counter;

    // Mealy State Machine: GATHER -> CALC -> RENDER -> loop
    enum MealyState
    {
        MS_GATHER,
        MS_CALC,
        MS_RENDER,
        MS_FINISH
    };

    bool done = false;
    MealyState state = MS_GATHER;
    std::string pending_cmd;
    int iterations = 0;
    const int kMaxIterations = 100; // Safety limit

    while (!done && !m_stop_requested.load() && iterations < kMaxIterations)
    {
        ++iterations;
        switch (state)
        {
        case MS_GATHER:
            m_slate.cycle_id = m_cycle_counter;
            gather();

            // Discrete 1: log combat-resolution state after every gather
            if (!m_slate.active_combats.empty())
            {
                for (const AACombatHex& ch : m_slate.active_combats)
                {
                    Logger::instance().ai(
                        "[COMBAT-DETECT] hex=" + ch.hex_id +
                        " stage=" + std::to_string(ch.stage) + " round=" +
                        std::to_string(ch.round) + " ai_committed=" +
                        std::to_string(ch.ai_committed ? 1 : 0) +
                        " ai_attacker=" +
                        std::to_string(ch.ai_is_attacker ? 1 : 0) +
                        " stalemate=" + std::to_string(ch.stalemate_counter));
                }
            }

            if (m_slate.game_over)
            {
                done = true;
            }
            else if (m_slate.is_aa_turn)
            {
                // AI's turn - log ship positions and combat state
                log_debug_state();
                state = MS_CALC;
            }
            else if (combat_needs_response())
            {
                // Not AI's turn, but combat needs AI response
                log_debug_state();
                state = MS_CALC;
            }
            else
            {
                // Not AI's turn and no combat - sleep
                done = true;
            }
            break;

        case MS_CALC:
        {
            // Calculate ONE command based on current state
            std::vector<std::string> commands;
            std::vector<std::pair<std::string, double>> metrics;
            calculate(commands, metrics);

            // Persist any metrics returned by Lisp
            for (const std::pair<std::string, double>& met : metrics)
            {
                persist_metric(met.first, met.second, m_slate.round);
            }

            // Discrete 2: log what ECL decided for combat
            if (!m_slate.active_combats.empty())
            {
                std::string decision =
                    commands.empty() ? "(EMPTY)" : commands[0];
                Logger::instance().ai(
                    "[COMBAT-DECISION] phase=" +
                    std::to_string(m_slate.phase_index) + " combats=" +
                    std::to_string(m_slate.active_combats.size()) +
                    " ecl_cmd=" + decision);
            }

            if (commands.empty())
            {
                // Nothing to do - shouldn't happen, Lisp should return NEXT
                done = true;
            }
            else
            {
                pending_cmd = commands[0];
                state = MS_RENDER;
            }
        }
        break;

        case MS_RENDER:
        {
            // Execute the ONE pending command
            AICommandInjector::inject(m_game_id, m_aa_player, pending_cmd);

            // Block until TaskRunner completes the command
            wait_for_task_completion();

            // Check if terminal (hands control to other player or next phase)
            bool is_terminal =
                (KH_EQU(pending_cmd, "NEXT") || KH_EQU(pending_cmd, "DONE") ||
                 KH_EQU(pending_cmd.rfind("NEXT", 0), 0) ||
                 KH_EQU(pending_cmd.rfind("DONE", 0), 0));

            if (is_terminal)
            {
                done = true;
            }
            else
            {
                // Fresh gather with updated DB state
                ++m_cycle_counter;
                state = MS_GATHER;
            }
            pending_cmd.clear();
        }
        break;

        case MS_FINISH:
        default:
            done = true;
            break;
        }
    }

    if (iterations >= kMaxIterations)
    {
        std::cerr << "hit max iterations in AI-Autonomy Loop" << std::endl;
    }
}

// ----------------------------------------------------------------------------
// Gather Step (C++)
// ----------------------------------------------------------------------------

void AutonomyAgency::gather()
{
    m_slate.clear();
    m_slate.game_id = m_game_id;
    m_slate.aa_player = m_aa_player;
    m_slate.cycle_id = m_cycle_counter;

    // Safety guard: if game context was cleared, bail out gracefully
    if (m_game_id <= 0)
    {
        m_slate.game_over = true;
        return;
    }

    // ----------------------------------
    // From StateMachine
    // ----------------------------------
    GameState gs = StateMachine::instance().get_game_state();

    m_slate.round = gs.round;
    m_slate.phase_index = gs.phase_index;
    m_slate.game_over = gs.game_over;

    if (!gs.active_player.empty())
    {
        m_slate.active_player = gs.active_player[0];
    }
    else
    {
        m_slate.active_player = 'A';
    }

    m_slate.is_aa_turn = (KH_EQU(m_slate.active_player, m_slate.aa_player));

    if (KH_EQU(m_slate.aa_player, 'A'))
    {
        m_slate.credits = gs.creditsA;
        m_slate.vp = gs.vpA;
        m_slate.enemy_vp = gs.vpB;
        m_slate.home_side = gs.home_side_A;
    }
    else
    {
        m_slate.credits = gs.creditsB;
        m_slate.vp = gs.vpB;
        m_slate.enemy_vp = gs.vpA;
        m_slate.home_side = gs.home_side_B;
    }

    m_slate.tech_level = gs.get_current_tech_level();

    // ----------------------------------
    // From Database (with mutex)
    // ----------------------------------
    std::lock_guard<std::mutex> db_lock(AIDBMutex::ai_mutex);
    DatabaseManager& db = DatabaseManager::instance();

    std::string aa_str(1, m_slate.aa_player);
    std::string game_id_str = std::to_string(m_slate.game_id);

    // Own base hexes
    // Prefer base_stars (per-game claimed bases, matches VP scoring system).
    // Fall back to star_systems (module template) if base_stars not yet
    // populated.
    std::string our_player(1, m_slate.aa_player);
    std::string enemy_player = (KH_EQU(m_slate.aa_player, 'A')) ? "B" : "A";

    auto own_base_rows =
        db.Query("SELECT hex_id FROM base_stars WHERE game_id=? AND owner=?",
                 {m_slate.game_id, our_player});

    if (!own_base_rows.empty())
    {
        // base_stars populated: use it for both sides
        for (const std::vector<std::string>& row : own_base_rows)
        {
            if (!row.empty())
            {
                m_slate.own_base_hexes.push_back(row[0]);
            }
        }
        auto enemy_base_rows = db.Query(
            "SELECT hex_id FROM base_stars WHERE game_id=? AND owner=?",
            {m_slate.game_id, enemy_player});
        for (const std::vector<std::string>& row : enemy_base_rows)
        {
            if (!row.empty())
            {
                m_slate.enemy_base_hexes.push_back(row[0]);
            }
        }
    }
    else
    {
        // Fallback: star_systems template (pre-claim, first turn)
        std::string our_side = m_slate.home_side;
        if (our_side.empty())
        {
            our_side = (KH_EQU(m_slate.aa_player, 'B')) ? "B" : "A";
        }
        std::string sql_bases =
            "SELECT hex_id FROM star_systems "
            "WHERE module_id=1 AND is_base=1 AND base_side=?";
        auto base_rows = db.Query(sql_bases, {our_side});
        for (const std::vector<std::string>& row : base_rows)
        {
            if (!row.empty())
            {
                m_slate.own_base_hexes.push_back(row[0]);
            }
        }
        std::string enemy_side = (KH_EQU(our_side, "A")) ? "B" : "A";
        auto enemy_base_rows = db.Query(sql_bases, {enemy_side});
        for (const std::vector<std::string>& row : enemy_base_rows)
        {
            if (!row.empty())
            {
                m_slate.enemy_base_hexes.push_back(row[0]);
            }
        }
    }

    // Own ships
    // Query pd_spent to compute remaining PD for this turn
    // Also fetch racked_in to track systemship rack status
    // Extended for economic layer: cargo, capacity, max values, equipment
    std::string sql_own =
        "SELECT ship_code, ship_name, at_hex, at_system, pd, beam, screen, "
        "tube, missiles, sr, tech_level, ship_type, COALESCE(pd_spent,0), "
        "COALESCE(racked_in,''), "
        "cargo_ferrous, cargo_rare_earth, cargo_radioactive, "
        "cargo_crystalline, "
        "cargo_volatile, cargo_water, cargo_organic, cargo_exotic, "
        "cargo_missiles, cargo_capacity, missiles_max, "
        "pd_max, beam_max, screen_max, tube_max, "
        "COALESCE(lrs,0), COALESCE(tb,0) "
        "FROM ships WHERE game_id=? AND owner=? AND destroyed_at IS NULL";

    auto own_rows = db.Query(sql_own, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : own_rows)
    {
        if (!row.empty())
        {
            AAShipInfo ship;
            ship.code = row[0];
            ship.name = row[1];
            ship.hex_id = row[2];
            ship.at_system = row[3];

            // If at_hex is empty, resolve from at_system
            if (ship.hex_id.empty() && !row[3].empty())
            {
                ship.hex_id = MapUtil::instance().resolve_system_hex(
                    m_slate.game_id, row[3]);
            }

            // base_pd = physical PD for damage assignment
            // pd = remaining PD for power allocation (base - spent this turn)
            int raw_pd = std::atoi(row[4].c_str());
            int pd_spent = std::atoi(row[12].c_str());
            ship.base_pd = raw_pd;
            ship.pd = raw_pd - pd_spent;
            ship.beam = std::atoi(row[5].c_str());
            ship.screen = std::atoi(row[6].c_str());
            ship.tube = std::atoi(row[7].c_str());
            ship.missile = std::atoi(row[8].c_str());
            ship.sr = std::atoi(row[9].c_str());
            ship.tech_level = std::atoi(row[10].c_str());
            ship.is_warpship = (KH_EQU(row[11], "W"));
            ship.racked_in = row[13];

            // Economic layer: cargo
            ship.cargo_ferrous = std::atoi(row[14].c_str());
            ship.cargo_rare_earth = std::atoi(row[15].c_str());
            ship.cargo_radioactive = std::atoi(row[16].c_str());
            ship.cargo_crystalline = std::atoi(row[17].c_str());
            ship.cargo_volatile = std::atoi(row[18].c_str());
            ship.cargo_water = std::atoi(row[19].c_str());
            ship.cargo_organic = std::atoi(row[20].c_str());
            ship.cargo_exotic = std::atoi(row[21].c_str());
            ship.cargo_missiles = std::atoi(row[22].c_str());
            ship.cargo_capacity = std::atoi(row[23].c_str());
            ship.missiles_max = std::atoi(row[24].c_str());

            // Max values for repair decisions
            ship.pd_max = std::atoi(row[25].c_str());
            ship.beam_max = std::atoi(row[26].c_str());
            ship.screen_max = std::atoi(row[27].c_str());
            ship.tube_max = std::atoi(row[28].c_str());

            // Equipment
            ship.lrs = std::atoi(row[29].c_str());
            ship.tb = std::atoi(row[30].c_str());

            // Query systemships racked in this warpship
            if (ship.is_warpship && ship.sr > 0)
            {
                std::string sql_racked =
                    "SELECT ship_code FROM ships WHERE game_id=? AND owner=? "
                    "AND racked_in=? AND destroyed_at IS NULL";
                auto racked_rows =
                    db.Query(sql_racked,
                             {m_slate.game_id, m_slate.aa_player, ship.code});
                for (const std::vector<std::string>& rrow : racked_rows)
                {
                    if (!rrow.empty())
                    {
                        ship.carried_systemships.push_back(rrow[0]);
                    }
                }
            }

            m_slate.own_ships.push_back(ship);
        }
    }

    // Enemy ships
    std::string sql_enemy =
        "SELECT ship_code, ship_name, at_hex, at_system, pd, beam, screen, "
        "tube, missiles, sr, tech_level, ship_type "
        "FROM ships WHERE game_id=? AND owner!=? AND destroyed_at IS NULL";

    auto enemy_rows = db.Query(sql_enemy, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : enemy_rows)
    {
        AAShipInfo ship;
        ship.code = row[0];
        ship.name = row[1];
        ship.hex_id = row[2];

        // If at_hex is empty, resolve from at_system
        if (ship.hex_id.empty() && !row[3].empty())
        {
            ship.hex_id =
                MapUtil::instance().resolve_system_hex(m_slate.game_id, row[3]);
        }

        ship.pd = std::atoi(row[4].c_str());
        ship.beam = std::atoi(row[5].c_str());
        ship.screen = std::atoi(row[6].c_str());
        ship.tube = std::atoi(row[7].c_str());
        ship.missile = std::atoi(row[8].c_str());
        ship.sr = std::atoi(row[9].c_str());
        ship.tech_level = std::atoi(row[10].c_str());
        ship.is_warpship = KH_EQU(row[11], "W");
        m_slate.enemy_ships.push_back(ship);
    }

    // Draft ships (from drafts table)
    std::string sql_drafts =
        "SELECT ship_code, ship_name, pd, beam, screen, tube, missiles, sr, "
        "ship_type FROM drafts WHERE game_id=? AND owner=?";

    auto draft_rows =
        db.Query(sql_drafts, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : draft_rows)
    {
        AAShipInfo ship;
        ship.code = row[0];
        ship.name = row[1];
        ship.pd = std::atoi(row[2].c_str());
        ship.beam = std::atoi(row[3].c_str());
        ship.screen = std::atoi(row[4].c_str());
        ship.tube = std::atoi(row[5].c_str());
        ship.missile = std::atoi(row[6].c_str());
        ship.sr = std::atoi(row[7].c_str());
        ship.tech_level = m_slate.tech_level;
        ship.is_warpship = KH_EQU(row[8], "W");
        m_slate.draft_ships.push_back(ship);
    }

    // Contested hexes (hexes with ships from both players)
    // CRITICAL: Exclude racked systemships (racked_in NULL or empty)
    std::string sql_contested =
        "SELECT at_hex FROM ships WHERE game_id=? AND destroyed_at IS NULL "
        "AND (racked_in IS NULL OR racked_in = '') "
        "AND at_hex IS NOT NULL AND at_hex <> '' "
        "GROUP BY at_hex HAVING COUNT(DISTINCT owner) > 1";

    auto contested_rows = db.Query(sql_contested, {m_slate.game_id});

    for (const std::vector<std::string>& row : contested_rows)
    {
        if (!row.empty())
        {
            m_slate.contested_hexes.push_back(row[0]);
        }
    }

    m_slate.in_combat = !m_slate.contested_hexes.empty();

    // ----------------------------------
    // Combat State (from combat_state, combat_orders, pending_damage)
    // ----------------------------------

    // Get active combats - only for hexes that are actually contested
    // This filters out stale combat_state entries from resolved combats
    std::string sql_combats =
        "SELECT hex_id, stage, round, stalemate_counter, attacker_remains "
        "FROM combat_state WHERE game_id=?";
    auto combat_rows = db.Query(sql_combats, {m_slate.game_id});

    // Build set of contested hexes for fast lookup
    std::set<std::string> contested_set(m_slate.contested_hexes.begin(),
                                        m_slate.contested_hexes.end());

    for (const std::vector<std::string>& row : combat_rows)
    {
        std::string hex_id = row[0];

        // Skip combat_state entries for hexes that are no longer contested
        if (KH_EQU(contested_set.find(hex_id), contested_set.end()))
        {
            continue;
        }

        AACombatHex ch;
        ch.hex_id = hex_id;
        ch.stage = std::atoi(row[1].c_str());
        ch.round = std::atoi(row[2].c_str());
        ch.stalemate_counter = std::atoi(row[3].c_str());
        // attacker_remains indicates if attacker still has initiative
        // AI is attacker if they moved into the hex (simplified: check
        // later)
        ch.ai_is_attacker = (KH_EQU(row[4], "1"));

        // Check if AI has committed orders for this hex and round
        // Must filter destroyed ships to avoid stale orders from
        // prior combats at the same hex
        std::string sql_committed =
            "SELECT COUNT(*) FROM combat_orders co "
            "JOIN ships s ON s.game_id = co.game_id "
            "AND s.ship_code = co.ship_code AND s.owner = co.owner "
            "WHERE co.game_id=? AND co.owner=? AND co.round=? "
            "AND co.committed=1 AND s.at_hex=? "
            "AND s.destroyed_at IS NULL";
        auto commit_rows =
            db.Query(sql_committed,
                     {m_slate.game_id, m_slate.aa_player, ch.round, hex_id});
        ch.ai_committed =
            (!commit_rows.empty() && std::atoi(commit_rows[0][0].c_str()) > 0);

        m_slate.active_combats.push_back(ch);
    }

    // For AI ships in combat hexes, check if they need orders or damage
    for (AAShipInfo& ship : m_slate.own_ships)
    {
        for (const AACombatHex& ch : m_slate.active_combats)
        {
            if (KH_EQU(ship.hex_id, ch.hex_id))
            {
                // Stage 0: Check if this ship has ANY order (draft or
                // committed) Once an order exists (even uncommitted draft),
                // don't re-issue
                if (KH_EQU(ch.stage, 0))
                {
                    std::string sql_has_order =
                        "SELECT COUNT(*) FROM combat_orders "
                        "WHERE game_id=? AND owner=? AND ship_code=? AND "
                        "round=?";
                    auto order_rows = db.Query(
                        sql_has_order, {m_slate.game_id, m_slate.aa_player,
                                        ship.code, ch.round});

                    int order_count = order_rows.empty()
                                          ? 0
                                          : std::atoi(order_rows[0][0].c_str());

                    Logger::instance().ai(
                        "Order check: ship=" + ship.code + " hex=" + ch.hex_id +
                        " round=" + std::to_string(ch.round) +
                        " found=" + std::to_string(order_count));

                    if (KH_EQU(order_count, 0))
                    {
                        ship.needs_combat_order = true;
                    }
                }

                // Stage 2: Check pending damage
                if (KH_EQU(ch.stage, 2))
                {
                    std::string sql_pending =
                        "SELECT damage_amount FROM pending_damage "
                        "WHERE game_id=? AND hex_id=? AND ship_code=? AND "
                        "owner=?";
                    auto dmg_rows =
                        db.Query(sql_pending, {m_slate.game_id, ch.hex_id,
                                               ship.code, m_slate.aa_player});
                    if (!dmg_rows.empty())
                    {
                        ship.pending_damage = std::atoi(dmg_rows[0][0].c_str());
                    }
                }
            }
        }

        // Check escape_pending flag
        std::string sql_escape = "SELECT escape_pending FROM ships "
                                 "WHERE game_id=? AND ship_code=? AND owner=?";
        auto escape_rows = db.Query(
            sql_escape, {m_slate.game_id, ship.code, m_slate.aa_player});
        if (!escape_rows.empty() && KH_EQU(escape_rows[0][0], "1"))
        {
            ship.escape_pending = true;
        }
    }

    // Query revealed enemy orders from last committed round
    // Orders are public after both players commit
    for (AAShipInfo& enemy : m_slate.enemy_ships)
    {
        // Get most recent committed order for this enemy ship
        std::string sql_last_order =
            "SELECT tactic, power_d, power_b, power_s, power_t "
            "FROM combat_orders "
            "WHERE game_id=? AND ship_code=? AND owner!=? AND committed=1 "
            "ORDER BY round DESC LIMIT 1";
        auto order_rows = db.Query(
            sql_last_order, {m_slate.game_id, enemy.code, m_slate.aa_player});
        if (!order_rows.empty() && !order_rows[0][0].empty())
        {
            enemy.last_tactic = order_rows[0][0][0];
            enemy.last_drive = std::atoi(order_rows[0][1].c_str());
            enemy.last_beam = std::atoi(order_rows[0][2].c_str());
            enemy.last_screen = std::atoi(order_rows[0][3].c_str());
            enemy.last_tube = std::atoi(order_rows[0][4].c_str());
        }
    }

    // Compute suggested destinations for movement planning
    // Use MapGraph to find reachable waypoints toward enemy bases
    // STRATEGY: Spread ships across DIFFERENT enemy bases to maximize VP

    if (!m_slate.enemy_base_hexes.empty())
    {
        MapGraph graph(m_slate.game_id);
        graph.load_state(m_slate.aa_player);

        // Track which enemy bases are already targeted or occupied
        std::set<std::string> targeted_bases;

        // First pass: mark bases already occupied by our ships
        for (const AAShipInfo& ship : m_slate.own_ships)
        {
            for (const std::string& eb : m_slate.enemy_base_hexes)
            {
                if (KH_EQU(ship.hex_id, eb))
                {
                    targeted_bases.insert(eb);
                }
            }
        }

        for (AAShipInfo& ship : m_slate.own_ships)
        {
            // Skip ships that can't move
            if (!ship.is_warpship || ship.pd <= 0 || ship.hex_id.empty())
            {
                continue;
            }

            // Already at enemy base? No need to move
            bool at_enemy_base = false;
            for (const std::string& eb : m_slate.enemy_base_hexes)
            {
                if (KH_EQU(ship.hex_id, eb))
                {
                    at_enemy_base = true;
                    break;
                }
            }
            if (at_enemy_base)
            {
                continue;
            }

            // Find an untargeted enemy base, or fall back to first one
            std::string target;
            for (const std::string& eb : m_slate.enemy_base_hexes)
            {
                if (KH_EQU(targeted_bases.find(eb), targeted_bases.end()))
                {
                    target = eb;
                    break;
                }
            }
            // If all bases are targeted, pile onto the first one
            if (target.empty())
            {
                target = m_slate.enemy_base_hexes[0];
            }

            std::vector<std::string> full_path =
                graph.get_path(ship.hex_id, target, 100);

            if (!full_path.empty() && full_path.size() > 1)
            {
                // Truncate path to what's reachable with remaining PD
                size_t reachable = static_cast<size_t>(ship.pd);
                if (reachable >= full_path.size())
                {
                    reachable = full_path.size() - 1;
                }
                // Don't suggest staying at current hex
                if (reachable > 0 && full_path[reachable] != ship.hex_id)
                {
                    ship.suggested_destination = full_path[reachable];
                    // Mark this base as targeted so next ship goes elsewhere
                    targeted_bases.insert(target);
                }
            }
        }
    }

    // ----------------------------------
    // Economic Layer: Codex, Resources, Facilities
    // ----------------------------------

    // Codex entries (player knowledge of systems)
    std::string sql_codex =
        "SELECT system_name, knowledge_level FROM codex_entries "
        "WHERE game_id=? AND player=?";
    auto codex_rows = db.Query(sql_codex, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : codex_rows)
    {
        AACodexEntry entry;
        entry.system_name = row[0];
        entry.level = row[1];
        m_slate.codex.push_back(entry);
    }

    // Resources at systems where AI has ships
    // Collect unique system names from own ships
    std::set<std::string> ship_systems;
    for (const AAShipInfo& ship : m_slate.own_ships)
    {
        if (!ship.at_system.empty())
        {
            ship_systems.insert(ship.at_system);
        }
    }

    for (const std::string& sys : ship_systems)
    {
        // Query resources via planets
        std::string sql_res =
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty "
            "FROM system_resources sr "
            "JOIN system_planets sp ON sr.location_type='Planet' AND "
            "sr.location_id=sp.id "
            "WHERE sp.system_name=? "
            "UNION "
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty "
            "FROM system_resources sr "
            "JOIN system_asteroid_belts sb ON sr.location_type='Belt' AND "
            "sr.location_id=sb.id "
            "WHERE sb.system_name=?";

        auto res_rows = db.Query(sql_res, {sys, sys});

        for (const std::vector<std::string>& row : res_rows)
        {
            AAResourceInfo res;
            res.system = sys;
            res.type = row[0];
            res.abundance = row[1];

            // Pre-compute expected yield
            int base_yield = 1;
            if (KH_EQU(res.abundance, "Rich"))
            {
                base_yield = 16;
            }
            else if (KH_EQU(res.abundance, "High"))
            {
                base_yield = 8;
            }
            else if (KH_EQU(res.abundance, "Moderate"))
            {
                base_yield = 4;
            }
            else if (KH_EQU(res.abundance, "Low"))
            {
                base_yield = 2;
            }

            double modifier = 1.0;
            std::string diff = row[2];
            if (KH_EQU(diff, "Difficult"))
            {
                modifier = 0.4;
            }
            else if (KH_EQU(diff, "Moderate"))
            {
                modifier = 0.7;
            }
            else if (KH_EQU(diff, "Extreme"))
            {
                modifier = 0.2;
            }
            res.yield = static_cast<int>(base_yield * modifier);
            if (res.yield < 1)
            {
                res.yield = 1;
            }
            m_slate.resources.push_back(res);
        }
    }

    // Facilities (player-controlled or neutral)
    std::string sql_fac =
        "SELECT system_name, facility_type, controller FROM facility_control "
        "WHERE game_id=?";
    auto fac_rows = db.Query(sql_fac, {m_slate.game_id});

    for (const std::vector<std::string>& row : fac_rows)
    {
        AAFacilityInfo fac;
        fac.system = row[0];
        fac.type = row[1];
        fac.controller = row[2].empty() ? '\0' : row[2][0];
        m_slate.facilities.push_back(fac);
    }

    // Market prices (per-game dynamic prices, fall back to base prices)
    std::string sql_market =
        "SELECT mp.resource_type, mp.current_price, mp.base_price "
        "FROM market_prices mp WHERE mp.game_id=?";
    auto market_rows = db.Query(sql_market, {m_slate.game_id});

    if (market_rows.empty())
    {
        // Fall back to module base prices
        std::string sql_base = "SELECT resource_type, base_price, base_price "
                               "FROM market_base_prices WHERE module_id=1";
        market_rows = db.Query(sql_base, {});
    }

    for (const std::vector<std::string>& row : market_rows)
    {
        AAMarketPrice mp;
        mp.resource_type = row[0];
        mp.current_price = std::atoi(row[1].c_str());
        mp.base_price = std::atoi(row[2].c_str());
        m_slate.market_prices.push_back(mp);
    }

    // Salvageable objects at AI ship hex locations
    // Collect unique hex IDs from own ships
    std::set<std::string> ship_hexes;
    for (const AAShipInfo& ship : m_slate.own_ships)
    {
        if (!ship.hex_id.empty())
        {
            ship_hexes.insert(ship.hex_id);
        }
    }

    for (const std::string& hex : ship_hexes)
    {
        std::string sql_salvage =
            "SELECT hex_id, object_type, state, salvage_value "
            "FROM hex_objects WHERE game_id=? AND hex_id=? "
            "AND state IN ('detected','identified')";
        auto salvage_rows = db.Query(sql_salvage, {m_slate.game_id, hex});

        for (const std::vector<std::string>& row : salvage_rows)
        {
            AASalvageableInfo si;
            si.hex_id = row[0];
            si.object_type = row[1];
            si.state = row[2];
            si.salvage_value = std::atoi(row[3].c_str());
            m_slate.salvageables.push_back(si);
        }
    }

    // ----------------------------------
    // Cross-Turn Memory: Persisted Metrics
    // ----------------------------------
    std::string sql_metrics = "SELECT metric_name, metric_value, updated_round "
                              "FROM aa_metrics WHERE game_id=? AND player=?";
    auto metric_rows =
        db.Query(sql_metrics, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : metric_rows)
    {
        AAMetric met;
        met.name = row[0];
        met.value = std::atof(row[1].c_str());
        met.updatedRound = std::atoi(row[2].c_str());
        m_slate.persisted_metrics.push_back(met);
    }

    // ----------------------------------
    // BFS Distance Matrix (topology-aware)
    // ----------------------------------
    // Precompute BFS hop counts for all key hex pairs the Lisp layer needs.
    // Replaces Manhattan approximation with real pathfinding via warplines.
    {
        MapGraph dist_graph(m_slate.game_id);
        dist_graph.load_state(m_slate.aa_player);

        // Collect all base hexes for base-to-base distances
        std::vector<std::string> all_bases;
        all_bases.insert(all_bases.end(), m_slate.own_base_hexes.begin(),
                         m_slate.own_base_hexes.end());
        all_bases.insert(all_bases.end(), m_slate.enemy_base_hexes.begin(),
                         m_slate.enemy_base_hexes.end());

        // (a) Own ship -> own base (reinforcement: P1)
        // (b) Own ship -> enemy base (attack assignment: P3)
        for (const AAShipInfo& ship : m_slate.own_ships)
        {
            if (ship.hex_id.empty() || !ship.is_warpship)
            {
                continue;
            }
            for (const std::string& baseHex : all_bases)
            {
                if (KH_EQU(ship.hex_id, baseHex))
                {
                    continue;
                }
                int cost = dist_graph.get_path_cost(ship.hex_id, baseHex, 50);
                AAHexDistance entry;
                entry.fromHex = ship.hex_id;
                entry.toHex = baseHex;
                entry.cost = (cost < 0) ? 999 : cost;
                m_slate.hex_distances.push_back(entry);
            }
        }

        // (c) Enemy ship -> own base (threat detection: P2)
        for (const AAShipInfo& enemy : m_slate.enemy_ships)
        {
            if (enemy.hex_id.empty())
            {
                continue;
            }
            for (const std::string& baseHex : m_slate.own_base_hexes)
            {
                int cost = dist_graph.get_path_cost(enemy.hex_id, baseHex, 50);
                AAHexDistance entry;
                entry.fromHex = enemy.hex_id;
                entry.toHex = baseHex;
                entry.cost = (cost < 0) ? 999 : cost;
                m_slate.hex_distances.push_back(entry);
            }
        }

        // (d) Base -> base (proximity valuation: P4)
        for (size_t i = 0; i < all_bases.size(); ++i)
        {
            for (size_t j = i + 1; j < all_bases.size(); ++j)
            {
                int cost =
                    dist_graph.get_path_cost(all_bases[i], all_bases[j], 50);
                AAHexDistance entry;
                entry.fromHex = all_bases[i];
                entry.toHex = all_bases[j];
                entry.cost = (cost < 0) ? 999 : cost;
                m_slate.hex_distances.push_back(entry);
            }
        }

        // (e) Own ship -> enemy ship (force projection: P5)
        for (const AAShipInfo& own : m_slate.own_ships)
        {
            if (own.hex_id.empty() || !own.is_warpship)
            {
                continue;
            }
            for (const AAShipInfo& foe : m_slate.enemy_ships)
            {
                if (foe.hex_id.empty() || KH_EQU(own.hex_id, foe.hex_id))
                {
                    continue;
                }
                int cost = dist_graph.get_path_cost(own.hex_id, foe.hex_id, 50);
                AAHexDistance entry;
                entry.fromHex = own.hex_id;
                entry.toHex = foe.hex_id;
                entry.cost = (cost < 0) ? 999 : cost;
                m_slate.hex_distances.push_back(entry);
            }
        }
    }

    // ----------------------------------
    // Warpline Hexes (strategic chokepoints)
    // ----------------------------------
    {
        std::string sql_warpline =
            "SELECT DISTINCT hex_id FROM warpline_hexes WHERE module_id=1";
        auto warp_rows = db.Query(sql_warpline, {});

        for (const std::vector<std::string>& row : warp_rows)
        {
            if (!row.empty())
            {
                m_slate.warpline_hexes.push_back(row[0]);
            }
        }
    }

    // ----------------------------------
    // Hex Adjacency (geometric + warpline neighbors)
    // ----------------------------------
    // Provide Lisp with adjacency data for spatial reasoning:
    // retreat corridors, flanking, chokepoint detection.
    {
        MapGraph adjGraph(m_slate.game_id);

        std::set<std::string> needAdj;
        for (const AAShipInfo& ship : m_slate.own_ships)
        {
            if (!ship.hex_id.empty())
            {
                needAdj.insert(ship.hex_id);
            }
        }
        for (const AAShipInfo& foe : m_slate.enemy_ships)
        {
            if (!foe.hex_id.empty())
            {
                needAdj.insert(foe.hex_id);
            }
        }
        for (const std::string& base : m_slate.own_base_hexes)
        {
            needAdj.insert(base);
        }
        for (const std::string& base : m_slate.enemy_base_hexes)
        {
            needAdj.insert(base);
        }
        for (const std::string& wh : m_slate.warpline_hexes)
        {
            needAdj.insert(wh);
        }

        for (const std::string& hex : needAdj)
        {
            AAHexAdjacency adj;
            adj.hexId = hex;
            adj.neighbors = adjGraph.get_adjacent_hexes(hex);

            // Append warpline jump destinations
            std::vector<std::string> warpNbrs =
                adjGraph.get_warp_neighbors(hex);
            for (const std::string& wn : warpNbrs)
            {
                adj.neighbors.push_back(wn);
            }
            m_slate.hex_adjacency.push_back(adj);
        }
    }

#ifdef WANT_MORE_DEBUGGING_TELEMETRY
    // Instrumentation: distance matrix + warpline hex + adjacency summary
    Logger::instance().ai(std::format(
        "[AA-GATHER] hex_distances={} warpline_hexes={} adjacency={}",
        m_slate.hex_distances.size(), m_slate.warpline_hexes.size(),
        m_slate.hex_adjacency.size()));

    // Sample up to 3 distance entries to verify BFS costs
    for (size_t idx = 0; idx < m_slate.hex_distances.size() && idx < 3; ++idx)
    {
        const AAHexDistance& hd = m_slate.hex_distances[idx];
        Logger::instance().ai(std::format("[AA-GATHER] dist {}->{} cost={}",
                                          hd.fromHex, hd.toHex, hd.cost));
    }
#endif // WANT_MORE_DEBUGGING_TELEMETRY
}

// ----------------------------------------------------------------------------
// Debug Logging - Ship Positions and Combat State
// ----------------------------------------------------------------------------

void AutonomyAgency::log_debug_state()
{
    static const char* kPhaseNames[] = {"BUILD", "MOVE", "COMBAT", "PICKDROP",
                                        "END"};
    const char* phase_name =
        (m_slate.phase_index >= 0 && m_slate.phase_index <= 4)
            ? kPhaseNames[m_slate.phase_index]
            : "???";

    std::string msg = "[AA] Phase=" + std::string(phase_name) +
                      " Round=" + std::to_string(m_slate.round);

    // AI ships
    msg += " | AI ships:";
    if (m_slate.own_ships.empty())
    {
        msg += " (none)";
    }
    else
    {
        for (const AAShipInfo& s : m_slate.own_ships)
        {
            msg += " " + s.name + "@" + (s.hex_id.empty() ? "?" : s.hex_id);
        }
    }

    // User ships
    msg += " | User ships:";
    if (m_slate.enemy_ships.empty())
    {
        msg += " (none)";
    }
    else
    {
        for (const AAShipInfo& s : m_slate.enemy_ships)
        {
            msg += " " + s.name + "@" + (s.hex_id.empty() ? "?" : s.hex_id);
        }
    }

    // Combat state
    msg += " | Combat:";
    if (m_slate.contested_hexes.empty())
    {
        msg += " none";
    }
    else
    {
        msg += " contested=[";
        for (size_t idx = 0; idx < m_slate.contested_hexes.size(); ++idx)
        {
            if (idx > 0)
            {
                msg += ",";
            }
            msg += m_slate.contested_hexes[idx];
        }
        msg += "]";
    }

    if (!m_slate.active_combats.empty())
    {
        msg += " active=[";
        for (size_t idx = 0; idx < m_slate.active_combats.size(); ++idx)
        {
            if (idx > 0)
            {
                msg += ",";
            }
            msg += m_slate.active_combats[idx].hex_id + "/stg" +
                   std::to_string(m_slate.active_combats[idx].stage);
        }
        msg += "]";
    }

    Logger::instance().ai(msg);
}

// ----------------------------------------------------------------------------
// Calculate Step (ECL)
// ----------------------------------------------------------------------------

bool AutonomyAgency::calculate(
    std::vector<std::string>& commands_out,
    std::vector<std::pair<std::string, double>>& metrics_out)
{
    commands_out.clear();
    metrics_out.clear();

    if (!m_ecl_initialized)
    {
        commands_out.push_back("NEXT");
        return true;
    }

    bool bres = EclBridge::calculate(m_slate, commands_out, metrics_out);
    if (!bres)
    {
        Logger::instance().error(
            "[AI] ECL calculate failed - no command issued");
        commands_out.clear();
        metrics_out.clear();
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Combat Response Check
// ----------------------------------------------------------------------------

bool AutonomyAgency::combat_needs_response() const
{
    // Check if any active combat needs AI action
    // This is checked even when it's not AI's turn

    // Check for active combats needing orders (stage 0) or damage (stage 2)
    for (const AACombatHex& ch : m_slate.active_combats)
    {
        if (KH_EQU(ch.stage, 0) && !ch.ai_committed)
        {
            // Stage 0 and AI hasn't committed - need to issue orders
            return true;
        }
        if (KH_EQU(ch.stage, 2))
        {
            // Stage 2 - check if AI has pending damage
            for (const AAShipInfo& ship : m_slate.own_ships)
            {
                if (ship.pending_damage > 0)
                {
                    return true;
                }
            }
        }
    }

    // Check for ships needing escape
    for (const AAShipInfo& ship : m_slate.own_ships)
    {
        if (ship.escape_pending)
        {
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------------------
// Metric Persistence
// ----------------------------------------------------------------------------

bool AutonomyAgency::persist_metric(const std::string& name, double value,
                                    int round)
{
    std::lock_guard<std::mutex> db_lock(AIDBMutex::ai_mutex);
    DatabaseManager& db = DatabaseManager::instance();

    std::string sql =
        "INSERT INTO aa_metrics "
        "(game_id, player, metric_name, metric_value, updated_round) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE "
        "metric_value=VALUES(metric_value), "
        "updated_round=VALUES(updated_round)";

    std::string aa_str(1, m_aa_player);

    bool bres = db.Exec(sql, {m_game_id, aa_str, name, value, round});
    return bres;
}

// ----------------------------------------------------------------------------
// ECL Initialization
// ----------------------------------------------------------------------------

void AutonomyAgency::init_ecl()
{
    if (m_ecl_initialized)
    {
        return;
    }

    // Boot ECL runtime
    if (!EclBridge::boot())
    {
        return;
    }

    // Get DSL directory from config (--ai PATH)
    std::string dsl_dir = Configr::instance().get<Key::ai>();
    if (dsl_dir.empty())
    {
        dsl_dir = "dsl";
    }

    // Ensure trailing slash
    if (!dsl_dir.empty() && dsl_dir.back() != '/')
    {
        dsl_dir += '/';
    }

    // Load files in dependency order (util first, core last)
    static const char* kDslFiles[] = {"aa-util.lisp",   "aa-strategy.lisp",
                                      "aa-goals.lisp",  "aa-economic.lisp",
                                      "aa-build.lisp",  "aa-movement.lisp",
                                      "aa-combat.lisp", "aa-core.lisp"};

    for (const char* fname : kDslFiles)
    {
        std::string path = dsl_dir + fname;
        bool loaded = EclBridge::load_file(path);
        if (!loaded)
        {
            EclBridge::shutdown();
            return;
        }
    }

    m_ecl_initialized = true;
}

void AutonomyAgency::shutdown_ecl()
{
    if (m_ecl_initialized)
    {
        EclBridge::shutdown();
        m_ecl_initialized = false;
    }
}
