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

    if (sys.empty())
    {
        Telemetry::instance().write(
            std::format("HEX: No system found at '{}'", m_location));
        return false;
    }

    std::string sysName = sys[0][0];
    std::string hexId = sys[0][1];
    bool isBase = KH_EQU(sys[0][2], "1");
    std::string baseOwner = sys[0][3];

    // Scanning costs 1 PD — find a player ship at this hex with available PD
    // Ships with LRS scan for free
    auto scanShips = db.Query(
        "SELECT ship_code, pd, pd_spent, lrs FROM ships "
        " WHERE game_id=? AND owner=? AND at_hex=? AND destroyed_at IS NULL "
        " ORDER BY lrs DESC, (pd - pd_spent) DESC LIMIT 1",
        {game_id, me, hexId});

    if (scanShips.empty())
    {
        Telemetry::instance().write(
            "HEX: You have no ships at this location to perform a scan.");
        return false;
    }

    std::string scanShipCode = scanShips[0][0];
    int pdTotal = std::atoi(scanShips[0][1].c_str());
    int pdSpent = std::atoi(scanShips[0][2].c_str());
    int lrs = std::atoi(scanShips[0][3].c_str());
    bool freeScan = (lrs > 0);

    if (!freeScan && (pdTotal - pdSpent) < 1)
    {
        Telemetry::instance().write(
            "HEX: Insufficient PD to perform scan. Requires 1 PD.");
        return false;
    }

    std::ostringstream out;
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

    // Deduct 1 PD for the scan (LRS-equipped ships scan for free)
    if (!freeScan)
    {
        db.Exec("UPDATE ships SET pd_spent=pd_spent+1 "
                "WHERE game_id=? AND owner=? AND ship_code=?",
                {game_id, me, scanShipCode});
        out << std::format("(Scan cost: 1 PD from {})\n", scanShipCode);
    }
    else
    {
        out << std::format("(LRS scan via {})\n", scanShipCode);
    }

    Telemetry::instance().write(out.str());
    return true;
}
