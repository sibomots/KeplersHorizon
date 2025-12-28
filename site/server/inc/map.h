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
// 1. Redistributions of source code must retain the above copyright notice, this
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MAP_H__
#define __MAP_H__

#include "db.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <utility>

/**
 * MapGraph encapsulates the game map topology, including:
 * - Hex grid coordinates (q, r)
 * - Warpline connections (Warp Jumps)
 * - Dynamic state like Enemy Blockades (ships in star systems)
 */
class MapGraph
{
public:
    MapGraph(Db *db, int game_id);

    // Load dynamic state for the current turn/command context.
    // 'owner' is the player moving; 'enemy' is inferred.
    void load_state(char owner);

    // Resolve a user token (e.g. "h0101", "0101", "Ur") to a hex ID (e.g. "0101").
    // Returns empty string if invalid.
    std::string resolve_hex(const std::string& token);

    // Resolve a user token to a system name (e.g. "ur" -> "UR").
    // Returns canonical name or empty string.
    std::string resolve_system(const std::string& token);

    // Calculate PD cost from 'fromHex' to 'toHex'.
    // Returns -1 if unreachable within 'limit'.
    // Respects blockades loaded by load_state().
    int get_path_cost(const std::string &fromHex, const std::string &toHex, int limit);

    // Utility: Upper case ASCII string
    static std::string upper_ascii(const std::string &s);

private:
    Db *db;
    int game_id;
    char me = ' ';
    char enemy = ' ';

    // Static Map Data
    std::unordered_map<std::string, std::pair<int, int>> qr;
    std::unordered_map<long long, std::string> byQr;
    std::unordered_map<std::string, std::vector<std::string>> warpJumps;

    // Dynamic State
    std::unordered_set<std::string> enemyBlockades;

    // Helpers
    void load_hexes();
    void load_warplines();
};

#endif
