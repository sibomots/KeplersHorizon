//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "csv_loader.h"

#include <fstream>
#include <sstream>

#include "db.h"
#include "logger.h"

std::vector<std::string> CSVLoader::parse_csv_line(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"')
            {
                // Escaped quote
                field += '"';
                ++i;
            }
            else
            {
                in_quotes = !in_quotes;
            }
        }
        else if (c == ',' && !in_quotes)
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

std::string CSVLoader::escape_sql(const std::string& val)
{
    if (val.empty())
    {
        return "NULL";
    }

    std::string escaped;
    escaped.reserve(val.size() + 2);

    for (char c : val)
    {
        if (c == '\'')
        {
            escaped += "''";
        }
        else if (c == '\\')
        {
            escaped += "\\\\";
        }
        else
        {
            escaped += c;
        }
    }

    return "'" + escaped + "'";
}

int CSVLoader::load_csv(const std::string& filepath,
                        const std::string& table_name)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        Logger::instance().error("[CSV] Failed to open: " + filepath);
        return -1;
    }

    std::string line;

    // Read header line
    if (!std::getline(file, line))
    {
        Logger::instance().error("[CSV] Empty file: " + filepath);
        return -1;
    }

    std::vector<std::string> headers = parse_csv_line(line);
    if (headers.empty())
    {
        Logger::instance().error("[CSV] No headers in: " + filepath);
        return -1;
    }

    // Build column list
    std::ostringstream col_list;
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (i > 0)
            col_list << ",";
        col_list << headers[i];
    }

    DatabaseManager& db = DatabaseManager::getInstance();
    int inserted = 0;

    // Read data rows
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue; // Skip empty lines and comments
        }

        std::vector<std::string> values = parse_csv_line(line);
        if (values.size() != headers.size())
        {
            Logger::instance().error("[CSV] Column mismatch in " + filepath +
                                     ": expected " +
                                     std::to_string(headers.size()) + ", got " +
                                     std::to_string(values.size()));
            continue;
        }

        // Build INSERT statement
        std::ostringstream sql;
        sql << "INSERT IGNORE INTO " << table_name << " (" << col_list.str()
            << ") VALUES (";

        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
                sql << ",";
            sql << escape_sql(values[i]);
        }
        sql << ")";

        try
        {
            db.exec(sql.str());
            ++inserted;
        }
        catch (const std::exception& ex)
        {
            Logger::instance().error("[CSV] Insert failed: " +
                                     std::string(ex.what()));
        }
    }

    return inserted;
}

int CSVLoader::load_game_data(const std::string& base_path)
{
    Logger::instance().info("[CSV] Loading game data from: " + base_path);

    // Game data tables - load order matters
    struct CSVTable
    {
        const char* filename;
        const char* table;
    };

    static const CSVTable tables[] = {
        {"star_systems.csv", "star_systems"},
        {"warplines.csv", "warplines"},
        {"hexes.csv", "hexes"},
        {"warpline_hexes.csv", "warpline_hexes"},
    };

    int total = 0;
    for (const auto& t : tables)
    {
        std::string path = base_path + "/" + t.filename;
        int count = load_csv(path, t.table);
        if (count >= 0)
        {
            Logger::instance().info("[CSV] Loaded " + std::to_string(count) +
                                    " rows from " + t.filename);
            total += count;
        }
    }

    Logger::instance().info("[CSV] Total: " + std::to_string(total) +
                            " game records loaded");
    return total;
}

int CSVLoader::load_milieu_data(const std::string& base_path)
{
    Logger::instance().info("[CSV] Loading milieu data from: " + base_path);

    // Load order matters due to foreign key relationships
    struct CSVTable
    {
        const char* filename;
        const char* table;
    };

    static const CSVTable tables[] = {
        {"species.csv", "system_species"},
        {"stars.csv", "system_stars"},
        {"planets.csv", "system_planets"},
        {"moons.csv", "system_moons"},
        {"asteroid_belts.csv", "system_asteroid_belts"},
        {"resources.csv", "system_resources"},
        {"populations.csv", "system_populations"},
        {"facilities.csv", "system_facilities"},
        {"anomalies.csv", "system_anomalies"},
        {"grimoire_rumors.csv", "system_grimoire_rumors"},
    };

    int total = 0;
    for (const auto& t : tables)
    {
        std::string path = base_path + "/" + t.filename;
        int count = load_csv(path, t.table);
        if (count >= 0)
        {
            Logger::instance().info("[CSV] Loaded " + std::to_string(count) +
                                    " rows from " + t.filename);
            total += count;
        }
    }

    Logger::instance().info("[CSV] Total: " + std::to_string(total) +
                            " milieu records loaded");
    return total;
}

