//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "aiagent.h"
#include "ai_command_injector.h"
#include "logger.h"
#include "statemachine.h"

AIAgent& AIAgent::instance() {
    static AIAgent singleton;
    return singleton;
}

void AIAgent::take_turn(int game_id, char ai_player) {
    Logger::instance().info("[AI] Player " + std::string(1, ai_player) + 
                            " taking turn (game_id=" + std::to_string(game_id) + ")");
    
    // Phase 2 simple implementation: Just advance through all phases
    // We'll issue NEXT commands until we reach end of turn, then DONE
    
    int safety_counter = 0;
    const int MAX_ITERATIONS = 10;  // Prevent infinite loops
    
    Logger::instance().info("[AI] DEBUG: Entering main loop");
    
    while (safety_counter < MAX_ITERATIONS) {
        safety_counter++;
        
        Logger::instance().info("[AI] DEBUG: Loop iteration " + std::to_string(safety_counter));
        
        // Get current game state
        GameState state = StateMachine::instance().get_game_state();
        
        Logger::instance().info("[AI] Phase: " + state.phase_name() + 
                                ", Phase Index: " + std::to_string(state.phase_index) +
                                ", Active Player: " + state.active_player);
        
        // Check if we still have control
        if (state.active_player.empty() || state.active_player[0] != ai_player) {
            Logger::instance().info("[AI] No longer active player, turn complete");
            break;
        }
        
        // For Phase 2, we just skip through all phases with NEXT
        // Phase 3+ will actually do intelligent actions per phase
        
        if (state.phase_index == PH_END_TURN) {
            // End of turn - issue DONE to pass control back
            Logger::instance().info("[AI] End of turn, issuing DONE");
            bool success = AICommandInjector::instance().inject(game_id, ai_player, "DONE");
            if (!success) {
                Logger::instance().error("[AI] Failed to execute DONE");
                break;
            }
            // After DONE, we should no longer be active player
            break;
        } else {
            // Any other phase - just advance
            Logger::instance().info("[AI] Advancing to next phase with NEXT");
            bool success = AICommandInjector::instance().inject(game_id, ai_player, "NEXT");
            if (!success) {
                Logger::instance().error("[AI] Failed to execute NEXT");
                break;
            }
        }
        
        // Small delay to let state update (optional, for debugging)
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (safety_counter >= MAX_ITERATIONS) {
        Logger::instance().error("[AI] Safety limit reached - aborting turn");
    }
    
    Logger::instance().info("[AI] Turn complete");
}
