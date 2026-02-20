///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_LOCALISATION_H__
#define __KH_LOCALISATION_H__

// LOCALE is selected at build time via CMake -DLOCALE=en_US (default).
// CMake defines exactly one of: LOCALE_EN_US, LOCALE_DE_DE, etc.

#if defined(LOCALE_EN_US)
    #include "string_en_US.h"
#elif defined(LOCALE_DE_DE)
    #include "string_de_DE.h"
#else
    #error "Unsupported or unknown LOCALE specified in CMake. Pass -DLOCALE=en_US (or de_DE, etc.)"
#endif

#endif
