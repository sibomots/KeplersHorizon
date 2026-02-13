///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SALVAGE_STRATEGY_H__
#define __KH_SALVAGE_STRATEGY_H__

#include <sstream>
#include <string>

#include "db.h"
#include "logger.h"
#include "moduleutil.h"
#include "salvage_modes.h"
#include "statemachine.h"
#include "telemetry.h"

class SalvageStrategy
{
  public:
    // Proxied Methods
    static bool do_scan();
    static bool do_salvage(const std::string& ship_code,
                           const std::string& target);

  private:
    SalvageStrategy() = default;
    ~SalvageStrategy()
    {
    }
};

#endif
