//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "mapgraph.h"

#include <algorithm>
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

    DatabaseManager& db = DatabaseManager::getInstance();
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
    DatabaseManager& db = DatabaseManager::getInstance();
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
    DatabaseManager& db = DatabaseManager::getInstance();
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
    DatabaseManager& db = DatabaseManager::getInstance();
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
    DatabaseManager& db = DatabaseManager::getInstance();
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

        std::vector<std::string> neighbors;

        // Hex neighbors
        auto qt = qr.find(cur);
        if (qt != qr.end())
        {
            int cq = qt->second.first;
            int cr = qt->second.second;
            const int dq[6] = {1, 1, 0, -1, -1, 0};
            const int dr[6] = {0, -1, -1, 0, 1, 1};
            for (int k = 0; k < 6; ++k)
            {
                long long key = (static_cast<long long>(cq + dq[k]) << 32) ^
                                static_cast<unsigned int>(cr + dr[k]);
                auto bit = byQr.find(key);
                if (bit != byQr.end())
                    neighbors.push_back(bit->second);
            }
        }

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

        std::vector<std::string> neighbors;

        // Hex neighbors
        auto qt = qr.find(cur);
        if (qt != qr.end())
        {
            int cq = qt->second.first;
            int cr = qt->second.second;
            const int dq[6] = {1, 1, 0, -1, -1, 0};
            const int dr[6] = {0, -1, -1, 0, 1, 1};
            for (int k = 0; k < 6; ++k)
            {
                long long key = (static_cast<long long>(cq + dq[k]) << 32) ^
                                static_cast<unsigned int>(cr + dr[k]);
                auto bit = byQr.find(key);
                if (bit != byQr.end())
                    neighbors.push_back(bit->second);
            }
        }

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
    std::vector<std::string> neighbors;

    auto it = qr.find(hex_id);
    if (it == qr.end())
        return neighbors;

    int q = it->second.first;
    int r = it->second.second;

    // Axial coordinate neighbor offsets for hex grid
    static const int dq[] = {1, 1, 0, -1, -1, 0};
    static const int dr[] = {0, -1, -1, 0, 1, 1};

    for (int i = 0; i < 6; i++)
    {
        int nq = q + dq[i];
        int nr = r + dr[i];
        long long key =
            (static_cast<long long>(nq) << 32) ^ static_cast<unsigned int>(nr);
        auto nit = byQr.find(key);
        if (nit != byQr.end())
        {
            neighbors.push_back(nit->second);
        }
    }

    return neighbors;
}
