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
#include "build_commit_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

bool BuildCommitCommand::invoke(void)
{
    // Get active player from game state
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Get current draft
    std::string draft_code = get_current_draft(m_sm.get_db(), m_sm.get_game_id(), active_player);
    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to commit");
        Telemetry::write("Error: No current draft to commit");
        return false;
    }

    // Validate draft exists
    if (!draft_exists(m_sm.get_db(), m_sm.get_game_id(), active_player, draft_code))
    {
        Logger::instance().error("Draft not found: " + draft_code);
        Telemetry::write("Error: Draft not found: " + draft_code);
        return false;
    }

    // Load draft
    DraftRow d = load_draft(m_sm.get_db(), m_sm.get_game_id(), active_player, draft_code);

    // Validate draft attributes
    if (d.attr.type == 'S' && d.attr.SR != 0)
    {
        Logger::instance().error("SystemShips cannot have SR");
        Telemetry::write("Error: SystemShips cannot have SR");
        return false;
    }
    if (d.attr.M % 3 != 0)
    {
        Logger::instance().error("Missiles must be a multiple of 3");
        Telemetry::write("Error: Missiles must be a multiple of 3");
        return false;
    }
    if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 || d.attr.T < 0 ||
        d.attr.M < 0 || d.attr.SR < 0)
    {
        Logger::instance().error("Negative attribute");
        Telemetry::write("Error: Negative attribute values not allowed");
        return false;
    }

    // Calculate cost
    int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
    cost += (d.attr.M + 2) / 3;
    if (d.attr.type == 'W')
        cost += 5; // Warp generator

    // Check BP availability
    int &bp = (s.active_player == "A") ? s.bpA : s.bpB;
    if (cost > bp)
    {
        Logger::instance().error("Insufficient BP. Need " +
                                 std::to_string(cost) + ", have " +
                                 std::to_string(bp));
        Telemetry::write("Error: Insufficient BP. Need " +
                        std::to_string(cost) + ", have " +
                        std::to_string(bp));
        return false;
    }

    // Compute tech level
    int tech = 0;
    if (s.scenario == "advanced" && s.round >= 1)
    {
        tech = (s.round - 1) / 4;
    }

    // Create ship using factory method
    ShipRow sh = ShipRow::from_draft(d, tech, s.round, s.active_player);

    // Commit to DB
    insert_ship(m_sm.get_db(), m_sm.get_game_id(), s.active_player[0], sh);
    delete_draft(m_sm.get_db(), m_sm.get_game_id(), s.active_player[0], d.code);
    set_current_draft(m_sm.get_db(), m_sm.get_game_id(), s.active_player[0], "");

    // Deduct BP and save
    bp -= cost;
    save_game(m_sm.get_db(), s);

    Logger::instance().info("Committed: " + sh.name + " - " + sh.code + " (L" +
                            std::to_string(sh.attr.tech) +
                            ") cost=" + std::to_string(cost) +
                            " BP. Remaining BP=" + std::to_string(bp));

    Telemetry::write("Committed: " + sh.name + " - " + sh.code + " (L" +
                    std::to_string(sh.attr.tech) + ")");
    Telemetry::write("Cost: " + std::to_string(cost) + " BP, Remaining: " +
                    std::to_string(bp) + " BP");

    return true;
}
