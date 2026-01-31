//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <iomanip>
#include <sstream>

#include "ce.h"
#include "constraints.h"
#include "db.h"
#include "deploy_command.h"
#include "hex_events.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "moduleutil.h"
#include "move_command.h"
#include "shipmgr.h"
#include "star_system_constraints.h"
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
    if (!StateMachine::instance().check_inhibits(CommandID::DEPLOY,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();

    GameState s = StateMachine::instance().get_game_state();
    int m_game_id = StateMachine::instance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    std::string sys =
        MapUtil::instance().resolve_system_name(m_game_id, m_system_name);

    if (!shipmgr.ship_exists_by_code_or_name(m_game_id, active_player,
                                             m_ship_code))
    {
        Telemetry::instance().write("FLEET REGISTRY: Vessel " + m_ship_code +
                                    " is not in your fleet!");
        return false;
    }

    ShipRow sh;
    bool has_ship = shipmgr.load_ship_by_code_or_name(
        sh, m_game_id, active_player, m_ship_code);
    if (has_ship && !sh.racked_in.empty())
    {
        Telemetry::instance().write(
            "Error: Ship is racked; drop it before deploying: " + m_ship_code);
        return false;
    }

    // Validate destination is a base-star system
    int mod_id = get_module_id_for_game(m_game_id);
    auto base_info =
        db.Query("SELECT is_base, base_side, territory_name FROM star_systems "
                 "WHERE module_id=? AND name=?",
                 {mod_id, sys});

    if (base_info.empty())
    {
        Telemetry::instance().write("DEPLOY: Unknown system: " + m_system_name);
        return false;
    }

    if (base_info[0][0] != "1")
    {
        Telemetry::instance().write(
            "DEPLOY: Ships can only be deployed at a home base system.");
        return false;
    }

    std::string base_side = base_info[0][1];
    std::string territory_name = base_info[0][2];

    // Get player's home side
    std::string player_side =
        (active_player == 'A') ? s.home_side_A : s.home_side_B;

    // BUGBUG jdw   We need a query that can get both territories
    // announced when the first base is claimed.

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

        // Get player's actual username for the broadcast
        int user_id = StateMachine::instance().get_current_user_id();
        auto user_rows = DatabaseManager::instance().Query(
            "SELECT username FROM users WHERE id=?", {user_id});
        std::string player_name =
            user_rows.empty() ? "Player" : user_rows[0][0];

        Telemetry::instance().broadcast("DEPLOY: " + player_name +
                                        " has claimed the " + territory_name +
                                        ".");
    }
    else if (player_side != base_side)
    {
        // Trying to deploy on enemy's side
        Telemetry::instance().write(
            "DEPLOY: Cannot deploy in enemy territory. Use your home bases.");
        return false;
    }

    std::string hex = MapUtil::instance().resolve_system_hex(m_game_id, sys);
    shipmgr.update_ship_location(m_game_id, active_player, m_ship_code, sys,
                                 hex);

    // Save game state to persist side assignment
    StateMachine::instance().save_game(s);

    Telemetry::instance().write("FLEET COMMAND: " + sh.name + " (" + sh.code +
                                ") deployed to " + sys);

    return true;
}

bool MoveCommand::invoke(void)
{
    MoveParams_t params;
    params.ship_code = m_ship_code;
    params.destination = m_destinations.empty() ? "" : m_destinations[0];

    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::MOVE,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int m_game_id = StateMachine::instance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    bool ship_present = shipmgr.ship_exists_by_code_or_name(
        m_game_id, active_player, m_ship_code);

    if (!ship_present)
    {
        Telemetry::instance().write("FLEET REGISTRY: Vessel " + m_ship_code +
                                    " is not in your fleet!");
        return false;
    }

    ShipRow sh;
    bool has_ship = shipmgr.load_ship_by_code_or_name(
        sh, m_game_id, active_player, m_ship_code);

    if (has_ship && sh.attr.type != 'W')
    {
        Telemetry::instance().write(
            "NAV: Only WarpShip class vessels can engage hyperdrive.");
        return false;
    }

    if (sh.attr.PD <= 0)
    {
        Telemetry::instance().write(
            "NAV: " + sh.name +
            " has no power drive capacity. Unable to maneuver.");
        return false;
    }

    if (!sh.racked_in.empty())
    {
        Telemetry::instance().write("OPS: Ship is racked and cannot move: " +
                                    sh.racked_in);
        return false;
    }

    std::string startHex = sh.at_hex;
    if (startHex.empty() && !sh.at_system.empty())
    {
        startHex =
            MapUtil::instance().resolve_system_hex(m_game_id, sh.at_system);
    }

    if (startHex.empty())
    {
        Telemetry::instance().write(
            "NAV: " + sh.name + " is not deployed. It is still in shipyard.");
        return false;
    }

    // Check if trying to move to current location
    if (!m_destinations.empty())
    {
        std::string firstDest = m_destinations[0];
        // Resolve system name to hex if needed
        std::string destHex =
            MapUtil::instance().resolve_system_hex(m_game_id, firstDest);

        if (destHex.empty())
        {
            destHex = firstDest; // Use as-is if not a system name
        }

        if (destHex == startHex)
        {
            Telemetry::instance().write("NAV: " + sh.name + " is already at " +
                                        firstDest + ".");
            return false;
        }
    }

    // ============================================================================
    // PATHFINDING ALGORITHM - REVISED
    //
    // Correctness goals:
    //  1) Base PD spent is exactly the number of traversed edges in the
    //  computed route.
    //     (Each geometric neighbor move OR star-to-star warpline jump costs 1
    //     PD.)
    //  2) Warplines are naturally used when they reduce hop-count because
    //  MapGraph::get_path()
    //     is the authority for shortest-path search on the full movement graph.
    //  3) Preserve a full ordered itinerary of visited hexes, annotated as:
    //        - NORMAL
    //        - STAR
    //        - STAR_BASE
    // ============================================================================
    MapGraph graph(m_game_id);
    graph.load_state(active_player);

    // Process multi-step path (user waypoints are already ordered in
    // m_destinations)
    std::string currentHex = startHex;
    int totalCost = 0;
    int allowance = sh.attr.PD - sh.pd_spent;
    std::string finalSystem = sh.at_system;
    std::string finalHex = startHex;
    std::string errorMsg;

    if (allowance <= 0)
    {
        Telemetry::instance().write(
            "NAV: " + sh.name + " has exhausted power drive for this turn.");
        return false;
    }

    // Full ordered list of visited hexes (including intermediates)
    std::vector<std::string> fullPath;
    fullPath.push_back(startHex);

    // For debugging / telemetry
    int warplinesUsed = 0;

    // --- Preload star systems and warplines for fast lookups
    // ------------------
    const int mod = get_module_id_for_game(m_game_id);

    // hex_id -> (system_name, is_base)
    std::unordered_map<std::string, std::pair<std::string, bool>> starByHex;
    {
        auto sysList = db.Query("SELECT hex_id, name, is_base "
                                "FROM star_systems WHERE module_id=?",
                                {mod});
        starByHex.reserve(sysList.size() * 2);
        for (const auto& row : sysList)
        {
            if (row.size() >= 3)
            {
                const std::string& hx = row[0];
                const std::string& nm = row[1];
                const bool isBase = (row[2] == "1");
                starByHex[hx] = {nm, isBase};
            }
        }
    }

    auto make_edge_key = [](const std::string& a,
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
                warplineEdges.insert(make_edge_key(row[0], row[1]));
            }
        }
    }

    auto isStarHex = [&](const std::string& hx) -> bool
    { return (starByHex.find(hx) != starByHex.end()); };

    auto isWarplineEdge = [&](const std::string& h1,
                              const std::string& h2) -> bool {
        return (warplineEdges.find(make_edge_key(h1, h2)) !=
                warplineEdges.end());
    };

    // --- Execute each user-specified segment
    // ----------------------------------
    for (size_t i = 0; i < m_destinations.size(); ++i)
    {
        const std::string destTok = m_destinations[i];

        // Accept either a system name or a hex id.
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
                stepSys = it->second.first;
            }
        }

        // Remaining PD budget for base movement (1 PD per edge).
        // IMPORTANT: modifiers (hazards) are applied AFTER base movement cost.
        const int remaining = allowance - totalCost;
        if (remaining <= 0)
        {
            errorMsg = "Path exceeds PD allowance.";
            break;
        }

        // Compute shortest path segment using the authoritative graph
        // (includes geometric adjacency + warplines + blockades).
        const auto segPath = graph.get_path(currentHex, stepHex, remaining);

        if (segPath.empty())
        {
            const int needed = graph.get_path_cost(currentHex, stepHex, 999999);
            if (needed >= 0)
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           ". Needed " + std::to_string(needed) +
                           " PD, but remaining is " +
                           std::to_string(remaining) + ".";
            }
            else
            {
                errorMsg = "Cannot reach " + destTok + " from " + currentHex +
                           " (No path or Blocked).";
            }
            break;
        }

        if (segPath.front() != currentHex || segPath.back() != stepHex)
        {
            errorMsg = "Internal error: path endpoints mismatch for " +
                       currentHex + " -> " + stepHex;
            break;
        }

        // --- Base PD cost: exact number of traversed edges
        // --------------------- If the segment visits N hexes, it traverses N-1
        // edges; each edge costs 1 PD.
        int stepCost = static_cast<int>(segPath.size()) - 1;

        // Append segment to full path and count warplines actually used
        for (size_t j = 1; j < segPath.size(); ++j)
        {
            if (isWarplineEdge(segPath[j - 1], segPath[j]))
                ++warplinesUsed;
            fullPath.push_back(segPath[j]);
        }

        // --- Apply system constraint modifiers (legacy behavior preserved)
        // ----- NOTE: The legacy code applies movement modifiers only when the
        // *destination* is a star-system hex (stepSys non-empty). If you intend
        // modifiers to apply for every star-system hex traversed (including
        // intermediates), that is a rules change and should move into MapGraph
        // as weighted edges.
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

            // Add star system environmental movement modifier
            modifier +=
                StarSystemConstraints::getMovementModifier(m_game_id, stepHex);

            // Add dynamic hex event modifier (NAVIGATION_HAZARD)
            modifier += HexEventEngine::get_movement_modifier(m_game_id,
                                                              s.round, stepHex);

            stepCost += modifier;
            if (stepCost < 1)
                stepCost = 1; // Minimum 1 PD
        }

        // Enforce PD allowance
        if (stepCost > remaining)
        {
            errorMsg = "Path exceeds PD allowance. Segment cost=" +
                       std::to_string(stepCost) +
                       ", remaining=" + std::to_string(remaining);
            break;
        }

        totalCost += stepCost;
        currentHex = stepHex;
        finalHex = stepHex;
        finalSystem = stepSys;
    }
    // ============================================================================
    // END PATHFINDING ALGORITHM - REVISED
    // ============================================================================

    if (!errorMsg.empty())
    {
        Logger::instance().error(errorMsg);
        Telemetry::instance().write("Error: " + errorMsg);
        return false;
    }

    // Update ship location and PD spent
    if (finalSystem.empty())
    {
        finalSystem.clear();
    }
    shipmgr.update_ship_location(m_game_id, active_player, sh.code, finalSystem,
                                 finalHex);
    db.Exec("UPDATE ships SET pd_spent=pd_spent+? "
            "WHERE game_id=? AND owner=? AND ship_code=?",
            {totalCost, m_game_id, active_player, sh.code});

    // Save game state to persist changes
    StateMachine::instance().save_game(s);

    std::string milieu_report;

    // Auto-update codex knowledge if entering a new system
    if (!finalSystem.empty())
    {
        // Check current knowledge level
        auto know = db.Query("SELECT knowledge_level FROM codex_entries "
                             "WHERE game_id=? AND player=? AND system_name=?",
                             {m_game_id, active_player, finalSystem});

        std::string current_level = know.empty() ? "Unknown" : know[0][0];

        // Upgrade if Unknown - check for LRS
        if (current_level == "Unknown")
        {
            auto lrs_check = db.Query("SELECT lrs FROM ships WHERE game_id=? "
                                      "AND owner=? AND ship_code=?",
                                      {m_game_id, active_player, sh.code});

            int lrs =
                lrs_check.empty() ? 0 : std::atoi(lrs_check[0][0].c_str());
            std::string new_level = (lrs > 0) ? "Charted" : "Rumored";

            if (know.empty())
            {
                db.Exec("INSERT INTO codex_entries(game_id, player, "
                        "system_name, knowledge_level, last_updated_turn) "
                        "VALUES(?,?,?,?,?)",
                        {m_game_id, active_player, finalSystem, new_level,
                         s.round});
            }
            else
            {
                db.Exec("UPDATE codex_entries SET knowledge_level=?, "
                        "last_updated_turn=? WHERE game_id=? AND player=? "
                        "AND system_name=?",
                        {new_level, s.round, m_game_id, active_player,
                         finalSystem});
            }

            milieu_report.append("CODEX: ");
            milieu_report.append(finalSystem);
            milieu_report.append(" now ");
            milieu_report.append(new_level);
            milieu_report += '.';
        }
    }

    // Display computed path for inspection
    if (fullPath.size() > 1)
    {
        // Build enhanced path display:
        // - Star hexes shown as NAME (hex#)
        // - Warpline traversals use "=" connector
        // - Regular hex moves use "->" connector

        // Cache star system info for hexes (name + base flag)
        struct StarInfo
        {
            std::string name;
            bool is_base = false;
        };
        std::unordered_map<std::string, StarInfo> hexToStar;

        int mod = get_module_id_for_game(m_game_id);
        auto sysList = db.Query(
            "SELECT hex_id, name, is_base FROM star_systems WHERE module_id=?",
            {mod});
        hexToStar.reserve(sysList.size() * 2);
        for (const auto& row : sysList)
        {
            if (row.size() >= 3)
            {
                StarInfo si;
                si.name = row[1];
                si.is_base = (row[2] == "1");
                hexToStar[row[0]] = si;
            }
        }

        // Cache warplines for fast edge check (undirected)
        auto make_edge_key = [](const std::string& a,
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
                    warplineEdges.insert(make_edge_key(row[0], row[1]));
            }
        }

        auto isWarpline = [&](const std::string& h1,
                              const std::string& h2) -> bool {
            return (warplineEdges.find(make_edge_key(h1, h2)) !=
                    warplineEdges.end());
        };

        std::ostringstream pathOut;
        pathOut << "NAV: Course plotted. Standby.\n";
        for (size_t i = 0; i < fullPath.size(); ++i)
        {
            // Add connector before all but first
            if (i > 0)
            {
                if (isWarpline(fullPath[i - 1], fullPath[i]))
                {
                    pathOut << " >> "; // Warpline jump
                }
                else
                {
                    pathOut << " -> "; // Regular hex move
                }
            }

            // Format hex: STARNAME (hex#) or just hex#
            const std::string& hex = fullPath[i];
            auto sit = hexToStar.find(hex);
            if (sit != hexToStar.end())
            {
                // Mark bases explicitly (helps debugging + matches "base
                // star-hex" requirement)
                if (sit->second.is_base)
                    pathOut << sit->second.name << " [BASE] (" << hex << ")";
                else
                    pathOut << sit->second.name << " (" << hex << ")";
            }
            else
            {
                pathOut << hex;
            }
        }
        if (fullPath.size() > 0)
        {
            pathOut << "\n";
        }
        Telemetry::instance().write(pathOut.str());

        std::ostringstream o;
        o << "OPS: " << sh.name << " (" << sh.code << ") transit to "
          << (finalSystem.empty() ? finalHex : finalSystem) << " [" << finalHex
          << "] complete.\nPower expended: " << totalCost << " PD";
        Telemetry::instance().write(o.str());

        Telemetry::instance().write(milieu_report);
    }

    // Check for combat trigger: enemy ships in destination hex
    char enemy = (active_player == 'A') ? 'B' : 'A';
    bool litmus = CombatEngine::is_real_combat_state(m_game_id);

    // if (!enemy_ships.empty() && std::atoi(enemy_ships[0][0].c_str()) > 0)
    if (litmus) //  && std::atoi(enemy_ships[0][0].c_str()) > 0)
    {
        // Enemy ships present - trigger combat check
        CombatEngine ce(m_game_id);
        ce.check_for_combat_triggers(); // Creates combat if not exists

        // Notify both players
        std::string sysName = finalSystem.empty() ? finalHex : finalSystem;
        std::string alertMsg =
            "TACTICAL ALERT: Contact! Enemy forces detected in " + sysName +
            "!\n>> Combat will resolve when movement phase ends.";

        Telemetry::instance().write(alertMsg);
        Telemetry::instance().tell(PlayerTarget::THEM, alertMsg);
    }
    else
    {
        // this is not a star-base hex.  So we cannot initiate combat
        // even if there are enemy ships.  let's see what is there for
        // alerting the player(s)
        std::string qprox =
            "SELECT "
            " UPPER(s.ship_code), UPPER(s.ship_name),  s.owner, "
            " CASE WHEN s.owner=? "
            "    THEN 1 ELSE 0 END AS is_friendly, "
            " CASE WHEN s.owner <> ? "
            " THEN 1 ELSE 0 END AS is_enemy, "
            " 0 AS is_neither_friend_nor_enemy, "
            "  hx.ships_in_hex, s.at_hex "
            " FROM ships s "
            " JOIN ( "
            " SELECT "
            " s2.at_hex, "
            " COUNT(*) AS ships_in_hex "
            "  FROM ships s2 "
            "  LEFT JOIN star_systems ss "
            "    ON ss.hex_id    = s2.at_hex "
            "  WHERE s2.game_id=? "
            "    AND s2.destroyed_at IS NULL "
            "    AND (s2.racked_in IS NULL OR s2.racked_in = '')"
            "    AND s2.at_hex IS NOT NULL "
            "  GROUP BY s2.at_hex "
            "  HAVING "
            "    SUM(CASE WHEN s2.owner =? "
            "   THEN 1 ELSE 0 END) > 0 "
            "    AND "
            "    SUM(CASE WHEN s2.owner <> ? "
            "   THEN 1 ELSE 0 END) > 0 "
            ") hx "
            "  ON hx.at_hex = s.at_hex "
            "  WHERE s.game_id=? "
            "  AND s.destroyed_at IS NULL "
            "  AND (s.racked_in IS NULL OR s.racked_in = '')"
            "  AND s.at_hex IS NOT NULL "
            "ORDER BY s.at_hex, s.owner, s.ship_code";

        auto obprox =
            db.Query(qprox, {active_player, active_player, m_game_id,
                             active_player, active_player, m_game_id});

        // If there are opposing forces in any non-star hex, we want to alert
        // this!
        if (!obprox.empty())
        {
            std::ostringstream pids;
            std::string qqr = "SELECT u.id, UPPER(u.username), u.id, gs.seat "
                              " FROM users u, game_seats gs "
                              " WHERE gs.user_id = u.id "
                              " AND gs.game_id=?  AND "
                              " (gs.seat=?  OR gs.seat =?) "
                              " ORDER by gs.seat ASC";
            auto involved = db.Query(qqr, {m_game_id, active_player, enemy});

            std::ostringstream whodat;
            std::string you("You");
            std::string them("Enemy");
            char keyc = 'A';

            // are both players in the game?
            if (involved.size() == 2)
            {
                you = involved[0][1];
                them = involved[1][1];
            }

            whodat << "> TACTICAL ALERT:\n"
                   << "              SCANNERS DETECT\n"
                   << "  ---------------------------------------\n"
                   << "  Code  Name                        Owner\n"
                   << "  ---------------------------------------\n";

            for (std::vector<std::vector<std::string>>::iterator itr =
                     obprox.begin();
                 itr != obprox.end(); ++itr)
            {
                std::vector<std::string> row = *itr;
                // s.ship_code, s.ship_name,  s.owner, "
                whodat << std::left << std::setw(6) << row[0];
                whodat << std::left << std::setw(28) << row[1];
                whodat << std::left << std::setw(5);
                keyc = (char)(row[2].front());
                std::string pis = (keyc == 'A') ? you : them;
                whodat << pis << "\n";
            }
            Telemetry::instance().broadcast(whodat.str());
        }
    }
    return true;
}
