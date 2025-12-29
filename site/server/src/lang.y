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
%{
#include <cstdio>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "typedefs.h"

#include "logger.h"
#include "start_command.h"
#include "build_command.h"
#include "build_commit_command.h"
#include "build_new_command.h"
#include "build_cancel_command.h"
#include "build_list_drafts_command.h"
#include "build_show_draft_command.h"
#include "build_set_attribute_command.h"
#include "next_command.h"
#include "done_command.h"
#include "deploy_command.h"
#include "move_command.h"
#include "statemachine.h"
#include "game.h"
#include "db.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);

// Globals to inject context into the parser actions
extern Db* g_db;
extern int g_game_id;

// Global builder for accumulating build set attributes
BuildSetAttributeCommand::Builder g_build_set_builder;

%}

%code requires {
#include <string>
#include <vector>
}

%union {
   int ival;
   std::string* sval;
   //std::vector<std::string>* vec_sval;
   std::vector<std::string>* vec_sval;
}

%token <ival> TOK_INT
%token <sval> TOK_STRING
%type  <sval> deployable_ship
%type  <sval> building_draft_ship
%type  <vec_sval> chain_move_location

%token TOK_ADVANCED
%token TOK_APPLY
%token TOK_ATTACK
%token TOK_BASIC
%token TOK_BEAM
%token TOK_BUILD
%token TOK_CANCEL
%token TOK_COMBAT
%token TOK_COMMIT
%token TOK_CRT
%token TOK_DELETE
%token TOK_DEPLOY
%token TOK_DODGE
%token TOK_DONE
%token TOK_DRAFTS
%token TOK_DROP
%token TOK_ESCAPE
%token TOK_FLEET
%token TOK_GALAXY
%token TOK_HELP
%token TOK_HEX
%token TOK_LEARNING
%token TOK_LIST
%token TOK_LOAD
%token TOK_MISSILE
%token TOK_MOVE
%token TOK_NEW
%token TOK_NEXT
%token TOK_NEXT_SHORT
%token TOK_ONLINE
%token TOK_ORDER
%token TOK_PICK
%token TOK_POWER_DRIVE
%token TOK_QUIT
%token TOK_REPAIR
%token TOK_RESET
%token TOK_RESUPPLY
%token TOK_SAVE
%token TOK_SCREEN
%token TOK_SET_ATTR
%token TOK_START_GAME
%token TOK_STATS
%token TOK_STATUS
%token TOK_SYSTEM
%token TOK_TUBE

/* Attribute assignment tokens - value in yylval.ival */
%token <ival> TOK_PD_ASSIGN
%token <ival> TOK_B_ASSIGN
%token <ival> TOK_S_ASSIGN
%token <ival> TOK_T_ASSIGN
%token <ival> TOK_M_ASSIGN
%token <ival> TOK_SR_ASSIGN

%token TOK_UNKNOWN

%%

commands:
  // no command
  | commands command
  ;

command:
  help_cmd
  | session_cmd
  | info_cmd
  | looking_cmd
  | turn_cmd
  | combat_cmd
  | build_cmd
  | deploy_cmd
  | move_cmd
  | pickdrop_cmd
  | rep_cmd
  ;

session_cmd:
   TOK_START_GAME TOK_LEARNING
   {
        Logger::instance().info("Start a learning game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_db(g_db)
                      .set_game_id(g_game_id)
                      .set_scenario(ScenarioType::LEARNING)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_START_GAME TOK_BASIC
   {
        Logger::instance().info("Start a basic game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_db(g_db)
                      .set_game_id(g_game_id)
                      .set_scenario(ScenarioType::BASIC)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_START_GAME TOK_ADVANCED
   {
        Logger::instance().info("Start an advanced game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_db(g_db)
                      .set_game_id(g_game_id)
                      .set_scenario(ScenarioType::ADVANCED)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_RESET
   {
        Logger::instance().info("Reset the current game, wiping everything");
   }
   |
   TOK_SAVE TOK_STRING
   {
        // the name of the 'thing' to save is supplied by the user.
        // it's  in the same vein as a filename.. just a-zA-Z0-9
        std::string save_record(*$2);
        Logger::instance().info("Save game with id >"
                                + save_record + "<");
   }
   |
   TOK_LOAD
   {
        Logger::instance().info("List the games that are saved, if any");
   }
   |
   TOK_LOAD TOK_STRING
   {
        // the user can learn the filenames (they aren't files, they are
        // records in the DB) that are loadable by the name used when
        // the game was saved.
        std::string load_record(*$2);
        Logger::instance().info("Load game >"
                     + load_record + "<");
   }
   | TOK_DELETE {
        // deleting the most recent game saved, if any
        Logger::instance().info("Delete the most recent saved");
   }
   | TOK_DELETE TOK_STRING {
        // delete the named game (the same naming convention used for saving,
        // loading)
        std::string target_game(*$2);
        Logger::instance().info("Delete the game saved under "
               ">" + target_game + "<");
   }
   |
   TOK_QUIT
   {
        // quit the game.  it will automatically save the game under the name
        // 'lastgame' overwriting lastgame.
        // the user has to 'load lastgame' to retrieve it when they play again
        // Everythign about the state of the game, ships, everything is saved
        //
        // Stateful conditions like player A and player B are logged in must be
        // true of coure for the State to be in GAME_START
        Logger::instance().info("Quit the game. Same as Reset but unsets game scenario.");
   }
   ;

// Information
// drafts is unique -- contextual (for combat, building, applying damage) 
// list is unique -- contextual (for combat, building)

info_cmd:
   TOK_STATUS {
      Logger::instance().info("Show Game Status, important records.");
   }
   | TOK_CRT {
      Logger::instance().info("Show the CRT");
   }
   ;

looking_cmd:
  TOK_HEX TOK_STRING {
      std::string identifier(*$2);
      Logger::instance().info("Location and spatial information "
                              "about current hex named >" + identifier + "<");
  }
  | TOK_ONLINE {
      Logger::instance().info("Who's online and when?");
  }
  | TOK_STATS {
      Logger::instance().info("Statistics about game, fleet, opponent");
  }
  | TOK_FLEET {
      Logger::instance().info("Manifest of your fleet");
  }
  | TOK_SYSTEM TOK_STRING {
      std::string identifier(*$2);
      Logger::instance().info("Environmental information about "
                              "system named >" + identifier + "<");
  }
  | TOK_GALAXY {
      Logger::instance().info("Complete run-down of all systems, "
                              " all ships in the game.");
  }
  ;

turn_cmd:
  TOK_NEXT {
      Logger::instance().info("Advance active player to next phase");
      ICmd* pCmd = NextCommand::Builder(g_db, g_game_id).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_DONE {
      Logger::instance().info("Advance active player to first phase "
                      "of opponent, if possible");
      ICmd* pCmd = DoneCommand::Builder(g_db, g_game_id).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  ;

combat_order_commitment_cmd:
   TOK_CANCEL {
      Logger::instance().info("Cancel whatever the current operation "
                "of the current operation in progress. Example:  "
                "build cancel, combat cancel. Doesn't advance the phase, "
                " just undo everything that was proposed IN THE PHASE.");
   }
   | TOK_COMMIT {
      Logger::instance().info("Commit whatever the current operation "
                "of the current operation in progress. Example: "
                "build commit, combat commit. The operation is "
                "effective upon the commit. REMAIN IN PHASE. ");
   }
   ;

damage_allotment_commitment_cmd:
   TOK_CANCEL {
      Logger::instance().info("Cancel whatever the current operation "
                "of the current operation in progress. Example:  "
                "build cancel, combat cancel. Doesn't advance the phase, "
                " just undo everything that was proposed IN THE PHASE.");
   }
   | TOK_COMMIT {
      Logger::instance().info("Commit whatever the current operation "
                "of the current operation in progress. Example: "
                "build commit, combat commit. The operation is "
                "effective upon the commit. REMAIN IN PHASE. ");
   }
   ;

// combat and combat sub-commands
// "combat" { return TOK_COMBAT; }
// "order" { return TOK_ORDER; }
// "apply" { return TOK_APPLY; }

combat_cmd:
   TOK_COMBAT {
      Logger::instance().info("Status of combat situation");
   }
   | TOK_COMBAT TOK_ORDER combat_initiator_ship combat_tactic combat_target_ship combat_order_spec {
      Logger::instance().info("Combat order spec: ");
   }
   | TOK_COMBAT TOK_APPLY combat_damaged_ship combat_application_spec {
      // TODO (used by the non-active player)
      Logger::instance().info("Combat application spec: ");
   }
   | TOK_COMBAT TOK_ORDER combat_order_commitment_cmd {
      // TODO
   }
   | TOK_COMBAT TOK_APPLY damage_allotment_commitment_cmd {
      // TODO (used by the non-active player)
   }
  ;


building_draft_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Drafting ship: "
                              ">" + ship_id + "<" );
      $$ = $1;  // Pass the string up
  }
  ;

combat_initiator_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Combat ship initiator: "
                              ">" + ship_id + "<" );
  }
  ;

combat_damaged_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Combat damaged ship: "
                              ">" + ship_id + "<" );
  }
  ;

combat_tactic:
  TOK_ATTACK {
      Logger::instance().info("Combat Attack Tactic");
  }
  | TOK_DODGE {
      Logger::instance().info("Combat Dodge Tactic");
  }
  | TOK_ESCAPE {
      Logger::instance().info("Combat Escape Tactic");
  }
  ;

combat_target_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Combat target ship: "
                              ">" + ship_id + "<");
  }
  ;

combat_order_spec:
  TOK_PD_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec PD ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_B_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec BEAM ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_S_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec SCREEN ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_T_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec TUBE ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_M_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec MISSILE ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  ;

additional_combat_order_spec:
  // no spec
  | TOK_PD_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec PD ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_B_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec BEAM ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_S_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec SCREEN ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_T_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec TUBE ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  | TOK_M_ASSIGN additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec MISSILE ";
      ss << $1;
      Logger::instance().info(ss.str());
  }
  ;

// Ship Attribute labels
// "pd" { return TOK_POWER_DRIVE; }
// "d" { return TOK_POWER_DRIVE; }
// "beam" { return TOK_BEAM; }
// "b" { return TOK_BEAM; }
// "screen" { return TOK_SCREEN; }
// "s" { return TOK_SCREEN; }
// "tube" { return TOK_TUBE; }
// "t" { return TOK_TUBE; }
// "missile" { return TOK_MISSILE; }
// "m" { return TOK_MISSILE; }

combat_application_spec:
  TOK_PD_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec PD ";
      Logger::instance().info(ss.str());
  }
  | TOK_B_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec BEAM ";
      Logger::instance().info(ss.str());
  }
  | TOK_S_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec SCREEN ";
      Logger::instance().info(ss.str());
  }
  | TOK_T_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec TUBE ";
      Logger::instance().info(ss.str());
  }
  | TOK_M_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec MISSILE ";
      Logger::instance().info(ss.str());
  }
  ;

additional_combat_application_spec:
  // no spec
  | TOK_PD_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec PD ";
      Logger::instance().info(ss.str());
  }
  | TOK_B_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec BEAM ";
      Logger::instance().info(ss.str());
  }
  | TOK_S_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec SCREEN ";
      Logger::instance().info(ss.str());
  }
  | TOK_T_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec TUBE ";
      Logger::instance().info(ss.str());
  }
  | TOK_M_ASSIGN additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec MISSILE ";
      Logger::instance().info(ss.str());
  }
  ;

// building and building sub-commands
// "build" { return TOK_BUILD; }
// "new" { return TOK_NEW; }
// "set" { return TOK_SET_ATTR; }

// Ship Attribute labels
// "pd" { return TOK_POWER_DRIVE; }
// "d" { return TOK_POWER_DRIVE; }
// "beam" { return TOK_BEAM; }
// "b" { return TOK_BEAM; }
// "screen" { return TOK_SCREEN; }
// "s" { return TOK_SCREEN; }
// "tube" { return TOK_TUBE; }
// "t" { return TOK_TUBE; }
// "missile" { return TOK_MISSILE; }
// "m" { return TOK_MISSILE; }

build_cmd:
  TOK_BUILD {
      Logger::instance().info("Without arguments, shows "
                              "current build state");
  }
  | TOK_BUILD TOK_NEW TOK_STRING TOK_STRING {
      std::string code(*$3);
      std::string name(*$4);
      Logger::instance().info("Create new draft: " + code + " '" + name + "'");
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_BUILD TOK_DRAFTS {
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_BUILD TOK_DRAFTS building_draft_ship {
      std::string ship_code = *$3;
      Logger::instance().info("Show draft details: " + ship_code);
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .set_draft_code(ship_code)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_BUILD TOK_SET_ATTR build_attr_spec {
      Logger::instance().info("Set attributes on current draft");
      ICmd *pCmd = g_build_set_builder
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
      g_build_set_builder = BuildSetAttributeCommand::Builder(); // Reset
  }
  | TOK_BUILD TOK_COMMIT {
      Logger::instance().info("Build commit - committing current draft");
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_BUILD TOK_CANCEL {
      Logger::instance().info("Build cancel - canceling current draft");
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .set_db(g_db)
                  .set_game_id(g_game_id)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  ;

build_attr_spec:
  TOK_PD_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_pd($1);
  }
  | TOK_B_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_b($1);
  }
  | TOK_S_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_s($1);
  }
  | TOK_T_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_t($1);
  }
  | TOK_M_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_m($1);
  }
  | TOK_SR_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_sr($1);
  }
  ;

additional_build_attr_spec:
  // or no more spec
  | TOK_PD_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_pd($1);
  }
  | TOK_B_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_b($1);
  }
  | TOK_S_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_s($1);
  }
  | TOK_T_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_t($1);
  }
  | TOK_M_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_m($1);
  }
  | TOK_SR_ASSIGN additional_build_attr_spec {
      g_build_set_builder.set_sr($1);
  }
  ;

// deployment
// "deploy" { return TOK_DEPLOY; }
deployable_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Deployable ship: "
                              ">" + ship_id + "<" );
      $$ = $1;
  }
  ;

deploy_cmd:
   TOK_DEPLOY {
       Logger::instance().info("Show current self deployment");
       Logger::instance().info("Show opponent deployment");
   }
   | TOK_DEPLOY deployable_ship TOK_STRING {
       std::string ship(*$2);
       std::string destination(*$3);
       Logger::instance().info("Attempt to deploy ship "
                               ">" + ship + "<" 
                               " at destination: "
                               ">" + destination + "<");
       ICmd* pCmd = DeployCommand::Builder(g_db, g_game_id)
           .ship_code(ship)
           .system_name(destination)
           .build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       delete $2;
       delete $3;
   }
  ;

target_systemship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Target SystemShip: "
                              ">" + ship_id + "<");
  }
  ;

// Movement
// "move" { return TOK_MOVE; }
chain_move_location:
   /* empty - no more waypoints */ {
       $$ = new std::vector<std::string>;
   }
   | TOK_STRING chain_move_location {
       std::string waypoint(*$1);
       Logger::instance().info("Additional waypoint on chain: >" + waypoint + "<");
       $$ = $2;  // Take the vector from the recursive call
       $$->insert($$->begin(), waypoint);  // Prepend this waypoint
       delete $1;
   }
   ;


move_cmd:
  TOK_MOVE {
       Logger::instance().info("Show move status");
  }
  | TOK_MOVE TOK_STRING TOK_STRING chain_move_location {
       std::string ship(*$2);
       std::string first_dest(*$3);
       std::vector<std::string>* waypoints = $4;
       
       Logger::instance().info("Moving ship >" + ship + "< to >" + first_dest + "<");
       
       // Build move command with all destinations
       MoveCommand::Builder builder(g_db, g_game_id);
       builder.ship_code(ship);
       builder.add_destination(first_dest);
       
       // Add chained waypoints
       for (const auto& wp : *waypoints) {
           builder.add_destination(wp);
       }
       
       ICmd* pCmd = builder.build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       
       delete $2;
       delete $3;
       delete waypoints;
  }
  ;


// Pickup, Drop
// "pick" { return TOK_PICK; }
// "drop" { return TOK_DROP; }
warpship_pick_destination:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Pick destination warpship: "
                              ">" + ship_id + "<");
  }
  ;
warpship_drop_source:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Drop source warpship: "
                              ">" + ship_id + "<");
  }
  ;
pickdrop_cmd:
  TOK_PICK TOK_STRING {
     std::string location(*$2);
     Logger::instance().info("Status of what can be picked up at hex "
                 ">" + location + "<");
  }
  // syntax: drop WARPSHIP_ID
  | TOK_DROP warpship_drop_source {
     Logger::instance().info("Only list what may be dropped from "
                             "target WarpShip");
  }
  // syntax: pick SOURCE DESTINATION */
  | TOK_PICK target_systemship warpship_pick_destination {
     Logger::instance().info("Picking up SystemShip to rack "
                             "in target WarpShip");

  }
  | TOK_DROP target_systemship warpship_drop_source {
     Logger::instance().info("Dropping target SystemShipship from "
                             "target WarpShip");
  }
  ;


rep_cmd:
  TOK_REPAIR {
     // No arguments, always just explains briefly what you can do.
     Logger::instance().info("List the ships that can be repaired by "
                "location, type of ship and their current attributes.");
  }
  | TOK_REPAIR TOK_STRING {
     // specify the ship always just explains briefly what you can do.
     std::string repairable_ship(*$2);
     Logger::instance().info("Repair ship >" + repairable_ship + "<");
  }
  | TOK_RESUPPLY {
     // No arguments, always just explains briefly what you can do.
     Logger::instance().info("List/show how many BP you have and what "
                "kind of support you can provide.  The rules state that "
                "all that can be 'resupplied' are MISSILES. For later work "
                "we might revisit this for more interesting gameplay");
  }
  | TOK_RESUPPLY TOK_STRING TOK_INT {
     // specify the ship always just explains briefly what you can do.
     // and how many MISSILES to resupply from BuildPoints
     std::string ship(*$2);
     int qty = (int) $3;
     std::string tmp = "" + qty;
     Logger::instance().info("Resupply ship >" + ship + "< "
                   "with >" + tmp + "< MISSILES");
  }
  ; 
 
 
// Cellar -- things that are misc

// "help" { return TOK_HELP; }
// "?" { return TOK_HELP; }

help_cmd:
  TOK_HELP
  {
       // show help screen
       // show all topics
      Logger::instance().info("Show help screen");
      Logger::instance().info("List all help topics");
  }
  | help_composite
  ;

help_composite:
  TOK_HELP TOK_STRING
  {
      Logger::instance().info("Help about topic");
      std::string topic(*$2);

      Logger::instance().info(topic);
  }
  | help_command
  ;

help_command:
    TOK_HELP TOK_SET_ATTR
    {
        Logger::instance().info("Help about TOK_SET_ATTR");
    }
    | TOK_HELP TOK_APPLY
    {
        Logger::instance().info("Help about TOK_APPLY");
    }
    | TOK_HELP TOK_BEAM
    {
        Logger::instance().info("Help about TOK_BEAM");
    }
    | TOK_HELP TOK_BUILD
    {
        Logger::instance().info("Help about TOK_BUILD");
    }
    | TOK_HELP TOK_CANCEL
    {
        Logger::instance().info("Help about TOK_CANCEL");
    }
    | TOK_HELP TOK_COMBAT
    {
        Logger::instance().info("Help about TOK_COMBAT");
    }
    | TOK_HELP TOK_COMMIT
    {
        Logger::instance().info("Help about TOK_COMMIT");
    }
    | TOK_HELP TOK_DEPLOY
    {
        Logger::instance().info("Help about TOK_DEPLOY");
    }
    | TOK_HELP TOK_DONE
    {
        Logger::instance().info("Help about TOK_DONE");
    }
    | TOK_HELP TOK_DRAFTS
    {
        Logger::instance().info("Help about TOK_DRAFTS");
    }
    | TOK_HELP TOK_DROP
    {
        Logger::instance().info("Help about TOK_DROP");
    }
    | TOK_HELP TOK_FLEET
    {
        Logger::instance().info("Help about TOK_FLEET");
    }
    | TOK_HELP TOK_GALAXY
    {
        Logger::instance().info("Help about TOK_GALAXY");
    }
    | TOK_HELP TOK_HELP
    {
        Logger::instance().info("Help about TOK_HELP");
    }
    | TOK_HELP TOK_HEX
    {
        Logger::instance().info("Help about TOK_HEX");
    }
    | TOK_HELP TOK_LIST
    {
        Logger::instance().info("Help about TOK_LIST");
    }
    | TOK_HELP TOK_MISSILE
    {
        Logger::instance().info("Help about TOK_MISSILE");
    }
    | TOK_HELP TOK_MOVE
    {
        Logger::instance().info("Help about TOK_MOVE");
    }
    | TOK_HELP TOK_NEW
    {
        Logger::instance().info("Help about TOK_NEW");
    }
    | TOK_HELP TOK_NEXT
    {
        Logger::instance().info("Help about TOK_NEXT");
    }
    | TOK_HELP TOK_NEXT_SHORT
    {
        Logger::instance().info("Help about TOK_NEXT_SHORT");
    }
    | TOK_HELP TOK_ONLINE
    {
        Logger::instance().info("Help about TOK_ONLINE");
    }
    | TOK_HELP TOK_ORDER
    {
        Logger::instance().info("Help about TOK_ORDER");
    }
    | TOK_HELP TOK_PICK
    {
        Logger::instance().info("Help about TOK_PICK");
    }
    | TOK_HELP TOK_POWER_DRIVE
    {
        Logger::instance().info("Help about TOK_POWER_DRIVE");
    }
    | TOK_HELP TOK_RESET
    {
        Logger::instance().info("Help about TOK_RESET");
    }
    | TOK_HELP TOK_SCREEN
    {
        Logger::instance().info("Help about TOK_SCREEN");
    }
    | TOK_HELP TOK_START_GAME
    {
        Logger::instance().info("Help about TOK_START_GAME");
    }
    | TOK_HELP TOK_STATS
    {
        Logger::instance().info("Help about TOK_STATS");
    }
    | TOK_HELP TOK_STATUS
    {
        Logger::instance().info("Help about TOK_STATUS");
    }
    | TOK_HELP TOK_SYSTEM
    {
        Logger::instance().info("Help about TOK_SYSTEM");
    }
    | TOK_HELP TOK_TUBE
    {
        Logger::instance().info("Help about TOK_TUBE");
    }
    | TOK_HELP TOK_UNKNOWN
    {
        Logger::instance().info("Help about TOK_UNKNOWN");
    }
    ;




%%
void yyerror(const char* s)
{
   Logger::instance().error("Parse error: " + std::string(s));
}
