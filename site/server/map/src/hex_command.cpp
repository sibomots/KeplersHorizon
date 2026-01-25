//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "hex_command.h"

#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool HexCommand::invoke(void)
{
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // Try to find system by hex_id or name
    auto sys = db.query(
        "SELECT name, hex_id, is_base, base_owner FROM star_systems "
        "WHERE module_id=1 AND (hex_id='" +
        db.esc(m_location) + "' OR UPPER(name)=UPPER('" + db.esc(m_location) +
        "')) LIMIT 1");

    std::ostringstream out;
    if (sys.empty())
    {
        out << "HEX: No system found at '" << m_location << "'\n";
    }
    else
    {
        std::string sysName = sys[0][0];
        std::string hexId = sys[0][1];
        bool isBase = sys[0][2] == "1";
        std::string baseOwner = sys[0][3];

        out << "         HEX INFORMATION\n";
        out << "===========================================\n";
        out << "System: " << sysName << " [" << hexId << "]\n";
        if (isBase)
        {
            out << "BASE STAR - Controlled by Player " << baseOwner << "\n";
        }
        out << "-------------------------------------------\n";

        // Get ships at this hex
        auto ships = db.query(
            "SELECT ship_code, ship_name, owner, ship_type FROM ships "
            "WHERE game_id=" +
            std::to_string(game_id) + " AND at_hex='" + db.esc(hexId) +
            "' AND destroyed_at IS NULL "
            "ORDER BY owner, ship_code");

        if (ships.empty())
        {
            out << "No ships present.\n";
        }
        else
        {
            out << "Ships present:\n";
            for (const auto& ship : ships)
            {
                std::string shipType =
                    (ship[3] == "W") ? "WarpShip" : "SystemShip";
                if (ship[2][0] == me)
                {
                    out << "  [FRIENDLY] " << ship[0] << " '" << ship[1]
                        << "' (" << shipType << ")\n";
                }
                else
                {
                    out << "  [HOSTILE]  " << ship[0] << " '" << ship[1]
                        << "' (" << shipType << ")\n";
                }
            }
        }
        out << "===========================================\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}
