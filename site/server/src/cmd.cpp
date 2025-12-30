//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <queue>
#include <unordered_map>

#include "app.h"
#include "combat.h"
#include "comms.h"
#include "db.h"
#include "events.h"
#include "game.h"
#include "logger.h"
#include "mapgraph.h"
#include "state.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typs.h"
#include "util.h"

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern "C" int yyparse();

void handle_usr_command(const HttpRequest *req, HttpResponse *resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }
    AuthContext a = require_auth((const HttpRequest *)req, resp);
    if (resp->status != 200)
    {
        return;
    }
    std::string cmdline = trim(json_get_string(req->body, "command"));

    std::string debug_cmd = "Raw command from user >";
    debug_cmd.append(cmdline);
    debug_cmd.append("<");
    Logger::instance().debug(debug_cmd);

    if (cmdline.empty())
    {
        resp->status = 400;
        resp->body = json_error("empty command");
        return;
    }

    // Configure StateMachine with DB context
    // StateMachine::getInstance().set_db(db);
    // StateMachine::getInstance().set_game_id(a.game_id);

    // Set global parser context
    // g_db = db;
    // g_game_id = a.game_id;

    // Try parser first (handles migrated commands)
    YY_BUFFER_STATE buffer = yy_scan_string(cmdline.c_str());

    int parse_result = yyparse();
    yy_delete_buffer(buffer);

    // If parser succeeded, return response with Telemetry
    if (parse_result == 0)
    {
        Logger::instance().info("Command handled by parser: " + cmdline);

        GameState s = load_game(db, a.game_id);

        resp->body = Telemetry::write("Command executed");

        /* std::ostringstream err; */
        /* err << "{\"ok\":false,\"event\":\"Command failed\",\"state\":" <<
         * s.to_json() << "}"; */
        /* resp->body = err.str(); */

        return;
    }

    // Parser failed - fall back to legacy handlers for unmigrated commands
    Logger::instance().info("Falling back to legacy handler: " + cmdline);

    {
        resp->status = 400;
        resp->body = json_error("unknown command");
        Logger::instance().error("Unknown command attempted");
        return;
    }

#if USING_LEGACY
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

    auto require_build_phase = [&]() -> bool
    {
        if (s.scenario.empty())
        {
            eventText = "No scenario. Type: start learning|basic|advanced";
            Logger::instance().error(eventText);
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            std::ostringstream o;
            o << "Not in Build Ships phase. Current: " << s.phase_name();
            eventText = o.str();
            Logger::instance().error(eventText);
            return false;
        }
        return true;
    };

    auto require_movement_phase = [&]() -> bool
    {
        if (s.scenario.empty())
        {
            eventText = "No scenario. Type: start learning|basic|advanced";
            return false;
        }
        if (s.phase_index != PH_MOVEMENT)
        {
            std::ostringstream o;
            o << "Not in Movement phase. Current: " << s.phase_name();
            eventText = o.str();
            Logger::instance().error(eventText);
            return false;
        }
        return true;
    };

    auto require_my_turn = [&]() -> bool
    {
        if (active != me)
        {
            std::ostringstream o;
            o << "Not your turn. Active player is " << active << ".";
            eventText = o.str();
            Logger::instance().error(eventText);
            return false;
        }
        return true;
    };

    auto ship_cost_bp = [&](char ship_type, const DraftRow &d) -> int
    {
        int cost = 0;
        cost += d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
        cost += (d.attr.M + 2) /
                3; // M is validated multiple-of-3 elsewhere, but keep safe
        if (ship_type == 'W')
            cost += 5; // Warp generator
        return cost;
    };

    auto compute_tech_level = [&]() -> int
    {
        if (s.scenario != "advanced")
            return 0;
        // Tech level increases every 4 game-turns (turns 1-4 = 0, 5-8 = 1, ...)
        if (s.round < 1)
            return 0;
        return (s.round - 1) / 4;
    };

    auto fmt_attrs = [&](int PD, int B, int Sx, int T, int M,
                         int SR) -> std::string
    {
        std::ostringstream o;
        o << "PD=" << PD << ", B=" << B << ", S=" << Sx << ", T=" << T
          << ", M=" << M << ", SR=" << SR;
        return o.str();
    };

    auto list_fleet_text = [&](char whichOwner) -> std::string
    {
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
        Logger::instance().error(eventText);
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
            Logger::instance().info(eventText);
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
    else if (cmd == "combat")
    {
        std::istringstream iss(cmdline);
        std::string cmdName;
        iss >> cmdName; // "combat"

        std::string action;
        iss >> action;
        if (action == "order")
        {
            // combat order <ship> [tactic=A|D|R] [target=ID] [d=N] [b=N] [s=N]
            // [t=N] [m=JSON]
            CombatOrder ord;
            ord.game_id = a.game_id;
            ord.round = 0; // Set by engine

            // Defaults
            ord.tactic = 'A'; // Default to Attack
            ord.target_id = "";
            ord.power_d = 0;
            ord.power_b = 0;
            ord.power_s = 0;
            ord.power_t = 0;
            ord.missiles_json = "[]";

            if (!(iss >> ord.ship_code))
            {
                resp->body = json_error("missing ship code");
                return;
            }
            ord.ship_code = upper_ascii(ord.ship_code);

            std::string token;
            while (iss >> token)
            {
                size_t eq = token.find('=');
                if (eq == std::string::npos)
                {
                    // check for implicit tactic (A, D, R)
                    if (token.size() == 1)
                    {
                        char c = std::toupper(token[0]);
                        if (c == 'A' || c == 'D' || c == 'R')
                        {
                            ord.tactic = c;
                            continue;
                        }
                    }
                    // Implicit target ID (e.g. "W1" in "combat order W2 D W1")
                    ord.target_id = token;
                    continue;
                }

                std::string key = to_lower(token.substr(0, eq));
                std::string val = token.substr(eq + 1);

                if (key == "tactic" || key == "mode" || key == "opt")
                {
                    if (!val.empty())
                        ord.tactic = std::toupper(val[0]);
                }
                else if (key == "target" || key == "tgt")
                {
                    ord.target_id = val;
                }
                else if (key == "d" || key == "drive")
                    ord.power_d = std::atoi(val.c_str());
                else if (key == "b" || key == "beam")
                    ord.power_b = std::atoi(val.c_str());
                else if (key == "s" || key == "screen")
                    ord.power_s = std::atoi(val.c_str());
                else if (key == "t" || key == "tube")
                    ord.power_t = std::atoi(val.c_str());
                else if (key == "m" || key == "missiles")
                    ord.missiles_json = val;
            }

            // No strict syntax check needed, defaults apply.

            std::string candidate_target(ord.target_id);
            Logger::instance().info(candidate_target);

            // Validation
            if (ord.tactic == 'D' && ord.target_id.empty())
            {
                eventText =
                    "Combat order to dodge requires a target opponent ship";
                // Don't submit
            }
            else
            {
                CombatEngine ce(db, a.game_id);
                eventText = ce.submit_order(owner, ord);
            }
        }
        else if (action == "resolve")
        {
            std::string hex;
            iss >> hex;
            if (hex.empty())
            {
                resp->status = 400;
                resp->body = json_error("missing hex");
                Logger::instance().error(
                    "Trying to resolve combat, hex ID is missing");
                return;
            }
            CombatEngine ce(db, a.game_id);
            eventText = ce.resolve_round(hex);
        }
        else if (action == "apply")
        {
            // combat apply <ship> <attr=val>...
            std::string ship_code;
            if (!(iss >> ship_code))
            {
                resp->body = json_error("missing ship code");
                Logger::instance().error(
                    "Trying combat subcommand apply, ship code is missing");
                return;
            }
            ship_code = upper_ascii(ship_code);
            std::map<std::string, int> assignments;
            std::string token;
            while (iss >> token)
            {
                size_t eq = token.find('=');
                if (eq == std::string::npos)
                    continue;
                std::string k = to_lower(token.substr(0, eq));
                int v = std::atoi(token.substr(eq + 1).c_str());
                if (k == "beam" || k == "b")
                    k = "B";
                else if (k == "d" || k == "drive" || k == "pd")
                    k = "D";
                else if (k == "screen" || k == "s")
                    k = "S";
                else if (k == "tube" || k == "t")
                    k = "T";
                else if (k == "missiles" || k == "m")
                    k = "M";
                assignments[k] = v;
            }
            CombatEngine ce(db, a.game_id);
            eventText = ce.apply_damage(owner, ship_code, assignments);
        }
        else if (action == "list")
        {
            CombatEngine ce(db, a.game_id);
            auto list = ce.get_active_combats();
            eventText = "Active Combats: " + std::to_string(list.size());
        }
        else
        {
            resp->status = 400;
            resp->body = json_error("unknown combat action");
            Logger::instance().error("Unknown combat action attempted");
            return;
        }
    }
#endif

    // save_game(db, s);
    // append_event(db, a.game_id, a.user_id, cmdline, eventText, s);

    // Logger::instance().info("[" + std::string(1, owner) + "] " + cmdline +
    //                        " -> " + eventText);

    // resp->body = json_ok_with_state_and_event(s, eventText);
    return;
}
