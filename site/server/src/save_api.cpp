//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <sstream>

#include "comms.h"
#include "db.h"
#include "json.h"
#include "statemachine.h"

// Helper to get user_id AND game_id from token (session is authoritative)
static bool get_session_from_token(const std::string& token, int& user_id, int& game_id)
{
    user_id = 0;
    game_id = 0;
    if (token.empty())
        return false;
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query("SELECT user_id, COALESCE(game_id, 0) FROM sessions WHERE token='" +
                         db.esc(token) + "'");
    if (rows.empty())
        return false;
    user_id = std::stoi(rows[0][0]);
    game_id = std::stoi(rows[0][1]);
    return true;
}

// Helper to get username
static std::string get_username(int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query("SELECT username FROM users WHERE id=" +
                         std::to_string(user_id));
    if (rows.empty())
        return "";
    return rows[0][0];
}

// POST /api/save - Save current game state (game_id comes from session, not URL)
void handle_game_save(const HttpRequest* req, HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = 0;
    int game_id = 0;
    
    if (!get_session_from_token(token, user_id, game_id))
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    if (game_id == 0)
    {
        resp->status = 400;
        resp->body = json_error("no active game in session");
        return;
    }

    // Get save name from request body
    std::string save_name = json_get_string(req->body, "name");
    if (save_name.empty())
    {
        save_name = "Save " + std::to_string(time(nullptr));
    }

    DatabaseManager& db = DatabaseManager::getInstance();

    // Get game state
    auto game_rows = db.query(
        "SELECT state_json, scenario FROM games WHERE id=" +
        std::to_string(game_id));
    if (game_rows.empty())
    {
        resp->status = 404;
        resp->body = json_error("game not found");
        return;
    }

    std::string state_json = game_rows[0][0];
    std::string scenario = game_rows[0][1];

    // Get room code if exists
    auto room_rows = db.query(
        "SELECT room_code FROM rooms WHERE game_id=" + std::to_string(game_id));
    std::string room_code = room_rows.empty() ? "" : room_rows[0][0];

    // Get player names from game_seats
    auto seat_rows = db.query(
        "SELECT gs.seat, u.username FROM game_seats gs "
        "JOIN users u ON gs.user_id = u.id "
        "WHERE gs.game_id=" + std::to_string(game_id));
    
    std::string player_a_name, player_b_name;
    for (const auto& row : seat_rows)
    {
        if (row[0] == "A") player_a_name = row[1];
        else if (row[0] == "B") player_b_name = row[1];
    }

    // Parse round from state
    int round = 1;
    size_t round_pos = state_json.find("\"round\":");
    if (round_pos != std::string::npos)
    {
        round = std::atoi(state_json.c_str() + round_pos + 8);
    }

    // Insert save
    std::string insert_sql =
        "INSERT INTO saved_games(save_name, user_id, original_game_id, "
        "room_code, scenario, state_json, round, player_a_name, player_b_name) "
        "VALUES('" +
        db.esc(save_name) + "'," + std::to_string(user_id) + "," +
        std::to_string(game_id) + ",'" + db.esc(room_code) + "','" +
        db.esc(scenario) + "','" + db.esc(state_json) + "'," +
        std::to_string(round) + ",'" + db.esc(player_a_name) + "','" +
        db.esc(player_b_name) + "')";
    
    db.exec(insert_sql);

    // Get save ID
    auto id_rows = db.query("SELECT LAST_INSERT_ID()");
    int save_id = id_rows.empty() ? 0 : std::stoi(id_rows[0][0]);

    // Save ships
    auto ships = db.query(
        "SELECT ship_code, ship_name, owner, "
        "CONCAT('{\"pd\":', pd, ',\"beam\":', beam, ',\"screen\":', screen, ',\"tube\":', tube, "
        "',\"missiles\":', missiles, ',\"hull\":', hull, ',\"at_hex\":\"', "
        "IFNULL(at_hex,''), '\",\"at_system\":\"', IFNULL(at_system,''), "
        "'\",\"deployed_at\":\"', IFNULL(deployed_at,''), '\"}') "
        "FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND destroyed_at IS NULL");
    
    for (const auto& ship : ships)
    {
        db.exec("INSERT INTO saved_ships(save_id, ship_code, ship_name, owner, ship_json) "
                "VALUES(" + std::to_string(save_id) + ",'" + db.esc(ship[0]) + "','" +
                db.esc(ship[1]) + "','" + ship[2] + "','" + db.esc(ship[3]) + "')");
    }

    std::ostringstream o;
    o << "{\"ok\":true,\"save_id\":" << save_id
      << ",\"save_name\":\"" << json_escape(save_name) << "\""
      << ",\"round\":" << round << "}";
    resp->body = o.str();
}

// GET /api/saved - List user's saved games
void handle_saves_list(const HttpRequest* req, HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = 0;
    int game_id = 0;
    
    if (!get_session_from_token(token, user_id, game_id))
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT id, save_name, scenario, round, player_a_name, player_b_name, "
        "saved_at FROM saved_games WHERE user_id=" + std::to_string(user_id) +
        " ORDER BY saved_at DESC LIMIT 20");

    std::ostringstream o;
    o << "{\"ok\":true,\"saves\":[";
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (i > 0) o << ",";
        o << "{\"id\":" << rows[i][0]
          << ",\"name\":\"" << json_escape(rows[i][1]) << "\""
          << ",\"scenario\":\"" << json_escape(rows[i][2]) << "\""
          << ",\"round\":" << rows[i][3]
          << ",\"player_a\":\"" << json_escape(rows[i][4]) << "\""
          << ",\"player_b\":\"" << json_escape(rows[i][5]) << "\""
          << ",\"saved_at\":\"" << json_escape(rows[i][6]) << "\"}";
    }
    o << "]}";
    resp->body = o.str();
}

// POST /api/saved/:id/load - Load a saved game into a new room
void handle_save_load(int save_id, const HttpRequest* req, HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = 0;
    int game_id = 0;
    
    if (!get_session_from_token(token, user_id, game_id))
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    DatabaseManager& db = DatabaseManager::getInstance();

    // Get save data
    auto save_rows = db.query(
        "SELECT save_name, scenario, state_json, player_a_name, player_b_name "
        "FROM saved_games WHERE id=" + std::to_string(save_id) +
        " AND user_id=" + std::to_string(user_id));
    
    if (save_rows.empty())
    {
        resp->status = 404;
        resp->body = json_error("save not found");
        return;
    }

    std::string save_name = save_rows[0][0];
    std::string scenario = save_rows[0][1];
    std::string state_json = save_rows[0][2];
    std::string player_a_name = save_rows[0][3];
    std::string player_b_name = save_rows[0][4];

    // Create new game with saved state
    std::string ins = "INSERT INTO games(scenario, state_json) VALUES('" +
                      db.esc(scenario) + "','" + db.esc(state_json) + "')";
    db.exec(ins);

    auto id_rows = db.query("SELECT LAST_INSERT_ID()");
    int new_game_id = id_rows.empty() ? 0 : std::stoi(id_rows[0][0]);

    if (new_game_id == 0)
    {
        resp->status = 500;
        resp->body = json_error("failed to create game");
        return;
    }

    // Load saved ships
    auto ships = db.query(
        "SELECT ship_code, ship_name, owner, ship_json "
        "FROM saved_ships WHERE save_id=" + std::to_string(save_id));
    
    for (const auto& ship : ships)
    {
        // Parse ship JSON and insert into ships table
        // For now, insert with basic data
        std::string ship_code = ship[0];
        std::string ship_name = ship[1];
        char owner = ship[2][0];
        
        // TODO: Parse ship_json for full restoration
        db.exec("INSERT INTO ships(game_id, ship_code, ship_name, owner, pd, b, s, t) "
                "VALUES(" + std::to_string(new_game_id) + ",'" + db.esc(ship_code) +
                "','" + db.esc(ship_name) + "','" + std::string(1, owner) + "',1,1,1,1)");
    }

    // Create room for this game
    std::string room_code;
    static const char* chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    for (int i = 0; i < 6; ++i)
    {
        room_code += chars[rand() % 32];
    }

    std::string room_name = "Resumed: " + save_name;
    db.exec("INSERT INTO rooms(room_code, name, created_by, seat_a, status, scenario, game_id) "
            "VALUES('" + db.esc(room_code) + "','" + db.esc(room_name) + "'," +
            std::to_string(user_id) + "," + std::to_string(user_id) +
            ",'waiting','" + db.esc(scenario) + "'," + std::to_string(new_game_id) + ")");

    // Link user to game
    db.exec("INSERT INTO game_seats(game_id, user_id, seat) VALUES(" +
            std::to_string(new_game_id) + "," + std::to_string(user_id) + ",'A')");
    db.exec("UPDATE sessions SET game_id=" + std::to_string(new_game_id) +
            " WHERE user_id=" + std::to_string(user_id));

    std::ostringstream o;
    o << "{\"ok\":true,\"game_id\":" << new_game_id
      << ",\"room_code\":\"" << room_code << "\"}";
    resp->body = o.str();
}

// DELETE /api/saved/:id - Delete a saved game
void handle_save_delete(int save_id, const HttpRequest* req, HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = 0;
    int game_id = 0;
    
    if (!get_session_from_token(token, user_id, game_id))
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    DatabaseManager& db = DatabaseManager::getInstance();

    // Check ownership
    auto rows = db.query("SELECT id FROM saved_games WHERE id=" +
                         std::to_string(save_id) + " AND user_id=" +
                         std::to_string(user_id));
    if (rows.empty())
    {
        resp->status = 404;
        resp->body = json_error("save not found");
        return;
    }

    // Delete (cascade will remove saved_ships)
    db.exec("DELETE FROM saved_games WHERE id=" + std::to_string(save_id));
    resp->body = "{\"ok\":true}";
}

// Router for save/load endpoints
void handle_saves(const HttpRequest* req, HttpResponse* resp)
{
    std::string path = req->path;

    // GET /api/saved - list saves
    if (path == "/api/saved" && req->method == "GET")
    {
        handle_saves_list(req, resp);
        return;
    }

    // POST /api/save (session-based, no game_id in URL)
    if (path == "/api/save" && req->method == "POST")
    {
        handle_game_save(req, resp);
        return;
    }

    // LEGACY: POST /api/games/:id/save (redirect to session-based)
    if (path.rfind("/api/games/", 0) == 0 && path.find("/save") != std::string::npos)
    {
        // Ignore the ID in URL, use session instead
        if (req->method == "POST")
        {
            handle_game_save(req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
        return;
    }

    // /api/saved/:id/load or /api/saved/:id
    if (path.rfind("/api/saved/", 0) == 0)
    {
        std::string remainder = path.substr(11);
        size_t slash_pos = remainder.find('/');
        std::string id_str = (slash_pos == std::string::npos) 
            ? remainder : remainder.substr(0, slash_pos);
        std::string action = (slash_pos == std::string::npos) 
            ? "" : remainder.substr(slash_pos + 1);
        
        int save_id = std::atoi(id_str.c_str());

        if (action == "load" && req->method == "POST")
        {
            handle_save_load(save_id, req, resp);
        }
        else if (action.empty() && req->method == "DELETE")
        {
            handle_save_delete(save_id, req, resp);
        }
        else
        {
            resp->status = 404;
            resp->body = json_error("not found");
        }
        return;
    }

    resp->status = 404;
    resp->body = json_error("not found");
}
