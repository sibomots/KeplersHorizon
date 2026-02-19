///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "quit_command.h"

#include <ctime>
#include <format>
#include <sstream>

#include "autonomy_agency.h"
#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool QuitCommand::invoke(void)
{
    int game_id = StateMachine::instance().get_game_id();
    int user_id = StateMachine::instance().get_current_user_id();
    char quitter = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // -------------------------------------------------------
    // 3a. Auto-save with lexer-safe name (letters, digits, dashes only)
    // -------------------------------------------------------
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    std::string save_name = std::format("autosaved-{:04d}-{:02d}-{:02d}-{:02d}-{:02d}",
                                        t->tm_year + 1900, t->tm_mon + 1,
                                        t->tm_mday, t->tm_hour, t->tm_min);

    // Check for existing auto-save from quit, update or insert
    std::vector<std::vector<std::string>> existing =
        db.Query("SELECT id FROM saved_games WHERE user_id=? "
                 "AND save_name LIKE 'autosaved-%'",
                 {user_id});

    if (!existing.empty())
    {
        db.Exec("UPDATE saved_games SET game_id=?, save_name=?, saved_at=NOW() "
                "WHERE id=?",
                {game_id, save_name, std::stoi(existing[0][0])});
    }
    else
    {
        db.Exec("INSERT INTO saved_games(user_id, save_name, game_id) "
                "VALUES(?,?,?)",
                {user_id, save_name, game_id});
    }

    Logger::instance().info(std::format("QUIT: Auto-saved game as '{}'", save_name));

    // -------------------------------------------------------
    // 3b. Nullify combat state (ships remain, combat re-triggers on reload)
    // -------------------------------------------------------
    db.Exec("DELETE FROM combat_state WHERE game_id=?", {game_id});
    db.Exec("DELETE FROM combat_orders WHERE game_id=?", {game_id});
    db.Exec("DELETE FROM pending_damage WHERE game_id=?", {game_id});

    // -------------------------------------------------------
    // 3c. Notify the other player
    // -------------------------------------------------------
    char opponent = (quitter == 'A') ? 'B' : 'A';
    Telemetry::instance().add_tell(game_id, opponent, LC_TURN_OPP_LEFT_GAME);

    // -------------------------------------------------------
    // 3d. Clear session game_id for all players in this game
    // -------------------------------------------------------
    db.Exec("UPDATE sessions SET game_id=NULL WHERE game_id=?", {game_id});

    // -------------------------------------------------------
    // 3e. Update room status
    // -------------------------------------------------------
    db.Exec("UPDATE rooms SET status='finished' WHERE game_id=?", {game_id});

    // -------------------------------------------------------
    // 3f. Reset AI game context (thread stays alive)
    // -------------------------------------------------------
    AutonomyAgency::instance().configure(0, '\0');

    // -------------------------------------------------------
    // 3g. Send LOBBY signal + clear StateMachine
    // -------------------------------------------------------
    std::string msg = std::format(LC_TURN_TARGET_AUTO_SAVE_TO_LOBBY, save_name);
    Telemetry::instance().write(msg);

    StateMachine::instance().clear_game_session();

    return true;
}
