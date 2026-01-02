//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "combat_commit_command.h"

#include <sstream>
#include "combat.h"
#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool CombatCommitCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get the active combat hex
    auto gameRow = db.query(
        "SELECT active_combat_hex FROM games WHERE id=" + std::to_string(s.game_id));
    
    std::string activeHex;
    if (!gameRow.empty() && !gameRow[0][0].empty())
    {
        activeHex = gameRow[0][0];
    }

    // Get all hexes with uncommitted orders for this player
    auto hexRows = db.query(
        "SELECT DISTINCT s.at_hex FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND s.ship_code=co.ship_code "
        "WHERE co.game_id=" + std::to_string(s.game_id) +
        " AND co.owner='" + std::string(1, owner) + "' AND co.committed=0");

    if (hexRows.empty())
    {
        Telemetry::getInstance().write("No orders to commit.");
        return true;
    }

    // Validate orders are for active hex (if one is set)
    if (!activeHex.empty())
    {
        for (const auto& row : hexRows)
        {
            if (row[0] != activeHex)
            {
                Telemetry::getInstance().write(
                    "Error: Orders pending for hex " + row[0] + 
                    " but active combat is in hex " + activeHex);
                return false;
            }
        }
    }

    // Mark all uncommitted orders as committed
    db.exec(
        "UPDATE combat_orders SET committed=1 WHERE game_id=" + 
        std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) + 
        "' AND committed=0");

    Telemetry::getInstance().write("Combat orders committed.");

    // Check each affected hex for resolution
    CombatEngine ce(s.game_id);
    for (const auto& row : hexRows)
    {
        std::string hex_id = row[0];
        auto cs = ce.get_combat_state(hex_id);
        
        if (ce.all_orders_committed(hex_id, cs.round))
        {
            std::string result = ce.resolve_round(hex_id);
            Telemetry::getInstance().write(result);
        }
        else
        {
            Telemetry::getInstance().write("Hex " + hex_id + ": Waiting for opponent.");
        }
    }
    return true;
}
