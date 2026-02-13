///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_MAPUTIL_H__
#define __KH_MAPUTIL_H__

#include <memory>
#include <string>

#include "typedefs.h"

class MapUtil
{
  public:
  public:
    static MapUtil& instance()
    {
        static MapUtil instance;
        return instance;
    }

    MapUtil(const MapUtil&) = delete;
    MapUtil& operator=(const MapUtil&) = delete;
    MapUtil(MapUtil&&) = delete;
    MapUtil& operator=(MapUtil&&) = delete;

    std::string resolve_system_hex(int game_id, const std::string& canon_name);
    std::string resolve_system_name(int game_id,
                                    const std::string& user_supplied);
    bool system_exists(int game_id, const std::string& user_supplied);

  private:
    MapUtil()
    {
    }
};

#endif
