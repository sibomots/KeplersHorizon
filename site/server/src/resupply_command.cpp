//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "resupply_command.h"

#include <sstream>
#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool ResupplyCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // If no ship specified, list ships that can be resupplied
    if (m_ship_code.empty())
    {
        // Find ships at player's base stars that can take missiles
        auto rows = db.query(
            "SELECT s.ship_code, s.ship_name, s.at_hex, s.missiles, s.missiles_orig "
            "FROM ships s "
            "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = s.game_id "
            "WHERE s.game_id=" + std::to_string(s.game_id) +
            " AND s.owner='" + std::string(1, owner) + "' AND bs.owner='" + 
            std::string(1, owner) + "' AND s.missiles < s.missiles_orig");

        if (rows.empty())
        {
            Telemetry::getInstance().write("No ships need missile resupply at your base stars.");
            return true;
        }

        std::ostringstream out;
        out << "Ships that can be resupplied at your base stars:\n";
        for (const auto& r : rows)
        {
            int cur = std::atoi(r[3].c_str());
            int max = std::atoi(r[4].c_str());
            out << "  " << r[0] << " (" << r[1] << ") - Missiles: " << cur << "/" << max << "\n";
        }
        out << "Use: resupply <ship_code> <quantity>\n";
        out << "Example: resupply W1 6 (costs 2 BP for 6 missiles)\n";
        Telemetry::getInstance().write(out.str());
        return true;
    }

    // Validate ship exists and is at player's base star
    auto shipRow = db.query(
        "SELECT s.missiles, s.missiles_orig "
        "FROM ships s "
        "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = s.game_id "
        "WHERE s.game_id=" + std::to_string(s.game_id) +
        " AND s.owner='" + std::string(1, owner) + "' AND s.ship_code='" +
        db.esc(m_ship_code) + "' AND bs.owner='" + std::string(1, owner) + "'");

    if (shipRow.empty())
    {
        Telemetry::getInstance().write("Ship not found or not at your base star.");
        return false;
    }

    int curMissiles = std::atoi(shipRow[0][0].c_str());
    int maxMissiles = std::atoi(shipRow[0][1].c_str());
    int canAdd = maxMissiles - curMissiles;

    if (canAdd <= 0)
    {
        Telemetry::getInstance().write("Ship already at maximum missile capacity.");
        return false;
    }

    if (m_missiles <= 0)
    {
        Telemetry::getInstance().write("Specify quantity: resupply " + m_ship_code + " <N>");
        return false;
    }

    int addAmt = std::min(m_missiles, canAdd);
    int cost = (addAmt + 2) / 3; // 1 BP per 3 missiles, round up

    // Check BP
    int availBP = (owner == 'A') ? s.bpA : s.bpB;
    if (cost > availBP)
    {
        Telemetry::getInstance().write("Insufficient BP. Need " + std::to_string(cost) + 
            ", have " + std::to_string(availBP));
        return false;
    }

    // Apply resupply
    int newMissiles = curMissiles + addAmt;
    db.exec("UPDATE ships SET missiles=" + std::to_string(newMissiles) +
            " WHERE game_id=" + std::to_string(s.game_id) +
            " AND ship_code='" + db.esc(m_ship_code) + "'");

    // Deduct BP
    std::string bpCol = (owner == 'A') ? "bpA" : "bpB";
    db.exec("UPDATE game_state SET " + bpCol + "=" + bpCol + "-" + std::to_string(cost) +
            " WHERE game_id=" + std::to_string(s.game_id));

    Telemetry::getInstance().write("Resupplied " + m_ship_code + " with " +
        std::to_string(addAmt) + " missiles (cost: " + std::to_string(cost) + " BP)");
    return true;
}
