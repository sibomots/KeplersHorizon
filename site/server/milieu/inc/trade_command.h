//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __TRADE_COMMAND_H__
#define __TRADE_COMMAND_H__

#include <string>

#include "icmd.h"

// TradeCommand - Buy/sell resources at trade hubs
// Usage: trade list
//        trade buy <resource> <qty>
//        trade sell <resource> <qty>
//        trade transfer <ship1> <ship2> <resource> <qty>
class TradeCommand : public ICmd
{
  public:
    enum Mode
    {
        MODE_LIST,
        MODE_BUY,
        MODE_SELL,
        MODE_TRANSFER
    };

    class Builder
    {
      public:
        Builder()
            : m_mode(MODE_LIST), m_resource(""), m_quantity(0), m_from_ship(""),
              m_to_ship("")
        {
        }
        Builder& listMode()
        {
            m_mode = MODE_LIST;
            return *this;
        }
        Builder& buyMode()
        {
            m_mode = MODE_BUY;
            return *this;
        }
        Builder& sellMode()
        {
            m_mode = MODE_SELL;
            return *this;
        }
        Builder& transferMode()
        {
            m_mode = MODE_TRANSFER;
            return *this;
        }
        Builder& resource(const std::string& res)
        {
            m_resource = res;
            return *this;
        }
        Builder& quantity(int qty)
        {
            m_quantity = qty;
            return *this;
        }
        Builder& from_ship(const std::string& s)
        {
            m_from_ship = s;
            return *this;
        }
        Builder& to_ship(const std::string& s)
        {
            m_to_ship = s;
            return *this;
        }
        TradeCommand* build()
        {
            return new TradeCommand(m_mode, m_resource, m_quantity, m_from_ship,
                                    m_to_ship);
        }

      private:
        Mode m_mode;
        std::string m_resource;
        int m_quantity;
        std::string m_from_ship;
        std::string m_to_ship;
    };

    bool invoke(void) override;

  private:
    TradeCommand(Mode mode, const std::string& res, int qty,
                 const std::string& from, const std::string& to)
        : m_mode(mode), m_resource(res), m_quantity(qty), m_from_ship(from),
          m_to_ship(to)
    {
    }
    Mode m_mode;
    std::string m_resource;
    int m_quantity;
    std::string m_from_ship;
    std::string m_to_ship;

    void show_prices();
    bool do_buy();
    bool do_sell();
    bool do_transfer();
};

#endif
