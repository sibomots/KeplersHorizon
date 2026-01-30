//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "galaxy_command.h"

#include <iomanip>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool GalaxyCommand::invoke(void)
{
    int game_id = StateMachine::instance().get_game_id();
    DatabaseManager& db = DatabaseManager::instance();

    std::ostringstream out;
    out << "         GALAXY OVERVIEW\n";
    out << "=========================================================\n";
    out << " System          Hex     Base   Ships A  Ships B\n";
    out << "---------------------------------------------------------\n";

    // Get all systems
    std::string q = 
        "SELECT name, hex_id, is_base, base_owner FROM star_systems "
        "WHERE module_id=1 ORDER BY name";

    auto systems = db.Query(q, {});

    for (const auto& sys : systems)
    {
        std::string name = sys[0];
        std::string hex = sys[1];
        bool isBase = sys[2] == "1";
        std::string owner = sys[3];

        // Count ships at this hex
        std::string q = 
            "SELECT COUNT(*) FROM ships WHERE game_id=? "
            " AND at_hex=? AND owner=? AND destroyed_at IS NULL";

        auto countA = db.Query(q, { game_id, hex, 'A'});
        auto countB = db.Query(q, { game_id, hex, 'B'});

        int shipsA = countA.empty() ? 0 : std::atoi(countA[0][0].c_str());
        int shipsB = countB.empty() ? 0 : std::atoi(countB[0][0].c_str());

        // Pad name to 15 chars
        std::string paddedName = name;
        if (paddedName.length() < 15)
        {
            paddedName.append(15 - paddedName.length(), ' ');
        }
        else
        {
            paddedName = paddedName.substr(0, 15);
        }

        out << " " << paddedName << " " << hex;
        if (isBase)
        {
            out << "   [" << owner << "]  ";
        }
        else
        {
            out << "        ";
        }
        out << std::setw(8) << shipsA << " " << std::setw(8) << shipsB << "\n";
    }

    out << "=========================================================\n";
    Telemetry::instance().write(out.str());
    return true;
}
