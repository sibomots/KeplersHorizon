//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "save_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

//------------------------------------------------------------------------------
// SaveCommand - creates a bookmark for the current game
//------------------------------------------------------------------------------
bool SaveCommand::invoke()
{
    if (m_show_usage)
    {
        Telemetry::instance().write(
            "Usage: save <NAME> - save current game under label NAME");
        return true;
    }

    StateMachine& sm = StateMachine::instance();
    int game_id = sm.get_game_id();
    int user_id = sm.get_current_user_id();
    GameState gs = sm.get_game_state();

// BUGBUG
#if 0
    // Check phase - only allow during BUILD_SHIPS
    if (gs.phase_index != 0)
    {
        Telemetry::instance().write(
            "SAVE: Only allowed during BUILD_SHIPS phase.");
        return false;
    }
#endif

    if (m_name.empty())
    {
        Telemetry::instance().write(
            "Usage: save <NAME> - save current game under label NAME");
        return true;
    }

    DatabaseManager& db = DatabaseManager::instance();

    // Check if save name already exists for this user
    auto existing = db.Query(
        "SELECT id FROM saved_games WHERE user_id=? AND save_name=?",
        {user_id, m_name});

    if (!existing.empty())
    {
        // Update existing save to point to current game
        fprintf(stderr, "stoi 5\n");
        int sgid = std::stoi(existing[0][0]);
        db.Exec("UPDATE saved_games SET game_id=?, saved_at=NOW() WHERE id=?",
                {game_id, sgid});
        Telemetry::instance().write("SAVE: Updated '" + m_name +
                                       "' to point to current game (Turn " +
                                       std::to_string(gs.round) + ").");
    }
    else
    {
        // Create new save
        db.Exec("INSERT INTO saved_games(user_id, save_name, game_id) VALUES(?,?,?)",
                {user_id, m_name, game_id});
        Telemetry::instance().write("SAVE: Game saved as '" + m_name +
                                       "' (Turn " + std::to_string(gs.round) +
                                       ").");
    }
    return true;
}

//------------------------------------------------------------------------------
// LoadCommand - lists saves or initiates a two-factor load request
//------------------------------------------------------------------------------
bool LoadCommand::invoke()
{
    StateMachine& sm = StateMachine::instance();
    int game_id = sm.get_game_id();
    int user_id = sm.get_current_user_id();
    char player = sm.get_current_player();
    GameState gs = sm.get_game_state();

    DatabaseManager& db = DatabaseManager::instance();

    // If just listing saves
    if (m_list_saves || m_name.empty())
    {
        auto saves = db.Query(
            "SELECT sg.save_name, sg.game_id, g.module_id, "
            "JSON_UNQUOTE(JSON_EXTRACT(g.state_json, '$.round')) as round, "
            "sg.saved_at FROM saved_games sg "
            "JOIN games g ON sg.game_id = g.id "
            "WHERE sg.user_id=? ORDER BY sg.saved_at DESC LIMIT 10",
            {user_id});

        if (saves.empty())
        {
            Telemetry::instance().write("LOAD: No saved games found.");
            Telemetry::instance().write(
                "Usage: load <NAME> - load a saved game by name");
            return false;
        }

        std::ostringstream out;
        out << "LOAD: Your saved games:\n";
        for (const auto& row : saves)
        {
            out << "  " << row[0] << " (Turn " << row[3] << ", " << row[2]
                << ")\n";
        }
        out << "Usage: load <NAME> - load a saved game by name";
        Telemetry::instance().write(out.str());
        return true;
    }

    // Check phase - only allow during BUILD_SHIPS
    if (gs.phase_index != 0)
    {
        Telemetry::instance().write(
            "LOAD: Only allowed during BUILD_SHIPS phase.");
        return false;
    }

    // Find the saved game
    auto saves = db.Query(
        "SELECT sg.game_id, g.module_id, "
        "JSON_UNQUOTE(JSON_EXTRACT(g.state_json, '$.round')) as round "
        "FROM saved_games sg "
        "JOIN games g ON sg.game_id = g.id "
        "WHERE sg.user_id=? AND sg.save_name=?",
        {user_id, m_name});

    if (saves.empty())
    {
        Telemetry::instance().write("LOAD: No saved game named '" + m_name +
                                       "' found.");
        return false;
    }

    fprintf(stderr, "stoi 6\n");
    int target_game_id = std::stoi(saves[0][0]);
    fprintf(stderr, "stoi 7\n");
    int module_id = std::atoi(saves[0][1].c_str());
    std::string round = saves[0][2];
    (void)module_id;

    // Can't load the same game we're in
    if (target_game_id == game_id)
    {
        Telemetry::instance().write("LOAD: You're already in that game.");
        return false;
    }

    // Check for existing pending request
    auto pending = db.Query(
        "SELECT requester, save_name FROM load_requests WHERE game_id=?",
        {game_id});

    if (!pending.empty())
    {
        std::string existing_requester = pending[0][0];
        std::string existing_name = pending[0][1];

        // Is this the same request? If so, this is the confirmation!
        if (existing_name == m_name && existing_requester[0] != player)
        {
            // This is the second player confirming - execute the load!
            Logger::instance().info("[LOAD] Second player confirmed load: " + m_name);

            // Get both user IDs from game_seats
            auto seats =
                db.Query("SELECT user_id FROM game_seats WHERE game_id=?",
                         {game_id});

            // Update both sessions to new game_id
            for (const auto& seat : seats)
            {
                fprintf(stderr, "stoi 8\n");
                int uid = std::stoi(seat[0]);
                db.Exec("UPDATE sessions SET game_id=? WHERE user_id=?",
                        {target_game_id, uid});
            }

            // Clear the pending request
            db.Exec("DELETE FROM load_requests WHERE game_id=?",
                    {game_id});

            // Notify both players
            std::string msg = "COMMAND: Game '" + m_name +
                              "' loaded. Resuming Turn " + round + ".";
            Telemetry::instance().add_tell(target_game_id, 'A', msg);
            Telemetry::instance().add_tell(target_game_id, 'B', msg);
            Telemetry::instance().write(msg);
            return true;
        }
        else
        {
            // Different request pending
            Telemetry::instance().write("LOAD: A load request for '" +
                                           existing_name +
                                           "' is pending. "
                                           "Type 'reject " +
                                           existing_name + "' first.");
            return true;
        }
    }

    // Create new load request
    db.Exec("INSERT INTO load_requests(game_id, requester, requester_user_id, "
            "target_game_id, save_name) VALUES(?,?,?,?,?)",
            {game_id, player, user_id, target_game_id, m_name});

    // Get requester's username
    auto username_row = db.Query("SELECT username FROM users WHERE id=?",
                                 {user_id});
    std::string username = username_row.empty() ? "Player" : username_row[0][0];

    // Notify the requester
    Telemetry::instance().write("LOAD: Requested to load '" + m_name +
                                   "' (Turn " + round +
                                   "). "
                                   "Waiting for other player to accept.");

    // Notify the opponent
    char opponent = (player == 'A') ? 'B' : 'A';
    std::string opponent_msg = "COMMAND: " + username + " proposes loading '" +
                               m_name + "' (Turn " + round +
                               ").\n"
                               "Type 'accept " +
                               m_name + "' or 'reject " + m_name + "'.";
    Telemetry::instance().add_tell(game_id, opponent, opponent_msg);

    return true;
}

//------------------------------------------------------------------------------
// AcceptCommand - confirms a pending load request
//------------------------------------------------------------------------------
bool AcceptCommand::invoke()
{
    StateMachine& sm = StateMachine::instance();
    int game_id = sm.get_game_id();
    char player = sm.get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    // Find pending request
    auto pending = db.Query("SELECT requester, target_game_id, save_name FROM "
                            "load_requests WHERE game_id=?",
                            {game_id});

    if (pending.empty())
    {
        Telemetry::instance().write("ACCEPT: No pending load request.");
        return false;
    }

    std::string requester = pending[0][0];
    fprintf(stderr, "stoi 9\n");
    int target_game_id = std::stoi(pending[0][1]);
    std::string save_name = pending[0][2];

    // Check if name matches (if provided)
    if (!m_name.empty() && m_name != save_name)
    {
        Telemetry::instance().write("ACCEPT: Pending request is for '" +
                                       save_name + "', not '" + m_name + "'.");
        return false;
    }

    // Can't accept your own request
    if (requester[0] == player)
    {
        Telemetry::instance().write(
            "ACCEPT: Waiting for the other player to accept your request.");
        return false;
    }

    // Execute the load!
    Logger::instance().info("[LOAD] Accept confirmed, switching to game " +
                            std::to_string(target_game_id));

    // Get round from target game
    auto target_state = db.Query("SELECT JSON_UNQUOTE(JSON_EXTRACT(state_json, "
                                 "'$.round')) FROM games WHERE id=?",
                                 {target_game_id});
    std::string round = target_state.empty() ? "?" : target_state[0][0];

    // Get both user IDs from game_seats
    auto seats = db.Query("SELECT user_id FROM game_seats WHERE game_id=?",
                          {game_id});

    // Update both sessions to new game_id
    for (const auto& seat : seats)
    {
        fprintf(stderr, "stoi 10\n");
        int uid = std::stoi(seat[0]);
        db.Exec("UPDATE sessions SET game_id=? WHERE user_id=?",
                {target_game_id, uid});
    }

    // Clear the pending request
    db.Exec("DELETE FROM load_requests WHERE game_id=?",
            {game_id});


    auto conflict_rows = db.Query(
    "SELECT s.at_hex "
    " FROM ships s "
    " JOIN star_systems ss ON ss.hex_id = s.at_hex "
    " WHERE s.game_id=? "
    " AND s.destroyed_at IS NULL "
    " AND s.racked_in IS NULL "
    " AND s.at_hex IS NOT NULL AND s.at_hex <> '' "
    " AND s.owner IN ('A','B') "
    " AND ss.is_base = 1 "
    " GROUP BY s.at_hex "
    " HAVING COUNT(DISTINCT s.owner) = 2 "
    " LIMIT 1",
    {target_game_id});

    std::string extra_msg;
    if (!conflict_rows.empty())
    {
        // Force combat phase - update state_json
        db.Exec("UPDATE games SET state_json = JSON_SET(state_json, "
                "'$.phase_index', 3) WHERE id=?",
                {target_game_id});
        extra_msg = "\nALERT: Ships in conflict detected at " +
                    conflict_rows[0][0] + ". Entering combat phase.";
    }

    // Notify both players
    std::string msg = "COMMAND: Game '" + save_name +
                      "' loaded. Resuming Turn " + round + "." + extra_msg;
    Telemetry::instance().add_tell(target_game_id, 'A', msg);
    Telemetry::instance().add_tell(target_game_id, 'B', msg);
    Telemetry::instance().write(msg);
    return true;
}

//------------------------------------------------------------------------------
// RejectCommand - declines a pending load request
//------------------------------------------------------------------------------
bool RejectCommand::invoke()
{
    StateMachine& sm = StateMachine::instance();
    int game_id = sm.get_game_id();

    DatabaseManager& db = DatabaseManager::instance();

    // Find pending request
    auto pending = db.Query("SELECT requester, requester_user_id, save_name "
                            "FROM load_requests WHERE game_id=?",
                            {game_id});

    if (pending.empty())
    {
        Telemetry::instance().write("REJECT: No pending load request.");
        return false;
    }

    std::string requester = pending[0][0];
    fprintf(stderr, "stoi 11\n");
    int requester_user_id = std::stoi(pending[0][1]);
    std::string save_name = pending[0][2];

    // Check if name matches (if provided)
    if (!m_name.empty() && m_name != save_name)
    {
        Telemetry::instance().write("REJECT: Pending request is for '" +
                                       save_name + "', not '" + m_name + "'.");
        return false;
    }

    // Get rejector's username
    int user_id = sm.get_current_user_id();
    auto username_row = db.Query("SELECT username FROM users WHERE id=?",
                                 {user_id});
    std::string username = username_row.empty() ? "Player" : username_row[0][0];

    // Delete the request
    db.Exec("DELETE FROM load_requests WHERE game_id=?",
            {game_id});

    // Notify the rejector
    Telemetry::instance().write("REJECT: Load request for '" + save_name +
                                   "' rejected.");

    // Notify the requester
    char requester_player = requester[0];
    std::string requester_msg =
        "COMMAND: " + username + " rejected loading '" + save_name + "'.";
    Telemetry::instance().add_tell(game_id, requester_player, requester_msg);

    return true;
}
