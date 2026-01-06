//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "comms.h"

#include "app.h"
#include "cmd.h"
#include "db.h"
#include "events.h"
#include "logger.h"
#include "state.h"
#include "util.h"

void dispatch_request(const HttpRequest* req, HttpResponse* resp)
{
    if (req->path == "/api/login")
    {
        handle_login(req, resp);
        return;
    }
    else if (req->path == "/api/register")
    {
        handle_register(req, resp);
        return;
    }
    else if (req->path == "/api/logout")
    {
        handle_logout(req, resp);
        return;
    }
    else if (req->path == "/api/state")
    {
        handle_state(req, resp);
        return;
    }
    else if (req->path == "/api/command")
    {
        handle_usr_command(req, resp);
        return;
    }
    else if (req->path == "/api/events")
    {
        // Optional: events for console persistence
        handle_events(req, resp);
        return;
    }
    else if (req->path == "/api/rooms" || starts_with(req->path, "/api/rooms/"))
    {
        handle_rooms(req, resp);
        return;
    }
    else if (req->path == "/api/saved" || starts_with(req->path, "/api/saved/") ||
             req->path == "/api/save" ||
             (starts_with(req->path, "/api/games/") && req->path.find("/save") != std::string::npos))
    {
        handle_saves(req, resp);
        return;
    }
    else
    {
        resp->status = 404;
        resp->body = json_error("not found");
    }
    return;
}

std::string status_text(int code)
{
    switch (code)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 500:
        return "Internal Server Error";
    case 502:
        return "Bad Gateway";
    default:
        return "OK";
    }
}

std::string http_serialize(const HttpResponse& r)
{
    std::ostringstream o;
    o << "HTTP/1.1 " << r.status << " " << status_text(r.status) << "\r\n";
    o << "Content-Type: " << r.content_type << "\r\n";
    o << "Content-Length: " << r.body.size() << "\r\n";
    o << "Connection: close\r\n";
    o << "Cache-Control: no-store\r\n";
    o << "\r\n";
    o << r.body;
    return o.str();
}

HttpRequest http_parse(int fd)
{
    std::string data;
    data.reserve(8192);

    char buf[4096];
    ssize_t n;
    while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0)
    {
        data.append(buf, buf + n);
        // stop if we have headers and full body
        size_t header_end = data.find("\r\n\r\n");
        if (header_end != std::string::npos)
        {
            // crude Content-Length support
            size_t clpos = to_lower(data).find("content-length:");
            size_t content_len = 0;
            if (clpos != std::string::npos)
            {
                size_t line_end = data.find("\r\n", clpos);
                std::string line = data.substr(clpos, line_end - clpos);
                size_t colon = line.find(':');
                if (colon != std::string::npos)
                {
                    content_len = static_cast<size_t>(
                        std::atoi(trim(line.substr(colon + 1)).c_str()));
                }
            }
            size_t body_start = header_end + 4;
            if (data.size() >= body_start + content_len)
                break;
        }
        if (data.size() > 1024 * 1024)
            break; // safety
    }

    HttpRequest req;
    size_t line_end = data.find("\r\n");
    if (line_end == std::string::npos)
        return req;

    std::string request_line = data.substr(0, line_end);
    {
        std::istringstream is(request_line);
        is >> req.method >> req.path;
    }

    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos)
        header_end = data.size();

    size_t pos = line_end + 2;
    while (pos + 2 <= header_end)
    {
        size_t e = data.find("\r\n", pos);
        if (e == std::string::npos || e > header_end)
            break;
        std::string line = data.substr(pos, e - pos);
        pos = e + 2;
        if (line.empty())
            break;
        size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string k = to_lower(trim(line.substr(0, colon)));
            std::string v = trim(line.substr(colon + 1));
            req.headers[k] = v;
        }
    }

    if (header_end + 4 <= data.size())
        req.body = data.substr(header_end + 4);

    return req;
}

std::string pick_bearer(const HttpRequest* req)
{
    auto it = req->headers.find("authorization");
    if (it == req->headers.end())
    {
        return "";
    }

    std::string v = it->second;

    if (!starts_with(to_lower(v), "bearer "))
    {
        return "";
    }

    return trim(v.substr(7));
}

AuthContext require_auth(const HttpRequest* req, HttpResponse* resp)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    AuthContext a;
    std::string tok = pick_bearer(req);
    if (tok.empty())
    {
        resp->status = 401;
        resp->body = "{\"ok\":false,\"error\":\"missing bearer token\"}";
        return a;
    }

    std::string q = "SELECT s.user_id,u.username,COALESCE(s.game_id,0) FROM "
                    "sessions s JOIN users u ON "
                    "u.id=s.user_id WHERE s.token='" +
                    db.esc(tok) + "'";
    auto rows = db.query(q);
    if (rows.empty())
    {
        resp->status = 401;
        resp->body = "{\"ok\":false,\"error\":\"invalid token\"}";
        return a;
    }
    a.user_id = std::atoi(rows[0][0].c_str());
    a.username = rows[0][1];
    a.game_id = std::atoi(
        rows[0][2].c_str()); // Load game_id from session (set by room flow)
    a.token = tok;

    // Look up player's seat from game_seats table
    // Room flow already set up game_id and game_seats properly
    a.player = seat_for_user(a.game_id, a.user_id);

    // If no seat found (user not in a game yet), that's okay for lobby/room
    // operations Commands that require a game will check for valid game state
    if (a.game_id != 0 && a.player == 0)
    {
        Logger::instance().debug("[require_auth] User " + a.username +
                                 " has game_id=" + std::to_string(a.game_id) +
                                 " but no seat (may be stale session)");
    }

    // heartbeat
    db.exec("UPDATE sessions SET last_seen=NOW() WHERE token='" + db.esc(tok) +
            "'");

    return a;
}

bool authenticated(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "GET")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return false;
    }
    AuthContext a = require_auth((const HttpRequest*)req, resp);
    if (resp->status != 200)
    {
        return false;
    }
    return true;
}
