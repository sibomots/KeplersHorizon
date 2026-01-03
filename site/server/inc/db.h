//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __DB_H__
#define __DB_H__

#include "app.h"
#include "db.h"
#include "typedefs.h"

class DatabaseManager
{
  public:
    static DatabaseManager& getInstance()
    {
        // The static instance is created upon the first call to this function.
        static DatabaseManager instance;
        return instance;
    }

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) noexcept = delete;
    DatabaseManager& operator=(DatabaseManager&&) noexcept = delete;

    static std::string dbhost;
    static std::string dbuser;
    static std::string dbpass;
    static std::string dbname;

    // Setup
    void configure(void* param);
    void load(void* param);
    void connect();
    bool driver_invalid();

    // Query tools
    void exec(const std::string& q);
    std::vector<std::vector<std::string>> query(const std::string& q);
    std::string esc(const std::string& s);

  private:
    static MYSQL* driver;

    DatabaseManager()
    {
    }

    ~DatabaseManager()
    {
        if (driver)
        {
            mysql_close(driver);
        }
    }
};

#endif
