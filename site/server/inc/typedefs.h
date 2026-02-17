///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_TYPEDEFS_H__
#define __KH_TYPEDEFS_H__

#include <algorithm>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "attributemap.h"
#include "json.h"

// clang-format off
#ifndef SafeDelete
#define SafeDelete(x) do {if((x)!=NULL){delete (x); (x)=NULL;}}while(0);
#endif
#ifndef SafeDeleteA
#define SafeDeleteA(x) do {if((x)!=NULL){delete [] (x); (x)=NULL;}}while(0);
#endif
// clang-format on

#define KH_EQU(x,y) ((x)==(y))
#define KH_NEQ(x,y) ((x)!=(y))

typedef struct DataConfig
{
    // Drop all tables (game + milieu)
    bool clean;
    // Create all tables (game + milieu)
    bool schema;
    // Path to game seed CSVs (empty = skip)
    std::string seed_game_path;
    // Path to milieu seed CSVs (empty = skip)
    std::string seed_milieu_path;

    DataConfig()
    {
        // Paths are empty by default (no seeding)
        clean = false;
        schema = false;
    }
} DataConfig;

// Kepler's Horizon phase sequencing:
//  VP count is implicit at start-of-turn;
//  player-facing phases begin at Build Ships.
enum PhaseIndex
{
    PH_BUILD_SHIPS = 0,
    PH_MOVEMENT,
    PH_RESOLVE_COMBAT,
    PH_SYSTEM_PICKDROP,
    PH_END_TURN
};

typedef int FiringTorpedo;
typedef std::vector<FiringTorpedo> TorpedoSet;

// Command IDs for the check_inhibits() system
enum class CommandID
{
    BUILD_NEW,
    BUILD_SET,
    BUILD_COMMIT,
    DEPLOY,
    MOVE,
    NEXT,
    DONE,
    STATUS,
    HELP,
    COMBAT_ORDER,
    COMBAT_CANCEL,
    COMBAT_COMMIT,
    COMBAT_DRAFTS,
    COMBAT_APPLY,
    COMBAT_FIRE,
    SAVE,
    LOAD,
    ACCEPT,
    REJECT,
    PICK,
    DROP,
    CARGO,
    EXTRACT,
    SYSTEM,
    GALAXY,
    TRADE,
    FABRICATE,
    // Add more as needed
};

// Parameter type definitions for check_inhibits (typedef these types)
typedef struct
{
    char ship_type;
    std::string ship_name;
} BuildNewParams_t;

typedef struct
{
    std::string ship_code;
    std::string ship_name;
    AttributeMap attributes;
} BuildSetParams_t;

typedef struct
{
    std::string ship_code;
} BuildCommitParams_t;

typedef struct
{
    std::string ship_code;
    std::string destination;
} DeployParams_t;

typedef struct
{
    std::string ship_code;
    std::string destination;
} MoveParams_t;

typedef struct
{ /* empty - no parameters */
} NextParams_t;

typedef struct
{ /* empty - no parameters */
} DoneParams_t;

typedef struct
{ /* empty - no parameters */
} StatusParams_t;

typedef struct
{ /* empty - no parameters */
} HelpParams_t;

typedef struct
{
    std::string ship_code;
    int order_type;
} CombatOrderParams_t;

typedef struct
{
    const char* type;
    int base_price;
} CommodityItem;

typedef std::pair<char, char> FightingPlayers;

#endif
