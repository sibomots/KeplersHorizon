//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "hex_events.h"

#include <cstdlib>

#include "db.h"
#include "logger.h"

int HexEventEngine::get_movement_modifier(int game_id, int round, const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.query(
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND hex_id='" + db.esc(hex) +
        "' AND event_type='NAVIGATION_HAZARD'"
        " AND expires_turn > " + std::to_string(round));

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // +N = more PD cost
}

int HexEventEngine::get_combat_modifier(int game_id, int round, const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.query(
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND hex_id='" + db.esc(hex) +
        "' AND event_type='COMBAT_INTERFERENCE'"
        " AND expires_turn > " + std::to_string(round));

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // -N = less damage
}

float HexEventEngine::get_salvage_multiplier(int game_id, int round, const std::string& hex)
{
    if (hex.empty())
    {
        return 1.0f;
    }

    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.query(
        "SELECT COUNT(*) FROM hex_events "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND hex_id='" + db.esc(hex) +
        "' AND event_type='SALVAGE_OPPORTUNITY'"
        " AND expires_turn > " + std::to_string(round));

    if (rows.empty() || rows[0][0] == "0")
    {
        return 1.0f;
    }
    return 1.25f; // +25% yield when active
}

int HexEventEngine::get_extraction_modifier(int game_id, int round, const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.query(
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND hex_id='" + db.esc(hex) +
        "' AND event_type='EXTRACTION_BONUS'"
        " AND expires_turn > " + std::to_string(round));

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // +N = more yield
}

void HexEventEngine::process_events(int game_id, int round)
{
    // Events auto-expire via expires_turn check in queries
    // No cleanup needed - expired events simply return 0 from modifiers
    // Future: spawn random events based on anomalies
    Logger::instance().info("[HEX_EVENTS] Processed events for game " +
                            std::to_string(game_id) + " round " +
                            std::to_string(round));
}
