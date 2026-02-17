///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "db.h"
#include "logger.h"
#include "save_command.h"
#include "statemachine.h"
#include "telemetry.h"

#include <sstream>

//------------------------------------------------------------------------------
// cloneGameState - deep-copies all game data to a new game_id
// Returns true on success; newGameId receives the cloned game's id.
//------------------------------------------------------------------------------
static bool cloneGameState(int sourceGameId, int& newGameId)
{
    DatabaseManager& db = DatabaseManager::instance();
    newGameId = 0;

    std::vector<std::string> stmts;

    // 1. Clone the games row itself
    stmts.push_back(
        std::format("INSERT INTO games (room_id, module_id, state_json, "
                    "current_draft_A, current_draft_B, active_combat_hex, "
                    "vp_A, vp_B, winner, created_at) "
                    "SELECT room_id, module_id, state_json, "
                    "current_draft_A, current_draft_B, active_combat_hex, "
                    "vp_A, vp_B, winner, created_at "
                    "FROM games WHERE id = {}",
                    sourceGameId));

    // 2. Capture the new game_id in a session variable
    stmts.push_back("SET @kh_clone_gid = LAST_INSERT_ID()");

    // 3. game_seats
    stmts.push_back(
        std::format("INSERT INTO game_seats (game_id, user_id, seat) "
                    "SELECT @kh_clone_gid, user_id, seat "
                    "FROM game_seats WHERE game_id = {}",
                    sourceGameId));

    // 4. ships (must precede combat_orders and hex_objects)
    stmts.push_back(std::format(
        "INSERT INTO ships (game_id, owner, ship_code, ship_name, "
        "ship_type, tech_level, built_turn, "
        "pd, pd_max, beam, beam_max, screen, screen_max, "
        "tube, tube_max, missiles, missiles_max, sr, sr_max, "
        "lrs, dr, pd_spent, "
        "at_system, at_hex, racked_in, destroyed_at, escape_pending, "
        "cargo_ferrous, cargo_rare_earth, cargo_radioactive, "
        "cargo_crystalline, cargo_volatile, cargo_water, "
        "cargo_organic, cargo_exotic, cargo_missiles, "
        "cargo_capacity, created_at) "
        "SELECT @kh_clone_gid, owner, ship_code, ship_name, "
        "ship_type, tech_level, built_turn, "
        "pd, pd_max, beam, beam_max, screen, screen_max, "
        "tube, tube_max, missiles, missiles_max, sr, sr_max, "
        "lrs, dr, pd_spent, "
        "at_system, at_hex, racked_in, destroyed_at, escape_pending, "
        "cargo_ferrous, cargo_rare_earth, cargo_radioactive, "
        "cargo_crystalline, cargo_volatile, cargo_water, "
        "cargo_organic, cargo_exotic, cargo_missiles, "
        "cargo_capacity, created_at "
        "FROM ships WHERE game_id = {}",
        sourceGameId));

    // 5. combat_orders (FK to ships via game_id, owner, ship_code)
    stmts.push_back(std::format(
        "INSERT INTO combat_orders (game_id, ship_code, owner, round, "
        "tactic, target_id, power_d, power_b, power_s, power_t, "
        "missiles_data, committed) "
        "SELECT @kh_clone_gid, ship_code, owner, round, "
        "tactic, target_id, power_d, power_b, power_s, power_t, "
        "missiles_data, committed "
        "FROM combat_orders WHERE game_id = {}",
        sourceGameId));

    // 6. hex_objects (source_ship_id set NULL; ship PKs change in clone)
    stmts.push_back(std::format(
        "INSERT INTO hex_objects (game_id, hex_id, object_type, state, "
        "owner, discovered_by, source_ship_id, salvage_value, "
        "created_at, salvaged_at) "
        "SELECT @kh_clone_gid, hex_id, object_type, state, "
        "owner, discovered_by, NULL, salvage_value, "
        "created_at, salvaged_at "
        "FROM hex_objects WHERE game_id = {}",
        sourceGameId));

    // 7. game_events
    stmts.push_back(std::format(
        "INSERT INTO game_events (game_id, user_id, seq, created_at, "
        "command_text, result_text, state_json) "
        "SELECT @kh_clone_gid, user_id, seq, created_at, "
        "command_text, result_text, state_json "
        "FROM game_events WHERE game_id = {}",
        sourceGameId));

    // 8. drafts
    stmts.push_back(std::format(
        "INSERT INTO drafts (game_id, owner, ship_code, ship_name, "
        "ship_type, pd, beam, screen, tube, missiles, sr, created_at) "
        "SELECT @kh_clone_gid, owner, ship_code, ship_name, "
        "ship_type, pd, beam, screen, tube, missiles, sr, created_at "
        "FROM drafts WHERE game_id = {}",
        sourceGameId));

    // 9. sightings
    stmts.push_back(std::format(
        "INSERT INTO sightings (game_id, observer_owner, subject_owner, "
        "ship_code, ship_name, ship_type, at_system, "
        "last_seen_turn, created_at) "
        "SELECT @kh_clone_gid, observer_owner, subject_owner, "
        "ship_code, ship_name, ship_type, at_system, "
        "last_seen_turn, created_at "
        "FROM sightings WHERE game_id = {}",
        sourceGameId));

    // 10. codex_entries
    stmts.push_back(
        std::format("INSERT INTO codex_entries (game_id, player, system_name, "
                    "knowledge_level, last_updated_turn, notes) "
                    "SELECT @kh_clone_gid, player, system_name, "
                    "knowledge_level, last_updated_turn, notes "
                    "FROM codex_entries WHERE game_id = {}",
                    sourceGameId));

    // 11. combat_state
    stmts.push_back(
        std::format("INSERT INTO combat_state (game_id, hex_id, round, stage, "
                    "attacker_remains, stalemate_counter, "
                    "damage_assigned_A, damage_assigned_B, last_log) "
                    "SELECT @kh_clone_gid, hex_id, round, stage, "
                    "attacker_remains, stalemate_counter, "
                    "damage_assigned_A, damage_assigned_B, last_log "
                    "FROM combat_state WHERE game_id = {}",
                    sourceGameId));

    // 12. telemetry_queue
    stmts.push_back(std::format(
        "INSERT INTO telemetry_queue (game_id, target_player, message, "
        "created_at, sent_at, sent_to_A, sent_to_B) "
        "SELECT @kh_clone_gid, target_player, message, "
        "created_at, sent_at, sent_to_A, sent_to_B "
        "FROM telemetry_queue WHERE game_id = {}",
        sourceGameId));

    // 13. extract_operations
    stmts.push_back(std::format(
        "INSERT INTO extract_operations (game_id, ship_code, owner, "
        "location_type, location_id, resource_type, "
        "started_turn, completed, yield) "
        "SELECT @kh_clone_gid, ship_code, owner, "
        "location_type, location_id, resource_type, "
        "started_turn, completed, yield "
        "FROM extract_operations WHERE game_id = {}",
        sourceGameId));

    // 14. fabrication_queue
    stmts.push_back(std::format(
        "INSERT INTO fabrication_queue (game_id, player, ship_code, "
        "recipe, quantity, started_turn, completion_turn, "
        "materials_consumed, status) "
        "SELECT @kh_clone_gid, player, ship_code, "
        "recipe, quantity, started_turn, completion_turn, "
        "materials_consumed, status "
        "FROM fabrication_queue WHERE game_id = {}",
        sourceGameId));

    // 15. market_prices
    stmts.push_back(std::format(
        "INSERT INTO market_prices (game_id, resource_type, current_price, "
        "base_price, price_trend, total_bought, total_sold, "
        "last_updated_turn) "
        "SELECT @kh_clone_gid, resource_type, current_price, "
        "base_price, price_trend, total_bought, total_sold, "
        "last_updated_turn "
        "FROM market_prices WHERE game_id = {}",
        sourceGameId));

    // 16. market_history
    stmts.push_back(std::format(
        "INSERT INTO market_history (game_id, resource_type, price, turn) "
        "SELECT @kh_clone_gid, resource_type, price, turn "
        "FROM market_history WHERE game_id = {}",
        sourceGameId));

    // 17. resource_state
    stmts.push_back(std::format(
        "INSERT INTO resource_state (game_id, resource_id, current_supply, "
        "max_supply, regen_rate, last_extracted_turn) "
        "SELECT @kh_clone_gid, resource_id, current_supply, "
        "max_supply, regen_rate, last_extracted_turn "
        "FROM resource_state WHERE game_id = {}",
        sourceGameId));

    // 18. facility_control
    stmts.push_back(std::format(
        "INSERT INTO facility_control (game_id, system_name, facility_type, "
        "controller, occupied_since, capture_progress) "
        "SELECT @kh_clone_gid, system_name, facility_type, "
        "controller, occupied_since, capture_progress "
        "FROM facility_control WHERE game_id = {}",
        sourceGameId));

    // 19. anomaly_events
    stmts.push_back(std::format(
        "INSERT INTO anomaly_events (game_id, system_name, anomaly_name, "
        "event_type, player, ship_code, turn, result_json) "
        "SELECT @kh_clone_gid, system_name, anomaly_name, "
        "event_type, player, ship_code, turn, result_json "
        "FROM anomaly_events WHERE game_id = {}",
        sourceGameId));

    // 20. discovered_salvageables
    stmts.push_back(std::format(
        "INSERT INTO discovered_salvageables (game_id, salvageable_id, "
        "discovered_by, discovered_turn, times_salvaged, depleted) "
        "SELECT @kh_clone_gid, salvageable_id, "
        "discovered_by, discovered_turn, times_salvaged, depleted "
        "FROM discovered_salvageables WHERE game_id = {}",
        sourceGameId));

    // 21. salvage_operations
    stmts.push_back(std::format(
        "INSERT INTO salvage_operations (game_id, system_name, ship_code, "
        "turn, resources_found, hazard_encountered) "
        "SELECT @kh_clone_gid, system_name, ship_code, "
        "turn, resources_found, hazard_encountered "
        "FROM salvage_operations WHERE game_id = {}",
        sourceGameId));

    // 22. pending_damage
    stmts.push_back(std::format(
        "INSERT INTO pending_damage (game_id, hex_id, round, ship_code, "
        "owner, damage_amount, created_at) "
        "SELECT @kh_clone_gid, hex_id, round, ship_code, "
        "owner, damage_amount, created_at "
        "FROM pending_damage WHERE game_id = {}",
        sourceGameId));

    // 23. hex_events
    stmts.push_back(
        std::format("INSERT INTO hex_events (game_id, hex_id, event_type, "
                    "modifier_value, spawned_turn, expires_turn) "
                    "SELECT @kh_clone_gid, hex_id, event_type, "
                    "modifier_value, spawned_turn, expires_turn "
                    "FROM hex_events WHERE game_id = {}",
                    sourceGameId));

    // 24. base_stars
    stmts.push_back(
        std::format("INSERT INTO base_stars (game_id, hex_id, owner) "
                    "SELECT @kh_clone_gid, hex_id, owner "
                    "FROM base_stars WHERE game_id = {}",
                    sourceGameId));

    // Execute the entire batch atomically
    bool ok = db.ExecTransaction(stmts);
    if (!ok)
    {
        Logger::instance().info(std::format(
            "[SAVE] cloneGameState FAILED for game_id={}", sourceGameId));
        return false;
    }

    // Retrieve the new game_id from the session variable
    auto idRows = db.Query("SELECT @kh_clone_gid");
    if (idRows.empty() || idRows[0][0].empty())
    {
        Logger::instance().info(
            "[SAVE] cloneGameState: could not retrieve new game_id");
        return false;
    }
    newGameId = std::stoi(idRows[0][0]);
    return true;
}

//------------------------------------------------------------------------------
// SaveCommand - clones game state and bookmarks the clone
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

    if (m_name.empty())
    {
        Telemetry::instance().write(
            "Usage: save <NAME> - save current game under label NAME");
        return true;
    }

    // Clone all game data to a new game_id
    int cloneId = 0;
    bool cloned = cloneGameState(game_id, cloneId);
    if (!cloned)
    {
        Telemetry::instance().write("SAVE: Failed to snapshot game state.");
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();

    // Check if save name already exists for this user
    auto existing =
        db.Query("SELECT id FROM saved_games WHERE user_id=? AND save_name=?",
                 {user_id, m_name});

    if (!existing.empty())
    {
        // Update existing save to point to the clone
        int sgid = std::stoi(existing[0][0]);
        db.Exec("UPDATE saved_games SET game_id=?, saved_at=NOW() WHERE id=?",
                {cloneId, sgid});
        Telemetry::instance().write(
            std::format("SAVE: Updated '{}' (Turn {}).", m_name, gs.round));
    }
    else
    {
        // Create new save pointing to the clone
        db.Exec("INSERT INTO saved_games(user_id, save_name, game_id) "
                "VALUES(?,?,?)",
                {user_id, m_name, cloneId});
        Telemetry::instance().write(std::format(
            "SAVE: Game saved as '{}' (Turn {}).", m_name, gs.round));
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
    auto saves =
        db.Query("SELECT sg.game_id, g.module_id, "
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

    int target_game_id = std::stoi(saves[0][0]);
    int module_id = std::atoi(saves[0][1].c_str());
    std::string round = saves[0][2];
    (void)module_id;

    // Can't load the same game we're in
    if (KH_EQU(target_game_id, game_id))
    {
        Telemetry::instance().write("LOAD: You're already in that game.");
        return false;
    }

    // Single-player: load immediately, no confirmation needed
    if (sm.is_singlep())
    {
        Logger::instance().info(
            std::format("[LOAD] Single-player load: {} -> game {}", m_name,
                        target_game_id));

        // Switch all sessions for this game to the saved game_id
        auto seats = db.Query("SELECT user_id FROM game_seats WHERE game_id=?",
                              {game_id});
        for (const auto& seat : seats)
        {
            int uid = std::stoi(seat[0]);
            db.Exec("UPDATE sessions SET game_id=? WHERE user_id=?",
                    {target_game_id, uid});
        }

        std::string msg = std::format(
            "COMMAND: Game '{}' loaded. Resuming Turn {}.", m_name, round);
        Telemetry::instance().write(msg);
        return true;
    }

    // Two-player: check for existing pending request
    auto pending = db.Query(
        "SELECT requester, save_name FROM load_requests WHERE game_id=?",
        {game_id});

    if (!pending.empty())
    {
        std::string existing_requester = pending[0][0];
        std::string existing_name = pending[0][1];

        // Is this the same request? If so, this is the confirmation!
        if (KH_EQU(existing_name, m_name)
             && KH_NEQ(existing_requester[0], player))
        {
            // This is the second player confirming - execute the load!
            Logger::instance().info("[LOAD] Second player confirmed load: " +
                                    m_name);

            // Get both user IDs from game_seats
            auto seats = db.Query(
                "SELECT user_id FROM game_seats WHERE game_id=?", {game_id});

            // Update both sessions to new game_id
            for (const auto& seat : seats)
            {
                int uid = std::stoi(seat[0]);
                db.Exec("UPDATE sessions SET game_id=? WHERE user_id=?",
                        {target_game_id, uid});
            }

            // Clear the pending request
            db.Exec("DELETE FROM load_requests WHERE game_id=?", {game_id});

            // Notify both players
            std::string msg = std::format(
                "COMMAND: Game '{}' loaded. Resuming Turn {}.", m_name, round);
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
    auto username_row =
        db.Query("SELECT username FROM users WHERE id=?", {user_id});
    std::string username = username_row.empty() ? "Player" : username_row[0][0];

    // Notify the requester
    Telemetry::instance().write("LOAD: Requested to load '" + m_name +
                                "' (Turn " + round +
                                "). "
                                "Waiting for other player to accept.");

    // Notify the opponent
    char opponent = player ^ 0x03;
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
    if (KH_EQU(requester[0], player))
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
    auto seats =
        db.Query("SELECT user_id FROM game_seats WHERE game_id=?", {game_id});

    // Update both sessions to new game_id
    for (const auto& seat : seats)
    {
        int uid = std::stoi(seat[0]);
        db.Exec("UPDATE sessions SET game_id=? WHERE user_id=?",
                {target_game_id, uid});
    }

    // Clear the pending request
    db.Exec("DELETE FROM load_requests WHERE game_id=?", {game_id});

    auto conflict_rows =
        db.Query("SELECT s.at_hex "
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
    auto username_row =
        db.Query("SELECT username FROM users WHERE id=?", {user_id});
    std::string username = username_row.empty() ? "Player" : username_row[0][0];

    // Delete the request
    db.Exec("DELETE FROM load_requests WHERE game_id=?", {game_id});

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
