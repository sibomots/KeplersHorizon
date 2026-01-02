//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "star_system_constraints.h"

// Stub implementations - all return 0 (no modifier) for now
// Future: Query DB for star system properties and compute modifiers

int StarSystemConstraints::getBeamModifier(const std::string& hex_id)
{
    // TODO: Plasma clouds, radiation could reduce beam effectiveness
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getScreenModifier(const std::string& hex_id)
{
    // TODO: EM interference could reduce screen absorption
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getMissileModifier(const std::string& hex_id)
{
    // TODO: Asteroid fields could affect missile tracking
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getDriveModifier(const std::string& hex_id)
{
    // TODO: Gravity wells could affect maneuverability
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getMovementModifier(const std::string& hex_id)
{
    // TODO: Gravity, debris fields could slow movement
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getRepairModifier(const std::string& hex_id)
{
    // TODO: Hazardous conditions could impair repair
    (void)hex_id;
    return 0;
}

int StarSystemConstraints::getResupplyModifier(const std::string& hex_id)
{
    // TODO: System hazards could limit resupply efficiency
    (void)hex_id;
    return 0;
}
