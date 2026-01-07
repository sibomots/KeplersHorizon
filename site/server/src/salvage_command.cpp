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

bool SalvageCommand::invoke(void)
{
    if (m_scan_mode)
    {
        do_scan();
        return true;
    }

    if (m_ship_code.empty())
    {
        Telemetry::getInstance().write("Usage: salvage scan\n"
                                       "       salvage <ship>\n"
                                       "       salvage <ship> <target_name>");
        return true;
    }

    return do_salvage();
}

void SalvageCommand::do_scan()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Get player's ship locations
    auto ships =
        db.query("SELECT DISTINCT at_system FROM ships WHERE game_id=" +
                 std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                 "' AND at_system IS NOT NULL AND at_system != '' AND "
                 "destroyed_at IS NULL");

    if (ships.empty())
    {
        Telemetry::getInstance().write(
            "SALVAGE SCAN: No deployed ships to scan from.");
        return;
    }

    std::ostringstream out;
    out << "=== SALVAGE SCAN RESULTS ===\n";

    int total_found = 0;
    srand(time(NULL) + game_id + s.round);

    for (const auto& loc : ships)
    {
        std::string sys = loc[0];

        // Query salvageables in this system
        auto salvs = db.query(
            "SELECT id, name, description, discovery_chance FROM salvageables "
            "WHERE system_name='" +
            db.esc(sys) + "'");

        if (salvs.empty())
            continue;

        out << "\n" << sys << ":\n";

        for (const auto& salv : salvs)
        {
            int salv_id = std::atoi(salv[0].c_str());
            std::string name = salv[1];
            std::string desc = salv[2];
            int disc_chance = std::atoi(salv[3].c_str());

            // Check if already discovered
            auto disc = db.query(
                "SELECT discovered_by, depleted FROM discovered_salvageables "
                "WHERE game_id=" +
                std::to_string(game_id) +
                " AND salvageable_id=" + std::to_string(salv_id));

            if (!disc.empty())
            {
                // Already discovered
                bool depleted = (disc[0][1] == "1");
                out << "  [KNOWN] " << name;
                if (depleted)
                    out << " (DEPLETED)";
                out << "\n";
                total_found++;
            }
            else
            {
                // Roll for discovery
                int roll = rand() % 100;
                if (roll < disc_chance)
                {
                    // Discovered!
                    db.exec("INSERT INTO "
                            "discovered_salvageables(game_id,salvageable_id,"
                            "discovered_by,discovered_turn) VALUES(" +
                            std::to_string(game_id) + "," +
                            std::to_string(salv_id) + ",'" +
                            std::string(1, me) + "'," +
                            std::to_string(s.round) + ")");

                    out << "  [NEW!] " << name << "\n";
                    out << "         \"" << desc << "\"\n";
                    total_found++;
                }
                else
                {
                    out << "  [?] Unknown signal detected (scan again?)\n";
                }
            }
        }
    }

    if (total_found == 0)
    {
        out << "\nNo salvageables found. Try scanning in SYDRA or similar "
               "systems.\n";
    }
    else
    {
        out << "\n" << total_found << " salvageable(s) known.\n";
        out << "Use: salvage <ship> <target_name> to salvage.\n";
    }

    Telemetry::getInstance().write(out.str());
}

bool SalvageCommand::do_salvage()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Verify ship exists
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

    // If no target, list available
    if (m_target_name.empty())
    {
        auto avail = db.query(
            "SELECT s.name, ds.depleted FROM salvageables s "
            "JOIN discovered_salvageables ds ON s.id=ds.salvageable_id "
            "WHERE s.system_name='" +
            db.esc(ship.at_system) +
            "' "
            "AND ds.game_id=" +
            std::to_string(game_id) + " AND ds.depleted=0");

        if (avail.empty())
        {
            Telemetry::getInstance().write(
                "SALVAGE: No known salvageables in " + ship.at_system +
                ". Use 'salvage scan' to search.");
            return true;
        }

        std::ostringstream out;
        out << "Available salvageables in " << ship.at_system << ":\n";
        for (const auto& a : avail)
        {
            out << "  - " << a[0] << "\n";
        }
        out << "Use: salvage " << m_ship_code << " <target_name>";
        Telemetry::getInstance().write(out.str());
        return true;
    }

    // Find target salvageable
    auto target =
        db.query("SELECT s.id, s.name, s.hazard_chance, s.hazard_damage_min, "
                 "s.hazard_damage_max, s.max_salvages, ds.times_salvaged "
                 "FROM salvageables s "
                 "JOIN discovered_salvageables ds ON s.id=ds.salvageable_id "
                 "WHERE s.system_name='" +
                 db.esc(ship.at_system) +
                 "' "
                 "AND UPPER(s.name) LIKE UPPER('%" +
                 db.esc(m_target_name) +
                 "%') "
                 "AND ds.game_id=" +
                 std::to_string(game_id) + " AND ds.depleted=0 LIMIT 1");

    if (target.empty())
    {
        Telemetry::getInstance().write("SALVAGE: No salvageable matching '" +
                                       m_target_name + "' found in " +
                                       ship.at_system + ".");
        return false;
    }

    int salv_id = std::atoi(target[0][0].c_str());
    std::string salv_name = target[0][1];
    int hazard_chance = std::atoi(target[0][2].c_str());
    int hazard_min = std::atoi(target[0][3].c_str());
    int hazard_max = std::atoi(target[0][4].c_str());
    int max_salvages =
        target[0][5].empty() ? -1 : std::atoi(target[0][5].c_str());
    int times_salvaged = std::atoi(target[0][6].c_str());

    // Check ship has drones
    auto dr_check = db.query(
        "SELECT dr FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, me) + "' AND ship_code='" +
        db.esc(m_ship_code) + "'");

    int drones = dr_check.empty() ? 0 : std::atoi(dr_check[0][0].c_str());
    if (drones < 1)
    {
        Telemetry::getInstance().write("SALVAGE: " + ship.name +
                                       " needs Drones (DR) to salvage safely.");
        return false;
    }

    // Perform salvage
    srand(time(NULL) + game_id + s.round + salv_id);
    std::ostringstream result;

    // Hazard check
    int hazard_roll = rand() % 100;
    if (hazard_roll < hazard_chance)
    {
        int damage = hazard_min + (rand() % (hazard_max - hazard_min + 1));
        result << "SALVAGE: " << ship.name << " encountered hazards at "
               << salv_name << "! Suffered " << damage << " hull damage.\n";

        int new_pd = ship.attr.PD - damage;
        if (new_pd < 0)
            new_pd = 0;

        db.exec("UPDATE ships SET pd=" + std::to_string(new_pd) +
                " WHERE game_id=" + std::to_string(game_id) + " AND owner='" +
                std::string(1, me) + "' AND ship_code='" + db.esc(m_ship_code) +
                "'");

        if (new_pd == 0)
        {
            result << "Ship destroyed!";
            db.exec("UPDATE ships SET destroyed_at=NOW() WHERE game_id=" +
                    std::to_string(game_id) + " AND owner='" +
                    std::string(1, me) + "' AND ship_code='" +
                    db.esc(m_ship_code) + "'");
            Telemetry::getInstance().write(result.str());
            return true;
        }
    }

    // Process drops
    auto drops = db.query(
        "SELECT item_type, item_name, drop_chance, quantity_min, quantity_max "
        "FROM salvageable_drops WHERE salvageable_id=" +
        std::to_string(salv_id));

    bool got_something = false;
    for (const auto& drop : drops)
    {
        std::string item_type = drop[0];
        std::string item_name = drop[1];
        int drop_chance = std::atoi(drop[2].c_str());
        int qty_min = std::atoi(drop[3].c_str());
        int qty_max = std::atoi(drop[4].c_str());

        int roll = rand() % 100;
        if (roll >= drop_chance)
            continue;

        int qty = qty_min + (rand() % (qty_max - qty_min + 1));
        got_something = true;

        if (item_type == "resource")
        {
            // Map to cargo column
            std::string col = "cargo_ferrous";
            if (item_name == "RARE_EARTH")
                col = "cargo_rare_earth";
            else if (item_name == "RADIOACTIVE")
                col = "cargo_radioactive";
            else if (item_name == "CRYSTALLINE")
                col = "cargo_crystalline";
            else if (item_name == "VOLATILE")
                col = "cargo_volatile";
            else if (item_name == "WATER")
                col = "cargo_water";
            else if (item_name == "ORGANIC")
                col = "cargo_organic";
            else if (item_name == "EXOTIC")
                col = "cargo_exotic";

            db.exec("UPDATE ships SET " + col + "=" + col + "+" +
                    std::to_string(qty) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND owner='" + std::string(1, me) + "' AND ship_code='" +
                    db.esc(m_ship_code) + "'");

            result << "  Recovered " << qty << " " << item_name << "\n";
        }
        else if (item_type == "missiles")
        {
            db.exec("UPDATE ships SET missiles=missiles+" +
                    std::to_string(qty) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND owner='" + std::string(1, me) + "' AND ship_code='" +
                    db.esc(m_ship_code) + "'");

            result << "  Recovered " << qty << " missiles\n";
        }
        else if (item_type == "credits")
        {
            std::string cred_col = (me == 'A') ? "credits_A" : "credits_B";
            db.exec("UPDATE games SET " + cred_col + "=" + cred_col + "+" +
                    std::to_string(qty) +
                    " WHERE id=" + std::to_string(game_id));

            result << "  Found " << qty << " CR in valuables\n";
        }
    }

    if (!got_something)
    {
        result << "SALVAGE: " << ship.name << " salvaged " << salv_name
               << " but found nothing useful.\n";
    }
    else
    {
        std::string header =
            "SALVAGE: " + ship.name + " salvaged " + salv_name + ":\n";
        result.str(header + result.str());
    }

    // Update salvage count and check depletion
    db.exec(
        "UPDATE discovered_salvageables SET times_salvaged=times_salvaged+1 "
        "WHERE game_id=" +
        std::to_string(game_id) +
        " AND salvageable_id=" + std::to_string(salv_id));

    if (max_salvages > 0 && times_salvaged + 1 >= max_salvages)
    {
        db.exec("UPDATE discovered_salvageables SET depleted=1 "
                "WHERE game_id=" +
                std::to_string(game_id) +
                " AND salvageable_id=" + std::to_string(salv_id));
        result << salv_name << " is now depleted.\n";
    }

    // Log operation
    db.exec("INSERT INTO salvage_operations(game_id,system_name,ship_code,turn,"
            "resources_found,hazard_encountered) VALUES(" +
            std::to_string(game_id) + ",'" + db.esc(ship.at_system) + "','" +
            db.esc(m_ship_code) + "'," + std::to_string(s.round) +
            ",'{}',FALSE)");

    Logger::instance().info(result.str());
    Telemetry::getInstance().write(result.str());

    return true;
}
