//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "turn_end.h"

#include <cstdlib>
#include <ctime>

#include "db.h"
#include "facilities.h"
#include "logger.h"
#include "telemetry.h"

void TurnEndProcessor::on_round_complete(int game_id, int completed_round)
{
    Logger::instance().info("[TURN_END] Round " + std::to_string(completed_round) +
                            " complete for game " + std::to_string(game_id));

    // 1. Update facility control based on ship presence
    update_facilities(game_id, completed_round);

    // 2. Update market prices based on trading activity
    update_market_prices(game_id, completed_round);

    // 3. Regenerate depleted resources
    regenerate_resources(game_id, completed_round);

    // 4. Apply trade hub income to players
    apply_trade_hub_income(game_id, completed_round);

    Logger::instance().info("[TURN_END] Round-end processing complete");
}

void TurnEndProcessor::update_facilities(int game_id, int round)
{
    FacilityEngine::update_control(game_id, round);
}

void TurnEndProcessor::update_market_prices(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get all tracked resources
    auto resources = db.query(
        "SELECT resource_type, base_price, current_price, total_bought, "
        "total_sold FROM market_prices WHERE game_id=" +
        std::to_string(game_id));

    srand(time(NULL) + game_id + round);

    for (const auto& res : resources)
    {
        std::string res_type = res[0];
        int base_price = std::atoi(res[1].c_str());
        int current_price = std::atoi(res[2].c_str());
        int bought = std::atoi(res[3].c_str());
        int sold = std::atoi(res[4].c_str());

        // Calculate demand pressure: net bought = price up, net sold = price down
        int net_demand = bought - sold;
        int demand_delta = (net_demand / 10) * 5;  // +/- 5% per 10 net units

        // Random market variance +/- 2%
        int variance = (rand() % 5) - 2;  // -2 to +2

        // Calculate new price
        int delta_percent = demand_delta + variance;
        int new_price = current_price + (base_price * delta_percent / 100);

        // Clamp to 50%-200% of base
        int min_price = base_price / 2;
        int max_price = base_price * 2;
        if (new_price < min_price)
            new_price = min_price;
        if (new_price > max_price)
            new_price = max_price;

        // Determine trend
        std::string trend = "stable";
        if (new_price > current_price)
            trend = "rising";
        else if (new_price < current_price)
            trend = "falling";

        // Record history before updating
        db.exec(
            "INSERT INTO market_history(game_id, resource_type, price, turn) "
            "VALUES(" +
            std::to_string(game_id) + ",'" + db.esc(res_type) + "'," +
            std::to_string(current_price) + "," + std::to_string(round) + ")");

        // Update current price and reset counters
        db.exec("UPDATE market_prices SET current_price=" +
                std::to_string(new_price) + ", trend='" + trend +
                "', total_bought=0, total_sold=0 WHERE game_id=" +
                std::to_string(game_id) + " AND resource_type='" +
                db.esc(res_type) + "'");

        if (delta_percent != 0 || variance != 0)
        {
            Logger::instance().info("[MARKET] " + res_type + ": " +
                                    std::to_string(current_price) + " -> " +
                                    std::to_string(new_price) + " CR (" + trend +
                                    ")");
        }
    }
}

void TurnEndProcessor::regenerate_resources(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Regenerate resources that haven't been extracted this round
    db.exec("UPDATE resource_state SET current_supply = "
            "LEAST(max_supply, current_supply + regen_rate) "
            "WHERE game_id=" +
            std::to_string(game_id) +
            " AND (last_extracted_turn IS NULL OR last_extracted_turn < " +
            std::to_string(round) + ")");

    Logger::instance().info("[RESOURCES] Regeneration applied for game " +
                            std::to_string(game_id));
}

void TurnEndProcessor::apply_trade_hub_income(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Calculate income for each player
    for (char player : {'A', 'B'})
    {
        int income = FacilityEngine::calculate_trade_hub_income(game_id, player);

        if (income > 0)
        {
            // Get current credits from game state
            auto state_row =
                db.query("SELECT state_json FROM games WHERE id=" +
                         std::to_string(game_id));

            if (!state_row.empty())
            {
                // Parse credits from JSON (simple string search)
                std::string json = state_row[0][0];
                std::string search_key = "\"credits\":{\"A\":";
                size_t credits_pos = json.find(search_key);

                if (credits_pos != std::string::npos)
                {
                    size_t a_start = credits_pos + search_key.length();
                    size_t a_end = json.find(",", a_start);
                    size_t b_start = json.find("\"B\":", a_end) + 4;
                    size_t b_end = json.find("}", b_start);

                    int creditsA = std::atoi(json.substr(a_start, a_end - a_start).c_str());
                    int creditsB = std::atoi(json.substr(b_start, b_end - b_start).c_str());

                    if (player == 'A')
                        creditsA += income;
                    else
                        creditsB += income;

                    // Reconstruct that portion of JSON
                    std::string new_credits = "\"credits\":{\"A\":" +
                                              std::to_string(creditsA) + ",\"B\":" +
                                              std::to_string(creditsB) + "}";

                    std::string new_json =
                        json.substr(0, credits_pos) + new_credits +
                        json.substr(b_end + 1);

                    db.exec("UPDATE games SET state_json='" + db.esc(new_json) +
                            "' WHERE id=" + std::to_string(game_id));

                    Logger::instance().info("[INCOME] Player " +
                                            std::string(1, player) + " earned " +
                                            std::to_string(income) +
                                            " CR from trade hubs");

                    // Notify player
                    Telemetry::getInstance().add_tell(
                        game_id, player,
                        "INCOME: Trade hub revenue +" + std::to_string(income) +
                            " CR this round.");
                }
            }
        }
    }
}
