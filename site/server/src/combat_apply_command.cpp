//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "combat_apply_command.h"

#include "combat.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool CombatApplyCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    CombatOrderParams_t params;
    params.ship_code = m_ship_code;
    params.order_type = 0;  // Apply command type
    
    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::COMBAT_FIRE, &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Apply damage via combat engine
    CombatEngine ce(s.game_id);
    std::string result = ce.apply_damage(owner, m_ship_code, m_assignments);
    
    Telemetry::getInstance().write(result);
    
    return true;
}
