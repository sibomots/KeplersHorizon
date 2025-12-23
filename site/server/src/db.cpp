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
#include "db.h"

#include "app.h"
#include "typs.h"

GameState load_game(Db *db, int game_id)
{
    auto rows = db->query("SELECT scenario,state_json FROM games WHERE id=" +
                          std::to_string(game_id) + " LIMIT 1");
    if (rows.empty())
        throw std::runtime_error("game not found");
    std::string scenario = rows[0][0];
    std::string state_json = rows[0][1];

    // state_json already includes scenario; trust it.
    GameState s = GameState::from_json_min(state_json);
    s.game_id = game_id;

    // safer: extract scenario/activePlayer/phaseIndex/round/bp/vp from the
    // authoritative stored JSON is omitted. We'll keep a minimal local parse +
    // a fallback:
    if (s.scenario.empty())
    {
        s.scenario = scenario;
    }

    // Quick re-sync by reading a few known fields with string search (simple).
    auto find_int = [&](const std::string &key, int fallback) -> int {
        std::string pat = "\"" + key + "\":";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
            return fallback;
        p += pat.size();
        while (p < state_json.size() &&
               std::isspace((unsigned char)state_json[p]))
            p++;
        int sign = 1;
        if (p < state_json.size() && state_json[p] == '-')
        {
            sign = -1;
            p++;
        }
        long v = 0;
        while (p < state_json.size() &&
               std::isdigit((unsigned char)state_json[p]))
        {
            v = v * 10 + (state_json[p] - '0');
            p++;
        }
        return (int)(v * sign);
    };
    auto find_str = [&](const std::string &key,
                        const std::string &fallback) -> std::string {
        std::string pat = "\"" + key + "\":\"";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
            return fallback;
        p += pat.size();
        size_t e = state_json.find("\"", p);
        if (e == std::string::npos)
            return fallback;
        return state_json.substr(p, e - p);
    };

    s.round = std::max(1, find_int("round", s.round));
    s.active_player = find_str("activePlayer", s.active_player);
    s.phase_index = find_int("phaseIndex", s.phase_index);
    s.scenario = find_str("scenario", s.scenario);

    // Parse bp/vp objects
    auto find_obj_int = [&](const std::string &objKey,
                            const std::string &fieldKey, int fallback) -> int {
        std::string op = "\"" + objKey + "\":{";
        size_t p = state_json.find(op);
        if (p == std::string::npos)
            return fallback;
        size_t end = state_json.find("}", p + op.size());
        if (end == std::string::npos)
            return fallback;
        std::string sub = state_json.substr(p, end - p + 1);
        return find_int(
            fieldKey,
            fallback); // re-use global find_int (ok for now; small JSON)
    };

    // We'll do simple direct searches for "vp":{"A":X,"B":Y} etc.
    size_t vpPos = state_json.find("\"vp\":{");
    if (vpPos != std::string::npos)
    {
        size_t end = state_json.find("}", vpPos);
        std::string vpSub = state_json.substr(vpPos, end - vpPos + 1);
        auto getv = [&](const std::string &k) -> int {
            std::string pat = "\"" + k + "\":";
            size_t p = vpSub.find(pat);
            if (p == std::string::npos)
                return 0;
            p += pat.size();
            while (p < vpSub.size() && std::isspace((unsigned char)vpSub[p]))
                p++;
            long v = 0;
            while (p < vpSub.size() && std::isdigit((unsigned char)vpSub[p]))
            {
                v = v * 10 + (vpSub[p] - '0');
                p++;
            }
            return (int)v;
        };
        s.vpA = getv("A");
        s.vpB = getv("B");
    }
    size_t bpPos = state_json.find("\"bp\":{");
    if (bpPos != std::string::npos)
    {
        size_t end = state_json.find("}", bpPos);
        std::string bpSub = state_json.substr(bpPos, end - bpPos + 1);
        auto getv = [&](const std::string &k) -> int {
            std::string pat = "\"" + k + "\":";
            size_t p = bpSub.find(pat);
            if (p == std::string::npos)
                return 0;
            p += pat.size();
            while (p < bpSub.size() && std::isspace((unsigned char)bpSub[p]))
                p++;
            long v = 0;
            while (p < bpSub.size() && std::isdigit((unsigned char)bpSub[p]))
            {
                v = v * 10 + (bpSub[p] - '0');
                p++;
            }
            return (int)v;
        };
        s.bpA = getv("A");
        s.bpB = getv("B");
    }
    return s;
}
