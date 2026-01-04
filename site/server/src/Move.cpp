//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <sstream>

#include "combat.h"
#include "db.h"
#include "deploy_command.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "move_command.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool DeployCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    DeployParams_t params;
    params.ship_code = m_ship_code;
    params.destination = m_system_name;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::DEPLOY, &params,
                                                    inhibit_error))
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
        Telemetry::getInstance().write("FLEET REGISTRY: Vessel " + m_ship_code +
                                       " is not in your fleet!");
        return false;
    }

    ShipRow sh = load_ship(m_game_id, active_player, m_ship_code);
    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked; drop it before deploying: " +
                                 m_ship_code);
        Telemetry::getInstance().write(
            "Error: Ship is racked; drop it before deploying: " + m_ship_code);
        return false;
    }

    std::string hex = MapUtil::getInstance().resolve_system_hex(m_game_id, sys);
    update_ship_location(m_game_id, active_player, m_ship_code, sys, hex, "");

    // Save game state to persist changes
    StateMachine::getInstance().save_game(s);

    Logger::instance().info("Deployed " + sh.name + " - " + sh.code + " to " +
                            sys);
    Telemetry::getInstance().write("FLEET COMMAND: " + sh.name + " (" +
                                   sh.code + ") deployed to " + sys);

    return true;
}

bool MoveCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    MoveParams_t params;
    params.ship_code = m_ship_code;
    params.destination = m_destinations.empty() ? "" : m_destinations[0];

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::MOVE, &params,
                                                    inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::getInstance();
    GameState s = StateMachine::getInstance().get_game_state();
    int m_game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    if (!ship_exists(m_game_id, active_player, m_ship_code))
    {
        Logger::instance().error("Ship not found: " + m_ship_code);
        Telemetry::getInstance().write("FLEET REGISTRY: Vessel " + m_ship_code +
                                       " is not in your fleet!");
        return false;
    }

    ShipRow sh = load_ship(m_game_id, active_player, m_ship_code);

    if (sh.attr.type != 'W')
    {
        Logger::instance().error("Only Warpships can move");
        Telemetry::getInstance().write(
            "NAV: Only WarpShip class vessels can engage hyperdrive.");
        return false;
    }

    if (sh.attr.PD <= 0)
    {
        Logger::instance().error("Ship has PD=0 and cannot move");
        Telemetry::getInstance().write(
            "NAV: " + sh.name +
            " has no power drive capacity. Unable to maneuver.");
        return false;
    }

    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked and cannot move: " +
                                 sh.racked_in);
        Telemetry::getInstance().write(
            "Error: Ship is racked and cannot move: " + sh.racked_in);
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
        Telemetry::getInstance().write(
            "NAV: " + sh.name +
            " is not deployed. Ship must be in-theater to move.");
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
        Telemetry::getInstance().write(
            "NAV: " + sh.name + " has exhausted power drive for this turn.");
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
            // display/logic) - star_systems uses map_id, not game_id
            auto sysr = db.query("SELECT name FROM star_systems WHERE "
                                 "map_id=1 AND hex_id='" +
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
    o << "NAV: " << sh.name << " (" << sh.code << ") transit to "
      << (finalSystem.empty() ? finalHex : finalSystem) << " [" << finalHex
      << "] complete. Power expended: " << totalCost << " PD";

    Logger::instance().info(o.str());
    Telemetry::getInstance().write(o.str());

    // Check for combat trigger: enemy ships in destination hex
    char enemy = (active_player == 'A') ? 'B' : 'A';
    auto enemy_ships =
        db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                 std::to_string(m_game_id) + " AND owner='" +
                 std::string(1, enemy) + "' AND at_hex='" + db.esc(finalHex) +
                 "' AND racked_in IS NULL AND destroyed_at IS NULL");

    if (!enemy_ships.empty() && std::atoi(enemy_ships[0][0].c_str()) > 0)
    {
        // Enemy ships present - trigger combat check
        CombatEngine ce(m_game_id);
        ce.check_for_combat_triggers();  // Creates combat if not exists

        Logger::instance().info("[MOVE] Contact - enemy ships in " + finalHex);

        // Notify both players
        std::string sysName = finalSystem.empty() ? finalHex : finalSystem;
        std::string alertMsg =
            "TACTICAL ALERT: Contact! Enemy forces detected in " + sysName +
            "!\n>> Combat will resolve when movement phase ends.";

        Telemetry::getInstance().write(alertMsg);
        Telemetry::getInstance().tell(PlayerTarget::THEM, alertMsg);
    }

    return true;
}
