///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "survey_actor.h"

#include <format>
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

    // Resolve the hex for this system
    auto hexRows = db.Query(
        "SELECT hex_id FROM star_systems WHERE UPPER(name)=UPPER(?)",
        {m_system_name});

    if (hexRows.empty())
    {
        Telemetry::instance().write(
            std::format(LC_MILIEU_SYSTEM_TARGET_SYSTEM_UNKNOWN, m_system_name));
        return false;
    }

    std::string hexId = hexRows[0][0];

    // Surveying costs 1 PD — find a player ship at this hex
    // Ships with LRS survey for free
    auto scanShips = db.Query(
        "SELECT ship_code, pd, pd_spent, lrs FROM ships "
        " WHERE game_id=? AND owner=? AND at_hex=? AND destroyed_at IS NULL "
        " ORDER BY lrs DESC, (pd - pd_spent) DESC LIMIT 1",
        {s.game_id, owner, hexId});

    if (scanShips.empty())
    {
        Telemetry::instance().write(LC_MILIEU_SYSTEM_NO_SHIPS_FOR_SURVEY); 
        return false;
    }

    std::string scanShipCode = scanShips[0][0];
    int pdTotal = std::atoi(scanShips[0][1].c_str());
    int pdSpent = std::atoi(scanShips[0][2].c_str());
    int lrs = std::atoi(scanShips[0][3].c_str());
    bool freeScan = (lrs > 0);

    if (!freeScan && (pdTotal - pdSpent) < 1)
    {
        Telemetry::instance().write(
            std::format(LC_MILIEU_SYSTEM_LACK_CR_FOR_SURVEY, 1));
        return false;
    }

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

    // Deduct 1 PD on success (LRS-equipped ships survey for free)
    if (bres && !freeScan)
    {
        db.Exec("UPDATE ships SET pd_spent=pd_spent+1 "
                "WHERE game_id=? AND owner=? AND ship_code=?",
                {s.game_id, owner, scanShipCode});
    }

    return bres;
}
