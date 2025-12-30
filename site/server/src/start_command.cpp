//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "start_command.h"

#include "db.h"
#include "game.h"
#include "logger.h"
#include "telemetry.h"

bool StartCommand::invoke(void)
{
    // Scenario type to string mapping (indexed by ScenarioType enum)
    static const char *scenario_names[] = {
        nullptr,    // UNDEFINED
        "learning", // LEARNING
        "basic",    // BASIC
        "advanced"  // ADVANCED
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
    m_db->exec("DELETE FROM drafts WHERE game_id=" + std::to_string(m_game_id));
    m_db->exec("DELETE FROM ships WHERE game_id=" + std::to_string(m_game_id));
    set_current_draft(m_db, m_game_id, 'A', "");
    set_current_draft(m_db, m_game_id, 'B', "");

    // Save the initialized game state
    save_game(m_db, s);

    Logger::instance().info(
        "Game initialized: " + sc_str + " scenario, Round " +
        std::to_string(s.round) + ", Phase: " + s.phase_name() +
        ", BP A=" + std::to_string(s.bpA) + " B=" + std::to_string(s.bpB));

    Telemetry::write("Game initialized: " + sc_str + " scenario");
    Telemetry::write("Round " + std::to_string(s.round) +
                     ", Phase: " + s.phase_name());
    Telemetry::write("BP A=" + std::to_string(s.bpA) +
                     " B=" + std::to_string(s.bpB));

    return true;
}
