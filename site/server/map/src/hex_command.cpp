///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "hex_command.h"

#include <format>
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

        out << std::format("─── {} [{}]", sysName, hexId);
        if (isBase)
        {
            if (baseOwner.empty())
            {
                out << " (Base, Uncontrolled)";
            }
            else
            {
                out << std::format(" (Base, {})", baseOwner);
            }
        }
        out << " ───\n";

        // Get ships at this hex
        std::string sq =
            "SELECT ship_code, ship_name, owner, ship_type FROM ships "
            " WHERE game_id=? AND at_hex=? AND destroyed_at IS NULL "
            " ORDER BY owner, ship_code";
        auto ships = db.Query(sq, {game_id, hexId});

        if (ships.empty())
        {
            out << "No ships present.\n";
        }
        else
        {
            for (const auto& ship : ships)
            {
                std::string shipType =
                    (KH_EQU(ship[3], "W")) ? "W" : "S";
                std::string side =
                    (KH_EQU(ship[2][0], me)) ? "+" : "-";
                out << std::format("  [{}] {} {} ({})\n", side, ship[0],
                                   ship[1], shipType);
            }
        }
    }

    Telemetry::instance().write(out.str());
    return true;
}
