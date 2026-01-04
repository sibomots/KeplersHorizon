//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __CSV_LOADER_H__
#define __CSV_LOADER_H__

#include <string>
#include <vector>

// CSV Loader - Reads CSV files and generates SQL INSERT statements
// Used for loading milieu seed data at server startup
class CSVLoader
{
  public:
    // Load a CSV file into a database table
    // Returns number of rows inserted, or -1 on error
    static int load_csv(const std::string& filepath,
                        const std::string& table_name);

    // Load game seed CSVs from a directory (star_systems, warplines, hexes)
    static int load_game_data(const std::string& base_path);

    // Load milieu CSVs from a directory (species, planets, resources, etc.)
    static int load_milieu_data(const std::string& base_path);

  private:
    // Parse a single CSV line respecting quoted fields
    static std::vector<std::string> parse_csv_line(const std::string& line);

    // Escape a value for SQL insertion
    static std::string escape_sql(const std::string& val);
};

#endif
