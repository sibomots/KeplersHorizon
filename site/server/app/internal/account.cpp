//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "app.h"
#include "comms.h"
#include "db.h"
#include "json.h"
#include "typedefs.h"
#include "util.h"

void handle_login(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }
    std::string u = json_get_string(req->body, "username");
    std::string p = json_get_string(req->body, "password");
    if (u.empty() || p.empty())
    {
        resp->status = 400;
        resp->body = json_error("missing username/password");
        return;
    }

    auto rows = DatabaseManager::getInstance().query(
        "SELECT id,password_plain "
        "FROM users "
        "WHERE username='" +
        DatabaseManager::getInstance().esc(u) + "' LIMIT 1");
    if (rows.empty() || rows[0][1] != p)
    {
        resp->status = 401;
        resp->body = json_error("bad credentials");
        return;
    }
    int user_id = std::atoi(rows[0][0].c_str());

    std::string token = rand_hex_64();

    DatabaseManager::getInstance().exec(
        "INSERT INTO sessions(token,user_id) VALUES('" +
        DatabaseManager::getInstance().esc(token) + "'," +
        std::to_string(user_id) + ")");

    resp->body = std::string("{\"ok\":true,\"token\":\"") + token +
                 "\",\"username\":\"" + json_escape(u) + "\"" +
#ifdef GIT_SHA
                 ",\"git_sha\":\"" GIT_SHA "\""
#endif
                 "}";
    return;
}

void handle_logout(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }
    std::string tok = pick_bearer(req);

    if (!tok.empty())
    {
        DatabaseManager::getInstance().exec(
            "DELETE FROM sessions WHERE token='" +
            DatabaseManager::getInstance().esc(tok) + "'");
    }
    resp->body = "{\"ok\":true}";
    return;
}
