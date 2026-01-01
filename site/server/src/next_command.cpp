//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "next_command.h"

#include <sstream>

// #include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typedefs.h"

bool NextCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();

    // Validate it's this player's turn
    char requesting_player = StateMachine::getInstance().get_current_player();
    
    Logger::instance().info("[next] game_id=" + std::to_string(game_id) + 
                           ", s.game_id=" + std::to_string(s.game_id) +
                           ", s.active_player=" + s.active_player + 
                           ", requesting_player=" + std::string(1, requesting_player));
    
    if (s.active_player[0] != requesting_player)
    {
        Telemetry::getInstance().write("Error: It's not your turn (active player: " + 
                                       s.active_player + ")");
        return false;
    }

    std::string before_phase = s.phase_name();
    std::string before_player = s.active_player;
    int before_round = s.round;

    StateMachine::getInstance().advance_next(s);

    // Save the updated game state
    StateMachine::getInstance().save_game(s);

    std::ostringstream msg;
    msg << "Advanced: " << before_player << " / " << before_phase << " -> "
        << s.active_player << " / " << s.phase_name();
    if (s.round != before_round)
    {
        msg << " (round " << s.round << ")";
    }

    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    // If active player changed, notify the NEW active player
    // After advance, s.active_player is the new player (e.g., 'B')
    // The requester (ME) is the old player (e.g., 'A'), so THEM = the new player
    if (before_player != s.active_player)
    {
        Telemetry::getInstance().tell(PlayerTarget::THEM,
                        "⏰ It's YOUR turn! " + s.phase_name());
    }

    return true;
}
