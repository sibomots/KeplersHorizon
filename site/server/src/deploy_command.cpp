//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "deploy_command.h"

#include <cctype>
#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

// Utility: Convert string to uppercase
static std::string upper_ascii(const std::string &s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        r[i] = (char)std::toupper((unsigned char)r[i]);
    return r;
}

// Utility: Resolve system name (case-insensitive lookup)
static std::string resolve_system_name(Db *db, int game_id,
                                       const std::string &user_supplied)
{
    std::string u = upper_ascii(user_supplied);
    auto r = db->query("SELECT name FROM star_systems WHERE game_id=" +
                       std::to_string(game_id) + " AND UPPER(name)='" +
                       db->esc(u) + "' LIMIT 1");
    if (!r.empty() && !r[0].empty())
        return r[0][0];
    return u;
}

// Utility: Get hex ID for a system
static std::string resolve_system_hex(Db *db, int game_id,
                                      const std::string &canon_name)
{
    std::ostringstream q;
    q << "SELECT hex_id FROM star_systems WHERE game_id=" << game_id
      << " AND name='" << db->esc(canon_name) << "' LIMIT 1";
    auto r = db->query(q.str());
    if (r.empty())
    {
        return "";
    }
    return r[0][0];
}

DeployCommand::Builder::Builder(StateMachine &sm) : m_sm(sm)
{
}

DeployCommand::Builder &
DeployCommand::Builder::ship_code(const std::string &code)
{
    m_ship_code = code;
    return *this;
}

DeployCommand::Builder &
DeployCommand::Builder::system_name(const std::string &sys)
{
    m_system_name = sys;
    return *this;
}

ICmd *DeployCommand::Builder::build()
{
    return new DeployCommand(m_sm, m_ship_code, m_system_name);
}

DeployCommand::DeployCommand(StateMachine &sm, const std::string &ship_code,
                             const std::string &system_name)
    : m_sm(sm), m_ship_code(ship_code), m_system_name(system_name)
{
}

bool DeployCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    Db *m_db = m_sm.get_db();
    int m_game_id = m_sm.get_game_id();

    std::string sys = resolve_system_name(m_db, m_game_id, m_system_name);

    if (!ship_exists(m_db, m_game_id, active_player, m_ship_code))
    {
        Logger::instance().error("Ship not found: " + m_ship_code);
        Telemetry::write("Error: Ship not found: " + m_ship_code);
        return false;
    }

    ShipRow sh = load_ship(m_db, m_game_id, active_player, m_ship_code);
    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked; drop it before deploying: " +
                                 m_ship_code);
        Telemetry::write("Error: Ship is racked; drop it before deploying: " +
                         m_ship_code);
        return false;
    }

    std::string hex = resolve_system_hex(m_db, m_game_id, sys);
    update_ship_location(m_db, m_game_id, active_player, m_ship_code, sys, hex,
                         "");

    // Save game state to persist changes
    save_game(m_db, s);

    Logger::instance().info("Deployed " + sh.name + " - " + sh.code + " to " +
                            sys);
    Telemetry::write("Deployed " + sh.name + " - " + sh.code + " to " + sys);

    return true;
}
