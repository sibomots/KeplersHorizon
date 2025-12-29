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
#include "next_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

NextCommand::Builder::Builder(Db *db, int game_id)
    : m_db(db), m_game_id(game_id)
{
}

ICmd *NextCommand::Builder::build()
{
    return new NextCommand(m_db, m_game_id);
}

NextCommand::NextCommand(Db *db, int game_id) : m_db(db), m_game_id(game_id)
{
}

bool NextCommand::invoke(void)
{
    GameState s = load_game(m_db, m_game_id);
    
    std::string before_phase = s.phase_name();
    std::string before_player = s.active_player;
    int before_round = s.round;

    advance_next(m_db, s);

    std::ostringstream msg;
    msg << "Advanced: " << before_player << " / " << before_phase << " -> "
        << s.active_player << " / " << s.phase_name();
    if (s.round != before_round)
    {
        msg << " (round " << s.round << ")";
    }

    Logger::instance().info(msg.str());
    Telemetry::write(msg.str());

    // If active player changed, notify the new player
    if (before_player != s.active_player)
    {
        Telemetry::tell(PlayerTarget::THEM, 
                       "⏰ It's YOUR turn! " + s.phase_name());
    }

    return true;
}
