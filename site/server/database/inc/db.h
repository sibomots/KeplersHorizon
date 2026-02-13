///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_DB_H__
#define __KH_DB_H__

#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <mysql/mysql.h>
#include <string>
#include <variant>
#include <vector>

#include "app.h"
#include "logger.h"
#include "typedefs.h"

using SqlArg = std::variant<char, int, long long, double, std::string, bool>;

struct MySqlThreadGuard
{
    MySqlThreadGuard()
    {
        // Initializes thread-specific variables
        mysql_thread_init();
    }
    ~MySqlThreadGuard()
    {
        // Cleans them up when the thread exits
        mysql_thread_end();
    }
};

class DatabaseManager
{

  public:
    // Define a variant for supported SQL types

    class MySQLWrapper
    {
        MYSQL* conn;
        std::mutex mtx;

      public:
        MySQLWrapper(const char* host, const char* user, const char* pass,
                     const char* db)
        {
            conn = mysql_init(nullptr);
            if (!mysql_real_connect(conn, host, user, pass, db, 0, nullptr, 0))
            {
                throw std::runtime_error(mysql_error(conn));
            }
            mysql_set_character_set(conn, "utf8mb4");
        }

        ~MySQLWrapper()
        {
            mysql_close(conn);
        }

        // Thread-safe query execution
        bool execute(const std::string& query_template,
                     const std::vector<SqlArg>& args)
        {
            std::lock_guard<std::mutex> lock(mtx); // Mutex acquired here

            std::string final_query = query_template;
            for (const auto& arg : args)
            {
                // Process each argument based on its type
                std::string formatted_value = std::visit(
                    [this](auto&& arg_val) -> std::string
                    {
                        using T = std::decay_t<decltype(arg_val)>;

                        if constexpr (std::is_same_v<T, std::string>)
                        {
                            // Escape strings using the driver handle
                            return "'" + this->escape_string(arg_val) + "'";
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            return arg_val ? "1" : "0";
                        }
                        else if constexpr (std::is_same_v<T, char>)
                        {
                            return std::string("'") + arg_val + "'";
                        }
                        else
                        {
                            // Numeric types (int, double, etc) don't need
                            // escaping
                            return std::to_string(arg_val);
                        }
                    },
                    arg);

                size_t pos = final_query.find('?');
                if (pos != std::string::npos)
                {
                    final_query.replace(pos, 1, formatted_value);
                }
            }

            if (mysql_query(conn, final_query.c_str()))
            {
                const std::string raw_mysql_err = mysql_error(conn);
                const std::string mysql_err_msg =
                    std::format("MYSQL Error: >{}<", raw_mysql_err);
                Logger::instance().debug(mysql_err_msg);
                const std::string sql_format =
                    std::format("EXEC SQL FORMAT= >{}<", query_template);
                Logger::instance().debug(sql_format);
                const std::string sql_final =
                    std::format("EXEC FINAL SQL= >%s<", final_query);
                Logger::instance().debug(sql_final);
                return false;
            }
            return true;
        }

        // Thread-safe query execution
        bool execute(const std::string& query_template)
        {
            std::lock_guard<std::mutex> lock(mtx); // Mutex acquired here

            std::string final_query = query_template;
            if (mysql_query(conn, final_query.c_str()))
            {
                const std::string raw_mysql_err = mysql_error(conn);
                const std::string mysql_err_msg =
                    std::format("MYSQL Error: >{}<", raw_mysql_err);
                Logger::instance().debug(mysql_err_msg);
                const std::string sql_format =
                    std::format("EXEC SQL FORMAT= >{}<", query_template);
                Logger::instance().debug(sql_format);
                const std::string sql_final =
                    std::format("EXEC FINAL SQL= >%s<", final_query);
                Logger::instance().debug(sql_final);
                return false;
            }
            return true;
        }

        std::vector<std::vector<std::string>>
        query(const std::string& query_template,
              const std::vector<SqlArg>& args)
        {
            std::lock_guard<std::mutex> lock(mtx);

            std::string final_query = query_template;
            for (const auto& arg : args)
            {
                // Process each argument based on its type
                std::string formatted_value = std::visit(
                    [this](auto&& arg_val) -> std::string
                    {
                        using T = std::decay_t<decltype(arg_val)>;

                        if constexpr (std::is_same_v<T, std::string>)
                        {
                            // Escape strings using the driver handle
                            return "'" + this->escape_string(arg_val) + "'";
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            return arg_val ? "1" : "0";
                        }
                        else if constexpr (std::is_same_v<T, char>)
                        {
                            return std::string("'") + arg_val + "'";
                        }
                        else
                        {
                            // Numeric types (int, double, etc) don't need
                            // escaping
                            return std::to_string(arg_val);
                        }
                    },
                    arg);
                size_t pos = final_query.find('?');
                if (pos != std::string::npos)
                {
                    final_query.replace(pos, 1, formatted_value);
                }
            }

            if (mysql_query(conn, final_query.c_str()))
            {
                const std::string sql_format =
                    std::format("QUERY SQL FORMAT= >{}<", query_template);
                Logger::instance().debug(sql_format);
                const std::string sql_final =
                    std::format("QUERY FINAL SQL= >%s<", final_query);
                Logger::instance().debug(sql_final);
                throw std::runtime_error(mysql_error(conn));
            }

            MYSQL_RES* res_set = mysql_store_result(conn);
            if (!res_set)
            {
                // mysql_store_result returns NULL if the query didn't return a
                // result set (e.g., UPDATE)
                if (mysql_field_count(conn) > 0)
                {
                    const std::string raw_mysql_err = mysql_error(conn);
                    const std::string raw_err =
                        std::format("RAW MYSQL ERROR= >{}<", raw_mysql_err);
                    Logger::instance().debug(raw_err);
                    throw std::runtime_error(raw_mysql_err.c_str());
                }

                return {};
            }

            std::vector<std::vector<std::string>> results;
            int num_fields = mysql_num_fields(res_set);
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res_set)))
            {
                std::vector<std::string> current_row;
                for (int i = 0; i < num_fields; i++)
                {
                    // row[i] can be NULL for SQL NULL values
                    current_row.push_back(row[i] ? row[i] : "");
                }
                results.push_back(std::move(current_row));
            }

            // Free the result set before releasing the lock
            mysql_free_result(res_set);
            return results;
        }

        std::vector<std::vector<std::string>>
        query(const std::string& query_template)
        {
            std::lock_guard<std::mutex> lock(mtx);

            std::string final_query = query_template;

            if (mysql_query(conn, final_query.c_str()))
            {
                throw std::runtime_error(mysql_error(conn));
            }

            MYSQL_RES* res_set = mysql_store_result(conn);
            if (!res_set)
            {
                // mysql_store_result returns NULL if the query didn't return a
                // result set (e.g., UPDATE)
                if (mysql_field_count(conn) > 0)
                {
                    const std::string raw_mysql_err = mysql_error(conn);
                    const std::string raw_err =
                        std::format("RAW MYSQL ERROR= >{}<", raw_mysql_err);
                    Logger::instance().debug(raw_err);
                    throw std::runtime_error(raw_mysql_err.c_str());
                }
                return {};
            }

            std::vector<std::vector<std::string>> results;
            int num_fields = mysql_num_fields(res_set);
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res_set)))
            {
                std::vector<std::string> current_row;
                for (int i = 0; i < num_fields; i++)
                {
                    // row[i] can be NULL for SQL NULL values
                    current_row.push_back(row[i] ? row[i] : "");
                }
                results.push_back(std::move(current_row));
            }

            // Free the result set before releasing the lock
            mysql_free_result(res_set);
            return results;
        }

        // Atomic batch execution under one lock + one transaction.
        // All statements are pre-formatted SQL (no parameter binding).
        // On any failure: ROLLBACK and return false.
        bool execTransaction(const std::vector<std::string>& stmts)
        {
            std::lock_guard<std::mutex> lock(mtx);

            if (mysql_query(conn, "START TRANSACTION"))
            {
                const std::string err = std::format(
                    "ExecTransaction: START failed: >{}<", mysql_error(conn));
                Logger::instance().debug(err);
                return false;
            }

            for (size_t idx = 0; idx < stmts.size(); ++idx)
            {
                if (mysql_query(conn, stmts[idx].c_str()))
                {
                    const std::string raw_err = mysql_error(conn);
                    const std::string errmsg =
                        std::format("ExecTransaction ROLLBACK at stmt {}: >{}<",
                                    idx, raw_err);
                    Logger::instance().debug(errmsg);
                    const std::string sql_dbg = std::format(
                        "ExecTransaction FAILED SQL= >{}<", stmts[idx]);
                    Logger::instance().debug(sql_dbg);
                    mysql_query(conn, "ROLLBACK");
                    return false;
                }
            }

            if (mysql_query(conn, "COMMIT"))
            {
                const std::string err = std::format(
                    "ExecTransaction: COMMIT failed: >{}<", mysql_error(conn));
                Logger::instance().debug(err);
                mysql_query(conn, "ROLLBACK");
                return false;
            }

            return true;
        }

      private:
        std::string escape_string(const std::string& input)
        {
            std::vector<char> buffer(input.length() * 2 + 1);
            unsigned long len = mysql_real_escape_string(
                conn, buffer.data(), input.c_str(), input.length());
            return std::string(buffer.data(), len);
        }
    };

    static const size_t SQLSZ = 1024;

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
    void init();

    bool Exec(const std::string& query_template,
              const std::vector<SqlArg>& args);
    bool Exec(const std::string& query_template);

    std::vector<std::vector<std::string>>
    Query(const std::string& query_template, const std::vector<SqlArg>& args);

    std::vector<std::vector<std::string>>
    Query(const std::string& query_template);

    // Atomic batch: all stmts in one transaction under one mutex hold.
    // Statements are pre-formatted SQL (no parameter binding).
    bool ExecTransaction(const std::vector<std::string>& stmts);

  private:
    static MySQLWrapper* driver;
    static bool m_ready;

    DatabaseManager()
    {
    }

    ~DatabaseManager()
    {
        if (driver)
        {
            SafeDelete(driver);
        }
    }
};

#endif
