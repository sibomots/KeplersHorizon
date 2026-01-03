//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "combat_drafts_command.h"

#include <sstream>
#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool CombatDraftsCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Query uncommitted orders for this player, grouped by hex
    auto rows = db.query(
        "SELECT s.at_hex, co.ship_code, co.tactic, co.target_id, "
        "co.power_d, co.power_b, co.power_s, co.power_t, co.missiles_data "
        "FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND s.ship_code=co.ship_code "
        "WHERE co.game_id=" + std::to_string(s.game_id) +
        " AND co.owner='" + std::string(1, owner) + "' AND co.committed=0 AND s.destroyed_at IS NULL "
        "ORDER BY s.at_hex, co.ship_code");

    if (rows.empty())
    {
        Telemetry::getInstance().write("No pending combat orders.");
        return true;
    }

    std::ostringstream out;
    out << "Pending Combat Orders:\n";
    std::string lastHex;
    for (const auto& r : rows)
    {
        if (r[0] != lastHex)
        {
            out << "  Hex " << r[0] << ":\n";
            lastHex = r[0];
        }
        char tactic = r[2].empty() ? 'A' : r[2][0];
        std::string tacticName = (tactic == 'A') ? "Attack" : 
                                  (tactic == 'D') ? "Dodge" : "Escape";
        out << "    " << r[1] << ": " << tacticName
            << " -> " << (r[3].empty() ? "(none)" : r[3])
            << " [D=" << r[4] << " B=" << r[5] 
            << " S=" << r[6] << " T=" << r[7];
        if (!r[8].empty())
            out << " M=" << r[8];
        out << "]\n";
    }
    Telemetry::getInstance().write(out.str());
    return true;
}
