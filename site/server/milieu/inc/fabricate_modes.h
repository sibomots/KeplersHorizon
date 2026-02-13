///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_FABRICATE_MODES_H__
#define __KH_FABRICATE_MODES_H__

#include <string>
#include "typedefs.h"

typedef enum : int
{
    FIRST_FABRICATE_MODE = 0,
    LIST_PLANS = 0,
    FABRICATE_MISSILE,
    FABRICATE_TUBE,
    FABRICATE_BEAM,
    FABRICATE_SCREEN,
    FABRICATE_TECH,
    FABRICATE_DRONE,
    LAST_FABRICATE_MODE
} FabricateMode;


#endif
