//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __AILOGIC_H__
#define __AILOGIC_H__

#include <string>

// Forward declaration
class AIGameState;

/**
 * @brief AI Logic - Decision engine for AI player actions
 */
class AILogic {
public:
    static AILogic& instance();
    
    enum class BuildDecision {
        SKIP,
        BUILD_WARPSHIP
    };
    
    enum class MovementDecision {
        SKIP,
        MOVE_SHIPS
    };
    
    enum class CombatDecision {
        SKIP,
        DRAFT_ORDERS
    };
    
    enum class PickDropDecision {
        SKIP,
        PICKUP,
        DROP
    };
    
    // Phase-specific decision makers (return bool for success, pass decision by reference)
    bool decide_build_action(const AIGameState& state, BuildDecision& decision, std::string& command_out);
    bool decide_movement_action(const AIGameState& state, MovementDecision& decision, std::string& command_out);
    bool decide_combat_action(const AIGameState& state, CombatDecision& decision, std::string& command_out);
    bool decide_pickup_drop_action(const AIGameState& state, PickDropDecision& decision, std::string& command_out);
    
private:
    AILogic();
    ~AILogic() = default;
    
    AILogic(const AILogic&) = delete;
    AILogic& operator=(const AILogic&) = delete;
    
    int m_ship_counter;
    
    // Build helpers
    bool get_next_ship_name(std::string& name_out);
    bool should_build_ship(const AIGameState& state);
    bool create_build_commands(const AIGameState& state, std::string& command_out);
};

#endif // __AILOGIC_H__
