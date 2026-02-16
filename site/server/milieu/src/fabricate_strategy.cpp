///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "fabricate_strategy.h"

#include <sstream>

#include "db.h"
#include "fabricate_modes.h"
#include "facilities.h"
#include "logger.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

static const char* RES_NAMES[8] = {"Fe", "Re",  "Rd", "Cr",
                                   "Vo", "H2O", "Or", "Ex"};
static const char* CARGO_COLS[8] = {"cargo_ferrous",     "cargo_rare_earth",
                                    "cargo_radioactive", "cargo_crystalline",
                                    "cargo_volatile",    "cargo_water",
                                    "cargo_organic",     "cargo_exotic"};

static const char* COST_COLS[8] = {
    "cost_ferrous",  "cost_rare_earth", "cost_radioactive", "cost_crystalline",
    "cost_volatile", "cost_water",      "cost_organic",     "cost_exotic"};

bool FabricateStrategy::show_plans(int game_id, int module_id)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    std::string sql =
        "SELECT name, description, build_time, output_type, output_qty, "
        "cost_ferrous, cost_rare_earth, cost_radioactive, cost_crystalline, "
        "cost_volatile, cost_water, cost_organic, cost_exotic "
        "FROM fabrication_plan WHERE module_id=?";

    auto plans = db.Query(sql, {module_id});

    std::ostringstream out;
    out << "                    FABRICATION PLANS\n";
    out << "───────────────────────────────────────────────────────────\n";
    out << std::format("{:<10} {:<8} {:<5} {}\n", "Plan", "Output", "Rnd",
                       "Cost");
    out << std::format("{:<10} {:<8} {:<5} {}\n", "────────", "──────", "───",
                       "──────────────────────");

    for (const auto& row : plans)
    {
        std::string output =
            row[3] + " x" + row[4];

        std::string cost_str;
        for (int i = 0; i < 8; i++)
        {
            int cost = std::atoi(row[5 + i].c_str());
            if (cost > 0)
            {
                if (!cost_str.empty())
                {
                    cost_str += ", ";
                }
                cost_str += std::to_string(cost) + " " + RES_NAMES[i];
            }
        }

        out << std::format("{:<10} {:<8} {:<5} {}\n", row[0], output, row[2],
                           cost_str);
    }
    out << "───────────────────────────────────────────────────────────\n";
    out << "Use: fabricate <plan> [qty]";

    Telemetry::instance().write(out.str());
    bres = true;

    return bres;
}

bool FabricateStrategy::check_cargo_cost(int game_id, char owner, int cost[8],
                                         int qty)
{
    bool bres = true;
    DatabaseManager& db = DatabaseManager::instance();

    std::string check_query;
    check_query.append("SELECT ");
    for (int i = 0; i < 8; i++)
    {
        if (i > 0)
        {
            check_query.append(", ");
        }
        check_query.append("COALESCE(SUM(");
        check_query.append(CARGO_COLS[i]);
        check_query.append("),0)");
    }
    check_query.append(
        " FROM ships WHERE game_id=? AND owner=? AND destroyed_at IS NULL");

    auto cargo = db.Query(check_query, {game_id, owner});

    if (cargo.empty())
    {
        Telemetry::instance().write(
            "FABRICATE: Unable to check cargo inventory.");
        bres = false;
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            int have = std::atoi(cargo[0][i].c_str());
            int need = cost[i] * qty;
            if (have < need)
            {
                Telemetry::instance().write("FABRICATE: Insufficient " +
                                            std::string(RES_NAMES[i]) +
                                            ". Need " + std::to_string(need) +
                                            ", have " + std::to_string(have));
                bres = false;
            }
        }
    }

    return bres;
}

bool FabricateStrategy::deduct_cargo(int game_id, char owner, int cost[8],
                                     int qty)
{
    bool bres = true;
    DatabaseManager& db = DatabaseManager::instance();

    for (int res = 0; res < 8; res++)
    {
        int total_needed = cost[res] * qty;
        if (KH_EQU(total_needed, 0))
        {
            continue;
        }

        int remaining = total_needed;
        auto res_ships =
            db.Query("SELECT ship_code, " + std::string(CARGO_COLS[res]) +
                         " FROM ships WHERE game_id=? AND owner=? "
                         "AND destroyed_at IS NULL AND " +
                         CARGO_COLS[res] + ">0",
                     {game_id, owner});

        for (const auto& rs : res_ships)
        {
            if (remaining <= 0)
            {
                break;
            }
            int has = std::atoi(rs[1].c_str());
            int take = std::min(remaining, has);
            remaining -= take;

            db.Exec("UPDATE ships SET " + std::string(CARGO_COLS[res]) + "=" +
                        CARGO_COLS[res] +
                        "-? WHERE game_id=? AND owner=? "
                        "AND ship_code=?",
                    {take, game_id, owner, rs[0]});
        }
    }

    return bres;
}

// Helper: load cost array from fabrication_plan for a given output_type
static bool load_plan_costs(const char* output_type, int cost[8],
                            int& build_time, int& output_qty)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    auto result = db.Query(
        "SELECT build_time, output_qty, "
        "cost_ferrous, cost_rare_earth, cost_radioactive, cost_crystalline, "
        "cost_volatile, cost_water, cost_organic, cost_exotic "
        "FROM fabrication_plan WHERE output_type=?",
        {output_type});

    if (!result.empty())
    {
        build_time = std::atoi(result[0][0].c_str());
        output_qty = std::atoi(result[0][1].c_str());
        for (int i = 0; i < 8; i++)
        {
            cost[i] = std::atoi(result[0][2 + i].c_str());
        }
        bres = true;
    }
    else
    {
        Telemetry::instance().write("FABRICATE: Plan not found in database.");
    }

    return bres;
}

// Helper: find a player ship at SHIPYARD or REFINERY
static bool find_fabrication_ship(int game_id, char owner,
                                  std::string& ship_code,
                                  std::string& at_system)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    auto ships =
        db.Query("SELECT s.ship_code, s.at_system FROM ships s "
                 "WHERE s.game_id=? AND s.owner=? "
                 "AND s.destroyed_at IS NULL AND s.at_hex IS NOT NULL LIMIT 1",
                 {game_id, owner});

    if (ships.empty())
    {
        Telemetry::instance().write(
            "FABRICATE: No ships deployed to fabrication sites.");
    }
    else
    {
        ship_code = ships[0][0];
        at_system = ships[0][1];

        bool hasShipyard = FacilityEngine::player_controls(game_id, at_system,
                                                           "SHIPYARD", owner);
        bool hasRefinery = FacilityEngine::player_controls(game_id, at_system,
                                                           "REFINERY", owner);

        if (hasShipyard || hasRefinery)
        {
            bres = true;
        }
        else
        {
            Telemetry::instance().write("FABRICATE: Ship must be at a SHIPYARD "
                                        "or REFINERY you control.");
        }
    }

    return bres;
}

bool FabricateStrategy::fabricate_missile(int game_id, char owner, int qty)
{
    bool bres = false;
    int cost[8] = {0};
    int build_time = 0;
    int output_qty = 0;
    std::string ship_code;
    std::string at_system;

    if (load_plan_costs("MISSILE", cost, build_time, output_qty) &&
        find_fabrication_ship(game_id, owner, ship_code, at_system) &&
        check_cargo_cost(game_id, owner, cost, qty))
    {
        deduct_cargo(game_id, owner, cost, qty);
        DatabaseManager& db = DatabaseManager::instance();

        // Missiles are immediate output
        int total_missiles = output_qty * qty;
        db.Exec("UPDATE ships SET cargo_missiles=cargo_missiles+? "
                "WHERE game_id=? AND owner=? AND ship_code=?",
                {total_missiles, game_id, owner, ship_code});

        std::string msg = "Fabricated " + std::to_string(total_missiles) +
                          " missiles, loaded to " + ship_code;
        Telemetry::instance().write("FABRICATE: " + msg);
        bres = true;
    }

    return bres;
}

bool FabricateStrategy::fabricate_tube(int game_id, char owner, int qty)
{
    bool bres = false;
    int cost[8] = {0};
    int build_time = 0;
    int output_qty = 0;
    std::string ship_code;
    std::string at_system;

    if (load_plan_costs("TUBE", cost, build_time, output_qty) &&
        find_fabrication_ship(game_id, owner, ship_code, at_system) &&
        check_cargo_cost(game_id, owner, cost, qty))
    {
        deduct_cargo(game_id, owner, cost, qty);
        DatabaseManager& db = DatabaseManager::instance();
        GameState s = StateMachine::instance().get_game_state();
        int completion = s.round + build_time;

        db.Exec("INSERT INTO fabrication_queue"
                "(game_id,player,ship_code,recipe,"
                "quantity,started_turn,completion_turn,status) VALUES("
                "?,?,?,?,?,?,?,'IN_PROGRESS')",
                {game_id, owner, ship_code, "tubes", qty, s.round, completion});

        std::string msg = "Queued tube upgrade x" + std::to_string(qty) +
                          ". Completes round " + std::to_string(completion);
        Telemetry::instance().write("FABRICATE: " + msg);
        bres = true;
    }

    return bres;
}

bool FabricateStrategy::fabricate_beam(int game_id, char owner, int qty)
{
    bool bres = false;
    int cost[8] = {0};
    int build_time = 0;
    int output_qty = 0;
    std::string ship_code;
    std::string at_system;

    if (load_plan_costs("BEAM", cost, build_time, output_qty) &&
        find_fabrication_ship(game_id, owner, ship_code, at_system) &&
        check_cargo_cost(game_id, owner, cost, qty))
    {
        deduct_cargo(game_id, owner, cost, qty);
        DatabaseManager& db = DatabaseManager::instance();
        GameState s = StateMachine::instance().get_game_state();
        int completion = s.round + build_time;

        db.Exec("INSERT INTO fabrication_queue"
                "(game_id,player,ship_code,recipe,"
                "quantity,started_turn,completion_turn,status) VALUES("
                "?,?,?,?,?,?,?,'IN_PROGRESS')",
                {game_id, owner, ship_code, "beams", qty, s.round, completion});

        std::string msg = "Queued beam upgrade x" + std::to_string(qty) +
                          ". Completes round " + std::to_string(completion);
        Telemetry::instance().write("FABRICATE: " + msg);
        bres = true;
    }

    return bres;
}

bool FabricateStrategy::fabricate_screen(int game_id, char owner, int qty)
{
    bool bres = false;
    int cost[8] = {0};
    int build_time = 0;
    int output_qty = 0;
    std::string ship_code;
    std::string at_system;

    if (load_plan_costs("SCREEN", cost, build_time, output_qty) &&
        find_fabrication_ship(game_id, owner, ship_code, at_system) &&
        check_cargo_cost(game_id, owner, cost, qty))
    {
        deduct_cargo(game_id, owner, cost, qty);
        DatabaseManager& db = DatabaseManager::instance();
        GameState s = StateMachine::instance().get_game_state();
        int completion = s.round + build_time;

        db.Exec(
            "INSERT INTO fabrication_queue"
            "(game_id,player,ship_code,recipe,"
            "quantity,started_turn,completion_turn,status) VALUES("
            "?,?,?,?,?,?,?,'IN_PROGRESS')",
            {game_id, owner, ship_code, "screens", qty, s.round, completion});

        std::string msg = "Queued screen upgrade x" + std::to_string(qty) +
                          ". Completes round " + std::to_string(completion);
        Telemetry::instance().write("FABRICATE: " + msg);
        bres = true;
    }

    return bres;
}

bool FabricateStrategy::fabricate_tech(int game_id, char owner, int qty)
{
    bool bres = false;
    int cost[8] = {0};
    int build_time = 0;
    int output_qty = 0;
    std::string ship_code;
    std::string at_system;

    // Tech does not require SHIPYARD, but still needs a ship somewhere
    if (load_plan_costs("TECH", cost, build_time, output_qty) &&
        find_fabrication_ship(game_id, owner, ship_code, at_system) &&
        check_cargo_cost(game_id, owner, cost, qty))
    {
        deduct_cargo(game_id, owner, cost, qty);
        DatabaseManager& db = DatabaseManager::instance();
        GameState s = StateMachine::instance().get_game_state();
        int completion = s.round + build_time;

        db.Exec("INSERT INTO fabrication_queue"
                "(game_id,player,ship_code,recipe,"
                "quantity,started_turn,completion_turn,status) VALUES("
                "?,?,?,?,?,?,?,'IN_PROGRESS')",
                {game_id, owner, ship_code, "tech", qty, s.round, completion});

        std::string msg = "Queued tech research x" + std::to_string(qty) +
                          ". Completes round " + std::to_string(completion);
        Telemetry::instance().write("FABRICATE: " + msg);
        bres = true;
    }

    return bres;
}

