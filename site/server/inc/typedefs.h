//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_TYPEDEFS_H__
#define __KH_TYPEDEFS_H__

#include <map>

// Belongs in the typedefs collection
typedef enum : int
{
    UNDEFINED,
    LEARNING,
    BASIC,
    ADVANCED,
} ScenarioType;

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

#ifndef SafeDelete
#define SafeDelete(x)                                                          \
    do                                                                         \
    {                                                                          \
        if ((x) != NULL)                                                       \
        {                                                                      \
            delete (x);                                                        \
            (x) = NULL;                                                        \
        }                                                                      \
    } while (0)
#endif

#ifndef SafeDeleteA
#define SafeDeleteA(x)                                                         \
    do                                                                         \
    {                                                                          \
        if ((x) != NULL)                                                       \
        {                                                                      \
            delete[] (x);                                                      \
            (x) = NULL;                                                        \
        }                                                                      \
    } while (0)
#endif

#endif
