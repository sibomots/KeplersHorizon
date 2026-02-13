///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "trade_strategy.h"

#include <format>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "moduleutil.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

// Base prices (CR per unit)
static const struct
{
    const char* type;
    int buy_price;
    int sell_price;
} PRICES[] = {{"FERROUS", 5, 3},       {"RARE_EARTH", 20, 15},
              {"RADIOACTIVE", 30, 22}, {"CRYSTALLINE", 25, 18},
              {"VOLATILE", 8, 5},      {"WATER", 3, 2},
              {"ORGANIC", 6, 4},       {"EXOTIC", 100, 75}};

static const int NUM_PRICES = sizeof(PRICES) / sizeof(PRICES[0]);

// Get current market price (from database if available)
int TradeStrategy::get_market_price(int game_id, const std::string& res)
{
    DatabaseManager& db = DatabaseManager::instance();
    auto mp =
        db.Query("SELECT current_price FROM market_prices WHERE game_id=? "
                 "AND resource_type=?",
                 {game_id, res});

    if (!mp.empty())
    {
        return std::atoi(mp[0][0].c_str());
    }

    // Fallback to base price
    for (int i = 0; i < NUM_PRICES; i++)
    {
        if (KH_EQU(res, PRICES[i].type))
        {
            return PRICES[i].buy_price;
        }
    }
    return 0;
}

// Track trade for market dynamics
void TradeStrategy::track_trade(int game_id, const std::string& res, int qty,
                                bool is_buy)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string col = is_buy ? "total_bought" : "total_sold";

    db.Exec("UPDATE market_prices SET " + col + "=" + col +
                "+? "
                "WHERE game_id=? AND resource_type=?",
            {qty, game_id, res});
}

std::string TradeStrategy::get_cargo_column(const std::string& res)
{
    if (KH_EQU(res, "FERROUS"))
    {
        return "cargo_ferrous";
    }
    else if (KH_EQU(res, "RARE_EARTH"))
    {
        return "cargo_rare_earth";
    }
    else if (KH_EQU(res, "RADIOACTIVE"))
    {
        return "cargo_radioactive";
    }
    else if (KH_EQU(res, "CRYSTALLINE"))
    {
        return "cargo_crystalline";
    }
    else if (KH_EQU(res, "VOLATILE"))
    {
        return "cargo_volatile";
    }
    else if (KH_EQU(res, "WATER"))
    {
        return "cargo_water";
    }
    else if (KH_EQU(res, "ORGANIC"))
    {
        return "cargo_organic";
    }
    else if (KH_EQU(res, "EXOTIC"))
    {
        return "cargo_exotic";
    }
    else {
    return "";
    }
}

bool TradeStrategy::show_prices(void)
{
    std::ostringstream out;
    out << "         TRADE EXCHANGE RATES\n";
    out << "-------------------------------------------\n";
    out << "Resource      Buy    Sell\n";
    out << "----------    ---    ----\n";

    for (int i = 0; i < NUM_PRICES; i++)
    {
        std::string name = PRICES[i].type;
        if (name.length() < 12)
            name += std::string(12 - name.length(), ' ');
        out << name << "  " << PRICES[i].buy_price << " CR   "
            << PRICES[i].sell_price << " CR\n";
    }

    out << "-------------------------------------------\n";
    out << "Trade requires ship at TRADE_HUB facility.";
    Telemetry::instance().write(out.str());
    return true;
}

bool TradeStrategy::do_buy(const std::string& resource, const int qty)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    std::string res_upper = resource;
    for (auto& c : res_upper)
    {
        c = toupper(c);
    }

    int price = get_market_price(game_id, res_upper);
    if (KH_EQU(price, 0))
    {
        Telemetry::instance().write("TRADE: Unknown resource type: " +
                                    resource);
        bres = false;
    }
    else
    {
        int total_cost = price * qty;
        int my_credits = (KH_EQU(me, 'A')) ? s.creditsA : s.creditsB;

        if (total_cost > my_credits)
        {
            Telemetry::instance().write("TRADE: Insufficient credits. Need " +
                                        std::to_string(total_cost) +
                                        " CR, have " +
                                        std::to_string(my_credits) + " CR");
            bres = false;
        }
        else
        {
            // Find a ship at a trade hub to receive cargo
            int mod = get_module_id_for_game(game_id);
            auto ships = db.Query(
                "SELECT s.ship_code, s.ship_name, s.at_system FROM ships s "
                "JOIN star_systems ss ON s.at_hex = ss.hex_id AND "
                "ss.module_id=? "
                "WHERE s.game_id=? AND s.owner=? AND s.destroyed_at IS NULL "
                "AND s.at_hex IS NOT NULL LIMIT 1",
                {mod, game_id, me});

            if (!ships.empty())
            {
                std::string ship_code = ships[0][0];
                std::string ship_name = ships[0][1];
                std::string col = get_cargo_column(res_upper);

                // Update credits
                if (KH_EQU(me, 'A'))
                {
                    s.creditsA -= total_cost;
                }
                else
                {
                    s.creditsB -= total_cost;
                }

                // Update cargo
                db.Exec("UPDATE ships SET " + col + "=" + col +
                            "+? "
                            "WHERE game_id=? AND owner=? AND ship_code=?",
                        {qty, game_id, me, ship_code});

                StateMachine::instance().save_game(s);

                // Track for market dynamics
                track_trade(game_id, res_upper, qty, true);

                std::ostringstream msg;
                msg << "TRADE: Purchased " << qty << " " << res_upper << " for "
                    << total_cost << " CR. Loaded onto " << ship_name;
                Telemetry::instance().write(msg.str());
                bres = true;
            }
            else
            {
                Telemetry::instance().write(
                    "TRADE: No ships available to receive cargo.");
                bres = false;
            }
        }
    }
    return bres;
}

bool TradeStrategy::do_sell(const std::string& resource, const int qty)
{
    bool bres = false;

    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    std::string res_upper = resource;
    for (auto& c : res_upper)
    {
        c = toupper(c);
    }

    // Sell price is 75% of market price
    int market = get_market_price(game_id, res_upper);
    int price = (market * 3) / 4; // 75% of market
    if (KH_EQU(price, 0))
    {
        Telemetry::instance().write("TRADE: Unknown resource type: " +
                                    resource);
        bres = false;
    }
    else
    {
        std::string col = get_cargo_column(res_upper);

        // Find total of this resource across all ships
        auto cargo =
            db.Query("SELECT COALESCE(SUM(" + col +
                         "),0) FROM ships "
                         "WHERE game_id=? AND owner=? AND destroyed_at IS NULL",
                     {game_id, me});

        int available = cargo.empty() ? 0 : std::atoi(cargo[0][0].c_str());

        if (qty > available)
        {
            Telemetry::instance().write("TRADE: Insufficient " + res_upper +
                                        ". Have " + std::to_string(available) +
                                        ", need " + std::to_string(qty));
            bres = false;
        }
        else
        {
            int total_revenue = price * qty;

            // Deduct from first ship that has cargo
            int remaining = qty;
            auto ships =
                db.Query("SELECT ship_code, " + col +
                             " FROM ships WHERE game_id=? "
                             "AND owner=? AND destroyed_at IS NULL AND " +
                             col + ">0",
                         {game_id, me});

            for (const auto& ship : ships)
            {
                if (remaining <= 0)
                {
                    break;
                }
                int has = std::atoi(ship[1].c_str());
                int take = std::min(remaining, has);
                remaining -= take;

                db.Exec("UPDATE ships SET " + col + "=" + col +
                            "-? "
                            "WHERE game_id=? AND owner=? AND ship_code=?",
                        {take, game_id, me, ship[0]});
            }

            // Add credits
            if (KH_EQU(me, 'A'))
            {
                s.creditsA += total_revenue;
            }
            else
            {
                s.creditsB += total_revenue;
            }

            StateMachine::instance().save_game(s);

            // Track for market dynamics
            track_trade(game_id, res_upper, qty, false);

            std::string msg = std::format("TRADE: Sold {} {} for {} CR",
              qty, res_upper, total_revenue);

            Telemetry::instance().write(msg);
            bres = true;
        }
    }
    return bres;
}

bool TradeStrategy::do_transfer(const std::string& resource, const int qty,
                                const std::string& srcship,
                                const std::string& destship)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    DatabaseManager& db = DatabaseManager::instance();

    std::string res_upper = resource;
    for (auto& c : res_upper)
    {
        c = toupper(c);
    }

    std::string col = get_cargo_column(res_upper);
    if (col.empty())
    {
        Telemetry::instance().write("TRADE: Unknown resource: " + resource);
        bres = false;
    }
    else
    {
        // Check source ship cargo
        auto src = db.Query("SELECT " + col +
                                " FROM ships WHERE game_id=? "
                                "AND owner=? AND (ship_code=? OR ship_name=?)",
                            {game_id, me, srcship, srcship});

        if (!src.empty())
        {

            int has = std::atoi(src[0][0].c_str());
            if (has < qty)
            {
                Telemetry::instance().write(std::format(
                    "TRADE: {} only has {} {}", srcship, has, res_upper));
                bres = false;
            }
            else
            {
                // Transfer
                // transfer source and destination have to be different!

                db.Exec(
                    "UPDATE ships SET " + col + "=" + col +
                        "-? "
                        "WHERE game_id=? AND owner=? AND (ship_code=? OR "
                        "ship_name=?) "
                        " AND NOT (ship_code=? OR ship_name=?)",
                    {qty, game_id, me, srcship, srcship, destship, destship});

                db.Exec(
                    "UPDATE ships SET " + col + "=" + col +
                        "+? "
                        "WHERE game_id=? AND owner=? AND (ship_code=? OR "
                        "ship_name=?) "
                        " AND NOT (ship_code=? OR ship_name=?)",
                    {qty, game_id, me, destship, destship, srcship, srcship});

                std::string msg(
                    std::format("TRADE: Transferred {} {} from {} to {}", qty,
                                res_upper, srcship, destship));
                Telemetry::instance().write(msg);
                bres = true;
            }
        }
        else
        {
            Telemetry::instance().write("TRADE: Ship " + srcship +
                                        " not found.");
            bres = false;
        }
    }
    return bres;
}
