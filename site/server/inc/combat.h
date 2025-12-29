///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
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
    std::string pending_damage_json;
    std::string last_log;

    CombatState()
        : game_id(0), round(0), stage(0), attacker_remains(false),
          stalemate_counter(0)
    {
    }

    CombatState(int gid, std::string hex, int rnd, int stg, bool att, int stale,
                std::string pend, std::string log)
        : game_id(gid), hex_id(hex), round(rnd), stage(stg),
          attacker_remains(att), stalemate_counter(stale),
          pending_damage_json(pend), last_log(log)
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
    char tactic; // 'A', 'D', 'R'
    std::string target_id;
    int power_d;
    int power_b;
    int power_s;
    int power_t;
    std::string missiles_json;
};

class CombatEngine
{
  public:
    CombatEngine(Db *db, int game_id);

    // Initial checks
    void check_for_combat_triggers();

    // State retrieval
    std::vector<CombatState> get_active_combats();
    CombatState get_combat_state(const std::string &hex_id);

    // Order handling
    std::string submit_order(char owner, const CombatOrder &order);

    // Resolution (called when stage advances)
    // Returns event log text
    std::string resolve_round(const std::string &hex_id);

    // Damage Assignment
    std::string apply_damage(char owner, const std::string &ship_code,
                             const std::map<std::string, int> &assignments);

  private:
    Db *db;
    int game_id;

    // Helpers
    bool all_orders_submitted(const std::string &hex_id, int round);
    void create_combat(const std::string &hex_id);
};

#endif
