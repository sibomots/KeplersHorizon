///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "cargo_actor.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "milieuagent.h"
#include "statemachine.h"
#include "telemetry.h"

bool CargoActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::CARGO,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        bres = false;
    }
    else
    {
        CargoParam cp = CargoParam::Builder()
                            .set_game_id(s.game_id)
                            .set_player(owner)
                            .set_ship_code(m_ship_code)
                            .build();
        MilieuAgentParam mp(cp);
        bres = MilieuAgent::instance().apply(mp);
    }
    return bres;
}
