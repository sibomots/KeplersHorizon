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

    // Need GameState to get the game_id
    // Need StateMachine to get thisplayer

    AttributeMap attack_attr(m_power_d, m_power_b, m_power_s, m_power_t);

    CombatOrderParam cp =
        CombatOrderParam::Builder().set_game_id(s.game_id)
                              .set_player(owner)
                              .set_attacker(m_ship_code)
                              .set_attackee(m_target_id)
                              .set_tactic(m_tactic)
                              .set_attr(attack_attr)
                              .set_missiles(m_firing_missiles)
                              .build();
     CombatAgentParam capm(cp);
     bool bresult = CombatAgent::getInstance().apply(capm);
     return bresult;

#if 0
    // Check if this command is allowed given current game state
    CombatOrderParams_t params;
    params.ship_code = m_ship_code;
    params.order_type = m_tactic;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::COMBAT_ORDER,
                                                    &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    // Check for self-targeting (Bug #9)
    if (m_ship_code == m_target_id)
    {
        Telemetry::getInstance().write(
            "TACTICAL: Cannot target your own ship!");
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Build CombatOrder struct
    CombatOrder order;
    order.game_id = s.game_id;
    order.ship_code = m_ship_code;
    order.target_id = m_target_id;
    order.tactic = m_tactic;
    order.round = 0; // Will be set by CombatEngine from current combat state
    order.power_d = m_power_d;
    order.power_b = m_power_b;
    order.power_s = m_power_s;
    order.power_t = m_power_t;
    order.missiles_data = m_missiles_json;

    // Submit order to combat engine
    CombatEngine ce(s.game_id);
    std::string result = ce.submit_order(owner, order);

    Telemetry::getInstance().write(result);

    return true;
#endif
}
