///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
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
        Logger::instance().info("Turn ended. Active player: " + s.active_player);
        
        Telemetry::write("Your turn has ended");
        Telemetry::tell(PlayerTarget::THEM, 
                       "⏰ It's YOUR turn! " + s.phase_name() + 
                       " (Round " + std::to_string(s.round) + ")");
        Telemetry::broadcast("Turn advanced to " + s.active_player);
    }

    // Save game state to persist changes
    save_game(m_db, s);

    return true;
}
