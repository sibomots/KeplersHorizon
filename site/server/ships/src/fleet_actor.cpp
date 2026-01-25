//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "fleet_actor.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

#include "buildagent.h"
#include "statemachine.h"

bool BuildFleetListActor::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    BuildFleetListParam bp = BuildFleetListParam::Builder()
                              .set_game_id(s.game_id)
                              // JDW BUGBUG .set_module_id(s.module_id)
                              .set_player(owner)
                              .build();
    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::instance().apply(bap);
}
