//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include <sstream>

#include "combat.h"
#include "db.h"
#include "done_command.h"
#include "logger.h"
#include "next_command.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool DoneCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    DoneParams_t params;
    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::DONE, &params,
                                                    inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char me = s.active_player.empty() ? 'A' : s.active_player[0];

    // Auto-advance until active player changes, game over, or blocked on combat
    // Maximum iterations: one per phase plus turn flip (PH_END_TURN + 2 to be
    // safe)
    int iterations = 0;
    const int max_iterations = PH_END_TURN + 2;

    while (s.active_player == std::string(1, me) && !s.game_over &&
           iterations < max_iterations)
    {
        StateMachine::getInstance().advance_next(s);
        iterations++;
    }

    // Check if we're blocked on combat
    if (s.phase_index == PH_RESOLVE_COMBAT &&
        s.active_player == std::string(1, me))
    {
        CombatEngine ce(s.game_id);
        auto combats = ce.get_active_combats();
        if (!combats.empty())
        {
            // Blocked on combat - don't advance turn, report combat pending
            std::ostringstream msg;
            msg << "TACTICAL: Combat pending! " << combats.size()
                << " active engagement(s) must be resolved.";
            for (const auto& c : combats)
            {
                msg << "\n  - Sector " << c.hex_id << " (Round " << c.round
                    << ")";
            }
            msg << "\nUse: combat order <ship> <target> <tactic> <power "
                   "allocation>";

            Telemetry::getInstance().write(msg.str());

            // Save state (still in combat phase)
            StateMachine::getInstance().save_game(s);
            return true;
        }
    }

    // Game over check
    if (s.game_over)
    {
        Logger::instance().info("Game Over during turn end");
        Telemetry::getInstance().write("Game Over");
        Telemetry::getInstance().broadcast(">> GAME OVER <<");
    }
    else
    {
        char new_player = s.active_player.empty() ? 'A' : s.active_player[0];
        Logger::instance().info("Turn ended. Active player: " +
                                s.active_player);

        // Look up opponent's username for the broadcast
        DatabaseManager& db = DatabaseManager::getInstance();
        std::string oppUser = s.active_player;
        auto oppRow = db.query("SELECT u.username FROM users u "
                               "JOIN game_seats gs ON gs.user_id = u.id "
                               "WHERE gs.game_id=" +
                               std::to_string(s.game_id) + " AND gs.seat='" +
                               s.active_player + "'");
        if (!oppRow.empty())
        {
            oppUser = oppRow[0][0];
        }

        Telemetry::getInstance().write(
            "COMMAND: Your turn has ended. Standing down.");
        // After advance, s.active_player is the NEW active player
        // The requester (ME) is the OLD player, so THEM = the new player
        Telemetry::getInstance().tell(PlayerTarget::THEM,
                                      "COMMAND: You are now in command! " +
                                          s.phase_name() + " (Round " +
                                          std::to_string(s.round) + ")");
        Telemetry::getInstance().broadcast("COMMAND: " + oppUser +
                                           "'s turn has begun.");
    }

    // Save game state to persist changes
    StateMachine::getInstance().save_game(s);

    return true;
}
//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

bool NextCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    NextParams_t params;
    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::NEXT, &params,
                                                    inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();

    std::string before_phase = s.phase_name();
    std::string before_player = s.active_player;
    int before_round = s.round;

    StateMachine::getInstance().advance_next(s);

    // Save the updated game state
    StateMachine::getInstance().save_game(s);

    std::ostringstream msg;
    msg << "New Phase: " << s.phase_name();
    if (s.round != before_round)
    {
        msg << " (round " << s.round << ")";
    }

    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    // If active player changed, notify the NEW active player
    // After advance, s.active_player is the new player (e.g., 'B')
    // The requester (ME) is the old player (e.g., 'A'), so THEM = the new
    // player
    if (before_player != s.active_player)
    {
        Telemetry::getInstance().tell(PlayerTarget::THEM,
                                      "It's YOUR turn! " + s.phase_name());
    }

    return true;
}
//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

// StartCommand removed - game initialization happens through room flow only
