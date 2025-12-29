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
#include "telemetry.h"

#include <sstream>

#include "json.h"

// Static member initialization
std::vector<std::string> Telemetry::s_messages_me;
std::vector<std::string> Telemetry::s_messages_them;
std::vector<std::string> Telemetry::s_messages_all;
std::string Telemetry::s_status_json;
std::mutex Telemetry::s_mutex;
char Telemetry::s_current_player = 'A';

void Telemetry::write(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_messages_me.push_back(msg);
}

void Telemetry::tell(PlayerTarget target, const std::string &msg)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (target == PlayerTarget::ME)
    {
        s_messages_me.push_back(msg);
    }
    else // PlayerTarget::THEM
    {
        s_messages_them.push_back(msg);
    }
}

void Telemetry::broadcast(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_messages_all.push_back(msg);
}

void Telemetry::status(int game_id, 
                      const std::string &scenario,
                      int round,
                      const std::string &active_player,
                      const std::string &phase,
                      int vp_a, int vp_b,
                      int bp_a, int bp_b,
                      const std::string &notes,
                      int combat_count,
                      const std::string &combat_hexes)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    
    std::ostringstream json;
    json << "{";
    json << "\"gameId\":" << game_id << ",";
    json << "\"scenario\":\"" << json_escape(scenario) << "\",";
    json << "\"round\":" << round << ",";
    json << "\"activePlayer\":\"" << json_escape(active_player) << "\",";
    json << "\"phase\":\"" << json_escape(phase) << "\",";
    json << "\"vp\":{\"A\":" << vp_a << ",\"B\":" << vp_b << "},";
    json << "\"bp\":{\"A\":" << bp_a << ",\"B\":" << bp_b << "},";
    json << "\"notes\":\"" << json_escape(notes) << "\"";
    
    if (combat_count > 0)
    {
        json << ",\"combat\":{";
        json << "\"count\":" << combat_count << ",";
        json << "\"active_hexes\":\"" << json_escape(combat_hexes) << "\"";
        json << "}";
    }
    
    json << "}";
    
    s_status_json = json.str();
}

std::string Telemetry::get_messages(PlayerTarget target)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    const std::vector<std::string> *msgs =
        (target == PlayerTarget::ME) ? &s_messages_me : &s_messages_them;

    if (msgs->empty())
    {
        return "";
    }

    std::ostringstream out;
    for (size_t i = 0; i < msgs->size(); ++i)
    {
        if (i > 0)
        {
            out << "\n";
        }
        out << (*msgs)[i];
    }
    return out.str();
}

std::string Telemetry::get_broadcast_messages()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_messages_all.empty())
    {
        return "";
    }

    std::ostringstream out;
    for (size_t i = 0; i < s_messages_all.size(); ++i)
    {
        if (i > 0)
        {
            out << "\n";
        }
        out << s_messages_all[i];
    }
    return out.str();
}

std::string Telemetry::get_status_json()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_status_json;
}

void Telemetry::clear()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_messages_me.clear();
    s_messages_them.clear();
    s_messages_all.clear();
    s_status_json.clear();
}

void Telemetry::set_current_player(char player)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_current_player = player;
}

std::string Telemetry::build_response(PlayerTarget target, const GameState &s,
                                      bool ok)
{
    std::string event_text = get_messages(target);

    std::ostringstream o;
    o << "{";
    o << "\"ok\":" << (ok ? "true" : "false") << ",";
    o << "\"event\":\"" << json_escape(event_text) << "\",";
    o << "\"state\":" << s.to_json();
    o << "}";

    return o.str();
}

char Telemetry::resolve_player(PlayerTarget target)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (target == PlayerTarget::ME)
    {
        return s_current_player;
    }
    else // PlayerTarget::THEM
    {
        return (s_current_player == 'A') ? 'B' : 'A';
    }
}
