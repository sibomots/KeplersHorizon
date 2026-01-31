//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <sstream>
#include <mutex>
#include <iomanip>

#include "db.h"
#include "json.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

typedef enum : int
{
   TLM_ADD_MSG,
   TLM_ADD_TELL,
   TLM_ADD_BCAST
} TlmMsgTyp;

static int sequence = 0;

static void seq_log(TlmMsgTyp typ, const std::string& msg)
{
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    sequence++;

    // jdw
    std::ostringstream oss;
    oss << "[";
    switch(typ) {
      case TLM_ADD_MSG:  
        oss << "+MSG |";
        break;
      case TLM_ADD_TELL:
        oss << "TELL |";
        break;
      case TLM_ADD_BCAST:
        oss << "BCAST|";
        break;
      default:
        oss << "???? |";
        break;
    }
    oss << std::setw(6) << std::setfill('0') << sequence;
    oss << "] ";
    oss << msg;
    Logger::instance().info(oss.str());
}

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
    seq_log(TLM_ADD_MSG, msg);
    write_buffer.push_back(msg);
}

void Telemetry::add_tell(char player, const std::string& msg)
{
    int game_id = StateMachine::instance().get_game_id();
    add_tell(game_id, player, msg);
}

void Telemetry::add_tell(int game_id, char player, const std::string& msg)
{
    if (game_id == 0)
    {
        return;
    }

    seq_log(TLM_ADD_TELL, msg);
    DatabaseManager& db = DatabaseManager::instance();
    std::string q =
    "INSERT INTO telemetry_queue(game_id, target_player, message) VALUES(?, ?, ?)";
    db.Exec(q, { game_id, player, msg }); 
}

void Telemetry::add_broadcast(const std::string& msg)
{
    int game_id = StateMachine::instance().get_game_id();
    add_broadcast(game_id, msg);
}

void Telemetry::add_broadcast(int game_id, const std::string& msg)
{
    DatabaseManager& db = DatabaseManager::instance();
    seq_log(TLM_ADD_BCAST, msg);
    std::string q =
    "INSERT INTO telemetry_queue(game_id, target_player, message) VALUES(?, 'BOTH', ?)";
    db.Exec(q, { game_id, msg }); 
}

Telemetry::QueuedMessages Telemetry::get_queued_messages(char player)
{
    QueuedMessages result;
    DatabaseManager& db = DatabaseManager::instance();
    int game_id = StateMachine::instance().get_game_id();

    result.player = player;
    // Get direct messages (target_player = A or B, use sent_at)
    std::string q = 
    "SELECT id, message "
    " FROM telemetry_queue "
    " WHERE game_id=? AND target_player=? AND sent_at IS NULL ORDER BY id";

    auto direct_rows = db.Query(q, {game_id, player});


    for (const auto& row : direct_rows)
    {
        result.direct_ids.push_back(row[0]);
        result.messages.push_back(row[1]);
    }

    // Get broadcast messages (target_player = BOTH, use sent_to_A/sent_to_B)
    std::string sent_col = (player == 'A') ? "sent_to_A" : "sent_to_B";
    std::string both_q =
        "SELECT id, message FROM telemetry_queue WHERE game_id=? "
        "AND target_player='BOTH' AND " + sent_col + "=0 ORDER BY id";
    auto both_rows = db.Query(both_q, {game_id});

    for (const auto& row : both_rows)
    {
        result.both_ids.push_back(row[0]);
        result.messages.push_back(row[1]);
    }

    // DO NOT mark sent here - that happens in status() after response is built
    return result;
}

void Telemetry::mark_messages_sent(const QueuedMessages& msgs)
{
    if (msgs.direct_ids.empty() && msgs.both_ids.empty())
        return;

    DatabaseManager& db = DatabaseManager::instance();

    // Mark direct messages as sent (sent_at)
    if (!msgs.direct_ids.empty())
    {
        std::string id_list;
        for (auto it = msgs.direct_ids.cbegin(); it != msgs.direct_ids.cend();
             ++it)
        {
            if (it != msgs.direct_ids.cbegin())
                id_list += ",";
            id_list += *it;
        }
        db.Exec("UPDATE telemetry_queue SET sent_at=NOW() WHERE id IN (" + id_list + ")", {});
    }

    // Mark broadcast messages as sent TO THIS PLAYER ONLY (sent_to_A or
    // sent_to_B)
    if (!msgs.both_ids.empty())
    {
        std::string sent_col = (msgs.player == 'A') ? "sent_to_A" : "sent_to_B";
        std::string id_list;
        for (auto it = msgs.both_ids.cbegin(); it != msgs.both_ids.cend(); ++it)
        {
            if (it != msgs.both_ids.cbegin())
                id_list += ",";
            id_list += *it;
        }
        db.Exec("UPDATE telemetry_queue SET " + sent_col + "=1 WHERE id IN (" + id_list + ")", {});
    }
}

void Telemetry::write(const std::string& msg)
{
    // Add message to buffer for later retrieval
    add_message(msg);
    Logger::instance().info(msg); 
}


void Telemetry::source_messages(std::string& result_msg)
{
    // Get accumulated telemetry messages and combine them
    std::string event_msg;

    std::ostringstream combined;
    for (size_t i = 0; i < write_buffer.size(); ++i)
    {
        if (i > 0)
        {
             combined << "\n";
        }
        combined << write_buffer[i];
    }
    event_msg = combined.str().empty() ? "Command executed\n" : combined.str();

    std::ostringstream oss;
    oss << "{";
    oss << "\"ok\":true,";
    oss << "\"event\":\"" << json_escape(event_msg) << "\"";

    // Only include state if a game has been started
    int game_id = StateMachine::instance().get_game_id();
    if (game_id != 0)
    {
        GameState s = StateMachine::instance().get_game_state();
        oss << ",\"state\":" << s.to_json();
    }

    oss << "}";
    result_msg = oss.str();
}

void Telemetry::tell(PlayerTarget target, const std::string& msg)
{
    // Queue message for specific player (delivered via heartbeat)
    // ME = the player who made this request (from get_current_player)
    // THEM = the other player
    char requesting_player = StateMachine::instance().get_current_player();
    char target_player = (target == PlayerTarget::ME)
                             ? requesting_player
                             : (requesting_player == 'A' ? 'B' : 'A');


    std::ostringstream oss;
    oss << "[TLM: " << requesting_player << "->" << target_player << "] " << msg.c_str();
    Logger::instance().info(oss.str());

    add_tell(target_player, msg);
    Logger::instance().info(msg); 
}

void Telemetry::broadcast(const std::string& raw_msg)
{
    // Queue message for all players (delivered via heartbeat)
    add_broadcast(raw_msg);
    Logger::instance().info(raw_msg); 
}

// Helper: Build ships JSON for map rendering
static std::string getShipsJson(int game_id, char player)
{
    DatabaseManager& db = DatabaseManager::instance();
    char opponent = (player == 'A') ? 'B' : 'A';

    std::ostringstream out;
    out << "{";

    // Friendly ships (owned by current player) - include specs for tooltip
    std::string friendly_q =
        "SELECT ship_code, ship_type, at_hex, at_system, pd, beam, screen, "
        "tube, missiles, sr, ship_name FROM ships "
        "WHERE destroyed_at IS NULL AND game_id=? AND owner=?";
    auto friendly = db.Query(friendly_q, {game_id, player});

    out << "\"friendly\":[";
    for (size_t i = 0; i < friendly.size(); ++i)
    {
        if (i > 0)
            out << ",";
        out << "{\"code\":\"" << json_escape(friendly[i][0]) << "\""
            << ",\"type\":\"" << json_escape(friendly[i][1]) << "\""
            << ",\"hex\":\"" << json_escape(friendly[i][2]) << "\""
            << ",\"system\":\"" << json_escape(friendly[i][3]) << "\""
            << ",\"pd\":" << friendly[i][4] << ",\"b\":" << friendly[i][5]
            << ",\"s\":" << friendly[i][6] << ",\"t\":" << friendly[i][7]
            << ",\"m\":" << friendly[i][8] << ",\"sr\":" << friendly[i][9]
            << ",\"name\":\"" << json_escape(friendly[i][10]) << "\""
            << "}";
    }
    out << "]";

    // Enemy ships (owned by opponent)
    std::string enemy_q =
        "SELECT ship_code, ship_type, at_hex, at_system, ship_name FROM ships "
        "WHERE destroyed_at IS NULL AND game_id=? AND owner=?";
    auto enemy = db.Query(enemy_q, {game_id, opponent});

    out << ",\"enemy\":[";
    for (size_t i = 0; i < enemy.size(); ++i)
    {
        if (i > 0)
            out << ",";
        out << "{\"code\":\"" << json_escape(enemy[i][0]) << "\""
            << ",\"type\":\"" << json_escape(enemy[i][1]) << "\""
            << ",\"hex\":\"" << json_escape(enemy[i][2]) << "\""
            << ",\"system\":\"" << json_escape(enemy[i][3]) << "\""
            << ",\"name\":\"" << json_escape(enemy[i][4]) << "\""
            << "}";
    }
    out << "]";

    // Neutral ships (owner N or X for xeno/third-party)
    std::string neutral_q =
        "SELECT ship_code, ship_type, at_hex, at_system FROM ships "
        "WHERE destroyed_at IS NULL AND owner NOT IN ('A','B') AND game_id=?";
    auto neutral = db.Query(neutral_q, {game_id});

    out << ",\"neutral\":[";
    for (size_t i = 0; i < neutral.size(); ++i)
    {
        if (i > 0)
            out << ",";
        out << "{\"code\":\"" << json_escape(neutral[i][0]) << "\""
            << ",\"type\":\"" << json_escape(neutral[i][1]) << "\""
            << ",\"hex\":\"" << json_escape(neutral[i][2]) << "\""
            << ",\"system\":\"" << json_escape(neutral[i][3]) << "\"}";
    }
    out << "]";

    out << "}";
    return out.str();
}

void Telemetry::status(char player, HttpResponse* resp)
{
    int game_id = StateMachine::instance().get_game_id();

    DatabaseManager& db = DatabaseManager::instance();

    // If no game has been started yet, return minimal status but still check
    // peer online

    // BUGBUG this is so broken.
    // why are we  using game_id as a flag for this??
    if (game_id == 0)
    {
        // Get the requesting user's username from the current request context
        // We need to find any OTHER user who has an active session

        // Query all active sessions (within 90 seconds) grouped by user
        std::string sessions_q =
            "SELECT u.username, "
            "DATE_FORMAT(MAX(s.last_seen),'%Y-%m-%d %H:%i:%s') "
            "FROM sessions s JOIN users u ON u.id = s.user_id "
            "WHERE TIMESTAMPDIFF(SECOND, s.last_seen, NOW()) <= 6 "
            "GROUP BY u.id, u.username "
            "ORDER BY MAX(s.last_seen) DESC";
        auto sessionsQuery = db.Query(sessions_q, {});

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
            << "\"round\":0,"
            << "\"activePlayer\":\"\","
            << "\"phaseIndex\":0,"
            << "\"phase\":\"\","
            << "\"vp\":{\"A\":0,\"B\":0},"
            << "\"bp\":{\"A\":0,\"B\":0},"
            << "\"notes\":\"Game not started\""
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

    GameState s = StateMachine::instance().get_game_state();

    // Build status JSON
    std::ostringstream status_json;
    status_json << "{";
    status_json << "\"gameId\":" << s.game_id << ",";
    status_json << "\"round\":" << s.round << ",";
    status_json << "\"activePlayer\":\"" << json_escape(s.active_player)
                << "\",";
    status_json << "\"phaseIndex\":" << s.phase_index << ",";
    status_json << "\"phase\":\"" << json_escape(s.phase_name()) << "\",";
    status_json << "\"vp\":{\"A\":" << s.vpA << ",\"B\":" << s.vpB << "},";
    status_json << "\"bp\":{\"A\":" << s.creditsA << ",\"B\":" << s.creditsB
                << "},";

    if (!s.combat_summary_json.empty())
    {

        //Logger::instance().info(">>>COMBAT TO SEND:");
        //Logger::instance().info(s.combat_summary_json);
        status_json << "\"combat\":" << s.combat_summary_json << ",";
        // Logger::instance().info("<<<COMBAT SUMMARY SENT");
    }

    status_json << "\"notes\":\"" << json_escape(s.notes()) << "\"";
    status_json << "}";

    // Determine self/opponent info from the requesting player (not
    // active_player!) 'player' is who made the request, 'active_player' is
    // whose turn it is
    char selfOwner = player;
    char oppOwner = (selfOwner == 'A') ? 'B' : 'A';

    // Look up actual usernames from game_seats
    std::string selfUser, oppUser;
    std::string seat_q =
        "SELECT u.username FROM users u "
        "JOIN game_seats gs ON gs.user_id = u.id "
        "WHERE gs.game_id=? AND gs.seat=?";
    auto selfRow = db.Query(seat_q, {game_id, selfOwner});
    auto oppRow = db.Query(seat_q, {game_id, oppOwner});

    selfUser = selfRow.empty() ? "" : selfRow[0][0];
    oppUser = oppRow.empty() ? "" : oppRow[0][0];

    // Query opponent online status
    bool oppOnline = false;
    std::string oppLastSeen;

    std::string lastseen_q =
        "SELECT DATE_FORMAT(last_seen,'%Y-%m-%d %H:%i:%s') FROM "
        "sessions s JOIN users u ON u.id=s.user_id "
        "WHERE u.username=? ORDER BY s.last_seen DESC LIMIT 1";
    auto prow = db.Query(lastseen_q, {oppUser});

    if (!prow.empty())
    {
        oppLastSeen = prow[0][0];
        std::string online_q =
            "SELECT (TIMESTAMPDIFF(SECOND, last_seen, NOW()) <= 6) "
            "FROM sessions s JOIN users u ON u.id=s.user_id "
            "WHERE u.username=? ORDER BY s.last_seen DESC LIMIT 1";
        auto prow2 = db.Query(online_q, {oppUser});

        if (!prow2.empty() && !prow2[0][0].empty() && prow2[0][0] != "0")
        {
            oppOnline = true;
        }
    }

    // Get queued messages for this player (from tell/broadcast)
    auto queued = get_queued_messages(player);

    // Build complete response and set directly
    std::ostringstream out;
    out << "{\"ok\":true,\"state\":" << status_json.str()
        << ",\"self\":{\"owner\":\"" << selfOwner << "\",\"username\":\""
        << json_escape(selfUser) << "\"}"
        << ",\"peer\":{\"owner\":\"" << oppOwner << "\",\"username\":\""
        << oppUser << "\",\"online\":" << (oppOnline ? "true" : "false")
        << ",\"last_seen\":\"" << json_escape(oppLastSeen) << "\"}";

    // Include queued messages if any
    if (!queued.messages.empty())
    {
        out << ",\"messages\":[";
        for (size_t i = 0; i < queued.messages.size(); ++i)
        {
            if (i > 0)
                out << ",";
            out << "\"" << json_escape(queued.messages[i]) << "\"";
        }
        out << "]";
    }

    // Include ships for map rendering
    out << ",\"ships\":" << getShipsJson(game_id, player);

    out << "}";

    resp->body = out.str();

    // Mark messages as sent AFTER response is built
    mark_messages_sent(queued);
}
