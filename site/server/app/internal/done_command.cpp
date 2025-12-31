//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "done_command.h"

// BUGBUG #include "game.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool DoneCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char me = s.active_player.empty() ? 'A' : s.active_player[0];

    // Auto-advance until active player changes or game over
    int safety = 0;

    // BUGBUG safety should be based on the last enum value of possible
    // phases of a turn.
    while (s.active_player == std::string(1, me) && !s.game_over && safety < 10)
    {
        StateMachine::getInstance().advance_next(s);
        safety++;
    }

    // NO, game over decision cannot be made until the count of
    // victory points on the onset of a new turn.
    if (s.game_over)
    {
        Logger::instance().info("Game Over during turn end");
        Telemetry::write("Game Over");
        Telemetry::broadcast("🏁 Game Over!");
    }
    else
    {
        char new_player = s.active_player.empty() ? 'A' : s.active_player[0];
        Logger::instance().info("Turn ended. Active player: " +
                                s.active_player);

        Telemetry::write("Your turn has ended");
        Telemetry::tell(PlayerTarget::THEM, "⏰ It's YOUR turn! " +
                                                s.phase_name() + " (Round " +
                                                std::to_string(s.round) + ")");
        Telemetry::broadcast("Turn advanced to " + s.active_player);
    }

    // Save game state to persist changes
    StateMachine::getInstance().save_game(s);

    return true;
}
