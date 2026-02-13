///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "survey_actor.h"

#include <sstream>

#include "db.h"
#include "milieuagent.h"
#include "statemachine.h"
#include "telemetry.h"

bool SurveyActor::invoke(void)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // If no system specified, find where player has ships
    std::string target_system = m_system_name;

    // BUGBUG
    // We need a better way or more interesting way to
    // establish the robustness of the survey capability

    switch (m_mode)
    {
    case SurveyMode::SURV_NONE:
    case SurveyMode::SURV_BASIC:
    case SurveyMode::SURV_ENHANCED:
        SurveyParam sm = SurveyParam::Builder()
                             .set_game_id(s.game_id)
                             .set_player(owner)
                             .set_system_name(m_system_name)
                             .set_mode(m_mode)
                             .build();
        MilieuAgentParam mp(sm);
        bres = MilieuAgent::instance().apply(mp);
        break;
    }
    return bres;
}
