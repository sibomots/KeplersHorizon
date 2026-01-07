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
#include "start_command.h"
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

bool StartCommand::invoke(void)
{
    // Scenario type to string mapping (indexed by ScenarioType enum)
    static const char* scenario_names[] = {
        nullptr,    // UNDEFINED
        "learning", // LEARNING
        "basic",    // BASIC
        "advanced"  // ADVANCED
    };

    DatabaseManager& db = DatabaseManager::getInstance();

    // Check if user is already in a game (from room flow)
    int current_game_id = StateMachine::getInstance().get_game_id();
    int current_user_id = StateMachine::getInstance().get_current_user_id();

    if (current_game_id > 0 && current_user_id > 0)
    {
        // Check if there's a game_seat for this user in this game
        auto seat_check =
            db.query("SELECT seat FROM game_seats WHERE game_id=" +
                     std::to_string(current_game_id) +
                     " AND user_id=" + std::to_string(current_user_id));
        if (!seat_check.empty())
        {
            // User already has a game from room flow - don't create another!
            Logger::instance().info(
                "Game already started via room flow (game_id=" +
                std::to_string(current_game_id) + ")");
            Telemetry::getInstance().write(
                "Game already started! Use 'status' to view game state.");
            return true; // Not an error, just already done
        }
    }

    // Validate scenario type
    if (m_scenario <= ScenarioType::UNDEFINED ||
        m_scenario >= sizeof(scenario_names) / sizeof(scenario_names[0]))
    {
        Logger::instance().error("Unknown scenario type");
        return false;
    }

    const std::string sc_str(scenario_names[m_scenario]);

    Logger::instance().info("Initializing game scenario: " + sc_str);

    // Create new game state for the scenario
    GameState s =
        StateMachine::getInstance().new_game_state_for_scenario(sc_str);

    // Create a new game in the database
    std::string state = s.to_json();
    std::string ins = "INSERT INTO games(scenario,state_json) VALUES('" +
                      db.esc(sc_str) + "','" + db.esc(state) + "')";
    db.exec(ins);

    // Get the new game ID
    auto r = db.query("SELECT LAST_INSERT_ID()");
    int new_game_id = std::atoi(r[0][0].c_str());
    s.game_id = new_game_id;

    // Set the game_id in the StateMachine so subsequent commands use it
    StateMachine::getInstance().set_game_id(new_game_id);

    // Create game_seats entry for the player who started the game (they are
    // player A)
    int starter_user_id = StateMachine::getInstance().get_current_user_id();
    if (starter_user_id > 0)
    {
        db.exec("INSERT INTO game_seats(game_id, user_id, seat) VALUES(" +
                std::to_string(new_game_id) + "," +
                std::to_string(starter_user_id) + ",'A')");
        Logger::instance().info("Created game_seat: user " +
                                std::to_string(starter_user_id) +
                                " is player A");
    }

    // Note: Map data (star_systems, warplines, hexes) is shared across all
    // games using map_id=1. No need to copy - map queries always use map_id=1.

    // Clear any existing drafts and ships for this game
    db.exec("DELETE FROM drafts WHERE game_id=" + std::to_string(new_game_id));
    db.exec("DELETE FROM ships WHERE game_id=" + std::to_string(new_game_id));
    set_current_draft(new_game_id, 'A', "");
    set_current_draft(new_game_id, 'B', "");

    // Save the initialized game state
    StateMachine::getInstance().save_game(s);

    Logger::instance().info(
        "Game initialized: " + sc_str + " scenario (game_id=" +
        std::to_string(new_game_id) + "), Round " + std::to_string(s.round) +
        ", Phase: " + s.phase_name() + ", CR A=" + std::to_string(s.creditsA) +
        " B=" + std::to_string(s.creditsB));

    Telemetry::getInstance().write("Game initialized: " + sc_str + " scenario");
    Telemetry::getInstance().write("Round " + std::to_string(s.round) +
                                   ", Phase: " + s.phase_name());
    Telemetry::getInstance().write(
        "Starting credits: " + std::to_string(s.creditsA) + " CR");

    return true;
}
