///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "system_actor.h"

#include <algorithm>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool SystemActor::invoke(void)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    // Validate system exists
    std::string system_name = get_system_name();
    std::string q = "SELECT name FROM star_systems WHERE UPPER(name)=?";
    auto check = db.Query(q, {system_name});

    // let's be defensive..
    if (check.empty())
    {
        Telemetry::instance().write(
            std::format("SYSTEM: Unknown system '{}'", system_name));
        bres = false;
    }
    else
    {
        GameState s = StateMachine::instance().get_game_state();
        char owner = StateMachine::instance().get_current_player();

        SystemMode mode = get_mode();

        if (!(mode > FIRST_SYSTEM_STRATEGY && mode < LAST_SYSTEM_STRATEGY))
        {
            Telemetry::instance().write("SYSTEM: Unknown subcommand. "
                                        "Use: show, planets, resources, "
                                        "population, facilities, anomalies");
            bres = false;
        }
        else
        {
            // Normalize system name from database (we already found it)
            system_name = check[0][0];
            SystemParam sp = SystemParam::Builder()
                                 .set_game_id(s.game_id)
                                 .set_player(owner)
                                 .set_mode(mode)
                                 .set_system_name(system_name)
                                 .build();
            MilieuAgentParam mp(sp);
            bres = MilieuAgent::instance().apply(mp);
        }
    }
    return bres;
}
