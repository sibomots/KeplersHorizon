//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "fleet_command.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool FleetCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Join with star_systems table to get star names for at_hex
    auto rows = db.query(
        "SELECT s.ship_code, s.ship_name, s.at_hex, s.racked_in, s.pd, s.beam, "
        "s.screen, s.tube, s.missiles, s.tech_level, s.lrs, s.tb, s.dr, ss.name "
        "FROM ships s "
        "LEFT JOIN star_systems ss ON s.at_hex = ss.hex_id AND ss.module_id = 1 "
        "WHERE s.game_id=" +
        std::to_string(s.game_id) + " AND s.owner='" + std::string(1, owner) +
        "' AND s.destroyed_at IS NULL ORDER BY s.ship_code");

    if (rows.empty())
    {
        Telemetry::getInstance().write(
            "FLEET OPS: No vessels under your command.");
        return true;
    }

    std::ostringstream out;
    out << "FLEET REGISTRY [" << rows.size() << " vessels operational]\n";
    out << "HULL  DESIGNATION      SECTOR                 PD   B  S  T  M  "
           "LRS TB DR  TECH\n";
    out << "----  --------------  -------------------    --  -- -- -- --  "
           "--- -- --  ----\n";

    for (const auto& r : rows)
    {
        // Uppercase the hull designator (ship_code)
        std::string hull = r[0];
        std::transform(hull.begin(), hull.end(), hull.begin(), ::toupper);

        // Format location: show star name with hex ID if available
        std::string loc;
        if (!r[3].empty())
        {
            // Racked in another ship
            loc = "in " + r[3];
        }
        else if (!r[13].empty())
        {
            // Have system name from join (index shifted due to lrs/tb/dr)
            loc = r[13] + " (" + r[2] + ")";
        }
        else
        {
            // Just hex ID
            loc = r[2];
        }

        // Use iomanip for proper formatting with right-justified numbers
        out << std::left << std::setw(6) << hull;
        out << std::left << std::setw(16) << r[1].substr(0, 14);
        out << std::left << std::setw(23) << loc.substr(0, 21);
        out << std::right << std::setw(2) << r[4];  // pd
        out << std::right << std::setw(4) << r[5];  // beam
        out << std::right << std::setw(3) << r[6];  // screen
        out << std::right << std::setw(3) << r[7];  // tube
        out << std::right << std::setw(3) << r[8];  // missiles
        out << std::right << std::setw(4) << r[10]; // lrs
        out << std::right << std::setw(3) << r[11]; // tb
        out << std::right << std::setw(3) << r[12]; // dr
        out << std::right << std::setw(6) << r[9];  // tech_level
        out << "\n";
    }

    Telemetry::getInstance().write(out.str());
    return true;
}
