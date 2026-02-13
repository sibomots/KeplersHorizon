///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include <sstream>

#include "ce.h"
#include "db.h"
#include "done_command.h"
#include "logger.h"
#include "next_command.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include <format>

bool DoneCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    DoneParams_t params;
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::DONE,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::instance().get_game_state();
    char me = s.active_player.empty() ? 'A' : s.active_player[0];

    // Auto-advance until active player changes, game over, or blocked on combat
    // Maximum iterations: one per phase plus turn flip (PH_END_TURN + 2 to be
    // safe)
    int iterations = 0;
    const int max_iterations = PH_END_TURN + 2;

    while (KH_EQU(s.active_player, std::string(1, me))
           && !s.game_over && iterations < max_iterations)
    {
        StateMachine::instance().advance_next(s);
        iterations++;
    }

    // Check if we're blocked on combat
    if (KH_EQU(s.phase_index, PH_RESOLVE_COMBAT)
        && KH_EQU(s.active_player, std::string(1, me)))
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

            Telemetry::instance().write(msg.str());

            // Save state (still in combat phase)
            StateMachine::instance().save_game(s);
            return true;
        }
    }

    // Check for pending retreats
    DatabaseManager& db = DatabaseManager::instance();
    std::string qq = "SELECT ship_code FROM ships WHERE game_id=? AND owner=? "
                     " AND escape_pending=1 AND destroyed_at IS NULL";
    auto retreat_pending = db.Query(qq, {s.game_id, me});

    if (!retreat_pending.empty())
    {
        std::ostringstream msg;
        msg << "TACTICAL: Retreat pending! " << retreat_pending.size()
            << " ship(s) must complete withdrawal:";
        for (const auto& r : retreat_pending)
        {
            msg << "\n  - " << r[0];
        }
        msg << "\nUse: retreat <ship> <hex>";
        Telemetry::instance().write(msg.str());
        StateMachine::instance().save_game(s);
        return true;
    }

    // Game over check
    if (s.game_over)
    {
        Telemetry::instance().broadcast(">> GAME OVER <<");
    }
    else
    {
        char new_player = s.active_player.empty() ? 'A' : s.active_player[0];
        Logger::instance().info("[TURN] Turn ended. Active player: " +
                                s.active_player);

        // Look up opponent's username for the broadcast
        DatabaseManager& db = DatabaseManager::instance();
        std::string oppUser = s.active_player;

        std::string qq = "SELECT u.username FROM users u "
                         " JOIN game_seats gs ON gs.user_id = u.id "
                         " WHERE gs.game_id=? AND gs.seat=?";

        auto oppRow = db.Query(qq, {s.game_id, s.active_player});

        if (!oppRow.empty())
        {
            oppUser = oppRow[0][0];
        }

        std::string phase_name = s.phase_name();
        int round = s.round;

        Telemetry::instance().write("Your turn has ended.");
        // After advance, s.active_player is the NEW active player
        // The requester (ME) is the OLD player, so THEM = the new player
        Telemetry::instance().broadcast(
          std::format("{}'s turn has begun.", oppUser));
        Telemetry::instance().tell(PlayerTarget::THEM,
               std::format("Your Turn: {} Round {}", phase_name, round));
    }

    // Save game state to persist changes
    StateMachine::instance().save_game(s);
    return true;
}

bool NextCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    NextParams_t params;
    std::string inhibit_error;

    typedef struct
    {
        std::string player;
        int phase;
        int round;
    } TurnMetrics;

    if (!StateMachine::instance().check_inhibits(CommandID::NEXT,
                                                 inhibit_error))
    {
        Telemetry::instance().write(inhibit_error);
        return false;
    }

    TurnMetrics before_advance;
    TurnMetrics after_advance;

    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();

    before_advance.phase = s.phase_index;
    before_advance.round = s.round;
    before_advance.player = s.active_player;

    // Before we try to advance to the next phase
    // let's save the current phase and round.
    StateMachine::instance().advance_next(s);

    // Save the updated game state
    StateMachine::instance().save_game(s);

    after_advance.phase = s.phase_index;
    after_advance.round = s.round;
    after_advance.player = s.active_player;

    bool phase_change = (before_advance.phase != after_advance.phase);
    bool round_change = (before_advance.round != after_advance.round);
    bool player_change =
        (before_advance.player.compare(after_advance.player) != 0);

    std::ostringstream msg;
    bool msg_updated = false;
    if (player_change)
    {
        if (player_change)
        {
            Telemetry::instance().tell(PlayerTarget::THEM,
                                       "It's YOUR turn! " + s.phase_name());
        }
    }
    else
    {
        if (phase_change)
        {
            msg << "Phase: " << s.phase_name();
            if (round_change)
            {
                msg << " Round: " << s.round;
            }
            msg_updated = true;
        }
        else
        {
            if (round_change)
            {
                msg << "Round: " << s.round;
                msg_updated = true;
            }
        }
    }
    if (msg_updated)
    {
        Telemetry::instance().write(msg.str());
    }
    return true;
}
