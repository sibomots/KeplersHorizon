///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "build_set_attribute_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

bool BuildSetAttributeCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::string draft_code = get_current_draft(m_sm.get_db(), m_sm.get_game_id(), active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to modify");
        Telemetry::write("Error: No current draft to modify");
        return false;
    }

    DraftRow d = load_draft(m_sm.get_db(), m_sm.get_game_id(), active_player, draft_code);

    // Apply attributes using C++17 structured bindings
    for (const auto &[attr_id, value] : m_attributes)
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

    update_draft_attrs(m_sm.get_db(), m_sm.get_game_id(), active_player, draft_code, d);

    std::ostringstream msg;
    msg << "Draft updated: " << draft_code << " [PD=" << d.attr.PD
        << ", B=" << d.attr.B << ", S=" << d.attr.S << ", T=" << d.attr.T
        << ", M=" << d.attr.M << ", SR=" << d.attr.SR << "]";
    Logger::instance().info(msg.str());
    Telemetry::write(msg.str());

    return true;
}
