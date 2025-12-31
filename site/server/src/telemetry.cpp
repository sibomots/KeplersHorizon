//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "telemetry.h"

#include <sstream>

//#include "game.h"
#include "json.h"
#include "statemachine.h"

std::string Telemetry::write(const std::string& msg)
{
    GameState s = StateMachine::getInstance().get_game_state();

    std::ostringstream o;
    o << "{";
    o << "\"ok\":true,";
    o << "\"event\":\"" << json_escape(msg) << "\",";
    o << "\"state\":" << s.to_json();
    o << "}";

    return o.str();
}

std::string Telemetry::tell(PlayerTarget target, const std::string& msg)
{
    // Same as write - routing handled by caller
    return write(msg);
}

std::string Telemetry::broadcast(const std::string& msg)
{
    // Same as write - broadcast to all
    return write(msg);
}

void Telemetry::status(HttpResponse* resp)
{
    GameState s = StateMachine::getInstance().get_game_state();

    // Build status JSON
    std::ostringstream status_json;
    status_json << "{";
    status_json << "\"gameId\":" << s.game_id << ",";
    status_json << "\"scenario\":\"" << json_escape(s.scenario) << "\",";
    status_json << "\"round\":" << s.round << ",";
    status_json << "\"activePlayer\":\"" << json_escape(s.active_player)
                << "\",";
    status_json << "\"phaseIndex\":" << s.phase_index << ",";
    status_json << "\"phase\":\"" << json_escape(s.phase_name()) << "\",";
    status_json << "\"vp\":{\"A\":" << s.vpA << ",\"B\":" << s.vpB << "},";
    status_json << "\"bp\":{\"A\":" << s.bpA << ",\"B\":" << s.bpB << "},";

    if (!s.combat_summary_json.empty())
    {
        status_json << "\"combat\":" << s.combat_summary_json << ",";
    }

    status_json << "\"notes\":\"" << json_escape(s.notes()) << "\"";
    status_json << "}";

    // Determine self/opponent info from current player
    char selfOwner = s.active_player.empty() ? 'A' : s.active_player[0];
    char oppOwner = (selfOwner == 'A') ? 'B' : 'A';
    std::string selfUser = (selfOwner == 'A') ? "alice" : "bob";
    std::string oppUser = (oppOwner == 'A') ? "alice" : "bob";

    // Query opponent online status
    bool oppOnline = false;
    std::string oppLastSeen = "";

    DatabaseManager& db = DatabaseManager::getInstance();
    auto prow =
        db.query("SELECT DATE_FORMAT(last_seen,'%Y-%m-%d %H:%i:%s') FROM "
                 "sessions s JOIN users u ON u.id=s.user_id "
                 "WHERE u.username='" +
                 db.esc(oppUser) + "' ORDER BY s.last_seen DESC LIMIT 1");

    if (!prow.empty())
    {
        oppLastSeen = prow[0][0];
        auto prow2 =
            db.query("SELECT (TIMESTAMPDIFF(SECOND, last_seen, NOW()) <= 90) "
                     "FROM sessions s JOIN users u ON u.id=s.user_id "
                     "WHERE u.username='" +
                     db.esc(oppUser) + "' ORDER BY s.last_seen DESC LIMIT 1");

        if (!prow2.empty() && !prow2[0][0].empty() && prow2[0][0] != "0")
        {
            oppOnline = true;
        }
    }

    // Build complete response and set directly
    std::ostringstream out;
    out << "{\"ok\":true,\"state\":" << status_json.str()
        << ",\"self\":{\"owner\":\"" << selfOwner << "\",\"username\":\""
        << json_escape(selfUser) << "\"}"
        << ",\"peer\":{\"owner\":\"" << oppOwner << "\",\"username\":\""
        << oppUser << "\",\"online\":" << (oppOnline ? "true" : "false")
        << ",\"last_seen\":\"" << json_escape(oppLastSeen) << "\"}"
        << "}";

    resp->body = out.str();
}
