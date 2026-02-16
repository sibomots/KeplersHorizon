///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "autonomy_agency.h"
#include "configure_command.h"
#include "db.h"
#include "gamedev_state.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

#include <format>
#include <fstream>
#include <sstream>
#include <string>

bool ConfigureCommand::invoke(void)
{
    bool bres = false;
    if (!check_privilege())
    {
        Telemetry::instance().write(
            "CONFIGURE: Access denied. Administrator privilege required.");
        bres = false;
    }
    else
    {
        switch (m_mode)
        {
        case ConfigureMode::CFG_SHOW:
            bres = do_show();
            break;
        case ConfigureMode::CFG_RELOAD_CONF:
            bres = do_reload_conf();
            break;
        case ConfigureMode::CFG_RELOAD_AI:
            bres = do_reload_ai();
            break;
        }
    }
    return bres;
}

bool ConfigureCommand::check_privilege()
{
    bool bres = false;
    int userId = StateMachine::instance().get_current_user_id();
    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.Query("SELECT is_admin FROM users WHERE id=?", {userId});

    if (!rows.empty() && KH_EQU(rows[0][0], "1"))
    {
        bres = true;
    }

    return bres;
}

bool ConfigureCommand::do_show()
{
    bool bres = true;
    GameDevState& gds = GameDevState::instance();
    std::ostringstream out;

    out << "CONFIGURE: Current Settings\n";
    out << "───────────────────────────────────────\n";

    std::string status;
    gds.get_status(status);
    out << status;

    out << "───────────────────────────────────────\n";
    out << "Subcommands:\n";
    out << "  configure reload conf  — Reload kh.conf\n";
    out << "  configure reload ai    — Reload AI DSL\n";

    Telemetry::instance().write(out.str());
    return bres;
}

bool ConfigureCommand::do_reload_conf()
{
    bool bres = false;
    GameDevState& gds = GameDevState::instance();

    // Look for kh.conf relative to server working directory
    std::ifstream confFile("kh.conf");
    if (!confFile.is_open())
    {
        Telemetry::instance().write("CONFIGURE: Cannot open kh.conf. "
                                    "File not found in server directory.");
        return false;
    }

    std::ostringstream out;
    out << "CONFIGURE: Loading kh.conf\n";

    std::string line;
    int loaded = 0;
    while (std::getline(confFile, line))
    {
        // Skip comments and blank lines
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#')
        {
            continue;
        }

        // Parse key = value
        size_t eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(start, eq - start);
        std::string val = line.substr(eq + 1);

        // Trim whitespace from key and value
        auto trim = [](std::string& s)
        {
            size_t a = s.find_first_not_of(" \t");
            size_t b = s.find_last_not_of(" \t");
            if (a == std::string::npos)
            {
                s.clear();
            }
            else
            {
                s = s.substr(a, b - a + 1);
            }
        };
        trim(key);
        trim(val);

        int intVal = std::atoi(val.c_str());

        if (key == "override.combat_modifier")
        {
            gds.set_combat_modifier(intVal);
            out << std::format("  combat_modifier = {}\n", intVal);
            ++loaded;
        }
        else if (key == "override.movement_modifier")
        {
            gds.set_movement_modifier(intVal);
            out << std::format("  movement_modifier = {}\n", intVal);
            ++loaded;
        }
        else if (key == "override.environment_chance")
        {
            gds.set_environment_chance(intVal);
            out << std::format("  environment_chance = {}%\n", intVal);
            ++loaded;
        }
        else if (key == "override.force_crt")
        {
            gds.set_force_crt(intVal);
            out << std::format("  force_crt = {}\n", intVal);
            ++loaded;
        }
        else if (key.rfind("event.", 0) == 0)
        {
            // Store event probabilities in the database for runtime use
            DatabaseManager& db = DatabaseManager::instance();
            int gameId = StateMachine::instance().get_game_id();
            std::string eventKey = key.substr(6); // strip "event."

            db.Exec(
                "INSERT INTO game_config(game_id, config_key, config_value) "
                "VALUES(?,?,?) ON DUPLICATE KEY UPDATE config_value=?",
                {gameId, eventKey, val, val});

            out << std::format("  {} = {}\n", key, val);
            ++loaded;
        }
    }

    if (loaded > 0)
    {
        gds.enable(true);
    }

    out << std::format("CONFIGURE: Loaded {} settings.\n", loaded);
    Telemetry::instance().write(out.str());
    Logger::instance().info(
        std::format("[CONFIGURE] Reloaded kh.conf ({} settings)", loaded));

    bres = true;
    return bres;
}

bool ConfigureCommand::do_reload_ai()
{
    bool bres = false;
    std::ostringstream out;

    out << "CONFIGURE: Reloading AI DSL...\n";

    AutonomyAgency& aa = AutonomyAgency::instance();
    aa.shutdown_ecl();
    aa.init_ecl();

    out << "CONFIGURE: AI DSL reloaded.\n";

    Telemetry::instance().write(out.str());
    Logger::instance().info("[CONFIGURE] AI DSL reloaded by admin");

    bres = true;
    return bres;
}
