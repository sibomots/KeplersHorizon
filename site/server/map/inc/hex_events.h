//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __HEX_EVENTS_H__
#define __HEX_EVENTS_H__

#include <string>

// HexEventEngine - Dynamic hex events that affect gameplay
// Events are stored in hex_events table with spawn/expire turns
// Modifier functions query active events and return gameplay modifiers
class HexEventEngine
{
  public:
    // Gameplay modifiers - return 0 if no active event
    static int get_movement_modifier(int game_id, int round, const std::string& hex);
    static int get_combat_modifier(int game_id, int round, const std::string& hex);
    static float get_salvage_multiplier(int game_id, int round, const std::string& hex);
    static int get_extraction_modifier(int game_id, int round, const std::string& hex);

    // Turn-end processing
    static void process_events(int game_id, int round);
};

#endif
