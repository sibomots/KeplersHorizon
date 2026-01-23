//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>

#include "configr.h"
#include "app.h"
#include "db.h"
#include "srvmgr.h"
#include "util.h"

static const char* License =
   "+--------------------------------------------------------------+\n"
   "| Kepler's Horizion is licensed under the BSD 3-Clause License |\n"
   "| Copyright (c) 2025, sibomots                                 |\n"
   "| https://github.com/sibomots/KeplersHorizon                   |\n"
   "+--------------------------------------------------------------+";

static const char* dbpass = "[TEST] PASS:";
static const char* dbfail = "[TEST] FAIL:";
static const char* dbskip = "[TEST] SKIP:";

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
    if (!Configr::invalidate(argc, argv)) {
          std::cerr << "Config error: " 
                    << "Configr::instance().error() "
                    << std::endl;
    }
    else
    {
       Configr::instance().summary();
           
       DatabaseManager::getInstance().configure();
       ServerManager::getInstance().configure();
    }
}

// Static Functions

void activate_db()
{
    try
    {
        DatabaseManager::getInstance().connect();
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "fatal: %s\n", ex.what());
        return;
    }
}

// clang-format off
void test_db(void)
{

    std::cout << "[TEST] Starting database validation..."
              << std::endl;

    DatabaseManager& db = DatabaseManager::getInstance();

    std::cout << "DB Instance set" << std::endl;

    // Test 1: Connection test (already connected by activate_db)
    try
    {
        auto ping = db.query("SELECT 1");
        if (ping.empty())
        {
            std::cerr << dbfail
                      << "Connection test returned no rows"
                      << std::endl;
            return;
        }
        std::cout << dbpass
                  << "Database connection OK" << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << dbfail
                  << "Connection test: "
                  << ex.what()
                  << std::endl;
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
                db.query("SELECT COUNT(*) "
                         "FROM information_schema.tables "
                         "WHERE "
                         "table_schema = DATABASE() "
                         " AND table_name = '" +
                         std::string(table) + "'");

            if (check.empty() || check[0][0] == "0")
            {
                std::cerr << dbfail
                          << "Table '" << table
                          << "' does not exist" << std::endl;
                all_tables_ok = false;
            }
            else 
            {
                std::cerr << dbpass
                          << "Table '" << table
                          << "' OK" << std::endl;

            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << dbfail
                      << "Error checking table '"
                      << table
                      << "': " << ex.what() << std::endl;
            all_tables_ok = false;
        }
    }

    if (all_tables_ok)
    {
        std::cout << dbpass
                  << "All "
                  << " required tables exist" << std::endl;
    }

    // Test 3: Insert/delete test row in 
    // sessions (verify write access)
    try
    {
        // Need a user to reference - check if test user exists
        auto userCheck = db.query("SELECT id FROM users LIMIT 1");
        if (!userCheck.empty())
        {
            int user_id = std::stoi(userCheck[0][0]);
            std::string test_token =
                "__test_token_" 
                + std::to_string(std::time(nullptr));

            // Insert test session
            db.exec("INSERT INTO "
                    "sessions(token, user_id) VALUES('" +
                    db.esc(test_token)
                    + "', "
                    + std::to_string(user_id) + ")");

            // Verify it exists
            auto verify =
                db.query("SELECT token "
                         "FROM sessions "
                         "WHERE token='"
                         + db.esc(test_token) + "'");
            if (verify.empty())
            {
                std::cerr << dbfail
                          << "Test insert verification failed"
                          << std::endl;
            }
            else
            {
                std::cout << dbpass
                          << "Write access OK (insert verified)"
                          << std::endl;
            }

            // Cleanup: delete test row
            db.exec("DELETE "
                    "FROM sessions "
                    "WHERE token='" + db.esc(test_token)
                    + "'");
            std::cout << dbpass
                      << "Cleanup OK (test row deleted)"
                      << std::endl;
        }
        else
        {
            std::cout
                << dbskip
                << "Write test skipped (no users in database)"
                << std::endl;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << dbfail
                  << "Write test: "
                  << ex.what() << std::endl;
    }

    // Test 4: Check that at least one module exists
    try
    {
        auto moduleCheck = db.query("SELECT COUNT(*) FROM modules");
        if (!moduleCheck.empty() && moduleCheck[0][0] != "0")
        {
            std::cout << dbpass
                      << moduleCheck[0][0]
                      << " module(s) defined" << std::endl;
        }
        else
        {
            std::cerr << dbfail
                      << "No modules defined in database"
                      << std::endl;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << dbfail
                  << "Module check: "
                  << ex.what() << std::endl;
    }

    std::cout << "[TEST] Database validation complete."
              << std::endl;
}
// clang-format on

void load_services()
{
    // Backend Services
    //////////////////////
    try {
       activate_db();
    }
    catch(const std::exception& ex)
    {
        std::cerr << dbfail
                  << "activation of database failed: "
                  << ex.what() << std::endl;
        return;
    }
    test_db();

    // This creates the server and descritpr for
    // listening on port for REST-ful  transactions
    ServerManager::getInstance().connect();

    // Front-end Services
    /////////////////////

    // Now, let's register our services with the Actors
    // in the system:

    // Parser
    // StateMachine
}
