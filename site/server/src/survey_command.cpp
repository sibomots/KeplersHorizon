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
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get the hex for this system
    auto hex_rows = db.query("SELECT hex_id FROM star_systems WHERE name='" +
                             db.esc(system) + "'");
    if (hex_rows.empty())
    {
        return false;
    }
    std::string hex = hex_rows[0][0];

    // Check for ships at this hex
    auto ship_rows = db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                              std::to_string(s.game_id) + " AND owner='" +
                              std::string(1, owner) + "' AND at_hex='" +
                              db.esc(hex) + "' AND destroyed_at IS NULL");

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
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // If no system specified, find where player has ships
    std::string target_system = m_system_name;

    if (target_system.empty())
    {
        // Find first system where player has a ship
        auto loc_rows = db.query("SELECT DISTINCT ss.name FROM ships s "
                                 "JOIN star_systems ss ON s.at_hex = ss.hex_id "
                                 "WHERE s.game_id=" +
                                 std::to_string(s.game_id) + " AND s.owner='" +
                                 std::string(1, owner) +
                                 "' AND s.destroyed_at IS NULL LIMIT 1");

        if (loc_rows.empty())
        {
            Telemetry::getInstance().write(
                "SURVEY: No ships available to conduct survey.");
            return false;
        }
        target_system = loc_rows[0][0];
    }

    // Validate system exists
    auto check =
        db.query("SELECT name FROM star_systems WHERE UPPER(name)=UPPER('" +
                 db.esc(target_system) + "')");
    if (check.empty())
    {
        Telemetry::getInstance().write("SURVEY: Unknown system '" +
                                       target_system + "'");
        return false;
    }
    target_system = check[0][0];

    // Check ship presence
    if (!has_ship_in_system(target_system))
    {
        Telemetry::getInstance().write("SURVEY: No ships present in " +
                                       target_system +
                                       ".\nYou must have a vessel in-system to "
                                       "conduct survey operations.");
        return false;
    }

    // Get current knowledge level
    auto know_rows = db.query("SELECT knowledge_level FROM codex_entries "
                              "WHERE game_id=" +
                              std::to_string(s.game_id) + " AND player='" +
                              std::string(1, owner) + "' AND system_name='" +
                              db.esc(target_system) + "'");

    std::string current_level = "Unknown";
    if (!know_rows.empty())
    {
        current_level = know_rows[0][0];
    }

    std::string new_level = upgrade_knowledge(current_level);

    if (new_level == current_level)
    {
        Telemetry::getInstance().write(
            "SURVEY: " + target_system +
            " already at maximum knowledge level (Intimate).\n"
            "All system secrets are known to you.");
        return true;
    }

    // Update or insert codex entry
    std::string turn_str = std::to_string(s.round);
    if (know_rows.empty())
    {
        db.exec("INSERT INTO codex_entries (game_id, player, system_name, "
                "knowledge_level, last_updated_turn) VALUES (" +
                std::to_string(s.game_id) + ", '" + std::string(1, owner) +
                "', '" + db.esc(target_system) + "', '" + new_level + "', '" +
                turn_str + "')");
    }
    else
    {
        db.exec("UPDATE codex_entries SET knowledge_level='" + new_level +
                "', last_updated_turn='" + turn_str +
                "' WHERE game_id=" + std::to_string(s.game_id) +
                " AND player='" + std::string(1, owner) +
                "' AND system_name='" + db.esc(target_system) + "'");
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

    Telemetry::getInstance().write(out.str());
    return true;
}
