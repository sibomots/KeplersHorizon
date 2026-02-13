///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
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
    std::string q =
        "SELECT name, hex_id, is_base, base_owner FROM star_systems "
        " WHERE module_id=1 AND (hex_id=? OR UPPER(name)=UPPER(?)) LIMIT 1";
    auto sys = db.Query(q, {m_location, m_location});

    std::ostringstream out;
    if (sys.empty())
    {
        out << "HEX: No system found at '" << m_location << "'\n";
    }
    else
    {
        std::string sysName = sys[0][0];
        std::string hexId = sys[0][1];
        bool isBase = KH_EQU(sys[0][2], "1");

        std::string baseOwner = sys[0][3];
        std::string baseOwnerMsg("BASE STAR SYSTEM - ");
        if (baseOwner.empty())
        {
            baseOwnerMsg = std::string("Uncontrolled");
        }
        else
        {
            baseOwnerMsg = std::string("Controlled by ");
            baseOwnerMsg.append(baseOwner);
        }
        out << "                 REPORT\n"
            << "───────────────────────────────────────────\n"
            << "System: " << sysName << " [" << hexId << "]\n";
        if (isBase)
        {
            out << baseOwnerMsg << "\n";
        }
        out << "───────────────────────────────────────────\n";

        // Get ships at this hex
        std::string q =
            "SELECT ship_code, ship_name, owner, ship_type FROM ships "
            " WHERE game_id=? AND at_hex=? AND destroyed_at IS NULL "
            " ORDER BY owner, ship_code";
        auto ships = db.Query(q, {game_id, hexId});

        if (ships.empty())
        {
            // BUGBUG We want to use Long Range Scanner capabilities here..
            // BUGBUG FIX THIS
            out << "No ships present.\n";
        }
        else
        {
            // BUGBUG Must be modulated based on Long Range Scan
            // BUGBUG But the map shows it anyway..
            out << "Ships present:\n";
            for (const auto& ship : ships)
            {
                std::string shipType =
                    (KH_EQU(ship[3], "W")) ? "WarpShip" : "SystemShip";
                if (KH_EQU(ship[2][0], me))
                {
                    out << "  [FRIENDLY] " << ship[0] << " '" << ship[1]
                        << "' (" << shipType << ")\n";
                }
                else
                {
                    out << "  [HOSTILE]  " << ship[0] << " '" << ship[1]
                        << "' (" << shipType << ")\n";
                }

                // BUGBUG what about alien ships?
            }
        }
        out << "===========================================\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}
