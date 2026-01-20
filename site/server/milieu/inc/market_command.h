//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MARKET_COMMAND_H__
#define __MARKET_COMMAND_H__

#include <string>

#include "icmd.h"

// MarketCommand - Display market prices
// Usage: market [<resource>]
class MarketCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_resource("")
        {
        }
        Builder& resource(const std::string& res)
        {
            m_resource = res;
            return *this;
        }
        MarketCommand* build()
        {
            return new MarketCommand(m_resource);
        }

      private:
        std::string m_resource;
    };

    bool invoke(void) override;

  private:
    MarketCommand(const std::string& res) : m_resource(res)
    {
    }
    std::string m_resource;

    void show_all_prices();
    void show_price_history();
};

#endif
