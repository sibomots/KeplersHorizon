///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "build_cancel_actor.h"

#include "buildagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildCancelActor::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Check if target is empty
    if (m_target.empty())
    {
        Telemetry::instance().write(
            "SHIPYARD: Need ship hull designator or name to find it");
        return false;
    }

    // Build parameter
    BuildCancelParam bp = BuildCancelParam::Builder()
                              .set_game_id(s.game_id)
                              // JDW .set_module_id(s.module_id)
                              .set_player(owner)
                              .set_target(m_target)
                              .build();

    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::instance().apply(bap);
}
