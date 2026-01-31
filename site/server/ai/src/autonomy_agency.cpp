//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "autonomy_agency.h"

#include <chrono>
#include <set>

#include "ai_command_injector.h"
#include "ai_db_mutex.h"
#include "configr.h"
#include "db.h"
#include "ecl_bridge.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "statemachine.h"

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
      m_stop_requested(false), m_pumped(false), m_ecl_initialized(false)
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

    Logger::instance().info(
        "[AA] Configured: game_id=" + std::to_string(game_id) +
        " aa_player=" + aa_player);
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

    Logger::instance().info("[AA] Started");
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
// Thread Entry
// ----------------------------------------------------------------------------

void AutonomyAgency::thread_main()
{
    MySqlThreadGuard mysql_guard;

    // Register this thread with ECL runtime
    // Required for threads not created by ECL itself
    ecl_import_current_thread(ECL_NIL, ECL_NIL);
    Logger::instance().info("[AA] Thread started (ECL thread registered)");

    while (!m_stop_requested.load())
    {
        // Wait for pump or timeout
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            bool signaled = m_cv.wait_for(
                lk, std::chrono::milliseconds(3000),
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
    Logger::instance().info("[AA] Thread exiting (ECL thread released)");
}

// ----------------------------------------------------------------------------
// Run One Cycle
// ----------------------------------------------------------------------------

void AutonomyAgency::run_cycle()
{
    ++m_cycle_counter;

    // Mealy State Machine: GATHER -> CALC -> RENDER -> TELEMETER -> WAIT -> loop
    enum MealyState
    {
        MS_GATHER,
        MS_CALC,
        MS_RENDER,
        MS_TELEMETER,
        MS_WAIT,
        MS_FINISH
    };

    bool done = false;
    MealyState state = MS_GATHER;
    std::string pending_cmd;
    int iterations = 0;
    const int kMaxIterations = 50; // Safety limit

    while (!done && !m_stop_requested.load() && iterations < kMaxIterations)
    {
        ++iterations;
        switch (state)
        {
        case MS_GATHER:
            m_slate.cycle_id = m_cycle_counter;
            gather();

            if (m_slate.game_over)
            {
                done = true;
            }
            else if (m_slate.is_aa_turn)
            {
                // AI's turn - proceed normally
                Logger::instance().info(
                    "[AA] Cycle " + std::to_string(m_cycle_counter) +
                    ": phase=" + std::to_string(m_slate.phase_index) +
                    " credits=" + std::to_string(m_slate.credits));
                state = MS_CALC;
            }
            else if (combat_needs_response())
            {
                // Not AI's turn, but combat needs AI response
                Logger::instance().info(
                    "[AA] Cycle " + std::to_string(m_cycle_counter) +
                    ": combat response needed (not our turn)");
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
            calculate(commands);

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
            Logger::instance().info("[AA] Render: " + pending_cmd);
            AICommandInjector::inject(m_game_id, m_aa_player, pending_cmd);

            // Check if terminal (hands control to other player or next phase)
            bool is_terminal =
                (pending_cmd == "NEXT" || pending_cmd == "DONE" ||
                 pending_cmd.rfind("NEXT", 0) == 0 ||
                 pending_cmd.rfind("DONE", 0) == 0);

            if (is_terminal)
            {
                done = true;
            }
            else
            {
                state = MS_TELEMETER;
            }
            pending_cmd.clear();
        }
        break;

        case MS_TELEMETER:
            // Currently no-op; commands handle their own logging
            state = MS_WAIT;
            break;

        case MS_WAIT:
            // Let TaskRunner process command and DB update propagate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++m_cycle_counter;
            state = MS_GATHER; // Back to top with fresh state
            break;

        case MS_FINISH:
        default:
            done = true;
            break;
        }
    }

    if (iterations >= kMaxIterations)
    {
        Logger::instance().debug("[AA] Safety limit reached (" +
                                std::to_string(kMaxIterations) +
                                " iterations) - forcing exit");
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

    m_slate.is_aa_turn = (m_slate.active_player == m_slate.aa_player);

    if (m_slate.aa_player == 'A')
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
    // Query star_systems (module template) since base_stars is per-game dynamic
    // Use home_side to determine which bases belong to AI player
    // If home_side not set yet (no deploy), default to opposite of human's
    // likely choice
    std::string our_side = m_slate.home_side;
    if (our_side.empty())
    {
        // AI is player B, default to map side B (human player A gets side A)
        our_side = (m_slate.aa_player == 'B') ? "B" : "A";
    }

    std::string sql_bases = "SELECT hex_id FROM star_systems "
                            "WHERE module_id=1 AND is_base=1 AND base_side=?";

    auto base_rows = db.Query(sql_bases, {our_side});

    for (const std::vector<std::string>& row : base_rows)
    {
        if (!row.empty())
        {
            m_slate.own_base_hexes.push_back(row[0]);
        }
    }

    // Enemy base hexes (opposite side)
    std::string enemy_side = (our_side == "A") ? "B" : "A";
    auto enemy_base_rows = db.Query(sql_bases, {enemy_side});

    for (const std::vector<std::string>& row : enemy_base_rows)
    {
        if (!row.empty())
        {
            m_slate.enemy_base_hexes.push_back(row[0]);
        }
    }

    // Own ships
    // Query pd_spent to compute remaining PD for this turn
    std::string sql_own =
        "SELECT ship_code, ship_name, at_hex, at_system, pd, beam, screen, "
        "tube, missiles, sr, tech_level, ship_type, COALESCE(pd_spent,0) "
        "FROM ships WHERE game_id=? AND owner=? AND destroyed_at IS NULL";

    auto own_rows = db.Query(sql_own, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : own_rows)
    {
        if (row.size() >= 13)
        {
            AAShipInfo ship;
            ship.code = row[0];
            ship.name = row[1];
            ship.hex_id = row[2];

            // If at_hex is empty, resolve from at_system
            if (ship.hex_id.empty() && !row[3].empty())
            {
                ship.hex_id =
                    MapUtil::instance().resolve_system_hex(m_slate.game_id,
                                                           row[3]);
            }

            // Compute remaining PD (base PD minus spent this turn)
            int base_pd = std::atoi(row[4].c_str());
            int pd_spent = std::atoi(row[12].c_str());
            ship.pd = base_pd - pd_spent;
            ship.beam = std::atoi(row[5].c_str());
            ship.screen = std::atoi(row[6].c_str());
            ship.tube = std::atoi(row[7].c_str());
            ship.missile = std::atoi(row[8].c_str());
            ship.sr = std::atoi(row[9].c_str());
            ship.tech_level = std::atoi(row[10].c_str());
            ship.is_warpship = (row[11] == "W");
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
        if (row.size() >= 12)
        {
            AAShipInfo ship;
            ship.code = row[0];
            ship.name = row[1];
            ship.hex_id = row[2];

            // If at_hex is empty, resolve from at_system
            if (ship.hex_id.empty() && !row[3].empty())
            {
                ship.hex_id =
                    MapUtil::instance().resolve_system_hex(m_slate.game_id,
                                                           row[3]);
            }

            ship.pd = std::atoi(row[4].c_str());
            ship.beam = std::atoi(row[5].c_str());
            ship.screen = std::atoi(row[6].c_str());
            ship.tube = std::atoi(row[7].c_str());
            ship.missile = std::atoi(row[8].c_str());
            ship.sr = std::atoi(row[9].c_str());
            ship.tech_level = std::atoi(row[10].c_str());
            ship.is_warpship = (row[11] == "W");
            m_slate.enemy_ships.push_back(ship);
        }
    }

    // Draft ships (from drafts table)
    std::string sql_drafts =
        "SELECT ship_code, ship_name, pd, beam, screen, tube, missiles, sr, "
        "ship_type FROM drafts WHERE game_id=? AND owner=?";

    auto draft_rows =
        db.Query(sql_drafts, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : draft_rows)
    {
        if (row.size() >= 9)
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
            ship.is_warpship = (row[8] == "W");
            m_slate.draft_ships.push_back(ship);
        }
    }

    // Contested hexes (hexes with ships from both players)
    std::string sql_contested =
        "SELECT at_hex FROM ships WHERE game_id=? AND destroyed_at IS NULL "
        "AND at_hex IS NOT NULL GROUP BY at_hex HAVING COUNT(DISTINCT owner) > "
        "1";

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

    // Get active combats
    std::string sql_combats =
        "SELECT hex_id, stage, round FROM combat_state WHERE game_id=?";
    auto combat_rows = db.Query(sql_combats, {m_slate.game_id});

    for (const std::vector<std::string>& row : combat_rows)
    {
        if (row.size() >= 3)
        {
            AACombatHex ch;
            ch.hex_id = row[0];
            ch.stage = std::atoi(row[1].c_str());
            ch.round = std::atoi(row[2].c_str());

            // Check if AI has committed orders for this round
            std::string sql_committed =
                "SELECT COUNT(*) FROM combat_orders "
                "WHERE game_id=? AND owner=? AND round=? AND committed=1";
            auto commit_rows = db.Query(
                sql_committed, {m_slate.game_id, m_slate.aa_player, ch.round});
            ch.ai_committed = (!commit_rows.empty() &&
                               std::atoi(commit_rows[0][0].c_str()) > 0);

            m_slate.active_combats.push_back(ch);
            Logger::instance().info("[AA] Combat at " + ch.hex_id +
                                    " stage=" + std::to_string(ch.stage) +
                                    " round=" + std::to_string(ch.round) +
                                    " ai_committed=" +
                                    (ch.ai_committed ? "Y" : "N"));
        }
    }

    // For AI ships in combat hexes, check if they need orders or damage
    for (AAShipInfo& ship : m_slate.own_ships)
    {
        for (const AACombatHex& ch : m_slate.active_combats)
        {
            if (ship.hex_id == ch.hex_id)
            {
                // Stage 0: Check if this ship has a committed order
                if (ch.stage == 0)
                {
                    std::string sql_has_order =
                        "SELECT committed FROM combat_orders "
                        "WHERE game_id=? AND owner=? AND ship_code=? AND round=?";
                    auto order_rows = db.Query(
                        sql_has_order,
                        {m_slate.game_id, m_slate.aa_player, ship.code, ch.round});
                    if (order_rows.empty() || order_rows[0][0] != "1")
                    {
                        ship.needs_combat_order = true;
                    }
                }

                // Stage 2: Check pending damage
                if (ch.stage == 2)
                {
                    std::string sql_pending =
                        "SELECT damage_amount FROM pending_damage "
                        "WHERE game_id=? AND hex_id=? AND ship_code=? AND owner=?";
                    auto dmg_rows = db.Query(
                        sql_pending,
                        {m_slate.game_id, ch.hex_id, ship.code, m_slate.aa_player});
                    if (!dmg_rows.empty())
                    {
                        ship.pending_damage = std::atoi(dmg_rows[0][0].c_str());
                    }
                }
            }
        }

        // Check escape_pending flag
        std::string sql_escape =
            "SELECT escape_pending FROM ships "
            "WHERE game_id=? AND ship_code=? AND owner=?";
        auto escape_rows = db.Query(
            sql_escape, {m_slate.game_id, ship.code, m_slate.aa_player});
        if (!escape_rows.empty() && escape_rows[0][0] == "1")
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
            "SELECT tactic, power_d, power_b FROM combat_orders "
            "WHERE game_id=? AND ship_code=? AND owner!=? AND committed=1 "
            "ORDER BY round DESC LIMIT 1";
        char enemy_owner = (m_slate.aa_player == 'A') ? 'B' : 'A';
        auto order_rows = db.Query(
            sql_last_order, {m_slate.game_id, enemy.code, m_slate.aa_player});
        if (!order_rows.empty() && !order_rows[0][0].empty())
        {
            enemy.last_tactic = order_rows[0][0][0];
            enemy.last_drive = std::atoi(order_rows[0][1].c_str());
            enemy.last_beam = std::atoi(order_rows[0][2].c_str());
        }
    }

    // Compute suggested destinations for movement planning
    // Use MapGraph to find reachable waypoints toward enemy bases
    // STRATEGY: Spread ships across DIFFERENT enemy bases to maximize VP
    Logger::instance().info("[AA] enemy_bases=" +
                            std::to_string(m_slate.enemy_base_hexes.size()) +
                            " own_ships=" +
                            std::to_string(m_slate.own_ships.size()));

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
                if (ship.hex_id == eb)
                {
                    targeted_bases.insert(eb);
                }
            }
        }

        for (AAShipInfo& ship : m_slate.own_ships)
        {
            Logger::instance().info("[AA] Ship " + ship.name + " hex=" +
                                    ship.hex_id + " remaining_pd=" +
                                    std::to_string(ship.pd) + " warp=" +
                                    std::to_string(ship.is_warpship));

            // Skip ships that can't move
            if (!ship.is_warpship || ship.pd <= 0 || ship.hex_id.empty())
            {
                Logger::instance().info("[AA] -> skip (can't move)");
                continue;
            }

            // Already at enemy base? No need to move
            bool at_enemy_base = false;
            for (const std::string& eb : m_slate.enemy_base_hexes)
            {
                if (ship.hex_id == eb)
                {
                    at_enemy_base = true;
                    break;
                }
            }
            if (at_enemy_base)
            {
                Logger::instance().info("[AA] -> skip (at enemy base)");
                continue;
            }

            // Find an untargeted enemy base, or fall back to first one
            std::string target;
            for (const std::string& eb : m_slate.enemy_base_hexes)
            {
                if (targeted_bases.find(eb) == targeted_bases.end())
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

            Logger::instance().info("[AA] -> full path from " + ship.hex_id +
                                    " to " + target + " len=" +
                                    std::to_string(full_path.size()));

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
                    Logger::instance().info("[AA] -> suggested (remaining_pd=" +
                                            std::to_string(ship.pd) + "): " +
                                            ship.suggested_destination +
                                            " (target base: " + target + ")");
                }
                else
                {
                    Logger::instance().info("[AA] -> skip (no PD or at dest)");
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Calculate Step (ECL)
// ----------------------------------------------------------------------------

void AutonomyAgency::calculate(std::vector<std::string>& commands_out)
{
    commands_out.clear();

    if (!m_ecl_initialized)
    {
        Logger::instance().error("[AA] Calculate: ECL not initialized");
        commands_out.push_back("NEXT");
        return;
    }

    bool ok = EclBridge::calculate(m_slate, commands_out);
    if (!ok)
    {
        Logger::instance().error(
            "[AA] Calculate: ECL call failed, defaulting to NEXT");
        commands_out.clear();
        commands_out.push_back("NEXT");
    }
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
        if (ch.stage == 0 && !ch.ai_committed)
        {
            // Stage 0 and AI hasn't committed - need to issue orders
            return true;
        }
        if (ch.stage == 2)
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
        Logger::instance().error("[AA] Failed to boot ECL");
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

    Logger::instance().info("[AA] Loading DSL from: " + dsl_dir);

    // Load files in dependency order (util first, core last)
    static const char* kDslFiles[] = {"aa-util.lisp", "aa-build.lisp",
                                      "aa-movement.lisp", "aa-combat.lisp",
                                      "aa-core.lisp"};

    for (const char* fname : kDslFiles)
    {
        std::string path = dsl_dir + fname;
        bool loaded = EclBridge::load_file(path);
        if (!loaded)
        {
            Logger::instance().error("[AA] Failed to load " + path);
            EclBridge::shutdown();
            return;
        }
    }

    m_ecl_initialized = true;
    Logger::instance().info("[AA] ECL initialized");
}

void AutonomyAgency::shutdown_ecl()
{
    if (m_ecl_initialized)
    {
        EclBridge::shutdown();
        m_ecl_initialized = false;
        Logger::instance().info("[AA] ECL shutdown");
    }
}
