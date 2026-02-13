///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "build_drafts_actor.h"

#include "buildagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildDraftsActor::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Note: BuildDrafts typically doesn't need inhibits checking
    // It's a read-only query command

    // Build parameter
    BuildDraftsParam bp = BuildDraftsParam::Builder()
                              .set_game_id(s.game_id)
                              // BIGBUG .set_module_id(s.module_id)
                              .set_player(owner)
                              .build();

    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::instance().apply(bap);
}
