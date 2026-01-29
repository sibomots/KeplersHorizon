//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "ailogic.h"
#include "aigamestate.h"
#include "ai_ship_names.h"
#include "logger.h"
#include <sstream>

AILogic& AILogic::instance()
{
    static AILogic singleton;
    return singleton;
}

AILogic::AILogic()
    : m_ship_counter(0)
{
}

bool AILogic::get_next_ship_name(std::string& name_out)
{
    bool success = false;
    
    if (m_ship_counter < static_cast<int>(AIShipNames::names.size()))
    {
        name_out = AIShipNames::names[m_ship_counter];
        m_ship_counter++;
        success = true;
    }
    else
    {
        Logger::instance().error("[AI] Ran out of ship names!");
        name_out = "";
    }
    
    return success;
}

bool AILogic::should_build_ship(const AIGameState& state)
{
    bool can_build = false;
    int credits = 0;
    
    if (state.get_credits(credits))
    {
        can_build = (credits >= 10);
    }
    
    return can_build;
}

bool AILogic::create_build_commands(const AIGameState& state, std::string& command_out)
{
    bool success = false;
    std::string ship_name;
    std::string base_hex;
    
    if (!get_next_ship_name(ship_name))
    {
        Logger::instance().error("[AI] Cannot get ship name");
    }
    else if (!state.get_ai_base_hex(base_hex))
    {
        Logger::instance().error("[AI] No base hex found, cannot deploy ship");
    }
    else
    {
#if 0
        // BUGBUG: THIS IS WRONG.  We don't send multiple commands.
        // We do not send commands separated by semi-colons.. The lexical
        // scanner for  'commands' doesn't allow this.
        std::ostringstream cmd;
        cmd << "bn w1 " << ship_name << " ; "
            << "bc " << ship_name << " ; "
            << "deploy " << ship_name << " " << base_hex;
       
        command_out = cmd.str();
#endif 
        success = true;
        
        Logger::instance().info("[AI] Building ship: " + ship_name + " at " + base_hex);
    }
    
    return success;
}

bool AILogic::decide_build_action(const AIGameState& state, BuildDecision& decision, std::string& command_out)
{
    bool success = true;
    
    if (should_build_ship(state))
    {
        if (create_build_commands(state, command_out))
        {
            decision = BuildDecision::BUILD_WARPSHIP;
        }
        else
        {
            decision = BuildDecision::SKIP;
            command_out = "NEXT";
            Logger::instance().error("[AI] Failed to create build commands, skipping");
        }
    }
    else
    {
        int credits = 0;
        state.get_credits(credits);
        
        decision = BuildDecision::SKIP;
        command_out = "NEXT";
        Logger::instance().info("[AI] Skipping build (credits=" + std::to_string(credits) + ")");
    }
    
    return success;
}

bool AILogic::decide_movement_action(const AIGameState& state, MovementDecision& decision, std::string& command_out)
{
    decision = MovementDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().info("[AI] Skipping movement (Phase 3.1)");
    return true;
}

bool AILogic::decide_combat_action(const AIGameState& state, CombatDecision& decision, std::string& command_out)
{
    decision = CombatDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().info("[AI] Skipping combat (Phase 3.1)");
    return true;
}

bool AILogic::decide_pickup_drop_action(const AIGameState& state, PickDropDecision& decision, std::string& command_out)
{
    decision = PickDropDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().info("[AI] Skipping pickup/drop (Phase 3.1)");
    return true;
}
