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

#include "game.h"
#include "logger.h"
#include "map.h"
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

MoveCommand::Builder::Builder(StateMachine &sm) : m_sm(sm)
{
}

MoveCommand::Builder &MoveCommand::Builder::ship_code(const std::string &code)
{
    m_ship_code = code;
    return *this;
}

MoveCommand::Builder &
MoveCommand::Builder::add_destination(const std::string &dest)
{
    m_destinations.push_back(dest);
    return *this;
}

ICmd *MoveCommand::Builder::build()
{
    return new MoveCommand(m_sm, m_ship_code, m_destinations);
}

MoveCommand::MoveCommand(StateMachine &sm, const std::string &ship_code,
                         const std::vector<std::string> &destinations)
    : m_sm(sm), m_ship_code(ship_code), m_destinations(destinations)
{
}

bool MoveCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    Db *m_db = m_sm.get_db();
    int m_game_id = m_sm.get_game_id();

    if (!ship_exists(m_db, m_game_id, active_player, m_ship_code))
    {
        Logger::instance().error("Ship not found: " + m_ship_code);
        Telemetry::write("Error: Ship not found: " + m_ship_code);
        return false;
    }

    ShipRow sh = load_ship(m_db, m_game_id, active_player, m_ship_code);

    if (sh.attr.type != 'W')
    {
        Logger::instance().error("Only Warpships can move");
        Telemetry::write("Error: Only Warpships can move");
        return false;
    }

    if (sh.attr.PD <= 0)
    {
        Logger::instance().error("Ship has PD=0 and cannot move");
        Telemetry::write("Error: Ship has PD=0 and cannot move");
        return false;
    }

    if (!sh.racked_in.empty())
    {
        Logger::instance().error("Ship is racked and cannot move: " +
                                 sh.racked_in);
        Telemetry::write("Error: Ship is racked and cannot move: " +
                         sh.racked_in);
        return false;
    }

    std::string startHex = sh.at_hex;
    if (startHex.empty() && !sh.at_system.empty())
    {
        startHex = resolve_system_hex(m_db, m_game_id, sh.at_system);
    }

    if (startHex.empty())
    {
        Logger::instance().error("Ship is not deployed");
        Telemetry::write("Error: Ship is not deployed");
        return false;
    }

    // ============================================================================
    // PATHFINDING ALGORITHM - PRESERVED EXACTLY FROM LEGACY CODE
    // ============================================================================
    MapGraph graph(m_db, m_game_id);
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
        Telemetry::write("Error: Ship has no movement remaining (PD spent)");
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
            auto sysr = m_db->query("SELECT name FROM star_systems WHERE "
                                    "game_id=" +
                                    std::to_string(m_game_id) +
                                    " AND hex_id='" + stepHex + "' LIMIT 1");
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
        Telemetry::write("Error: " + errorMsg);
        return false;
    }

    // Update ship location and PD spent
    if (finalSystem.empty())
        finalSystem = "";
    update_ship_location(m_db, m_game_id, active_player, sh.code, finalSystem,
                         finalHex, "");
    m_db->exec("UPDATE ships SET pd_spent=pd_spent+" +
               std::to_string(totalCost) +
               " WHERE game_id=" + std::to_string(m_game_id) + " AND owner='" +
               std::string(1, active_player) + "'" + " AND ship_code='" +
               m_db->esc(sh.code) + "'");

    // Save game state to persist changes
    save_game(m_db, s);

    std::ostringstream o;
    o << "Moved " << sh.name << " - " << sh.code << " to "
      << (finalSystem.empty() ? finalHex : finalSystem) << " (" << finalHex
      << ") cost " << totalCost << " PD";

    Logger::instance().info(o.str());
    Telemetry::write(o.str());

    return true;
}
