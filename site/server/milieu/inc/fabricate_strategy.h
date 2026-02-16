///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_FABRICATE_STRATEGY_H__
#define __KH_FABRICATE_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "fabricate_modes.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

class FabricateStrategy
{
  public:
    static bool show_plans(int game_id, int module_id);
    static bool fabricate_missile(int game_id, char owner, int qty);
    static bool fabricate_tube(int game_id, char owner, int qty);
    static bool fabricate_beam(int game_id, char owner, int qty);
    static bool fabricate_screen(int game_id, char owner, int qty);
    static bool fabricate_tech(int game_id, char owner, int qty);

  private:
    static bool check_cargo_cost(int game_id, char owner, int cost[8], int qty);
    static bool deduct_cargo(int game_id, char owner, int cost[8], int qty);

    FabricateStrategy() = default;
    ~FabricateStrategy()
    {
    }
};

#endif
