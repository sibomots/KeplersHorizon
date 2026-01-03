//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "fleet_command.h"

#include <sstream>
#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool FleetCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows = db.query(
        "SELECT ship_code, ship_name, at_hex, racked_in, pd, beam, screen, "
        "tube, missiles, tech_level FROM ships WHERE game_id=" +
        std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) +
        "' ORDER BY ship_code");

    if (rows.empty())
    {
        Telemetry::getInstance().write("You have no ships.");
        return true;
    }

    std::ostringstream out;
    out << "Your Fleet (" << rows.size() << " ships):\n";
    out << "Code  Name            Location    PD  B  S  T  M  Tech\n";
    out << "----  --------------  ----------  --  -  -  -  -  ----\n";

    for (const auto& r : rows)
    {
        std::string loc = r[3].empty() ? r[2] : ("in " + r[3]);
        out << r[0];  // ship_code
        out << std::string(6 - r[0].size(), ' ');
        
        std::string name = r[1].substr(0, 14);
        out << name << std::string(16 - name.size(), ' ');
        
        out << loc << std::string(12 - loc.size(), ' ');
        out << r[4] << std::string(4 - r[4].size(), ' ');  // pd
        out << r[5] << std::string(3 - r[5].size(), ' ');  // beam
        out << r[6] << std::string(3 - r[6].size(), ' ');  // screen
        out << r[7] << std::string(3 - r[7].size(), ' ');  // tube
        out << r[8] << std::string(3 - r[8].size(), ' ');  // missiles
        out << r[9] << "\n";  // tech_level
    }

    Telemetry::getInstance().write(out.str());
    return true;
}
