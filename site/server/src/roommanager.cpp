//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "roommanager.h"

#include <random>
#include <sstream>

#include "db.h"
#include "json.h"
#include "statemachine.h"
#include "telemetry.h"
#include "util.h"

std::string RoomManager::generateRoomCode()
{
    // Generate 6-character alphanumeric code
    static const char chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);

    std::string code;
    for (int i = 0; i < 6; ++i)
    {
        code += chars[dis(gen)];
    }
    return code;
}

std::string RoomManager::createRoom(int user_id, const std::string& name)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Generate unique room code
    std::string code;
    int attempts = 0;
    do
    {
        code = generateRoomCode();
        auto existing = db.query("SELECT id FROM rooms WHERE room_code='" +
                                 db.esc(code) + "'");
        if (existing.empty())
            break;
    } while (++attempts < 10);

    if (attempts >= 10)
    {
        return ""; // Failed to generate unique code
    }

    // Create room with user in seat A
    std::string sql = "INSERT INTO rooms(room_code, name, created_by, seat_a, "
                      "status) VALUES('" +
                      db.esc(code) + "','" + db.esc(name) + "'," +
                      std::to_string(user_id) + "," + std::to_string(user_id) +
                      ",'waiting')";
    db.exec(sql);

    return code;
}

bool RoomManager::joinRoom(const std::string& code, int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Check room exists and has empty seat
    auto rows = db.query(
        "SELECT id, seat_a, seat_b, status FROM rooms WHERE room_code='" +
        db.esc(code) + "'");
    if (rows.empty())
        return false;

    int seat_a = rows[0][1].empty() ? 0 : std::stoi(rows[0][1]);
    int seat_b = rows[0][2].empty() ? 0 : std::stoi(rows[0][2]);
    std::string status = rows[0][3];

    // Can't join if not waiting
    if (status != "waiting")
        return false;

    // Can't join if already in room
    if (seat_a == user_id || seat_b == user_id)
        return false;

    // Join appropriate seat
    if (seat_a == 0)
    {
        db.exec("UPDATE rooms SET seat_a=" + std::to_string(user_id) +
                ", status='ready' WHERE room_code='" + db.esc(code) + "'");
    }
    else if (seat_b == 0)
    {
        db.exec("UPDATE rooms SET seat_b=" + std::to_string(user_id) +
                ", status='ready' WHERE room_code='" + db.esc(code) + "'");
    }
    else
    {
        return false; // Room full
    }

    // Update status to ready if both seats filled
    auto check = db.query("SELECT seat_a, seat_b FROM rooms WHERE room_code='" +
                          db.esc(code) + "'");
    if (!check.empty() && !check[0][0].empty() && !check[0][1].empty())
    {
        db.exec("UPDATE rooms SET status='ready' WHERE room_code='" +
                db.esc(code) + "'");
    }

    return true;
}

bool RoomManager::leaveRoom(const std::string& code, int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows = db.query(
        "SELECT id, seat_a, seat_b, status FROM rooms WHERE room_code='" +
        db.esc(code) + "'");
    if (rows.empty())
        return false;

    int room_id = std::stoi(rows[0][0]);
    int seat_a = rows[0][1].empty() ? 0 : std::stoi(rows[0][1]);
    int seat_b = rows[0][2].empty() ? 0 : std::stoi(rows[0][2]);
    std::string status = rows[0][3];

    // Can't leave during game
    if (status == "playing")
        return false;

    if (seat_a == user_id)
    {
        db.exec("UPDATE rooms SET seat_a=NULL, status='waiting' WHERE id=" +
                std::to_string(room_id));
    }
    else if (seat_b == user_id)
    {
        db.exec("UPDATE rooms SET seat_b=NULL, status='waiting' WHERE id=" +
                std::to_string(room_id));
    }
    else
    {
        return false; // User not in room
    }

    // If room now empty, delete it
    auto check = db.query("SELECT seat_a, seat_b FROM rooms WHERE id=" +
                          std::to_string(room_id));
    if (!check.empty() && check[0][0].empty() && check[0][1].empty())
    {
        db.exec("DELETE FROM rooms WHERE id=" + std::to_string(room_id));
    }

    return true;
}

bool RoomManager::deleteRoom(const std::string& code, int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows =
        db.query("SELECT id, created_by, status FROM rooms WHERE room_code='" +
                 db.esc(code) + "'");
    if (rows.empty())
        return false;

    int created_by = std::stoi(rows[0][1]);
    std::string status = rows[0][2];

    // Only creator can delete, and only if not playing
    if (created_by != user_id)
        return false;
    if (status == "playing")
        return false;

    db.exec("DELETE FROM rooms WHERE room_code='" + db.esc(code) + "'");
    return true;
}

std::vector<RoomInfo> RoomManager::listOpenRooms()
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<RoomInfo> rooms;

    auto rows =
        db.query("SELECT r.id, r.room_code, r.name, r.created_by, u.username, "
                 "r.seat_a, ua.username, r.seat_b, ub.username, "
                 "r.game_id, r.status, r.scenario, r.created_at "
                 "FROM rooms r "
                 "LEFT JOIN users u ON r.created_by = u.id "
                 "LEFT JOIN users ua ON r.seat_a = ua.id "
                 "LEFT JOIN users ub ON r.seat_b = ub.id "
                 "WHERE r.status IN ('waiting', 'ready') "
                 "ORDER BY r.created_at DESC");

    for (const auto& row : rows)
    {
        RoomInfo info;
        info.id = std::stoi(row[0]);
        info.room_code = row[1];
        info.name = row[2];
        info.created_by = row[3].empty() ? 0 : std::stoi(row[3]);
        info.creator_name = row[4];
        info.seat_a = row[5].empty() ? 0 : std::stoi(row[5]);
        info.seat_a_name = row[6];
        info.seat_b = row[7].empty() ? 0 : std::stoi(row[7]);
        info.seat_b_name = row[8];
        info.game_id = row[9].empty() ? 0 : std::stoi(row[9]);
        info.status = row[10];
        info.scenario = row[11];
        info.created_at = row[12];
        rooms.push_back(info);
    }

    return rooms;
}

RoomInfo RoomManager::getRoom(const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    RoomInfo info = {};

    auto rows =
        db.query("SELECT r.id, r.room_code, r.name, r.created_by, u.username, "
                 "r.seat_a, ua.username, r.seat_b, ub.username, "
                 "r.game_id, r.status, r.scenario, r.created_at "
                 "FROM rooms r "
                 "LEFT JOIN users u ON r.created_by = u.id "
                 "LEFT JOIN users ua ON r.seat_a = ua.id "
                 "LEFT JOIN users ub ON r.seat_b = ub.id "
                 "WHERE r.room_code='" +
                 db.esc(code) + "'");

    if (rows.empty())
        return info;

    const auto& row = rows[0];
    info.id = std::stoi(row[0]);
    info.room_code = row[1];
    info.name = row[2];
    info.created_by = row[3].empty() ? 0 : std::stoi(row[3]);
    info.creator_name = row[4];
    info.seat_a = row[5].empty() ? 0 : std::stoi(row[5]);
    info.seat_a_name = row[6];
    info.seat_b = row[7].empty() ? 0 : std::stoi(row[7]);
    info.seat_b_name = row[8];
    info.game_id = row[9].empty() ? 0 : std::stoi(row[9]);
    info.status = row[10];
    info.scenario = row[11];
    info.created_at = row[12];

    return info;
}

RoomInfo RoomManager::getRoomByUser(int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows = db.query(
        "SELECT room_code FROM rooms WHERE (seat_a=" + std::to_string(user_id) +
        " OR seat_b=" + std::to_string(user_id) +
        ") AND status != 'finished' LIMIT 1");

    if (rows.empty())
    {
        return RoomInfo{};
    }

    return getRoom(rows[0][0]);
}

bool RoomManager::roomExists(const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows =
        db.query("SELECT id FROM rooms WHERE room_code='" + db.esc(code) + "'");
    return !rows.empty();
}

bool RoomManager::setScenario(const std::string& code,
                              const std::string& scenario)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    if (scenario != "learning" && scenario != "basic" && scenario != "advanced")
    {
        return false;
    }

    db.exec("UPDATE rooms SET scenario='" + db.esc(scenario) +
            "' WHERE room_code='" + db.esc(code) + "'");
    return true;
}

int RoomManager::startGame(const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get room info
    RoomInfo room = getRoom(code);
    if (room.id == 0)
        return 0;
    if (room.status != "ready")
        return 0;
    if (room.seat_a == 0 || room.seat_b == 0)
        return 0;

    // Use StateMachine to create game
    std::string scenario = room.scenario.empty() ? "basic" : room.scenario;
    GameState gs =
        StateMachine::getInstance().new_game_state_for_scenario(scenario);

    // Randomly assign initiative (who goes first)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    gs.active_player = (dis(gen) == 0) ? "A" : "B";

    // Insert game with room_id
    std::string sql =
        "INSERT INTO games(room_id, scenario, state_json) VALUES(" +
        std::to_string(room.id) + ",'" + db.esc(scenario) + "','" +
        db.esc(gs.to_json()) + "')";
    db.exec(sql);

    // Get new game ID
    auto id_rows = db.query("SELECT LAST_INSERT_ID()");
    if (id_rows.empty())
        return 0;
    int game_id = std::stoi(id_rows[0][0]);

    // Update room
    db.exec("UPDATE rooms SET game_id=" + std::to_string(game_id) +
            ", status='playing' WHERE id=" + std::to_string(room.id));

    // Create game_seats
    db.exec("INSERT INTO game_seats(game_id, user_id, seat) VALUES(" +
            std::to_string(game_id) + "," + std::to_string(room.seat_a) +
            ",'A')");
    db.exec("INSERT INTO game_seats(game_id, user_id, seat) VALUES(" +
            std::to_string(game_id) + "," + std::to_string(room.seat_b) +
            ",'B')");

    // Update both players' sessions with the new game_id
    db.exec("UPDATE sessions SET game_id=" + std::to_string(game_id) +
            " WHERE user_id=" + std::to_string(room.seat_a));
    db.exec("UPDATE sessions SET game_id=" + std::to_string(game_id) +
            " WHERE user_id=" + std::to_string(room.seat_b));

    // Set StateMachine context for this operation
    // This ensures all subsequent state operations (like Telemetry) work
    // correctly
    StateMachine::getInstance().set_game_id(game_id);

    // Look up player usernames
    std::string playerA_name = "PLAYER 1";
    std::string playerB_name = "PLAYER 2";
    auto userA = db.query("SELECT username FROM users WHERE id=" +
                          std::to_string(room.seat_a));
    auto userB = db.query("SELECT username FROM users WHERE id=" +
                          std::to_string(room.seat_b));
    if (!userA.empty())
        playerA_name = userA[0][0];
    if (!userB.empty())
        playerB_name = userB[0][0];

    // Queue initial turn notification for both players
    // The active player (gs.active_player) goes first
    std::string first_player = gs.active_player;
    std::string first_phase = gs.phase_name();
    std::string first_name =
        (first_player == "A") ? playerA_name : playerB_name;

    // Add to telemetry queue for each player (StateMachine context now set)
    if (first_player == "A")
    {
        Telemetry::getInstance().add_tell(
            'A', "COMMAND: YOU HAVE THE INITIATIVE! " + first_phase);
        Telemetry::getInstance().add_tell(
            'B', "COMMAND: " + first_name + " HAS INITIATIVE. STANDING BY...");
    }
    else
    {
        Telemetry::getInstance().add_tell(
            'B', "COMMAND: YOU HAVE THE INITIATIVE! " + first_phase);
        Telemetry::getInstance().add_tell(
            'A', "COMMAND: " + first_name + " HAS INITIATIVE. STANDING BY...");
    }

    return game_id;
}

bool RoomManager::isUserOnline(int user_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT 1 FROM sessions WHERE user_id=" + std::to_string(user_id) +
        " AND TIMESTAMPDIFF(SECOND, last_seen, NOW()) <= 6 LIMIT 1");
    return !rows.empty();
}

std::vector<int> RoomManager::getOnlineUserIds()
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<int> ids;

    auto rows = db.query("SELECT DISTINCT user_id FROM sessions "
                         "WHERE TIMESTAMPDIFF(SECOND, last_seen, NOW()) <= 6");

    for (const auto& row : rows)
    {
        ids.push_back(std::stoi(row[0]));
    }
    return ids;
}

int RoomManager::countOnlineUsers()
{
    return getOnlineUserIds().size();
}
