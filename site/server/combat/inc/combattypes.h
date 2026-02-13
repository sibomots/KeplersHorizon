///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMBAT_TYPES_H__
#define __KH_COMBAT_TYPES_H__

#include <string>
#include <variant>

typedef enum : int
{
    KH_CNOP = 0,
    KH_CSTATUS,
    KH_CORDER,
    KH_CCOMMIT,
    KH_CCANCEL,
    KH_CAPPLY,
    KH_CDRAFTS,
    KH_CRETR
} CombatOpCode;

typedef enum : char
{
    KH_N_TACTIC = 0,
    KH_A_TACTIC = 'A', // attack
    KH_D_TACTIC = 'D', // dodge
    KH_R_TACTIC = 'E', // escape
} CombatOpTactic;

typedef enum : int
{
    NONE = 0,
    ORDERS,
    RESOLVE_READY,
    DAMAGE_PENDING,
    RETREAT_PENDING,
} CombatStage;

// Represents the state of combat in a specific hex
typedef struct CombatSessionState
{
    int game_id;
    int module_id;
    std::string hex_id;
    int round;
    int stage;
    bool attacker_remains;
    int stalemate_counter;
    std::string last_log;

    CombatSessionState()
        : game_id(0), round(0), stage(CombatStage::NONE),
          attacker_remains(false), stalemate_counter(0)
    {
        hex_id = {};
        last_log = {};
    }

    CombatSessionState(int gid, std::string hex, int rnd, int stg, bool att,
                       int stale, std::string log)
        : game_id(gid), hex_id(hex), round(rnd), stage(stg),
          attacker_remains(att), stalemate_counter(stale), last_log(log)
    {
    }
} CombatSessionState;

#endif
