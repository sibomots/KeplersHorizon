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
#include "configr.h"
#include "logger.h"
#include "typedefs.h"
#include "util.h"

// Static variables for the DatabaseManager
// Should be overwritten when the configuration is
// provided by user command line arguments.

MYSQL* DatabaseManager::driver = 0;

std::string DatabaseManager::dbhost = "127.0.0.1";
std::string DatabaseManager::dbuser = "dbusr";
std::string DatabaseManager::dbpass = "dbpass";
std::string DatabaseManager::dbname = "dbname";

void DatabaseManager::driver_check()
{
    static int retry_count = 0;

    try {
        int rc = mysql_ping(driver);
        if (rc)
        {
            if (++retry_count < 3)
            {
                std::cerr << "DatabaseManager: Attempting reconnect #"
                          << std::to_string(retry_count) << std::endl;
                reconnect();
            }
            else
            {
                throw std::runtime_error("\nUnable to recover trying to connect database\n");
            }
        }
        else
        {
            retry_count = 0;
        }
   }
   catch (const std::exception& ex) {
        throw std::runtime_error("\nUnable to recover trying to connect database\n");
   }
}

void DatabaseManager::connect()
{
    driver = mysql_init(NULL);

    try {
       driver_check();
    }
    catch(const std::exception& ex) {
                std::string err = "Unable to recover trying to connect database";
                throw std::runtime_error(err.c_str());
    }

    if (!mysql_real_connect(driver, dbhost.c_str(), dbuser.c_str(),
                            dbpass.c_str(), dbname.c_str(), 0, NULL, 0))
    {
        std::string err = "mysql_real_connect failed";
        throw std::runtime_error(err.c_str());
    }
    mysql_set_character_set(driver, "utf8mb4");
}

void DatabaseManager::reconnect()
{
    try {
       driver = mysql_init(NULL);
    }
    catch(const std::exception& ex) {
        std::string err = "mysql_init failed";
        throw std::runtime_error(err.c_str());

    }

    if (!mysql_real_connect(driver, dbhost.c_str(), dbuser.c_str(),
                            dbpass.c_str(), dbname.c_str(), 0, NULL, 0))
    {
        std::string err = "mysql_real_connect failed";
        throw std::runtime_error(err.c_str());
    }
    mysql_set_character_set(driver, "utf8mb4");
}

void DatabaseManager::exec(const std::string& q)
{
    try {
       driver_check();
    }
    catch(const std::exception& ex) {
                std::string err = "Unable to recover trying to connect database";
                throw std::runtime_error(err.c_str());
    }
    fprintf(stderr, "DB::EXEC: %s\n", q.c_str());
    if (mysql_query(driver, q.c_str()))
    {
        Logger::instance().info("[DB] QUERY Failed: " + q);
        std::string err = "exec failed:";
        err.append(q.c_str());
        throw std::runtime_error(err.c_str());
    }
}

std::vector<std::vector<std::string>>
DatabaseManager::query(const std::string& q)
{
    try {
       driver_check();
    }
    catch(const std::exception& ex) {
                std::string err = "Unable to recover trying to connect database";
                throw std::runtime_error(err.c_str());
    }

    fprintf(stderr, "DB::QUERY: %s\n", q.c_str());
    if (mysql_query(driver, q.c_str()))
    {
        Logger::instance().info("QUERY: " + q);
        std::string err = "query failed";
        err.append(q.c_str());
        throw std::runtime_error(err.c_str());
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
    dump(out);
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

void DatabaseManager::configure()
{
    dbhost = Configr::instance().get<Key::dbhost>();
    dbuser = Configr::instance().get<Key::dbusr>();
    dbpass = Configr::instance().get<Key::dbpass>();
    dbname = Configr::instance().get<Key::dbname>();
}

void DatabaseManager::dump(const std::vector<std::vector<std::string> >& rows )
{

   if (rows.empty()) {
      return;
   }

   fprintf(stderr, "\nDUMP:\n"); 
   for(std::vector<std::vector<std::string> >::const_iterator itr = rows.begin();
           itr != rows.end(); 
           ++itr)
   {
       bool seencol = false;
       const std::vector<std::string> row = (*itr);
       for(std::vector<std::string>::const_iterator inner = row.begin();
           inner != row.end();
           ++inner)
       {
           if (seencol) {
             fprintf(stderr, "\t");
           }
           else {
              seencol = true;
           } 
           fprintf(stderr, "%s", (*inner).c_str());
       }
       if (seencol) {
          fprintf(stderr, "\n");
       }
   }
   fprintf(stderr, "\n"); 

}
