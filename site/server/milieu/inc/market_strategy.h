///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_MARKET_STRATEGY_H__
#define __KH_MARKET_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "logger.h"
#include "market_modes.h"
#include "statemachine.h"
#include "telemetry.h"

class MarketStrategy
{
  public:
    static bool show_all_prices(int game_id);
    static bool show_price_history(int game_id, const std::string& resource);

  private:
    MarketStrategy() = default;
    ~MarketStrategy()
    {
    }
};

#endif
