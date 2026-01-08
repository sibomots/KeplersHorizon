//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "extract_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"
#include "hex_events.h"

bool ExtractCommand::invoke(void)
{
    if (m_scan_mode)
    {
        do_scan();
        return true;
    }

    if (m_ship_code.empty())
    {
        Telemetry::getInstance().write("Usage: extract scan\n"
                                       "       extract <ship> <resource>");
        return true;
    }

    return do_extract();
}

void ExtractCommand::do_scan()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Get all ships and their locations
    auto ships = db.query(
        "SELECT ship_code, at_system, at_hex FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, me) +
        "' AND destroyed_at IS NULL AND at_hex IS NOT NULL");

    if (ships.empty())
    {
        Telemetry::getInstance().write(
            "No deployed ships available for extracting.");
        return;
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
        auto resources = db.query(
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty, "
            "sp.common_name "
            "FROM system_resources sr "
            "JOIN system_planets sp ON sr.location_type='Planet' AND "
            "sr.location_id=sp.id "
            "WHERE sp.system_name='" +
            db.esc(sys) +
            "' "
            "UNION "
            "SELECT sr.resource_type, sr.abundance, sr.extraction_difficulty, "
            "sb.designation "
            "FROM system_resources sr "
            "JOIN system_asteroid_belts sb ON sr.location_type='Belt' AND "
            "sr.location_id=sb.id "
            "WHERE sb.system_name='" +
            db.esc(sys) + "'");

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
    Telemetry::getInstance().write(out.str());
}

bool ExtractCommand::do_extract()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Verify ship exists and get location
    if (!ship_exists(game_id, me, m_ship_code))
    {
        Telemetry::getInstance().write("FLEET REGISTRY: Vessel " + m_ship_code +
                                       " not found.");
        return false;
    }

    ShipRow ship = load_ship(game_id, me, m_ship_code);

    if (ship.at_system.empty())
    {
        Telemetry::getInstance().write(
            "HARVEST: Ship must be deployed to a system to extract.");
        return false;
    }

    // Look up resource in this system
    std::string res_upper = m_resource_type;
    for (auto& c : res_upper)
        c = toupper(c);

    auto res_check = db.query(
        "SELECT sr.id, sr.abundance, sr.extraction_difficulty, sp.common_name "
        "FROM system_resources sr "
        "JOIN system_planets sp ON sr.location_type='Planet' AND "
        "sr.location_id=sp.id "
        "WHERE sp.system_name='" +
        db.esc(ship.at_system) + "' AND sr.resource_type='" +
        db.esc(res_upper) + "' LIMIT 1");

    if (res_check.empty())
    {
        // Try asteroid belts
        res_check = db.query(
            "SELECT sr.id, sr.abundance, sr.extraction_difficulty, "
            "sb.designation "
            "FROM system_resources sr "
            "JOIN system_asteroid_belts sb ON sr.location_type='Belt' AND "
            "sr.location_id=sb.id "
            "WHERE sb.system_name='" +
            db.esc(ship.at_system) + "' AND sr.resource_type='" +
            db.esc(res_upper) + "' LIMIT 1");
    }

    if (res_check.empty())
    {
        Telemetry::getInstance().write("HARVEST: No " + res_upper +
                                       " deposits found in " + ship.at_system);
        return false;
    }

    // Calculate yield based on abundance and difficulty
    std::string abundance = res_check[0][1];
    std::string difficulty = res_check[0][2];
    std::string location = res_check[0][3];

    int base_yield = 1;
    if (abundance == "Rich")
        base_yield = 16;
    else if (abundance == "High")
        base_yield = 8;
    else if (abundance == "Moderate")
        base_yield = 4;
    else if (abundance == "Low")
        base_yield = 2;

    double modifier = 1.0;
    if (difficulty == "Difficult")
        modifier = 0.4;
    else if (difficulty == "Moderate")
        modifier = 0.7;
    else if (difficulty == "Extreme")
        modifier = 0.2;

    // Query knowledge level for yield modifier
    auto knowledge = db.query(
        "SELECT knowledge_level FROM grimoire_entries WHERE game_id=" +
        std::to_string(game_id) + " AND player='" + std::string(1, me) +
        "' AND system_name='" + db.esc(ship.at_system) + "'");

    std::string know_level = knowledge.empty() ? "Unknown" : knowledge[0][0];
    double intel_mod = 1.0;
    if (know_level == "Unknown")
        intel_mod = 0.25; // 25% yield without survey
    else if (know_level == "Charted")
        intel_mod = 0.50; // 50% yield with basic charts
    // Surveyed/Intimate = 100%

    int yield = (int)(base_yield * modifier * intel_mod);
    if (yield < 1)
        yield = 1;

    // Apply dynamic hex event modifier (EXTRACTION_BONUS)
    yield += HexEventEngine::get_extraction_modifier(game_id, ship.at_hex);

    // Check cargo capacity
    auto cargo =
        db.query("SELECT cargo_ferrous+cargo_rare_earth+cargo_radioactive+"
                 "cargo_crystalline+cargo_volatile+cargo_water+cargo_organic+"
                 "cargo_exotic+cargo_missiles, cargo_capacity FROM ships WHERE "
                 "game_id=" +
                 std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                 "' AND ship_code='" + db.esc(m_ship_code) + "'");

    int current_cargo = cargo.empty() ? 0 : std::atoi(cargo[0][0].c_str());
    int capacity = cargo.empty() ? 10 : std::atoi(cargo[0][1].c_str());

    if (current_cargo + yield > capacity)
    {
        yield = capacity - current_cargo;
        if (yield <= 0)
        {
            Telemetry::getInstance().write("HARVEST: " + ship.name +
                                           " cargo hold is full!");
            return false;
        }
    }

    // Map resource type to column
    std::string col = "cargo_ferrous";
    if (res_upper == "RARE_EARTH")
        col = "cargo_rare_earth";
    else if (res_upper == "RADIOACTIVE")
        col = "cargo_radioactive";
    else if (res_upper == "CRYSTALLINE")
        col = "cargo_crystalline";
    else if (res_upper == "VOLATILE")
        col = "cargo_volatile";
    else if (res_upper == "WATER")
        col = "cargo_water";
    else if (res_upper == "ORGANIC")
        col = "cargo_organic";
    else if (res_upper == "EXOTIC")
        col = "cargo_exotic";

    // Update cargo
    db.exec(
        "UPDATE ships SET " + col + "=" + col + "+" + std::to_string(yield) +
        " WHERE game_id=" + std::to_string(game_id) + " AND owner='" +
        std::string(1, me) + "' AND ship_code='" + db.esc(m_ship_code) + "'");

    // Log extract operation
    db.exec(
        "INSERT INTO extract_operations(game_id,ship_code,owner,location_type,"
        "location_id,resource_type,started_turn,completed,yield) VALUES(" +
        std::to_string(game_id) + ",'" + db.esc(m_ship_code) + "','" +
        std::string(1, me) + "','Planet',0,'" + db.esc(res_upper) + "'," +
        std::to_string(s.round) + ",1," + std::to_string(yield) + ")");

    std::ostringstream msg;
    msg << "HARVEST: " << ship.name << " extracted " << yield << " units of "
        << res_upper << " from " << location << " (" << ship.at_system << ")";

    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    return true;
}
