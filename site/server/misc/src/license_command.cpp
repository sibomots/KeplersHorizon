///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "license_command.h"

#include <sstream>

#include "telemetry.h"

bool LicenseCommand::invoke(void)
{
    static constexpr const char license[] =
        "┌──────────────────────────────────────────────────────────────┐\n"
        "│ Kepler's Horizion is licensed under the BSD 3-Clause License │\n"
        "│ Copyright (c) 2025, sibomots                                 │\n"
        "│ https://keplershorizon.com                                   │\n"
        "└──────────────────────────────────────────────────────────────┘\n";
    Telemetry::instance().write(license);
    return true;
}
