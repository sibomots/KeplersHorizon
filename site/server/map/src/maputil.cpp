///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "maputil.h"

#include "db.h"
#include "util.h"

// Default module_id for cases where game doesn't exist yet
static const int DEFAULT_MODULE_ID = 1;

// Helper: Get module_id for a game from the games table
static int get_module_id_for_game(int game_id)
{
    if (game_id <= 0)
    {
        return DEFAULT_MODULE_ID;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "SELECT module_id FROM games WHERE id=?";

    auto rows = db.Query(q, {game_id});

    if (rows.empty() || rows[0].empty() || rows[0][0].empty())
    {
        return DEFAULT_MODULE_ID;
    }
    return std::atoi(rows[0][0].c_str());
}

std::string MapUtil::resolve_system_hex(int gid, const std::string& canon_name)
{
    DatabaseManager& db = DatabaseManager::instance();
    int module_id = get_module_id_for_game(gid);
    std::string q =
        "SELECT hex_id FROM star_systems WHERE module_id=? AND name=? LIMIT 1";
    auto r = db.Query(q, {module_id, canon_name});

    if (r.empty())
    {
        return "";
    }
    return r[0][0];
}

std::string MapUtil::resolve_system_name(int gid,
                                         const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::instance();
    int module_id = get_module_id_for_game(gid);

    std::string q = "SELECT name FROM star_systems "
                    "WHERE module_id=? AND ("
                    "UPPER(name)=UPPER(?) OR hex_id=REPLACE(LOWER(?),'h','')) "
                    "LIMIT 1";
    auto r = db.Query(q, {module_id, user_supplied, user_supplied});

    if (!r.empty() && !r[0].empty())
    {
        return r[0][0];
    }
    return user_supplied;
}

bool MapUtil::system_exists(int gid, const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::instance();
    int module_id = get_module_id_for_game(gid);
    std::string name = upper_ascii(user_supplied);
    std::string q = "SELECT name FROM star_systems WHERE module_id=? AND "
                    "UPPER(name)=? LIMIT 1";
    auto r = db.Query(q, {module_id, name});
    return !r.empty();
}
