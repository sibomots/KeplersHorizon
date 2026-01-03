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
#include "logger.h"
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
    int game_id = StateMachine::getInstance().get_game_id();
    add_tell(game_id, player, msg);
}

void Telemetry::add_tell(int game_id, char player, const std::string& msg)
{
    if (game_id == 0) {
        Logger::instance().info("[add_tell] Skipping - game_id=0");
        return;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string target = std::string(1, player);
    std::string ins = "INSERT INTO telemetry_queue(game_id, target_player, message) VALUES(" +
                      std::to_string(game_id) + ",'" + target + "','" +
                      db.esc(msg) + "')";
    Logger::instance().info("[add_tell] game_id=" + std::to_string(game_id) + 
                           " player=" + target + " msg=" + msg.substr(0, 40));
    db.exec(ins);
}

void Telemetry::add_broadcast(const std::string& msg)
{
    int game_id = StateMachine::getInstance().get_game_id();
    add_broadcast(game_id, msg);
}

void Telemetry::add_broadcast(int game_id, const std::string& msg)
{
    if (game_id == 0) return; // No game, no messages
    
    DatabaseManager& db = DatabaseManager::getInstance();
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
    // ME = the player who made this request (from set_current_player)
    // THEM = the other player
    char requesting_player = StateMachine::getInstance().get_current_player();
    char target_player = (target == PlayerTarget::ME) ? requesting_player : 
                         (requesting_player == 'A' ? 'B' : 'A');
    add_tell(target_player, msg);
    
    // Don't return via write() - tell() is for async delivery via heartbeat only
    // Returning empty so the caller's response isn't duplicated
    return "";
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
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // If no game has been started yet, return minimal status but still check peer online
    if (game_id == 0)
    {
        // Get the requesting user's username from the current request context
        // We need to find any OTHER user who has an active session
        
        // Query all active sessions (within 90 seconds) grouped by user
        auto sessionsQuery = db.query(
            "SELECT u.username, DATE_FORMAT(MAX(s.last_seen),'%Y-%m-%d %H:%i:%s') "
            "FROM sessions s JOIN users u ON u.id = s.user_id "
            "WHERE TIMESTAMPDIFF(SECOND, s.last_seen, NOW()) <= 6 "
            "GROUP BY u.id, u.username "
            "ORDER BY MAX(s.last_seen) DESC");
        
        // If there are 2+ distinct users with active sessions, peer is online
        bool peerOnline = false;
        std::string peerUser;
        std::string peerLastSeen;
        
        if (sessionsQuery.size() >= 2)
        {
            // First user in list (most recent) might be the requester, 
            // so peer is second one... but we don't know requester name here.
            // Just report that there IS a peer online (pick different user).
            peerOnline = true;
            // Report the second user as peer (first might be self)
            peerUser = sessionsQuery[1][0];
            peerLastSeen = sessionsQuery[1][1];
        }
        
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
        out << ",\"self\":{\"owner\":\"" << player << "\"}";
        out << ",\"peer\":{\"online\":" << (peerOnline ? "true" : "false");
        if (peerOnline && !peerUser.empty())
        {
            out << ",\"username\":\"" << json_escape(peerUser) << "\"";
            out << ",\"last_seen\":\"" << json_escape(peerLastSeen) << "\"";
        }
        out << "}}";
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

    // Determine self/opponent info from the requesting player (not active_player!)
    // 'player' is who made the request, 'active_player' is whose turn it is
    char selfOwner = player;
    char oppOwner = (selfOwner == 'A') ? 'B' : 'A';
    
    // Look up actual usernames from game_seats
    std::string selfUser, oppUser;
    auto selfRow = db.query("SELECT u.username FROM users u "
                            "JOIN game_seats gs ON gs.user_id = u.id "
                            "WHERE gs.game_id=" + std::to_string(game_id) + 
                            " AND gs.seat='" + std::string(1, selfOwner) + "'");
    auto oppRow = db.query("SELECT u.username FROM users u "
                           "JOIN game_seats gs ON gs.user_id = u.id "
                           "WHERE gs.game_id=" + std::to_string(game_id) + 
                           " AND gs.seat='" + std::string(1, oppOwner) + "'");
    
    selfUser = selfRow.empty() ? "" : selfRow[0][0];
    oppUser = oppRow.empty() ? "" : oppRow[0][0];

    // Query opponent online status
    bool oppOnline = false;
    std::string oppLastSeen;

    auto prow =
        db.query("SELECT DATE_FORMAT(last_seen,'%Y-%m-%d %H:%i:%s') FROM "
                 "sessions s JOIN users u ON u.id=s.user_id "
                 "WHERE u.username='" +
                 db.esc(oppUser) + "' ORDER BY s.last_seen DESC LIMIT 1");

    if (!prow.empty())
    {
        oppLastSeen = prow[0][0];
        auto prow2 =
            db.query("SELECT (TIMESTAMPDIFF(SECOND, last_seen, NOW()) <= 6) "
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
