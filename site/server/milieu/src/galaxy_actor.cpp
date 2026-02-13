///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "galaxy_actor.h"

#include <iomanip>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool GalaxyActor::invoke(void)
{
    bool bres = false;
    int game_id = StateMachine::instance().get_game_id();
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::GALAXY,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        bres = false;
    }
    else
    {
        GalaxyParam gp = GalaxyParam::Builder()
                             .set_game_id(s.game_id)
                             .set_player(owner)
                             .build();
        MilieuAgentParam mp(gp);
        bres = MilieuAgent::instance().apply(mp);
    }
    return bres;
}
