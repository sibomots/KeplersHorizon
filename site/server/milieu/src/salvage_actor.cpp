///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "salvage_actor.h"

#include <cstdlib>
#include <ctime>
#include <sstream>

#include "db.h"
#include "hex_events.h"
#include "logger.h"
#include "salvage_strategy.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

bool SalvageActor::invoke(void)
{
    if (m_mode == SalvageMode::SALVAGE_SCAN)
    {
        return SalvageStrategy::do_scan();
    }

    if (m_ship_code.empty())
    {
        Telemetry::instance().write("Usage: salvage scan\n"
                                    "       salvage <ship>\n"
                                    "       salvage <ship> <target_name>");
        return false;
    }

    return SalvageStrategy::do_salvage(m_ship_code, m_resource);
}
