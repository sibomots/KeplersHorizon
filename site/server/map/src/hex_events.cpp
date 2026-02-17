///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "hex_events.h"

#include <cstdlib>
#include <ctime>
#include <format>

#include "db.h"
#include "logger.h"
#include "tuning_state.h"

int HexEventEngine::get_movement_modifier(int game_id, int round,
                                          const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    std::string qq =
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=? AND hex_id=? AND event_type='NAVIGATION_HAZARD'"
        " AND expires_turn > ?";
    auto rows = db.Query(qq, {game_id, hex, round});

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // +N = more PD cost
}

int HexEventEngine::get_combat_modifier(int game_id, int round,
                                        const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    std::string qq =
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=? AND hex_id=? AND event_type='COMBAT_INTERFERENCE'"
        " AND expires_turn > ?";
    auto rows = db.Query(qq, {game_id, hex, round});

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // -N = less damage
}

float HexEventEngine::get_salvage_multiplier(int game_id, int round,
                                             const std::string& hex)
{
    if (hex.empty())
    {
        return 1.0f;
    }

    DatabaseManager& db = DatabaseManager::instance();

    std::string qq =
        "SELECT COUNT(*) FROM hex_events "
        "WHERE game_id=? AND hex_id=? AND event_type='SALVAGE_OPPORTUNITY'"
        " AND expires_turn > ?";
    auto rows = db.Query(qq, {game_id, hex, round});

    if (rows.empty() || KH_EQU(rows[0][0], "0"))
    {
        return 1.0f;
    }
    return 1.25f; // +25% yield when active
}

int HexEventEngine::get_extraction_modifier(int game_id, int round,
                                            const std::string& hex)
{
    if (hex.empty())
    {
        return 0;
    }

    DatabaseManager& db = DatabaseManager::instance();

    std::string qq =
        "SELECT COALESCE(SUM(modifier_value),0) FROM hex_events "
        "WHERE game_id=? AND hex_id=? AND event_type='EXTRACTION_BONUS'"
        " AND expires_turn > ? ";
    auto rows = db.Query(qq, {game_id, hex, round});

    if (rows.empty() || rows[0][0].empty())
    {
        return 0;
    }
    return std::atoi(rows[0][0].c_str()); // +N = more yield
}

void HexEventEngine::process_events(int game_id, int round)
{
    TuningState& ts = TuningState::instance();

    int probNavHazard = 0;
    int probCombatInt = 0;
    int probResBonus = 0;
    int probAnomaly = 0;

    ts.get_setting_int("event.navigation_hazard", probNavHazard);
    ts.get_setting_int("event.combat_interference", probCombatInt);
    ts.get_setting_int("event.resource_bonus", probResBonus);
    ts.get_setting_int("event.anomaly_discovery", probAnomaly);

    if (probNavHazard == 0 && probCombatInt == 0 &&
        probResBonus == 0 && probAnomaly == 0)
    {
        Logger::instance().info(
            std::format("[HEX_EVENTS] Round {}: all probabilities 0, skipping",
                        round));
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();

    std::vector<std::vector<std::string>> hexRows = db.Query(
        "SELECT DISTINCT at_hex FROM ships "
        "WHERE game_id=? AND at_hex IS NOT NULL AND destroyed_at IS NULL "
        "AND (racked_in IS NULL OR racked_in = '')",
        {game_id});

    if (hexRows.empty())
    {
        Logger::instance().info(
            std::format("[HEX_EVENTS] Round {}: no occupied hexes", round));
        return;
    }

    srand(static_cast<unsigned int>(time(NULL) + game_id + round));

    int nSpawned = 0;
    int nExpires = round + 2;

    struct EventDef
    {
        const char* pszType;
        int nProb;
        int nModifier;
    };

    EventDef events[4] = {
        {"NAVIGATION_HAZARD",   probNavHazard,  +1},
        {"COMBAT_INTERFERENCE", probCombatInt,  -1},
        {"EXTRACTION_BONUS",    probResBonus,   +1},
        {"SALVAGE_OPPORTUNITY", probAnomaly,    +1}
    };

    for (size_t hIdx = 0; hIdx < hexRows.size(); ++hIdx)
    {
        const std::string& hexId = hexRows[hIdx][0];
        if (hexId.empty())
        {
            continue;
        }

        for (int eIdx = 0; eIdx < 4; ++eIdx)
        {
            if (events[eIdx].nProb <= 0)
            {
                continue;
            }

            int roll = rand() % 100;
            if (roll < events[eIdx].nProb)
            {
                db.Exec(
                    "INSERT INTO hex_events "
                    "(game_id, hex_id, event_type, modifier_value, "
                    "spawned_turn, expires_turn) "
                    "VALUES (?, ?, ?, ?, ?, ?)",
                    {game_id, hexId, std::string(events[eIdx].pszType),
                     events[eIdx].nModifier, round, nExpires});
                ++nSpawned;
            }
        }
    }

    Logger::instance().info(
        std::format("[HEX_EVENTS] Round {}: spawned {} events across {} hexes",
                    round, nSpawned, hexRows.size()));
}
