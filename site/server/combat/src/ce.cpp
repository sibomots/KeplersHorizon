//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "ce.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

#include "combat_apply_actor.h"
#include "combat_cancel_actor.h"
#include "combat_commit_actor.h"
#include "combat_drafts_actor.h"
#include "combat_order_actor.h"
#include "combatagent.h"
#include "constraints.h"
#include "db.h"
#include "hex_events.h"
#include "logger.h"
#include "mapgraph.h"
#include "maputil.h"
#include "moduleutil.h"
#include "star_system_constraints.h"
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
    DatabaseManager& db = DatabaseManager::instance();
    bool litmus = CombatEngine::is_real_combat_state(game_id);

    if (!litmus)
    {
        return;
    }

    {
        auto rows = db.Query("SELECT at_hex, owner FROM ships "
                             "WHERE game_id=? AND at_hex IS NOT NULL AND "
                             "(racked_in IS NULL OR racked_in = '')"
                             "AND destroyed_at IS NULL",
                             {game_id});

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
                auto exist = db.Query(
                    "SELECT 1 FROM combat_state WHERE game_id=? AND hex_id=?",
                    {game_id, hex});
                if (exist.empty())
                {
                    Logger::instance().info("[COMBAT] Creating combat at " +
                                            hex);
                    create_combat(hex);
                }
            }
        }
    }
}

// return vector of rows.
// each row has at_hex, owner of each ship involved in conflict
bool CombatEngine::is_real_combat_state(int gid)
{
    DatabaseManager& db = DatabaseManager::instance();
    bool real_combat = false;

    // Find out how many hexes are conflicted.

    auto conflict_rows = db.Query("SELECT at_hex, COUNT(DISTINCT owner) "
                                  "FROM ships "
                                  "WHERE game_id=? AND destroyed_at IS NULL "
                                  "AND (racked_in IS NULL OR racked_in = '') "
                                  "AND at_hex IS NOT NULL AND at_hex <> '' "
                                  "AND owner IN ('A','B') "
                                  "GROUP BY at_hex",
                                  {gid});

    // for each hex, look at the number of distinct ship owners at that hex.
    for (const std::vector<std::string>& row : conflict_rows)
    {
        int owner_count = std::stoi(row[1]);
        Logger::instance().info("[COMBAT] hex=" + row[0] + " distinct_owners=" +
                                std::to_string(owner_count));

        if (owner_count > 1)
        {
            real_combat = true;
        }
    }
    return real_combat;
}

void CombatEngine::create_combat(const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::instance();
    // Identify attacker? For now, we assume simple engagement.
    // In strict rules, the "moving player" is attacker.
    // We might need to passed that context in, or infer it (active player).
    // For now, default to active player being attacker is reasonable for new
    // combats initiated during their turn.

    // We need to fetch active player to set attacker_remains (if that implies
    // initiative). Let's just INSERT.
    db.Exec("INSERT INTO combat_state (game_id, hex_id, round, stage, "
            "attacker_remains, stalemate_counter) VALUES (?, ?, 1, 0, 0, 0)",
            {game_id, hex_id});
}

// The point here is that we are looking for potential combats
// "Active combat" is only set when attacker_remains is 1
// Initially, attacker_remains will be 0 before any Combat Orders are given!

std::vector<CombatState> CombatEngine::get_active_combats()
{
    DatabaseManager& db = DatabaseManager::instance();
    int gid = StateMachine::instance().get_game_id();
    std::vector<CombatState> result;

    std::string active_combat =
        "SELECT cs.hex_id, cs.round, cs.stage, cs.attacker_remains, "
        "cs.stalemate_counter, cs.last_log "
        "FROM combat_state AS cs "
        "JOIN games AS g ON g.id = cs.game_id "
        "JOIN star_systems AS ss ON ss.module_id = g.module_id "
        "AND ss.hex_id = cs.hex_id "
        "WHERE cs.game_id=? "
        "AND EXISTS (SELECT 1 FROM ships AS s "
        "WHERE s.game_id = cs.game_id AND s.at_hex = cs.hex_id "
        "AND s.destroyed_at IS NULL "
        "GROUP BY s.game_id, s.at_hex HAVING COUNT(DISTINCT s.owner)>1)";

    auto rows = db.Query(active_combat, {gid});
    for (const auto& r : rows)
    {
        result.emplace_back(game_id, r[0], std::atoi(r[1].c_str()),
                            std::atoi(r[2].c_str()), (r[3] == "1"),
                            std::atoi(r[4].c_str()), r[5]);
    }
    return result;
}

CombatState CombatEngine::get_combat_state(const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::instance();

    auto rows = db.Query(
        "SELECT round, stage, attacker_remains, stalemate_counter, last_log "
        "FROM combat_state WHERE game_id=? AND hex_id=?",
        {game_id, hex_id});

    if (rows.empty())
    {
        return {0, "", 0, 0, false, 0, ""};
    }

    return CombatState(game_id, hex_id, std::atoi(rows[0][0].c_str()),
                       std::atoi(rows[0][1].c_str()), (rows[0][2] == "1"),
                       std::atoi(rows[0][3].c_str()), rows[0][4]);
}

std::string CombatEngine::submit_order(char owner, const CombatOrder& order_in)
{
    DatabaseManager& db = DatabaseManager::instance();
    CombatOrder order = order_in;
    // 1. Validate ownership
    {
        auto r =
            db.Query("SELECT owner FROM ships WHERE game_id=? AND ship_code=? "
                     "AND destroyed_at IS NULL",
                     {game_id, order.ship_code});
        if (r.empty())
            return "Ship not found";

        char shipOwner = r[0][0][0];
        if (shipOwner != owner)
        {
            return "You do not own this ship";
        }
    }

    // 2. Validate Combat State & Stats
    // Find the hex this ship is in and its stats
    auto r = db.Query(
        "SELECT at_hex, pd, beam, screen, tube FROM ships WHERE game_id=? "
        "AND ship_code=? AND destroyed_at IS NULL",
        {game_id, order.ship_code});
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
    db.Exec(
        "INSERT INTO combat_orders (game_id, owner, ship_code, round, "
        "tactic, target_id, power_d, power_b, power_s, power_t, "
        "missiles_data, committed) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0) "
        "ON DUPLICATE KEY UPDATE tactic=?, target_id=?, power_d=?, power_b=?, "
        "power_s=?, power_t=?, missiles_data=?, committed=0",
        {game_id, owner, order.ship_code, order.round, order.tactic,
         order.target_id, order.power_d, order.power_b, order.power_s,
         order.power_t, order.missiles_data, order.tactic, order.target_id,
         order.power_d, order.power_b, order.power_s, order.power_t,
         order.missiles_data});

    // Resolution triggered by explicit 'combat commit'
    return "Order draft saved. Use 'combat commit' when ready.";
}

bool CombatEngine::all_orders_submitted(const std::string& hex_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();
    // Count ships in hex
    auto sr = db.Query(
        "SELECT COUNT(*) FROM ships WHERE game_id=? AND at_hex=? "
        "AND (racked_in IS NULL OR racked_in = '') AND destroyed_at IS NULL",
        {game_id, hex_id});
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count orders
    auto or_ = db.Query("SELECT COUNT(*) FROM combat_orders co "
                        "JOIN ships s ON s.game_id = co.game_id AND "
                        "s.ship_code = co.ship_code AND s.owner = co.owner "
                        "WHERE co.game_id=? AND co.round=? AND s.at_hex=? "
                        "AND s.destroyed_at IS NULL",
                        {game_id, round, hex_id});
    int orderCount = std::atoi(or_[0][0].c_str());

    Logger::instance().info("[COMBAT][all_orders_submitted] Hex " + hex_id +
                            " Round " + std::to_string(round) +
                            ": Ships=" + std::to_string(shipCount) +
                            " Orders=" + std::to_string(orderCount));

    return orderCount >= shipCount;
}

bool CombatEngine::all_orders_committed(const std::string& hex_id, int round)
{
    DatabaseManager& db = DatabaseManager::instance();
    // Count ships in hex
    auto sr = db.Query(
        "SELECT COUNT(*) FROM ships WHERE game_id=? AND at_hex=? "
        "AND (racked_in IS NULL OR racked_in = '') AND destroyed_at IS NULL",
        {game_id, hex_id});
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count COMMITTED orders
    auto or_ = db.Query("SELECT COUNT(*) FROM combat_orders co "
                        "JOIN ships s ON s.game_id = co.game_id AND "
                        "s.ship_code = co.ship_code AND s.owner = co.owner "
                        "WHERE co.game_id=? AND co.round=? AND co.committed=1 "
                        "AND s.at_hex=? AND s.destroyed_at IS NULL",
                        {game_id, round, hex_id});
    int commitCount = std::atoi(or_[0][0].c_str());

    Logger::instance().info("[COMBAT][all_orders_committed] Hex " + hex_id +
                            " Round " + std::to_string(round) +
                            ": Ships=" + std::to_string(shipCount) +
                            " Committed=" + std::to_string(commitCount));

    return commitCount >= shipCount;
}

// --- CRT Helper ---
static int get_crt_mod(int drive_diff, char tactic_fire, char tactic_target,
                       bool& escaped)
{
    DatabaseManager& db = DatabaseManager::instance();
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
            if (drive_diff <= -3)
            {
                return -999;
            }
            if (drive_diff <= -1)
            {
                return 0; // Hit
            }
            if (drive_diff <= 1)
            {
                return 2; // Hit+2
            }
            if (drive_diff == 2)
            {
                return 1; // Hit+1
            }
            return -999; // +3 or more Miss
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3)
            {
                return -999;
            }
            if (drive_diff <= 1)
            {
                return -999;
            }
            if (drive_diff == 2)
            {
                return 1; // Hit+1
            }
            if (drive_diff <= 4)
            {
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

            if (drive_diff <= 1)
            {
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
            if (drive_diff <= 2)
            {
                return -999; // Miss (Pinned)
            }
            if (drive_diff <= 4)
            {
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
            if (drive_diff <= -4)
            {
                return -999;
            }
            if (drive_diff <= -2)
            {
                return -999;
            }
            if (drive_diff <= 2)
            {
                return 0; // Hit
            }
            return -999;
        }
        if (tactic_target == 'D')
        {
            if (drive_diff <= -3)
            {
                return -999;
            }
            if (drive_diff == -2)
            {
                return 0; // Hit
            }
            if (drive_diff <= 0)
            {
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
            if (drive_diff <= -2)
            {
                return -999;
            }
            else if (drive_diff <= 0)
            {
                return 0; // Hit
            }
            else
            {
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
    DatabaseManager& db = DatabaseManager::instance();
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
            // Update stage in database for consistency, then proceed
            db.Exec(
                "UPDATE combat_state SET stage=1 WHERE game_id=? AND hex_id=?",
                {game_id, hex_id});
        }
        else
        {
            if (cs.stage == 0)
            {
                return "Awaiting orders. Use 'combat order' then 'combat "
                       "commit'.";
            }
            else if (cs.stage == 2)
            {
                return "Damage pending. Use 'combat apply' to assign damage.";
            }
            else if (cs.stage == 3)
            {
                return "Retreat pending. Use 'retreat <ship> <hex>' to "
                       "withdraw.";
            }
            return "Not ready to resolve (Stage " + std::to_string(cs.stage) +
                   ")";
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

    // Load active ships in hex (exclude ships pending retreat)
    auto r =
        db.Query("SELECT ship_code, ship_name, owner, tech_level FROM ships "
                 "WHERE game_id=? AND at_hex=? AND (racked_in IS NULL OR "
                 "racked_in = '') "
                 "AND destroyed_at IS NULL AND escape_pending=0",
                 {game_id, hex_id});
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
    auto ro = db.Query(
        "SELECT ship_code, owner, tactic, target_id, power_d, power_b, "
        "power_s, power_t, missiles_data FROM combat_orders "
        "WHERE game_id=? AND round=?",
        {game_id, cs.round});

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
    int mod = get_module_id_for_game(game_id);
    auto sys_row = db.Query(
        "SELECT name FROM star_systems WHERE module_id=? AND hex_id=? LIMIT 1",
        {mod, hex_id});
    if (!sys_row.empty())
    {
        combat_system = sys_row[0][0];
    }

    std::ostringstream log;
    log << "COMBAT ROUND " << cs.round << " RESOLUTION\n";

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
                {
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

                    // Apply star system environmental constraints
                    dmg +=
                        StarSystemConstraints::getBeamModifier(game_id, hex_id);

                    // Apply dynamic hex event modifier (COMBAT_INTERFERENCE)
                    dmg += HexEventEngine::get_combat_modifier(
                        game_id, cs.round, hex_id);

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
                    if (escaped)
                    {
                        log << target->code << " '" << target->name
                            << "' outruns " << ship.code << " '" << ship.name
                            << "' beams.\n";
                    }
                    else
                    {
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
            // Retreating ships hold fire - log acknowledgment
            log << ship.code << " '" << ship.name
                << "' holds fire (Retreating).\n";
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

                            // Apply star system environmental constraints
                            dmg += StarSystemConstraints::getMissileModifier(
                                game_id, hex_id);

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
                            if (escaped)
                            {
                                log << target->code << " '" << target->name
                                    << "' outruns " << ship.code << " '"
                                    << ship.name << "' missile.\n";
                            }
                            else
                            {
                                log << ship.code << " '" << ship.name
                                    << "' missile misses " << target->code
                                    << " '" << target->name << "'.\n";
                            }
                        }
                    }
                    // Deduct fired missiles from inventory
                    db.Exec(
                        "UPDATE ships SET missiles = GREATEST(0, missiles - ?) "
                        "WHERE game_id=? AND ship_code=?",
                        {count, game_id, ship.code});
                }
            }
        }
    }

    // 4. Calc Net Damage & Absorb
    int total_net_damage = 0;

    // Simply insert rows into pending_damage table - no JSON!
    for (auto& [key, ship] : ships)
    {
        // ... calculate absorb ...
        int absorb = 0;
        if (ship.ord.power_s > 0)
        {
            absorb = ship.ord.power_s +
                     ship.tech; // Tech adds to shields? Rules: "Screen Power +
                                // Tech Level (if powered)"
        }

        int net = std::max(0, ship.damage_received - absorb);

        if (net > 0)
        {
            // Insert one row per damaged ship
            db.Exec("INSERT INTO pending_damage "
                    "(game_id, hex_id, round, ship_code, owner, damage_amount) "
                    "VALUES (?, ?, ?, ?, ?, ?)",
                    {game_id, hex_id, cs.round, ship.code, ship.owner, net});

            total_net_damage += net;
            log << ship.code << " '" << ship.name << "' takes " << net
                << " net damage (Absorbed " << absorb << ").\n";
        }
    }

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
            Telemetry::instance().add_tell(
                game_id, 'A',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " DAMAGE REPORT\n" + dmgA.str() + "TOTAL: " +
                    std::to_string(dmgA_total) + " DAMAGE TO ASSIGN\n" +
                    ">> Use 'combat apply <ship> pd=N b=N...' to assign "
                    "damage.");
        }
        else
        {
            Telemetry::instance().add_tell(game_id, 'A',
                                           "TACTICAL: ROUND " +
                                               std::to_string(cs.round) +
                                               " - YOUR SHIPS TOOK NO DAMAGE.");
        }

        if (dmgB_total > 0)
        {
            Telemetry::instance().add_tell(
                game_id, 'B',
                "TACTICAL: ROUND " + std::to_string(cs.round) +
                    " DAMAGE REPORT\n" + dmgB.str() + "TOTAL: " +
                    std::to_string(dmgB_total) + " DAMAGE TO ASSIGN\n" +
                    ">> Use 'combat apply <ship> pd=N b=N...' to assign "
                    "damage.");
        }
        else
        {
            Telemetry::instance().add_tell(game_id, 'B',
                                           "TACTICAL: ROUND " +
                                               std::to_string(cs.round) +
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
            // Mark for retreat - user must issue 'retreat <ship> <hex>'
            // command

            GameState s = StateMachine::instance().get_game_state();
            int game_id = StateMachine::instance().get_game_id();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);

            // jdw char initiative =
            // StateMachine::instance().get_current_player();
            std::string dbg;
            dbg.append("Active player: ");
            dbg += active_player;
            dbg.append(" size of ships list: ");
            size_t sz = ships.size();
            dbg.append(std::to_string(sz));
            Logger::instance().info(dbg.c_str());

            for (auto& [key, ship] : ships)
            {
                // BUGBUG
                db.Exec("UPDATE ships SET escape_pending=1 "
                        "WHERE game_id=? AND ship_code=? AND owner=?",
                        {game_id, ship.code, active_player});
            }
            // Notify initiative player they must retreat
            Telemetry::instance().add_tell(
                game_id, active_player,
                "Three consecutive stalemates. You must retreat your ships "
                "from "
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
            Telemetry::instance().add_broadcast(
                "Combat Round " + std::to_string(next_round) +
                " begins in hex " + hex_id + ". Submit orders.");
        }
    }

    // Reset damage assignment flags for new damage phase
    db.Exec(
        "UPDATE combat_state SET round=?, stage=?, stalemate_counter=?, "
        "damage_assigned_A=0, damage_assigned_B=0, last_log=? "
        "WHERE game_id=? AND hex_id=?",
        {next_round, next_stage, next_stalemate, log.str(), game_id, hex_id});

    // Append log to game events?
    return log.str();
}

std::string CombatEngine::apply_damage(char owner, const std::string& ship_code,
                                       const AttributeMap& assignments)
{
    DatabaseManager& db = DatabaseManager::instance();

    // 1. Get ship current stats and location
    auto ship_rows =
        db.Query("SELECT at_hex, pd, beam, screen, tube, missiles, sr "
                 "FROM ships WHERE game_id=? AND ship_code=? AND owner=? "
                 "AND destroyed_at IS NULL",
                 {game_id, ship_code, owner});

    if (ship_rows.empty())
    {
        return "Ship " + ship_code + " not found";
    }

    std::string hex = ship_rows[0][0];
    int cur_pd = std::atoi(ship_rows[0][1].c_str());
    int cur_beam = std::atoi(ship_rows[0][2].c_str());
    int cur_screen = std::atoi(ship_rows[0][3].c_str());
    int cur_tube = std::atoi(ship_rows[0][4].c_str());
    int cur_missiles = std::atoi(ship_rows[0][5].c_str());

    // 2. Verify combat state
    auto cs = get_combat_state(hex);
    if (cs.stage != 2)
    { // 2 = DAMAGE_PENDING
        return "No pending damage for this hex";
    }

    // 3. Get pending damage for this ship - ONE SIMPLE QUERY!
    auto dmg_rows = db.Query(
        "SELECT damage_amount FROM pending_damage "
        "WHERE game_id=? AND hex_id=? AND round=? AND ship_code=? AND owner=?",
        {game_id, hex, cs.round, ship_code, owner});

    if (dmg_rows.empty())
    {
        return "No pending damage for ship " + ship_code;
    }

    int damage_needed = std::atoi(dmg_rows[0][0].c_str());

    // 4. Validate assignments - iterate through AttributeMap.data
    int current_hp = cur_pd + cur_beam + cur_screen + cur_tube;
    int assigned = 0;

    for (const auto& [attr_id, dmg] : assignments.data)
    {
        assigned += dmg;
    }

    // Check assignment rules
    if (assigned > damage_needed)
    {
        if (assigned < current_hp)
        {
            return "Cannot over-assign damage. Ship has " +
                   std::to_string(current_hp) + " HP but only took " +
                   std::to_string(damage_needed) + " damage.";
        }
    }
    else if (assigned < damage_needed)
    {
        if (assigned < current_hp)
        {
            return "Must assign all " + std::to_string(damage_needed) +
                   " damage. Currently assigned: " + std::to_string(assigned);
        }
    }

    // 5. Apply damage to ship attributes
    std::vector<std::string> updates;

    auto apply_attr =
        [&](AttributeID attr_id, int cur_val, const std::string& col_name)
    {
        auto it = assignments.data.find(attr_id);
        if (it != assignments.data.end())
        {
            int dmg = it->second;
            int new_val;

            if (attr_id == AttributeID::MISSILE)
            {
                // Missiles: each hit destroys 3 missiles
                int loss = dmg * 3;
                if (cur_val < 3 && dmg > 0)
                {
                    loss = cur_val; // Remaining 1-2 absorbed by 1 hit
                }
                new_val = std::max(0, cur_val - loss);
            }
            else
            {
                new_val = std::max(0, cur_val - dmg);
            }

            updates.push_back(col_name + "=" + std::to_string(new_val));
        }
    };

    apply_attr(AttributeID::POWER_DRIVE, cur_pd, "pd");
    apply_attr(AttributeID::BEAM, cur_beam, "beam");
    apply_attr(AttributeID::SCREEN, cur_screen, "screen");
    apply_attr(AttributeID::TUBE, cur_tube, "tube");
    apply_attr(AttributeID::MISSILE, cur_missiles, "missiles");

    // 6. Execute ship updates
    if (!updates.empty())
    {
        std::string update_sql = "UPDATE ships SET ";
        for (size_t i = 0; i < updates.size(); i++)
        {
            if (i > 0)
                update_sql += ", ";
            update_sql += updates[i];
        }
        update_sql += " WHERE game_id=? AND ship_code=? AND owner=?";
        db.Exec(update_sql, {game_id, ship_code, owner});
    }

    // 7. Check for ship destruction
    auto check_rows = db.Query("SELECT pd, beam, screen, tube FROM ships "
                               "WHERE game_id=? AND ship_code=? AND owner=?",
                               {game_id, ship_code, owner});

    int new_pd = std::atoi(check_rows[0][0].c_str());
    int new_beam = std::atoi(check_rows[0][1].c_str());
    int new_screen = std::atoi(check_rows[0][2].c_str());
    int new_tube = std::atoi(check_rows[0][3].c_str());

    if (new_pd == 0 && new_beam == 0 && new_screen == 0 && new_tube == 0)
    {
        // Ship destroyed
        db.Exec("UPDATE ships SET destroyed_at=NOW() "
                "WHERE game_id=? AND ship_code=? AND owner=?",
                {game_id, ship_code, owner});

        // Award VP to enemy
        char enemy = (owner == 'A') ? 'B' : 'A';
        std::string vp_col = (enemy == 'A') ? "vp_A" : "vp_B";
        db.Exec("UPDATE games SET " + vp_col + "=" + vp_col + "+1 WHERE id=?",
                {game_id});

        Telemetry::instance().add_tell(
            game_id, enemy,
            "VICTORY: +1 VP for destroying enemy ship " + ship_code);

        Logger::instance().info("Destroyed ship " + ship_code + " owned by " +
                                std::string(1, owner) + " at hex " + hex);
    }

    // 8. Delete pending damage entry for this ship
    db.Exec(
        "DELETE FROM pending_damage "
        "WHERE game_id=? AND hex_id=? AND round=? AND ship_code=? AND owner=?",
        {game_id, hex, cs.round, ship_code, owner});

    // 9. Check if player has more pending damage
    auto remaining_rows =
        db.Query("SELECT COUNT(*) FROM pending_damage "
                 "WHERE game_id=? AND hex_id=? AND round=? AND owner=?",
                 {game_id, hex, cs.round, owner});

    int remaining_count = std::atoi(remaining_rows[0][0].c_str());

    // 10. Update player's damage_assigned flag if done
    if (remaining_count == 0)
    {
        std::string flag =
            (owner == 'A') ? "damage_assigned_A" : "damage_assigned_B";
        db.Exec("UPDATE combat_state SET " + flag +
                    "=1 "
                    "WHERE game_id=? AND hex_id=?",
                {game_id, hex});

        char opponent = (owner == 'A') ? 'B' : 'A';
        Telemetry::instance().add_tell(game_id, opponent,
                                       "Player " + std::string(1, owner) +
                                           " has assigned all damage.");
    }

    // 11. Check if ALL damage is assigned (both players)
    auto both_done = db.Query(
        "SELECT damage_assigned_A, damage_assigned_B FROM combat_state "
        "WHERE game_id=? AND hex_id=?",
        {game_id, hex});

    bool a_done = (both_done[0][0] == "1");
    bool b_done = (both_done[0][1] == "1");

    if (a_done && b_done)
    {
        // Check for combat end or next round
        auto ships_a = db.Query("SELECT COUNT(*) FROM ships "
                                "WHERE game_id=? AND at_hex=? AND owner='A' "
                                "AND destroyed_at IS NULL",
                                {game_id, hex});
        auto ships_b = db.Query("SELECT COUNT(*) FROM ships "
                                "WHERE game_id=? AND at_hex=? AND owner='B' "
                                "AND destroyed_at IS NULL",
                                {game_id, hex});

        int count_a = std::atoi(ships_a[0][0].c_str());
        int count_b = std::atoi(ships_b[0][0].c_str());

        if (count_a == 0 || count_b == 0)
        {
            // Combat ends
            db.Exec("DELETE FROM combat_state WHERE game_id=? AND hex_id=?",
                    {game_id, hex});
            db.Exec("UPDATE games SET active_combat_hex=NULL WHERE id=?",
                    {game_id});

            std::string winner = (count_a > 0) ? "A" : "B";
            Telemetry::instance().add_broadcast(
                game_id, "Combat in hex " + hex + " ends. Player " + winner +
                             " controls the hex.");

            return "Damage Applied. Combat ends - " + winner + " victorious!";
        }

        // Combat continues - next round
        int next_round = cs.round + 1;
        db.Exec("UPDATE combat_state SET stage=0, round=?, "
                "damage_assigned_A=0, damage_assigned_B=0 "
                "WHERE game_id=? AND hex_id=?",
                {next_round, game_id, hex});

        Telemetry::instance().add_broadcast(
            game_id, "All damage assigned. Combat Round " +
                         std::to_string(next_round) + " begins in hex " + hex +
                         ". Submit orders!");
    }

    return "Damage Applied";
}
