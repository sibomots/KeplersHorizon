///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_TRADE_STRATEGY_H__
#define __KH_TRADE_STRATEGY_H__

#include <sstream>
#include <string>

#include "db.h"
#include "logger.h"
#include "moduleutil.h"
#include "statemachine.h"
#include "telemetry.h"
#include "trade_modes.h"

class TradeStrategy
{
  public:
    // Proxied Methods
    static bool do_buy(const std::string& resource, const int qty);
    static bool do_sell(const std::string& resource, const int qty);
    static bool do_transfer(const std::string& resource, int qty,
                            const std::string& srcship,
                            const std::string& destship);
    static bool show_prices(void);

    // Utilities
    static void track_trade(int game_id, const std::string& res, int qty,
                            bool is_buy);
    static int get_market_price(int game_id, const std::string& res);
    static std::string get_cargo_column(const std::string& res);

  private:
    TradeStrategy() = default;
    ~TradeStrategy()
    {
    }
};

#endif
