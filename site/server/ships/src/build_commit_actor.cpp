//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "build_commit_actor.h"
#include "buildagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildCommitActor::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::getInstance()
               .check_inhibits(CommandID::BUILD_COMMIT, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    // Build parameter
    BuildCommitParam bp = BuildCommitParam::Builder()
                              .set_game_id(s.game_id)
                              //JDW BUGBUG .set_module_id(s.module_id)
                              .set_player(owner)
                              .set_target(m_target)
                              .build();

    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::getInstance().apply(bap);
}
