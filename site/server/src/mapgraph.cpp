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

MapGraph::MapGraph(int gId) : game_id(gId)
{
    load_hexes();
    load_warplines();
}

void MapGraph::load_hexes()
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<std::vector<std::string>> allHex =
        db.query("SELECT hex_id,q,r FROM hexes WHERE game_id=" +
                 std::to_string(game_id));

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
    std::vector<std::vector<std::string>> wh = db.query(
        "SELECT wh.hex_id,w.a_hex,w.b_hex "
        "FROM warpline_hexes wh "
        "JOIN warplines w ON w.id=wh.warpline_id AND w.game_id=wh.game_id "
        "WHERE wh.game_id=" +
        std::to_string(game_id));

    std::vector<std::vector<std::string>> wlines =
        db.query("SELECT a_hex,b_hex FROM warplines WHERE game_id=" +
                 std::to_string(game_id));

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

    std::vector<std::vector<std::string>> blocks =
        db.query("SELECT DISTINCT ss.hex_id FROM ships s "
                 "JOIN star_systems ss ON s.at_hex = ss.hex_id "
                 "WHERE s.game_id=" +
                 std::to_string(game_id) + " AND s.owner='" +
                 std::string(1, enemy) + "'");

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
    // Implicitly returns hex if found, else logic flow up to caller
    return resolve_system(token);
}

std::string MapGraph::resolve_system(const std::string& token)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string u = upper_ascii(token);
    auto r = db.query("SELECT hex_id FROM star_systems WHERE game_id=" +
                      std::to_string(game_id) + " AND UPPER(name)='" +
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
