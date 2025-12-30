//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_command.h"

#include "statemachine.h"

bool BuildCommand::invoke(void)
{
    // Delegate to StateMachine to handle build commit logic.
    // The Command's job is simply to set the draft code and trigger transition.
    StateMachine &sm = StateMachine::getInstance();
    sm.set_pending_build_draft(m_draft_code);
    return sm.transition();
}
