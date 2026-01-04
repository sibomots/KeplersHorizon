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
#include "roommanager.h"

// Helper to get user_id from token
static int get_user_id_from_token(const std::string& token)
{
    if (token.empty())
        return 0;
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query("SELECT user_id FROM sessions WHERE token='" +
                         db.esc(token) + "'");
    if (rows.empty())
        return 0;
    return std::stoi(rows[0][0]);
}

// Helper to build room JSON
static std::string room_to_json(const RoomInfo& r)
{
    std::ostringstream o;
    o << "{";
    o << "\"id\":" << r.id << ",";
    o << "\"room_code\":\"" << json_escape(r.room_code) << "\",";
    o << "\"name\":\"" << json_escape(r.name) << "\",";
    o << "\"created_by\":" << r.created_by << ",";
    o << "\"creator_name\":\"" << json_escape(r.creator_name) << "\",";
    o << "\"seat_a\":" << (r.seat_a > 0 ? std::to_string(r.seat_a) : "null")
      << ",";
    o << "\"seat_a_name\":\"" << json_escape(r.seat_a_name) << "\",";
    o << "\"seat_b\":" << (r.seat_b > 0 ? std::to_string(r.seat_b) : "null")
      << ",";
    o << "\"seat_b_name\":\"" << json_escape(r.seat_b_name) << "\",";
    o << "\"game_id\":" << (r.game_id > 0 ? std::to_string(r.game_id) : "null")
      << ",";
    o << "\"status\":\"" << json_escape(r.status) << "\",";
    o << "\"scenario\":\"" << json_escape(r.scenario) << "\",";
    o << "\"is_full\":" << (r.isFull() ? "true" : "false");
    o << "}";
    return o.str();
}

// GET /api/rooms - List open rooms
void handle_rooms_list(const HttpRequest* req, HttpResponse* resp)
{
    // Update heartbeat for online count tracking
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id > 0)
    {
        DatabaseManager& db = DatabaseManager::getInstance();
        db.exec("UPDATE sessions SET last_seen=NOW() WHERE user_id=" +
                std::to_string(user_id));
    }

    RoomManager& rm = RoomManager::getInstance();
    auto rooms = rm.listOpenRooms();

    std::ostringstream o;
    o << "{\"ok\":true,\"rooms\":[";
    for (size_t i = 0; i < rooms.size(); ++i)
    {
        if (i > 0)
            o << ",";
        o << room_to_json(rooms[i]);
    }
    o << "],\"online_count\":" << rm.countOnlineUsers() << "}";

    resp->body = o.str();
}

// POST /api/rooms - Create room
void handle_rooms_create(const HttpRequest* req, HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id == 0)
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    std::string name = json_get_string(req->body, "name");
    if (name.empty())
        name = "Game Room";

    RoomManager& rm = RoomManager::getInstance();
    std::string code = rm.createRoom(user_id, name);

    if (code.empty())
    {
        resp->status = 500;
        resp->body = json_error("failed to create room");
        return;
    }

    RoomInfo room = rm.getRoom(code);
    resp->body = "{\"ok\":true,\"room\":" + room_to_json(room) + "}";
}

// GET /api/rooms/:code - Get room details
void handle_room_get(const std::string& code, const HttpRequest* req,
                     HttpResponse* resp)
{
    RoomManager& rm = RoomManager::getInstance();
    RoomInfo room = rm.getRoom(code);

    if (room.id == 0)
    {
        resp->status = 404;
        resp->body = json_error("room not found");
        return;
    }

    resp->body = "{\"ok\":true,\"room\":" + room_to_json(room) + "}";
}

// POST /api/rooms/:code/join - Join room
void handle_room_join(const std::string& code, const HttpRequest* req,
                      HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id == 0)
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    RoomManager& rm = RoomManager::getInstance();
    if (!rm.joinRoom(code, user_id))
    {
        resp->status = 400;
        resp->body = json_error("cannot join room");
        return;
    }

    RoomInfo room = rm.getRoom(code);
    resp->body = "{\"ok\":true,\"room\":" + room_to_json(room) + "}";
}

// POST /api/rooms/:code/leave - Leave room
void handle_room_leave(const std::string& code, const HttpRequest* req,
                       HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id == 0)
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    RoomManager& rm = RoomManager::getInstance();
    if (!rm.leaveRoom(code, user_id))
    {
        resp->status = 400;
        resp->body = json_error("cannot leave room");
        return;
    }

    resp->body = "{\"ok\":true}";
}

// POST /api/rooms/:code/scenario - Set scenario
void handle_room_scenario(const std::string& code, const HttpRequest* req,
                          HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id == 0)
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    std::string scenario = json_get_string(req->body, "scenario");

    RoomManager& rm = RoomManager::getInstance();
    if (!rm.setScenario(code, scenario))
    {
        resp->status = 400;
        resp->body = json_error("invalid scenario");
        return;
    }

    RoomInfo room = rm.getRoom(code);
    resp->body = "{\"ok\":true,\"room\":" + room_to_json(room) + "}";
}

// POST /api/rooms/:code/start - Start game
void handle_room_start(const std::string& code, const HttpRequest* req,
                       HttpResponse* resp)
{
    std::string token = pick_bearer(req);
    int user_id = get_user_id_from_token(token);
    if (user_id == 0)
    {
        resp->status = 401;
        resp->body = json_error("not authenticated");
        return;
    }

    RoomManager& rm = RoomManager::getInstance();
    RoomInfo room = rm.getRoom(code);

    // Verify user is in this room
    if (room.seat_a != user_id && room.seat_b != user_id)
    {
        resp->status = 403;
        resp->body = json_error("not in this room");
        return;
    }

    // Check if optional scenario in body
    std::string scenario = json_get_string(req->body, "scenario");
    if (!scenario.empty())
    {
        rm.setScenario(code, scenario);
    }

    int game_id = rm.startGame(code);
    if (game_id == 0)
    {
        resp->status = 400;
        resp->body = json_error("cannot start game - room not ready");
        return;
    }

    room = rm.getRoom(code);
    resp->body = "{\"ok\":true,\"game_id\":" + std::to_string(game_id) +
                 ",\"room\":" + room_to_json(room) + "}";
}

// Router for room endpoints
void handle_rooms(const HttpRequest* req, HttpResponse* resp)
{
    if (req->path == "/api/rooms")
    {
        if (req->method == "GET")
        {
            handle_rooms_list(req, resp);
        }
        else if (req->method == "POST")
        {
            handle_rooms_create(req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
        return;
    }

    // Extract room code from path: /api/rooms/{code} or
    // /api/rooms/{code}/action
    std::string path = req->path;
    if (path.rfind("/api/rooms/", 0) != 0)
    {
        resp->status = 404;
        resp->body = json_error("not found");
        return;
    }

    std::string remainder = path.substr(11); // after "/api/rooms/"
    size_t slash = remainder.find('/');
    std::string code =
        (slash == std::string::npos) ? remainder : remainder.substr(0, slash);
    std::string action =
        (slash == std::string::npos) ? "" : remainder.substr(slash + 1);

    if (action.empty())
    {
        if (req->method == "GET")
        {
            handle_room_get(code, req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
    }
    else if (action == "join")
    {
        if (req->method == "POST")
        {
            handle_room_join(code, req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
    }
    else if (action == "leave")
    {
        if (req->method == "POST")
        {
            handle_room_leave(code, req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
    }
    else if (action == "scenario")
    {
        if (req->method == "POST")
        {
            handle_room_scenario(code, req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
    }
    else if (action == "start")
    {
        if (req->method == "POST")
        {
            handle_room_start(code, req, resp);
        }
        else
        {
            resp->status = 405;
            resp->body = json_error("method not allowed");
        }
    }
    else
    {
        resp->status = 404;
        resp->body = json_error("unknown action");
    }
}
