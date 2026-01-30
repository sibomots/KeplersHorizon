//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "autonomy_agency.h"

#include <chrono>

#include "ai_command_injector.h"
#include "ai_db_mutex.h"
#include "configr.h"
#include "db.h"
#include "ecl_bridge.h"
#include "logger.h"
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
    Logger::instance().info("[AA] Thread started");

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

    Logger::instance().info("[AA] Thread exiting");
}

// ----------------------------------------------------------------------------
// Run One Cycle
// ----------------------------------------------------------------------------

void AutonomyAgency::run_cycle()
{
    ++m_cycle_counter;
    m_slate.cycle_id = m_cycle_counter;

    // Step 1: Gather
    gather();

    // Guard: Only act if it's our turn
    if (!m_slate.is_aa_turn)
    {
        return;
    }

    // Guard: Don't act if game is over
    if (m_slate.game_over)
    {
        return;
    }

    Logger::instance().info("[AA] Cycle " + std::to_string(m_cycle_counter) +
                            ": phase=" + std::to_string(m_slate.phase_index) +
                            " credits=" + std::to_string(m_slate.credits));

    // Step 2: Calculate
    std::vector<std::string> commands;
    calculate(commands);

    // Step 3: Render
    if (!commands.empty())
    {
        render(commands);
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
    std::string sql_bases =
    "SELECT hex_id FROM base_stars WHERE game_id=? AND owner=?";

    auto base_rows = db.Query(sql_bases, { m_slate.game_id, m_slate.aa_player });

    for (const std::vector<std::string>& row : base_rows)
    {
        if (!row.empty())
        {
            m_slate.own_base_hexes.push_back(row[0]);
        }
    }

    // Own ships
    std::string sql_own =
        "SELECT ship_code, ship_name, at_hex, pd, beam, screen, tube, "
        "missiles, sr, tech_level, ship_type "
        "FROM ships WHERE game_id=? AND owner=? AND destroyed_at IS NULL";

    auto own_rows = db.Query(sql_own, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : own_rows)
    {
        if (row.size() >= 11)
        {
            AAShipInfo ship;
            ship.code = row[0];
            ship.name = row[1];
            ship.hex_id = row[2];
            ship.pd = std::atoi(row[3].c_str());
            ship.beam = std::atoi(row[4].c_str());
            ship.screen = std::atoi(row[5].c_str());
            ship.tube = std::atoi(row[6].c_str());
            ship.missile = std::atoi(row[7].c_str());
            ship.sr = std::atoi(row[8].c_str());
            ship.tech_level = std::atoi(row[9].c_str());
            ship.is_warpship = (row[10] == "W");
            m_slate.own_ships.push_back(ship);
        }
    }

    // Enemy ships
    std::string sql_enemy =
        "SELECT ship_code, ship_name, at_hex, pd, beam, screen, tube, "
        "missiles, sr, tech_level, ship_type "
        "FROM ships WHERE game_id=? AND owner!=? AND destroyed_at IS NULL";

    auto enemy_rows = db.Query(sql_enemy, {m_slate.game_id, m_slate.aa_player});

    for (const std::vector<std::string>& row : enemy_rows)
    {
        if (row.size() >= 11)
        {
            AAShipInfo ship;
            ship.code = row[0];
            ship.name = row[1];
            ship.hex_id = row[2];
            ship.pd = std::atoi(row[3].c_str());
            ship.beam = std::atoi(row[4].c_str());
            ship.screen = std::atoi(row[5].c_str());
            ship.tube = std::atoi(row[6].c_str());
            ship.missile = std::atoi(row[7].c_str());
            ship.sr = std::atoi(row[8].c_str());
            ship.tech_level = std::atoi(row[9].c_str());
            ship.is_warpship = (row[10] == "W");
            m_slate.enemy_ships.push_back(ship);
        }
    }

    // Draft ships (from drafts table)
    std::string sql_drafts =
        "SELECT ship_code, ship_name, pd, beam, screen, tube, missiles, sr, "
        "ship_type FROM drafts WHERE game_id=? AND owner=?";

    auto draft_rows = db.Query(sql_drafts, {m_slate.game_id, m_slate.aa_player});

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
        "AND at_hex IS NOT NULL GROUP BY at_hex HAVING COUNT(DISTINCT owner) > 1";

    auto contested_rows = db.Query(sql_contested, {m_slate.game_id});

    for (const std::vector<std::string>& row : contested_rows)
    {
        if (!row.empty())
        {
            m_slate.contested_hexes.push_back(row[0]);
        }
    }

    m_slate.in_combat = !m_slate.contested_hexes.empty();
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
// Render Step (C++)
// ----------------------------------------------------------------------------

void AutonomyAgency::render(const std::vector<std::string>& commands)
{
    for (const std::string& cmd : commands)
    {
        Logger::instance().info("[AA] Render: " + cmd);

        bool ok = AICommandInjector::inject(m_game_id, m_aa_player, cmd);
        if (!ok)
        {
            Logger::instance().error("[AA] Inject failed: " + cmd);
            // Stop on first failure
            break;
        }
    }
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
    static const char* kDslFiles[] = {
        "aa-util.lisp",
        "aa-build.lisp",
        "aa-movement.lisp",
        "aa-combat.lisp",
        "aa-core.lisp"
    };

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
