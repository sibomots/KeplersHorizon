///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "crt_command.h"

#include <sstream>

#include "telemetry.h"

bool CrtCommand::invoke(void)
{
    static constexpr const char crt_table[] =
        "              COMBAT RESULTS TABLE\n"
        "┌────────┬─────────┬────────┬──────────┬────────┐\n"
        "│Tactic  │ Drive Δ │ vs ATT │ vs DODGE │ vs RET │\n"
        "├────────┼─────────┼────────┼──────────┼────────┤\n"
        "│ATTACK  │ <=-3    │ Miss   │ Miss     │ Escapes│\n"
        "│ATTACK  │ -2,-1   │ Hit    │ Miss     │ Escapes│\n"
        "│ATTACK  │  0,+1   │ Hit+2  │ Miss     │ Miss   │\n"
        "│ATTACK  │ +2      │ Hit+1  │ Hit+1    │ Miss   │\n"
        "│ATTACK  │ +3,+4   │ Miss   │ Hit      │ Hit    │\n"
        "│ATTACK  │ >=+5    │ Miss   │ Miss     │ Miss   │\n"
        "├────────┼─────────┼────────┼──────────┼────────┤\n"
        "│DODGE   │ <=-4    │ Miss   │ Miss     │ Escapes│\n"
        "│DODGE   │ -3,-2   │ Miss   │ Hit      │ Escapes│\n"
        "│DODGE   │ -1,0    │ Hit    │ Hit      │ Escapes│\n"
        "│DODGE   │ +1,+2   │ Hit    │ Miss     │ Escapes│\n"
        "│DODGE   │ >=+3    │ Miss   │ Miss     │ Escapes│\n"
        "├────────┼─────────┼────────┼──────────┼────────┤\n"
        "│RETREAT │ <=-2    │ Miss   │ Miss     │ Escapes│\n"
        "│RETREAT │ -1,0    │ Hit    │ Miss     │ Escapes│\n"
        "│RETREAT │ >=+1    │ Miss   │ Miss     │ Escapes│\n"
        "└────────┴─────────┴────────┴──────────┴────────┘\n"
        "      Damage = Beam Power + Tech Level\n"
        "     >> may be modified by hex events << \n";

    Telemetry::instance().write(crt_table);
    return true;
}
