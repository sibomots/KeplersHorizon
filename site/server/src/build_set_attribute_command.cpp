//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_set_attribute_command.h"

#include <sstream>

#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "ships.h"

bool BuildSetAttributeCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::string draft_code = get_current_draft(game_id, active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to modify");
        Telemetry::getInstance().write("Error: No current draft to modify");
        return false;
    }

    DraftRow d = load_draft(game_id, active_player, draft_code);

    // Apply attributes using C++17 structured bindings
    for (const auto& [attr_id, value] : m_attributes)
    {
        switch (attr_id)
        {
        case AttributeID::POWER_DRIVE:
            d.attr.PD = value;
            break;
        case AttributeID::BEAM:
            d.attr.B = value;
            break;
        case AttributeID::SCREEN:
            d.attr.S = value;
            break;
        case AttributeID::TUBE:
            d.attr.T = value;
            break;
        case AttributeID::MISSILE:
            d.attr.M = value;
            break;
        case AttributeID::SYSTEM_RACK:
            d.attr.SR = value;
            break;
        }
    }

    update_draft_attrs(game_id, active_player, draft_code, d);

    std::ostringstream msg;
    msg << "Draft updated: " << draft_code << " [PD=" << d.attr.PD
        << ", B=" << d.attr.B << ", S=" << d.attr.S << ", T=" << d.attr.T
        << ", M=" << d.attr.M << ", SR=" << d.attr.SR << "]";
    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    return true;
}
