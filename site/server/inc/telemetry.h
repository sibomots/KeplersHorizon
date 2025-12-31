//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include <mutex>
#include <string>
#include <vector>

#include "comms.h"
#include "typedefs.h"

// Player targeting - clean enum-based API
// Future: migrate entire codebase from char 'A'/'B' to this pattern
enum class PlayerTarget
{
    ME,  // Current player (who executed the command)
    THEM // Opponent player
};

class Telemetry
{
  private:
    std::vector<std::string> write_buffer;
    
    // Singleton
    Telemetry() = default;
    Telemetry(const Telemetry&) = delete;
    Telemetry& operator=(const Telemetry&) = delete;
    
  public:
    static Telemetry& getInstance()
    {
        static Telemetry instance;
        return instance;
    }
    
    // Core methods - return complete JSON response
    std::string write(const std::string& msg);
    std::string tell(PlayerTarget target, const std::string& msg);
    std::string broadcast(const std::string& msg);

    // Status response - called by heartbeat handler
    void status(char player, HttpResponse* resp);
    
    // Message accumulation for write() - used during command execution
    void clear_messages();
    void add_message(const std::string& msg);
    std::vector<std::string> get_messages();
    
    // Message queuing for tell/broadcast - delivered via heartbeat (DB-backed)
    void add_tell(char player, const std::string& msg);
    void add_broadcast(const std::string& msg);
    std::vector<std::string> get_queued_messages(char player);
};

#endif
