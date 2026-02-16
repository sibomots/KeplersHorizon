///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "constraints.h"

#include <cstdlib>

#include "db.h"
#include "logger.h"

int ConstraintEngine::get_movement_modifier(int game_id,
                                            const std::string& system)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Check for movement constraints
    std::string q =
        "SELECT modifier_type, modifier_value FROM system_constraints "
        "WHERE system_name=? AND constraint_type='MOVEMENT'";
    auto rows = db.Query(q, {system});

    int total_modifier = 0;
    for (const auto& row : rows)
    {
        std::string mod_type = row[0];
        int value = std::atoi(row[1].c_str());

        // BUGBUG we should not be using strings as keys.
        if (KH_EQU(mod_type, "PENALTY"))
        {
            total_modifier += value;
        }
        else if (KH_EQU(mod_type, "BONUS"))
        {
            total_modifier -= value;
        }
        // BLOCK is handled separately
    }

    return total_modifier;
}

bool ConstraintEngine::is_movement_blocked(int game_id,
                                           const std::string& system)
{
    DatabaseManager& db = DatabaseManager::instance();

    std::string q = "SELECT COUNT(*) FROM system_constraints "
                    "WHERE system_name=? AND constraint_type='MOVEMENT' AND "
                    "modifier_type='BLOCK'";

    auto rows = db.Query(q, {system});

    return (!rows.empty() && std::atoi(rows[0][0].c_str()) > 0);
}

int ConstraintEngine::get_combat_modifier(int game_id,
                                          const std::string& system,
                                          char player)
{
    DatabaseManager& db = DatabaseManager::instance();

    int total_modifier = 0;

    // General system combat modifiers
    std::string q =
        "SELECT modifier_type, modifier_value, source FROM system_constraints "
        " WHERE system_name=? AND constraint_type='COMBAT'";
    auto rows = db.Query(q, {system});

    for (const auto& row : rows)
    {
        std::string mod_type = row[0];
        int value = std::atoi(row[1].c_str());
        std::string source = row[2];

        // Check if it's a facility-based bonus (only for controller)


        // BUGBUG strings for keys..
        if (KH_EQU(source, "FORTRESS"))
        {
            // Check facility ownership
            std::string fq = "SELECT controller FROM system_facilities "
                             "WHERE system_name=? AND facility_type='FORTRESS'";

            auto owner_check = db.Query(fq, {system});

            if (!owner_check.empty()
               && KH_EQU(owner_check[0][0][0], player))
            {
                // BUGBUG strings for keys.. why??
                if (KH_EQU(mod_type, "BONUS")) 
                {
                    total_modifier += value;
                }
            }
        }
        else
        {
            // Environmental modifier applies to all
            if (KH_EQU(mod_type, "BONUS"))
            {
                total_modifier += value;
            }
            else if (KH_EQU(mod_type, "PENALTY"))
            {
                total_modifier -= value;
            }
        }
    }

    return total_modifier;
}

int ConstraintEngine::get_extraction_modifier(int game_id,
                                              const std::string& system)
{
    DatabaseManager& db = DatabaseManager::instance();

    std::string q =
        "SELECT modifier_type, modifier_value FROM system_constraints "
        " WHERE system_name=? AND constraint_type='HARVEST'";
    auto rows = db.Query(q, {system});

    int total_modifier = 0;
    for (const auto& row : rows)
    {
        std::string mod_type = row[0];
        int value = std::atoi(row[1].c_str());

        if (KH_EQU(mod_type, "BONUS"))
        {
            total_modifier += value;
        }
        else if (KH_EQU(mod_type, "PENALTY"))
        {
            total_modifier -= value;
        }
    }

    return total_modifier;
}

std::vector<SystemConstraint>
ConstraintEngine::get_constraints(int game_id, const std::string& system)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::vector<SystemConstraint> result;

    std::string q =
        "SELECT system_name, constraint_type, modifier_type, modifier_value, "
        " condition_text, source FROM system_constraints "
        " WHERE system_name=?";
    auto rows = db.Query(q, {system});

    for (const auto& row : rows)
    {
        SystemConstraint c;
        c.system_name = row[0];

        std::string type_str = row[1];

        // BUGBUG WHY ARE THESE RESOVLED TO STRINGS FOR KEYS
        if (KH_EQU(type_str, "MOVEMENT"))
        {
            c.type = CONSTRAINT_MOVEMENT;
        }
        else if (KH_EQU(type_str, "COMBAT"))
        {
            c.type = CONSTRAINT_COMBAT;
        }
        else if (KH_EQU(type_str, "TRADE"))
        {
            c.type = CONSTRAINT_TRADE;
        }
        else if (KH_EQU(type_str, "HARVEST"))
        {
            c.type = CONSTRAINT_HARVEST;
        }
        else
        {
            c.type = CONSTRAINT_BUILD;
        }

        std::string mod_str = row[2];
        if (KH_EQU(mod_str, "BONUS"))
        {
            c.modifier = MODIFIER_BONUS;
        }
        else if (KH_EQU(mod_str, "PENALTY"))
        {
            c.modifier = MODIFIER_PENALTY;
        }
        else
        {
            c.modifier = MODIFIER_BLOCK;
        }
        c.value = std::atoi(row[3].c_str());
        c.condition = row[4];
        c.source = row[5];

        result.push_back(c);
    }

    return result;
}
