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
#include "statemachine.h"

void handle_events(const HttpRequest* req, HttpResponse* resp)
{
    DatabaseManager& db = DatabaseManager::instance();
    int game_id = StateMachine::instance().get_game_id();

    if (!authenticated(req, resp))
    {
        return;
    }

    int limit = 50;
    size_t qpos = req->path.find("?");

    std::string q = 
     " SELECT seq,command_text,result_text,created_at "
     " FROM game_events "
     " WHERE game_id=? ORDER BY seq DESC LIMIT 100";

    auto rows = db.Query(q, { game_id });

    std::ostringstream o;
    o << "{\"ok\":true,\"events\":[";
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (i)
            o << ",";
        o << "{";
        o << "\"seq\":" << rows[i][0] << ",";
        o << "\"cmd\":\"" << json_escape(rows[i][1]) << "\",";
        o << "\"result\":\"" << json_escape(rows[i][2]) << "\",";
        o << "\"ts\":\"" << json_escape(rows[i][3]) << "\"";
        o << "}";
    }
    o << "]}";
    resp->body = o.str();
    return;
}

void append_event(int game_id, int user_id, const std::string& cmd,
                  const std::string& result, const GameState& s)
{
    DatabaseManager& db = DatabaseManager::instance();
    int seq = StateMachine::instance().next_event_seq(game_id);
    std::string q =
     "INSERT INTO "
     "game_events (game_id, user_id, seq, command_text, result_text, state_json) "
     " VALUES( ?, ?, ?, ?, ?, ? )";
    std::string jsn = s.to_json();
    db.Exec(q, {game_id, user_id, seq, cmd, result, jsn });
}
