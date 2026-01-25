//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "market_command.h"

#include <cstdlib>
#include <sstream>

#include "db.h"
#include "typedefs.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

static const int NUM_RESOURCES = 8;
static const CommodityItem BASE_PRICES[] = {
    {"FERROUS", 5},      {"RARE_EARTH", 20}, {"RADIOACTIVE", 30},
    {"CRYSTALLINE", 25}, {"VOLATILE", 8},    {"WATER", 3},
    {"ORGANIC", 6},      {"EXOTIC", 100}};

bool MarketCommand::invoke(void)
{
    if (m_resource.empty())
    {
        show_all_prices();
    }
    else
    {
        show_price_history();
    }
    return true;
}

void MarketCommand::show_all_prices()
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();

    DatabaseManager& db = DatabaseManager::instance();

    std::ostringstream out;
    out << "          MARKET EXCHANGE (Round " << s.round << ")\n"
        << "-------------------------------------------\n"
        << "Resource      Price   Trend\n"
        << "----------    -----   -----\n";

    for (int i = 0; i < NUM_RESOURCES; i++)
    {
        std::string res = BASE_PRICES[i].type;

        // Check if market price exists for this game
        auto mp = db.query(
            "SELECT current_price, price_trend FROM market_prices "
            "WHERE game_id=" +
            std::to_string(game_id) + " AND resource_type='" + res + "'");

        int price;
        std::string trend_char = "─";

        if (mp.empty())
        {
            // Initialize market price
            price = BASE_PRICES[i].base_price;
            db.exec(
                "INSERT INTO market_prices(game_id,resource_type,current_price,"
                "base_price,price_trend,last_updated_turn) VALUES(" +
                std::to_string(game_id) + ",'" + res + "'," +
                std::to_string(price) + "," + std::to_string(price) +
                ",'STABLE'," + std::to_string(s.round) + ")");
        }
        else
        {
            price = std::atoi(mp[0][0].c_str());
            std::string trend = mp[0][1];
            if (trend == "RISING")
            {
                trend_char = "▲";
            }
            else if (trend == "FALLING")
            {
                trend_char = "▼";
            }
        }

        // Format output
        std::string name = res;
        if (name.length() < 12)
        {
            name += std::string(12 - name.length(), ' ');
        }

        out << name << "  " << price << " CR   " << trend_char << "\n";
    }

    out << "-------------------------------------------";
    Telemetry::instance().write(out.str());
}

void MarketCommand::show_price_history()
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();

    DatabaseManager& db = DatabaseManager::instance();

    std::string res_upper = m_resource;
    for (auto& c : res_upper)
        c = toupper(c);

    auto history =
        db.query("SELECT turn, price FROM market_history WHERE game_id=" +
                 std::to_string(game_id) + " AND resource_type='" +
                 db.esc(res_upper) + "' ORDER BY turn DESC LIMIT 10");

    std::ostringstream out;
    out << "     " << res_upper << " PRICE HISTORY\n"
        << "-------------------------------------------\n";

    if (history.empty())
    {
        out << "No trading history for " << res_upper << "\n";
    }
    else
    {
        out << "Round   Price\n"
            << "-----   -----\n";
        for (const auto& row : history)
        {
            out << row[0] << "       " << row[1] << " CR\n";
        }
    }

    out << "-------------------------------------------";
    Telemetry::instance().write(out.str());
}

// Called at end of each full turn (both players done)
void update_market_prices(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    for (int i = 0; i < NUM_RESOURCES; i++)
    {
        std::string res = BASE_PRICES[i].type;
        int base = BASE_PRICES[i].base_price;

        auto mp = db.query(
            "SELECT current_price, total_bought, total_sold, base_price "
            "FROM market_prices WHERE game_id=" +
            std::to_string(game_id) + " AND resource_type='" + res + "'");

        if (mp.empty())
        {
            continue;
        }

        int current = std::atoi(mp[0][0].c_str());
        int bought = std::atoi(mp[0][1].c_str());
        int sold = std::atoi(mp[0][2].c_str());
        int base_price = std::atoi(mp[0][3].c_str());

        // Calculate supply/demand adjustment
        int demand_pressure = bought - sold; // positive = more buying

        // Synthesize market movement (random factor)
        int roll = rand() % 6 + 1;

        std::string new_trend = "STABLE";
        int price_change = 0;

        // Player demand affects trend
        if (demand_pressure > 5)
        {
            new_trend = "RISING";
            price_change = 1 + (demand_pressure / 10);
        }
        else if (demand_pressure < -5)
        {
            new_trend = "FALLING";
            price_change = -1 - ((-demand_pressure) / 10);
        }
        else
        {
            // Natural market fluctuation
            if (roll == 6)
            {
                new_trend = "RISING";
                price_change = 1;
            }
            else if (roll == 1)
            {
                new_trend = "FALLING";
                price_change = -1;
            }
        }

        int new_price = current + price_change;

        // Clamp to 50%-200% of base
        int min_price = base_price / 2;
        int max_price = base_price * 2;

        if (new_price < min_price)
        {
            new_price = min_price;
        }
        if (new_price > max_price)
        {
            new_price = max_price;
        }

        // Update market
        db.exec("UPDATE market_prices SET current_price=" +
                std::to_string(new_price) + ", price_trend='" + new_trend +
                "', total_bought=0, total_sold=0, last_updated_turn=" +
                std::to_string(round) + " WHERE game_id=" +
                std::to_string(game_id) + " AND resource_type='" + res + "'");

        // Record history
        db.exec("INSERT INTO market_history(game_id,resource_type,price,turn) "
                "VALUES(" +
                std::to_string(game_id) + ",'" + res + "'," +
                std::to_string(new_price) + "," + std::to_string(round) + ")");
    }

    Logger::instance().info("[MARKET] Prices updated for game " +
                            std::to_string(game_id) + " round " +
                            std::to_string(round));
}
