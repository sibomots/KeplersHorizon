//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "mapgraph.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <queue>
#include <sstream>

#include "db.h"
#include "util.h"

// Default module_id for cases where game doesn't exist yet
static const int DEFAULT_MODULE_ID = 1;

// Helper: Get module_id for a game from the games table
static int get_module_id_for_game(int game_id)
{
    if (game_id <= 0)
        return DEFAULT_MODULE_ID;

    DatabaseManager& db = DatabaseManager::instance();
    auto rows = db.query("SELECT module_id FROM games WHERE id=" +
                         std::to_string(game_id));
    if (rows.empty() || rows[0].empty() || rows[0][0].empty())
        return DEFAULT_MODULE_ID;
    return std::atoi(rows[0][0].c_str());
}

MapGraph::MapGraph(int gId)
    : game_id(gId), module_id(get_module_id_for_game(gId))
{
    load_hexes();
    load_warplines();
}

void MapGraph::load_hexes()
{
    DatabaseManager& db = DatabaseManager::instance();
    // Module hexes are shared across all games using same module
    std::vector<std::vector<std::string>> allHex =
        db.query("SELECT hex_id,q,r FROM hexes WHERE module_id=" +
                 std::to_string(module_id));

    for (size_t i = 0; i < allHex.size(); i++)
    {
        const std::string& hid = allHex[i][0];
        int q = std::atoi(allHex[i][1].c_str());
        int r = std::atoi(allHex[i][2].c_str());
        qr[hid] = std::make_pair(q, r);
        long long key =
            (static_cast<long long>(q) << 32) ^ static_cast<unsigned int>(r);
        byQr[key] = hid;
    }
}

void MapGraph::load_warplines()
{
    DatabaseManager& db = DatabaseManager::instance();
    // Warplines are shared across all games using same module
    std::vector<std::vector<std::string>> wh = db.query(
        "SELECT wh.hex_id,w.a_hex,w.b_hex "
        "FROM warpline_hexes wh "
        "JOIN warplines w ON w.id=wh.warpline_id AND w.module_id=wh.module_id "
        "WHERE wh.module_id=" +
        std::to_string(module_id));

    std::vector<std::vector<std::string>> wlines =
        db.query("SELECT a_hex,b_hex FROM warplines WHERE module_id=" +
                 std::to_string(module_id));

    for (size_t i = 0; i < wh.size(); i++)
    {
        std::string h = wh[i][0];
        std::string ahex = wh[i][1];
        std::string bhex = wh[i][2];
        warpJumps[h].push_back(ahex);
        warpJumps[h].push_back(bhex);
    }
    for (size_t i = 0; i < wlines.size(); i++)
    {
        std::string ahex = wlines[i][0];
        std::string bhex = wlines[i][1];
        warpJumps[ahex].push_back(bhex);
        warpJumps[bhex].push_back(ahex);
    }
}

void MapGraph::load_state(char owner)
{
    DatabaseManager& db = DatabaseManager::instance();
    me = owner;
    enemy = (me == 'A') ? 'B' : 'A';
    enemyBlockades.clear();

    // Ships are game-specific (use game_id), but star_systems uses module_id
    std::vector<std::vector<std::string>> blocks = db.query(
        "SELECT DISTINCT ss.hex_id FROM ships s "
        "JOIN star_systems ss ON s.at_hex = ss.hex_id AND ss.module_id=" +
        std::to_string(module_id) +
        " "
        "WHERE s.game_id=" +
        std::to_string(game_id) + " AND s.owner='" + std::string(1, enemy) +
        "' AND s.destroyed_at IS NULL");

    for (const auto& r : blocks)
    {
        enemyBlockades.insert(r[0]);
    }
}

std::string MapGraph::resolve_hex(const std::string& token)
{
    std::string destHex;
    // Strip 'h' prefix if present
    if (token.size() >= 1 && (token[0] == 'h' || token[0] == 'H'))
    {
        destHex = token.substr(1);
    }
    else
    {
        // Infer if it looks like a hex ID (4 digits)
        bool allDigits = true;
        for (char c : token)
        {
            if (!std::isdigit((unsigned char)c))
            {
                allDigits = false;
                break;
            }
        }
        if (allDigits && token.size() == 4)
        {
            destHex = token;
        }
    }

    if (!destHex.empty())
    {
        // Verify existence
        if (qr.find(destHex) != qr.end())
        {
            return destHex;
        }
    }

    // Try system name resolution
    return resolve_system(token);
}

std::string MapGraph::resolve_system(const std::string& token)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string u = upper_ascii(token);
    // Star systems use module_id (shared across all games using same module)
    auto r = db.query("SELECT hex_id FROM star_systems WHERE module_id=" +
                      std::to_string(module_id) + " AND UPPER(name)='" +
                      db.esc(u) + "' LIMIT 1");
    if (!r.empty() && !r[0].empty())
    {
        return r[0][0];
    }
    return "";
}

int MapGraph::get_path_cost(const std::string& from, const std::string& to,
                            int limit)
{
    if (from == to)
    {
        return 0;
    }

    std::unordered_map<std::string, int> dist;
    std::queue<std::string> qn;

    dist[from] = 0;
    qn.push(from);

    while (!qn.empty())
    {
        std::string cur = qn.front();
        qn.pop();
        int d = dist[cur];

        if (cur == to)
            return d;
        if (d >= limit)
            continue;

        // Geometric neighbors (as rendered in the UI)
        std::vector<std::string> neighbors = get_adjacent_hexes(cur);

        // Warp neighbors
        auto wit = warpJumps.find(cur);
        if (wit != warpJumps.end())
        {
            neighbors.insert(neighbors.end(), wit->second.begin(),
                             wit->second.end());
        }

        for (const auto& n : neighbors)
        {
            // Blockade Check: Cannot enter blocked hex unless it is the
            // destination
            if (enemyBlockades.count(n) && n != to)
                continue;

            if (dist.find(n) == dist.end())
            {
                dist[n] = d + 1;
                qn.push(n);
            }
        }
    }
    return -1; // Not reachable
}

std::vector<std::string> MapGraph::get_path(const std::string& from,
                                            const std::string& to, int limit)
{
    std::vector<std::string> path;
    if (from == to)
    {
        path.push_back(from);
        return path;
    }

    std::unordered_map<std::string, int> dist;
    std::unordered_map<std::string, std::string> parent;
    std::queue<std::string> qn;

    dist[from] = 0;
    parent[from] = "";
    qn.push(from);

    bool found = false;
    while (!qn.empty())
    {
        std::string cur = qn.front();
        qn.pop();
        int d = dist[cur];

        if (cur == to)
        {
            found = true;
            break;
        }
        if (d >= limit)
            continue;

        // Geometric neighbors (as rendered in the UI)
        std::vector<std::string> neighbors = get_adjacent_hexes(cur);

        // Warp neighbors
        auto wit = warpJumps.find(cur);
        if (wit != warpJumps.end())
        {
            neighbors.insert(neighbors.end(), wit->second.begin(),
                             wit->second.end());
        }

        for (const auto& n : neighbors)
        {
            // Blockade Check: Cannot enter blocked hex unless it is the
            // destination
            if (enemyBlockades.count(n) && n != to)
                continue;

            if (dist.find(n) == dist.end())
            {
                dist[n] = d + 1;
                parent[n] = cur;
                qn.push(n);
            }
        }
    }

    if (!found)
        return path; // Empty - unreachable

    // Reconstruct path from 'to' back to 'from'
    std::string cur = to;
    while (!cur.empty())
    {
        path.push_back(cur);
        cur = parent[cur];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::string> MapGraph::get_adjacent_hexes(const std::string& hex_id)
{
    // IMPORTANT
    // =========
    // The UI (web/map_view*.html) renders the map using a parity-staggered,
    // half-row ("double-height") lattice based on the hex label "XXYY".
    // Players reason about adjacency from that rendering. Therefore server-side
    // pathfinding must derive geometric adjacency from the SAME model.
    //
    // We intentionally do NOT use the stored (q,r) from the database here.
    // The DB hex table is still the authoritative *existence set* of hex IDs,
    // but adjacency must match the UI's implicit topology.

    std::vector<std::string> neighbors;

    // Validate input exists in this module
    if (qr.find(hex_id) == qr.end())
        return neighbors;

    // Parse label "XXYY" -> XX, YY
    if (hex_id.size() != 4 ||
        !std::isdigit((unsigned char)hex_id[0]) ||
        !std::isdigit((unsigned char)hex_id[1]) ||
        !std::isdigit((unsigned char)hex_id[2]) ||
        !std::isdigit((unsigned char)hex_id[3]))
    {
        return neighbors;
    }

    const int XX = (hex_id[0] - '0') * 10 + (hex_id[1] - '0');
    const int YY = (hex_id[2] - '0') * 10 + (hex_id[3] - '0');

    // UI "row" coordinate is based on S = XX + YY (see web template)
    const int S = XX + YY;

    // UI "column" coordinate matches template's XX_start = floor((S-6+1)/2)
    // The constant offsets cancel for relative adjacency; the parity dependency
    // is what matters. A robust derived column index is:
    //   col = XX - floor((S + 1)/2)
    const int col = XX - ((S + 1) / 2);

    // Helper: (S, col) -> hex label "XXYY"
    auto make_hex = [&](int nS, int nCol) -> std::string {
        // invert
        const int nXX = nCol + ((nS + 1) / 2);
        const int nYY = nS - nXX;

        if (nXX < 0 || nXX > 99 || nYY < 0 || nYY > 99)
            return std::string();

        char buf[5];
        std::snprintf(buf, sizeof(buf), "%02d%02d", nXX, nYY);
        return std::string(buf);
    };

    // Neighbor coordinates in the UI's double-height staggered grid.
    // North/South moves change S by 2; diagonals change S by 1 and adjust col
    // based on parity of S.
    const bool odd = (S & 1) != 0;

    struct SC { int s; int c; };
    const SC cand[6] = {
        {S - 2, col},
        {S + 2, col},
        {S - 1, col + (odd ? 1 : 0)},
        {S - 1, col - (odd ? 0 : 1)},
        {S + 1, col + (odd ? 1 : 0)},
        {S + 1, col - (odd ? 0 : 1)},
    };

    neighbors.reserve(6);
    for (const auto& p : cand)
    {
        std::string nh = make_hex(p.s, p.c);
        if (nh.empty())
            continue;
        // Existence filter: only return hexes that exist in the module.
        if (qr.find(nh) != qr.end())
            neighbors.push_back(nh);
    }

    return neighbors;
}
