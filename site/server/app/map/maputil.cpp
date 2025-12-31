//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "maputil.h"

#include "db.h"
#include "util.h"

// BUGBUG
std::string MapUtil::resolve_system_hex(int game_id,
                                        const std::string& canon_name)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::ostringstream q;
    q << "SELECT hex_id FROM star_systems WHERE game_id=" << game_id
      << " AND name='" << db.esc(canon_name) << "' LIMIT 1";
    auto r = db.query(q.str());
    if (r.empty())
    {
        return "";
    }
    return r[0][0];
}

// BUGBUG
std::string MapUtil::resolve_system_name(int game_id,
                                         const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string u = upper_ascii(user_supplied);
    auto r = db.query("SELECT name FROM star_systems WHERE game_id=" +
                      std::to_string(game_id) + " AND UPPER(name)='" +
                      db.esc(u) + "' LIMIT 1");
    if (!r.empty() && !r[0].empty())
        return r[0][0];
    return u;
}

// BUGBUG
bool MapUtil::system_exists(int game_id, const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string u = upper_ascii(user_supplied);
    auto r = db.query("SELECT name FROM star_systems WHERE game_id=" +
                      std::to_string(game_id) + " AND UPPER(name)='" +
                      db.esc(u) + "' LIMIT 1");
    return !r.empty();
}
