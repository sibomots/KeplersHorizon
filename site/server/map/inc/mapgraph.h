///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_MAP_H__
#define __KH_MAP_H__

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * MapGraph encapsulates the game map topology, including:
 * - Hex grid coordinates (q, r)
 * - Warpline connections (Warp Jumps)
 * - Dynamic state like Enemy Blockades (ships in star systems)
 */
class MapGraph
{
  public:
    MapGraph(int game_id);

    // Load dynamic state for the current turn/command context.
    // 'owner' is the player moving; 'enemy' is inferred.
    void load_state(char owner);

    // Resolve a user token (e.g. "h0101", "0101", "Ur") to a hex ID (e.g.
    // "0101"). Returns empty string if invalid.
    std::string resolve_hex(const std::string& token);

    // Resolve a user token to a system name (e.g. "ur" -> "UR").
    // Returns canonical name or empty string.
    std::string resolve_system(const std::string& token);

    // Calculate PD cost from 'fromHex' to 'toHex'.
    // Returns -1 if unreachable within 'limit'.
    // Respects blockades loaded by load_state().
    int get_path_cost(const std::string& fromHex, const std::string& toHex,
                      int limit);

    // Get the actual path (hex sequence) from 'fromHex' to 'toHex'.
    // Returns empty vector if unreachable within 'limit'.
    std::vector<std::string> get_path(const std::string& fromHex,
                                      const std::string& toHex, int limit);

    // Get all hexes adjacent to 'hex_id' (up to 6 neighbors).
    // IMPORTANT: adjacency is derived to match the UI-rendered topology in
    // web/map_view*.html (a parity-staggered, half-row "double-height" flat-top
    // layout based on the "XXYY" label). We intentionally do NOT use the
    // database (q,r) axial coordinates for adjacency.
    std::vector<std::string> get_adjacent_hexes(const std::string& hex_id);

    // Get hexes reachable via warpline jump from 'hex_id'.
    // Returns empty vector if hex has no warp connections.
    std::vector<std::string> get_warp_neighbors(const std::string& hex_id);

  private:
    int game_id;
    int module_id; // Loaded from games table based on game_id
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
