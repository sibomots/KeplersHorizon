///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "market_strategy.h"

#include <cstdlib>
#include <format>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

// Base prices live in market_base_prices (module-level, seeded in Game.sql).
// Per-game market_prices should be populated at game creation from
// market_base_prices. Price fluctuation is handled by TurnEndProcessor.
// If market_prices is empty, fall back to market_base_prices.

// Hardcoded fallback prices matching trade_strategy.cpp
static const struct
{
    const char* type;
    int price;
} BASE_PRICES[] = {{"FERROUS", 5},      {"RARE_EARTH", 20},
                   {"RADIOACTIVE", 30},  {"CRYSTALLINE", 25},
                   {"VOLATILE", 8},      {"WATER", 3},
                   {"ORGANIC", 6},       {"EXOTIC", 100}};

static const int NUM_BASE_PRICES =
    sizeof(BASE_PRICES) / sizeof(BASE_PRICES[0]);

// Display-friendly resource name (RARE_EARTH -> Rare Earth)
static std::string display_resource_name(const std::string& raw)
{
    std::string result;
    bool cap_next = true;
    for (size_t i = 0; i < raw.size(); i++)
    {
        if (raw[i] == '_')
        {
            result += ' ';
            cap_next = true;
        }
        else if (cap_next)
        {
            result += (char)toupper(raw[i]);
            cap_next = false;
        }
        else
        {
            result += (char)tolower(raw[i]);
        }
    }
    return result;
}

bool MarketStrategy::show_all_prices(int game_id)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();

    auto rows =
        db.Query("SELECT resource_type, current_price, price_trend "
                 "FROM market_prices WHERE game_id=? ORDER BY resource_type",
                 {game_id});

    std::ostringstream out;
    out << std::format("          MARKET EXCHANGE (Round {})\n", s.round)
        << "-------------------------------------------\n"
        << "Resource      Price   Trend\n"
        << "----------    -----   -----\n";

    if (!rows.empty())
    {
        for (const auto& row : rows)
        {
            std::string res = display_resource_name(row[0]);
            int price = std::atoi(row[1].c_str());
            std::string trend = row[2];
            std::string trend_char = "~";

            if (KH_EQU(trend, "RISING"))
            {
                trend_char = "^";
            }
            else if (KH_EQU(trend, "FALLING"))
            {
                trend_char = "v";
            }

            out << std::format("{:<12}  {} CR   {}\n", res, price, trend_char);
        }
    }
    else
    {
        // Fallback to base prices when market_prices not yet populated
        for (int i = 0; i < NUM_BASE_PRICES; i++)
        {
            std::string res = display_resource_name(BASE_PRICES[i].type);
            out << std::format("{:<12}  {} CR   ~\n", res, BASE_PRICES[i].price);
        }
    }

    out << "-------------------------------------------";
    Telemetry::instance().write(out.str());
    bres = true;

    return bres;
}

bool MarketStrategy::show_price_history(int game_id,
                                        const std::string& resource)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    std::string res_upper = resource;
    for (unsigned int idx = 0; idx < res_upper.size(); idx++)
    {
        res_upper[idx] = toupper(res_upper[idx]);
    }

    auto history =
        db.Query("SELECT turn, price FROM market_history WHERE game_id=? "
                 "AND resource_type=? ORDER BY turn DESC LIMIT 10",
                 {game_id, res_upper});

    std::ostringstream out;
    out << std::format("     {} PRICE HISTORY\n", res_upper)
        << "-------------------------------------------\n";

    if (history.empty())
    {
        out << std::format("No trading history for {}.\n",
                           display_resource_name(res_upper));
        out << "Prices may not have fluctuated yet this game.\n";
        out << "Use 'trade list' to see current exchange rates.\n";
    }
    else
    {
        out << "Round   Price\n"
            << "-----   -----\n";
        for (const auto& row : history)
        {
            out << std::format("{}       {} CR\n", row[0], row[1]);
        }
        bres = true;
    }

    out << "-------------------------------------------";
    Telemetry::instance().write(out.str());

    return bres;
}
