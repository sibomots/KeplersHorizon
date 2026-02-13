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
#include "salvage_actor.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

bool SalvageActor::invoke(void)
{
    bool bres = false;
#ifdef NEEDS_REFACTOR
    if (m_scan_mode)
    {
        do_scan();
        return true;
    }

    if (m_ship_code.empty())
    {
        Telemetry::instance().write("Usage: salvage scan\n"
                                    "       salvage <ship>\n"
                                    "       salvage <ship> <target_name>");
        return true;
    }

    return do_salvage();
#endif
    return bres;
}
