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
        else if (k == "--port")
        {
            std::string t;
            next(t);
            srvconfig.port = std::atoi(t.c_str());
        }
    }
    DatabaseManager::getInstance().configure(&dbconfig);
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
    // BUGBUG
    // Might as well try to test the DB?
}

void load_services()
{
    // Backend Services
    //////////////////////
    create_db();

    // Optional
    // TBD test_db();

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
