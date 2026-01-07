//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __CONSTRAINTS_H__
#define __CONSTRAINTS_H__

#include <string>
#include <vector>

// System constraint types
enum ConstraintType
{
    CONSTRAINT_MOVEMENT,
    CONSTRAINT_COMBAT,
    CONSTRAINT_TRADE,
    CONSTRAINT_HARVEST,
    CONSTRAINT_BUILD
};

// Modifier types
enum ModifierType
{
    MODIFIER_BONUS,
    MODIFIER_PENALTY,
    MODIFIER_BLOCK
};

struct SystemConstraint
{
    std::string system_name;
    ConstraintType type;
    ModifierType modifier;
    int value;
    std::string condition;
    std::string source;
};

class ConstraintEngine
{
  public:
    // Get movement cost modifier for entering a system
    // Returns: PD cost adjustment (positive = more expensive, -1 = blocked)
    static int get_movement_modifier(int game_id, const std::string& system);

    // Check if movement to system is blocked
    static bool is_movement_blocked(int game_id, const std::string& system);

    // Get combat modifier for a system
    // Returns: modifier to apply (positive = bonus, negative = penalty)
    static int get_combat_modifier(int game_id, const std::string& system,
                                   char player);

    // Get extraction modifier for a system
    static int get_extraction_modifier(int game_id, const std::string& system);

    // Check if extraction requires drones (hazardous)
    static bool requires_drones(int game_id, const std::string& system,
                                const std::string& resource);

    // Get all constraints for a system
    static std::vector<SystemConstraint>
    get_constraints(int game_id, const std::string& system);

    // Load constraints from facilities and anomalies
    static void initialize_constraints(int game_id);
};

#endif
