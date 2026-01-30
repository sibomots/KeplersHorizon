//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "moduleutil.h"

#include "db.h"

// Default module_id for Kepler's Horizon
static const int DEFAULT_MODULE_ID = 1;

int get_module_id_for_game(int game_id)
{
    if (game_id <= 0)
    {
        return DEFAULT_MODULE_ID;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "SELECT module_id FROM games WHERE id=?";
    auto rows = db.Query(q, { game_id});

    if (rows.empty() || rows[0].empty() || rows[0][0].empty())
    {
        return DEFAULT_MODULE_ID;
    }
    return std::atoi(rows[0][0].c_str());
}
