///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// 
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "app.h"
#include "comms.h"
#include "db.h"
#include "game.h"
#include "json.h"

void handle_events(const HttpRequest *req, Db *db, HttpResponse *resp)
{
    if (req->method != "GET")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }
    AuthContext a = require_auth(db, (const HttpRequest *)req, resp);
    if (resp->status != 200)
    {
        return;
    }

    // ?limit=50
    int limit = 50;
    size_t qpos = req->path.find("?");
    (void)qpos;

    auto rows =
        db->query("SELECT seq,command_text,result_text,created_at FROM "
                  "game_events WHERE game_id=" +
                  std::to_string(a.game_id) + " ORDER BY seq DESC LIMIT 100");
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

void append_event(Db *db, int game_id, int user_id, const std::string &cmd,
                  const std::string &result, const GameState &s)
{
    int seq = next_event_seq(db, game_id);
    std::string q = "INSERT INTO "
                    "game_events(game_id,user_id,seq,command_text,result_text,"
                    "state_json) VALUES(" +
                    std::to_string(game_id) + "," + std::to_string(user_id) +
                    "," + std::to_string(seq) + ",'" + db->esc(cmd) + "','" +
                    db->esc(result) + "','" + db->esc(s.to_json()) + "')";
    db->exec(q);
}
