///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "combat_commit_actor.h"

#include "combatagent.h"
#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

bool CombatCommitActor::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Are we allowed to use Combat Commit now?
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::COMBAT_COMMIT,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    // Build the CombatCommitParam
    CombatCommitParam cp = CombatCommitParam::Builder()
                               .set_game_id(s.game_id)
                               // BIGBUG .set_module_id(s.module_id)
                               .set_player(owner)
                               .build();

    CombatAgentParam capm(cp);
    bool bresult = CombatAgent::instance().apply(capm);
    return bresult;
}
