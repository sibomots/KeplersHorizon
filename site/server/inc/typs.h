//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __TYPS_H__
#define __TYPS_H__

#include <algorithm>
#include <map>
#include <ostream>
#include <sstream>
#include <string>

#include "json.h"

// Kepler's Horizon phase sequencing: VP count is implicit at start-of-turn;
// player-facing phases begin at Build Ships.
enum PhaseIndex
{
    PH_BUILD_SHIPS = 0,
    PH_MOVEMENT = 1,
    PH_RESOLVE_COMBAT = 2,
    PH_SYSTEM_PICKDROP = 3,
    PH_END_TURN = 4
};

typedef struct
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
} HttpRequest;

typedef struct
{
    int status = 200;
    std::string content_type = "application/json";
    std::string body;
} HttpResponse;

class GameState
{
  public:
    int game_id;
    std::string scenario; // "", "learning","basic","advanced"
    int round;
    std::string active_player;
    int phase_index;
    int vpA = 0;
    int vpB = 0;
    int bpA = 0;
    int bpB = 0;
    bool game_over = false;
    std::string winner = "";

    // ship lists (future)
    // right now: none
    std::string combat_summary_json; // JSON object for combat state

  public:
    GameState()
    {
        clear();
    }

    void create_empty_game()
    {
        clear();
        game_id = 0;
        scenario = "";
        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        bpA = 0;
        bpB = 0;
        game_over = false;
        winner = "";
    }

    void clear()
    {
        game_id = 0;
        scenario = "";
        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        vpA = 0;
        vpB = 0;
        bpA = 0;
        bpB = 0;
        game_over = false;
        winner = "";
    }
    std::string phase_name() const
    {
        static const char *P[] = {"Build Ships", "Movement", "Resolve Combat",
                                  "SystemShip Pick/Drop", "End of Turn"};
        if (phase_index < PH_BUILD_SHIPS || phase_index > PH_END_TURN)
            return "Build Ships";
        return P[phase_index];
    }

    std::string notes() const
    {
        if (game_over)
            return "Game over. Use 'list' / 'list all' to review.";
        if (scenario.empty())
            return "Type: start learning|basic|advanced";
        if (phase_index == PH_BUILD_SHIPS)
            return "Build/repair/resupply. Use build/deploy/pickup/drop then "
                   "'next'.";
        if (phase_index == PH_MOVEMENT)
            return "Movement (not implemented). Use 'next' to continue.";
        if (phase_index == PH_RESOLVE_COMBAT)
            return "Combat (not implemented). Use 'next' to continue.";
        if (phase_index == PH_SYSTEM_PICKDROP)
            return "SystemShip shuffle (not implemented). Use 'next' to "
                   "continue.";
        if (phase_index == PH_END_TURN)
            return "End of turn. Use 'next' to pass initiative.";
        return "";
    }

    std::string to_json() const
    {
        std::ostringstream o;
        o << "{";
        o << "\"gameId\":" << game_id << ",";
        o << "\"scenario\":\"" << json_escape(scenario) << "\",";
        o << "\"round\":" << round << ",";
        o << "\"activePlayer\":\"" << active_player << "\",";
        o << "\"phaseIndex\":" << phase_index << ",";
        o << "\"phase\":\"" << json_escape(phase_name()) << "\",";
        o << "\"vp\":{\"A\":" << vpA << ",\"B\":" << vpB << "},";
        o << "\"bp\":{\"A\":" << bpA << ",\"B\":" << bpB << "},";
        if (!combat_summary_json.empty())
        {
            o << "\"combat\":" << combat_summary_json << ",";
        }
        o << "\"notes\":\"" << json_escape(notes()) << "\"";
        o << "}";
        return o.str();
    }

    static GameState from_json_min(const std::string &js)
    {
        // Minimal parse by searching for known fields; not a general parser.
        GameState s;
        s.game_over = false;
        s.winner = "";
        auto get_num = [&](const std::string &k) -> int
        {
            std::string pat = "\"" + k + "\":";
            size_t p = js.find(pat);
            if (p == std::string::npos)
                return 0;
            p += pat.size();
            while (p < js.size() && std::isspace((unsigned char)js[p]))
                p++;
            int sign = 1;
            if (p < js.size() && js[p] == '-')
            {
                sign = -1;
                p++;
            }
            long v = 0;
            while (p < js.size() && std::isdigit((unsigned char)js[p]))
            {
                v = v * 10 + (js[p] - '0');
                p++;
            }
            return (int)(v * sign);
        };
        auto get_str = [&](const std::string &k) -> std::string
        {
            std::string pat = "\"" + k + "\":\"";
            size_t p = js.find(pat);
            if (p == std::string::npos)
                return "";
            p += pat.size();
            size_t e = js.find("\"", p);
            if (e == std::string::npos)
                return "";
            return js.substr(p, e - p);
        };
        s.game_id = get_num("gameId");
        s.scenario = get_str("scenario");
        s.round = std::max(1, get_num("round"));
        s.active_player = get_str("activePlayer");
        if (s.active_player != "A" && s.active_player != "B")
            s.active_player = "A";
        s.phase_index = get_num("phaseIndex");
        s.vpA = get_num("A"); // NOTE: this will catch first "A" occurrence;
                              // acceptable for now
        s.vpB = 0;
        // We'll avoid parsing vp/bp deeply; state is small; for now server is
        // source of truth in RAM anyway.
        return s;
    }
};

typedef struct
{
    int user_id = 0;
    std::string username;
    std::string token;
    int game_id = 0;
    char player = 0; // 'A' or 'B'
} AuthContext;

class ShipAttributes
{
  public:
    char type;
    int tech;
    int PD;
    int B;
    int S;
    int T;
    int M;
    int SR;

  public:
    ShipAttributes()
    {
        type = 'W';
        tech = 0;
        PD = 0;
        B = 0;
        S = 0;
        T = 0;
        M = 0;
        SR = 0;
    }
};

class DraftRow
{
  public:
    std::string code;
    std::string name;
    ShipAttributes attr;
    DraftRow()
    {
        attr.type = 'W';
    }
};

class ShipRow
{
  public:
    std::string code;
    std::string name;
    std::string built_turn;
    std::string at_system;
    std::string at_hex;
    std::string racked_in;
    int pd_spent = 0;
    ShipAttributes attr;

  public:
    // Default constructor
    ShipRow()
    {
    }

    // Constructor from DraftRow with tech level and turn info
    ShipRow(const DraftRow &draft, int tech_level,
            const std::string &turn_built)
        : code(draft.code), name(draft.name), built_turn(turn_built),
          pd_spent(0)
    {
        attr = draft.attr;
        attr.tech = tech_level;
    }

    // Factory method for creating from draft
    static ShipRow from_draft(const DraftRow &draft, int tech_level, int round,
                              const std::string &active_player)
    {
        std::string turn_built = "R" + std::to_string(round) + active_player;
        return ShipRow(draft, tech_level, turn_built);
    }
};

class Args
{
  public:
    Args()
    {
        dbhost = "127.0.0.1";
        dbuser = "dbusr";
        dbpass = "dbpass";
        dbname = "dbname";
        listen = "127.0.0.1";
        port = 8080;
    }

  public:
    std::string dbhost;
    std::string dbuser;
    std::string dbpass;
    std::string dbname;
    std::string listen;
    int port;
};

#endif
