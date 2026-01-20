//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "star_system_constraints.h"

#include <cstdlib>

#include "db.h"
#include "moduleutil.h"

// Helper: Get star system name for a hex ID
std::string StarSystemConstraints::getSystemForHex(int game_id,
                                                    const std::string& hex_id)
{
    if (hex_id.empty())
        return "";

    DatabaseManager& db = DatabaseManager::getInstance();
    int mod = get_module_id_for_game(game_id);

    auto rows =
        db.query("SELECT name FROM star_systems WHERE module_id=" +
                 std::to_string(mod) + " AND hex_id='" + db.esc(hex_id) + "'");

    return rows.empty() ? "" : rows[0][0];
}

// Helper: Query constraint modifier from system_constraints table
int StarSystemConstraints::getConstraintModifier(int game_id,
                                                  const std::string& hex_id,
                                                  const std::string& constraint_type)
{
    std::string system = getSystemForHex(game_id, hex_id);
    if (system.empty())
        return 0;

    DatabaseManager& db = DatabaseManager::getInstance();

    auto rows =
        db.query("SELECT modifier_type, modifier_value FROM system_constraints "
                 "WHERE system_name='" +
                 db.esc(system) + "' AND constraint_type='" +
                 db.esc(constraint_type) + "'");

    int total = 0;
    for (const auto& row : rows)
    {
        std::string mod_type = row[0];
        int value = std::atoi(row[1].c_str());

        if (mod_type == "PENALTY")
        {
            total -= value; // Penalty reduces effectiveness
        }
        else if (mod_type == "BONUS")
        {
            total += value; // Bonus increases effectiveness
        }
        // BLOCK is handled separately where needed
    }

    return total;
}

// Combat modifiers
int StarSystemConstraints::getBeamModifier(int game_id, const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "COMBAT");
}

int StarSystemConstraints::getScreenModifier(int game_id,
                                              const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "COMBAT");
}

int StarSystemConstraints::getMissileModifier(int game_id,
                                               const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "COMBAT");
}

int StarSystemConstraints::getDriveModifier(int game_id,
                                             const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "MOVEMENT");
}

// Movement/Repair/Resupply modifiers
int StarSystemConstraints::getMovementModifier(int game_id,
                                                const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "MOVEMENT");
}

int StarSystemConstraints::getRepairModifier(int game_id,
                                              const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "BUILD");
}

int StarSystemConstraints::getResupplyModifier(int game_id,
                                                const std::string& hex_id)
{
    return getConstraintModifier(game_id, hex_id, "BUILD");
}
