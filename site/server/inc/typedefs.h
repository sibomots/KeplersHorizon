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

typedef enum : int
{
    UNDEFINED,
    LEARNING,
    BASIC,
    ADVANCED,
} ScenarioType;

typedef struct
{
    std::string dbhost;
    std::string dbuser;
    std::string dbpass;
    std::string dbname;
} DBConfig;

typedef struct
{
    unsigned short port;
} ServerConfig;

// Kepler's Horizon phase sequencing: VP count is implicit at start-of-turn;
// player-facing phases begin at Build Ships.
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


// end typedefs
#endif
