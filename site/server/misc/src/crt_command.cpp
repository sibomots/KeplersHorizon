//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "crt_command.h"

#include <sstream>

#include "telemetry.h"

bool CrtCommand::invoke(void)
{
    std::ostringstream out;
    out << "         COMBAT RESULTS TABLE\n";
    out << "===========================================\n";
    out << " Firing  | Drive  | vs ATK | vs DOD | vs RET\n";
    out << "---------+--------+--------+--------+--------\n";
    out << " ATTACK  | <=-3   | Miss   | Miss   | Escapes\n";
    out << " ATTACK  | -2,-1  | Hit    | Miss   | Escapes\n";
    out << " ATTACK  |  0,+1  | Hit+2  | Miss   | Miss   \n";
    out << " ATTACK  | +2     | Hit+1  | Hit+1  | Miss   \n";
    out << " ATTACK  | +3,+4  | Miss   | Hit    | Hit    \n";
    out << " ATTACK  | >=+5   | Miss   | Miss   | Miss   \n";
    out << "---------+--------+--------+--------+--------\n";
    out << " DODGE   | <=-4   | Miss   | Miss   | Escapes\n";
    out << " DODGE   | -3,-2  | Miss   | Hit    | Escapes\n";
    out << " DODGE   | -1,0   | Hit    | Hit    | Escapes\n";
    out << " DODGE   | +1,+2  | Hit    | Miss   | Escapes\n";
    out << " DODGE   | >=+3   | Miss   | Miss   | Escapes\n";
    out << "---------+--------+--------+--------+--------\n";
    out << " RETREAT | <=-2   | Miss   | Miss   | Escapes\n";
    out << " RETREAT | -1,0   | Hit    | Miss   | Escapes\n";
    out << " RETREAT | >=+1   | Miss   | Miss   | Escapes\n";
    out << "===========================================\n";
    out << "Damage = Beam Power + Tech Level\n";
    out << "         (may be modified by hex events)\n";

    Telemetry::instance().write(out.str());
    return true;
}
