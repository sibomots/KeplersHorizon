///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_OUTFIT_STRATEGY_H__
#define __KH_OUTFIT_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "facilities.h"
#include "logger.h"
#include "outfit_modes.h"
#include "statemachine.h"
#include "telemetry.h"

class OutfitStrategy
{
  public:
    static bool show_equipment(void);
    static bool outfit_lrs(int game_id, char owner,
                           const std::string& ship_code);

  private:
    static bool install_equipment(int game_id, char owner,
                                  const std::string& ship_code,
                                  const std::string& equipment_type);

    OutfitStrategy() = default;
    ~OutfitStrategy()
    {
    }
};

#endif
