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
// Per-game market_prices must be populated at game creation from
// market_base_prices. Price fluctuation is handled by TurnEndProcessor.

bool MarketStrategy::show_all_prices(int game_id)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();

    auto rows =
        db.Query("SELECT resource_type, current_price, price_trend "
                 "FROM market_prices WHERE game_id=? ORDER BY resource_type",
                 {game_id});

    if (rows.empty())
    {
        Telemetry::instance().write("MARKET: No market data available.");
    }
    else
    {
        std::ostringstream out;
        out << std::format("          MARKET EXCHANGE (Round {})\n", s.round)
            << "-------------------------------------------\n"
            << "Resource      Price   Trend\n"
            << "----------    -----   -----\n";

        for (const auto& row : rows)
        {
            std::string res = row[0];
            int price = std::atoi(row[1].c_str());
            std::string trend = row[2];
            std::string trend_char = "~";

            // BUGBUG why is this flag 'trend' stored as a string???
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

        out << "-------------------------------------------";
        Telemetry::instance().write(out.str());
        bres = true;
    }

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
        out << std::format("No trading history for {}\n", res_upper);
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
