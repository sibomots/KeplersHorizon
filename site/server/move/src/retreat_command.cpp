//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "retreat_command.h"

#include <algorithm>
#include <sstream>

#include "db.h"
#include "mapgraph.h"
#include "moduleutil.h"
#include "statemachine.h"
#include "telemetry.h"
#include "logger.h"

bool RetreatCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // Normalize ship code to uppercase
    std::string ship_code = m_ship_code;
    std::transform(ship_code.begin(), ship_code.end(), ship_code.begin(),
                   ::toupper);

    std::string q = 
     "SELECT at_hex, escape_pending FROM ships WHERE game_id=?"
     " AND owner=? AND ship_code=? AND destroyed_at IS NULL";

    auto ship_rows = db.Query(q, { s.game_id, owner, ship_code });

    if (ship_rows.empty())
    {
        Telemetry::instance().write("RETREAT: Ship not found: " + ship_code);
        return false;
    }

    std::string current_hex = ship_rows[0][0];
    int escape_pending = std::atoi(ship_rows[0][1].c_str());

    // Check if ship is allowed to retreat
    if (escape_pending != 1)
    {
        Telemetry::instance().write(
            "RETREAT: " + ship_code +
            " has no pending retreat. Use during escape or stalemate.");
        return false;
    }

    // Resolve destination hex
    MapGraph mg(s.game_id);
    std::string dest_hex = mg.resolve_hex(m_dest_hex);
    if (dest_hex.empty())
    {
        Telemetry::instance().write("RETREAT: Invalid hex: " + m_dest_hex);
        return false;
    }

    // Check if destination is adjacent
    std::vector<std::string> adjacent = mg.get_adjacent_hexes(current_hex);
    bool is_adjacent = false;
    for (const auto& adj : adjacent)
    {
        if (adj == dest_hex)
        {
            is_adjacent = true;
            break;
        }
    }

    if (!is_adjacent)
    {
        std::ostringstream out;
        out << "RETREAT: " << dest_hex << " is not adjacent to " << current_hex
            << ".\n";
        out << "Valid retreat destinations:";
        for (const auto& adj : adjacent)
        {
            // Get system name if any
            int mod = get_module_id_for_game(s.game_id);
            std::string q =
            "SELECT name FROM star_systems "
            " WHERE module_id=? AND hex_id=?";

            auto sys_rows = db.Query(q, { mod,  adj });

            if (!sys_rows.empty())
            {
                out << "\n  " << sys_rows[0][0] << " (" << adj << ")";
            }
            else
            {
                out << "\n  " << adj;
            }
        }
        Telemetry::instance().write(out.str());
        return false;
    }

    // Perform retreat - move ship to destination (no PD cost)
    std::string qq =
    "UPDATE ships SET at_hex=?, escape_pending=0 "
    " WHERE game_id=? AND ship_code=?";

    db.Exec(qq, { dest_hex,s.game_id, ship_code});

    // Get destination system name for message
    int mod = get_module_id_for_game(s.game_id);
 
    
    std::string qqq = 
    "SELECT name FROM star_systems WHERE module_id=? "
    " AND hex_id=?";
    auto dest_sys = db.Query(qqq, {mod, dest_hex});

    std::string dest_name = dest_hex;
    if (!dest_sys.empty())
    {
        dest_name = dest_sys[0][0] + " (" + dest_hex + ")";
    }

    Telemetry::instance().write("RETREAT: " + ship_code +
                                   " withdraws to " + dest_name);

    // Update the combat status -
    // The retreat might  have left the hex
    // in combat no longer in combat.

    // Sanity check. Is the 'owner' and the 'active player' the same?

    // BUGBUG WHY ARE WE STILL HOLDING ACTIVE PLAYER AS A/B WITH A STRING?
    char test_active_player = (s.active_player)[0];

    std::string santest;
    santest.append("Current player [owner]: ");
    santest += owner;
    santest.append("  Active player: ");
    santest += test_active_player;
    Logger::instance().info("Sanity Check: Is Active player = Current Player");
    Logger::instance().info(santest);

    // BUGBUG: Update the DB here.  JDWJDW
    std::string qqqq = 
    "UPDATE combat_state AS cs "
     " JOIN ( "
     " SELECT DISTINCT game_id, at_hex "
     " FROM ships "
     " WHERE game_id =?  AND owner =?  AND at_hex=? "
     " AND escape_pending = 1 ) "
     " AS s "
     " ON s.game_id = cs.game_id "
     " AND s.at_hex = cs.hex_id "
     " SET cs.attacker_remains = 0 "
     " WHERE cs.attacker_remains = 1 "
     " AND cs.stalemate_counter > 2";

    db.Exec(qqqq, {s.game_id, owner, current_hex });

    return true;
}
