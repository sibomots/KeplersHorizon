///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "extract_strategy.h"

#include "db.h"
#include "shipmgr.h"
#include "statemachine.h"

bool ExtractStrategy::do_scan(void)
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    // Get all ships and their locations
    auto ships = db.Query(
        "SELECT ship_code, at_system, at_hex FROM ships WHERE game_id=? "
        "AND owner=? AND destroyed_at IS NULL AND at_hex IS NOT NULL",
        {game_id, me});

    if (ships.empty())
    {
        Telemetry::instance().write(
            "No deployed ships available for extracting.");
        return false;
    }

    std::ostringstream out;
    out << "         HARVESTABLE RESOURCES\n";
    out << "-------------------------------------------\n";

    for (const auto& ship : ships)
    {
        std::string code = ship[0];
        std::string sys = ship[1];
        std::string hex = ship[2];

        if (sys.empty())
            continue;

        // Look up resources by system name via planets
        auto resources = db.Query(
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty, "
            "sp.common_name "
            "FROM system_resources sr "
            "JOIN system_planets sp ON sr.location_type='Planet' AND "
            "sr.location_id=sp.id "
            "WHERE sp.system_name=? "
            "UNION "
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty, "
            "sb.designation "
            "FROM system_resources sr "
            "JOIN system_asteroid_belts sb ON sr.location_type='Belt' AND "
            "sr.location_id=sb.id "
            "WHERE sb.system_name=?",
            {sys, sys});

        if (resources.empty())
        {
            out << code << " at " << sys << ": No resources\n";
        }
        else
        {
            out << code << " at " << sys << ":\n";
            for (const auto& r : resources)
            {
                out << "  " << r[0] << " (" << r[1] << "/" << r[2] << ") - "
                    << r[3] << "\n";
            }
        }
    }

    out << "-------------------------------------------\n";
    out << "Use: extract <ship> <resource_type>";
    Telemetry::instance().write(out.str());
    return true;
}

bool ExtractStrategy::do_extract(const std::string& ship_code,
                                 const std::string& resource)
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    // Verify ship exists and get location
    if (!ShipManager::instance().ship_exists(game_id, me, ship_code))
    {
        Telemetry::instance().write("FLEET REGISTRY: Vessel " + ship_code +
                                    " not found.");
        return false;
    }

    ShipRow ship;
    bool has_ship = ShipManager::instance().load_ship_by_code_or_name(
        ship, game_id, me, ship_code);

    if (!has_ship) // JDW  ship.at_system.empty())
    {
        Telemetry::instance().write(
            "HARVEST: Ship must be deployed to a system to extract.");
        return false;
    }

    // Look up resource in this system
    std::string res_upper = resource;
    for (auto& c : res_upper)
    {
        c = toupper(c);
    }
    auto res_check = db.Query(
        "SELECT sr.id, sr.abundance, sr.extraction_difficulty, sp.common_name "
        "FROM system_resources sr "
        "JOIN system_planets sp ON sr.location_type='Planet' AND "
        "sr.location_id=sp.id "
        "WHERE sp.system_name=? AND sr.resource_type=? LIMIT 1",
        {ship.at_system, res_upper});

    if (res_check.empty())
    {
        // Try asteroid belts
        res_check = db.Query(
            "SELECT sr.id, sr.abundance, sr.extraction_difficulty, "
            "sb.designation "
            "FROM system_resources sr "
            "JOIN system_asteroid_belts sb ON sr.location_type='Belt' AND "
            "sr.location_id=sb.id "
            "WHERE sb.system_name=? AND sr.resource_type=? LIMIT 1",
            {ship.at_system, res_upper});
    }

    if (res_check.empty())
    {
        Telemetry::instance().write("HARVEST: No " + res_upper +
                                    " deposits found in " + ship.at_system);
        return false;
    }

    // Calculate yield based on abundance and difficulty
    std::string abundance = res_check[0][1];
    std::string difficulty = res_check[0][2];
    std::string location = res_check[0][3];

    int base_yield = 1;
    if (KH_EQU(abundance, "Rich"))
    {
        base_yield = 16;
    }
    else if (KH_EQU(abundance, "High"))
    {
        base_yield = 8;
    }
    else if (KH_EQU(abundance, "Moderate"))
    {
        base_yield = 4;
    }
    else if (KH_EQU(abundance, "Low"))
    {
        base_yield = 2;
    }

    double modifier = 1.0;

    // BUGBUG why is this key a string, why are we comparing strings here??
    if (KH_EQU(difficulty, "Difficult"))
    {
        modifier = 0.4;
    }
    else if (KH_EQU(difficulty, "Moderate"))
    {
        modifier = 0.7;
    }
    else if (KH_EQU(difficulty, "Extreme"))
    {
        modifier = 0.2;
    }

    // Query knowledge level for yield modifier
    auto knowledge =
        db.Query("SELECT knowledge_level FROM codex_entries WHERE game_id=? "
                 "AND player=? AND system_name=?",
                 {game_id, me, ship.at_system});

    std::string know_level = knowledge.empty() ? "Unknown" : knowledge[0][0];
    double intel_mod = 1.0;

    // BUGBUG why are we still using strings to key this??
    if (KH_EQU(know_level, "Unknown"))
    {
        intel_mod = 0.25; // 25% yield without survey
    }
    else if (KH_EQU(know_level, "Charted"))
    {
        intel_mod = 0.50; // 50% yield with basic charts
    }
    // Surveyed/Intimate = 100%

    int yield = (int)(base_yield * modifier * intel_mod);
    if (yield < 1)
        yield = 1;

    // Apply dynamic hex event modifier (EXTRACTION_BONUS)
    yield +=
        HexEventEngine::get_extraction_modifier(game_id, s.round, ship.at_hex);

    // Check cargo capacity
    auto cargo =
        db.Query("SELECT cargo_ferrous+cargo_rare_earth+cargo_radioactive+"
                 "cargo_crystalline+cargo_volatile+cargo_water+cargo_organic+"
                 "cargo_exotic+cargo_missiles, cargo_capacity FROM ships WHERE "
                 "game_id=? AND owner=? AND ship_code=?",
                 {game_id, me, ship_code});

    int current_cargo = cargo.empty() ? 0 : std::atoi(cargo[0][0].c_str());
    int capacity = cargo.empty() ? 10 : std::atoi(cargo[0][1].c_str());

    if (current_cargo + yield > capacity)
    {
        yield = capacity - current_cargo;
        if (yield <= 0)
        {
            Telemetry::instance().write("HARVEST: " + ship.name +
                                        " cargo hold is full!");
            return false;
        }
    }

    // Map resource type to column
    std::string col = "cargo_ferrous";

    if (KH_EQU(res_upper, "RARE_EARTH"))
    {
        col = "cargo_rare_earth";
    }
    else if (KH_EQU(res_upper, "RADIOACTIVE"))
    {
        col = "cargo_radioactive";
    }
    else if (KH_EQU(res_upper, "CRYSTALLINE"))
    {
        col = "cargo_crystalline";
    }
    else if (KH_EQU(res_upper, "VOLATILE"))
    {
        col = "cargo_volatile";
    }
    else if (KH_EQU(res_upper, "WATER"))
    {
        col = "cargo_water";
    }
    else if (KH_EQU(res_upper, "ORGANIC"))
    {
        col = "cargo_organic";
    }
    else if (KH_EQU(res_upper, "EXOTIC"))
    {
        col = "cargo_exotic";
    }

    // Update cargo
    db.Exec("UPDATE ships SET " + col + "=" + col +
                "+? "
                "WHERE game_id=? AND owner=? AND ship_code=?",
            {yield, game_id, me, ship_code});

    // Log extract operation
    db.Exec(
        "INSERT INTO extract_operations(game_id,ship_code,owner,location_type,"
        "location_id,resource_type,started_turn,completed,yield) VALUES("
        "?,?,?,'Planet',0,?,?,1,?)",
        {game_id, ship_code, me, res_upper, s.round, yield});

    std::string msg = std::format("HARVEST: {} extracted {} units of {} from {} ({})", ship.name, yield, res_upper, location, ship.at_system);

    Telemetry::instance().write(msg);

    return true;
}
