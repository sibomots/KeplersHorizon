///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "survey_strategy.h"

#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "survey_modes.h"
#include "telemetry.h"

bool SurveyStrategy::has_ship_in_system(const std::string& system)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // Get the hex for this system
    auto hex_rows =
        db.Query("SELECT hex_id FROM star_systems WHERE name=?", {system});
    if (hex_rows.empty())
    {
        bres = false;
    }
    else
    {
        std::string hex = hex_rows[0][0];
        // Check for ships at this hex
        auto ship_rows =
            db.Query("SELECT COUNT(*) FROM ships WHERE game_id=? "
                     "AND owner=? AND at_hex=? AND destroyed_at IS NULL",
                     {s.game_id, owner, hex});
        if (ship_rows.empty())
        {
            bres = false;
        }
        else
        {
            bres = (std::stoi(ship_rows[0][0]) > 0);
        }
    }
    return bres;
}

// BIGBUG some kind of knowledge ladder.  We need a more refined way to do this.
std::string SurveyStrategy::upgrade_knowledge(const std::string& current)
{
    if (KH_EQU(current, "Unknown"))
    {
        return "Charted";
    }
    if (KH_EQU(current, "Rumored"))
    {
        return "Charted";
    }
    if (KH_EQU(current, "Charted"))
    {
        return "Surveyed";
    }
    if (KH_EQU(current, "Surveyed"))
    {
        return "Intimate";
    }
    return current; // Already Intimate, no change
}
