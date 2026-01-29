//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "aigamestate.h"
#include "db.h"
#include "logger.h"
#include "statemachine.h"

AIGameState::AIGameState(int game_id, char ai_player)
    : m_game_id(game_id)
    , m_ai_player(ai_player)
{
}

bool AIGameState::get_credits(int& credits_out) const
{
    bool success = false;
    
    GameState s = StateMachine::instance().get_game_state();
    
    if (m_ai_player == 'A')
    {
        credits_out = s.creditsA;
        success = true;
    }
    else if (m_ai_player == 'B')
    {
        credits_out = s.creditsB;
        success = true;
    }
    else
    {
        credits_out = 0;
        Logger::instance().error("[AI] Invalid player: " + std::string(1, m_ai_player));
    }
    
    return success;
}

bool AIGameState::get_tech_level(int& tech_out) const
{
    bool success = false;
    GameState s = StateMachine::instance().get_game_state();
    tech_out = s.get_current_tech_level();
    return success;
}

bool AIGameState::get_ai_base_hex(std::string& hex_out) const
{
    bool success = false;
    DatabaseManager& db = DatabaseManager::instance();
    
    // Query base_stars table for AI's claimed base
    auto base_rows = db.query(
        "SELECT hex_id FROM base_stars WHERE game_id=" + 
        std::to_string(m_game_id) + " AND owner='" + 
        std::string(1, m_ai_player) + "' LIMIT 1");
    
    if (!base_rows.empty())
    {
        hex_out = base_rows[0][0];
        success = true;
    }
    else
    {
        // Fallback: Query star_systems for base_side matching AI's side
        // Need to get module_id first
        auto module_rows = db.query(
            "SELECT module_id FROM games WHERE id=" + 
            std::to_string(m_game_id) + " LIMIT 1");
        
        if (!module_rows.empty())
        {
            int module_id = std::atoi(module_rows[0][0].c_str());
            
            // Get AI's home side from GameState
            GameState s = StateMachine::instance().get_game_state();
            std::string ai_side;
            
            if (m_ai_player == 'A')
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
                auto sys_rows = db.query(
                    "SELECT hex_id FROM star_systems WHERE module_id=" + 
                    std::to_string(module_id) + " AND is_base=1 AND base_side='" + 
                    db.esc(ai_side) + "' LIMIT 1");
                
                if (!sys_rows.empty())
                {
                    hex_out = sys_rows[0][0];
                    success = true;
                }
            }
        }
        
        if (!success)
        {
            hex_out = "";
            Logger::instance().error("[AI] Failed to find base hex");
        }
    }
    
    return success;
}
