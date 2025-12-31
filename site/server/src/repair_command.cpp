//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "repair_command.h"

#include "statemachine.h"

bool RepairCommand::invoke(void)
{
    // Delegate to StateMachine to handle repair logic.
    StateMachine& sm = StateMachine::getInstance();
    sm.set_pending_repair(m_ship_code, m_attribute, m_amount);
    return sm.transition();
}
