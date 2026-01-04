//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __STAR_SYSTEM_CONSTRAINTS_H__
#define __STAR_SYSTEM_CONSTRAINTS_H__

#include <string>

/**
 * StarSystemConstraints - Environmental modifiers per star system.
 *
 * Provides modifiers that affect combat, movement, repair effectiveness
 * based on star system properties (gravity, radiation, asteroids, etc.)
 *
 * Returns: -N (weaker), 0 (no effect), +N (stronger)
 */
class StarSystemConstraints
{
  public:
    static StarSystemConstraints& getInstance()
    {
        static StarSystemConstraints instance;
        return instance;
    }

    // Combat modifiers
    int getBeamModifier(const std::string& hex_id);
    int getScreenModifier(const std::string& hex_id);
    int getMissileModifier(const std::string& hex_id);
    int getDriveModifier(const std::string& hex_id);

    // Future: Movement/Repair/Resupply modifiers
    int getMovementModifier(const std::string& hex_id);
    int getRepairModifier(const std::string& hex_id);
    int getResupplyModifier(const std::string& hex_id);

  private:
    StarSystemConstraints() = default;
    StarSystemConstraints(const StarSystemConstraints&) = delete;
    StarSystemConstraints& operator=(const StarSystemConstraints&) = delete;

    // Future: Load constraints from DB based on star system properties
    // For now, all methods return 0 (no modifier)
};

#endif
