///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_STAR_SYSTEM_CONSTRAINTS_H__
#define __KH_STAR_SYSTEM_CONSTRAINTS_H__

#include <string>

/**
 * StarSystemConstraints - Environmental modifiers per star system.
 *
 * Provides modifiers that affect combat, movement, repair effectiveness
 * based on star system properties (gravity, radiation, asteroids, etc.)
 * Queries the system_constraints table for data-driven modifiers.
 *
 * Returns: -N (weaker), 0 (no effect), +N (stronger)
 */
class StarSystemConstraints
{
  public:
    // Combat modifiers - affect damage/defense calculations
    static int getPhasicModifier(int game_id, const std::string& hex_id);
    static int getShieldModifier(int game_id, const std::string& hex_id);
    static int getTorpedoModifier(int game_id, const std::string& hex_id);
    static int getDriveModifier(int game_id, const std::string& hex_id);

    // Movement/Repair/Resupply modifiers
    static int getMovementModifier(int game_id, const std::string& hex_id);
    static int getRepairModifier(int game_id, const std::string& hex_id);
    static int getResupplyModifier(int game_id, const std::string& hex_id);

  private:
    // Helper to get system name for a hex
    static std::string getSystemForHex(int game_id, const std::string& hex_id);

    // Helper to query constraint value
    static int getConstraintModifier(int game_id, const std::string& hex_id,
                                     const std::string& constraint_type);
};

#endif
