//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "score_command.h"

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
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();
    char enemy = (me == 'A') ? 'B' : 'A';

    DatabaseManager& db = DatabaseManager::getInstance();

    // Get active player's username (whose turn it is)
    std::string active_username = "Unknown";
    auto active_seat = db.query(
        "SELECT u.username FROM users u "
        "JOIN game_seats gs ON gs.user_id = u.id "
        "WHERE gs.game_id=" +
        std::to_string(game_id) + " AND gs.seat='" + s.active_player + "'");
    if (!active_seat.empty())
        active_username = active_seat[0][0];

    // VP needed based on scenario
    int vp_needed = 3;
    if (s.scenario == "learning")
        vp_needed = 1;
    else if (s.scenario == "basic")
        vp_needed = 2;

    int my_vp = (me == 'A') ? s.vpA : s.vpB;
    int enemy_vp = (me == 'A') ? s.vpB : s.vpA;
    int my_credits = (me == 'A') ? s.creditsA : s.creditsB;

    // Get tech level from ships (highest tech_level among player's ships)
    auto tech_row = db.query(
        "SELECT COALESCE(MAX(tech_level), 0) FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, me) +
        "' AND destroyed_at IS NULL");
    int tech_level = tech_row.empty() ? 0 : std::atoi(tech_row[0][0].c_str());

    std::ostringstream out;
    out << "TURN: " << active_username << "  PHASE: " << s.phase_name()
        << "  ROUND: " << s.round << "\n"
        << "CREDITS: " << my_credits << " CR  TECH: L" << tech_level << "\n"
        << "VICTORY: You " << my_vp << ", Enemy " << enemy_vp << " (need "
        << vp_needed << " to win)\n";

    Telemetry::getInstance().write(out.str());
}
