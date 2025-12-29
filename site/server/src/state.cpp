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
// 1. Redistributions of source code must retain the above copyright notice,
// this
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "app.h"
#include "comms.h"
#include "db.h"
#include "game.h"
#include "json.h"
#include "telemetry.h"
#include "typs.h"
#include "util.h"

void handle_state(const HttpRequest *req, Db *db, HttpResponse *resp)
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

    // Telemetry::status() accesses StateMachine singleton and builds complete JSON
    Telemetry::status(resp);
    return;
}

std::string json_ok_with_state_and_event(const GameState &s,
                                         const std::string &eventText)
{
    std::ostringstream o;
    o << "{";
    o << "\"ok\":true,";
    o << "\"event\":\"" << json_escape(eventText) << "\",";
    o << "\"state\":" << s.to_json();
    o << "}";
    return o.str();
}
