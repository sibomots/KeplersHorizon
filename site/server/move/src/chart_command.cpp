///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "chart_command.h"

#include <format>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "ce.h"
#include "constraints.h"
#include "db.h"
#include "hex_events.h"
#include "mapgraph.h"
#include "maputil.h"
#include "moduleutil.h"
#include "shipmgr.h"
#include "star_system_constraints.h"
#include "statemachine.h"
#include "telemetry.h"

bool ChartCommand::invoke(void)
{
    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int gameId = StateMachine::instance().get_game_id();
    char activePlayer = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Validate ship exists
    bool shipPresent = shipmgr.ship_exists_by_code_or_name(
        gameId, activePlayer, m_ship_code);

    if (!shipPresent)
    {
        Telemetry::instance().write(
            std::format(LC_MOVE_TARGET_SHIP_NOT_IN_FLEET, m_ship_code));
        return false;
    }

    ShipRow sh;
    bool hasShip = shipmgr.load_ship_by_code_or_name(
        sh, gameId, activePlayer, m_ship_code);

    if (hasShip && sh.attr.type != 'W')
    {
        Telemetry::instance().write(LC_MOVE_ONLY_W_SHIP_CAN_CHART);
        return false;
    }

    if (sh.attr.PD <= 0)
    {
        Telemetry::instance().write(
            std::format(LC_MOVE_TARGET_SHIP_HAS_NO_WARP_CAP, sh.name));
        return false;
    }

    std::string startHex = sh.at_hex;
    if (startHex.empty() && !sh.at_system.empty())
    {
        startHex =
            MapUtil::instance().resolve_system_hex(gameId, sh.at_system);
    }

    if (startHex.empty())
    {
        Telemetry::instance().write(
            std::format(LC_MOVE_TARGET_SHIP_NOT_DEPLOYED, sh.name));
        return false;
    }

    // Build the movement graph
    MapGraph graph(gameId);
    graph.load_state(activePlayer);

    std::string currentHex = startHex;
    int totalCost = 0;
    int allowance = sh.attr.PD - sh.pd_spent;
    std::string errorMsg;

    std::vector<std::string> fullPath;
    fullPath.push_back(startHex);

    int warplinesUsed = 0;

    // Preload star systems and warplines for display
    const int mod = get_module_id_for_game(gameId);

    struct StarInfo
    {
        std::string name;
        bool isBase = false;
    };
    std::unordered_map<std::string, StarInfo> starByHex;
    {
        auto sysList = db.Query("SELECT hex_id, name, is_base "
                                "FROM star_systems WHERE module_id=?",
                                {mod});
        starByHex.reserve(sysList.size() * 2);
        for (const auto& row : sysList)
        {
            if (row.size() >= 3)
            {
                StarInfo si;
                si.name = row[1];
                si.isBase = (KH_EQU(row[2], "1"));
                starByHex[row[0]] = si;
            }
        }
    }

    auto makeEdgeKey = [](const std::string& a,
                          const std::string& b) -> std::string
    { return (a < b) ? (a + "|" + b) : (b + "|" + a); };

    std::unordered_set<std::string> warplineEdges;
    {
        auto wr = db.Query(
            "SELECT a_hex, b_hex FROM warplines WHERE module_id=?", {mod});
        warplineEdges.reserve(wr.size() * 2);
        for (const auto& row : wr)
        {
            if (row.size() >= 2)
            {
                warplineEdges.insert(makeEdgeKey(row[0], row[1]));
            }
        }
    }

    auto isStarHex = [&](const std::string& hx) -> bool
    { return (starByHex.find(hx) != starByHex.end()); };

    auto isWarplineEdge = [&](const std::string& h1,
                              const std::string& h2) -> bool
    {
        return (warplineEdges.find(makeEdgeKey(h1, h2)) !=
                warplineEdges.end());
    };

    // Process each waypoint segment (same BFS logic as Move)
    for (size_t i = 0; i < m_destinations.size(); ++i)
    {
        const std::string destTok = m_destinations[i];
        const std::string stepHex = graph.resolve_hex(destTok);
        if (stepHex.empty())
        {
            errorMsg = "Unknown destination: " + destTok;
            break;
        }

        std::string stepSys;
        {
            auto it = starByHex.find(stepHex);
            if (it != starByHex.end())
            {
                stepSys = it->second.name;
            }
        }

        const int remaining = allowance - totalCost;
        if (remaining <= 0)
        {
            errorMsg = "Path exceeds PD allowance.";
            break;
        }

        const auto segPath = graph.get_path(currentHex, stepHex, remaining);

        if (segPath.empty())
        {
            const int needed =
                graph.get_path_cost(currentHex, stepHex, 999999);
            if (needed >= 0)
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           ". Needed " + std::to_string(needed) +
                           " PD, remaining " + std::to_string(remaining) + ".";
            }
            else
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           " (No path or Blocked).";
            }
            break;
        }

        int stepCost = static_cast<int>(segPath.size()) - 1;

        for (size_t j = 1; j < segPath.size(); ++j)
        {
            if (isWarplineEdge(segPath[j - 1], segPath[j]))
            {
                ++warplinesUsed;
            }
            fullPath.push_back(segPath[j]);
        }

        // Apply movement modifiers at star system destinations
        if (!stepSys.empty())
        {
            if (ConstraintEngine::is_movement_blocked(gameId, stepSys))
            {
                errorMsg = "Movement to " + stepSys +
                           " is blocked by environmental hazards.";
                break;
            }

            int modifier =
                ConstraintEngine::get_movement_modifier(gameId, stepSys);
            modifier +=
                StarSystemConstraints::getMovementModifier(gameId, stepHex);
            modifier +=
                HexEventEngine::get_movement_modifier(gameId, s.round, stepHex);

            stepCost += modifier;
            if (stepCost < 1)
            {
                stepCost = 1;
            }
        }

        if (stepCost > remaining)
        {
            errorMsg = "Path exceeds PD allowance. Segment cost=" +
                       std::to_string(stepCost) +
                       ", remaining=" + std::to_string(remaining);
            break;
        }

        totalCost += stepCost;
        currentHex = stepHex;
    }

    if (!errorMsg.empty())
    {
        Telemetry::instance().write(std::format("CHART: {}", errorMsg));
        return false;
    }

    // Display the charted course
    std::ostringstream out;
    out << std::format("CHART: {} ({}) — projected course\n",
                       sh.name, sh.code);

    for (size_t i = 0; i < fullPath.size(); ++i)
    {
        if (i > 0)
        {
            if (isWarplineEdge(fullPath[i - 1], fullPath[i]))
            {
                out << " >> ";
            }
            else
            {
                out << " -> ";
            }
        }

        const std::string& hex = fullPath[i];
        auto sit = starByHex.find(hex);
        if (sit != starByHex.end())
        {
            if (sit->second.isBase)
            {
                out << sit->second.name << " [BASE] (" << hex << ")";
            }
            else
            {
                out << sit->second.name << " (" << hex << ")";
            }
        }
        else
        {
            out << hex;
        }
    }
    out << "\n";

    out << std::format(LC_MOVE_TARGET_MOVEMENT_COST_PREAMBLE, totalCost, allowance);
    if (warplinesUsed > 0)
    {
        std::string plural_jump = (warplinesUsed > 1) ? "jumps" : "jump";
        out << std::format("," LC_MOVE_TARGET_WARPLINE_JUMP, warplinesUsed, plural_jump);
    }
    out << "\n";

    Telemetry::instance().write(out.str());
    return true;
}
