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
#include "constraints.h"
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

    // Validate destination is a base-star system
    auto base_info = db.query(
        "SELECT is_base, base_side, territory_name FROM star_systems "
        "WHERE map_id=1 AND name='" + db.esc(sys) + "'");

    if (base_info.empty())
    {
        Telemetry::getInstance().write(
            "DEPLOY: Unknown system: " + m_system_name);
        return false;
    }

    if (base_info[0][0] != "1")
    {
        Telemetry::getInstance().write(
            "DEPLOY: Ships can only be deployed at a home base system.");
        return false;
    }

    std::string base_side = base_info[0][1];
    std::string territory_name = base_info[0][2];

    // Get player's home side
    std::string player_side = (active_player == 'A') ? s.home_side_A : s.home_side_B;

    if (player_side.empty())
    {
        // First deployment - set player's side
        if (active_player == 'A')
        {
            s.home_side_A = base_side;
            s.home_side_B = (base_side == "A") ? "B" : "A";
        }
        else
        {
            s.home_side_B = base_side;
            s.home_side_A = (base_side == "A") ? "B" : "A";
        }

        Telemetry::getInstance().broadcast(
            "DEPLOY: " + std::string(1, active_player) + " has claimed the " +
            territory_name + ".");
    }
    else if (player_side != base_side)
    {
        // Trying to deploy on enemy's side
        Telemetry::getInstance().write(
            "DEPLOY: Cannot deploy in enemy territory. Use your home bases.");
        return false;
    }

    std::string hex = MapUtil::getInstance().resolve_system_hex(m_game_id, sys);
    update_ship_location(m_game_id, active_player, m_ship_code, sys, hex, "");

    // Save game state to persist side assignment
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

    // Check if trying to move to current location (Bug #5)
    if (!m_destinations.empty())
    {
        std::string firstDest = m_destinations[0];
        // Resolve system name to hex if needed
        std::string destHex = MapUtil::getInstance().resolve_system_hex(m_game_id, firstDest);
        if (destHex.empty()) destHex = firstDest;  // Use as-is if not a system name

        if (destHex == startHex)
        {
            Telemetry::getInstance().write(
                "NAV: " + sh.name + " is already at " + firstDest + ".");
            return false;
        }
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

    // Collect full path for telemetry display
    std::vector<std::string> fullPath;
    fullPath.push_back(startHex);

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

        // Get actual path segment for telemetry
        auto segPath = graph.get_path(currentHex, stepHex, allowance - totalCost);
        // Skip first element (it's the current hex, already in fullPath)
        for (size_t j = 1; j < segPath.size(); ++j)
        {
            fullPath.push_back(segPath[j]);
        }

        // Apply system constraint modifiers
        if (!stepSys.empty())
        {
            if (ConstraintEngine::is_movement_blocked(m_game_id, stepSys))
            {
                errorMsg = "Movement to " + stepSys +
                           " is blocked by environmental hazards.";
                break;
            }

            int modifier =
                ConstraintEngine::get_movement_modifier(m_game_id, stepSys);
            stepCost += modifier;
            if (stepCost < 1)
                stepCost = 1;  // Minimum 1 PD
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

    // Auto-update Grimoire knowledge if entering a new system
    if (!finalSystem.empty())
    {
        // Check current knowledge level
        auto know = db.query(
            "SELECT knowledge_level FROM grimoire_entries "
            "WHERE game_id=" +
            std::to_string(m_game_id) + " AND player='" +
            std::string(1, active_player) + "' AND system_name='" +
            db.esc(finalSystem) + "'");

        std::string current_level = know.empty() ? "Unknown" : know[0][0];

        // Upgrade if Unknown - check for LRS
        if (current_level == "Unknown")
        {
            auto lrs_check = db.query("SELECT lrs FROM ships WHERE game_id=" +
                                      std::to_string(m_game_id) + " AND owner='" +
                                      std::string(1, active_player) +
                                      "' AND ship_code='" + db.esc(sh.code) + "'");

            int lrs = lrs_check.empty() ? 0 : std::atoi(lrs_check[0][0].c_str());
            std::string new_level = (lrs > 0) ? "Charted" : "Rumored";

            if (know.empty())
            {
                db.exec("INSERT INTO grimoire_entries(game_id, player, "
                        "system_name, knowledge_level, last_updated_turn) "
                        "VALUES(" +
                        std::to_string(m_game_id) + ",'" +
                        std::string(1, active_player) + "','" +
                        db.esc(finalSystem) + "','" + new_level + "'," +
                        std::to_string(s.round) + ")");
            }
            else
            {
                db.exec("UPDATE grimoire_entries SET knowledge_level='" +
                        new_level + "', last_updated_turn=" +
                        std::to_string(s.round) + " WHERE game_id=" +
                        std::to_string(m_game_id) + " AND player='" +
                        std::string(1, active_player) + "' AND system_name='" +
                        db.esc(finalSystem) + "'");
            }

            Telemetry::getInstance().write("GRIMOIRE: " + finalSystem +
                                           " now " + new_level + ".");
        }
    }

    std::ostringstream o;
    o << "NAV: " << sh.name << " (" << sh.code << ") transit to "
      << (finalSystem.empty() ? finalHex : finalSystem) << " [" << finalHex
      << "] complete. Power expended: " << totalCost << " PD";

    Logger::instance().info(o.str());
    Telemetry::getInstance().write(o.str());

    // Display computed path for inspection
    if (fullPath.size() > 1)
    {
        // Build enhanced path display:
        // - Star hexes shown as NAME (hex#)
        // - Warpline traversals use "=" connector
        // - Regular hex moves use "->" connector

        // Cache system names for hexes
        std::unordered_map<std::string, std::string> hexToSys;
        auto sysList = db.query(
            "SELECT hex_id, name FROM star_systems WHERE map_id=1");
        for (const auto& row : sysList)
        {
            hexToSys[row[0]] = row[1];
        }

        // Check if two hexes are connected by warpline
        auto isWarpline = [&](const std::string& h1, const std::string& h2) -> bool {
            auto result = db.query(
                "SELECT 1 FROM warplines WHERE map_id=1 AND "
                "((a_hex='" + db.esc(h1) + "' AND b_hex='" + db.esc(h2) + "') OR "
                "(a_hex='" + db.esc(h2) + "' AND b_hex='" + db.esc(h1) + "')) LIMIT 1");
            return !result.empty();
        };

        std::ostringstream pathOut;
        pathOut << "NAV: Course plotted: ";
        for (size_t i = 0; i < fullPath.size(); ++i)
        {
            // Add connector before all but first
            if (i > 0)
            {
                if (isWarpline(fullPath[i-1], fullPath[i]))
                    pathOut << " = ";  // Warpline jump
                else
                    pathOut << " -> ";  // Regular hex move
            }

            // Format hex: STARNAME (hex#) or just hex#
            const std::string& hex = fullPath[i];
            auto sit = hexToSys.find(hex);
            if (sit != hexToSys.end())
            {
                pathOut << sit->second << " (" << hex << ")";
            }
            else
            {
                pathOut << hex;
            }
        }
        Telemetry::getInstance().write(pathOut.str());
    }

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
