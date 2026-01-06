//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "facilities.h"

#include <cstdlib>
#include <set>

#include "db.h"
#include "logger.h"

const int FacilityEngine::TURNS_TO_CAPTURE;

std::vector<FacilityInfo> FacilityEngine::get_facilities(
    int game_id, const std::string& system)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<FacilityInfo> result;

    auto rows = db.query(
        "SELECT system_name, facility_type, controller, occupied_since, "
        "capture_progress FROM facility_control WHERE game_id=" +
        std::to_string(game_id) + " AND system_name='" + db.esc(system) + "'");

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
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows = db.query(
        "SELECT 1 FROM facility_control WHERE game_id=" +
        std::to_string(game_id) + " AND system_name='" + db.esc(system) +
        "' AND facility_type='" + db.esc(facility_type) + "' AND controller='" +
        std::string(1, player) + "'");

    return !rows.empty();
}

void FacilityEngine::initialize_facilities(int game_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    Logger::instance().info("[FACILITIES] Initializing facilities for game " +
                            std::to_string(game_id));

    // Copy global facilities to game-specific control table
    // Western Alliance base facilities (ARVEN, BELIX, CAYRU) start controlled by A
    db.exec(
        "INSERT IGNORE INTO facility_control(game_id,system_name,facility_type,"
        "controller) VALUES "
        "(" +
        std::to_string(game_id) + ",'ARVEN','SHIPYARD','A')," + "(" +
        std::to_string(game_id) + ",'ARVEN','REPAIR_DOCK','A')," + "(" +
        std::to_string(game_id) + ",'ARVEN','REFINERY','A')," + "(" +
        std::to_string(game_id) + ",'BELIX','SHIPYARD','A')," + "(" +
        std::to_string(game_id) + ",'BELIX','REPAIR_DOCK','A')," + "(" +
        std::to_string(game_id) + ",'CAYRU','SHIPYARD','A')," + "(" +
        std::to_string(game_id) + ",'CAYRU','TRADE_HUB','A')");

    // Eastern Powers base facilities (ZAREK, ASTREX, BRION) controlled by B
    db.exec(
        "INSERT IGNORE INTO facility_control(game_id,system_name,facility_type,"
        "controller) VALUES "
        "(" +
        std::to_string(game_id) + ",'ZAREK','SHIPYARD','B')," + "(" +
        std::to_string(game_id) + ",'ZAREK','REPAIR_DOCK','B')," + "(" +
        std::to_string(game_id) + ",'ZAREK','RESEARCH_LAB','B')," + "(" +
        std::to_string(game_id) + ",'ASTREX','SHIPYARD','B')," + "(" +
        std::to_string(game_id) + ",'ASTREX','TRADE_HUB','B')," + "(" +
        std::to_string(game_id) + ",'ASTREX','FORTRESS','B')," + "(" +
        std::to_string(game_id) + ",'BRION','SHIPYARD','B')");

    // Neutral facilities in contested reach
    db.exec(
        "INSERT IGNORE INTO facility_control(game_id,system_name,facility_type,"
        "controller) VALUES "
        "(" +
        std::to_string(game_id) + ",'KORAL','TRADE_HUB',NULL)," + "(" +
        std::to_string(game_id) + ",'XYLEN','TRADE_HUB',NULL)," + "(" +
        std::to_string(game_id) + ",'WEXAR','TRADE_HUB',NULL)," + "(" +
        std::to_string(game_id) + ",'VARYN','REPAIR_DOCK',NULL)," + "(" +
        std::to_string(game_id) + ",'VARYN','RESEARCH_LAB',NULL)");

    Logger::instance().info("[FACILITIES] Facilities initialized");
}

void FacilityEngine::update_control(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get all systems with facilities
    auto facilities = db.query(
        "SELECT DISTINCT system_name, facility_type, controller, "
        "capture_progress FROM facility_control WHERE game_id=" +
        std::to_string(game_id));

    for (const auto& fac : facilities)
    {
        std::string system = fac[0];
        std::string fac_type = fac[1];
        char current_controller = fac[2].empty() ? '\0' : fac[2][0];
        int progress = std::atoi(fac[3].c_str());

        // Get hex for this system
        auto hex_row =
            db.query("SELECT hex_id FROM star_systems WHERE map_id=1 AND "
                     "name='" +
                     db.esc(system) + "'");
        if (hex_row.empty())
            continue;
        std::string hex = hex_row[0][0];

        // Check which players have ships in this system
        auto ship_owners = db.query(
            "SELECT DISTINCT owner FROM ships WHERE game_id=" +
            std::to_string(game_id) + " AND at_hex='" + db.esc(hex) +
            "' AND destroyed_at IS NULL AND racked_in IS NULL");

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
                if (current_controller == '\0')
                {
                    // Neutral - start capture
                    progress++;
                    if (progress >= TURNS_TO_CAPTURE)
                    {
                        db.exec("UPDATE facility_control SET controller='A', "
                                "occupied_since=" +
                                std::to_string(round) +
                                ", "
                                "capture_progress=0 WHERE game_id=" +
                                std::to_string(game_id) + " AND system_name='" +
                                db.esc(system) + "' AND facility_type='" +
                                db.esc(fac_type) + "'");
                        Logger::instance().info(
                            "[FACILITIES] Player A captured " + fac_type +
                            " at " + system);
                    }
                    else
                    {
                        db.exec("UPDATE facility_control SET capture_progress=" +
                                std::to_string(progress) + " WHERE game_id=" +
                                std::to_string(game_id) + " AND system_name='" +
                                db.esc(system) + "' AND facility_type='" +
                                db.esc(fac_type) + "'");
                    }
                }
                else
                {
                    // Enemy controlled - contested, progress toward capture
                    progress++;
                    if (progress >= TURNS_TO_CAPTURE)
                    {
                        db.exec("UPDATE facility_control SET controller='A', "
                                "occupied_since=" +
                                std::to_string(round) +
                                ", "
                                "capture_progress=0 WHERE game_id=" +
                                std::to_string(game_id) + " AND system_name='" +
                                db.esc(system) + "' AND facility_type='" +
                                db.esc(fac_type) + "'");
                        Logger::instance().info(
                            "[FACILITIES] Player A captured " + fac_type +
                            " at " + system + " from enemy");
                    }
                    else
                    {
                        db.exec("UPDATE facility_control SET capture_progress=" +
                                std::to_string(progress) + " WHERE game_id=" +
                                std::to_string(game_id) + " AND system_name='" +
                                db.esc(system) + "' AND facility_type='" +
                                db.esc(fac_type) + "'");
                    }
                }
            }
            else
            {
                // Already controlled by A - reset progress
                if (progress > 0)
                {
                    db.exec("UPDATE facility_control SET capture_progress=0 "
                            "WHERE game_id=" +
                            std::to_string(game_id) + " AND system_name='" +
                            db.esc(system) + "' AND facility_type='" +
                            db.esc(fac_type) + "'");
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
                    db.exec("UPDATE facility_control SET controller='B', "
                            "occupied_since=" +
                            std::to_string(round) +
                            ", "
                            "capture_progress=0 WHERE game_id=" +
                            std::to_string(game_id) + " AND system_name='" +
                            db.esc(system) + "' AND facility_type='" +
                            db.esc(fac_type) + "'");
                    Logger::instance().info("[FACILITIES] Player B captured " +
                                            fac_type + " at " + system);
                }
                else
                {
                    db.exec("UPDATE facility_control SET capture_progress=" +
                            std::to_string(progress) + " WHERE game_id=" +
                            std::to_string(game_id) + " AND system_name='" +
                            db.esc(system) + "' AND facility_type='" +
                            db.esc(fac_type) + "'");
                }
            }
            else
            {
                if (progress > 0)
                {
                    db.exec("UPDATE facility_control SET capture_progress=0 "
                            "WHERE game_id=" +
                            std::to_string(game_id) + " AND system_name='" +
                            db.esc(system) + "' AND facility_type='" +
                            db.esc(fac_type) + "'");
                }
            }
        }
        else if (a_present && b_present)
        {
            // Both present - contested, reset progress
            if (progress > 0)
            {
                db.exec("UPDATE facility_control SET capture_progress=0 "
                        "WHERE game_id=" +
                        std::to_string(game_id) + " AND system_name='" +
                        db.esc(system) + "' AND facility_type='" +
                        db.esc(fac_type) + "'");
            }
        }
        // Neither present - no change
    }
}

int FacilityEngine::calculate_trade_hub_income(int game_id, char player)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows = db.query(
        "SELECT COUNT(*) FROM facility_control WHERE game_id=" +
        std::to_string(game_id) +
        " AND facility_type='TRADE_HUB' AND controller='" +
        std::string(1, player) + "'");

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
