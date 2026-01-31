//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "survey_command.h"

#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool SurveyCommand::has_ship_in_system(const std::string& system)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // Get the hex for this system
    auto hex_rows = db.Query("SELECT hex_id FROM star_systems WHERE name=?",
                             {system});
    if (hex_rows.empty())
    {
        return false;
    }
    std::string hex = hex_rows[0][0];

    // Check for ships at this hex
    auto ship_rows = db.Query("SELECT COUNT(*) FROM ships WHERE game_id=? "
                              "AND owner=? AND at_hex=? AND destroyed_at IS NULL",
                              {s.game_id, owner, hex});

    if (ship_rows.empty())
    {
        return false;
    }
    return std::stoi(ship_rows[0][0]) > 0;
}

std::string SurveyCommand::upgrade_knowledge(const std::string& current)
{
    if (current == "Unknown")
        return "Charted";
    if (current == "Rumored")
        return "Charted";
    if (current == "Charted")
        return "Surveyed";
    if (current == "Surveyed")
        return "Intimate";
    return current; // Already Intimate, no change
}

bool SurveyCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // If no system specified, find where player has ships
    std::string target_system = m_system_name;

    if (target_system.empty())
    {
        // Find first system where player has a ship
        auto loc_rows = db.Query("SELECT DISTINCT ss.name FROM ships s "
                                 "JOIN star_systems ss ON s.at_hex = ss.hex_id "
                                 "WHERE s.game_id=? AND s.owner=? "
                                 "AND s.destroyed_at IS NULL LIMIT 1",
                                 {s.game_id, owner});

        if (loc_rows.empty())
        {
            Telemetry::instance().write(
                "SURVEY: No ships available to conduct survey.");
            return false;
        }
        target_system = loc_rows[0][0];
    }

    // Validate system exists
    auto check =
        db.Query("SELECT name FROM star_systems WHERE UPPER(name)=UPPER(?)",
                 {target_system});
    if (check.empty())
    {
        Telemetry::instance().write("SURVEY: Unknown system '" +
                                       target_system + "'");
        return false;
    }
    target_system = check[0][0];

    // Check ship presence
    if (!has_ship_in_system(target_system))
    {
        Telemetry::instance().write("SURVEY: No ships present in " +
                                       target_system +
                                       ".\nYou must have a vessel in-system to "
                                       "conduct survey operations.");
        return false;
    }

    // Get current knowledge level
    auto know_rows = db.Query("SELECT knowledge_level FROM codex_entries "
                              "WHERE game_id=? AND player=? AND system_name=?",
                              {s.game_id, owner, target_system});

    std::string current_level = "Unknown";
    if (!know_rows.empty())
    {
        current_level = know_rows[0][0];
    }

    std::string new_level = upgrade_knowledge(current_level);

    if (new_level == current_level)
    {
        Telemetry::instance().write(
            "SURVEY: " + target_system +
            " already at maximum knowledge level (Intimate).\n"
            "All system secrets are known to you.");
        return true;
    }

    // Update or insert codex entry
    if (know_rows.empty())
    {
        db.Exec("INSERT INTO codex_entries (game_id, player, system_name, "
                "knowledge_level, last_updated_turn) VALUES (?,?,?,?,?)",
                {s.game_id, owner, target_system, new_level, s.round});
    }
    else
    {
        db.Exec("UPDATE codex_entries SET knowledge_level=?, "
                "last_updated_turn=? WHERE game_id=? AND player=? "
                "AND system_name=?",
                {new_level, s.round, s.game_id, owner, target_system});
    }

    // Report success
    std::ostringstream out;
    out << "SURVEY: " << target_system << " survey complete.\n";
    out << "Knowledge upgraded: " << current_level << " -> " << new_level
        << "\n\n";

    // Show what's newly available
    if (new_level == "Charted")
    {
        out << "Planetary data and facility locations now available.\n";
        out << "Use 'system " << target_system << " planets' to view.\n";
    }
    else if (new_level == "Surveyed")
    {
        out << "Detailed resource and population data now available.\n";
        out << "Use 'system " << target_system << " resources' to view.\n";
        out << "Use 'system " << target_system << " population' to view.\n";
    }
    else if (new_level == "Intimate")
    {
        out << "All system secrets revealed, including anomalies.\n";
        out << "Use 'system " << target_system << " anomalies' to view.\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}
