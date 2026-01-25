//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __DB_H__
#define __DB_H__

#include <mysql/mysql.h>
#include "app.h"
#include "typedefs.h"

class DatabaseManager
{

  public:
    static const size_t SQLSZ = 1024;

  public:
    static DatabaseManager& instance()
    {
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
    void configure();
    void connect();

    // Query tools
    void exec(const std::string& q);
    std::vector<std::vector<std::string>> query(const std::string& q);
    std::string esc(const std::string& s);
    void dump(const std::vector<std::vector<std::string> >& rows);

  private:
    void reconnect();
    void driver_check();
    static MYSQL* driver;

    DatabaseManager()
    {
    }

    ~DatabaseManager()
    {
        if (driver)
        {
            fprintf(stderr, "Closing the driver to MySQL\n");
            mysql_close(driver);
        }
    }
};

#endif
