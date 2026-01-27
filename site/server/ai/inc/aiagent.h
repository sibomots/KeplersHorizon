//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __AIAGENT_H__
#define __AIAGENT_H__

#include <string>

/**
 * @brief AI Agent - Main controller for AI player behavior
 * 
 * Orchestrates AI turn execution by:
 * 1. Detecting when AI has initiative
 * 2. Executing phase-appropriate commands
 * 3. Advancing through phases until turn complete
 * 
 * Phase 2: Simple implementation - just issues NEXT repeatedly
 * Phase 3+: Will load KHS scripts and make intelligent decisions
 */
class AIAgent {
public:
    /**
     * @brief Get singleton instance
     */
    static AIAgent& instance();
    
    /**
     * @brief Execute AI turn
     * 
     * Called by StateMachine::advance_next() when AI gets initiative.
     * For Phase 2: Simply issues NEXT commands to advance through phases.
     * 
     * @param game_id The game ID
     * @param ai_player The AI's player side ('A' or 'B')
     */
    void take_turn(int game_id, char ai_player);
    
private:
    AIAgent() = default;
    ~AIAgent() = default;
    
    // Prevent copying
    AIAgent(const AIAgent&) = delete;
    AIAgent& operator=(const AIAgent&) = delete;
};

#endif // __AIAGENT_H__
