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
#include <sstream>

#include "combat_apply_command.h"
#include "combat_cancel_command.h"
#include "combat_commit_command.h"
#include "combat_drafts_command.h"
#include "combat_order_command.h"
#include "constraints.h"
#include "db.h"
#include "logger.h"
#include "maputil.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

CombatEngine::CombatEngine(int game_id) : game_id(game_id)
{
}

void CombatEngine::check_for_combat_triggers()
{
    // 1. Find all hexes containing ships from both players (A and B)
    //    We can do this by selecting all ship locations and grouping.
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query("SELECT at_hex, owner FROM ships "
                         "WHERE game_id=" +
                         std::to_string(game_id) +
                         " "
                         "AND at_hex IS NOT NULL AND racked_in IS NULL AND "
                         "destroyed_at IS NULL");

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
            " AND ship_code='" + db.esc(order.ship_code) +
            "' AND destroyed_at IS NULL");
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
        "' AND destroyed_at IS NULL");
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
            "missiles_data, committed) VALUES (" +
            std::to_string(game_id) + ", '" + std::string(1, owner) + "', '" +
            order.ship_code + "', " + std::to_string(order.round) + ", '" +
            std::string(1, order.tactic) + "', '" + order.target_id + "', " +
            std::to_string(order.power_d) + ", " +
            std::to_string(order.power_b) + ", " +
            std::to_string(order.power_s) + ", " +
            std::to_string(order.power_t) + ", '" + order.missiles_data +
            "', 0) "
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
            "missiles_data='" +
            order.missiles_data + "', committed=0");

    // Resolution triggered by explicit 'combat commit'
    return "Order draft saved. Use 'combat commit' when ready.";
}

bool CombatEngine::all_orders_submitted(const std::string& hex_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // Count ships in hex
    auto sr = db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                       std::to_string(game_id) + " AND at_hex='" + hex_id +
                       "' AND racked_in IS NULL AND destroyed_at IS NULL");
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count orders
    auto or_ = db.query(
        "SELECT COUNT(*) FROM combat_orders co "
        "JOIN ships s ON s.game_id = co.game_id AND "
        "s.ship_code = co.ship_code AND s.owner = co.owner "
        "WHERE co.game_id=" +
        std::to_string(game_id) + " AND co.round=" + std::to_string(round) +
        " AND s.at_hex='" + hex_id + "' AND s.destroyed_at IS NULL");
    int orderCount = std::atoi(or_[0][0].c_str());

    Logger::instance().info("[COMBAT] Hex " + hex_id + " Round " +
                            std::to_string(round) +
                            ": Ships=" + std::to_string(shipCount) +
                            " Orders=" + std::to_string(orderCount));

    return orderCount >= shipCount;
}

bool CombatEngine::all_orders_committed(const std::string& hex_id, int round)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // Count ships in hex
    auto sr = db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                       std::to_string(game_id) + " AND at_hex='" + hex_id +
                       "' AND racked_in IS NULL AND destroyed_at IS NULL");
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count COMMITTED orders
    auto or_ = db.query("SELECT COUNT(*) FROM combat_orders co "
                        "JOIN ships s ON s.game_id = co.game_id AND "
                        "s.ship_code = co.ship_code AND s.owner = co.owner "
                        "WHERE co.game_id=" +
                        std::to_string(game_id) +
                        " AND co.round=" + std::to_string(round) +
                        " AND co.committed=1 AND s.at_hex='" + hex_id +
                        "' AND s.destroyed_at IS NULL");
    int commitCount = std::atoi(or_[0][0].c_str());

    Logger::instance().info("[COMBAT] Hex " + hex_id + " Round " +
                            std::to_string(round) +
                            ": Ships=" + std::to_string(shipCount) +
                            " Committed=" + std::to_string(commitCount));

    return commitCount >= shipCount;
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
    // ATTACK vs:
    //  A: <=-3 Miss, -1/-2 Hit, 0/+1 Hit+2, +2 Hit+1, >=+3 Miss
    //  D: <=-3 Miss, ..., -2..+2 ranges...
    //  R: <=-2 Escape, -1..+2 Miss, >=+3 Hit
    // Implementing purely based on the text table provided in
    // docs/COMBAT_RULES.md

    if (tactic_fire == 'A')
    {
        if (tactic_target == 'A')
        {
            if (drive_diff <= -3) {
                return -999;
            }
            if (drive_diff <= -1) {
                return 0; // Hit
            }
            if (drive_diff <= 1) {
                return 2; // Hit+2
            }
            if (drive_diff == 2) {
                return 1; // Hit+1
            }
            return -999;  // +3 or more Miss
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3) {
                return -999;
            }
            if (drive_diff <= 1) {
                return -999;
            }
            if (drive_diff == 2) {
                return 1; // Hit+1
            }
            if (drive_diff <= 4) {
                return 0; // Hit
            }
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

            if (drive_diff <= 1) {
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
            }

            if (drive_diff <= -1)
            {
                escaped = true;
                return -999;
            }
            if (drive_diff <= 2) {
                return -999; // Miss (Pinned)
            }
            if (drive_diff <= 4) {
                return 0; // Hit
            }
            return -999;
        }
    }
    else if (tactic_fire == 'D')
    {
        // DODGE firing
        if (tactic_target == 'A')
        {
            if (drive_diff <= -4) {
                return -999;
            }
            if (drive_diff <= -2) {
                return -999;
            }
            if (drive_diff <= 2) {
                return 0; // Hit
            }
            return -999;
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3) {
                return -999;
            }
            if (drive_diff == -2) {
                return 0; // Hit
            }
            if (drive_diff <= 0) {
                return 0; // Hit (-1, 0)
            }
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
            if (drive_diff <= -2) {
                return -999;
            }
            else if (drive_diff <= 0) {
                return 0; // Hit
            }
            else {
                return -999;
            }
        }
        if (tactic_target == 'D')
        {
            return -999; // Miss
        }
        else if (tactic_target == 'R')
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
            // BUGBUG
        }
        else
        {
            return "Not ready to resolve (Stage "
                   + std::to_string(cs.stage) + ")";
        }
    }

    // 2. Load Ships & Orders
    struct ShipCtx
    {
        std::string code;
        std::string name;
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
    auto r = db.query("SELECT ship_code, ship_name, owner, tech_level FROM "
                      "ships WHERE game_id=" +
                      std::to_string(game_id) + " AND at_hex='" + hex_id +
                      "' AND racked_in IS NULL AND destroyed_at IS NULL");
    for (const auto& row : r)
    {
        ShipCtx s;
        s.code = row[0];
        s.name = row[1];
        s.owner = row[2][0];
        s.tech = std::atoi(row[3].c_str());
        std::string key = std::string(1, s.owner) + "_" + s.code;
        ships[key] = s;
    }

    // Load orders
    auto ro = db.query(
        "SELECT ship_code, owner, tactic, target_id, power_d, power_b, "
        "power_s, power_t, missiles_data FROM combat_orders WHERE game_id=" +
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
            ships[key].ord.missiles_data = row[8];
        }
    }

    // Lookup system name for constraint check
    std::string combat_system;
    auto sys_row =
        db.query("SELECT name FROM star_systems WHERE module_id=1 AND hex_id='" +
                 db.esc(hex_id) + "' LIMIT 1");
    if (!sys_row.empty())
    {
        combat_system = sys_row[0][0];
    }

    std::ostringstream log;
    log << "\n=====\n";
    log << "       COMBAT ROUND " << cs.round << " RESOLUTION\n";
    log << "========\n";

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

                if (escaped) {
                    target->escape_successes++;
                }

                if (mod != -999)
                {
                    int dmg = ship.ord.power_b + ship.tech + mod;

                    // Apply combat constraint modifier for attacker
                    if (!combat_system.empty())
                    {
                        int constraint_mod =
                            ConstraintEngine::get_combat_modifier(
                                game_id, combat_system, ship.owner);
                        dmg += constraint_mod;
                    }

                    if (dmg > 0)
                    {
                        target->damage_received += dmg;
                        log << ship.code << " '" << ship.name << "' beams "
                            << target->code << " '" << target->name << "' for "
                            << dmg << " dmg!\n";
                    }
                }
                else
                {
                    if (escaped) {
                        log << target->code << " '" << target->name
                            << "' outruns " << ship.code << " '" << ship.name
                            << "' beams.\n";
                    }
                    else {
                        log << ship.code << " '" << ship.name << "' misses "
                            << target->code << " '" << target->name << "'.\n";
                    }
                }
            }
            else
            {
                log << ship.code << " has no target '" << tid << "'\n";
            }
        }
        else if (ship.ord.tactic == 'R')
        {
            // If retreating and NOT fired upon, do we need to track?
            // Logic: Escape if escaped ALL attacks. If 0 attacks, Escape =
            // Success.
            // BUGBUG
        }
    }

    // Missiles
    for (auto& [key, ship] : ships)
    {
        if (!ship.ord.missiles_data.empty() && ship.ord.missiles_data != "[]")
        {
            int count = std::atoi(ship.ord.missiles_data.c_str());
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
                        {
                            target->escape_successes++;
                        }

                        if (mod != -999)
                        {
                            int dmg = 3 + ship.tech + mod; // Base warhead 3?

                            // Apply combat constraint modifier for attacker
                            if (!combat_system.empty())
                            {
                                int constraint_mod =
                                    ConstraintEngine::get_combat_modifier(
                                        game_id, combat_system, ship.owner);
                                dmg += constraint_mod;
                            }

                            if (dmg > 0)
                            {
                                target->damage_received += dmg;
                                log << ship.code << " '" << ship.name
                                    << "' missile hits " << target->code << " '"
                                    << target->name << "' for " << dmg
                                    << " dmg!\n";
                            }
                        }
                        else
                        {
                            if (escaped) {
                                log << target->code << " '" << target->name
                                    << "' outruns " << ship.code << " '"
                                    << ship.name << "' missile.\n";
                            }
                            else {
                                log << ship.code << " '" << ship.name
                                    << "' missile misses " << target->code
                                    << " '" << target->name << "'.\n";
                            }
                        }
                    }
                    // BUGBUG
                    // Deduct ammo? Need to update ships table?
                    // For now simplest simulation: assume deducted elsewhere or
                    // strictly simulation.
                    // BUGBUG Didn't deal with permanent
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
        if (ship.ord.power_s > 0) {
            absorb = ship.ord.power_s +
                     ship.tech; // Tech adds to shields? Rules: "Screen Power +
                                // Tech Level (if powered)"
        }

        int net = std::max(0, ship.damage_received - absorb);
        if (net > 0)
        {
            if (!first) {
                dmgJson << ",";
            }

            // Uses owner_code key
            dmgJson << "\\\"" << key << "\\\":" << net; // Uses owner_code key
            total_net_damage += net;
            first = false;
            log << ship.code << " '" << ship.name << "' takes " << net
                << " net damage (Absorbed " << absorb << ").\n";
        }

        // Retreat Logic
        if (ship.ord.tactic == 'R')
        {
            bool escape = false;
            if (ship.escape_attempts == 0) {
                // Unopposed retreat
                escape = true;
            }
            else if (ship.escape_successes == ship.escape_attempts) {
                // Eluded all fire
                escape = true;
            }

            if (escape)
            {
                log << ship.code << " '" << ship.name << "' "
                    << "successfully retreats!\n";

                // Execute Retreat immediately? Or at end of round?
                // Rules: "Retreat Resolution".
                // We can mark them. For now, strict damage focus.
                // BUGBUG must resolve retreat resolution
            }
            else
            {
                log << ship.code << " '" << ship.name
                    << "' failed to retreat.\n";
                // BUGBUG means what?
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
        // DAMAGE_PENDING
        next_stage = 2;
        // Reset stalemate counter
        next_stalemate = 0;
        log << "RESULT:\n"
            << "Critical Damage!\n"
            << "Assign Damage Now.\n";

        // Build per-player damage summaries
        std::ostringstream dmgA;
        std::ostringstream dmgB;
        int dmgA_total = 0;
        int dmgB_total = 0;

        for (auto& [key, ship] : ships)
        {
            int absorb = 0;
            if (ship.ord.power_s > 0) 
            {
                absorb = ship.ord.power_s + ship.tech;
            }
            int net = std::max(0, ship.damage_received - absorb);

            if (net > 0)
            {
                if (ship.owner == 'A')
                {
                    dmgA << "  " << ship.code << " '" << ship.name
                         << "': " << net << " DAMAGE\n";
                    dmgA_total += net;
                }
                else
                {
                    dmgB << "  " << ship.code << " '" << ship.name
                         << "': " << net << " DAMAGE\n";
                    dmgB_total += net;
                }
            }
        }

        // Notify each player about their damage
        if (dmgA_total > 0)
        {
            Telemetry::getInstance().add_tell(
                game_id, 'A',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " DAMAGE REPORT\n" + dmgA.str() + "TOTAL: " +
                    std::to_string(dmgA_total) + " DAMAGE TO ASSIGN\n" +
                    ">> Use 'combat apply <ship> pd=N b=N...' to assign "
                    "damage.");
        }
        else
        {
            Telemetry::getInstance().add_tell(
                game_id, 'A',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " - YOUR SHIPS TOOK NO DAMAGE.");
        }

        if (dmgB_total > 0)
        {
            Telemetry::getInstance().add_tell(
                game_id, 'B',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " DAMAGE REPORT\n" + dmgB.str() + "TOTAL: " +
                    std::to_string(dmgB_total) + " DAMAGE TO ASSIGN\n" +
                    ">> Use 'combat apply <ship> pd=N b=N...' to assign "
                    "damage.");
        }
        else
        {
            Telemetry::getInstance().add_tell(
                game_id, 'B',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " - YOUR SHIPS TOOK NO DAMAGE.");
        }
    }
    else
    {
        next_stalemate++;
        log << "----------------------------------------\n";
        log << "RESULT: Stalemate (No Hull Damage).\n";

        // Check for 3 consecutive stalemates - per rules, initiative player
        // must retreat
        if (next_stalemate >= 3)
        {
            // RETREAT_PENDING
            next_stage = 3; // RETREAT_PENDING
            log << "STALEMATE: 3 rounds with no damage!\n";
            log << "Initiative player must withdraw all ships from this "
                   "hex.\n";

            // Notify initiative player they must retreat
            char initiative = StateMachine::getInstance().get_current_player();
            Telemetry::getInstance().add_tell(
                game_id, initiative,
                "3 consecutive stalemates. You must retreat your ships from "
                "hex " +
                    hex_id);
        }
        else
        {
            log << "STATUS: Beginning Round " << (next_round + 1) << "\n";
            log << "ACTION: Submit Orders for Round " << (next_round + 1)
                << "\n";
            log << "========================================\n";
            next_round++;

            // Notify both players next round begins
            Telemetry::getInstance().add_broadcast(
                "Combat Round " + std::to_string(next_round) +
                " begins in hex " + hex_id + ". Submit orders.");
        }
    }

    // Reset damage assignment flags for new damage phase
    std::string sql =
        "UPDATE combat_state SET round=" + std::to_string(next_round) +
        ", stage=" + std::to_string(next_stage) +
        ", stalemate_counter=" + std::to_string(next_stalemate) +
        ", pending_damage_json='" + dmgJson.str() + "'" +
        ", damage_assigned_A=0, damage_assigned_B=0" + ", last_log='" +
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
                      std::string(1, owner) + "' AND destroyed_at IS NULL");
    if (r.empty())
    {
        std::ostringstream snf;
        snf << "Ship " << ship_code << " not found";
        return std::string(snf.str());
    }

    // BUGBUG explain this dereferencing
    std::string hex = r[0][0];
    char realOwner = r[0][1][0];

    if (realOwner != owner)
    {
        std::ostringstream nys;
        nys << "Ship " << ship_code << " is not your ship.";
        return std::string(nys.str());
    }

    auto cs = get_combat_state(hex);
    if (cs.stage != 2) 
    {
        // 2=DAMAGE_PENDING
        return std::string("No pending damage for this hex");
    }

    // 2. Parse Pending Damage
    // JSON: {"A_S20": 4, "B_W1": 2} ... simple parse

    std::map<std::string, int> pending;
    std::string json = cs.pending_damage_json;
    // Remove braces
    if (json.size() >= 2)
    {
        json = json.substr(1, json.size() - 2);
    }
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
            {
                k = k.substr(q1 + 1, q2 - q1 - 1);
            }
            pending[k] = std::atoi(v.c_str());
        }
    }

    std::string key = std::string(1, owner) + "_" + ship_code;
    if (pending.find(key) == pending.end())
    {
        std::ostringstream ndp;
        ndp << "No damage pending for ship " << ship_code;
        return std::string(ndp.str());
    }

    int needed = pending[key];
    int assigned = 0;
    for (auto const& [k, v] : assignments)
    {
        assigned += v;
    }

    if (assigned > needed)
    {
        // BUGBUG resolve this discrepency:
        // Or allow over-assignment? Rules say "applied by owning
        // player". Assumes exact match needed?
        return std::string("Assigned more damage than required");
    }

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
            {
                updateSql += ",";
            }
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
        db.exec("UPDATE ships"
                " SET " + updateSql 
                + " WHERE game_id=" 
                + std::to_string(game_id)
                + " AND ship_code='"
                + db.esc(ship_code)
                + "'");
    }

    // Check Destruction
    if (cur_pd == 0 && cur_b == 0 && cur_s == 0 && cur_t == 0)
    {
        // Ship Destroyed - soft delete
        db.exec("UPDATE ships SET destroyed_at=NOW() WHERE game_id=" +
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
    {
        return "Must assign all " + std::to_string(needed) + " hits";
    }

    pending.erase(key);

    // Check if this player has any more pending damage (other ships)
    bool playerHasMoreDamage = false;
    for (auto const& [k, v] : pending)
    {
        if (k[0] == owner) // key format is "A_W1" or "B_S20"
        {
            playerHasMoreDamage = true;
            break;
        }
    }

    // Update player's damage_assigned flag if they have no more ships to assign
    std::string ownerFlag =
        (owner == 'A') ? "damage_assigned_A" : "damage_assigned_B";
    if (!playerHasMoreDamage)
    {
        db.exec("UPDATE combat_state SET " + ownerFlag + "=1 WHERE game_id=" +
                std::to_string(game_id) + " AND hex_id='" + hex + "'");

        // Notify opponent that this player finished damage assignment
        char opponent = (owner == 'A') ? 'B' : 'A';
        Telemetry::getInstance().add_tell(game_id, opponent,
                                          "Player " + std::string(1, owner) +
                                          " has assigned all damage.");
    }

    // Reconstruct JSON
    std::ostringstream newJson;
    newJson << "{";
    int c = 0;
    for (auto const& [k, v] : pending)
    {
        if (c++ > 0)
        {
            newJson << ",";
        }
        newJson << "\\\"" << k << "\\\":" << v;
    }
    newJson << "}";

    std::string next_json = newJson.str();
    int next_stage = 2; // Stay in damage
    int next_round = cs.round;

    if (pending.empty())
    {
        // All damage assigned by both players!
        // Back to orders for next round
        next_stage = 0;
        next_round++;

        // Check if combat should end (one side has no ships left)
        auto shipsA = db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                               std::to_string(game_id) + " AND at_hex='" + hex +
                               "' AND owner='A' AND destroyed_at IS NULL");
        auto shipsB = db.query("SELECT COUNT(*) FROM ships WHERE game_id=" +
                               std::to_string(game_id) + " AND at_hex='" + hex +
                               "' AND owner='B' AND destroyed_at IS NULL");
        int countA = std::atoi(shipsA[0][0].c_str());
        int countB = std::atoi(shipsB[0][0].c_str());

        if (countA == 0 || countB == 0)
        {
            // Combat ends - one side eliminated
            db.exec("DELETE FROM combat_state WHERE game_id=" +
                    std::to_string(game_id) + " AND hex_id='" + hex + "'");
            db.exec("UPDATE games SET active_combat_hex=NULL WHERE id=" +
                    std::to_string(game_id));

            std::string winner = (countA > 0) ? "A" : "B";
            Telemetry::getInstance().add_broadcast("Combat in hex " + hex +
                                                   " ends. Player " + winner +
                                                   " controls the hex.");

            std::ostringstream cvic;
            cvic << "Damage Applied. Combat ends - "
                 << winner
                 << " victorious!";
            return std::string(cvic.str());
        }

        // Combat continues - notify both players
        Telemetry::getInstance().add_broadcast(
            "All damage assigned. Combat Round " + std::to_string(next_round) +
            " begins in hex " + hex + ". Submit orders!");
    }

    db.exec("UPDATE combat_state SET stage=" + std::to_string(next_stage) +
            ", round=" + std::to_string(next_round) +
            ", pending_damage_json='" + next_json + "' WHERE game_id=" +
            std::to_string(game_id) + " AND hex_id='" + hex + "'");

    return std::string("Damage Applied");
}

bool CombatApplyCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    CombatOrderParams_t params;
    params.ship_code = m_ship_code;
    // Apply command type
    params.order_type = 0;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::COMBAT_FIRE,
                                                    &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Apply damage via combat engine
    CombatEngine ce(s.game_id);
    std::string result = ce.apply_damage(owner, m_ship_code, m_assignments);

    Telemetry::getInstance().write(result);
    return true;
}

bool CombatCancelCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Check if there are any uncommitted orders to cancel (Bug #1)
    auto orderRows =
        db.query("SELECT COUNT(*) FROM combat_orders WHERE game_id=" +
                 std::to_string(s.game_id) + " AND owner='" +
                 std::string(1, owner) + "' AND committed=0");

    if (orderRows.empty() || orderRows[0][0] == "0")
    {
        Telemetry::getInstance().write(
            "TACTICAL: No combat orders have been received.");
        return true;
    }

    // Delete all uncommitted orders for this player
    db.exec(
        "DELETE FROM combat_orders WHERE game_id=" + std::to_string(s.game_id) +
        " AND owner='" + std::string(1, owner) + "' AND committed=0");

    Telemetry::getInstance().write(
        "TACTICAL: Orders rescinded. Issue new commands.");
    return true;
}

bool CombatCommitCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Get the active combat hex
    auto gameRow = db.query("SELECT active_combat_hex FROM games WHERE id=" +
                            std::to_string(s.game_id));

    std::string activeHex;
    if (!gameRow.empty() && !gameRow[0][0].empty())
    {
        activeHex = gameRow[0][0];
    }

    // Get all hexes with uncommitted orders for this player
    auto hexRows = db.query("SELECT DISTINCT s.at_hex FROM combat_orders co "
                            "JOIN ships s ON s.game_id=co.game_id AND "
                            "s.owner=co.owner AND s.ship_code=co.ship_code "
                            "WHERE co.game_id=" +
                            std::to_string(s.game_id) + " AND co.owner='" +
                            std::string(1, owner) +
                            "' AND co.committed=0 AND s.destroyed_at IS NULL");

    if (hexRows.empty())
    {
        Telemetry::getInstance().write("TACTICAL: No combat orders queued.");
        return true;
    }

    // Validate orders are for active hex (if one is set)
    if (!activeHex.empty())
    {
        for (const auto& row : hexRows)
        {
            if (row[0] != activeHex)
            {
                Telemetry::getInstance().write(
                    "Error: Orders pending for hex " + row[0] +
                    " but active combat is in hex " + activeHex);
                return false;
            }
        }
    }

    // Mark all uncommitted orders as committed
    db.exec("UPDATE combat_orders SET committed=1 WHERE game_id=" +
            std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) +
            "' AND committed=0");

    Telemetry::getInstance().write("TACTICAL: Combat orders transmitted.");

    // Check each affected hex for resolution
    CombatEngine ce(s.game_id);
    for (const auto& row : hexRows)
    {
        std::string hex_id = row[0];
        auto cs = ce.get_combat_state(hex_id);

        if (ce.all_orders_committed(hex_id, cs.round))
        {
            // Reveal all orders to both players before resolution (per rules)
            auto orders = db.query(
                "SELECT co.owner, co.ship_code, co.tactic, co.target_id, "
                "co.power_d, co.power_b, co.power_s, co.power_t "
                "FROM combat_orders co "
                "JOIN ships s ON s.game_id=co.game_id AND "
                "s.ship_code=co.ship_code AND s.owner=co.owner "
                "WHERE co.game_id=" +
                std::to_string(s.game_id) + " AND s.at_hex='" + hex_id +
                "' AND co.round=" + std::to_string(cs.round) +
                " AND s.destroyed_at IS NULL ORDER BY co.owner, co.ship_code");

            std::ostringstream reveal;
            reveal << "=== COMBAT ORDERS REVEALED ===\n";
            for (const auto& ord : orders)
            {
                char t = ord[2][0];
                std::string tactic;
                switch(t) {
                    case 'A': tactic = "Attack"; break;
                    case 'D': tactic = "Attack"; break;
                    case 'E': tactic = "Attack"; break;
                } 
                reveal << "  " << ord[0] << ":" << ord[1] << " " << tactic
                       << " " << ord[3] << " [D=" << ord[4] << " B=" << ord[5]
                       << " S=" << ord[6] << " T=" << ord[7] << "]\n";
            }
            reveal << "==============================";
            Telemetry::getInstance().add_broadcast(s.game_id, reveal.str());

            std::string result = ce.resolve_round(hex_id);
            Telemetry::getInstance().write(result);
        }
        else
        {
            // Notify opponent that this player has committed
            char opponent = (owner == 'A') ? 'B' : 'A';
            Telemetry::getInstance().add_tell(
                opponent, "Player " + std::string(1, owner) +
                              " has committed combat orders for hex " + hex_id +
                              ". Use 'combat order' then 'combat commit' for "
                              "your ships.");

            Telemetry::getInstance().write("TACTICAL: Sector " + hex_id +
                                           " - Awaiting enemy orders.");
        }
    }
    return true;
}

bool CombatDraftsCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Query uncommitted orders for this player, grouped by hex
    auto rows = db.query(
        "SELECT s.at_hex, co.ship_code, co.tactic, co.target_id, "
        "co.power_d, co.power_b, co.power_s, co.power_t, co.missiles_data "
        "FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND "
        "s.ship_code=co.ship_code "
        "WHERE co.game_id=" +
        std::to_string(s.game_id) + " AND co.owner='" + std::string(1, owner) +
        "' AND co.committed=0 AND s.destroyed_at IS NULL "
        "ORDER BY s.at_hex, co.ship_code");

    if (rows.empty())
    {
        Telemetry::getInstance().write("No pending combat orders.");
        return true;
    }

    std::ostringstream out;
    out << "Pending Combat Orders:\n";
    std::string lastHex;
    for (const auto& r : rows)
    {
        if (r[0] != lastHex)
        {
            out << "  Hex " << r[0] << ":\n";
            lastHex = r[0];
        }
        char tactic = r[2].empty() ? 'A' : r[2][0];
        std::string tacticName =
            (tactic == 'A') ? "Attack" : (tactic == 'D') ? "Dodge" : "Escape";
        out << "    " << r[1] << ": " << tacticName << " -> "
            << (r[3].empty() ? "(none)" : r[3]) << " [D=" << r[4]
            << " B=" << r[5] << " S=" << r[6] << " T=" << r[7];
        if (!r[8].empty())
        {
            out << " M=" << r[8];
        }
        out << "]\n";
    }
    Telemetry::getInstance().write(out.str());
    return true;
}

bool CombatOrderCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    CombatOrderParams_t params;
    params.ship_code = m_ship_code;
    params.order_type = m_tactic;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::COMBAT_ORDER,
                                                    &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    // Check for self-targeting (Bug #9)
    if (m_ship_code == m_target_id)
    {
        Telemetry::getInstance().write(
            "TACTICAL: Cannot target your own ship!");
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char owner = StateMachine::getInstance().get_current_player();

    // Build CombatOrder struct
    CombatOrder order;
    order.game_id = s.game_id;
    order.ship_code = m_ship_code;
    order.target_id = m_target_id;
    order.tactic = m_tactic;
    order.round = 0; // Will be set by CombatEngine from current combat state
    order.power_d = m_power_d;
    order.power_b = m_power_b;
    order.power_s = m_power_s;
    order.power_t = m_power_t;
    order.missiles_data = m_missiles_json;

    // Submit order to combat engine
    CombatEngine ce(s.game_id);
    std::string result = ce.submit_order(owner, order);

    Telemetry::getInstance().write(result);

    return true;
}
