//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "build_new_actor.h"

#include "buildagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildNewActor::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::BUILD_NEW,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    // Determine ship type from first character of ship code
    char ship_type = 'W'; // Default to Warship
    if (!m_ship_code.empty())
    {
        char code = (char)m_ship_code[0];
        switch (code)
        {
        case 'W':
        case 'w':
            ship_type = 'W';
            break;
        case 'S':
        case 's':
            ship_type = 'S';
            break;
        default:
            ship_type = 'W';
        }
    }

    // Build parameter
    BuildNewParam bp = BuildNewParam::Builder()
                           .set_game_id(s.game_id)
                           // JDW BUGBUG .set_module_id(s.module_id)
                           .set_player(owner)
                           .set_ship_type(ship_type)
                           .set_ship_code(m_ship_code)
                           .set_ship_name(m_ship_name)
                           .build();

    // Dispatch to agent
    BuildAgentParam bap(bp);
    return BuildAgent::instance().apply(bap);
}
