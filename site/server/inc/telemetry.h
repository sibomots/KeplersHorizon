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
  public:
    // Core methods - return complete JSON response
    static std::string write(const std::string& msg);
    static std::string tell(PlayerTarget target, const std::string& msg);
    static std::string broadcast(const std::string& msg);

    // Status response - called by heartbeat handler
    static void status(HttpResponse* resp);

  private:
    Telemetry() = delete;
};

#endif
