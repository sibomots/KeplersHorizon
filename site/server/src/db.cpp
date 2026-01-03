//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "db.h"

#include <iostream>

#include "app.h"
#include "logger.h"
#include "combat.h"
#include "load.h"
#include "typedefs.h"
#include "util.h"
// Static variables for the DatabaseManager Singleton

MYSQL* DatabaseManager::driver = 0;
std::string DatabaseManager::dbhost = "127.0.0.1";
std::string DatabaseManager::dbuser = "dbusr";
std::string DatabaseManager::dbpass = "dbpass";
std::string DatabaseManager::dbname = "dbname";

bool DatabaseManager::driver_invalid()
{
    if (!driver)
    {
        throw std::runtime_error("mysql_init failed");
    }
    return false;
}

void DatabaseManager::load(void* param)
{
    DataConfig* pconf = static_cast<DataConfig*>(param);
    if (!pconf) 
    {
        Logger::instance().info("[DB] Using database as-is. No initialization");
        return;
    }

    // Part 1: If --clean flag is set, drop all schema tables
    if (pconf->clean) {
        Logger::instance().info("[DB] CLEAN: Dropping schema tables...");
        int dropped = 0;
        for (const auto& stmt : DROP_STATEMENTS) {
            if (mysql_query(driver, stmt.c_str())) {
                std::ostringstream warning_msg;
                warning_msg << "[DB] Warning: "
                            << mysql_error(driver);
                Logger::instance().error(warning_msg.str());
            } else {
                dropped++;
            }
        }
        std::ostringstream notice;
        notice << "[DB] CLEAN: Dropped "
               << (int) dropped
               << " tables";
        Logger::instance().info(notice.str());
    }

    // Part 2: If --schema flag is set, create all schema tables
    if (pconf->schema) {
        std::ostringstream notice;
        notice <<  "[DB] SCHEMA: Creating tables...";
        Logger::instance().info(notice.str());

        int created = 0;
        for (const auto& stmt : SCHEMA_STATEMENTS) {
            if (mysql_query(driver, stmt.c_str())) {
                std::ostringstream err;
                err << "[DB] SQL Error: "
                    << mysql_error(driver);
                Logger::instance().error(err.str()); 
                std::ostringstream badline;
                badline << "[DB] Statement: "
                        << stmt.substr(0, 80)
                        << "...";
                Logger::instance().error(badline.str());
            } else {
                created++;
            }
        }
        std::ostringstream msg;
        msg << "[DB] SCHEMA: Executed "
            << (int)created 
            << " statements";
        Logger::instance().info(msg.str());
    }

    // Part 3: If --seed flag is set, insert seed data
    if (pconf->seed) {
        std::ostringstream msg;
        msg << "[DB] SEED: Inserting seed data...";
        Logger::instance().info(msg.str());
        int inserted = 0;
        
        // Star systems, warplines
        for (const auto& stmt : SEED_STATEMENTS) {
            if (mysql_query(driver, stmt.c_str())) {
                std::ostringstream err;
                err << "[DB] SQL Error: "
                    << mysql_error(driver);
                Logger::instance().error(err.str());
            } else {
                inserted++;
            }
        }
        
        // Hexes
        for (const auto& stmt : HEXES_SEED) {
            if (mysql_query(driver, stmt.c_str())) {
                std::ostringstream err;
                err << "[DB] SQL Error: "
                    << mysql_error(driver);
                Logger::instance().error(err.str());
            } else {
                inserted++;
            }
        }
        
        // Warpline hexes
        for (const auto& stmt : WARPLINE_HEXES_SEED) {
            if (mysql_query(driver, stmt.c_str())) {
                std::ostringstream err;
                err << "[DB] SQL Error: "
                    << mysql_error(driver);
                Logger::instance().error(err.str());
            } else {
                inserted++;
            }
        }
        
        std::ostringstream summary_msg;
        summary_msg << "[DB] SEED: Executed "
            << (int) inserted
            << " insert statements";
        Logger::instance().info(summary_msg.str());
    }
}

void DatabaseManager::connect()
{
    driver = mysql_init(NULL);
    if (driver_invalid())
    {
        return;
    }
    if (!mysql_real_connect(driver, dbhost.c_str(), dbuser.c_str(),
                            dbpass.c_str(), dbname.c_str(), 0, NULL, 0))
    {
        throw std::runtime_error(std::string("mysql_real_connect failed: ") +
                                 mysql_error(driver));
    }
    mysql_set_character_set(driver, "utf8mb4");
}

void DatabaseManager::exec(const std::string& q)
{
    if (driver_invalid())
    {
        return;
    }

    if (mysql_query(driver, q.c_str()))
    {
        throw std::runtime_error(std::string("mysql_query: ") +
                                 mysql_error(driver));
    }
}

std::vector<std::vector<std::string>>
DatabaseManager::query(const std::string& q)
{
    if (driver_invalid())
    {
        return {};
    }
    if (mysql_query(driver, q.c_str()))
    {
        throw std::runtime_error(std::string("mysql_query: ") +
                                 mysql_error(driver));
    }
    MYSQL_RES* res = mysql_store_result(driver);
    if (!res)
    {
        return {};
    }

    int cols = mysql_num_fields(res);
    std::vector<std::vector<std::string>> out;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        unsigned long* lens = mysql_fetch_lengths(res);
        std::vector<std::string> r;
        for (int i = 0; i < cols; i++)
        {
            if (!row[i])
            {
                r.push_back("");
            }
            else
            {
                r.push_back(std::string(row[i], row[i] + lens[i]));
            }
        }
        out.push_back(r);
    }
    mysql_free_result(res);
    return out;
}

std::string DatabaseManager::esc(const std::string& s)
{
    std::string out;
    out.resize(s.size() * 2 + 1);
    unsigned long n =
        mysql_real_escape_string(driver, &out[0], s.c_str(), s.size());
    out.resize(n);
    return out;
}

void DatabaseManager::configure(void* param)
{
    if (param != NULL)
    {
        DBConfig* pconf = static_cast<DBConfig*>(param);
        dbhost = std::string(pconf->dbhost);
        dbuser = std::string(pconf->dbuser);
        dbpass = std::string(pconf->dbpass);
        dbname = std::string(pconf->dbname);
    }
}
