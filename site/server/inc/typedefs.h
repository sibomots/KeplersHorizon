//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_TYPEDEFS_H__
#define __KH_TYPEDEFS_H__

#include <algorithm>
#include <map>
#include <ostream>
#include <sstream>
#include <string>

#include "json.h"

// clang-format off
#ifndef SafeDelete
#define SafeDelete(x) do {if((x)!=NULL){delete (x); (x)=NULL;}}while(0);
#endif
#ifndef SafeDeleteA
#define SafeDeleteA(x) do {if((x)!=NULL){delete [] (x); (x)=NULL;}}while(0);
#endif
// clang-format on


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
    PH_MOVEMENT = 1,
    PH_RESOLVE_COMBAT = 2,
    PH_SYSTEM_PICKDROP = 3,
    PH_END_TURN = 4
};

// Ship attribute identifiers
enum class AttributeID : int
{
    POWER_DRIVE,
    BEAM,
    SCREEN,
    TUBE,
    MISSILE,
    SYSTEM_RACK
};

// Attribute value type (all attributes are currently int)
typedef int AttributeValue;

// Map of attributes for commands
typedef std::map<AttributeID, AttributeValue> AttributeMap;

typedef std::pair<int, int> FiringMissile;
typedef std::vector<FiringMissile> MissileSet;

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
    COMBAT_FIRE,
    SAVE,
    LOAD,
    ACCEPT,
    REJECT,
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

// end typedefs
#endif
