///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "aigamestate.h"

#include "db.h"
#include "logger.h"
#include "statemachine.h"

AIGameState::AIGameState(int game_id, char ai_player)
    : m_game_id(game_id), m_ai_player(ai_player)
{
}

bool AIGameState::get_credits(int& credits_out) const
{
    bool bres = false;

    GameState s = StateMachine::instance().get_game_state();

    if (KH_EQU(m_ai_player, 'A'))
    {
        credits_out = s.creditsA;
        bres = true;
    }
    else if (KH_EQU(m_ai_player, 'B'))
    {
        credits_out = s.creditsB;
        bres = true;
    }
    else
    {
        credits_out = 0;
        Logger::instance().error("[AI] Invalid player: " +
                                 std::string(1, m_ai_player));
    }

    return bres;
}

bool AIGameState::get_tech_level(int& tech_out) const
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    tech_out = s.get_current_tech_level();
    return bres;
}

bool AIGameState::get_ai_base_hex(std::string& hex_out) const
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    // Query base_stars table for AI's claimed base
    std::string q =
        "SELECT hex_id FROM base_stars WHERE game_id=? AND owner=? LIMIT 1";

    auto base_rows = db.Query(q, std::vector<SqlArg>{m_game_id, m_ai_player});

    if (!base_rows.empty())
    {
        hex_out = base_rows[0][0];
        bres = true;
    }
    else
    {
        // Fallback: Query star_systems for base_side matching AI's side
        // Need to get module_id first
        std::string qq = "SELECT module_id FROM games WHERE id=? LIMIT 1";
        auto module_rows = db.Query(qq, std::vector<SqlArg>{m_game_id});

        if (!module_rows.empty())
        {
            int module_id = std::atoi(module_rows[0][0].c_str());

            // Get AI's home side from GameState
            GameState s = StateMachine::instance().get_game_state();
            std::string ai_side;

            if (KH_EQU(m_ai_player, 'A'))
            {
                ai_side = s.home_side_A;
            }
            else
            {
                ai_side = s.home_side_B;
            }

            if (!ai_side.empty())
            {
                // Query for base with matching base_side
                std::string qh =
                    "SELECT hex_id FROM star_systems WHERE module_id=?"
                    " AND is_base=1 AND base_side=? LIMIT 1";

                auto sys_rows = db.Query(qh, {module_id, ai_side});

                if (!sys_rows.empty())
                {
                    hex_out = sys_rows[0][0];
                    bres = true;
                }
            }
        }

        if (!bres)
        {
            hex_out = "";
            Logger::instance().error("[AI] Failed to find base hex");
        }
    }

    return bres;
}
