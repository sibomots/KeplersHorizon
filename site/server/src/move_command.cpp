//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "move_command.h"

#include <cctype>
#include <sstream>

#include "typedefs.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "statemachine.h"
#include "telemetry.h"
#include "ships.h"

bool MoveCommand::invoke(void)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    GameState s = StateMachine::getInstance().get_game_state();
    int m_game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    if (!ship_exists(m_game_id, active_player, m_ship_code))
    {
        Logger::instance().error("Ship not found: " + m_ship_code);
        Telemetry::getInstance().write("Error: Ship not found: " + m_ship_code);
        return false;
    }

    ShipRow sh = load_ship(m_game_id, active_player, m_ship_code);

    if (sh.attr.type != 'W')
    {
        Logger::instance().error("Only Warpships can move");
        Telemetry::getInstance().write("Error: Only Warpships can move");
        return false;
    }

    if (sh.attr.PD <= 0)
    {
        Logger::instance().error("Ship has PD=0 and cannot move");
        Telemetry::getInstance().write("Error: Ship has PD=0 and cannot move");
        return false;
    }

    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked and cannot move: " +
                                 sh.racked_in);
        Telemetry::getInstance().write("Error: Ship is racked and cannot move: " +
                         sh.racked_in);
        return false;
    }

    std::string startHex = sh.at_hex;
    if (startHex.empty() && !sh.at_system.empty())
    {
        startHex =
            MapUtil::getInstance().resolve_system_hex(m_game_id, sh.at_system);
    }

    if (startHex.empty())
    {
        Logger::instance().error("Ship is not deployed");
        Telemetry::getInstance().write("Error: Ship is not deployed");
        return false;
    }

    // ============================================================================
    // PATHFINDING ALGORITHM - PRESERVED EXACTLY FROM LEGACY CODE
    // ============================================================================
    MapGraph graph(m_game_id);
    graph.load_state(active_player);

    // Process multi-step path
    std::string currentHex = startHex;
    int totalCost = 0;
    int allowance = sh.attr.PD - sh.pd_spent;
    std::string finalSystem = sh.at_system;
    std::string finalHex = startHex;
    std::string errorMsg;

    if (allowance <= 0)
    {
        Logger::instance().error("Ship has no movement remaining (PD spent)");
        Telemetry::getInstance().write("Error: Ship has no movement remaining (PD spent)");
        return false;
    }

    for (size_t i = 0; i < m_destinations.size(); ++i)
    {
        std::string destTok = m_destinations[i];
        std::string stepHex = graph.resolve_hex(destTok);
        std::string stepSys;

        if (!stepHex.empty())
        {
            // See if it matches a system name (reverse lookup for
            // display/logic)
            auto sysr = db.query("SELECT name FROM star_systems WHERE "
                                 "game_id=" +
                                 std::to_string(m_game_id) + " AND hex_id='" +
                                 stepHex + "' LIMIT 1");
            if (!sysr.empty())
                stepSys = sysr[0][0];
        }
        else
        {
            errorMsg = "Unknown destination: " + destTok;
            break;
        }

        int stepCost =
            graph.get_path_cost(currentHex, stepHex, allowance - totalCost);
        if (stepCost == -1)
        {
            int needed = graph.get_path_cost(currentHex, stepHex, 999);
            if (needed != -1)
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           ". Needed " + std::to_string(needed) +
                           " PD, but limit is " +
                           std::to_string(allowance - totalCost) + ".";
            }
            else
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           " (No path or Blocked).";
            }
            break;
        }

        totalCost += stepCost;
        if (totalCost > allowance)
        {
            errorMsg = "Path exceeds PD allowance. "
                       "Total cost would be " +
                       std::to_string(totalCost) +
                       ", remaining=" + std::to_string(allowance);
            break;
        }

        currentHex = stepHex;
        finalHex = stepHex;
        finalSystem = stepSys;
    }
    // ============================================================================
    // END PATHFINDING ALGORITHM
    // ============================================================================

    if (!errorMsg.empty())
    {
        Logger::instance().error(errorMsg);
        Telemetry::getInstance().write("Error: " + errorMsg);
        return false;
    }

    // Update ship location and PD spent
    if (finalSystem.empty())
        finalSystem = "";
    update_ship_location(m_game_id, active_player, sh.code, finalSystem,
                         finalHex, "");
    db.exec("UPDATE ships SET pd_spent=pd_spent+" + std::to_string(totalCost) +
            " WHERE game_id=" + std::to_string(m_game_id) + " AND owner='" +
            std::string(1, active_player) + "'" + " AND ship_code='" +
            db.esc(sh.code) + "'");

    // Save game state to persist changes
    StateMachine::getInstance().save_game(s);

    std::ostringstream o;
    o << "Moved " << sh.name << " - " << sh.code << " to "
      << (finalSystem.empty() ? finalHex : finalSystem) << " (" << finalHex
      << ") cost " << totalCost << " PD";

    Logger::instance().info(o.str());
    Telemetry::getInstance().write(o.str());

    return true;
}
