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
#include "ships.h"
#include "combat.h"
#include "comms.h"
#include "db.h"
#include "events.h"
#include "logger.h"
#include "mapgraph.h"
#include "state.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern "C" int yyparse();

void handle_usr_command(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return;
    }

    AuthContext a = require_auth((const HttpRequest*)req, resp);

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

        GameState s = StateMachine::getInstance().load_game(a.game_id);

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

    //
    // BUGBUG
    ///////////

    // This is all legacy that has to be re-factored/rolled into more abstract
    // structure of commands...  soon..

    // BUGBUG      GameState s = load_game(db, a.game_id);
    // BUGBUG
    // BUGBUG      std::vector<std::string> tok = split_ws(cmdline);
    // BUGBUG      std::string cmd = to_lower(tok[0]);
    // BUGBUG
    // BUGBUG      std::string eventText;
    // BUGBUG
    // BUGBUG      // NOTE: 'me' is derived from the authenticated token, not
    // from game state. BUGBUG      char me = (a.player ? a.player : 'A');
    // BUGBUG      char owner = me;
    // BUGBUG      char enemy = (owner == 'A') ? 'B' : 'A';
    // BUGBUG      char active = (s.active_player.empty() ? 'A' :
    // s.active_player[0]); BUGBUG      auto turnToken = std::string("R") +
    // std::to_string(s.round) + active; BUGBUG BUGBUG      auto
    // require_build_phase = [&]() -> bool BUGBUG      { BUGBUG          if
    // (s.scenario.empty()) BUGBUG          { BUGBUG              eventText =
    // "No scenario. Type: start learning|basic|advanced"; BUGBUG
    // Logger::instance().error(eventText); BUGBUG              return false;
    // BUGBUG          }
    // BUGBUG          if (s.phase_index != PH_BUILD_SHIPS)
    // BUGBUG          {
    // BUGBUG              std::ostringstream o;
    // BUGBUG              o << "Not in Build Ships phase. Current: " <<
    // s.phase_name(); BUGBUG              eventText = o.str(); BUGBUG
    // Logger::instance().error(eventText); BUGBUG              return false;
    // BUGBUG          }
    // BUGBUG          return true;
    // BUGBUG      };
    // BUGBUG
    // BUGBUG      auto require_movement_phase = [&]() -> bool
    // BUGBUG      {
    // BUGBUG          if (s.scenario.empty())
    // BUGBUG          {
    // BUGBUG              eventText = "No scenario. Type: start
    // learning|basic|advanced"; BUGBUG              return false; BUGBUG }
    // BUGBUG          if (s.phase_index != PH_MOVEMENT)
    // BUGBUG          {
    // BUGBUG              std::ostringstream o;
    // BUGBUG              o << "Not in Movement phase. Current: " <<
    // s.phase_name(); BUGBUG              eventText = o.str(); BUGBUG
    // Logger::instance().error(eventText); BUGBUG              return false;
    // BUGBUG          }
    // BUGBUG          return true;
    // BUGBUG      };
    // BUGBUG
    // BUGBUG      auto require_my_turn = [&]() -> bool
    // BUGBUG      {
    // BUGBUG          if (active != me)
    // BUGBUG          {
    // BUGBUG              std::ostringstream o;
    // BUGBUG              o << "Not your turn. Active player is " << active <<
    // "."; BUGBUG              eventText = o.str(); BUGBUG
    // Logger::instance().error(eventText); BUGBUG              return false;
    // BUGBUG          }
    // BUGBUG          return true;
    // BUGBUG      };
    // BUGBUG
    // BUGBUG      auto ship_cost_bp = [&](char ship_type, const DraftRow &d) ->
    // int BUGBUG      { BUGBUG          int cost = 0; BUGBUG          cost +=
    // d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR; BUGBUG cost +=
    // (d.attr.M + 2) / BUGBUG                  3; // M is validated
    // multiple-of-3 elsewhere, but keep safe BUGBUG          if (ship_type ==
    // 'W') BUGBUG              cost += 5; // Warp generator BUGBUG return cost;
    // BUGBUG      };
    // BUGBUG
    // BUGBUG      auto compute_tech_level = [&]() -> int
    // BUGBUG      {
    // BUGBUG          if (s.scenario != "advanced")
    // BUGBUG              return 0;
    // BUGBUG          // Tech level increases every 4 game-turns (turns 1-4 =
    // 0, 5-8 = 1, ...) BUGBUG          if (s.round < 1) BUGBUG return 0; BUGBUG
    // return (s.round - 1) / 4; BUGBUG      }; BUGBUG BUGBUG      auto
    // fmt_attrs = [&](int PD, int B, int Sx, int T, int M, BUGBUG int SR) ->
    // std::string BUGBUG      { BUGBUG          std::ostringstream o; BUGBUG o
    // << "PD=" << PD << ", B=" << B << ", S=" << Sx << ", T=" << T BUGBUG << ",
    // M=" << M << ", SR=" << SR; BUGBUG          return o.str(); BUGBUG      };
    // BUGBUG
    // BUGBUG      auto list_fleet_text = [&](char whichOwner) -> std::string
    // BUGBUG      {
    // BUGBUG          auto ships = load_ships(db, a.game_id, whichOwner);
    // BUGBUG          std::ostringstream o;
    // BUGBUG          o << (whichOwner == me ? "Blue-force fleet:" : "Red-force
    // fleet:") BUGBUG            << "\n"; BUGBUG          if (ships.empty())
    // BUGBUG          {
    // BUGBUG              o << "  (none)\n";
    // BUGBUG              return o.str();
    // BUGBUG          }
    // BUGBUG          for (auto &sh : ships)
    // BUGBUG          {
    // BUGBUG              o << "  " << sh.name << " - " << sh.code << " (L" <<
    // sh.attr.tech BUGBUG                << ") " BUGBUG                <<
    // fmt_attrs(sh.attr.PD, sh.attr.B, sh.attr.S, sh.attr.T, BUGBUG sh.attr.M,
    // sh.attr.SR); BUGBUG              if (!sh.racked_in.empty()) BUGBUG o << "
    // [RACKED in " << sh.racked_in << "]"; BUGBUG              else if
    // (!sh.at_system.empty()) BUGBUG                  o << " @ " <<
    // sh.at_system; BUGBUG              else BUGBUG                  o << " @
    // (undeployed)"; BUGBUG              if (sh.attr.type == 'W' && sh.attr.SR
    // > 0) BUGBUG              { BUGBUG                  int cnt =
    // count_racked_in(db, a.game_id, whichOwner, sh.code); BUGBUG if (cnt > 0)
    // BUGBUG                  {
    // BUGBUG                      auto carried = db->query(
    // BUGBUG                          "SELECT ship_code FROM ships WHERE
    // game_id=" + BUGBUG                          std::to_string(a.game_id) + "
    // AND owner='" + BUGBUG                          std::string(1, whichOwner)
    // + "' AND racked_in='" + BUGBUG                          db->esc(sh.code)
    // + "' ORDER BY ship_code"); BUGBUG                      o << " carrying:";
    // BUGBUG                      for (auto &c : carried)
    // BUGBUG                          o << " " << c[0];
    // BUGBUG                  }
    // BUGBUG                  else
    // BUGBUG                  {
    // BUGBUG                      o << " carrying: (none)";
    // BUGBUG                  }
    // BUGBUG              }
    // BUGBUG              o << "\n";
    // BUGBUG          }
    // BUGBUG          return o.str();
    // BUGBUG      };
    // BUGBUG
    // BUGBUG      if (cmd == "status")
    // BUGBUG      {
    // BUGBUG          eventText = "Status refreshed.";
    // BUGBUG      }
    // BUGBUG      else if (cmd == "bases")
    // BUGBUG      {
    // BUGBUG          // Map/base-star configuration will move server-side
    // later; for now BUGBUG          // allow free-form system names. BUGBUG
    // std::ostringstream o; BUGBUG          o << "Base systems are not yet
    // configured server-side.\n" BUGBUG            << "Use 'deploy <W#|S##>
    // <SYSTEM>' with a system name (e.g., UR) for " BUGBUG "now."; BUGBUG
    // eventText = o.str(); BUGBUG          Logger::instance().error(eventText);
    // BUGBUG      }
    // BUGBUG      else if (cmd == "reset")
    // BUGBUG      {
    // BUGBUG          // Clear scenario and state; also clear DB ships/drafts
    // for the latest BUGBUG          // game BUGBUG          s.clear(); // =
    // GameState(); BUGBUG          s.game_id = a.game_id; BUGBUG
    // db->exec("DELETE FROM drafts WHERE game_id=" + BUGBUG
    // std::to_string(a.game_id)); BUGBUG          db->exec("DELETE FROM ships
    // WHERE game_id=" + BUGBUG                   std::to_string(a.game_id));
    // BUGBUG          set_current_draft(db, a.game_id, 'A', "");
    // BUGBUG          set_current_draft(db, a.game_id, 'B', "");
    // BUGBUG          eventText = "Game reset. Type: start
    // learning|basic|advanced"; BUGBUG      } BUGBUG      else if (cmd ==
    // "list") BUGBUG      { BUGBUG          if (tok.size() == 1) BUGBUG {
    // BUGBUG              eventText = list_fleet_text(owner) + "\n" +
    // list_fleet_text(enemy); BUGBUG          } BUGBUG          else BUGBUG {
    // BUGBUG              std::string sub = to_lower(tok[1]);
    // BUGBUG              if (sub == "drafts")
    // BUGBUG              {
    // BUGBUG                  eventText = "Use: build drafts (list drafts is
    // deprecated)."; BUGBUG              } BUGBUG              else if (sub ==
    // "system" && tok.size() >= 3) BUGBUG              { BUGBUG std::string sys
    // = resolve_system_name(db, a.game_id, tok[2]); BUGBUG auto aShips =
    // db->query( BUGBUG                      "SELECT " BUGBUG
    // "owner,ship_name,ship_code,ship_type,tech_level,pd,beam," BUGBUG
    // "screen,tube,missiles,sr,racked_in " BUGBUG                      "FROM
    // ships WHERE game_id=" + BUGBUG std::to_string(a.game_id) + " AND
    // at_system='" + BUGBUG                      db->esc(sys) + "' ORDER BY
    // owner,ship_code"); BUGBUG                  std::ostringstream o; BUGBUG
    // o << "Ships at " << sys << ":\n"; BUGBUG                  if
    // (aShips.empty()) BUGBUG                  { BUGBUG                      o
    // << "  (none)\n"; BUGBUG                      eventText = o.str(); BUGBUG
    // } BUGBUG                  else BUGBUG                  { BUGBUG for (auto
    // &r : aShips) BUGBUG                      { BUGBUG char ow = r[0].empty()
    // ? 'A' : r[0][0]; BUGBUG                          o << "  " << (ow ==
    // owner ? "Blue" : "Red") << ": " BUGBUG                            << r[1]
    // << " - " << r[2] << " (L" << r[4] << ") " BUGBUG <<
    // fmt_attrs(std::atoi(r[5].c_str()), BUGBUG std::atoi(r[6].c_str()), BUGBUG
    // std::atoi(r[7].c_str()), BUGBUG std::atoi(r[8].c_str()), BUGBUG
    // std::atoi(r[9].c_str()), BUGBUG std::atoi(r[10].c_str())) BUGBUG << "\n";
    // BUGBUG                      }
    // BUGBUG                      eventText = o.str();
    // BUGBUG                  }
    // BUGBUG              }
    // BUGBUG              else if (sub == "all")
    // BUGBUG              {
    // BUGBUG                  std::ostringstream o;
    // BUGBUG                  char me = a.player;
    // BUGBUG                  char enemy = (me == 'A') ? 'B' : 'A';
    // BUGBUG                  o << list_fleet_text(me) << "\n" <<
    // list_fleet_text(enemy); BUGBUG                  eventText = o.str();
    // BUGBUG              }
    // BUGBUG              else if (sub == "scan")
    // BUGBUG              {
    // BUGBUG                  eventText =
    // BUGBUG                      "list scan: not implemented yet (sightings
    // table is " BUGBUG                      "present for later)."; BUGBUG }
    // BUGBUG              else
    // BUGBUG              {
    // BUGBUG                  eventText =
    // BUGBUG                      "Usage: list | list drafts | list system
    // <SYS> | list " BUGBUG                      "all | list scan"; BUGBUG }
    // BUGBUG              Logger::instance().info(eventText);
    // BUGBUG          }
    // BUGBUG      }
    // BUGBUG      else if (cmd == "pickup" || cmd == "drop")
    // BUGBUG      {
    // BUGBUG          if (!require_my_turn() || !require_build_phase())
    // BUGBUG          {
    // BUGBUG              // eventText already set
    // BUGBUG          }
    // BUGBUG          else if (tok.size() < 3)
    // BUGBUG          {
    // BUGBUG              eventText = std::string("Usage: ") + cmd + " <W#>
    // <S##>"; BUGBUG          } BUGBUG          else BUGBUG          { BUGBUG
    // std::string wcode = tok[1]; BUGBUG              std::string scode =
    // tok[2]; BUGBUG BUGBUG              if (!ship_exists(db, a.game_id, owner,
    // wcode)) BUGBUG              { BUGBUG                  eventText =
    // "Warpship not found: " + wcode; BUGBUG              } BUGBUG else if
    // (!ship_exists(db, a.game_id, owner, scode)) BUGBUG              { BUGBUG
    // eventText = "Systemship not found: " + scode; BUGBUG              }
    // BUGBUG              else
    // BUGBUG              {
    // BUGBUG                  ShipRow w = load_ship(db, a.game_id, owner,
    // wcode); BUGBUG                  ShipRow sship = load_ship(db, a.game_id,
    // owner, scode); BUGBUG                  if (w.attr.type != 'W') BUGBUG {
    // BUGBUG                      eventText = "Not a Warpship: " + wcode;
    // BUGBUG                  }
    // BUGBUG                  else if (sship.attr.type != 'S')
    // BUGBUG                  {
    // BUGBUG                      eventText = "Not a Systemship: " + scode;
    // BUGBUG                  }
    // BUGBUG                  else if (w.built_turn != turnToken ||
    // BUGBUG                           sship.built_turn != turnToken)
    // BUGBUG                  {
    // BUGBUG                      eventText = "Pre-rack rule: both ships must
    // be committed " BUGBUG                                  "this same turn ("
    // + BUGBUG                                  turnToken + ")."; BUGBUG }
    // BUGBUG                  else if (cmd == "pickup")
    // BUGBUG                  {
    // BUGBUG                      if (w.at_system.empty() ||
    // sship.at_system.empty()) BUGBUG                      { BUGBUG eventText =
    // "Both ships must be deployed to the same " BUGBUG "system first."; BUGBUG
    // } BUGBUG                      else if (w.at_system != sship.at_system)
    // BUGBUG                      {
    // BUGBUG                          eventText = "Not co-located: " + wcode +
    // "@" + BUGBUG                                      w.at_system + " vs " +
    // scode + "@" + BUGBUG sship.at_system; BUGBUG                      }
    // BUGBUG                      else if (!sship.racked_in.empty())
    // BUGBUG                      {
    // BUGBUG                          eventText =
    // BUGBUG                              "Systemship already racked in " +
    // sship.racked_in; BUGBUG                      } BUGBUG else BUGBUG {
    // BUGBUG                          int carried =
    // BUGBUG                              count_racked_in(db, a.game_id, owner,
    // w.code); BUGBUG                          if (carried >= w.attr.SR) BUGBUG
    // { BUGBUG                              std::ostringstream o; BUGBUG o <<
    // "No SR capacity. SR=" << w.attr.SR BUGBUG << ", carrying=" << carried;
    // BUGBUG                              eventText = o.str();
    // BUGBUG                          }
    // BUGBUG                          else
    // BUGBUG                          {
    // BUGBUG                              update_ship_location(db, a.game_id,
    // owner, scode, BUGBUG "", w.at_hex, wcode); BUGBUG eventText = "Picked up
    // " + sship.name + " - " + BUGBUG sship.code + " into " + w.name + " - " +
    // BUGBUG                                          w.code;
    // BUGBUG                          }
    // BUGBUG                      }
    // BUGBUG                  }
    // BUGBUG                  else
    // BUGBUG                  { // drop
    // BUGBUG                      if (sship.racked_in != wcode)
    // BUGBUG                      {
    // BUGBUG                          eventText = "Systemship is not racked in
    // " + wcode; BUGBUG                      } BUGBUG                      else
    // BUGBUG                      {
    // BUGBUG                          if (w.at_system.empty())
    // BUGBUG                          {
    // BUGBUG                              eventText = "Warpship must be
    // deployed to a system " BUGBUG "to drop."; BUGBUG } BUGBUG else BUGBUG {
    // BUGBUG                              update_ship_location(db, a.game_id,
    // owner, scode, BUGBUG w.at_system, w.at_hex, ""); BUGBUG eventText =
    // "Dropped " + sship.name + " - " + BUGBUG sship.code + " at " +
    // w.at_system; BUGBUG                          } BUGBUG } BUGBUG } BUGBUG }
    // BUGBUG          }
    // BUGBUG      }
    // BUGBUG      else if (cmd == "combat")
    // BUGBUG      {
    // BUGBUG          std::istringstream iss(cmdline);
    // BUGBUG          std::string cmdName;
    // BUGBUG          iss >> cmdName; // "combat"
    // BUGBUG
    // BUGBUG          std::string action;
    // BUGBUG          iss >> action;
    // BUGBUG          if (action == "order")
    // BUGBUG          {
    // BUGBUG              // combat order <ship> [tactic=A|D|R] [target=ID]
    // [d=N] [b=N] [s=N] BUGBUG              // [t=N] [m=JSON] BUGBUG
    // CombatOrder ord; BUGBUG              ord.game_id = a.game_id; BUGBUG
    // ord.round = 0; // Set by engine BUGBUG BUGBUG              // Defaults
    // BUGBUG              ord.tactic = 'A'; // Default to Attack
    // BUGBUG              ord.target_id = "";
    // BUGBUG              ord.power_d = 0;
    // BUGBUG              ord.power_b = 0;
    // BUGBUG              ord.power_s = 0;
    // BUGBUG              ord.power_t = 0;
    // BUGBUG              ord.missiles_json = "[]";
    // BUGBUG
    // BUGBUG              if (!(iss >> ord.ship_code))
    // BUGBUG              {
    // BUGBUG                  resp->body = json_error("missing ship code");
    // BUGBUG                  return;
    // BUGBUG              }
    // BUGBUG              ord.ship_code = upper_ascii(ord.ship_code);
    // BUGBUG
    // BUGBUG              std::string token;
    // BUGBUG              while (iss >> token)
    // BUGBUG              {
    // BUGBUG                  size_t eq = token.find('=');
    // BUGBUG                  if (eq == std::string::npos)
    // BUGBUG                  {
    // BUGBUG                      // check for implicit tactic (A, D, R)
    // BUGBUG                      if (token.size() == 1)
    // BUGBUG                      {
    // BUGBUG                          char c = std::toupper(token[0]);
    // BUGBUG                          if (c == 'A' || c == 'D' || c == 'R')
    // BUGBUG                          {
    // BUGBUG                              ord.tactic = c;
    // BUGBUG                              continue;
    // BUGBUG                          }
    // BUGBUG                      }
    // BUGBUG                      // Implicit target ID (e.g. "W1" in "combat
    // order W2 D W1") BUGBUG                      ord.target_id = token; BUGBUG
    // continue; BUGBUG                  } BUGBUG BUGBUG std::string key =
    // to_lower(token.substr(0, eq)); BUGBUG                  std::string val =
    // token.substr(eq + 1); BUGBUG BUGBUG                  if (key == "tactic"
    // || key == "mode" || key == "opt") BUGBUG                  { BUGBUG if
    // (!val.empty()) BUGBUG                          ord.tactic =
    // std::toupper(val[0]); BUGBUG                  } BUGBUG else if (key ==
    // "target" || key == "tgt") BUGBUG                  { BUGBUG ord.target_id
    // = val; BUGBUG                  } BUGBUG                  else if (key ==
    // "d" || key == "drive") BUGBUG                      ord.power_d =
    // std::atoi(val.c_str()); BUGBUG                  else if (key == "b" ||
    // key == "beam") BUGBUG                      ord.power_b =
    // std::atoi(val.c_str()); BUGBUG                  else if (key == "s" ||
    // key == "screen") BUGBUG                      ord.power_s =
    // std::atoi(val.c_str()); BUGBUG                  else if (key == "t" ||
    // key == "tube") BUGBUG                      ord.power_t =
    // std::atoi(val.c_str()); BUGBUG                  else if (key == "m" ||
    // key == "missiles") BUGBUG                      ord.missiles_json = val;
    // BUGBUG              }
    // BUGBUG
    // BUGBUG              // No strict syntax check needed, defaults apply.
    // BUGBUG
    // BUGBUG              std::string candidate_target(ord.target_id);
    // BUGBUG              Logger::instance().info(candidate_target);
    // BUGBUG
    // BUGBUG              // Validation
    // BUGBUG              if (ord.tactic == 'D' && ord.target_id.empty())
    // BUGBUG              {
    // BUGBUG                  eventText =
    // BUGBUG                      "Combat order to dodge requires a target
    // opponent ship"; BUGBUG                  // Don't submit BUGBUG } BUGBUG
    // else BUGBUG              { BUGBUG                  CombatEngine ce(db,
    // a.game_id); BUGBUG                  eventText = ce.submit_order(owner,
    // ord); BUGBUG              } BUGBUG          } BUGBUG          else if
    // (action == "resolve") BUGBUG          { BUGBUG              std::string
    // hex; BUGBUG              iss >> hex; BUGBUG              if (hex.empty())
    // BUGBUG              {
    // BUGBUG                  resp->status = 400;
    // BUGBUG                  resp->body = json_error("missing hex");
    // BUGBUG                  Logger::instance().error(
    // BUGBUG                      "Trying to resolve combat, hex ID is
    // missing"); BUGBUG                  return; BUGBUG              } BUGBUG
    // CombatEngine ce(db, a.game_id); BUGBUG              eventText =
    // ce.resolve_round(hex); BUGBUG          } BUGBUG          else if (action
    // == "apply") BUGBUG          { BUGBUG              // combat apply <ship>
    // <attr=val>... BUGBUG              std::string ship_code; BUGBUG if (!(iss
    // >> ship_code)) BUGBUG              { BUGBUG                  resp->body =
    // json_error("missing ship code"); BUGBUG Logger::instance().error( BUGBUG
    // "Trying combat subcommand apply, ship code is missing"); BUGBUG return;
    // BUGBUG              }
    // BUGBUG              ship_code = upper_ascii(ship_code);
    // BUGBUG              std::map<std::string, int> assignments;
    // BUGBUG              std::string token;
    // BUGBUG              while (iss >> token)
    // BUGBUG              {
    // BUGBUG                  size_t eq = token.find('=');
    // BUGBUG                  if (eq == std::string::npos)
    // BUGBUG                      continue;
    // BUGBUG                  std::string k = to_lower(token.substr(0, eq));
    // BUGBUG                  int v = std::atoi(token.substr(eq + 1).c_str());
    // BUGBUG                  if (k == "beam" || k == "b")
    // BUGBUG                      k = "B";
    // BUGBUG                  else if (k == "d" || k == "drive" || k == "pd")
    // BUGBUG                      k = "D";
    // BUGBUG                  else if (k == "screen" || k == "s")
    // BUGBUG                      k = "S";
    // BUGBUG                  else if (k == "tube" || k == "t")
    // BUGBUG                      k = "T";
    // BUGBUG                  else if (k == "missiles" || k == "m")
    // BUGBUG                      k = "M";
    // BUGBUG                  assignments[k] = v;
    // BUGBUG              }
    // BUGBUG              CombatEngine ce(db, a.game_id);
    // BUGBUG              eventText = ce.apply_damage(owner, ship_code,
    // assignments); BUGBUG          } BUGBUG          else if (action ==
    // "list") BUGBUG          { BUGBUG              CombatEngine ce(db,
    // a.game_id); BUGBUG              auto list = ce.get_active_combats();
    // BUGBUG              eventText = "Active Combats: " +
    // std::to_string(list.size()); BUGBUG          } BUGBUG          else
    // BUGBUG          {
    // BUGBUG              resp->status = 400;
    // BUGBUG              resp->body = json_error("unknown combat action");
    // BUGBUG              Logger::instance().error("Unknown combat action
    // attempted"); BUGBUG              return; BUGBUG        } BUGBUG    }
    // BUGBUG    // save_game(db, s);
    // BUGBUG    // append_event(db, a.game_id, a.user_id, cmdline, eventText,
    // s); BUGBUG BUGBUG    // Logger::instance().info("[" + std::string(1,
    // owner) + "] " + cmdline + BUGBUG    //                        " -> " +
    // eventText); BUGBUG BUGBUG    // resp->body =
    // json_ok_with_state_and_event(s, eventText);

    //
    // BUGBUG

    return;
}
