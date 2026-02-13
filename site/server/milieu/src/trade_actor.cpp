///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "trade_actor.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "moduleutil.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

bool TradeActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::CARGO,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        bres = false;
    }
    else
    {
        MilieuAgentParam* pmp = nullptr;
        switch (m_mode)
        {
        case TradeMode::TRADE_LIST:
        {
            TradeParam tp = TradeParam::Builder().set_list_mode().build();
            MilieuAgentParam mp(tp);
            bres = MilieuAgent::instance().apply(mp);
            break;
        }
        case TradeMode::TRADE_BUY:
        {
            TradeParam tp = TradeParam::Builder()
                                .set_buy_mode()
                                .set_resource(m_resource)
                                .set_qty(m_qty)
                                .build();
            MilieuAgentParam mp(tp);
            bres = MilieuAgent::instance().apply(mp);
            break;
        }
        case TradeMode::TRADE_SELL:
        {
            TradeParam tp = TradeParam::Builder()
                                .set_sell_mode()
                                .set_resource(m_resource)
                                .set_qty(m_qty)
                                .build();
            MilieuAgentParam mp(tp);
            bres = MilieuAgent::instance().apply(mp);
            break;
        }
        case TradeMode::TRADE_TRANSFER:
        {
            TradeParam tp = TradeParam::Builder()
                                .set_transfer_mode()
                                .set_resource(m_resource)
                                .set_qty(m_qty)
                                .set_srcship(m_srcship)
                                .set_destship(m_destship)
                                .build();
            MilieuAgentParam mp(tp);
            bres = MilieuAgent::instance().apply(mp);
            break;
        }
        }
    }
    return bres;
}
