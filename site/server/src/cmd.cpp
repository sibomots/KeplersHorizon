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
#include "app.h"
#include "comms.h"
#include "db.h"
#include "events.h"
#include "game.h"
#include "state.h"
#include "typs.h"
#include "util.h"

static std::string upper_ascii(const std::string &s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        r[i] = (char)std::toupper((unsigned char)r[i]);
    return r;
}


static std::string resolve_system_hex(Db *db, int game_id, const std::string &canon_name)
{
    std::ostringstream q;
    q << "SELECT hex_id FROM star_systems WHERE game_id=" << game_id
      << " AND name='" << db->esc(canon_name) << "' LIMIT 1";
    auto r = db->query(q.str());
    if (r.empty()) {
        return "";
    }
    return r[0][0];
}

static std::string resolve_system_name(Db *db, int game_id,
                                       const std::string &user_supplied)
{
    std::string u = upper_ascii(user_supplied);
    auto r = db->query("SELECT name FROM star_systems WHERE game_id=" +
                       std::to_string(game_id) + " AND UPPER(name)='" +
                       db->esc(u) + "' LIMIT 1");
    if (!r.empty() && !r[0].empty())
        return r[0][0];
    return u;
}

static bool system_exists(Db *db, int game_id, const std::string &user_supplied)
{
    std::string u = upper_ascii(user_supplied);
    auto r = db->query("SELECT name FROM star_systems WHERE game_id=" +
                       std::to_string(game_id) + " AND UPPER(name)='" +
                       db->esc(u) + "' LIMIT 1");
    return !r.empty();
}

#include <iostream>
#include <unordered_map>
#include <queue>

void handle_usr_command(const HttpRequest *req, Db *db, HttpResponse *resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }
    AuthContext a = require_auth(db, (const HttpRequest *)req, resp);
    if (resp->status != 200)
    {
        return;
    }
    std::string cmdline = trim(json_get_string(req->body, "command"));

    //debug std::cout << "Command: " << cmdline.c_str() << std::endl;

    if (cmdline.empty())
    {
        resp->status = 400;
        resp->body = json_error("empty command");
        return;
    }
 
    GameState s = load_game(db, a.game_id);

    std::vector<std::string> tok = split_ws(cmdline);
    std::string cmd = to_lower(tok[0]);

    std::string eventText;

    // NOTE: 'me' is derived from the authenticated token, not from game state.
    char me = (a.player ? a.player : 'A');
    char owner = me;
    char enemy = (owner == 'A') ? 'B' : 'A';
    char active = (s.active_player.empty() ? 'A' : s.active_player[0]);
    auto turnToken = std::string("R") + std::to_string(s.round) + active;

    auto require_build_phase = [&]() -> bool {
        if (s.scenario.empty())
        {
            eventText = "No scenario. Type: start learning|basic|advanced";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            std::ostringstream o;
            o << "Not in Build Ships phase. Current: " << s.phase_name();
            eventText = o.str();
            return false;
        }
        return true;
    };

    auto require_my_turn = [&]() -> bool {
        if (active != me) {
            std::ostringstream o;
            o << "Not your turn. Active player is " << active << ".";
            eventText = o.str();
            return false;
        }
        return true;
    };


    auto ship_cost_bp = [&](char ship_type, const DraftRow &d) -> int {
        int cost = 0;
        cost += d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
        cost += (d.attr.M + 2) /
                3; // M is validated multiple-of-3 elsewhere, but keep safe
        if (ship_type == 'W')
            cost += 5; // Warp generator
        return cost;
    };

    auto compute_tech_level = [&]() -> int {
        if (s.scenario != "advanced")
            return 0;
        // Tech level increases every 4 game-turns (turns 1-4 = 0, 5-8 = 1, ...)
        if (s.round < 1)
            return 0;
        return (s.round - 1) / 4;
    };

    auto fmt_attrs = [&](int PD, int B, int Sx, int T, int M,
                         int SR) -> std::string {
        std::ostringstream o;
        o << "PD=" << PD << ", B=" << B << ", S=" << Sx << ", T=" << T
          << ", M=" << M << ", SR=" << SR;
        return o.str();
    };

    auto list_fleet_text = [&](char whichOwner) -> std::string {
        auto ships = load_ships(db, a.game_id, whichOwner);
        std::ostringstream o;
        o << (whichOwner == me ? "Blue-force fleet:" : "Red-force fleet:")
          << "\n";
        if (ships.empty())
        {
            o << "  (none)\n";
            return o.str();
        }
        for (auto &sh : ships)
        {
            o << "  " << sh.name << " - " << sh.code << " (L" << sh.attr.tech
              << ") "
              << fmt_attrs(sh.attr.PD, sh.attr.B, sh.attr.S, sh.attr.T,
                           sh.attr.M, sh.attr.SR);
            if (!sh.racked_in.empty())
                o << " [RACKED in " << sh.racked_in << "]";
            else if (!sh.at_system.empty())
                o << " @ " << sh.at_system;
            else
                o << " @ (undeployed)";
            if (sh.attr.type == 'W' && sh.attr.SR > 0)
            {
                int cnt = count_racked_in(db, a.game_id, whichOwner, sh.code);
                if (cnt > 0)
                {
                    auto carried = db->query(
                        "SELECT ship_code FROM ships WHERE game_id=" +
                        std::to_string(a.game_id) + " AND owner='" +
                        std::string(1, whichOwner) + "' AND racked_in='" +
                        db->esc(sh.code) + "' ORDER BY ship_code");
                    o << " carrying:";
                    for (auto &c : carried)
                        o << " " << c[0];
                }
                else
                {
                    o << " carrying: (none)";
                }
            }
            o << "\n";
        }
        return o.str();
    };

    if (cmd == "status")
    {
        eventText = "Status refreshed.";
    }
    else if (cmd == "bases")
    {
        // Map/base-star configuration will move server-side later; for now
        // allow free-form system names.
        std::ostringstream o;
        o << "Base systems are not yet configured server-side.\n"
          << "Use 'deploy <W#|S##> <SYSTEM>' with a system name (e.g., UR) for "
             "now.";
        eventText = o.str();
    }
    else if (cmd == "reset")
    {
        // Clear scenario and state; also clear DB ships/drafts for the latest
        // game
        s.clear(); // = GameState();
        s.game_id = a.game_id;
        db->exec("DELETE FROM drafts WHERE game_id=" +
                 std::to_string(a.game_id));
        db->exec("DELETE FROM ships  WHERE game_id=" +
                 std::to_string(a.game_id));
        set_current_draft(db, a.game_id, 'A', "");
        set_current_draft(db, a.game_id, 'B', "");
        eventText = "Game reset. Type: start learning|basic|advanced";
    }
    else if (cmd == "start")
    {
        if (tok.size() < 2)
        {
            eventText = "Usage: start learning|basic|advanced";
        }
        else
        {
            std::string sc = to_lower(tok[1]);
            if (sc != "learning" && sc != "basic" && sc != "advanced")
            {
                resp->status = 400;
                resp->body = json_error("unknown scenario");
                return;
            }
            s = new_game_state_for_scenario(sc);
            s.game_id = a.game_id;

            // Clear per-game DB state
            db->exec("DELETE FROM drafts WHERE game_id=" +
                     std::to_string(a.game_id));
            db->exec("DELETE FROM ships  WHERE game_id=" +
                     std::to_string(a.game_id));
            set_current_draft(db, a.game_id, 'A', "");
            set_current_draft(db, a.game_id, 'B', "");

            eventText = "Game started: " + sc + ". " + s.notes();
        }
    }
    else if (cmd == "next")
    {
        if (!require_my_turn()) {
            // eventText set by require_my_turn
            save_game(db, s);
            append_event(db, a.game_id, a.user_id, cmdline, eventText, s);
            resp->body = json_ok_with_state_and_event(s, eventText);
            return;
        }
        std::string before = s.phase_name();
        std::string beforeP = s.active_player;
        int beforeRound = s.round;

        advance_next(db, s);

        std::ostringstream msg;
        msg << "Advanced: " << beforeP << " / " << before << " -> "
            << s.active_player << " / " << s.phase_name();
        if (s.round != beforeRound)
            msg << " (round " << s.round << ")";
        eventText = msg.str();
    }
    else if (cmd == "list")
    {
        if (tok.size() == 1)
        {
            eventText = list_fleet_text(owner) + "\n" + list_fleet_text(enemy);
        }
        else
        {
            std::string sub = to_lower(tok[1]);
            if (sub == "drafts")
            {
                eventText = "Use: build drafts (list drafts is deprecated).";
            }
            else if (sub == "system" && tok.size() >= 3)
            {
                std::string sys = resolve_system_name(db, a.game_id, tok[2]);
                auto aShips = db->query(
                    "SELECT "
                    "owner,ship_name,ship_code,ship_type,tech_level,pd,beam,"
                    "screen,tube,missiles,sr,racked_in "
                    "FROM ships WHERE game_id=" +
                    std::to_string(a.game_id) + " AND at_system='" +
                    db->esc(sys) + "' ORDER BY owner,ship_code");
                std::ostringstream o;
                o << "Ships at " << sys << ":\n";
                if (aShips.empty())
                {
                    o << "  (none)\n";
                    eventText = o.str();
                }
                else
                {
                    for (auto &r : aShips)
                    {
                        char ow = r[0].empty() ? 'A' : r[0][0];
                        o << "  " << (ow == owner ? "Blue" : "Red") << ": "
                          << r[1] << " - " << r[2] << " (L" << r[4] << ") "
                          << fmt_attrs(std::atoi(r[5].c_str()),
                                       std::atoi(r[6].c_str()),
                                       std::atoi(r[7].c_str()),
                                       std::atoi(r[8].c_str()),
                                       std::atoi(r[9].c_str()),
                                       std::atoi(r[10].c_str()))
                          << "\n";
                    }
                    eventText = o.str();
                }
            }
            else if (sub == "all")
            {
                std::ostringstream o;
                char me = a.player;
                char enemy = (me == 'A') ? 'B' : 'A';
                o << list_fleet_text(me) << "\n" << list_fleet_text(enemy);
                eventText = o.str();
            }
            else if (sub == "scan")
            {
                eventText =
                    "list scan: not implemented yet (sightings table is "
                    "present for later).";
            }
            else
            {
                eventText =
                    "Usage: list | list drafts | list system <SYS> | list "
                    "all | list scan";
            }
            //debug std::cout << "event-text: " << eventText.c_str() << std::endl;
        }
    }
    else if (cmd == "build")
    {
        if (tok.size() < 2)
        {
            eventText =
                "Usage: build "
                "new|drafts|set|add|clear|show|validate|cost|commit|cancel ...";
        }
        else
        {
            std::string sub = to_lower(tok[1]);

            if (sub == "new")
            {
                int mybp = (owner == 'A') ? s.bpA : s.bpB;
                if (mybp <= 0)
                {
                    resp->status = 400;
                    resp->body = json_error("No Build Points available.");
                    return;
                }
                if (!require_my_turn() || !require_build_phase())
                {
                    // fallthrough: eventText already set
                }
                else if (tok.size() < 4)
                {
                    eventText =
                        "Usage: build new W1 <Name...> | build new W <Name...> "
                        "| build new S20 <Name...> | build new S <Name...>";
                }
                else
                {
                    std::string codeTok = tok[2];
                    if (codeTok.empty())
                    {
                        eventText = "bad ship code";
                    }
                    else
                    {
                        char stype = std::toupper(codeTok[0]);
                        if (stype != 'W' && stype != 'S')
                        {
                            eventText = "Ship type must start with W or S";
                        }
                        else
                        {
                            auto parse_num = [&](const std::string &s) -> int {
                                if (s.size() <= 1)
                                    return -1;
                                int n = 0;
                                for (size_t i = 1; i < s.size(); ++i)
                                {
                                    if (!std::isdigit(s[i]))
                                        return -1;
                                    n = n * 10 + (s[i] - '0');
                                }
                                return n;
                            };
                            int n = parse_num(codeTok);
                            if (n == -1)
                            {
                                // auto-assign next < 100 based on ships+drafts
                                int maxn = 0;
                                auto rows = db->query(
                                    "SELECT ship_code FROM ships WHERE "
                                    "game_id=" +
                                    std::to_string(a.game_id) + " AND owner='" +
                                    std::string(1, owner) +
                                    "' AND ship_type='" +
                                    std::string(1, stype) + "'");
                                for (auto &r : rows)
                                {
                                    int nn = parse_num(r[0]);
                                    if (nn > maxn)
                                        maxn = nn;
                                }
                                auto rows2 = db->query(
                                    "SELECT ship_code FROM drafts WHERE "
                                    "game_id=" +
                                    std::to_string(a.game_id) + " AND owner='" +
                                    std::string(1, owner) +
                                    "' AND ship_type='" +
                                    std::string(1, stype) + "'");
                                for (auto &r : rows2)
                                {
                                    int nn = parse_num(r[0]);
                                    if (nn > maxn)
                                        maxn = nn;
                                }
                                n = maxn + 1;
                            }
                            if (n <= 0 || n >= 100)
                            {
                                eventText = "Ship id must be 1..99";
                            }
                            else
                            {
                                std::ostringstream c;
                                c << stype << n;
                                std::string code = c.str();

                                if (draft_exists(db, a.game_id, owner, code) ||
                                    ship_exists(db, a.game_id, owner, code))
                                {
                                    eventText =
                                        "Ship code already in use: " + code;
                                }
                                else
                                {
                                    std::string name;
                                    for (size_t i = 3; i < tok.size(); ++i)
                                    {
                                        if (i > 3)
                                            name += " ";
                                        name += tok[i];
                                    }
                                    if (name.empty())
                                        name = (stype == 'W' ? "WarpShip"
                                                             : "SystemShip");
                                    if (name.size() > 32)
                                        name.resize(32);

                                    DraftRow d;
                                    d.code = code;
                                    d.name = name;
                                    d.attr.type = stype;
                                    insert_draft(db, a.game_id, owner, d);
                                    set_current_draft(db, a.game_id, owner,
                                                      code);

                                    std::ostringstream o;
                                    o << "Draft created: " << name << " - "
                                      << code
                                      << " (current). Use: build set "
                                         "PD|B|S|T|M|SR <n>";
                                    eventText = o.str();
                                }
                            }
                        }
                    }
                }
            }
            else if (sub == "drafts")
            {
                auto ds = load_drafts(db, a.game_id, owner);
                std::ostringstream o;
                o << "Draft ships:\n";
                if (ds.empty())
                    o << "  (none)\n";
                std::string cur = get_current_draft(db, a.game_id, owner);
                for (auto &d : ds)
                {
                    int cost = ship_cost_bp(d.attr.type, d);
                    o << "  " << d.code << " '" << d.name << "' cost=" << cost << " BP";
                    if (d.code == cur) o << "  [current]";
                    o << "\n";
                }
                eventText = o.str();
            }
            else if (sub == "show" || sub == "validate" || sub == "cost" ||
                     sub == "set" || sub == "add" || sub == "clear" ||
                     sub == "commit" || sub == "cancel")
            {
                // Determine target draft code
                std::string code;
                size_t argi = 2;

                auto looks_like_code = [&](const std::string &s) -> bool {
                    if (s.size() < 2)
                        return false;
                    char c = std::toupper(s[0]);
                    if (c != 'W' && c != 'S')
                        return false;
                    for (size_t i = 1; i < s.size(); ++i)
                        if (!std::isdigit(s[i]))
                            return false;
                    return true;
                };

                if (argi < tok.size() && looks_like_code(tok[argi]))
                {
                    code = tok[argi];
                    argi++;
                }
                else
                {
                    code = get_current_draft(db, a.game_id, owner);
                }

                if (code.empty())
                {
                    eventText = "No current draft. Use: build new ...";
                }
                else if (!draft_exists(db, a.game_id, owner, code))
                {
                    eventText = "Draft not found: " + code;
                }
                else
                {
                    DraftRow d = load_draft(db, a.game_id, owner, code);

                    auto validate_draft = [&](std::string &err) -> bool {
                        if (d.attr.type == 'S' && d.attr.SR != 0)
                        {
                            err = "SystemShips cannot have SR";
                            return false;
                        }
                        if (d.attr.type == 'S')
                        { /* no WG cost; ok */
                        }
                        if (d.attr.M % 3 != 0)
                        {
                            err = "Missiles must be a multiple of 3";
                            return false;
                        }
                        if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 ||
                            d.attr.T < 0 || d.attr.M < 0 || d.attr.SR < 0)
                        {
                            err = "Negative attribute";
                            return false;
                        }
                        return true;
                    };

                    if (sub == "show")
                    {
                        int cost = ship_cost_bp(d.attr.type, d);
                        std::ostringstream o;
                        o << "Draft: " << d.name << " - " << d.code << " ["
                          << fmt_attrs(d.attr.PD, d.attr.B, d.attr.S, d.attr.T,
                                       d.attr.M, d.attr.SR)
                          << "] cost=" << cost;
                        eventText = o.str();
                        set_current_draft(db, a.game_id, owner, d.code);
                    }
                    else if (sub == "validate")
                    {
                        std::string err;
                        if (!validate_draft(err))
                            eventText = "Draft invalid: " + err;
                        else
                            eventText =
                                "Draft valid: " + d.name + " - " + d.code;
                    }
                    else if (sub == "cost")
                    {
                        std::string err;
                        if (!validate_draft(err))
                            eventText = "Draft invalid: " + err;
                        else
                        {
                            int cost = ship_cost_bp(d.attr.type, d);
                            eventText = "Draft cost: " + d.code + " = " +
                                        std::to_string(cost) + " BP";
                        }
                    }
                    else if (sub == "clear")
                    {
                        if (argi >= tok.size())
                        {
                            eventText =
                                "Usage: build clear <PD|B|S|T|M|SR|all>";
                        }
                        else
                        {
                            std::string attr = to_lower(tok[argi]);
                            if (attr == "all")
                            {
                                d.attr.PD = d.attr.B = d.attr.S = d.attr.T =
                                    d.attr.M = d.attr.SR = 0;
                            }
                            else if (attr == "pd")
                                d.attr.PD = 0;
                            else if (attr == "b")
                                d.attr.B = 0;
                            else if (attr == "s")
                                d.attr.S = 0;
                            else if (attr == "t")
                                d.attr.T = 0;
                            else if (attr == "m")
                                d.attr.M = 0;
                            else if (attr == "sr")
                                d.attr.SR = 0;
                            else
                            {
                                eventText = "Unknown attribute: " + attr;
                                goto build_done;
                            }
                            update_draft_attrs(db, a.game_id, owner, d.code, d);
                            eventText =
                                "Draft updated: " + d.code + " [" +
                                fmt_attrs(d.attr.PD, d.attr.B, d.attr.S,
                                          d.attr.T, d.attr.M, d.attr.SR) +
                                "]";
                        }
                    }
                    else if (sub == "set" || sub == "add")
                    {
                        if (argi + 1 >= tok.size())
                        {
                            eventText =
                                "Usage: build set|add <PD|B|S|T|M|SR> <n>";
                        }
                        else
                        {
                            std::string attr = to_lower(tok[argi]);
                            int n = std::atoi(tok[argi + 1].c_str());
                            auto apply = [&](int &field) {
                                if (sub == "set")
                                    field = n;
                                else
                                    field += n;
                            };
                            if (attr == "pd")
                                apply(d.attr.PD);
                            else if (attr == "b")
                                apply(d.attr.B);
                            else if (attr == "s")
                                apply(d.attr.S);
                            else if (attr == "t")
                                apply(d.attr.T);
                            else if (attr == "m")
                                apply(d.attr.M);
                            else if (attr == "sr")
                                apply(d.attr.SR);
                            else
                            {
                                eventText = "Unknown attribute: " + attr;
                                goto build_done;
                            }
                            update_draft_attrs(db, a.game_id, owner, d.code, d);
                            eventText =
                                "Draft updated: " + d.code + " [" +
                                fmt_attrs(d.attr.PD, d.attr.B, d.attr.S,
                                          d.attr.T, d.attr.M, d.attr.SR) +
                                "]";
                        }
                    }
                    else if (sub == "cancel")
                    {
                        delete_draft(db, a.game_id, owner, d.code);
                        set_current_draft(db, a.game_id, owner, "");
                        eventText = "Draft canceled: " + d.code;
                    }
                    else if (sub == "commit")
                    {
                        if (!require_my_turn() || !require_build_phase())
                        {
                            // eventText already set
                        }
                        else
                        {
                            std::string err;
                            if (!validate_draft(err))
                            {
                                eventText = "Draft invalid: " + err;
                            }
                            else
                            {
                                int cost = ship_cost_bp(d.attr.type, d);
                                int &bp = (owner == 'A') ? s.bpA : s.bpB;
                                if (cost > bp)
                                {
                                    std::ostringstream o;
                                    o << "Insufficient BP. Need " << cost
                                      << ", have " << bp;
                                    eventText = o.str();
                                }
                                else
                                {
                                    // Commit -> ships table
                                    ShipRow sh;
                                    sh.code = d.code;
                                    sh.name = d.name;
                                    sh.attr.type = d.attr.type;
                                    sh.attr.PD = d.attr.PD;
                                    sh.attr.B = d.attr.B;
                                    sh.attr.S = d.attr.S;
                                    sh.attr.T = d.attr.T;
                                    sh.attr.M = d.attr.M;
                                    sh.attr.SR = d.attr.SR;
                                    sh.attr.tech = compute_tech_level();
                                    sh.built_turn = turnToken;

                                    insert_ship(db, a.game_id, owner, sh);
                                    delete_draft(db, a.game_id, owner, d.code);
                                    set_current_draft(db, a.game_id, owner, "");

                                    bp -= cost;
                                    std::ostringstream o;
                                    o << "Committed: " << sh.name << " - "
                                      << sh.code << " (L" << sh.attr.tech
                                      << ") cost=" << cost
                                      << " BP. Remaining BP=" << bp;
                                    eventText = o.str();
                                }
                            }
                        }
                    }
                build_done:;
                }
            }
            else
            {
                eventText =
                    "Usage: build "
                    "new|drafts|set|add|clear|show|validate|cost|commit|cancel ...";
            }
        }
    }
    else if (cmd == "deploy")
    {
        if (!require_my_turn() || !require_build_phase())
        {
            // eventText already set
        }
        else if (tok.size() < 3)
        {
            eventText = "Usage: deploy <W#|S##> <SYSTEM>";
        }
        else
        {
            std::string code = tok[1];
            std::string sys = resolve_system_name(db, a.game_id, tok[2]);
            if (!ship_exists(db, a.game_id, owner, code))
                eventText = "Ship not found: " + code;
            else
            {
                ShipRow sh = load_ship(db, a.game_id, owner, code);
                if (!sh.racked_in.empty())
                {
                    eventText =
                        "Ship is racked; drop it before deploying: " + code;
                }
                else
                {
                    std::string hex = resolve_system_hex(db, a.game_id, sys);
                    update_ship_location(db, a.game_id, owner, code, sys, hex, "");
                    eventText =
                        "Deployed " + sh.name + " - " + sh.code + " to " + sys;
                }
            }
        }
    }
    else if (cmd == "pickup" || cmd == "drop")
    {
        if (!require_my_turn() || !require_build_phase())
        {
            // eventText already set
        }
        else if (tok.size() < 3)
        {
            eventText = std::string("Usage: ") + cmd + " <W#> <S##>";
        }
        else
        {
            std::string wcode = tok[1];
            std::string scode = tok[2];

            if (!ship_exists(db, a.game_id, owner, wcode))
            {
                eventText = "Warpship not found: " + wcode;
            }
            else if (!ship_exists(db, a.game_id, owner, scode))
            {
                eventText = "Systemship not found: " + scode;
            }
            else
            {
                ShipRow w = load_ship(db, a.game_id, owner, wcode);
                ShipRow sship = load_ship(db, a.game_id, owner, scode);
                if (w.attr.type != 'W')
                {
                    eventText = "Not a Warpship: " + wcode;
                }
                else if (sship.attr.type != 'S')
                {
                    eventText = "Not a Systemship: " + scode;
                }
                else if (w.built_turn != turnToken ||
                         sship.built_turn != turnToken)
                {
                    eventText = "Pre-rack rule: both ships must be committed "
                                "this same turn (" +
                                turnToken + ").";
                }
                else if (cmd == "pickup")
                {
                    if (w.at_system.empty() || sship.at_system.empty())
                    {
                        eventText = "Both ships must be deployed to the same "
                                    "system first.";
                    }
                    else if (w.at_system != sship.at_system)
                    {
                        eventText = "Not co-located: " + wcode + "@" +
                                    w.at_system + " vs " + scode + "@" +
                                    sship.at_system;
                    }
                    else if (!sship.racked_in.empty())
                    {
                        eventText =
                            "Systemship already racked in " + sship.racked_in;
                    }
                    else
                    {
                        int carried =
                            count_racked_in(db, a.game_id, owner, w.code);
                        if (carried >= w.attr.SR)
                        {
                            std::ostringstream o;
                            o << "No SR capacity. SR=" << w.attr.SR
                              << ", carrying=" << carried;
                            eventText = o.str();
                        }
                        else
                        {
                            update_ship_location(db, a.game_id, owner, scode,
                                                 "", w.at_hex, wcode);
                            eventText = "Picked up " + sship.name + " - " +
                                        sship.code + " into " + w.name + " - " +
                                        w.code;
                        }
                    }
                }
                else
                { // drop
                    if (sship.racked_in != wcode)
                    {
                        eventText = "Systemship is not racked in " + wcode;
                    }
                    else
                    {
                        if (w.at_system.empty())
                        {
                            eventText = "Warpship must be deployed to a system "
                                        "to drop.";
                        }
                        else
                        {
                            update_ship_location(db, a.game_id, owner, scode,
                                                 w.at_system, w.at_hex, "");
                            eventText = "Dropped " + sship.name + " - " +
                                        sship.code + " at " + w.at_system;
                        }
                    }
                }
            }
        }
    }
    else
    {
        resp->status = 400;
        resp->body = json_error("unknown command");
        return;
    }

    save_game(db, s);
    append_event(db, a.game_id, a.user_id, cmdline, eventText, s);

    resp->body = json_ok_with_state_and_event(s, eventText);
    return;
}
