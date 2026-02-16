///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "score_command.h"

#include <format>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool ScoreCommand::invoke(void)
{
    show_overview();
    return true;
}

void ScoreCommand::show_overview()
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();
    char enemy = me ^ 0x03;

    DatabaseManager& db = DatabaseManager::instance();

    // Get player names
    std::string q = "SELECT u.username FROM users u "
                    "JOIN game_seats gs ON gs.user_id = u.id "
                    "WHERE gs.game_id=? AND gs.seat=?";

    auto my_seat = db.Query(q, {game_id, me});
    auto enemy_seat = db.Query(q, {game_id, enemy});

    std::string my_name = my_seat.empty() ? "You" : my_seat[0][0];
    std::string enemy_name = enemy_seat.empty() ? "Enemy" : enemy_seat[0][0];

    // Get active player name
    auto active_seat = db.Query(q, {game_id, s.active_player});
    std::string active_name =
        active_seat.empty() ? "Unknown" : active_seat[0][0];

    // VP needed to win
    int vp_needed = 3;

    int my_vp = (KH_EQU(me, 'A')) ? s.vpA : s.vpB;
    int enemy_vp = (KH_EQU(me, 'A')) ? s.vpB : s.vpA;
    int my_credits = (KH_EQU(me, 'A')) ? s.creditsA : s.creditsB;

    // Get tech level
    auto tech_row =
        db.Query("SELECT COALESCE(MAX(tech_level), 0) FROM ships WHERE "
                 "game_id=? AND owner=? AND destroyed_at IS NULL",
                 {game_id, me});
    int tech_level = tech_row.empty() ? 0 : std::atoi(tech_row[0][0].c_str());

    // Count ships
    std::string sq = "SELECT COUNT(*) FROM ships WHERE game_id=? AND owner=? "
                     "AND destroyed_at IS NULL";
    auto myShips = db.Query(sq, {game_id, me});
    auto enemyShips = db.Query(sq, {game_id, enemy});
    int my_ships = myShips.empty() ? 0 : std::atoi(myShips[0][0].c_str());
    int enemy_ships =
        enemyShips.empty() ? 0 : std::atoi(enemyShips[0][0].c_str());

    std::ostringstream out;
    out << "────────────────────────────────────────────\n"
        << std::format(" Round: {:<6} Phase: {:<12} Turn: {}\n", s.round,
                       s.phase_name(), active_name)
        << "────────────────────────────────────────────\n"
        << std::format(" {:>16}  {:>16}\n", my_name, enemy_name)
        << std::format(" Credits {:>8}  {:>16}\n",
                       (KH_EQU(me, 'A') ? s.creditsA : s.creditsB),
                       (KH_EQU(me, 'A') ? s.creditsB : s.creditsA))
        << std::format(" VP      {:>8}  {:>16}\n", my_vp, enemy_vp)
        << std::format(" Ships   {:>8}  {:>16}\n", my_ships, enemy_ships)
        << std::format(" Tech    {:>8}\n", tech_level)
        << "────────────────────────────────────────────\n"
        << std::format(" Victory: {} to win\n", vp_needed);

    if (s.game_over)
    {
        std::string winner_name =
            StateMachine::instance().get_player_name(game_id, s.winner);
        out << " GAME OVER - Winner: " << winner_name << "\n";
    }

    out << "────────────────────────────────────────────";
    Telemetry::instance().write(out.str());
}
