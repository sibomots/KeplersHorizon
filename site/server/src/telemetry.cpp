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
#include "db.h"

void Telemetry::clear_messages()
{
    write_buffer.clear();
}

std::vector<std::string> Telemetry::get_messages()
{
    return write_buffer;
}

void Telemetry::add_message(const std::string& msg)
{
    write_buffer.push_back(msg);
}

void Telemetry::add_tell(char player, const std::string& msg)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    int game_id = StateMachine::getInstance().get_game_id();
    if (game_id == 0) return; // No game, no messages
    
    std::string target = std::string(1, player);
    std::string ins = "INSERT INTO telemetry_queue(game_id, target_player, message) VALUES(" +
                      std::to_string(game_id) + ",'" + target + "','" +
                      db.esc(msg) + "')";
    db.exec(ins);
}

void Telemetry::add_broadcast(const std::string& msg)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    int game_id = StateMachine::getInstance().get_game_id();
    if (game_id == 0) return; // No game, no messages
    
    std::string ins = "INSERT INTO telemetry_queue(game_id, target_player, message) VALUES(" +
                      std::to_string(game_id) + ",'BOTH','" +
                      db.esc(msg) + "')";
    db.exec(ins);
}

std::vector<std::string> Telemetry::get_queued_messages(char player)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    int game_id = StateMachine::getInstance().get_game_id();
    if (game_id == 0) return {};
    
    std::string target = std::string(1, player);
    auto rows = db.query("SELECT message FROM telemetry_queue WHERE game_id=" +
                        std::to_string(game_id) + " AND (target_player='" +
                        target + "' OR target_player='BOTH') ORDER BY id");
    
    std::vector<std::string> messages;
    for (const auto& row : rows)
    {
        messages.push_back(row[0]);
    }
    
    // Flush messages for this player (including BOTH)
    db.exec("DELETE FROM telemetry_queue WHERE game_id=" +
            std::to_string(game_id) + " AND (target_player='" + target + "' OR target_player='BOTH')");
    
    return messages;
}

std::string Telemetry::write(const std::string& msg)
{
    // Add message to buffer for later retrieval
    add_message(msg);
    
    int game_id = StateMachine::getInstance().get_game_id();
    
    std::ostringstream o;
    o << "{";
    o << "\"ok\":true,";
    o << "\"event\":\"" << json_escape(msg) << "\"";
    
    // Only include state if a game has been started
    if (game_id != 0)
    {
        GameState s = StateMachine::getInstance().get_game_state();
        o << ",\"state\":" << s.to_json();
    }
    
    o << "}";
    return o.str();
}

std::string Telemetry::tell(PlayerTarget target, const std::string& msg)
{
    // Queue message for specific player (delivered via heartbeat)
    GameState s = StateMachine::getInstance().get_game_state();
    char target_player = (target == PlayerTarget::ME) ? s.active_player[0] : 
                         (s.active_player[0] == 'A' ? 'B' : 'A');
    add_tell(target_player, msg);
    return write(msg); // Also return immediate response
}

std::string Telemetry::broadcast(const std::string& msg)
{
    // Queue message for all players (delivered via heartbeat)
    add_broadcast(msg);
    return write(msg); // Also return immediate response
}

void Telemetry::status(char player, HttpResponse* resp)
{
    int game_id = StateMachine::getInstance().get_game_id();
    
    // If no game has been started yet, return minimal status
    if (game_id == 0)
    {
        std::ostringstream out;
        out << "{\"ok\":true,\"state\":{"
            << "\"gameId\":0,"
            << "\"scenario\":\"\","
            << "\"round\":0,"
            << "\"activePlayer\":\"\","
            << "\"phaseIndex\":0,"
            << "\"phase\":\"\","
            << "\"vp\":{\"A\":0,\"B\":0},"
            << "\"bp\":{\"A\":0,\"B\":0},"
            << "\"notes\":\"Type: start learning|basic|advanced\""
            << "}";
        out << ",\"self\":{},\"peer\":{}}";
        resp->body = out.str();
        return;
    }
    
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

    // Get queued messages for this player (from tell/broadcast)
    auto queued_messages = get_queued_messages(player);
    
    // Build complete response and set directly
    std::ostringstream out;
    out << "{\"ok\":true,\"state\":" << status_json.str()
        << ",\"self\":{\"owner\":\"" << selfOwner << "\",\"username\":\""
        << json_escape(selfUser) << "\"}"
        << ",\"peer\":{\"owner\":\"" << oppOwner << "\",\"username\":\""
        << oppUser << "\",\"online\":" << (oppOnline ? "true" : "false")
        << ",\"last_seen\":\"" << json_escape(oppLastSeen) << "\"}";
    
    // Include queued messages if any
    if (!queued_messages.empty())
    {
        out << ",\"messages\":[";
        for (size_t i = 0; i < queued_messages.size(); ++i)
        {
            if (i > 0) out << ",";
            out << "\"" << json_escape(queued_messages[i]) << "\"";
        }
        out << "]";
    }
    
    out << "}";

    resp->body = out.str();
}
