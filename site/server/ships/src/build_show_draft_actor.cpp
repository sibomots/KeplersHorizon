//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "build_show_draft_actor.h"
#include "buildagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildShowDraftActor::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Note: BuildShowDraft typically doesn't need inhibits checking
    // It's a read-only query command

    // Build parameter
    BuildShowDraftParam bp = BuildShowDraftParam::Builder()
                              .set_game_id(s.game_id)
                              .set_target(m_target)
                              .set_player(owner)
                              .build();

    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::getInstance().apply(bap);
}
