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
#include "start_command.h"

#include "db.h"
#include "game.h"
#include "logger.h"
#include "telemetry.h"


bool StartCommand::invoke(void)
{
    // Scenario type to string mapping (indexed by ScenarioType enum)
    static const char* scenario_names[] = {
        nullptr,      // UNDEFINED
        "learning",   // LEARNING
        "basic",      // BASIC
        "advanced"    // ADVANCED
    };

    // Validate scenario type
    if (m_scenario <= ScenarioType::UNDEFINED || 
        m_scenario >= sizeof(scenario_names) / sizeof(scenario_names[0]))
    {
        Logger::instance().error("Unknown scenario type");
        return false;
    }

    const std::string sc_str(scenario_names[m_scenario]);

    Logger::instance().info("Initializing game scenario: " + sc_str);

    // Create new game state for the scenario
    GameState s = new_game_state_for_scenario(sc_str);
    s.game_id = m_game_id;

    // Clear any existing drafts and ships
    m_db->exec("DELETE FROM drafts WHERE game_id=" +
               std::to_string(m_game_id));
    m_db->exec("DELETE FROM ships WHERE game_id=" + std::to_string(m_game_id));
    set_current_draft(m_db, m_game_id, 'A', "");
    set_current_draft(m_db, m_game_id, 'B', "");

    // Save the initialized game state
    save_game(m_db, s);

    Logger::instance().info("Game initialized: " + sc_str + " scenario, Round " +
                            std::to_string(s.round) + ", Phase: " +
                            s.phase_name() + ", BP A=" + std::to_string(s.bpA) +
                            " B=" + std::to_string(s.bpB));

    Telemetry::write("Game initialized: " + sc_str + " scenario");
    Telemetry::write("Round " + std::to_string(s.round) + ", Phase: " + s.phase_name());
    Telemetry::write("BP A=" + std::to_string(s.bpA) + " B=" + std::to_string(s.bpB));

    return true;
}
