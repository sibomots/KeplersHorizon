//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "next_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

NextCommand::Builder::Builder(StateMachine &sm) : m_sm(sm)
{
}

ICmd *NextCommand::Builder::build()
{
    return new NextCommand(m_sm);
}

NextCommand::NextCommand(StateMachine &sm) : m_sm(sm)
{
}

bool NextCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();

    std::string before_phase = s.phase_name();
    std::string before_player = s.active_player;
    int before_round = s.round;

    advance_next(m_sm.get_db(), s);

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
