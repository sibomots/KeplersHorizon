//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_cancel_command.h"

#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "ships.h"

bool BuildCancelCommand::invoke(void)
{
    // Redundancy comment
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    std::string draft_code = get_current_draft(game_id, active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to cancel");
        Telemetry::getInstance().write("Error: No current draft to cancel");
        return false;
    }

    delete_draft(game_id, active_player, draft_code);
    set_current_draft(game_id, active_player, "");
    Logger::instance().info("Canceled draft: " + draft_code);
    Telemetry::getInstance().write("Canceled draft: " + draft_code);

    return true;
}
