#ifndef COMBAT_H
#define COMBAT_H

#include "db.h"
#include <string>
#include <vector>
#include <map>

// Represents the state of combat in a specific hex
struct CombatState {
    int game_id;
    std::string hex_id;
    int round;
    int stage; // 0=ORDERS, 1=RESOLVE_READY, 2=DAMAGE_PENDING, 3=RETREAT_PENDING
    bool attacker_remains;
    int stalemate_counter;
    std::string pending_damage_json;
    std::string last_log;

    CombatState() : game_id(0), round(0), stage(0), attacker_remains(false), stalemate_counter(0) {}

    CombatState(int gid, std::string hex, int rnd, int stg, bool att, int stale, std::string pend, std::string log)
        : game_id(gid), hex_id(hex), round(rnd), stage(stg), attacker_remains(att), stalemate_counter(stale), pending_damage_json(pend), last_log(log) {}
};

// Represents a single ship's secret order for a round
struct CombatOrder {
    int game_id;
    char owner; // 'A' or 'B'
    std::string ship_code;
    int round;
    char tactic; // 'A', 'D', 'R'
    std::string target_id;
    int power_d;
    int power_b;
    int power_s;
    int power_t;
    std::string missiles_json;
};

class CombatEngine {
public:
    CombatEngine(Db* db, int game_id);

    // Initial checks
    void check_for_combat_triggers();
    
    // State retrieval
    std::vector<CombatState> get_active_combats();
    CombatState get_combat_state(const std::string& hex_id);
    
    // Order handling
    std::string submit_order(char owner, const CombatOrder& order);
    
    // Resolution (called when stage advances)
    // Returns event log text
    std::string resolve_round(const std::string& hex_id); 
    
    // Damage Assignment
    std::string apply_damage(char owner, const std::string& ship_code, const std::map<std::string, int>& assignments);

private:
    Db* db;
    int game_id;

    // Helpers
    bool all_orders_submitted(const std::string& hex_id, int round);
    void create_combat(const std::string& hex_id);
};

#endif
