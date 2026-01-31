//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef COMBAT_H
#define COMBAT_H

#include <map>
#include <string>
#include <vector>

#include "db.h"

// Represents the state of combat in a specific hex
struct CombatState
{
    int game_id;
    std::string hex_id;
    int round;
    int stage; // 0=ORDERS, 1=RESOLVE_READY, 2=DAMAGE_PENDING, 3=RETREAT_PENDING
    bool attacker_remains;
    int stalemate_counter;
    std::string last_log;

    CombatState()
        : game_id(0), round(0), stage(0), attacker_remains(false),
          stalemate_counter(0)
    {
    }

    CombatState(int gid, std::string hex, int rnd, int stg, bool att, int stale,
                std::string log)
        : game_id(gid), hex_id(hex), round(rnd), stage(stg),
          attacker_remains(att), stalemate_counter(stale), last_log(log)
    {
    }
};

// Represents a single ship's secret order for a round
struct CombatOrder
{
    int game_id;
    char owner; // 'A' or 'B'
    std::string ship_code;
    int round;
    char tactic; // 'A', 'D', 'E'
    std::string target_id;
    int power_d;
    int power_b;
    int power_s;
    int power_t;
    std::string missiles_data; // Comma-separated drive values, e.g. "4,5,3"
};

class CombatEngine
{
  public:
    CombatEngine(int game_id);

    // Initial checks
    void check_for_combat_triggers();

    // State retrieval
    std::vector<CombatState> get_active_combats();
    CombatState get_combat_state(const std::string& hex_id);


    // A test if any ships are actually in combat (has to be ON STAR HEX)
    static bool is_real_combat_state(int gid);

    // Order handling
    std::string submit_order(char owner, const CombatOrder& order);

    // Resolution (called when stage advances)
    // Returns event log text
    std::string resolve_round(const std::string& hex_id);

    // Damage Assignment
    std::string apply_damage(char owner, const std::string& ship_code,
                             const AttributeMap& assignments);

    // Commit checking
    bool all_orders_committed(const std::string& hex_id, int round);

  private:
    int game_id;

    // Helpers
    bool all_orders_submitted(const std::string& hex_id, int round);
    void create_combat(const std::string& hex_id);
};

#endif
