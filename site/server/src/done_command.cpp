//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "done_command.h"

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

DoneCommand::Builder::Builder(StateMachine &sm) : m_sm(sm)
{
}

ICmd *DoneCommand::Builder::build()
{
    return new DoneCommand(m_sm);
}

DoneCommand::DoneCommand(StateMachine &sm) : m_sm(sm)
{
}

bool DoneCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char me = s.active_player.empty() ? 'A' : s.active_player[0];

    Db *m_db = m_sm.get_db();

    // Auto-advance until active player changes or game over
    int safety = 0;
    while (s.active_player == std::string(1, me) && !s.game_over && safety < 50)
    {
        advance_next(m_db, s);
        safety++;
    }

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
    save_game(m_db, s);

    return true;
}
