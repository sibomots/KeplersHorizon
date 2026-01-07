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

// Map data uses map_id=1 (the default Kepler map)
// Future: could support multiple maps by passing map_id from game state
static const int DEFAULT_MAP_ID = 1;

std::string MapUtil::resolve_system_hex(int /* gid */ , const std::string& canon_name)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::ostringstream q;
    q << "SELECT hex_id FROM star_systems WHERE map_id=" << DEFAULT_MAP_ID
      << " AND name='" << db.esc(canon_name) << "' LIMIT 1";
    auto r = db.query(q.str());
    if (r.empty())
    {
        return "";
    }
    return r[0][0];
}

std::string MapUtil::resolve_system_name(int /* gid */,
                                         const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string u = upper_ascii(user_supplied);
    auto r = db.query("SELECT name FROM star_systems WHERE map_id=" +
                      std::to_string(DEFAULT_MAP_ID) + " AND UPPER(name)='" +
                      db.esc(u) + "' LIMIT 1");
    if (!r.empty() && !r[0].empty())
    {
        return r[0][0];
    }
    return u;
}

bool MapUtil::system_exists(int /* gid */, const std::string& user_supplied)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string u = upper_ascii(user_supplied);
    auto r = db.query("SELECT name FROM star_systems WHERE map_id=" +
                      std::to_string(DEFAULT_MAP_ID) + " AND UPPER(name)='" +
                      db.esc(u) + "' LIMIT 1");
    return !r.empty();
}
