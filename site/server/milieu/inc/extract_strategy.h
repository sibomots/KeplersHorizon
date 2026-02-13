///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_EXTRACT_STRATEGY_H__
#define __KH_EXTRACT_STRATEGY_H__

#include <sstream>

#include "db.h"
#include "extract_modes.h"
#include "hex_events.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

class ExtractStrategy
{
  public:
    static bool do_scan(void);
    static bool do_extract(const std::string& ship_code,
                           const std::string& resource);

  private:
    ExtractStrategy() = default;
    ~ExtractStrategy()
    {
    }
};

#endif
