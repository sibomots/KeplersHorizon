//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __TURN_END_H__
#define __TURN_END_H__

// Turn-End Hook System
// Called when a full round completes (both players finish their turns)
// Handles: facility control updates, market price fluctuations,
// resource regeneration, trade hub income

class TurnEndProcessor
{
  public:
    // Main entry point - called from StateMachine when round increments
    static void on_round_complete(int game_id, int completed_round);

    // Individual subsystems (called by on_round_complete)
    static void update_facilities(int game_id, int round);
    static void update_market_prices(int game_id, int round);
    static void regenerate_resources(int game_id, int round);
    static void apply_trade_hub_income(int game_id, int round);
};

#endif
