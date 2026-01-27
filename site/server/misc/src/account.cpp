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
#include "logger.h"

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

    auto rows = DatabaseManager::instance().query(
        "SELECT id,password_plain "
        "FROM users "
        "WHERE username='" +
        DatabaseManager::instance().esc(u) + "' LIMIT 1");
    if (rows.empty() || rows[0][1] != p)
    {
        resp->status = 401;
        resp->body = json_error("bad credentials");
        return;
    }
    int user_id = std::atoi(rows[0][0].c_str());

    std::string token = rand_hex_64();

    DatabaseManager::instance().exec(
        "INSERT INTO sessions(token,user_id) VALUES('" +
        DatabaseManager::instance().esc(token) + "'," +
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
        DatabaseManager::instance().exec(
            "DELETE FROM sessions WHERE token='" +
            DatabaseManager::instance().esc(tok) + "'");
    }
    resp->body = "{\"ok\":true}";
    return;
}

void handle_register(const HttpRequest* req, HttpResponse* resp)
{
    // Registration disabled - admin adds users manually
    resp->status = 403;
    resp->body = json_error("Registration is disabled. Contact administrator.");
    return;

    // --- Original registration code preserved below (disabled) ---
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }

    std::string username = json_get_string(req->body, "username");
    std::string password = json_get_string(req->body, "password");
    std::string email = json_get_string(req->body, "email");

    // Validate required fields
    if (username.empty() || password.empty())
    {
        resp->status = 400;
        resp->body = json_error("username and password required");
        return;
    }

    // Validate username length and format
    if (username.length() < 3 || username.length() > 32)
    {
        Logger::instance().info("username: " + username);
        resp->status = 400;
        resp->body = json_error("username must be 3-32 characters");
        return;
    }

    // Validate password length
    if (password.length() < 4)
    {
        Logger::instance().info("password: " + password);
        resp->status = 400;
        resp->body = json_error("password must be at least 4 characters");
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();

    // Check if username already exists
    auto existing = db.query("SELECT id FROM users WHERE username='" +
                             db.esc(username) + "'");
    if (!existing.empty())
    {
        resp->status = 409;
        resp->body = json_error("username already taken");
        return;
    }

    // Check if email already exists (if provided)
    if (!email.empty())
    {
        auto email_check = db.query("SELECT id FROM users WHERE email='" +
                                    db.esc(email) + "'");
        if (!email_check.empty())
        {
            resp->status = 409;
            resp->body = json_error("email already registered");
            return;
        }
    }

    // Insert new user (using password_plain for now, bcrypt to be added)
    std::string sql = "INSERT INTO users(username, password_plain";
    if (!email.empty())
        sql += ", email";
    sql += ") VALUES('" + db.esc(username) + "','" + db.esc(password) + "'";
    if (!email.empty())
        sql += ",'" + db.esc(email) + "'";
    sql += ")";

    db.exec(sql);

    // Get the new user's ID
    auto id_rows = db.query("SELECT LAST_INSERT_ID()");
    if (id_rows.empty())
    {
        resp->status = 500;
        resp->body = json_error("failed to create account");
        return;
    }
    int user_id = std::stoi(id_rows[0][0]);

    // Auto-login: create session
    std::string token = rand_hex_64();
    db.exec("INSERT INTO sessions(token, user_id) VALUES('" + db.esc(token) +
            "'," + std::to_string(user_id) + ")");

    resp->body = "{\"ok\":true,\"token\":\"" + token + "\",\"username\":\"" +
                 json_escape(username) +
                 "\",\"user_id\":" + std::to_string(user_id) + "}";
}
