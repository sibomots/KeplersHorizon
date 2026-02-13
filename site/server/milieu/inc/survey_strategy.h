///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SURVEY_STRATEGY_H__
#define __KH_SURVEY_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "hex_events.h"
#include "logger.h"
#include "statemachine.h"
#include "survey_modes.h"
#include "telemetry.h"

class SurveyStrategy
{
  public:
    // Check if player has a ship in the given system
    static bool has_ship_in_system(const std::string& system);

    // Upgrade Milieu Codex knowledge level
    static std::string upgrade_knowledge(const std::string& current);

  private:
    SurveyStrategy() = default;
    ~SurveyStrategy()
    {
    }
};

#endif
