///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_STAR_SYSTEM_STRATEGY_H__
#define __KH_STAR_SYSTEM_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "hex_events.h"
#include "logger.h"
#include "statemachine.h"
#include "system_modes.h"
#include "telemetry.h"

class SystemStrategy
{
  public:
    static bool show_overview(const std::string& system_name);
    static bool show_planets(const std::string& system_name);
    static bool show_resources(const std::string& system_name);
    static bool show_populations(const std::string& system_name);
    static bool show_facilities(const std::string& system_name);
    static bool show_anomalies(const std::string& system_name);

    // Get player's knowledge level for this system
    static std::string get_knowledge_level(const std::string& system_name);
    static int knowledge_rank(const std::string& level);

  private:
    SystemStrategy() = default;
    ~SystemStrategy()
    {
    }
};

#endif
