//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "statemachine.h"
#include "combatagent.h"
#include "combat_drafts_actor.h"

#include "db.h"
#include "logger.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

bool CombatDraftsActor::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Are we allowed to use Combat Drafts now?
    std::string inhibit_error;
    if (!StateMachine::getInstance()
               .check_inhibits(CommandID::COMBAT_DRAFTS, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    // Build the CombatDraftsParam
    CombatDraftsParam cp = CombatDraftsParam::Builder()
                              .set_game_id(s.game_id)
                              //JDW BUGBUG  .set_module_id(s.module_id)
                              .set_player(owner)
                              .build();
    
    CombatAgentParam capm(cp);
    bool bresult = CombatAgent::getInstance().apply(capm);
    return bresult;
}
