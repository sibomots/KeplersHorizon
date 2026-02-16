///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "fabricate_actor.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "milieuagent.h"
#include "moduleutil.h"
#include "statemachine.h"
#include "telemetry.h"

bool FabricateActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    int module_id = get_module_id_for_game(s.game_id);

    FabricateParam fp = FabricateParam::Builder()
                            .set_game_id(s.game_id)
                            .set_module_id(module_id)
                            .set_player(owner)
                            .set_mode(m_mode)
                            .set_qty(m_qty)
                            .build();
    MilieuAgentParam mp(fp);
    bres = MilieuAgent::instance().apply(mp);

    return bres;
}

// Translate user-typed plan name to FabricateMode
FabricateMode FabricateActor::plan_name_to_mode(const std::string& name)
{
    FabricateMode result = FabricateMode::LIST_PLANS;

    // BUGBUG WHY are these keyed on strings??
    if (KH_EQU(name, "missiles"))
    {
        result = FabricateMode::FABRICATE_MISSILE;
    }
    else if (KH_EQU(name, "tubes"))
    {
        result = FabricateMode::FABRICATE_TUBE;
    }
    else if (KH_EQU(name, "beams"))
    {
        result = FabricateMode::FABRICATE_BEAM;
    }
    else if (KH_EQU(name, "screens"))
    {
        result = FabricateMode::FABRICATE_SCREEN;
    }
    else if (KH_EQU(name, "tech"))
    {
        result = FabricateMode::FABRICATE_TECH;
    }
    else if (KH_EQU(name, "list"))
    {
        result = FabricateMode::LIST_PLANS;
    }
    return result;
}
