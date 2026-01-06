//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "salvage_command.h"

#include <cstdlib>
#include <ctime>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"

// Random resource table for salvage
static const char* SALVAGE_RESOURCES[] = {"FERROUS",     "RARE_EARTH", "CRYSTALLINE",
                                          "RADIOACTIVE", "VOLATILE",   "EXOTIC"};
static const int NUM_SALVAGE_RESOURCES = 6;

bool SalvageCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    if (m_ship_code.empty())
    {
        Telemetry::getInstance().write(
            "Usage: salvage <ship>\n"
            "Ship must be at a system with salvageable wreckage (e.g., SYDRA).");
        return true;
    }

    // Verify ship exists and get location
    if (!ship_exists(game_id, me, m_ship_code))
    {
        Telemetry::getInstance().write("SALVAGE: Ship " + m_ship_code +
                                       " not found.");
        return false;
    }

    ShipRow ship = load_ship(game_id, me, m_ship_code);

    if (ship.at_system.empty())
    {
        Telemetry::getInstance().write(
            "SALVAGE: Ship must be deployed to salvage.");
        return false;
    }

    // Check if system has salvageable anomaly
    auto anomaly = db.query(
        "SELECT condition_text FROM system_constraints "
        "WHERE system_name='" +
        db.esc(ship.at_system) + "' AND source='SCRAPYARD'");

    if (anomaly.empty())
    {
        Telemetry::getInstance().write("SALVAGE: No salvageable wreckage in " +
                                       ship.at_system +
                                       ". Try SYDRA (The Graveyard).");
        return false;
    }

    // Check ship has drones (required for salvage)
    auto dr_check = db.query("SELECT dr FROM ships WHERE game_id=" +
                             std::to_string(game_id) + " AND owner='" +
                             std::string(1, me) + "' AND ship_code='" +
                             db.esc(m_ship_code) + "'");

    int drones = dr_check.empty() ? 0 : std::atoi(dr_check[0][0].c_str());
    if (drones < 1)
    {
        Telemetry::getInstance().write(
            "SALVAGE: " + ship.name +
            " needs Drones (DR) to salvage wreckage safely.");
        return false;
    }

    // Perform salvage - random outcome
    srand(time(NULL) + game_id + s.round);

    int roll = rand() % 100;
    std::ostringstream result;

    if (roll < 10)
    {
        // Hazard encounter (10% chance)
        int damage = 1 + (rand() % 2);
        result << "SALVAGE: " << ship.name
               << " encountered automated defenses! "
               << "Suffered " << damage << " hull damage.";

        // Apply damage to PD
        int new_pd = ship.attr.PD - damage;
        if (new_pd < 0)
            new_pd = 0;

        db.exec("UPDATE ships SET pd=" + std::to_string(new_pd) +
                " WHERE game_id=" + std::to_string(game_id) + " AND owner='" +
                std::string(1, me) + "' AND ship_code='" + db.esc(m_ship_code) +
                "'");

        // Log event
        db.exec(
            "INSERT INTO salvage_operations(game_id,system_name,ship_code,turn,"
            "resources_found,hazard_encountered) VALUES(" +
            std::to_string(game_id) + ",'" + db.esc(ship.at_system) + "','" +
            db.esc(m_ship_code) + "'," + std::to_string(s.round) +
            ",'{}',TRUE)");

        if (new_pd == 0)
        {
            result << " Ship destroyed!";
            db.exec("UPDATE ships SET destroyed_at=NOW() WHERE game_id=" +
                    std::to_string(game_id) + " AND owner='" +
                    std::string(1, me) + "' AND ship_code='" +
                    db.esc(m_ship_code) + "'");
        }
    }
    else
    {
        // Successful salvage
        int res_idx = rand() % NUM_SALVAGE_RESOURCES;
        std::string resource = SALVAGE_RESOURCES[res_idx];
        int quantity = 2 + (rand() % 4);  // 2-5 units

        // Map resource to cargo column
        std::string col = "cargo_ferrous";
        if (resource == "RARE_EARTH")
            col = "cargo_rare_earth";
        else if (resource == "CRYSTALLINE")
            col = "cargo_crystalline";
        else if (resource == "RADIOACTIVE")
            col = "cargo_radioactive";
        else if (resource == "VOLATILE")
            col = "cargo_volatile";
        else if (resource == "EXOTIC")
            col = "cargo_exotic";

        db.exec("UPDATE ships SET " + col + "=" + col + "+" +
                std::to_string(quantity) + " WHERE game_id=" +
                std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                "' AND ship_code='" + db.esc(m_ship_code) + "'");

        result << "SALVAGE: " << ship.name << " recovered " << quantity
               << " units of " << resource << " from wreckage!";

        // Log event
        db.exec(
            "INSERT INTO salvage_operations(game_id,system_name,ship_code,turn,"
            "resources_found,hazard_encountered) VALUES(" +
            std::to_string(game_id) + ",'" + db.esc(ship.at_system) + "','" +
            db.esc(m_ship_code) + "'," + std::to_string(s.round) + "','{\"" +
            resource + "\":" + std::to_string(quantity) + "}',FALSE)");
    }

    Logger::instance().info(result.str());
    Telemetry::getInstance().write(result.str());

    return true;
}
