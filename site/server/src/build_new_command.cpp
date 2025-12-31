//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_new_command.h"

#include <cctype>
#include <sstream>

#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "ships.h"

bool BuildNewCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Check BP availability
    int& bp = (s.active_player == "A") ? s.bpA : s.bpB;
    if (bp <= 0)
    {
        Logger::instance().error("No Build Points available");
        Telemetry::getInstance().write("Error: No Build Points available");
        return false;
    }

    // Validate ship code format
    if (m_ship_code.empty() || m_ship_code.length() > 10)
    {
        Logger::instance().error("Invalid ship code format");
        Telemetry::getInstance().write("Error: Invalid ship code format");
        return false;
    }

    // Auto-assign ship number if not provided
    std::string ship_code = m_ship_code;
    if (ship_code.find_first_of("0123456789") == std::string::npos)
    {
        int next_num = 1;
        std::string candidate;
        do
        {
            candidate = ship_code + std::to_string(next_num);
            next_num++;
        } while (ship_exists(s.game_id, active_player, candidate) ||
                 draft_exists(s.game_id, active_player, candidate));
        ship_code = candidate;
    }

    // Check for duplicates
    if (draft_exists(s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Draft already exists: " + ship_code);
        Telemetry::getInstance().write("Error: Draft already exists: " + ship_code);
        return false;
    }

    if (ship_exists(s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Ship already exists: " + ship_code);
        Telemetry::getInstance().write("Error: Ship already exists: " + ship_code);
        return false;
    }

    // Create draft
    DraftRow draft;
    draft.code = ship_code;
    draft.name = m_ship_name;
    draft.attr.type = 'W'; // Default to warship

    insert_draft(s.game_id, active_player, draft);
    set_current_draft(s.game_id, active_player, ship_code);

    Logger::instance().info("Draft created: " + m_ship_name + " - " +
                            ship_code);
    Telemetry::getInstance().write("Draft created: " + m_ship_name + " - " + ship_code +
                     " (current)");
    Telemetry::getInstance().write("Use: build set PD|B|S|T|M|SR <n>");

    return true;
}
