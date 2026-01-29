//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __AIGAMESTATE_H__
#define __AIGAMESTATE_H__

#include <string>

/**
 * @brief AI Game State - Database query wrapper for AI decision making
 * 
 * Provides convenient access to game state information needed by AILogic.
 * Does NOT cache state - queries fresh from database each time.
 * StateMachine is the single source of truth.
 */
class AIGameState {
public:
    AIGameState(int game_id, char ai_player);
    
    // Resource queries (pass by reference, return bool for success)
    bool get_credits(int& credits_out) const;
    bool get_tech_level(int& tech_out) const;
    
    // Hex queries
    bool get_ai_base_hex(std::string& hex_out) const;
    
private:
    int m_game_id;
    char m_ai_player;
    
    // NO cached state - query database each time
};

#endif // __AIGAMESTATE_H__
