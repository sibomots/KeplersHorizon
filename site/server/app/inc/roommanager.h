///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_ROOMMANAGER_H__
#define __KH_ROOMMANAGER_H__

#include <string>
#include <vector>
#include "typedefs.h"

// Room information structure
struct RoomInfo
{
    int id;
    std::string room_code;
    std::string name;
    int created_by;
    std::string creator_name;
    int seat_a;
    std::string seat_a_name;
    int seat_b;
    std::string seat_b_name;
    int game_id;
    std::string status; // "waiting", "ready", "playing", "finished"
    int module_id;      // Universe module (default 1 = Kepler's Horizon)
    std::string created_at;

    bool isFull() const
    {
        return (seat_a > 0) && (seat_b > 0);
    }
    bool isEmpty() const
    {
        return KH_EQU(seat_a, 0) && KH_EQU(seat_b, 0);
    }
};

// RoomManager: Handles room lifecycle and presence, separate from game logic
class RoomManager
{
  public:
    static RoomManager& instance()
    {
        static RoomManager instance;
        return instance;
    }

    RoomManager(const RoomManager&) = delete;
    RoomManager& operator=(const RoomManager&) = delete;

    // Room lifecycle
    std::string createRoom(int user_id, const std::string& name);
    bool joinRoom(const std::string& code, int user_id);
    bool leaveRoom(const std::string& code, int user_id);
    bool deleteRoom(const std::string& code, int user_id);

    // Room queries
    std::vector<RoomInfo> listOpenRooms();
    RoomInfo getRoom(const std::string& code);
    RoomInfo getRoomByUser(int user_id);
    bool roomExists(const std::string& code);

    // Room state
    bool setModule(const std::string& code, int module_id);
    // int startGame(const std::string& code); // Returns game_id
    int startGame(const std::string& code,
                  bool singleplayer = false); // Returns game_id

    // Presence (independent of game)
    bool isUserOnline(int user_id);
    std::vector<int> getOnlineUserIds();
    int countOnlineUsers();

    // Utility
    std::string generateRoomCode();

  private:
    RoomManager()
    {
    }
};

#endif
