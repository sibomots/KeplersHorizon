//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MAPUTIL_H__
#define __MAPUTIL_H__

#include <memory>
#include <string>

#include "typedefs.h"

class MapUtil
{
  public:
  public:
    static MapUtil &getInstance()
    {
        static MapUtil instance;
        return instance;
    }

    MapUtil(const MapUtil &) = delete;
    MapUtil &operator=(const MapUtil &) = delete;
    MapUtil(MapUtil &&) = delete;
    MapUtil &operator=(MapUtil &&) = delete;

    static std::string
    MapUtil::resolve_system_hex(int game_id, const std::string &canon_name);
    static std::string
    MapUtil::resolve_system_name(int game_id, const std::string &user_supplied);
    static bool MapUtil::system_exists(int game_id,
                                       const std::string &user_supplied);

  private:
    MapUtil()
    {
    }
};

#endif
