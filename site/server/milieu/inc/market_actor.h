///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_MARKET_ACTOR_H__
#define __KH_MARKET_ACTOR_H__

#include <string>

#include "icmd.h"
#include "market_modes.h"
#include "market_strategy.h"
#include "milieuagent.h"

// MarketActor - Display market prices and history
// Usage: market
//        market <resource>
class MarketActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_mode(MarketMode::MARKET_LIST_PRICES)
        {
        }
        Builder& set_list_mode(void)
        {
            m_mode = MarketMode::MARKET_LIST_PRICES;
            return *this;
        }
        Builder& set_history_mode(void)
        {
            m_mode = MarketMode::MARKET_PRICE_HISTORY;
            return *this;
        }
        Builder& set_resource(const std::string& res)
        {
            m_resource = res;
            return *this;
        }
        ICmd* build()
        {
            return new MarketActor(m_mode, m_resource);
        }

      private:
        MarketMode m_mode;
        std::string m_resource;
    };

    bool invoke(void) override;

  private:
    MarketActor(MarketMode mode, const std::string& res)
        : m_mode(mode), m_resource(res)
    {
    }
    MarketMode m_mode;
    std::string m_resource;
};

#endif
