///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "turn_end.h"

#include <cstdlib>
#include <ctime>

#include "db.h"
#include "facilities.h"
#include "hex_events.h"
#include "logger.h"
#include "recap_command.h"
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

    // 7. Process hex events (expire old, prepare new)
    HexEventEngine::process_events(game_id, completed_round);

    Logger::instance().info("[TURN_END] Round-end processing complete");
}

void TurnEndProcessor::update_facilities(int game_id, int round)
{
    FacilityEngine::update_control(game_id, round);
}

void TurnEndProcessor::update_market_prices(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Get all tracked resources
    auto resources = db.Query(
        "SELECT resource_type, base_price, current_price, total_bought, "
        "total_sold FROM market_prices WHERE game_id=?",
        {game_id});

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
        db.Exec(
            "INSERT INTO market_history(game_id, resource_type, price, turn) "
            "VALUES(?,?,?,?)",
            {game_id, res_type, current_price, round});

        // Update current price and reset counters
        db.Exec(
            "UPDATE market_prices SET current_price=?, trend=?, "
            "total_bought=0, total_sold=0 WHERE game_id=? AND resource_type=?",
            {new_price, trend, game_id, res_type});

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
    DatabaseManager& db = DatabaseManager::instance();

    // Regenerate resources that haven't been extracted this round
    db.Exec("UPDATE resource_state SET current_supply = "
            "LEAST(max_supply, current_supply + regen_rate) "
            "WHERE game_id=? "
            "AND (last_extracted_turn IS NULL OR last_extracted_turn < ?)",
            {game_id, round});

    Logger::instance().info("[RESOURCES] Regeneration applied for game " +
                            std::to_string(game_id));
}

void TurnEndProcessor::apply_trade_hub_income(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Calculate income for each player
    for (char player : {'A', 'B'})
    {
        int income =
            FacilityEngine::calculate_trade_hub_income(game_id, player);

        if (income > 0)
        {
            // Get current credits from game state
            auto state_row =
                db.Query("SELECT state_json FROM games WHERE id=?", {game_id});

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

                    if (KH_EQU(player, 'A'))
                    {
                        creditsA += income;
                    }
                    else
                    {
                        creditsB += income;
                    }
                    // Reconstruct that portion of JSON
                    std::string new_credits =
                        "\"credits\":{\"A\":" + std::to_string(creditsA) +
                        ",\"B\":" + std::to_string(creditsB) + "}";

                    std::string new_json = json.substr(0, credits_pos) +
                                           new_credits + json.substr(b_end + 1);

                    db.Exec("UPDATE games SET state_json=? WHERE id=?",
                            {new_json, game_id});

                    Logger::instance().info(
                        "[INCOME] Player " + std::string(1, player) +
                        " earned " + std::to_string(income) +
                        " CR from trade hubs");

                    // Notify player
                    Telemetry::instance().add_tell(
                        game_id, player,
                        std::format(LC_MILIEU_MARKET_TARGET_HUB_REVENUE, income));
                }
            }
        }
    }
}

void TurnEndProcessor::process_fabrication_queue(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Find all jobs that are IN_PROGRESS and complete this round
    auto jobs = db.Query(
        "SELECT id, player, ship_code, recipe, quantity FROM fabrication_queue "
        "WHERE game_id=? AND status='IN_PROGRESS' AND completion_turn<=?",
        {game_id, round});

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

        // BUGBUG WHY ARE we still using strings as keys here??
        if (KH_EQU(recipe, "PHASIC_UPGRADE"))
        {
            update_sql =
                "UPDATE ships SET phasic=phasic+" + std::to_string(quantity);
            msg = "Phasic weapon upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (KH_EQU(recipe, "SHIELD_UPGRADE"))
        {
            update_sql =
                "UPDATE ships SET shield=shield+" + std::to_string(quantity);
            msg = "Shields upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (KH_EQU(recipe, "LAUNCHER_UPGRADE"))
        {
            update_sql =
                "UPDATE ships SET launcher=launcher+" + std::to_string(quantity);
            msg = "Torpedo launchers upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (KH_EQU(recipe, "TECH_UPGRADE"))
        {
            update_sql = "UPDATE ships SET tech_level=tech_level+" +
                         std::to_string(quantity);
            msg = "Tech level upgraded (+" + std::to_string(quantity) + ")";
        }
        else if (KH_EQU(recipe, "TORPEDOES"))
        {
            update_sql = "UPDATE ships SET torpedoes=torpedoes+" +
                         std::to_string(quantity);
            msg = "Torpedoes manufactured (+" + std::to_string(quantity) + ")";
        }
        else
        {
            Logger::instance().info("[FABRICATION] WARNING: Unknown recipe: " +
                                    recipe);
            continue;
        }

        // Apply to ship
        if (!ship_code.empty())
        {
            update_sql += " WHERE game_id=? AND ship_code=?";
            db.Exec(update_sql, {game_id, ship_code});

            // Notify player
            Telemetry::instance().add_tell(
                game_id, player,
                std::format(LC_MILIEU_FABRICATE_TARGET_QUEUED_COMPLETE, ship_code,msg));
        }

        // Mark job complete
        db.Exec("UPDATE fabrication_queue SET status='COMPLETED' WHERE id=?",
                {job_id});

        Logger::instance().info("[FABRICATION] Job " + std::to_string(job_id) +
                                " completed for player " +
                                std::string(1, player));
    }
}

void TurnEndProcessor::check_victory_conditions(int game_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Check if game already has a winner
    auto game_rows =
        db.Query("SELECT vp_A, vp_B, winner FROM games WHERE id=?", {game_id});
    if (game_rows.empty())
        return;

    int vp_A = std::atoi(game_rows[0][0].c_str());
    int vp_B = std::atoi(game_rows[0][1].c_str());
    std::string winner = game_rows[0][2];

    if (!winner.empty())
        return; // Game already ended

    // Check for enemy base star control
    // Get base stars from base_stars table
    auto base_stars = db.Query(
        "SELECT hex_id, owner FROM base_stars WHERE game_id=?", {game_id});

    for (const auto& bs : base_stars)
    {
        std::string hex_id = bs[0];
        char base_owner = bs[1][0];
        char enemy = base_owner ^ 0x03;

        // Check if enemy has ships at this base star
        auto ships = db.Query(
            "SELECT 1 FROM ships WHERE game_id=? AND at_hex=? AND owner=? "
            "AND destroyed_at IS NULL LIMIT 1",
            {game_id, hex_id, enemy});

        // Check if owner has NO ships at their base star
        auto owner_ships = db.Query(
            "SELECT 1 FROM ships WHERE game_id=? AND at_hex=? AND owner=? "
            "AND destroyed_at IS NULL LIMIT 1",
            {game_id, hex_id, base_owner});


        // BUGBUG THIS IS STILL BRITTLE.  How the game ends is broken.
        // Enemy controls if they have ships AND owner has none
        if (!ships.empty() && owner_ships.empty())
        {
            // Award +1 VP for controlling enemy base star
            std::string vp_col = (KH_EQU(enemy, 'A')) ? "vp_A" : "vp_B";
            db.Exec("UPDATE games SET " + vp_col + "=" + vp_col +
                        "+1 WHERE id=?",
                    {game_id});

            // Update local count
            if (KH_EQU(enemy, 'A'))
            {
                vp_A += 1;
            }
            else
            {
                vp_B += 1;
            }
            Telemetry::instance().add_tell(
                game_id, enemy,
                std::format(LC_ADDED_TARGET_VP, 1));

            Logger::instance().info("[VP] Player " + std::string(1, enemy) +
                                    " controls enemy base star at " + hex_id);
        }
    }

    // Check for win condition (3 VP)
    if (vp_A >= 3)
    {
        db.Exec("UPDATE games SET winner='A' WHERE id=?", {game_id});
        Telemetry::instance().add_tell(game_id, 'A', LC_VP_SELF_WON_GAME);
        Telemetry::instance().add_tell(game_id, 'B', LC_VP_SELF_LOST_GAME);
        Logger::instance().info("[VICTORY] Player A wins game " +
                                std::to_string(game_id));
    }
    else if (vp_B >= 3)
    {
        db.Exec("UPDATE games SET winner='B' WHERE id=?", {game_id});
        Telemetry::instance().add_tell(game_id, 'B', LC_VP_SELF_WON_GAME);
        Telemetry::instance().add_tell(game_id, 'A', LC_VP_SELF_LOST_GAME);
        Logger::instance().info("[VICTORY] Player B wins game " +
                                std::to_string(game_id));
    }


    // Post-game recap for both players
    std::string updated_winner;
    std::vector<std::vector<std::string>> winner_check = db.Query(
        "SELECT winner FROM games WHERE id=?", {game_id});
    if (!winner_check.empty())
    {
        updated_winner = winner_check[0][0];
    }
    if (!updated_winner.empty())
    {
        RecapCommand::emit_recap(game_id, 'A');
        RecapCommand::emit_recap(game_id, 'B');
    }
}
