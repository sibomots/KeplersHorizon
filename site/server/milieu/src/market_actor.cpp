///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "market_actor.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "milieuagent.h"
#include "moduleutil.h"
#include "statemachine.h"
#include "telemetry.h"

bool MarketActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    int module_id = get_module_id_for_game(s.game_id);

    MarketParam mp = MarketParam::Builder()
                         .set_game_id(s.game_id)
                         .set_module_id(module_id)
                         .set_player(owner)
                         .set_mode(m_mode)
                         .set_resource(m_resource)
                         .build();
    MilieuAgentParam ap(mp);
    bres = MilieuAgent::instance().apply(ap);

    return bres;
}
