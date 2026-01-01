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

#include "statemachine.h"
#include "db.h"
#include "logger.h"
#include "maputil.h"
#include "telemetry.h"
#include "typedefs.h"
#include "ships.h"

bool DeployCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    DeployParams_t params;
    params.ship_code = m_ship_code;
    params.destination = m_system_name;
    
    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::DEPLOY, &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::getInstance();
    GameState s = StateMachine::getInstance().get_game_state();
    int m_game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    std::string sys =
        MapUtil::getInstance().resolve_system_name(m_game_id, m_system_name);

    if (!ship_exists(m_game_id, active_player, m_ship_code))
    {
        Logger::instance().error("Ship not found: " + m_ship_code);
        Telemetry::getInstance().write("Error: Ship not found: " + m_ship_code);
        return false;
    }

    ShipRow sh = load_ship(m_game_id, active_player, m_ship_code);
    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked; drop it before deploying: " +
                                 m_ship_code);
        Telemetry::getInstance().write("Error: Ship is racked; drop it before deploying: " +
                         m_ship_code);
        return false;
    }

    std::string hex = MapUtil::getInstance().resolve_system_hex(m_game_id, sys);
    update_ship_location(m_game_id, active_player, m_ship_code, sys, hex, "");

    // Save game state to persist changes
    StateMachine::getInstance().save_game(s);

    Logger::instance().info("Deployed " + sh.name + " - " + sh.code + " to " +
                            sys);
    Telemetry::getInstance().write("Deployed " + sh.name + " - " + sh.code + " to " + sys);

    return true;
}
