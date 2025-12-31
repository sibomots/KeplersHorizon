//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "resupply_command.h"

#include "statemachine.h"

bool ResupplyCommand::invoke(void)
{
    // Delegate to StateMachine to handle resupply logic.
    StateMachine& sm = StateMachine::getInstance();
    sm.set_pending_resupply(m_ship_code, m_missiles);
    return sm.transition();
}
