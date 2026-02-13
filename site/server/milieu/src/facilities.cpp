///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "facilities.h"

#include <cstdlib>
#include <set>

#include "db.h"
#include "logger.h"
#include "moduleutil.h"

const int FacilityEngine::TURNS_TO_CAPTURE;

std::vector<FacilityInfo>
FacilityEngine::get_facilities(int game_id, const std::string& system)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::vector<FacilityInfo> result;

    auto rows = db.Query(
        "SELECT system_name, facility_type, controller, occupied_since, "
        "capture_progress FROM facility_control WHERE game_id=? "
        "AND system_name=?",
        {game_id, system});

    for (const auto& row : rows)
    {
        FacilityInfo f;
        f.system_name = row[0];
        f.facility_type = row[1];
        f.controller = row[2].empty() ? '\0' : row[2][0];
        f.occupied_since = row[3].empty() ? 0 : std::atoi(row[3].c_str());
        f.capture_progress = std::atoi(row[4].c_str());
        result.push_back(f);
    }

    return result;
}

bool FacilityEngine::player_controls(int game_id, const std::string& system,
                                     const std::string& facility_type,
                                     char player)
{
    DatabaseManager& db = DatabaseManager::instance();

    auto rows =
        db.Query("SELECT 1 FROM facility_control WHERE game_id=? "
                 "AND system_name=? AND facility_type=? AND controller=?",
                 {game_id, system, facility_type, player});

    return !rows.empty();
}

void FacilityEngine::initialize_facilities(int game_id)
{
    DatabaseManager& db = DatabaseManager::instance();

    Logger::instance().info("[FACILITIES] Initializing facilities for game " +
                            std::to_string(game_id));

    // Copy from reference table (static data loaded via SQL LOAD)
    // to per-game runtime table
    db.Exec("INSERT IGNORE INTO "
            "facility_control(game_id,system_name,facility_type,controller) "
            "SELECT ?, system_name, facility_type, controller "
            "FROM facility_control_initial",
            {game_id});
}

void FacilityEngine::update_control(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Get all systems with facilities
    auto facilities =
        db.Query("SELECT DISTINCT system_name, facility_type, controller, "
                 "capture_progress FROM facility_control WHERE game_id=?",
                 {game_id});

    for (const auto& fac : facilities)
    {
        std::string system = fac[0];
        std::string fac_type = fac[1];
        char current_controller = fac[2].empty() ? '\0' : fac[2][0];
        int progress = std::atoi(fac[3].c_str());

        // Get hex for this system
        int mod = get_module_id_for_game(game_id);
        auto hex_row = db.Query(
            "SELECT hex_id FROM star_systems WHERE module_id=? AND name=?",
            {mod, system});
        if (hex_row.empty())
            continue;
        std::string hex = hex_row[0][0];

        // Check which players have ships in this system
        auto ship_owners = db.Query(
            "SELECT DISTINCT owner FROM ships WHERE game_id=? "
            "AND at_hex=? AND destroyed_at IS NULL AND racked_in IS NULL",
            {game_id, hex});

        std::set<char> present;
        for (const auto& row : ship_owners)
        {
            present.insert(row[0][0]);
        }

        bool a_present = present.count('A') > 0;
        bool b_present = present.count('B') > 0;

        // Update control based on presence
        if (a_present && !b_present)
        {
            // Only A present
            if (current_controller != 'A')
            {
                if (KH_EQU(current_controller, '\0'))
                {
                    // Neutral - start capture
                    progress++;
                    if (progress >= TURNS_TO_CAPTURE)
                    {
                        db.Exec("UPDATE facility_control SET controller='A', "
                                "occupied_since=?, capture_progress=0 "
                                "WHERE game_id=? AND system_name=? "
                                "AND facility_type=?",
                                {round, game_id, system, fac_type});
                        Logger::instance().info(
                            "[FACILITIES] Player A captured " + fac_type +
                            " at " + system);
                    }
                    else
                    {
                        db.Exec(
                            "UPDATE facility_control SET capture_progress=? "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {progress, game_id, system, fac_type});
                    }
                }
                else
                {
                    // Enemy controlled - contested, progress toward capture
                    progress++;
                    if (progress >= TURNS_TO_CAPTURE)
                    {
                        db.Exec("UPDATE facility_control SET controller='A', "
                                "occupied_since=?, capture_progress=0 "
                                "WHERE game_id=? AND system_name=? "
                                "AND facility_type=?",
                                {round, game_id, system, fac_type});
                        Logger::instance().info(
                            "[FACILITIES] Player A captured " + fac_type +
                            " at " + system + " from enemy");
                    }
                    else
                    {
                        db.Exec(
                            "UPDATE facility_control SET capture_progress=? "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {progress, game_id, system, fac_type});
                    }
                }
            }
            else
            {
                // Already controlled by A - reset progress
                if (progress > 0)
                {
                    db.Exec("UPDATE facility_control SET capture_progress=0 "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {game_id, system, fac_type});
                }
            }
        }
        else if (b_present && !a_present)
        {
            // Only B present - mirror logic
            if (current_controller != 'B')
            {
                progress++;
                if (progress >= TURNS_TO_CAPTURE)
                {
                    db.Exec("UPDATE facility_control SET controller='B', "
                            "occupied_since=?, capture_progress=0 "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {round, game_id, system, fac_type});
                    Logger::instance().info("[FACILITIES] Player B captured " +
                                            fac_type + " at " + system);
                }
                else
                {
                    db.Exec("UPDATE facility_control SET capture_progress=? "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {progress, game_id, system, fac_type});
                }
            }
            else
            {
                if (progress > 0)
                {
                    db.Exec("UPDATE facility_control SET capture_progress=0 "
                            "WHERE game_id=? AND system_name=? "
                            "AND facility_type=?",
                            {game_id, system, fac_type});
                }
            }
        }
        else if (a_present && b_present)
        {
            // Both present - contested, reset progress
            if (progress > 0)
            {
                db.Exec("UPDATE facility_control SET capture_progress=0 "
                        "WHERE game_id=? AND system_name=? "
                        "AND facility_type=?",
                        {game_id, system, fac_type});
            }
        }
        // Neither present - no change
    }
}

int FacilityEngine::calculate_trade_hub_income(int game_id, char player)
{
    DatabaseManager& db = DatabaseManager::instance();

    auto rows =
        db.Query("SELECT COUNT(*) FROM facility_control WHERE game_id=? "
                 "AND facility_type='TRADE_HUB' AND controller=?",
                 {game_id, player});

    int hubs = rows.empty() ? 0 : std::atoi(rows[0][0].c_str());

    // Each trade hub provides 5 CR per turn
    return hubs * 5;
}

bool FacilityEngine::can_repair_at(int game_id, const std::string& system,
                                   char player)
{
    // Check for controlled REPAIR_DOCK or SHIPYARD
    return player_controls(game_id, system, "REPAIR_DOCK", player) ||
           player_controls(game_id, system, "SHIPYARD", player);
}
