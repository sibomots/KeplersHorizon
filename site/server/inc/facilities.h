//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __FACILITIES_H__
#define __FACILITIES_H__

#include <string>
#include <vector>

// Facility types
enum FacilityType
{
    FACILITY_SHIPYARD,
    FACILITY_REPAIR_DOCK,
    FACILITY_REFINERY,
    FACILITY_TRADE_HUB,
    FACILITY_FORTRESS,
    FACILITY_BEACON,
    FACILITY_MINING_STATION,
    FACILITY_RESEARCH_LAB
};

struct FacilityInfo
{
    std::string system_name;
    std::string facility_type;
    char controller;      // 'A', 'B', or '\0' for neutral
    int occupied_since;   // Turn when occupation began
    int capture_progress; // Turns of continuous occupation
};

class FacilityEngine
{
  public:
    // Get all facilities in a system
    static std::vector<FacilityInfo> get_facilities(int game_id,
                                                    const std::string& system);

    // Check if player controls a facility type in system
    static bool player_controls(int game_id, const std::string& system,
                                const std::string& facility_type, char player);

    // Initialize facility control for a game (copy from global templates)
    static void initialize_facilities(int game_id);

    // Update facility control based on ship presence (called at turn end)
    static void update_control(int game_id, int round);

    // Get income from controlled trade hubs
    static int calculate_trade_hub_income(int game_id, char player);

    // Check if ship can repair at this location
    static bool can_repair_at(int game_id, const std::string& system,
                              char player);

    // Capture progress constants
    static const int TURNS_TO_CAPTURE = 2;
};

#endif
