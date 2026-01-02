//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "combat_cancel_command.h"

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool CombatCancelCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Delete all uncommitted orders for this player
    db.exec(
        "DELETE FROM combat_orders WHERE game_id=" + 
        std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) + 
        "' AND committed=0");

    Telemetry::getInstance().write("Combat orders cancelled.");
    return true;
}
