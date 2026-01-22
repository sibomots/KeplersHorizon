//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "statemachine.h"
#include "combatagent.h"
#include "combat_order_actor.h"

#include "db.h"
#include "logger.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

bool CombatOrderActor::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Are we allowed to use Combat Order now?
    std::string inhibit_error;
    if (!StateMachine::getInstance()
               .check_inhibits(CommandID::COMBAT_ORDER, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }


    // Need GameState to get the game_id
    // Need StateMachine to get thisplayer

    CombatOrderParam cp =
        CombatOrderParam::Builder().set_game_id(s.game_id)
                              .set_player(owner)
                              .set_attacker(m_ship_code)
                              .set_attackee(m_target_id)
                              .set_tactic(m_tactic)
                              .set_attr(m_attributes)
                              .set_missiles(m_firing_missiles)
                              .build();
     CombatAgentParam capm(cp);
     bool bresult = CombatAgent::getInstance().apply(capm);
     return bresult;
}
