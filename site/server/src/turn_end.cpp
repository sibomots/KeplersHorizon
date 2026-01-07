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
    Logger::instance().info("[TURN_END] Round " +
                            std::to_string(completed_round) +
                            " complete for game " + std::to_string(game_id));

    // 1. Update facility control based on ship presence
    update_facilities(game_id, completed_round);

    // 2. Update market prices based on trading activity
    update_market_prices(game_id, completed_round);

    // 3. Regenerate depleted resources
    regenerate_resources(game_id, completed_round);

    // 4. Apply trade hub income to players
    apply_trade_hub_income(game_id, completed_round);

    // 5. Process completed fabrication jobs
    process_fabrication_queue(game_id, completed_round);

    // 6. Check victory conditions (VP count phase)
    check_victory_conditions(game_id, completed_round);

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

        // Calculate demand pressure: net bought = price up, net sold = price
        // down
        int net_demand = bought - sold;
        int demand_delta = (net_demand / 10) * 5; // +/- 5% per 10 net units

        // Random market variance +/- 2%
        int variance = (rand() % 5) - 2; // -2 to +2

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
            Logger::instance().info(
                "[MARKET] " + res_type + ": " + std::to_string(current_price) +
                " -> " + std::to_string(new_price) + " CR (" + trend + ")");
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
        int income =
            FacilityEngine::calculate_trade_hub_income(game_id, player);

        if (income > 0)
        {
            // Get current credits from game state
            auto state_row = db.query("SELECT state_json FROM games WHERE id=" +
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

                    int creditsA = std::atoi(
                        json.substr(a_start, a_end - a_start).c_str());
                    int creditsB = std::atoi(
                        json.substr(b_start, b_end - b_start).c_str());

                    if (player == 'A')
                        creditsA += income;
                    else
                        creditsB += income;

                    // Reconstruct that portion of JSON
                    std::string new_credits =
                        "\"credits\":{\"A\":" + std::to_string(creditsA) +
                        ",\"B\":" + std::to_string(creditsB) + "}";

                    std::string new_json = json.substr(0, credits_pos) +
                                           new_credits + json.substr(b_end + 1);

                    db.exec("UPDATE games SET state_json='" + db.esc(new_json) +
                            "' WHERE id=" + std::to_string(game_id));

                    Logger::instance().info(
                        "[INCOME] Player " + std::string(1, player) +
                        " earned " + std::to_string(income) +
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

void TurnEndProcessor::process_fabrication_queue(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Find all jobs that are IN_PROGRESS and complete this round
    auto jobs = db.query(
        "SELECT id, player, ship_code, recipe, quantity FROM fabrication_queue "
        "WHERE game_id=" +
        std::to_string(game_id) +
        " AND status='IN_PROGRESS' AND completion_turn<=" + std::to_string(round));

    for (const auto& job : jobs)
    {
        int job_id = std::atoi(job[0].c_str());
        char player = job[1][0];
        std::string ship_code = job[2];
        std::string recipe = job[3];
        int quantity = std::atoi(job[4].c_str());

        // Apply upgrade based on recipe type
        std::string update_sql;
        std::string msg;

        if (recipe == "BEAM_UPGRADE")
        {
            update_sql = "UPDATE ships SET beam=beam+" + std::to_string(quantity);
            msg = "Beam weapon upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (recipe == "SCREEN_UPGRADE")
        {
            update_sql = "UPDATE ships SET screen=screen+" + std::to_string(quantity);
            msg = "Shields upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (recipe == "TUBE_UPGRADE")
        {
            update_sql = "UPDATE ships SET tube=tube+" + std::to_string(quantity);
            msg = "Missile tubes upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (recipe == "TECH_UPGRADE")
        {
            update_sql = "UPDATE ships SET tech_level=tech_level+" + std::to_string(quantity);
            msg = "Tech level upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (recipe == "MISSILES")
        {
            update_sql = "UPDATE ships SET missiles=missiles+" + std::to_string(quantity);
            msg = "Missiles manufactured (+" + std::to_string(quantity) + ")";
        }
        else
        {
            Logger::instance().info("[FABRICATION] WARNING: Unknown recipe: " + recipe);
            continue;
        }

        // Apply to ship
        if (!ship_code.empty())
        {
            update_sql += " WHERE game_id=" + std::to_string(game_id) +
                          " AND ship_code='" + db.esc(ship_code) + "'";
            db.exec(update_sql);

            // Notify player
            Telemetry::getInstance().add_tell(
                game_id, player,
                "FABRICATION COMPLETE: " + ship_code + " - " + msg);
        }

        // Mark job complete
        db.exec("UPDATE fabrication_queue SET status='COMPLETED' WHERE id=" +
                std::to_string(job_id));

        Logger::instance().info("[FABRICATION] Job " + std::to_string(job_id) +
                                " completed for player " + std::string(1, player));
    }
}

void TurnEndProcessor::check_victory_conditions(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // Check if game already has a winner
    auto game_rows = db.query("SELECT vp_A, vp_B, winner FROM games WHERE id=" +
                              std::to_string(game_id));
    if (game_rows.empty())
        return;

    int vp_A = std::atoi(game_rows[0][0].c_str());
    int vp_B = std::atoi(game_rows[0][1].c_str());
    std::string winner = game_rows[0][2];

    if (!winner.empty())
        return; // Game already ended

    // Check for enemy base star control
    // Get base stars from base_stars table
    auto base_stars = db.query(
        "SELECT hex_id, owner FROM base_stars WHERE game_id=" +
        std::to_string(game_id));

    for (const auto& bs : base_stars)
    {
        std::string hex_id = bs[0];
        char base_owner = bs[1][0];
        char enemy = (base_owner == 'A') ? 'B' : 'A';

        // Check if enemy has ships at this base star
        auto ships = db.query(
            "SELECT 1 FROM ships WHERE game_id=" + std::to_string(game_id) +
            " AND at_hex='" + db.esc(hex_id) + "' AND owner='" +
            std::string(1, enemy) + "' AND destroyed_at IS NULL LIMIT 1");

        // Check if owner has NO ships at their base star
        auto owner_ships = db.query(
            "SELECT 1 FROM ships WHERE game_id=" + std::to_string(game_id) +
            " AND at_hex='" + db.esc(hex_id) + "' AND owner='" +
            std::string(1, base_owner) + "' AND destroyed_at IS NULL LIMIT 1");

        // Enemy controls if they have ships AND owner has none
        if (!ships.empty() && owner_ships.empty())
        {
            // Award +2 VP for controlling enemy base star
            std::string vp_col = (enemy == 'A') ? "vp_A" : "vp_B";
            db.exec("UPDATE games SET " + vp_col + "=" + vp_col +
                    "+2 WHERE id=" + std::to_string(game_id));

            // Update local count
            if (enemy == 'A')
                vp_A += 2;
            else
                vp_B += 2;

            Telemetry::getInstance().add_tell(
                game_id, enemy,
                "VICTORY: +2 VP for controlling enemy base star!");

            Logger::instance().info("[VP] Player " + std::string(1, enemy) +
                                    " controls enemy base star at " + hex_id);
        }
    }

    // Check for win condition (3 VP)
    if (vp_A >= 3)
    {
        db.exec("UPDATE games SET winner='A' WHERE id=" +
                std::to_string(game_id));
        Telemetry::getInstance().add_tell(game_id, 'A',
                                          "*** VICTORY! You have won the game! ***");
        Telemetry::getInstance().add_tell(game_id, 'B',
                                          "*** DEFEAT. Your opponent has won. ***");
        Logger::instance().info("[VICTORY] Player A wins game " +
                                std::to_string(game_id));
    }
    else if (vp_B >= 3)
    {
        db.exec("UPDATE games SET winner='B' WHERE id=" +
                std::to_string(game_id));
        Telemetry::getInstance().add_tell(game_id, 'B',
                                          "*** VICTORY! You have won the game! ***");
        Telemetry::getInstance().add_tell(game_id, 'A',
                                          "*** DEFEAT. Your opponent has won. ***");
        Logger::instance().info("[VICTORY] Player B wins game " +
                                std::to_string(game_id));
    }
}
