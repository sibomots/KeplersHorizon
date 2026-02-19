///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "salvage_strategy.h"

#include "db.h"
#include "hex_events.h"
#include "shipmgr.h"

bool SalvageStrategy::do_scan()
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    // Get player's ship locations
    auto ships = db.Query(
        "SELECT DISTINCT at_system FROM ships WHERE game_id=? "
        "AND owner=? AND at_system IS NOT NULL AND at_system != '' AND "
        "destroyed_at IS NULL",
        {game_id, me});

    if (ships.empty())
    {
        Telemetry::instance().write(LC_SALVAGE_NO_SHIPS);
        return false;
    }

    std::ostringstream out;
    out << "=== SALVAGE SCAN RESULTS ===\n";

    int total_found = 0;
    srand(time(NULL) + game_id + s.round);

    for (const auto& loc : ships)
    {
        std::string sys = loc[0];

        // Query salvageables in this system
        auto salvs = db.Query(
            "SELECT id, name, description, discovery_chance FROM salvageables "
            "WHERE system_name=?",
            {sys});

        if (salvs.empty())
        {
            continue;
        }

        out << "\n" << sys << ":\n";

        for (const auto& salv : salvs)
        {
            int salv_id = std::atoi(salv[0].c_str());
            std::string name = salv[1];
            std::string desc = salv[2];
            int disc_chance = std::atoi(salv[3].c_str());

            // Check if already discovered
            auto disc = db.Query(
                "SELECT discovered_by, depleted FROM discovered_salvageables "
                "WHERE game_id=? AND salvageable_id=?",
                {game_id, salv_id});

            if (!disc.empty())
            {
                // Already discovered
                bool depleted = (KH_EQU(disc[0][1], "1"));
                out << "  [KNOWN] " << name;
                if (depleted)
                {
                    out << " (DEPLETED)";
                }
                out << "\n";
                total_found++;
            }
            else
            {
                // Query knowledge level for discovery modifier
                auto knowledge = db.Query(
                    "SELECT knowledge_level FROM codex_entries WHERE game_id=? "
                    "AND player=? AND system_name=?",
                    {game_id, me, sys});

                // BUGBUG -- these strings are in the database and not localized.
                std::string know_level =
                    knowledge.empty() ? "Unknown" : knowledge[0][0];
                double intel_mult = 0.5; // Unknown = half chance
                if (KH_EQU(know_level, "Charted"))
                {
                    intel_mult = 0.75;
                }
                else if (KH_EQU(know_level, "Surveyed"))
                {
                    intel_mult = 1.0;
                }
                else if (KH_EQU(know_level, "Intimate"))
                {
                    // Better than base
                    intel_mult = 1.25;
                }

                int effective_chance = (int)(disc_chance * intel_mult);

                // Roll for discovery with knowledge-modified chance
                int roll = rand() % 100;
                if (roll < effective_chance)
                {
                    // Discovered!
                    db.Exec("INSERT INTO "
                            "discovered_salvageables(game_id,salvageable_id,"
                            "discovered_by,discovered_turn) VALUES(?,?,?,?)",
                            {game_id, salv_id, me, s.round});

                    out << "  [NEW!] " << name << "\n";
                    out << "         \"" << desc << "\"\n";
                    total_found++;
                }
                else
                {
                    out << "  " LC_SALVAGE_UNKNOWN_SIGNAL "\n";
                }
            }
        }
    }

    if (KH_EQU(total_found,0))
    {
        out << "\n" LC_SALVAGE_NONE "\n";
    }
    else
    {
        out << "\n"
            << std::format(LC_SALVAGE_FOUND, total_found)
            << "\n"
            << LC_SALVAGE_EXAMPLE
            << "\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}

bool SalvageStrategy::do_salvage(const std::string& ship_code,
                                 const std::string& resource)
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    // Verify ship exists
    if (!ShipManager::instance().ship_exists_by_code_or_name(game_id, me,
                                                             ship_code))
    {
        Telemetry::instance().write(
             std::format(LC_SALVAGE_SHIP_NOT_FOUND, ship_code));
        return false;
    }

    ShipRow ship;
    bool has_ship = ShipManager::instance().load_ship_by_code_or_name(
        ship, game_id, me, ship_code);

    if (has_ship && ship.at_system.empty())
    {
        Telemetry::instance().write(LC_SALVAGE_NEED_DEPLOYED_SHIP);
        return false;
    }

    if (resource.empty())
    {
        auto avail = db.Query(
            "SELECT s.name, ds.depleted FROM salvageables s "
            "JOIN discovered_salvageables ds ON s.id=ds.salvageable_id "
            "WHERE s.system_name=? AND ds.game_id=? AND ds.depleted=0",
            {ship.at_system, game_id});

        if (avail.empty())
        {
            Telemetry::instance().write(
                 std::format(LC_SALVAGE_TARGET_NOT_FOUND, ship.at_system));
            Telemetry::instance().write(LC_SALVAGE_SUGGEST_SCAN);
            return true;
        }

        std::ostringstream out;
        out << std::format(LC_SALVAGE_TARGET_AVAILABLE, ship.at_system)
            << "\n";
        for (const auto& a : avail)
        {
            out << "  - " << a[0] << "\n";
        }
        out << LC_SALVAGE_SUGGEST_TARGET_SALVAGE;
        Telemetry::instance().write(out.str());
        return true;
    }

    // Find target salvageable
    auto target =
        db.Query("SELECT s.id, s.name, s.hazard_chance, s.hazard_damage_min, "
                 "s.hazard_damage_max, s.max_salvages, ds.times_salvaged "
                 "FROM salvageables s "
                 "JOIN discovered_salvageables ds ON s.id=ds.salvageable_id "
                 "WHERE s.system_name=? "
                 "AND UPPER(s.name) LIKE UPPER(CONCAT('%',?,'%')) "
                 "AND ds.game_id=? AND ds.depleted=0 LIMIT 1",
                 {ship.at_system, resource, game_id});

    if (resource.empty())
    {
        Telemetry::instance().write(
           std::format(LC_SALVAGE_NO_MATCH_TARGET,
                  resource, ship.at_system));
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

    // Perform salvage
    srand(time(NULL) + game_id + s.round + salv_id);
    std::ostringstream result;

    // Query knowledge level for hazard modifier
    auto knowledge =
        db.Query("SELECT knowledge_level FROM codex_entries WHERE game_id=? "
                 "AND player=? AND system_name=?",
                 {game_id, me, ship.at_system});

    std::string know_level = knowledge.empty() ? "Unknown" : knowledge[0][0];
    int hazard_mod = 20; // Unknown = +20% more dangerous
    if (KH_EQU(know_level, "Charted"))
    {
        hazard_mod = 10;
    }
    else if (KH_EQU(know_level, "Surveyed"))
    {
        hazard_mod = 0;
    }
    else if (KH_EQU(know_level, "Intimate"))
    {
        hazard_mod = -10; // Safer with detailed charts
    }
    int effective_hazard = hazard_chance + hazard_mod;
    if (effective_hazard < 0)
    {
        effective_hazard = 0;
    }

    // Hazard check with knowledge-modified chance
    int hazard_roll = rand() % 100;
    if (hazard_roll < effective_hazard)
    {
        int damage = hazard_min + (rand() % (hazard_max - hazard_min + 1));

        result << std::format(LC_SALVAGE_TARGET_HAZARD,
                  ship.name, salv_name, damage) << "\n";

        int new_pd = ship.attr.PD - damage;
        if (new_pd < 0)
        {
            new_pd = 0;
        }
        db.Exec("UPDATE ships SET pd=? WHERE game_id=? AND owner=? "
                "AND ship_code=?",
                {new_pd, game_id, me, ship_code});

        if (KH_EQU(new_pd, 0))
        {
            result << std::format(LC_SALVAGE_SHIP_DESTROYED, ship.name)
                   << "\n"; 
            db.Exec("UPDATE ships SET destroyed_at=NOW() WHERE game_id=? "
                    "AND owner=? AND ship_code=?",
                    {game_id, me, ship_code});
            Telemetry::instance().write(result.str());
            return true;
        }
    }

    // Process drops
    auto drops = db.Query(
        "SELECT item_type, item_name, drop_chance, quantity_min, quantity_max "
        "FROM salvageable_drops WHERE salvageable_id=?",
        {salv_id});

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
        {
            continue;
        }

        int qty = qty_min + (rand() % (qty_max - qty_min + 1));

        // Apply dynamic hex event modifier (SALVAGE_OPPORTUNITY)
        float mult = HexEventEngine::get_salvage_multiplier(game_id, s.round,
                                                            ship.at_hex);
        qty = (int)(qty * mult);

        got_something = true;

        if (KH_EQU(item_type, "resource"))
        {
            // Map to cargo column
            std::string col("cargo_ferrous");
            if (KH_EQU(item_name, "RARE_EARTH"))
            {
                col = "cargo_rare_earth";
            }
            else if (KH_EQU(item_name, "RADIOACTIVE"))
            {
                col = "cargo_radioactive";
            }
            else if (KH_EQU(item_name, "CRYSTALLINE"))
            {
                col = "cargo_crystalline";
            }
            else if (KH_EQU(item_name, "VOLATILE"))
            {
                col = "cargo_volatile";
            }
            else if (KH_EQU(item_name, "WATER"))
            {
                col = "cargo_water";
            }
            else if (KH_EQU(item_name, "ORGANIC"))
            {
                col = "cargo_organic";
            }
            else if (KH_EQU(item_name, "EXOTIC"))
            {
                col = "cargo_exotic";
            }

            db.Exec("UPDATE ships SET " + col + "=" + col +
                        "+? "
                        "WHERE game_id=? AND owner=? AND ship_code=?",
                    {qty, game_id, me, ship_code});

            result << "  Recovered " << qty << " " << item_name << "\n";
        }
        else if (KH_EQU(item_type, "torpedoes"))
        {
            db.Exec("UPDATE ships SET torpedoes=torpedoes+? "
                    "WHERE game_id=? AND owner=? AND ship_code=?",
                    {qty, game_id, me, ship_code});

            result << "  Recovered " << qty << " torpedoes\n";
        }
        else if (KH_EQU(item_type, "credits"))
        {
            std::string cred_col = (KH_EQU(me, 'A')) ? "credits_A" : "credits_B";
            db.Exec("UPDATE games SET " + cred_col + "=" + cred_col +
                        "+? "
                        "WHERE id=?",
                    {qty, game_id});

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
    db.Exec(
        "UPDATE discovered_salvageables SET times_salvaged=times_salvaged+1 "
        "WHERE game_id=? AND salvageable_id=?",
        {game_id, salv_id});

    if (max_salvages > 0 && times_salvaged + 1 >= max_salvages)
    {
        db.Exec("UPDATE discovered_salvageables SET depleted=1 "
                "WHERE game_id=? AND salvageable_id=?",
                {game_id, salv_id});
        result << std::format(LC_SALVAGE_TARGET_DEPLEATED, salv_name);
    }

    // Log operation

    // BUGBUG the empty string for resources -- why are we inserting the empty
    // string into the record for resources?
    db.Exec("INSERT INTO salvage_operations(game_id,system_name,ship_code,turn,"
            "resources_found,hazard_encountered) VALUES(?,?,?,?,'{}',FALSE)",
            {game_id, ship.at_system, ship_code, s.round});

    Telemetry::instance().write(result.str());

    return true;
}
