//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "start_command.h"

#include "telemetry.h"
#include "logger.h"
#include "statemachine.h"
#include "db.h"
#include "ships.h"

bool StartCommand::invoke(void)
{
    // Scenario type to string mapping (indexed by ScenarioType enum)
    static const char* scenario_names[] = {
        nullptr,    // UNDEFINED
        "learning", // LEARNING
        "basic",    // BASIC
        "advanced"  // ADVANCED
    };

    DatabaseManager& db = DatabaseManager::getInstance();

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
    GameState s =
        StateMachine::getInstance().new_game_state_for_scenario(sc_str);

    // Create a new game in the database
    std::string state = s.to_json();
    std::string ins =
        "INSERT INTO games(scenario,state_json) VALUES('" +
        db.esc(sc_str) + "','" + db.esc(state) + "')";
    db.exec(ins);

    // Get the new game ID
    auto r = db.query("SELECT LAST_INSERT_ID()");
    int new_game_id = std::atoi(r[0][0].c_str());
    s.game_id = new_game_id;

    // Set the game_id in the StateMachine so subsequent commands use it
    StateMachine::getInstance().set_game_id(new_game_id);

    // Note: Map data (star_systems, warplines, hexes) is shared across all games
    // using game_id=1. No need to copy - map queries always use game_id=1.

    // Clear any existing drafts and ships for this game
    db.exec("DELETE FROM drafts WHERE game_id=" + std::to_string(new_game_id));
    db.exec("DELETE FROM ships WHERE game_id=" + std::to_string(new_game_id));
    set_current_draft(new_game_id, 'A', "");
    set_current_draft(new_game_id, 'B', "");

    // Save the initialized game state
    StateMachine::getInstance().save_game(s);

    Logger::instance().info(
        "Game initialized: " + sc_str + " scenario (game_id=" + 
        std::to_string(new_game_id) + "), Round " +
        std::to_string(s.round) + ", Phase: " + s.phase_name() +
        ", BP A=" + std::to_string(s.bpA) + " B=" + std::to_string(s.bpB));

    Telemetry::getInstance().write("Game initialized: " + sc_str + " scenario");
    Telemetry::getInstance().write("Round " + std::to_string(s.round) +
                     ", Phase: " + s.phase_name());
    Telemetry::getInstance().write("BP A=" + std::to_string(s.bpA) +
                     " B=" + std::to_string(s.bpB));

    return true;
}

