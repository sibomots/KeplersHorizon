///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_TRADE_ACTOR_H__
#define __KH_TRADE_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"
#include "trade_strategy.h"

// TradeActor - Buy/sell resources at trade hubs
// Usage: trade list
//        trade buy <resource> <qty>
//        trade sell <resource> <qty>
//        trade transfer <ship1> <ship2> <resource> <qty>
class TradeActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : mode(TradeMode::TRADE_LIST), qty(0)
        {
        }
        Builder& set_list_mode()
        {
            mode = TradeMode::TRADE_LIST;
            return *this;
        }
        Builder& set_buy_mode()
        {
            mode = TradeMode::TRADE_BUY;
            return *this;
        }
        Builder& set_sell_mode()
        {
            mode = TradeMode::TRADE_SELL;
            return *this;
        }
        Builder& set_transfer_mode()
        {
            mode = TradeMode::TRADE_TRANSFER;
            return *this;
        }
        Builder& set_resource(const std::string& res)
        {
            resource = res;
            return *this;
        }
        Builder& set_qty(int _qty)
        {
            qty = _qty;
            return *this;
        }
        Builder& set_from_ship(const std::string& s)
        {
            srcship = s;
            return *this;
        }
        Builder& set_to_ship(const std::string& s)
        {
            destship = s;
            return *this;
        }
        TradeActor* build()
        {
            return new TradeActor(mode, resource, qty, srcship, destship);
        }

      private:
        TradeMode mode;
        int qty;
        std::string resource;
        std::string srcship;
        std::string destship;
    };

    bool invoke(void) override;

    TradeMode get_trade_mode() const
    {
        return m_mode;
    }
    int get_qty() const
    {
        return m_qty;
    }
    std::string get_src_ship() const
    {
        return m_srcship;
    }
    std::string get_dest_ship() const
    {
        return m_destship;
    }

  private:
    TradeActor(TradeMode mode, const std::string& res, int qty,
               const std::string& from, const std::string& to)
        : m_mode(mode), m_resource(res), m_qty(qty), m_srcship(from),
          m_destship(to)
    {
    }
    TradeMode m_mode;
    std::string m_resource;
    int m_qty;
    std::string m_srcship;
    std::string m_destship;
};

#endif
