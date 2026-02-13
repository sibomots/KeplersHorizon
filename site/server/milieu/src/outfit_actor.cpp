///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "outfit_actor.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "milieuagent.h"
#include "moduleutil.h"
#include "statemachine.h"
#include "telemetry.h"

bool OutfitActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    int module_id = get_module_id_for_game(s.game_id);

    OutfitParam op = OutfitParam::Builder()
                         .set_game_id(s.game_id)
                         .set_module_id(module_id)
                         .set_player(owner)
                         .set_mode(m_mode)
                         .set_ship_code(m_ship_code)
                         .build();
    MilieuAgentParam ap(op);
    bres = MilieuAgent::instance().apply(ap);

    return bres;
}
