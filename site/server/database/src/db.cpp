//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>

#include "app.h"
#include "ce.h"
#include "configr.h"
#include "db.h"
#include "logger.h"
#include "typedefs.h"
#include "util.h"

// Static variables for the DatabaseManager
// Should be overwritten when the configuration is
// provided by user command line arguments.

DatabaseManager::MySQLWrapper* DatabaseManager::driver = 0;
bool DatabaseManager::m_ready = false;

std::string DatabaseManager::dbhost = "127.0.0.1";
std::string DatabaseManager::dbuser = "dbusr";
std::string DatabaseManager::dbpass = "dbpass";
std::string DatabaseManager::dbname = "dbname";

bool DatabaseManager::Exec(const std::string& query_template,
                           const std::vector<SqlArg>& args)
{
    bool bres = false;
    if (m_ready)
    {
        bres = driver->execute(query_template, args);
    }
    return bres;
}

std::vector<std::vector<std::string>>
DatabaseManager::Query(const std::string& query_template,
                       const std::vector<SqlArg>& args)
{
    if (m_ready)
    {
        return driver->query(query_template, args);
    }
    else
    {
        return {};
    }
}

bool DatabaseManager::Exec(const std::string& query_template)
{
    bool bres = false;
    if (m_ready)
    {
        bres = driver->execute(query_template);
    }
    return bres;
}

std::vector<std::vector<std::string>>
DatabaseManager::Query(const std::string& query_template)
{
    if (m_ready)
    {
        return driver->query(query_template);
    }
    else
    {
        return {};
    }
}

void DatabaseManager::init()
{
    m_ready = false;
    SafeDelete(driver);

    dbhost = Configr::instance().get<Key::dbhost>();
    dbuser = Configr::instance().get<Key::dbusr>();
    dbpass = Configr::instance().get<Key::dbpass>();
    dbname = Configr::instance().get<Key::dbname>();

    driver = new MySQLWrapper(dbhost.c_str(), dbuser.c_str(), dbpass.c_str(),
                              dbname.c_str());
    m_ready = true;
}
