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
#include "statemachine.h"
// BUGBUG #include "game.h"
#include "json.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

void handle_state(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "GET")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }

    AuthContext a = require_auth((const HttpRequest*)req, resp);
    if (resp->status != 200)
    {
        return;
    }

    // Telemetry::status() accesses StateMachine singleton and builds complete
    // JSON, including queued messages for this player
    Telemetry::getInstance().status(a.player, resp);
    return;
}

std::string json_ok_with_state_and_event(const GameState& s,
                                         const std::string& eventText)
{
    std::ostringstream o;
    o << "{";
    o << "\"ok\":true,";
    o << "\"event\":\"" << json_escape(eventText) << "\",";
    o << "\"state\":" << s.to_json();
    o << "}";
    return o.str();
}
