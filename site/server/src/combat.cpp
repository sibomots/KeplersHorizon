#include "combat.h"
#include "util.h"
#include <iostream>
#include <set>
#include <algorithm>

CombatEngine::CombatEngine(Db* db, int game_id) : db(db), game_id(game_id) {}

void CombatEngine::check_for_combat_triggers() {
    // 1. Find all hexes containing ships from both players (A and B)
    //    We can do this by selecting all ship locations and grouping.
    auto rows = db->query(
        "SELECT at_hex, owner FROM ships "
        "WHERE game_id=" + std::to_string(game_id) + " "
        "AND at_hex IS NOT NULL AND racks_ship IS NULL" // Only active ships in space
    );

    std::map<std::string, std::set<char>> hexOccupants;
    for(const auto& r : rows) {
        std::string h = r[0];
        char o = r[1][0];
        hexOccupants[h].insert(o);
    }

    for(auto const& [hex, occupants] : hexOccupants) {
        // If both A and B are present
        if (occupants.count('A') && occupants.count('B')) {
            // Check if combat already exists
            auto exist = db->query("SELECT 1 FROM combat_state WHERE game_id=" + std::to_string(game_id) + " AND hex_id='" + hex + "'");
            if (exist.empty()) {
                create_combat(hex);
            }
        }
    }
}

void CombatEngine::create_combat(const std::string& hex_id) {
    // Identify attacker? For now, we assume simple engagement.
    // In strict rules, the "moving player" is attacker. 
    // We might need to passed that context in, or infer it (active player).
    // For now, default to active player being attacker is reasonable for new combats initiated during their turn.
    
    // We need to fetch active player to set attacker_remains (if that implies initiative).
    // Let's just INSERT.
    db->exec(
        "INSERT INTO combat_state (game_id, hex_id, round, stage, attacker_remains, stalemate_counter) "
        "VALUES (" + std::to_string(game_id) + ", '" + hex_id + "', 1, 0, 0, 0)"
    );
}

std::vector<CombatState> CombatEngine::get_active_combats() {
    std::vector<CombatState> result;
    auto rows = db->query(
        "SELECT hex_id, round, stage, attacker_remains, stalemate_counter, pending_damage_json "
        "FROM combat_state WHERE game_id=" + std::to_string(game_id)
    );
    
    for(const auto& r : rows) {
        CombatState cs;
        cs.game_id = game_id;
        cs.hex_id = r[0];
        cs.round = std::atoi(r[1].c_str());
        cs.stage = std::atoi(r[2].c_str());
        cs.attacker_remains = (r[3] == "1");
        cs.stalemate_counter = std::atoi(r[4].c_str());
        cs.pending_damage_json = r[5];
        result.push_back(cs);
    }
    return result;
}

CombatState CombatEngine::get_combat_state(const std::string& hex_id) {
    auto rows = db->query(
        "SELECT round, stage, attacker_remains, stalemate_counter, pending_damage_json "
        "FROM combat_state WHERE game_id=" + std::to_string(game_id) + " AND hex_id='" + hex_id + "'"
    );
    if(rows.empty()) return {0, "", 0, 0, false, 0, ""};
    
    CombatState cs;
    cs.game_id = game_id;
    cs.hex_id = hex_id;
    cs.round = std::atoi(rows[0][0].c_str());
    cs.stage = std::atoi(rows[0][1].c_str());
    cs.attacker_remains = (rows[0][2] == "1");
    cs.stalemate_counter = std::atoi(rows[0][3].c_str());
    cs.pending_damage_json = rows[0][4];
    return cs;
}

std::string CombatEngine::submit_order(char owner, const CombatOrder& order) {
    // 1. Validate ownership
    {
        auto r = db->query("SELECT owner FROM ships WHERE game_id=" + std::to_string(game_id) + 
            " AND ship_code='" + db->esc(order.ship_code) + "'");
        if (r.empty()) return "Ship not found";
        
        char shipOwner = r[0][0][0];
        if (shipOwner != owner) return "You do not own this ship";
    }

    // 2. Validate Combat State
    // Find the hex this ship is in.
    auto r = db->query("SELECT at_hex FROM ships WHERE game_id=" + std::to_string(game_id) + " AND ship_code='" + db->esc(order.ship_code) + "'");
    if(r.empty() || r[0][0].empty()) return "Ship not in space";
    std::string hex_id = r[0][0];

    auto cs = get_combat_state(hex_id);
    if (cs.game_id == 0) return "No combat in this hex";
    if (cs.stage != 0) return "Not accepting orders (Stage " + std::to_string(cs.stage) + ")";
    if (cs.round != order.round) return "Wrong round";

    // 3. Insert/Update
    db->exec("INSERT INTO combat_orders (game_id, ship_code, round, tactic, target_id, power_d, power_b, power_s, power_t, missiles_json) VALUES (" +
        std::to_string(game_id) + ", '" +
        order.ship_code + "', " +
        std::to_string(order.round) + ", '" +
        std::string(1, order.tactic) + "', '" +
        order.target_id + "', " +
        std::to_string(order.power_d) + ", " +
        std::to_string(order.power_b) + ", " +
        std::to_string(order.power_s) + ", " +
        std::to_string(order.power_t) + ", '" +
        order.missiles_json + "') "
        "ON DUPLICATE KEY UPDATE "
        "tactic='" + std::string(1, order.tactic) + "', "
        "power_d=" + std::to_string(order.power_d) + ","
        "power_b=" + std::to_string(order.power_b) + ","
        "power_s=" + std::to_string(order.power_s) + ","
        "power_t=" + std::to_string(order.power_t) + ","
        "missiles_json='" + order.missiles_json + "'"
    );
    
    // 4. Check completion
    if (all_orders_submitted(hex_id, order.round)) {
        db->exec("UPDATE combat_state SET stage=1 WHERE game_id=" + std::to_string(game_id) + " AND hex_id='" + hex_id + "'");
        return "Orders Saved. Combat Ready to Resolve.";
    }

    return "Order Saved";
}

bool CombatEngine::all_orders_submitted(const std::string& hex_id, int round) {
    // Count ships in hex
    auto sr = db->query("SELECT COUNT(*) FROM ships WHERE game_id=" + std::to_string(game_id) + " AND at_hex='" + hex_id + "' AND racks_ship IS NULL");
    int shipCount = std::atoi(sr[0][0].c_str());

    // Count orders
    auto or_ = db->query("SELECT COUNT(DISTINCT co.ship_code) FROM combat_orders co "
        "JOIN ships s ON s.ship_code = co.ship_code "
        "WHERE co.game_id=" + std::to_string(game_id) + " AND co.round=" + std::to_string(round) + 
        " AND s.at_hex='" + hex_id + "'");
    int orderCount = std::atoi(or_[0][0].c_str());

    return orderCount >= shipCount;
}

std::string CombatEngine::resolve_round(const std::string& hex_id) {
    // Complex CRT logic goes here.
    return "Combat resolved (Stub)";
}

std::string CombatEngine::apply_damage(char owner, const std::string& ship_code, const std::map<std::string, int>& assignments) {
    return "Damage applied (Stub)";
}
