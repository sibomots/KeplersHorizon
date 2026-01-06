//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "trade_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "ships.h"
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
static int get_market_price(int game_id, const std::string& res)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto mp = db.query(
        "SELECT current_price FROM market_prices WHERE game_id=" +
        std::to_string(game_id) + " AND resource_type='" + db.esc(res) + "'");

    if (!mp.empty())
    {
        return std::atoi(mp[0][0].c_str());
    }

    // Fallback to base price
    for (int i = 0; i < NUM_PRICES; i++)
    {
        if (res == PRICES[i].type)
            return PRICES[i].buy_price;
    }
    return 0;
}

// Track trade for market dynamics
static void track_trade(int game_id, const std::string& res, int qty,
                        bool is_buy)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string col = is_buy ? "total_bought" : "total_sold";

    db.exec("UPDATE market_prices SET " + col + "=" + col + "+" +
            std::to_string(qty) + " WHERE game_id=" + std::to_string(game_id) +
            " AND resource_type='" + db.esc(res) + "'");
}

static std::string get_cargo_column(const std::string& res)
{
    if (res == "FERROUS")
        return "cargo_ferrous";
    if (res == "RARE_EARTH")
        return "cargo_rare_earth";
    if (res == "RADIOACTIVE")
        return "cargo_radioactive";
    if (res == "CRYSTALLINE")
        return "cargo_crystalline";
    if (res == "VOLATILE")
        return "cargo_volatile";
    if (res == "WATER")
        return "cargo_water";
    if (res == "ORGANIC")
        return "cargo_organic";
    if (res == "EXOTIC")
        return "cargo_exotic";
    return "";
}

bool TradeCommand::invoke(void)
{
    switch (m_mode)
    {
    case MODE_LIST:
        show_prices();
        return true;
    case MODE_BUY:
        return do_buy();
    case MODE_SELL:
        return do_sell();
    case MODE_TRANSFER:
        return do_transfer();
    }
    return true;
}

void TradeCommand::show_prices()
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
    Telemetry::getInstance().write(out.str());
}

bool TradeCommand::do_buy()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    std::string res_upper = m_resource;
    for (auto& c : res_upper)
        c = toupper(c);

    int price = get_market_price(game_id, res_upper);
    if (price == 0)
    {
        Telemetry::getInstance().write("TRADE: Unknown resource type: " +
                                       m_resource);
        return false;
    }

    int total_cost = price * m_quantity;
    int my_credits = (me == 'A') ? s.creditsA : s.creditsB;

    if (total_cost > my_credits)
    {
        Telemetry::getInstance().write(
            "TRADE: Insufficient credits. Need " + std::to_string(total_cost) +
            " CR, have " + std::to_string(my_credits) + " CR");
        return false;
    }

    // Find a ship at a trade hub to receive cargo
    auto ships = db.query(
        "SELECT s.ship_code, s.ship_name, s.at_system FROM ships s "
        "JOIN star_systems ss ON s.at_hex = ss.hex_id AND ss.map_id=1 "
        "WHERE s.game_id=" +
        std::to_string(game_id) + " AND s.owner='" + std::string(1, me) +
        "' AND s.destroyed_at IS NULL AND s.at_hex IS NOT NULL LIMIT 1");

    if (ships.empty())
    {
        Telemetry::getInstance().write(
            "TRADE: No ships available to receive cargo.");
        return false;
    }

    std::string ship_code = ships[0][0];
    std::string ship_name = ships[0][1];
    std::string col = get_cargo_column(res_upper);

    // Update credits
    if (me == 'A')
        s.creditsA -= total_cost;
    else
        s.creditsB -= total_cost;

    // Update cargo
    db.exec("UPDATE ships SET " + col + "=" + col + "+" +
            std::to_string(m_quantity) + " WHERE game_id=" +
            std::to_string(game_id) + " AND owner='" + std::string(1, me) +
            "' AND ship_code='" + db.esc(ship_code) + "'");

    StateMachine::getInstance().save_game(s);

    // Track for market dynamics
    track_trade(game_id, res_upper, m_quantity, true);

    std::ostringstream msg;
    msg << "TRADE: Purchased " << m_quantity << " " << res_upper << " for "
        << total_cost << " CR. Loaded onto " << ship_name;
    Telemetry::getInstance().write(msg.str());
    return true;
}

bool TradeCommand::do_sell()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    std::string res_upper = m_resource;
    for (auto& c : res_upper)
        c = toupper(c);

    // Sell price is 75% of market price
    int market = get_market_price(game_id, res_upper);
    int price = (market * 3) / 4;  // 75% of market
    if (price == 0)
    {
        Telemetry::getInstance().write("TRADE: Unknown resource type: " +
                                       m_resource);
        return false;
    }

    std::string col = get_cargo_column(res_upper);

    // Find total of this resource across all ships
    auto cargo = db.query("SELECT COALESCE(SUM(" + col + "),0) FROM ships WHERE "
                          "game_id=" +
                          std::to_string(game_id) + " AND owner='" +
                          std::string(1, me) + "' AND destroyed_at IS NULL");

    int available = cargo.empty() ? 0 : std::atoi(cargo[0][0].c_str());

    if (m_quantity > available)
    {
        Telemetry::getInstance().write(
            "TRADE: Insufficient " + res_upper + ". Have " +
            std::to_string(available) + ", need " + std::to_string(m_quantity));
        return false;
    }

    int total_revenue = price * m_quantity;

    // Deduct from first ship that has cargo
    int remaining = m_quantity;
    auto ships = db.query("SELECT ship_code, " + col +
                          " FROM ships WHERE game_id=" +
                          std::to_string(game_id) + " AND owner='" +
                          std::string(1, me) +
                          "' AND destroyed_at IS NULL AND " + col + ">0");

    for (const auto& ship : ships)
    {
        if (remaining <= 0)
            break;
        int has = std::atoi(ship[1].c_str());
        int take = std::min(remaining, has);
        remaining -= take;

        db.exec("UPDATE ships SET " + col + "=" + col + "-" +
                std::to_string(take) + " WHERE game_id=" +
                std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                "' AND ship_code='" + db.esc(ship[0]) + "'");
    }

    // Add credits
    if (me == 'A')
        s.creditsA += total_revenue;
    else
        s.creditsB += total_revenue;

    StateMachine::getInstance().save_game(s);

    // Track for market dynamics
    track_trade(game_id, res_upper, m_quantity, false);

    std::ostringstream msg;
    msg << "TRADE: Sold " << m_quantity << " " << res_upper << " for "
        << total_revenue << " CR";
    Telemetry::getInstance().write(msg.str());
    return true;
}

bool TradeCommand::do_transfer()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    if (m_from_ship.empty() || m_to_ship.empty())
    {
        Telemetry::getInstance().write(
            "Usage: trade transfer <from_ship> <to_ship> <resource> <qty>");
        return false;
    }

    std::string res_upper = m_resource;
    for (auto& c : res_upper)
        c = toupper(c);

    std::string col = get_cargo_column(res_upper);
    if (col.empty())
    {
        Telemetry::getInstance().write("TRADE: Unknown resource: " + m_resource);
        return false;
    }

    // Check source ship cargo
    auto src =
        db.query("SELECT " + col + " FROM ships WHERE game_id=" +
                 std::to_string(game_id) + " AND owner='" + std::string(1, me) +
                 "' AND ship_code='" + db.esc(m_from_ship) + "'");

    if (src.empty())
    {
        Telemetry::getInstance().write("TRADE: Ship " + m_from_ship + " not found.");
        return false;
    }

    int has = std::atoi(src[0][0].c_str());
    if (has < m_quantity)
    {
        Telemetry::getInstance().write(
            "TRADE: " + m_from_ship + " only has " + std::to_string(has) + " " +
            res_upper);
        return false;
    }

    // Transfer
    db.exec("UPDATE ships SET " + col + "=" + col + "-" +
            std::to_string(m_quantity) + " WHERE game_id=" +
            std::to_string(game_id) + " AND owner='" + std::string(1, me) +
            "' AND ship_code='" + db.esc(m_from_ship) + "'");

    db.exec("UPDATE ships SET " + col + "=" + col + "+" +
            std::to_string(m_quantity) + " WHERE game_id=" +
            std::to_string(game_id) + " AND owner='" + std::string(1, me) +
            "' AND ship_code='" + db.esc(m_to_ship) + "'");

    std::ostringstream msg;
    msg << "TRADE: Transferred " << m_quantity << " " << res_upper << " from "
        << m_from_ship << " to " << m_to_ship;
    Telemetry::getInstance().write(msg.str());
    return true;
}
