//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "help_command.h"

#include "logger.h"
#include "telemetry.h"

bool HelpCommand::invoke(void)
{
    Logger::instance().info("Show help screen");
    
    std::string help = 
        "══════════════════════════════════════════════════════════════════════\n"
        "BUILD\n"
        "  BN W/S <code>              Create new Warpship/Systemship\n"
        "  BS <code> PD=# B=# S=# T=# M=#   Set ship attributes\n"
        "  BC                         Commit ship draft to fleet\n"
        "  BX                         Cancel current draft\n"
        "  BD                         Show pending drafts\n"
        "MOVE\n"
        "  M <ship> <hex>             Move ship to hex\n"
        "  PICK <ship>                Rack systemship into warpship\n"
        "  DROP <ship>                Unrack systemship from warpship\n"
        "  DEPLOY <ship>              Deploy systemship at current hex\n"
        "COMBAT\n"
        "  C STATUS                   Show active combats\n"
        "  CO <ship> A/D/E <target>   Attack/Defend/Evade order\n"
        "  CD <ship> <dmg>            Assign damage to ship\n"
        "  CC                         Commit combat orders\n"
        "  CX                         Cancel combat orders\n"
        "TURN\n"
        "  FF                         Fast-forward (end current phase)\n"
        "INFO\n"
        "  STATUS | FLEET | SCORE | SYSTEM <name> | SURVEY | CRT\n"
        "ECONOMY\n"
        "  EXTRACT | MARKET | TRADE | FABRICATE\n"
        "══════════════════════════════════════════════════════════════════════\n"
        "B=BEAM S=SCREEN T=TUBE M=MISSILES PD=POWERDRIVE W=WARP S=SYSTEM\n"
        "WIKI: github.com/sibomots/KeplersHorizon/wiki/How-to-Play";
    
    Telemetry::getInstance().write(help);
    return true;
}
