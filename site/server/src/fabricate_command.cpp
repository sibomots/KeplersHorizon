//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "fabricate_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"

// Recipe definitions
struct Recipe
{
    const char* name;
    const char* description;
    int time_rounds;
    // Resource costs: Fe, Re, Rd, Cr, Vo, H2O, Or, Ex
    int cost[8];
    int output;
    const char* output_type; // "missiles", "tube", "beam", "screen", "tech"
};

static const Recipe RECIPES[] = {
    {"missiles",
     "Basic Missiles (x4)",
     1,
     {2, 0, 1, 0, 1, 0, 0, 0},
     4,
     "missiles"},
    {"adv_missiles",
     "Advanced Missiles (x4, +damage)",
     2,
     {2, 1, 2, 0, 2, 0, 0, 0},
     4,
     "missiles"},
    {"tubes",
     "Tube Upgrade (+1 capacity)",
     3,
     {5, 3, 0, 2, 0, 0, 0, 0},
     1,
     "tube"},
    {"beams",
     "Beam Upgrade (+1 rating)",
     3,
     {8, 4, 0, 3, 0, 0, 0, 0},
     1,
     "beam"},
    {"screens",
     "Screen Upgrade (+1 rating)",
     3,
     {6, 2, 0, 4, 0, 0, 0, 0},
     1,
     "screen"},
    {"tech",
     "Tech Research (+1 level)",
     5,
     {0, 10, 0, 5, 0, 0, 0, 2},
     1,
     "tech"},
};

static const int NUM_RECIPES = sizeof(RECIPES) / sizeof(RECIPES[0]);
static const char* RES_NAMES[] = {"Fe", "Re",  "Rd", "Cr",
                                  "Vo", "H2O", "Or", "Ex"};
static const char* CARGO_COLS[] = {"cargo_ferrous",     "cargo_rare_earth",
                                   "cargo_radioactive", "cargo_crystalline",
                                   "cargo_volatile",    "cargo_water",
                                   "cargo_organic",     "cargo_exotic"};

bool FabricateCommand::invoke(void)
{
    if (m_recipe.empty() || m_recipe == "list")
    {
        show_recipes();
        return true;
    }

    return do_fabricate();
}

void FabricateCommand::show_recipes()
{
    std::ostringstream out;
    out << "         FABRICATION RECIPES\n"
        << "-------------------------------------------\n";

    for (int i = 0; i < NUM_RECIPES; i++)
    {
        const Recipe& r = RECIPES[i];
        out << r.name << " - " << r.description << " [" << r.time_rounds
            << " rounds]\n";
        out << "  Cost: ";
        bool first = true;
        for (int j = 0; j < 8; j++)
        {
            if (r.cost[j] > 0)
            {
                if (!first)
                    out << ", ";
                out << r.cost[j] << " " << RES_NAMES[j];
                first = false;
            }
        }
        out << "\n";
    }

    out << "-------------------------------------------\n"
        << "Use: fabricate <recipe> [qty]\n"
        << "Requires ship at SHIPYARD or REFINERY.";
    Telemetry::getInstance().write(out.str());
}

bool FabricateCommand::do_fabricate()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Find recipe
    const Recipe* recipe = nullptr;
    for (int i = 0; i < NUM_RECIPES; i++)
    {
        if (m_recipe == RECIPES[i].name)
        {
            recipe = &RECIPES[i];
            break;
        }
    }

    if (!recipe)
    {
        Telemetry::getInstance().write("FABRICATE: Unknown recipe: " +
                                       m_recipe);
        return false;
    }

    // Check if player has ships at a facility
    auto ships = db.query(
        "SELECT s.ship_code, s.at_system FROM ships s "
        "WHERE s.game_id=" +
        std::to_string(game_id) + " AND s.owner='" + std::string(1, me) +
        "' AND s.destroyed_at IS NULL AND s.at_hex IS NOT NULL LIMIT 1");

    if (ships.empty())
    {
        Telemetry::getInstance().write(
            "FABRICATE: No ships deployed to fabrication sites.");
        return false;
    }

    std::string ship_code = ships[0][0];
    std::string at_system = ships[0][1];

    // TODO: Check if system has SHIPYARD or REFINERY facility

    // Calculate total resource costs
    int total_cost[8];
    for (int i = 0; i < 8; i++)
    {
        total_cost[i] = recipe->cost[i] * m_quantity;
    }

    // Check if player has enough resources across all ships
    std::ostringstream check_query;
    check_query << "SELECT ";
    for (int i = 0; i < 8; i++)
    {
        if (i > 0)
            check_query << ", ";
        check_query << "COALESCE(SUM(" << CARGO_COLS[i] << "),0)";
    }
    check_query << " FROM ships WHERE game_id=" << game_id << " AND owner='"
                << me << "' AND destroyed_at IS NULL";

    auto cargo = db.query(check_query.str());

    if (cargo.empty())
    {
        Telemetry::getInstance().write(
            "FABRICATE: Unable to check cargo inventory.");
        return false;
    }

    // Check each resource
    for (int i = 0; i < 8; i++)
    {
        int have = std::atoi(cargo[0][i].c_str());
        if (have < total_cost[i])
        {
            Telemetry::getInstance().write(
                "FABRICATE: Insufficient " + std::string(RES_NAMES[i]) +
                ". Need " + std::to_string(total_cost[i]) + ", have " +
                std::to_string(have));
            return false;
        }
    }

    // Deduct resources from ships (start from first ship with cargo)
    for (int res = 0; res < 8; res++)
    {
        if (total_cost[res] == 0)
            continue;

        int remaining = total_cost[res];
        auto res_ships = db.query(
            "SELECT ship_code, " + std::string(CARGO_COLS[res]) +
            " FROM ships WHERE game_id=" + std::to_string(game_id) +
            " AND owner='" + std::string(1, me) +
            "' AND destroyed_at IS NULL AND " + CARGO_COLS[res] + ">0");

        for (const auto& rs : res_ships)
        {
            if (remaining <= 0)
                break;
            int has = std::atoi(rs[1].c_str());
            int take = std::min(remaining, has);
            remaining -= take;

            db.exec("UPDATE ships SET " + std::string(CARGO_COLS[res]) + "=" +
                    CARGO_COLS[res] + "-" + std::to_string(take) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND owner='" + std::string(1, me) + "' AND ship_code='" +
                    db.esc(rs[0]) + "'");
        }
    }

    // Apply the output
    std::string result_msg;
    if (std::string(recipe->output_type) == "missiles")
    {
        int total_missiles = recipe->output * m_quantity;
        db.exec("UPDATE ships SET cargo_missiles=cargo_missiles+" +
                std::to_string(total_missiles) + " WHERE game_id=" +
                std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                "' AND ship_code='" + db.esc(ship_code) + "'");
        result_msg = "Fabricated " + std::to_string(total_missiles) +
                     " missiles, loaded to " + ship_code;
    }
    else
    {
        // Queue for later (ship upgrades take time)
        int completion = s.round + recipe->time_rounds;
        db.exec("INSERT INTO fabrication_queue(game_id,player,ship_code,recipe,"
                "quantity,started_turn,completion_turn,status) VALUES(" +
                std::to_string(game_id) + ",'" + std::string(1, me) + "','" +
                db.esc(ship_code) + "','" + db.esc(m_recipe) + "'," +
                std::to_string(m_quantity) + "," + std::to_string(s.round) +
                "," + std::to_string(completion) + ",'IN_PROGRESS')");
        result_msg = "Queued " + m_recipe + " x" + std::to_string(m_quantity) +
                     ". Completes round " + std::to_string(completion);
    }

    Logger::instance().info("FABRICATE: " + result_msg);
    Telemetry::getInstance().write("FABRICATE: " + result_msg);

    return true;
}
