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
#include "combat.h"
#include "logger.h"
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
        Logger::instance().info("QUERY: " + q );
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
        Logger::instance().info("QUERY: " + q );
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
