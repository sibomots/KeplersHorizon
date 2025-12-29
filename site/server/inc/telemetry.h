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
// 1. Redistributions of source code must retain the above copyright notice,
// this
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include <mutex>
#include <string>
#include <vector>

#include "typs.h"

// Player targeting - clean enum-based API
// Future: migrate entire codebase from char 'A'/'B' to this pattern
enum class PlayerTarget
{
    ME,   // Current player (who executed the command)
    THEM  // Opponent player
};

class Telemetry
{
  public:
    // Three core methods for command output (console log)
    static void write(const std::string &msg);
    static void tell(PlayerTarget target, const std::string &msg);
    static void broadcast(const std::string &msg);

    // Status panel update - parameterized for UI fields
    // Sends JSON to update status panel (gameId, scenario, round, player, phase, vp, bp, notes, combat)
    static void status(int game_id, 
                      const std::string &scenario,
                      int round,
                      const std::string &active_player,
                      const std::string &phase,
                      int vp_a, int vp_b,
                      int bp_a, int bp_b,
                      const std::string &notes = "",
                      int combat_count = 0,
                      const std::string &combat_hexes = "");

    // Internal: Get accumulated messages
    static std::string get_messages(PlayerTarget target);
    static std::string get_broadcast_messages();
    static std::string get_status_json();  // Get status panel JSON
    static void clear();

    // Set current player context (called by handler before command execution)
    static void set_current_player(char player);

    // Build JSON response matching client expectations
    static std::string build_response(PlayerTarget target, const GameState &s,
                                      bool ok = true);

    // Utility: Convert PlayerTarget to actual player char for delivery
    static char resolve_player(PlayerTarget target);

  private:
    Telemetry() = delete;

    static std::vector<std::string> s_messages_me;
    static std::vector<std::string> s_messages_them;
    static std::vector<std::string> s_messages_all;
    static std::string s_status_json;  // Current status panel JSON
    static std::mutex s_mutex;
    static char s_current_player;
};

#endif
