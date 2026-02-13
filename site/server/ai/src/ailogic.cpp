///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "ailogic.h"
#include <format>
#include <sstream>

#include "ai_ship_names.h"
#include "aigamestate.h"
#include "logger.h"

AILogic& AILogic::instance()
{
    static AILogic singleton;
    return singleton;
}

AILogic::AILogic() : m_ship_counter(0)
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

bool AILogic::create_build_commands(const AIGameState& state,
                                    std::string& command_out)
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
        Logger::instance().ai("Building ship: " + ship_name + " at " +
                                base_hex);
        success = true;
    }
    return success;
}

bool AILogic::decide_build_action(const AIGameState& state,
                                  BuildDecision& decision,
                                  std::string& command_out)
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
            Logger::instance().error(
                "[AI] Failed to create build commands, skipping");
        }
    }
    else
    {
        int credits = 0;
        state.get_credits(credits);

        decision = BuildDecision::SKIP;
        command_out = "NEXT";
        Logger::instance().ai(std::format(
            "Skipping build (credits={})", credits));
    }

    return success;
}

bool AILogic::decide_movement_action(const AIGameState& state,
                                     MovementDecision& decision,
                                     std::string& command_out)
{
    decision = MovementDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().ai("Skipping movement phase");
    return true;
}

bool AILogic::decide_combat_action(const AIGameState& state,
                                   CombatDecision& decision,
                                   std::string& command_out)
{
    decision = CombatDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().ai("Skipping combat phase");
    return true;
}

bool AILogic::decide_pickup_drop_action(const AIGameState& state,
                                        PickDropDecision& decision,
                                        std::string& command_out)
{
    decision = PickDropDecision::SKIP;
    command_out = "NEXT";
    Logger::instance().ai("Skipping pickup/drop phase");
    return true;
}
