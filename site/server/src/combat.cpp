//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "combat.h"

#include <algorithm>
#include <iostream>
#include <set>

#include "logger.h"
#include "maputil.h"
#include "util.h"

CombatEngine::CombatEngine(int game_id) : game_id(game_id)
{
}

void CombatEngine::check_for_combat_triggers()
{
    // 1. Find all hexes containing ships from both players (A and B)
    //    We can do this by selecting all ship locations and grouping.
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT at_hex, owner FROM ships "
        "WHERE game_id=" +
        std::to_string(game_id) +
        " "
        "AND at_hex IS NOT NULL AND racked_in IS NULL" // Only active ships in
                                                       // space
    );

    std::map<std::string, std::set<char>> hexOccupants;
    for (const auto& r : rows)
    {
        std::string h = r[0];
        char o = r[1][0];
        hexOccupants[h].insert(o);
    }

    for (auto const& [hex, occupants] : hexOccupants)
    {
        // If both A and B are present
        if (occupants.count('A') && occupants.count('B'))
        {
            // Check if combat already exists
            auto exist =
                db.query("SELECT 1 FROM combat_state WHERE game_id=" +
                         std::to_string(game_id) + " AND hex_id='" + hex + "'");
            if (exist.empty())
            {
                create_combat(hex);
            }
        }
    }
}

void CombatEngine::create_combat(const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // Identify attacker? For now, we assume simple engagement.
    // In strict rules, the "moving player" is attacker.
    // We might need to passed that context in, or infer it (active player).
    // For now, default to active player being attacker is reasonable for new
    // combats initiated during their turn.

    // We need to fetch active player to set attacker_remains (if that implies
    // initiative). Let's just INSERT.
    db.exec("INSERT INTO combat_state (game_id, hex_id, round, stage, "
            "attacker_remains, stalemate_counter) "
            "VALUES (" +
            std::to_string(game_id) + ", '" + hex_id + "', 1, 0, 0, 0)");
}

std::vector<CombatState> CombatEngine::get_active_combats()
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<CombatState> result;
    auto rows = db.query("SELECT hex_id, round, stage, attacker_remains, "
                         "stalemate_counter, pending_damage_json, last_log "
                         "FROM combat_state WHERE game_id=" +
                         std::to_string(game_id));

    for (const auto& r : rows)
    {
        result.emplace_back(game_id, r[0], std::atoi(r[1].c_str()),
                            std::atoi(r[2].c_str()), (r[3] == "1"),
                            std::atoi(r[4].c_str()), r[5], r[6]);
    }
    return result;
}

CombatState CombatEngine::get_combat_state(const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows =
        db.query("SELECT round, stage, attacker_remains, stalemate_counter, "
                 "pending_damage_json, last_log "
                 "FROM combat_state WHERE game_id=" +
                 std::to_string(game_id) + " AND hex_id='" + hex_id + "'");
    if (rows.empty())
        return {0, "", 0, 0, false, 0, "", ""};

    return CombatState(game_id, hex_id, std::atoi(rows[0][0].c_str()),
                       std::atoi(rows[0][1].c_str()), (rows[0][2] == "1"),
                       std::atoi(rows[0][3].c_str()), rows[0][4], rows[0][5]);
}

std::string CombatEngine::submit_order(char owner, const CombatOrder& order_in)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    CombatOrder order = order_in;
    // 1. Validate ownership
    {
        auto r = db.query(
            "SELECT owner FROM ships WHERE game_id=" + std::to_string(game_id) +
            " AND ship_code='" + db.esc(order.ship_code) + "'");
        if (r.empty())
            return "Ship not found";

        char shipOwner = r[0][0][0];
        if (shipOwner != owner)
            return "You do not own this ship";
    }

    // 2. Validate Combat State & Stats
    // Find the hex this ship is in and its stats
    auto r = db.query(
        "SELECT at_hex, pd, beam, screen, tube FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND ship_code='" + db.esc(order.ship_code) +
        "'");
    if (r.empty() || r[0][0].empty())
        return "Ship not in space";
    std::string hex_id = r[0][0];
    int max_pd = std::atoi(r[0][1].c_str());
    int max_b = std::atoi(r[0][2].c_str());
    int max_s = std::atoi(r[0][3].c_str());
    int max_t = std::atoi(r[0][4].c_str());

    // Validate Power Limits
    if (order.power_b > max_b)
        return "Beam power exceeds rating (" + std::to_string(max_b) + ")";
    if (order.power_s > max_s)
        return "Screen power exceeds rating (" + std::to_string(max_s) + ")";
    if (order.power_t > max_t)
        return "Tube power exceeds rating (" + std::to_string(max_t) + ")";

    int total = order.power_d + order.power_b + order.power_s + order.power_t;
    if (total > max_pd)
        return "Total power (" + std::to_string(total) + ") exceeds PD (" +
               std::to_string(max_pd) + ")";

    auto cs = get_combat_state(hex_id);
    if (cs.game_id == 0)
        return "No combat in this hex";
    if (cs.stage != 0)
        return "Not accepting orders (Stage " + std::to_string(cs.stage) + ")";

    // Auto-set the round to match the current combat state
    order.round = cs.round;

    // 3. Insert/Update
    db.exec("INSERT INTO combat_orders (game_id, owner, ship_code, round, "
            "tactic, target_id, power_d, power_b, power_s, power_t, "
            "missiles_json) VALUES (" +
            std::to_string(game_id) + ", '" + std::string(1, owner) + "', '" +
            order.ship_code + "', " + std::to_string(order.round) + ", '" +
            std::string(1, order.tactic) + "', '" + order.target_id + "', " +
            std::to_string(order.power_d) + ", " +
            std::to_string(order.power_b) + ", " +
            std::to_string(order.power_s) + ", " +
            std::to_string(order.power_t) + ", '" + order.missiles_json +
            "') "
            "ON DUPLICATE KEY UPDATE "
            "tactic='" +
            std::string(1, order.tactic) +
            "', "
            "target_id='" +
            order.target_id +
            "', "
            "power_d=" +
            std::to_string(order.power_d) +
            ", "
            "power_b=" +
            std::to_string(order.power_b) +
            ", "
            "power_s=" +
            std::to_string(order.power_s) +
            ", "
            "power_t=" +
            std::to_string(order.power_t) +
            ", "
            "missiles_json='" +
            order.missiles_json + "'");

    // 4. Check completion
    if (all_orders_submitted(hex_id, order.round))
    {
        // Auto-Resolve!
        std::string res = resolve_round(hex_id);
        return "Orders Saved. Invoking Auto-Resolve...\n" + res;
    }

    return "Order Saved";
}

bool CombatEngine::all_orders_submitted(const std::string& hex_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // Count ships in hex
    auto sr = db.query(
        "SELECT COUNT(*) FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND at_hex='" + hex_id + "' AND racked_in IS NULL");
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count orders
    auto or_ = db.query("SELECT COUNT(*) FROM combat_orders co "
                        "JOIN ships s ON s.game_id = co.game_id AND "
                        "s.ship_code = co.ship_code AND s.owner = co.owner "
                        "WHERE co.game_id=" +
                        std::to_string(game_id) +
                        " AND co.round=" + std::to_string(round) +
                        " AND s.at_hex='" + hex_id + "'");
    int orderCount = std::atoi(or_[0][0].c_str());

    Logger::instance().info("[DEBUG] Hex " + hex_id + " Round " +
                            std::to_string(round) +
                            ": Ships=" + std::to_string(shipCount) +
                            " Orders=" + std::to_string(orderCount));

    return orderCount >= shipCount;
}

// --- CRT Helper ---
static int get_crt_mod(int drive_diff, char tactic_fire, char tactic_target,
                       bool& escaped)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    escaped = false;
    // Tactic: A, D, R
    // Returns damage mod (0, 1, 2) or -999 for Miss.
    // If escaped=true, it's a miss but counts as successful retreat.

    // Simplification of logic from rules:
    /*
      ATTACK vs:
        A: <=-3 Miss, -1/-2 Hit, 0/+1 Hit+2, +2 Hit+1, >=+3 Miss
        D: <=-3 Miss, ..., -2..+2 ranges...
        R: <=-2 Escape, -1..+2 Miss, >=+3 Hit
    */
    // Implementing purely based on the text table provided in
    // docs/COMBAT_RULES.md

    if (tactic_fire == 'A')
    {
        if (tactic_target == 'A')
        {
            if (drive_diff <= -3)
                return -999;
            if (drive_diff <= -1)
                return 0; // Hit
            if (drive_diff <= 1)
                return 2; // Hit+2
            if (drive_diff == 2)
                return 1; // Hit+1
            return -999;  // +3 or more Miss
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3)
                return -999;
            if (drive_diff <= 1)
                return -999;
            if (drive_diff == 2)
                return 1; // Hit+1
            if (drive_diff <= 4)
                return 0; // Hit
            return -999;
        }
        if (tactic_target == 'R')
        {
            if (drive_diff <= -3)
            {
                escaped = true;
                return -999;
            }
            if (drive_diff <= -1)
            {
                escaped = true;
                return -999;
            }
            if (drive_diff <= 1)
                return -999; // Miss (prevent escape?) Rules say "Miss", assumes
                             // blocked logic logic? Table says: <=-3 Esc, -1/-2
                             // Esc, 0/+1 Miss, +2 Miss, +3/+4 Hit, +5 Miss?
                             // Re-reading Doc:
                             // ATK vs RET:
                             // <=-3 Esc, -1/-2 Esc, 0/+1 Miss, +2 Miss, +3/+4
                             // Hit, >=5 Miss ?? Wait, Doc says: Firing Tactic:
                             // ATTACK Col Retreat: -3 or less: Escapes -1 or
                             // -2: Escapes 0 or +1: Miss (pinned) +2: Miss +3
                             // or +4: Hit +5 or more: Miss
            if (drive_diff <= -1)
            {
                escaped = true;
                return -999;
            }
            if (drive_diff <= 2)
                return -999; // Miss (Pinned)
            if (drive_diff <= 4)
                return 0; // Hit
            return -999;
        }
    }
    else if (tactic_fire == 'D')
    {
        // DODGE firing
        if (tactic_target == 'A')
        {
            if (drive_diff <= -4)
                return -999;
            if (drive_diff <= -2)
                return -999;
            if (drive_diff <= 2)
                return 0; // Hit
            return -999;
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3)
                return -999;
            if (drive_diff == -2)
                return 0; // Hit
            if (drive_diff <= 0)
                return 0; // Hit (-1, 0)
            return -999;
        }
        if (tactic_target == 'R')
        {
            return -999; // Always escapes against Dodge according to "Escapes"
                         // in all rows roughly?
            // Doc:
            // <=-4 Esc
            // -2/-3 Esc
            // 0/-1 Esc
            // +1/+2 Esc
            // +3... Esc
            // Yes, Dodge cannot stop Retreat.
            escaped = true;
            return -999;
        }
    }
    else if (tactic_fire == 'R')
    {
        // RETREAT firing
        if (tactic_target == 'A')
        {
            if (drive_diff <= -2)
                return -999;
            if (drive_diff <= 0)
                return 0; // Hit
            return -999;
        }
        if (tactic_target == 'D')
        {
            return -999; // Miss
        }
        if (tactic_target == 'R')
        {
            escaped = true;
            return -999;
        }
    }
    return -999;
}

std::string CombatEngine::resolve_round(const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // 1. Get State
    auto cs = get_combat_state(hex_id);
    if (cs.game_id == 0)
        return "No combat";
    if (cs.stage != 1)
    {
        // Auto-advance if everyone submitted orders?
        // For now, strict check.
        if (cs.stage == 0 && all_orders_submitted(hex_id, cs.round))
        {
            // allow proceeding
        }
        else
        {
            return "Not ready to resolve (Stage " + std::to_string(cs.stage) +
                   ")";
        }
    }

    // 2. Load Ships & Orders
    struct ShipCtx
    {
        std::string code;
        char owner;
        int tech;
        CombatOrder ord;
        int damage_received = 0;
        int escape_attempts = 0;
        int escape_successes = 0;
        bool destroyed = false;
    };
    std::map<std::string, ShipCtx> ships;

    // Load active ships in hex
    auto r = db.query(
        "SELECT ship_code, owner, tech_level FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND at_hex='" + hex_id +
        "' AND racked_in IS NULL");
    for (const auto& row : r)
    {
        ShipCtx s;
        s.code = row[0];
        s.owner = row[1][0];
        s.tech = std::atoi(row[2].c_str());
        std::string key = std::string(1, s.owner) + "_" + s.code;
        ships[key] = s;
    }

    // Load orders
    auto ro = db.query(
        "SELECT ship_code, owner, tactic, target_id, power_d, power_b, "
        "power_s, power_t, missiles_json FROM combat_orders WHERE game_id=" +
        std::to_string(game_id) + " AND round=" + std::to_string(cs.round));

    for (const auto& row : ro)
    {
        std::string c = row[0];
        char o = row[1][0];
        std::string key = std::string(1, o) + "_" + c;
        if (ships.count(key))
        {
            ships[key].ord.tactic = row[2][0];
            ships[key].ord.target_id = row[3];
            ships[key].ord.power_d = std::atoi(row[4].c_str());
            ships[key].ord.power_b = std::atoi(row[5].c_str());
            ships[key].ord.power_s = std::atoi(row[6].c_str());
            ships[key].ord.power_t = std::atoi(row[7].c_str());
            ships[key].ord.missiles_json = row[8];
        }
    }

    std::ostringstream log;
    log << "\\n========================================\\n";
    log << "       COMBAT ROUND " << cs.round << " RESOLUTION\\n";
    log << "========================================\\n";

    // 3. Resolve Fire
    // Beams
    for (auto& [key, ship] : ships)
    {
        if (ship.ord.power_b > 0 && !ship.ord.target_id.empty())
        {
            std::string tid = ship.ord.target_id;
            // Find target (simple scan for enemy with this code)
            // Assumes 1 enemy with this code.
            ShipCtx* target = nullptr;
            for (auto& [tkey, tship] : ships)
            {
                if (tship.owner != ship.owner && tship.code == tid)
                {
                    target = &tship;
                    break;
                }
            }

            if (target)
            {
                target->escape_attempts++; // Being fired upon challenges escape

                int drive_diff = ship.ord.power_d - target->ord.power_d;
                bool escaped = false;
                int mod = get_crt_mod(drive_diff, ship.ord.tactic,
                                      target->ord.tactic, escaped);

                if (escaped)
                    target->escape_successes++;

                if (mod != -999)
                {
                    int dmg = ship.ord.power_b + ship.tech + mod;
                    if (dmg > 0)
                    {
                        target->damage_received += dmg;
                        log << ship.code << " beams " << target->code << " for "
                            << dmg << " dmg!\\n";
                    }
                }
                else
                {
                    if (escaped)
                        log << target->code << " outruns " << ship.code
                            << "'s beams.\\n";
                    else
                        log << ship.code << " misses " << target->code
                            << ".\\n";
                }
            }
            else
            {
                log << ship.code << " has no target '" << tid << "'\\n";
            }
        }
        else if (ship.ord.tactic == 'R')
        {
            // If retreating and NOT fired upon, do we need to track?
            // Logic: Escape if escaped ALL attacks. If 0 attacks, Escape =
            // Success.
        }
    }

    // Missiles
    for (auto& [key, ship] : ships)
    {
        if (!ship.ord.missiles_json.empty() && ship.ord.missiles_json != "[]")
        {
            int count = std::atoi(ship.ord.missiles_json.c_str());
            if (count > 0 && ship.ord.power_t >= count)
            {
                // Determine target
                std::string tid = ship.ord.target_id;
                ShipCtx* target = nullptr;
                for (auto& [tkey, tship] : ships)
                {
                    if (tship.owner != ship.owner && tship.code == tid)
                    {
                        target = &tship;
                        break;
                    }
                }

                if (target)
                {
                    for (int m = 0; m < count; ++m)
                    {
                        target->escape_attempts++;
                        int drive_diff = ship.ord.power_d - target->ord.power_d;
                        bool escaped = false;
                        int mod = get_crt_mod(drive_diff, ship.ord.tactic,
                                              target->ord.tactic, escaped);
                        if (escaped)
                            target->escape_successes++;

                        if (mod != -999)
                        {
                            int dmg = 3 + ship.tech + mod; // Base warhead 3?
                            target->damage_received += dmg;
                            log << ship.code << " missile hits " << target->code
                                << " for " << dmg << " dmg!\\n";
                        }
                        else
                        {
                            if (escaped)
                                log << target->code << " outruns " << ship.code
                                    << "'s missile.\\n";
                            else
                                log << ship.code << " missile misses "
                                    << target->code << ".\\n";
                        }
                    }
                    // Deduct ammo? Need to update ships table?
                    // For now simplest simulation: assume deducted elsewhere or
                    // strictly simulation. User didn't asking for permanent
                    // ammo tracking yet, just resolution.
                }
            }
        }
    }

    // 4. Calc Net Damage & Absorb
    int total_net_damage = 0;
    std::ostringstream dmgJson;
    dmgJson << "{";
    bool first = true;

    for (auto& [key, ship] : ships)
    {
        int absorb = 0;
        if (ship.ord.power_s > 0)
            absorb = ship.ord.power_s +
                     ship.tech; // Tech adds to shields? Rules: "Screen Power +
                                // Tech Level (if powered)"

        int net = std::max(0, ship.damage_received - absorb);
        if (net > 0)
        {
            if (!first)
                dmgJson << ",";
            dmgJson << "\\\"" << key << "\\\":" << net; // Uses owner_code key
            total_net_damage += net;
            first = false;
            log << ship.code << " takes " << net << " net damage (Absorbed "
                << absorb << ").\\n";
        }

        // Retreat Logic
        if (ship.ord.tactic == 'R')
        {
            bool escape = false;
            if (ship.escape_attempts == 0)
                escape = true; // Unopposed retreat
            else if (ship.escape_successes == ship.escape_attempts)
                escape = true; // Eluded all fire

            if (escape)
            {
                log << ship.code << " successfully retreats!\\n";
                // Execute Retreat immediately? Or at end of round?
                // Rules: "Retreat Resolution".
                // We can mark them. For now, strict damage focus.
            }
            else
            {
                log << ship.code << " failed to retreat.\\n";
            }
        }
    }
    dmgJson << "}";

    // 5. Update State
    int next_round = cs.round;
    int next_stage = 0; // Back to ORDERS
    int next_stalemate = cs.stalemate_counter;

    if (total_net_damage > 0)
    {
        next_stage = 2;     // DAMAGE_PENDING
        next_stalemate = 0; // Reset
        log << "----------------------------------------\\n";
        log << "RESULT: Critical Damage! Assign Damage now.\\n";
    }
    else
    {
        log << "----------------------------------------\\n";
        log << "RESULT: Stalemate (No Hull Damage).\\n";
        log << "STATUS: Beginning Round " << (next_round + 1) << "\\n";
        log << "ACTION: Submit Orders for Round " << (next_round + 1) << "\\n";
        log << "========================================\\n";
        next_round++;
        next_stalemate++;
    }

    std::string sql =
        "UPDATE combat_state SET round=" + std::to_string(next_round) +
        ", stage=" + std::to_string(next_stage) +
        ", stalemate_counter=" + std::to_string(next_stalemate) +
        ", pending_damage_json='" + dmgJson.str() + "'" + ", last_log='" +
        db.esc(log.str()) + "'" + " WHERE game_id=" + std::to_string(game_id) +
        " AND hex_id='" + hex_id + "'";
    db.exec(sql);

    // Append log to game events?
    return log.str();
}

std::string
CombatEngine::apply_damage(char owner, const std::string& ship_code,
                           const std::map<std::string, int>& assignments)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // 1. Verify State
    // Find hex for ship
    auto r = db.query("SELECT at_hex, owner, pd, beam, screen, tube, missiles "
                      "FROM ships WHERE game_id=" +
                      std::to_string(game_id) + " AND ship_code='" +
                      db.esc(ship_code) + "' AND owner='" +
                      std::string(1, owner) + "'");
    if (r.empty())
        return "Ship not found";

    std::string hex = r[0][0];
    char realOwner = r[0][1][0];
    if (realOwner != owner)
        return "Not your ship";

    auto cs = get_combat_state(hex);
    if (cs.stage != 2)
        return "No pending damage for this hex"; // 2=DAMAGE_PENDING

    // 2. Parse Pending Damage
    // JSON: {"A_S20": 4, "B_W1": 2} ... simple parse
    std::map<std::string, int> pending;
    std::string json = cs.pending_damage_json;
    // Remove braces
    if (json.size() >= 2)
        json = json.substr(1, json.size() - 2);

    std::vector<std::string> parts = split(json, ',');
    for (auto& p : parts)
    {
        size_t c = p.find(':');
        if (c != std::string::npos)
        {
            std::string k = p.substr(0, c);
            std::string v = p.substr(c + 1);
            // clean quotes from k
            size_t q1 = k.find('"');
            size_t q2 = k.rfind('"');
            if (q1 != std::string::npos && q2 > q1)
                k = k.substr(q1 + 1, q2 - q1 - 1);
            pending[k] = std::atoi(v.c_str());
        }
    }

    std::string key = std::string(1, owner) + "_" + ship_code;
    if (pending.find(key) == pending.end())
        return "No damage pending for this ship";

    int needed = pending[key];
    int assigned = 0;
    for (auto const& [k, v] : assignments)
        assigned += v;

    if (assigned > needed)
        return "Assigned more damage than required"; // Or allow
                                                     // over-assignment? Rules
                                                     // say "applied by owning
                                                     // player". Assumes exact
                                                     // match needed?
    // Let's enforce exact match OR destruction.

    // 3. Apply to attributes
    // DB columns: pd, beam, screen, tube, missiles
    // assignments keys: "D", "B", "S", "T", "M"
    // Validate current values
    int cur_pd = std::atoi(r[0][2].c_str());
    int cur_b = std::atoi(r[0][3].c_str());
    int cur_s = std::atoi(r[0][4].c_str());
    int cur_t = std::atoi(r[0][5].c_str());
    int cur_m = std::atoi(r[0][6].c_str());

    std::string updateSql;

    // Helper to reduce
    auto apply_attr = [&](const std::string& key, int& cur, std::string col) {
        if (assignments.count(key))
        {
            int dmg = assignments.at(key);
            if (key == "M")
            {
                // Missiles: 1 hit removes 3. If <3, removes all.
                // Wait, input 'assignments' acts as "hits applied".
                // So if user assigns 1 hit to M, we subtract 3 missiles.
                int loss = dmg * 3;
                if (cur < 3 && dmg > 0)
                    loss = cur; // 1-2 remain absorb 1 hit -> lost.
                cur = std::max(0, cur - loss);
            }
            else
            {
                cur = std::max(0, cur - dmg);
            }
            if (!updateSql.empty())
                updateSql += ",";
            updateSql += col + "=" + std::to_string(cur);
        }
    };

    apply_attr("D", cur_pd, "pd");
    apply_attr("B", cur_b, "beam");
    apply_attr("S", cur_s, "screen");
    apply_attr("T", cur_t, "tube");
    apply_attr("M", cur_m, "missiles");

    if (!updateSql.empty())
    {
        db.exec("UPDATE ships SET " + updateSql +
                " WHERE game_id=" + std::to_string(game_id) +
                " AND ship_code='" + db.esc(ship_code) + "'");
    }

    // Check Destruction
    if (cur_pd == 0 && cur_b == 0 && cur_s == 0 && cur_t == 0)
    {
        // Ship Destroyed
        db.exec("DELETE FROM ships WHERE game_id=" + std::to_string(game_id) +
                " AND ship_code='" + db.esc(ship_code) + "' AND owner='" +
                std::string(1, owner) + "'");
        // Clean orders
        db.exec("DELETE FROM combat_orders WHERE game_id=" +
                std::to_string(game_id) + " AND ship_code='" +
                db.esc(ship_code) + "' AND owner='" + std::string(1, owner) +
                "'");
        // Remove pending (already done below)
    }

    // 4. Update Pending
    int remaining = needed - assigned;
    // If not fully assigned, we update the pending?
    // Or do we require FULL assignment in one go?
    // Let's assume FULL assignment required for that single ship.
    if (remaining > 0)
        return "Must assign all " + std::to_string(needed) + " hits";

    if (remaining > 0)
        return "Must assign all " + std::to_string(needed) + " hits";

    pending.erase(key);

    // Reconstruct JSON
    std::ostringstream newJson;
    newJson << "{";
    int c = 0;
    for (auto const& [k, v] : pending)
    {
        if (c++ > 0)
            newJson << ",";
        newJson << "\\\"" << k << "\\\":" << v;
    }
    newJson << "}";

    std::string next_json = newJson.str();
    int next_stage = 2; // Stay in damage
    int next_round = cs.round;
    int next_stale = cs.stalemate_counter;

    if (pending.empty())
    {
        // All done!
        next_stage = 0;
        next_round++;
        next_stale++; // Round effectively ended. Stalemate increment logic was
                      // handled in resolve, but wait... If verification
                      // revealed hits, resolve reset stalemate to 0. So here we
                      // don't touch stalemate. Wait, we need to advance round.
    }

    db.exec("UPDATE combat_state SET stage=" + std::to_string(next_stage) +
            ", round=" + std::to_string(next_round) +
            ", pending_damage_json='" + next_json + "' WHERE game_id=" +
            std::to_string(game_id) + " AND hex_id='" + hex + "'");

    return "Damage Applied";
}
