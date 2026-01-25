//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "cargo_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool CargoCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // Query ship with cargo info
    auto rows = db.query(
        "SELECT ship_name, pd, sr, "
        "cargo_ferrous, cargo_rare_earth, cargo_radioactive, cargo_crystalline, "
        "cargo_volatile, cargo_water, cargo_organic, cargo_exotic, cargo_missiles "
        "FROM ships WHERE game_id=" +
        std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) +
        "' AND ship_code='" + db.esc(m_ship_code) +
        "' AND destroyed_at IS NULL");

    if (rows.empty())
    {
        Telemetry::instance().write("Ship not found: " + m_ship_code);
        return false;
    }

    std::string name = rows[0][0];
    int pd = std::atoi(rows[0][1].c_str());
    int sr = std::atoi(rows[0][2].c_str());

    // Cargo values
    int cargo[9];
    for (int i = 0; i < 9; i++)
    {
        cargo[i] = std::atoi(rows[0][3 + i].c_str());
    }

    // Capacity formula: 2*PD + SR
    int capacity = 2 * pd + sr;
    int total_cargo = 0;
    for (int i = 0; i < 8; i++)
    {
        total_cargo += cargo[i];  // missiles don't count against capacity
    }

    std::ostringstream out;
    out << "=== " << name << " (" << m_ship_code << ") CARGO MANIFEST ===\n";
    out << "Capacity: " << total_cargo << "/" << capacity;
    if (total_cargo >= capacity)
        out << " (FULL)";
    out << "\n";

    const char* names[] = {"FERROUS", "RARE_EARTH", "RADIOACTIVE", "CRYSTALLINE",
                           "VOLATILE", "WATER", "ORGANIC", "EXOTIC", "MISSILES"};

    for (int i = 0; i < 9; i++)
    {
        if (cargo[i] > 0)
        {
            out << "  " << names[i] << ": " << cargo[i] << "\n";
        }
    }

    if (total_cargo == 0 && cargo[8] == 0)
    {
        out << "  (empty)\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}
