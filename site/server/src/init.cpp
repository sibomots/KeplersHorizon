//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>

#include "app.h"
#include "db.h"
#include "srvmgr.h"
#include "util.h"

void init(void)
{
    // entropy
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

void license(void)
{
    std::cout
        << std::endl
        << std::endl
        << "Kepler's Horizion is licensed under the BSD 3-Clause License\n"
           "Copyright (c) 2025, sibomots\n\n"
           "https://github.com/sibomots/KeplersHorizon\n"
        << std::endl
        << std::endl;
}

// BUGBUG
void advice(void)
{
    std::cout << std::endl
              << std::endl
              << "Have fun!" << std::endl
              << std::endl;
}

// BUGBUG
void banner(void)
{
    license();
    advice();
}

void apply_arguments(int argc, char** argv)
{
    DBConfig dbconfig;
    ServerConfig srvconfig;
    DataConfig  dataconfig;

    for (int i = 1; i < argc; i++)
    {
        std::string k = argv[i];
        auto next = [&](std::string& out) {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("Missing arg for " + k);
            }
            out = argv[++i];
        };

        auto next_flag = [&](bool& out) {
            if ( i + 1 >= argc)
            {
                throw std::runtime_error("Missing arg for " + k);
            }
            // the fact the flag was used, it's true.
            // no need to look at i+1 argv.  the flag is just a flag. No arguments to it.
            out = true;
        };

        if (k == "--dbhost")
        {
            next(dbconfig.dbhost);
        }
        else if (k == "--dbuser")
        {
            next(dbconfig.dbuser);
        }
        else if (k == "--dbpass")
        {
            next(dbconfig.dbpass);
        }
        else if (k == "--dbname")
        {
            next(dbconfig.dbname);
        }
        else if (k == "--clean") 
        {
            next_flag(dataconfig.clean);
        }
        else if (k == "--schema")
        {
            next_flag(dataconfig.schema);
        }
        else if (k == "--seed")
        {
            next_flag(dataconfig.seed);
        }
        else if (k == "--port")
        {
            std::string t;
            next(t);
            srvconfig.port = std::atoi(t.c_str());
        }
    }
    DatabaseManager::getInstance().configure(&dbconfig);
    DatabaseManager::getInstance().load(&dataconfig);
    ServerManager::getInstance().configure(&srvconfig);
}

// Static Functions

void create_db()
{
    try
    {
        DatabaseManager::getInstance().connect();
    }
    catch (const std::exception& ex)
    {
        // BUGBUG
        std::fprintf(stderr, "fatal: %s\n", ex.what());
        return;
    }
}

void test_db(void)
{
    std::cout << "[test_db] Starting database validation..." << std::endl;
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // Test 1: Connection test (already connected by create_db)
    try
    {
        auto ping = db.query("SELECT 1");
        if (ping.empty())
        {
            std::cerr << "[test_db] FAIL: Connection test returned no rows" << std::endl;
            return;
        }
        std::cout << "[test_db] PASS: Database connection OK" << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[test_db] FAIL: Connection test: " << ex.what() << std::endl;
        return;
    }
    
    // Test 2: Required tables exist
    const char* required_tables[] = {
        "users",
        "sessions",
        "rooms",
        "games",
        "game_seats",
        "game_events",
        "drafts",
        "ships",
        "star_systems",
        "warplines",
        "hexes",
        "combat_state",
        "combat_orders",
        "telemetry_queue"
    };
    
    bool all_tables_ok = true;
    for (const char* table : required_tables)
    {
        try
        {
            auto check = db.query(
                "SELECT COUNT(*) FROM information_schema.tables "
                "WHERE table_schema = DATABASE() AND table_name = '" +
                std::string(table) + "'");
            
            if (check.empty() || check[0][0] == "0")
            {
                std::cerr << "[test_db] FAIL: Table '" << table << "' does not exist" << std::endl;
                all_tables_ok = false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[test_db] FAIL: Error checking table '" << table << "': " << ex.what() << std::endl;
            all_tables_ok = false;
        }
    }
    
    if (all_tables_ok)
    {
        std::cout << "[test_db] PASS: All " << (sizeof(required_tables)/sizeof(required_tables[0])) 
                  << " required tables exist" << std::endl;
    }
    
    // Test 3: Insert/delete test row in sessions (verify write access)
    try
    {
        // Need a user to reference - check if test user exists
        auto userCheck = db.query("SELECT id FROM users LIMIT 1");
        if (!userCheck.empty())
        {
            int user_id = std::stoi(userCheck[0][0]);
            std::string test_token = "__test_token_" + std::to_string(std::time(nullptr));
            
            // Insert test session
            db.exec("INSERT INTO sessions(token, user_id) VALUES('" + 
                    db.esc(test_token) + "', " + std::to_string(user_id) + ")");
            
            // Verify it exists
            auto verify = db.query("SELECT token FROM sessions WHERE token='" + 
                                   db.esc(test_token) + "'");
            if (verify.empty())
            {
                std::cerr << "[test_db] FAIL: Test insert verification failed" << std::endl;
            }
            else
            {
                std::cout << "[test_db] PASS: Write access OK (insert verified)" << std::endl;
            }
            
            // Cleanup: delete test row
            db.exec("DELETE FROM sessions WHERE token='" + db.esc(test_token) + "'");
            std::cout << "[test_db] PASS: Cleanup OK (test row deleted)" << std::endl;
        }
        else
        {
            std::cout << "[test_db] SKIP: Write test skipped (no users in database)" << std::endl;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[test_db] FAIL: Write test: " << ex.what() << std::endl;
    }
    
    std::cout << "[test_db] Database validation complete." << std::endl;
}

void load_services()
{
    // Backend Services
    //////////////////////
    create_db();

    // Optional: validate database schema
    test_db();

    // This creates the server and descritpr for listening on port for REST-ful
    // transactions
    ServerManager::getInstance().connect();

    // Front-end Services
    /////////////////////

    // Now, let's register our services with the Actors
    // in the system:

    // Parser
    // StateMachine
}
