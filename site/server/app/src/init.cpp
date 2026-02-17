///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include <format>
#include <iostream>
#include <fstream>
#include <memory>

#include "app.h"
#include "configr.h"
#include "db.h"
#include "srvmgr.h"
#include "tuning_command.h"
#include "util.h"

static const char* License = 
    "+--------------------------------------------------------------+\n"
    "| Kepler's Horizion is licensed under the BSD 3-Clause License |\n"
    "| Copyright (c) 2025, sibomots                                 |\n"
    "| https://github.com/sibomots/KeplersHorizon                   |\n"
    "+--------------------------------------------------------------+";

static const char* dbpass = "PASS:";
static const char* dbfail = "FAIL:";
static const char* dbskip = "SKIP:";

static void write_pid_file(const std::string& filename = "kh.pid")
{
    pid_t pid = getpid();
    std::ofstream pid_file(filename);
    if (!pid_file.is_open())
    {
        std::cerr << "Error: Unable to open file "
                  << filename
                  << " for writing."
                  << std::endl;
        exit(-1);
    }
    pid_file << pid;
    pid_file.close();
    if (pid_file.fail())
    {
        std::cerr << "Error: Failed to write PID to file "
                  << filename
                  << "."
                  << std::endl;
    }
}

void pre_init(void)
{
   write_pid_file();   
}

void init(void)
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

void license(void)
{
    std::cout << License << std::endl;
}

void advice(void)
{
}

void banner(void)
{
    license();
    advice();
}

void apply_arguments(int argc, char** argv)
{
    if (!Configr::invalidate(argc, argv))
    {
        Logger::instance().error(std::format(
             "Config error: {}", Configr::instance().error()));
    }
    else
    {
        DatabaseManager::instance().init();
        ServerManager::instance().configure();
        Configr::instance().summary();
        TuningCommand::load_conf_file();
    }
}

// clang-format off
void test_db(void)
{

    Logger::instance().debug("Starting database validation...");

    DatabaseManager& db = DatabaseManager::instance();

    Logger::instance().debug("DB Instance set");

    // Test 1: Connection test (already connected by activate_db)
    try
    {
        auto ping = db.Query("SELECT 1", {});
        if (ping.empty())
        {
            Logger::instance()
                .error(std::format("{} Connection test returned no rows", dbfail));
            return;
        }
        Logger::instance().debug(std::format("{} Database connection OK", dbpass));
    }
    catch (const std::exception& ex)
    {
        Logger::instance().error(
            std::format("{} Connection test: {}", dbfail, ex.what()));
        exit(-1);
    }

    // Test 2: Required tables exist
    const char* required_tables[] = {
        "anomaly_events", "base_stars", "codex_entries",
        "combat_orders", "combat_state", "discovered_salvageables",
        "drafts", "extract_operations", "fabrication_queue",
        "facility_control", "facility_control_initial",
        "game_events",
        "game_seats", "games", "help_lookup",
        "help_topics", "hex_events", "hexes",
        "load_requests", "market_base_prices", "market_history",
        "market_prices", "modules",
        "pending_damage",
        "resource_state",
        "rooms", "salvage_operations", "salvageable_drops",
        "salvageables", "saved_games", "saved_ships",
        "sessions", "ships", "sightings",
        "star_systems", "system_anomalies", "system_asteroid_belts",
        "system_codex_rumors", "system_constraints",
        "system_facilities", "system_moons", "system_planets",
        "system_populations", "system_resources", "system_species",
        "system_stars", "telemetry_queue", "users",
        "warpline_hexes", "warplines" };

    bool all_tables_ok = true;
    for (const char* table : required_tables)
    {
        try
        {
            auto check =
                db.Query("SELECT COUNT(*) "
                         "FROM information_schema.tables "
                         "WHERE "
                         "table_schema = DATABASE() "
                         " AND table_name = ?",
                         {std::string(table)});

            if (check.empty() || KH_EQU(check[0][0], "0"))
            {
                Logger::instance().error(
                     std::format("{} Table '{}' does not exist",
                        dbfail, table));
                all_tables_ok = false;
            }
            else 
            {
                Logger::instance().debug(
                     std::format("{} Table '{}' OK",
                        dbpass, table));

            }
        }
        catch (const std::exception& ex)
        {
                Logger::instance().error(
                     std::format("{} Error checking table '{}': {}",
                        dbfail, table, ex.what()));

            all_tables_ok = false;
        }
    }

    if (all_tables_ok)
    {
        Logger::instance().debug(std::format(
           "{} All required tables exist", dbpass));
    }

    // Test 3: Insert/delete test row in 
    // sessions (verify write access)
    try
    {
        // Need a user to reference - check if test user exists
        auto userCheck = db.Query("SELECT id FROM users LIMIT 1", {});
        if (!userCheck.empty())
        {
            int user_id = std::stoi(userCheck[0][0]);
            std::string test_token =
                "__test_token_" 
                + std::to_string(std::time(nullptr));

            // Insert test session
            db.Exec("INSERT INTO "
                    "sessions(token, user_id) VALUES(?,?)",
                    {test_token, user_id});

            // Verify it exists
            auto verify =
                db.Query("SELECT token "
                         "FROM sessions "
                         "WHERE token=?",
                         {test_token});
            if (verify.empty())
            {
                Logger::instance().error(std::format(
                 "{} Test insert verification failed", dbfail));
            }
            else
            {
                Logger::instance().debug(std::format(
                 "{} Write access OK (insert verified)", dbpass));
            }

            // Cleanup: delete test row
            db.Exec("DELETE "
                    "FROM sessions "
                    "WHERE token=?",
                    {test_token});
            Logger::instance().debug(std::format(
               "{} Cleanup OK (test row deleted)", dbpass));
        }
        else
        {
            Logger::instance().debug(std::format(
              "{} Write test skipped (no users in database)", dbskip));
        }
    }
    catch (const std::exception& ex)
    {
        Logger::instance().error(std::format(
            "{} Write test: {}", dbfail, ex.what()));
    }

    // Test 4: Check that at least one module exists
    try
    {
        auto moduleCheck = db.Query("SELECT COUNT(*) FROM modules", {});
        if (!moduleCheck.empty() && moduleCheck[0][0] != "0")
        {
            Logger::instance().debug(std::format(
             "{} {} module(s) defined", dbpass, moduleCheck[0][0]));
        }
        else
        {
            Logger::instance().debug(std::format(
             "{} No modules defined in database", dbfail));
        }
    }
    catch (const std::exception& ex)
    {
        Logger::instance().error(std::format(
          "{} Module check: {}", dbfail, ex.what()));
    }

    Logger::instance().debug("Database validation complete.");
}
// clang-format on

void load_services()
{
    // Backend Services
    //////////////////////
    test_db();

    // This creates the server and descritpr for
    // listening on port for REST-ful  transactions
    ServerManager::instance().connect();

    // Front-end Services
    /////////////////////

    // Now, let's register our services with the Actors
    // in the system:

    // Parser
    // StateMachine
}
