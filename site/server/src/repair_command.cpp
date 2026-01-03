//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "repair_command.h"

#include <sstream>
#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool RepairCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // If no ship specified, list repairable ships
    if (m_ship_code.empty())
    {
        // Find ships at player's base stars that have damage
        auto rows = db.query(
            "SELECT s.ship_code, s.ship_name, s.at_hex, s.pd, s.beam, s.screen, "
            "s.tube, s.missiles, s.pd_orig, s.beam_orig, s.screen_orig, "
            "s.tube_orig, s.missiles_orig FROM ships s "
            "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = s.game_id "
            "WHERE s.game_id=" + std::to_string(s.game_id) +
            " AND s.owner='" + std::string(1, owner) + "' AND bs.owner='" + 
            std::string(1, owner) + "' AND s.destroyed_at IS NULL AND ("
            "s.pd < s.pd_orig OR s.beam < s.beam_orig OR "
            "s.screen < s.screen_orig OR s.tube < s.tube_orig OR "
            "s.missiles < s.missiles_orig)");

        if (rows.empty())
        {
            Telemetry::getInstance().write("No damaged ships at your base stars.");
            return true;
        }

        std::ostringstream out;
        out << "Repairable ships at your base stars:\n";
        for (const auto& r : rows)
        {
            out << "  " << r[0] << " (" << r[1] << ") at " << r[2] << "\n";
        }
        out << "Use: repair <ship_code> <attribute>=<amount>\n";
        out << "Example: repair W1 pd=2 (costs 2 BP)\n";
        Telemetry::getInstance().write(out.str());
        return true;
    }

    // Validate ship exists and is at player's base star
    auto shipRow = db.query(
        "SELECT s.at_hex, s.pd, s.beam, s.screen, s.tube, s.missiles, "
        "s.pd_orig, s.beam_orig, s.screen_orig, s.tube_orig, s.missiles_orig "
        "FROM ships s "
        "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = s.game_id "
        "WHERE s.game_id=" + std::to_string(s.game_id) +
        " AND s.owner='" + std::string(1, owner) + "' AND s.ship_code='" +
        db.esc(m_ship_code) + "' AND bs.owner='" + std::string(1, owner) + "' AND s.destroyed_at IS NULL");

    if (shipRow.empty())
    {
        Telemetry::getInstance().write("Ship not found or not at your base star.");
        return false;
    }

    if (m_attribute.empty() || m_amount <= 0)
    {
        Telemetry::getInstance().write("Specify attribute and amount: repair " + 
            m_ship_code + " pd=N or b=N or s=N or t=N");
        return false;
    }

    // Map attribute to columns
    std::string col, origCol;
    if (m_attribute == "pd" || m_attribute == "d") { col = "pd"; origCol = "pd_orig"; }
    else if (m_attribute == "b" || m_attribute == "beam") { col = "beam"; origCol = "beam_orig"; }
    else if (m_attribute == "s" || m_attribute == "screen") { col = "screen"; origCol = "screen_orig"; }
    else if (m_attribute == "t" || m_attribute == "tube") { col = "tube"; origCol = "tube_orig"; }
    else {
        Telemetry::getInstance().write("Invalid attribute. Use pd, b, s, or t.");
        return false;
    }

    int current = 0, orig = 0;
    if (col == "pd") { current = std::atoi(shipRow[0][1].c_str()); orig = std::atoi(shipRow[0][6].c_str()); }
    else if (col == "beam") { current = std::atoi(shipRow[0][2].c_str()); orig = std::atoi(shipRow[0][7].c_str()); }
    else if (col == "screen") { current = std::atoi(shipRow[0][3].c_str()); orig = std::atoi(shipRow[0][8].c_str()); }
    else if (col == "tube") { current = std::atoi(shipRow[0][4].c_str()); orig = std::atoi(shipRow[0][9].c_str()); }

    int maxRepair = orig - current;
    if (maxRepair <= 0)
    {
        Telemetry::getInstance().write("Attribute already at maximum.");
        return false;
    }

    int repairAmt = std::min(m_amount, maxRepair);
    int cost = repairAmt; // 1 BP per unit

    // Check BP
    int availBP = (owner == 'A') ? s.bpA : s.bpB;
    if (cost > availBP)
    {
        Telemetry::getInstance().write("Insufficient BP. Need " + std::to_string(cost) + 
            ", have " + std::to_string(availBP));
        return false;
    }

    // Apply repair
    int newVal = current + repairAmt;
    db.exec("UPDATE ships SET " + col + "=" + std::to_string(newVal) +
            " WHERE game_id=" + std::to_string(s.game_id) +
            " AND ship_code='" + db.esc(m_ship_code) + "'");

    // Deduct BP
    std::string bpCol = (owner == 'A') ? "bpA" : "bpB";
    db.exec("UPDATE game_state SET " + bpCol + "=" + bpCol + "-" + std::to_string(cost) +
            " WHERE game_id=" + std::to_string(s.game_id));

    Telemetry::getInstance().write("Repaired " + m_ship_code + " " + col + " by " +
        std::to_string(repairAmt) + " (cost: " + std::to_string(cost) + " BP)");
    return true;
}
