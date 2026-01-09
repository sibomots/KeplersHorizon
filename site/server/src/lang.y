//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
// 
// Copyright (c) 2025, sibomots
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
#include "combat_order_command.h"
#include "combat_apply_command.h"
#include "combat_drafts_command.h"
#include "combat_commit_command.h"
#include "combat_cancel_command.h"
#include "fleet_command.h"
#include "cargo_command.h"
#include "system_command.h"
#include "survey_command.h"
#include "repair_command.h"
#include "resupply_command.h"
#include "score_command.h"
#include "extract_command.h"
#include "market_command.h"
#include "trade_command.h"
#include "fabricate_command.h"
#include "outfit_command.h"
#include "salvage_command.h"
#include "retreat_command.h"
#include "save_command.h"
#include "help_command.h"
#include "crt_command.h"
#include "status_command.h"
#include "hex_command.h"
#include "galaxy_command.h"
#include "statemachine.h"
#include "telemetry.h"
// #include "game.h"
#include "db.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);

// BUGBUG
// Global builder for accumulating build set attributes
BuildSetAttributeCommand::Builder* g_build_set_builder = new BuildSetAttributeCommand::Builder();
// Global builders for combat commands (accumulate power/damage specs from sub-rules)
CombatOrderCommand::Builder* g_combat_order_builder = new CombatOrderCommand::Builder();
CombatApplyCommand::Builder* g_combat_apply_builder = new CombatApplyCommand::Builder();

// Global builder for repair command
RepairCommand::Builder* g_repair_builder = new RepairCommand::Builder();

%}

%code requires {
#include <string>
#include <vector>
}

%define parse.error verbose

%union {
   int ival;
   std::string* sval;
   std::vector<std::string>* vec_sval;
}

%token <ival> TOK_INT
%token <sval> TOK_STRING
%type  <sval> deployable_ship
%type  <sval> building_draft_ship
%type  <vec_sval> chain_move_location
%type  <vec_sval> chain_resource_words
%type  <sval> deconflicted_string

%token TOK_APPLY
%token TOK_ATTACK
%token TOK_BEAM
%token TOK_BUILD
%token TOK_BUILD_COMMIT
%token TOK_BUILD_NEW
%token TOK_BUILD_DRAFTS
%token TOK_BUILD_CANCEL
%token TOK_BUILD_SET_ATTR

%token TOK_CANCEL

%token TOK_COMBAT
%token TOK_COMBAT_DRAFTS
%token TOK_COMBAT_ORDER
%token TOK_COMBAT_APPLY
%token TOK_COMBAT_COMMIT
%token TOK_COMBAT_CANCEL

%token TOK_COMMIT
%token TOK_CRT
%token TOK_DELETE
%token TOK_DEMO
%token TOK_DEPLOY
%token TOK_DODGE
%token TOK_DONE
%token TOK_DRAFTS
%token TOK_DROP
%token TOK_ESCAPE
%token TOK_FLEET
%token TOK_GALAXY
%token TOK_CARGO
%token TOK_HELP
%token TOK_HEX
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
%token TOK_RESUPPLY
%token TOK_RETREAT
%token TOK_SAVE
%token TOK_ACCEPT
%token TOK_REJECT
%token TOK_SCREEN
%token TOK_SCORE
%token TOK_EXTRACT
%token TOK_MARKET
%token TOK_TRADE
%token TOK_FABRICATE
%token TOK_OUTFIT
%token TOK_SCAN
%token TOK_BUY
%token TOK_SELL
%token TOK_TRANSFER
%token TOK_SALVAGE
%token TOK_SET_ATTR
%token TOK_STATS
%token TOK_STATUS
%token TOK_SYSTEM
%token TOK_SURVEY
%token TOK_TUBE
%token TOK_CLEAR

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
   TOK_SAVE
   {
        ICmd *pCmd = SaveCommand::Builder().set_show_usage().build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_SAVE deconflicted_string
   {
        std::string save_name(*$2);
        ICmd *pCmd = SaveCommand::Builder().set_name(save_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_LOAD
   {
        ICmd *pCmd = LoadCommand::Builder().set_list_saves().build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_LOAD deconflicted_string
   {
        std::string load_name(*$2);
        ICmd *pCmd = LoadCommand::Builder().set_name(load_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_ACCEPT deconflicted_string
   {
        std::string accept_name(*$2);
        ICmd *pCmd = AcceptCommand::Builder().set_name(accept_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_REJECT deconflicted_string
   {
        std::string reject_name(*$2);
        ICmd *pCmd = RejectCommand::Builder().set_name(reject_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   | TOK_DELETE deconflicted_string {
        // MISSING HANDLER
        // delete the named game (the same naming convention used for saving,
        // loading)
        std::string target_game(*$2);
        Logger::instance().info("Delete the game saved under " ">" + target_game + "<");
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
        Logger::instance().info("Quit the game. Same as Reset.");
   }
   |
   TOK_CLEAR
   {
        // Bug #6: Clear command sends special marker that client will detect
        // Using ANSI escape sequence ESC[2J which means "clear entire screen"
        Telemetry::getInstance().write("\033[2J");
   }
   ;

// Information
// drafts is unique -- contextual (for combat, building, applying damage) 
// list is unique -- contextual (for combat, building)

info_cmd:
   TOK_STATUS {
      ICmd* pCmd = StatusCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
   }
   | TOK_CRT {
      ICmd* pCmd = CrtCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
   }
   ;

looking_cmd:
  TOK_HEX deconflicted_string {
      std::string identifier(*$2);
      ICmd* pCmd = HexCommand::Builder().setLocation(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      delete $2;
  }
  | TOK_ONLINE {
      Logger::instance().info("Who's online and when?");
  }
  | TOK_STATS {
      Logger::instance().info("Statistics about game, fleet, opponent");
  }
  | TOK_FLEET {
      ICmd* pCmd = FleetCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM deconflicted_string {
      std::string identifier(*$2);
      ICmd* pCmd = SystemCommand::Builder().setName(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM deconflicted_string deconflicted_string {
      std::string identifier(*$2);
      std::string subcmd(*$3);
      ICmd* pCmd = SystemCommand::Builder().setName(identifier).setSubcommand(subcmd).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SURVEY {
      ICmd* pCmd = SurveyCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SURVEY deconflicted_string {
      std::string identifier(*$2);
      ICmd* pCmd = SurveyCommand::Builder().setSystem(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SCORE {
      ICmd* pCmd = ScoreCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_EXTRACT TOK_SCAN {
      ICmd* pCmd = ExtractCommand::Builder().scanMode().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_EXTRACT deconflicted_string deconflicted_string chain_resource_words {
      std::string ship(*$2);
      // Join words with underscore: "rare" + "earth" -> "RARE_EARTH"
      std::string res = *$3;
      for (auto& c : res) c = std::toupper(c);
      for (const auto& word : *$4) {
          std::string upper = word;
          for (auto& c : upper) c = std::toupper(c);
          res += "_" + upper;
      }
      ICmd* pCmd = ExtractCommand::Builder().ship(ship).resource(res).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      SafeDelete($2); SafeDelete($3); SafeDelete($4);
  }
  | TOK_MARKET {
      ICmd* pCmd = MarketCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_MARKET deconflicted_string {
      std::string res(*$2);
      ICmd* pCmd = MarketCommand::Builder().resource(res).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_LIST {
      ICmd* pCmd = TradeCommand::Builder().listMode().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_BUY deconflicted_string TOK_INT {
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeCommand::Builder().buyMode().resource(res).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_SELL deconflicted_string TOK_INT {
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeCommand::Builder().sellMode().resource(res).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_TRANSFER deconflicted_string deconflicted_string deconflicted_string TOK_INT {
      std::string ship1(*$3);
      std::string ship2(*$4);
      std::string res(*$5);
      int qty = $6;
      ICmd* pCmd = TradeCommand::Builder().transferMode().from_ship(ship1).to_ship(ship2).resource(res).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE TOK_LIST {
      ICmd* pCmd = FabricateCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE deconflicted_string {
      std::string rec(*$2);
      ICmd* pCmd = FabricateCommand::Builder().recipe(rec).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE deconflicted_string TOK_INT {
      std::string rec(*$2);
      int qty = $3;
      ICmd* pCmd = FabricateCommand::Builder().recipe(rec).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT {
      ICmd* pCmd = OutfitCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT TOK_LIST {
      ICmd* pCmd = OutfitCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT deconflicted_string deconflicted_string {
      std::string ship(*$2);
      std::string equip(*$3);
      ICmd* pCmd = OutfitCommand::Builder().ship(ship).equipment(equip).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE TOK_SCAN {
      ICmd* pCmd = SalvageCommand::Builder().scan().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE deconflicted_string {
      std::string ship(*$2);
      ICmd* pCmd = SalvageCommand::Builder().ship(ship).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE deconflicted_string TOK_STRING {
      std::string ship(*$2);
      std::string target(*$3);
      ICmd* pCmd = SalvageCommand::Builder().ship(ship).target(target).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE deconflicted_string deconflicted_string {
      std::string ship(*$2);
      std::string target(*$3);
      ICmd* pCmd = SalvageCommand::Builder().ship(ship).target(target).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_GALAXY {
      ICmd* pCmd = GalaxyCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_CARGO {
      // Show cargo help
      Telemetry::getInstance().write("Usage: cargo <ship_code>");
  }
  | TOK_CARGO deconflicted_string {
      std::string ship(*$2);
      ICmd* pCmd = CargoCommand::Builder().ship(ship).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      SafeDelete($2);
  }
  ;

turn_cmd:
  TOK_NEXT {
      Logger::instance().info("Advance active player to next phase");
      ICmd* pCmd = NextCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_DONE {
      Logger::instance().info("Advance active player to first phase "
                      "of opponent, if possible");
      ICmd* pCmd = DoneCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  ;

combat_cmd:
   // combat
   // c
   TOK_COMBAT {
      Logger::instance().info("Status of combat situation");
   }

   // combat drafts
   | TOK_COMBAT TOK_DRAFTS {
       ICmd* pCmd = CombatDraftsCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   //  cd
   | TOK_COMBAT_DRAFTS {
       ICmd* pCmd = CombatDraftsCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }


   // combat commit
   | TOK_COMBAT TOK_COMMIT {
       ICmd* pCmd = CombatCommitCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }

   // cc
   | TOK_COMBAT_COMMIT {
       ICmd* pCmd = CombatCommitCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }

   // combat cancel
   | TOK_COMBAT TOK_CANCEL {
       ICmd* pCmd = CombatCancelCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }

   // cx
   | TOK_COMBAT_CANCEL {
       ICmd* pCmd = CombatCancelCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }

   // combat order ...
   | TOK_COMBAT TOK_ORDER combat_initiator_ship combat_tactic combat_target_ship combat_order_spec {
       // Build and invoke combat order command
       ICmd* pCmd = g_combat_order_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_order_builder;
       g_combat_order_builder = new CombatOrderCommand::Builder();
   }

   // co
   | TOK_COMBAT_ORDER combat_initiator_ship combat_tactic combat_target_ship combat_order_spec {
       // Build and invoke combat order command
       ICmd* pCmd = g_combat_order_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_order_builder;
       g_combat_order_builder = new CombatOrderCommand::Builder();
   }

   // combat apply ...
   | TOK_COMBAT TOK_APPLY combat_damaged_ship combat_application_spec {
       // Build and invoke combat apply command
       ICmd* pCmd = g_combat_apply_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_apply_builder;
       g_combat_apply_builder = new CombatApplyCommand::Builder();
   }

   // ca ...
   | TOK_COMBAT_APPLY combat_damaged_ship combat_application_spec {
       // Build and invoke combat apply command
       ICmd* pCmd = g_combat_apply_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_apply_builder;
       g_combat_apply_builder = new CombatApplyCommand::Builder();
   }
  ;


building_draft_ship:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Drafting ship: "
                              ">" + ship_id + "<" );
      $$ = $1;  // Pass the string up
  }
  ;

combat_initiator_ship:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Combat ship initiator: >" + ship_id + "<");
      g_combat_order_builder->ship_code(ship_id);
      delete $1;
  }
  ;

combat_damaged_ship:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Combat damaged ship: >" + ship_id + "<");
      g_combat_apply_builder->ship_code(ship_id);
      delete $1;
  }
  ;

combat_tactic:
  TOK_ATTACK {
      Logger::instance().info("Combat Attack Tactic");
      g_combat_order_builder->tactic('A');
  }
  | TOK_DODGE {
      Logger::instance().info("Combat Dodge Tactic");
      g_combat_order_builder->tactic('D');
  }
  | TOK_ESCAPE {
      Logger::instance().info("Combat Escape Tactic");
      g_combat_order_builder->tactic('R');
  }
  ;

combat_target_ship:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Combat target ship: >" + ship_id + "<");
      g_combat_order_builder->target(ship_id);
      delete $1;
  }
  ;

combat_order_spec:
  TOK_PD_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->drive_power($1);
  }
  | TOK_B_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->beam_power($1);
  }
  | TOK_S_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->screen_power($1);
  }
  | TOK_T_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->tube_power($1);
  }
  | TOK_M_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->missiles(std::to_string($1));
  }
  ;

additional_combat_order_spec:
  /* empty - no more specs */
  | TOK_PD_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->drive_power($1);
  }
  | TOK_B_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->beam_power($1);
  }
  | TOK_S_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->screen_power($1);
  }
  | TOK_T_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->tube_power($1);
  }
  | TOK_M_ASSIGN additional_combat_order_spec {
      g_combat_order_builder->missiles(std::to_string($1));
  }
  ;

combat_application_spec:
  TOK_PD_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("D", $1);
  }
  | TOK_B_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("B", $1);
  }
  | TOK_S_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("S", $1);
  }
  | TOK_T_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("T", $1);
  }
  | TOK_M_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("M", $1);
  }
  ;

additional_combat_application_spec:
  /* empty - no more specs */
  | TOK_PD_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("D", $1);
  }
  | TOK_B_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("B", $1);
  }
  | TOK_S_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("S", $1);
  }
  | TOK_T_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("T", $1);
  }
  | TOK_M_ASSIGN additional_combat_application_spec {
      g_combat_apply_builder->assign("M", $1);
  }
  ;

build_cmd:
  // build
  // b
  TOK_BUILD {
      Logger::instance().info("Without arguments, shows "
                              "current build state");
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // build new ...
  | TOK_BUILD TOK_NEW deconflicted_string deconflicted_string {
      std::string code(*$3);
      std::string name(*$4);
      Logger::instance().info("Create new draft: " + code + " '" + name + "'");
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // bn ...
  | TOK_BUILD_NEW deconflicted_string deconflicted_string {
      std::string code(*$2);
      std::string name(*$3);
      Logger::instance().info("Create new draft: " + code + " '" + name + "'");
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // build drafts
  | TOK_BUILD TOK_DRAFTS {
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bd
  | TOK_BUILD_DRAFTS {
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // build drafts ...
  | TOK_BUILD TOK_DRAFTS building_draft_ship {
      std::string ship_code = *$3;
      Logger::instance().info("Show draft details: " + ship_code);
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_code(ship_code)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bd ...
  | TOK_BUILD_DRAFTS building_draft_ship {
      std::string ship_code = *$2;
      Logger::instance().info("Show draft details: " + ship_code);
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_code(ship_code)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }


  // build set ...
  | TOK_BUILD TOK_SET_ATTR build_attr_spec {
      Logger::instance().info("Set attributes on current draft");
      ICmd *pCmd = g_build_set_builder->build();
      pCmd->invoke();
      SafeDelete(pCmd);
      delete g_build_set_builder;
      g_build_set_builder = new BuildSetAttributeCommand::Builder();
  }

  // bs ...
  | TOK_BUILD_SET_ATTR build_attr_spec {
      Logger::instance().info("Set attributes on current draft");
      ICmd *pCmd = g_build_set_builder->build();
      pCmd->invoke();
      SafeDelete(pCmd);
      delete g_build_set_builder;
      g_build_set_builder = new BuildSetAttributeCommand::Builder();
  }

  // build commit
  | TOK_BUILD TOK_COMMIT {
      Logger::instance().info("Build commit - committing current draft");
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bc
  | TOK_BUILD_COMMIT {
      Logger::instance().info("Build commit - committing current draft");
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  | TOK_BUILD TOK_CANCEL {
      Logger::instance().info("Build cancel - canceling current draft");
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bx
  | TOK_BUILD_CANCEL {
      Logger::instance().info("Build cancel - canceling current draft");
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  ;


build_attr_spec:
  TOK_PD_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_pd($1);
  }
  | TOK_B_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_b($1);
  }
  | TOK_S_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_s($1);
  }
  | TOK_T_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_t($1);
  }
  | TOK_M_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_m($1);
  }
  | TOK_SR_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_sr($1);
  }
  ;

additional_build_attr_spec:
  // or no more spec
  | TOK_PD_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_pd($1);
  }
  | TOK_B_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_b($1);
  }
  | TOK_S_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_s($1);
  }
  | TOK_T_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_t($1);
  }
  | TOK_M_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_m($1);
  }
  | TOK_SR_ASSIGN additional_build_attr_spec {
      g_build_set_builder->set_sr($1);
  }
  ;

// deployment
// "deploy" { return TOK_DEPLOY; }
deployable_ship:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Deployable ship: "
                              ">" + ship_id + "<" );
      $$ = $1;
  }
  ;

deploy_cmd:
   // deploy - list undeployed ships
   TOK_DEPLOY {
       GameState s = StateMachine::getInstance().get_game_state();
       char owner = StateMachine::getInstance().get_current_player();
       DatabaseManager& db = DatabaseManager::getInstance();

       auto rows = db.query(
           "SELECT ship_code, ship_name FROM ships WHERE game_id=" +
           std::to_string(s.game_id) + " AND owner='" + std::string(1, owner) +
           "' AND destroyed_at IS NULL AND (at_hex IS NULL OR at_hex = '') "
           "AND (at_system IS NULL OR at_system = '') ORDER BY ship_code");

       if (rows.empty())
       {
           Telemetry::getInstance().write(
               "DEPLOY: All vessels are deployed. None in spacedock.");
       }
       else
       {
           std::ostringstream out;
           out << "DEPLOY: Ships in spacedock awaiting deployment:\n";
           for (const auto& r : rows)
           {
               out << "  " << r[0] << " - " << r[1] << "\n";
           }
           out << "Use: deploy <SHIP> <BASE_SYSTEM>";
           Telemetry::getInstance().write(out.str());
       }
   }
   | TOK_DEPLOY deployable_ship deconflicted_string {
       std::string ship(*$2);
       std::string destination(*$3);
       Logger::instance().info("Attempt to deploy ship "
                               ">" + ship + "<" 
                               " at destination: "
                               ">" + destination + "<");

       ICmd* pCmd = DeployCommand::Builder()
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
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Target SystemShip: "
                              ">" + ship_id + "<");
  }
  ;

// Movement
chain_move_location:
   // empty - no more waypoints
   {
       $$ = new std::vector<std::string>;
   }
   | deconflicted_string chain_move_location {
       std::string waypoint(*$1);
       Logger::instance().info("Additional waypoint on chain: >"
                                + waypoint + "<");
       $$ = $2;
       $$->insert($$->begin(), waypoint);
       SafeDelete($1);
   }
   ;

// Resource type chaining (for multi-word resources like "rare earth")
chain_resource_words:
   /* empty - no more words */
   {
       $$ = new std::vector<std::string>;
   }
   | deconflicted_string chain_resource_words {
       $$ = $2;
       $$->insert($$->begin(), *$1);
       SafeDelete($1);
   }
   ;

move_cmd:
  // move
  // m
  TOK_MOVE {
       Logger::instance().info("Show move status");
  }

  // move
  // m
  | TOK_MOVE deconflicted_string deconflicted_string chain_move_location {
       std::string ship(*$2);
       std::string first_dest(*$3);
       std::vector<std::string>* waypoints = $4;
       
       Logger::instance().info("Moving ship >" 
                               + ship 
                               + "< to >" 
                               + first_dest + "<");
       
       // Build move command with all destinations
       ICmd* pCmd = MoveCommand::Builder()
            .ship_code(ship)
            .add_destination(first_dest)
            .add_waypoints(waypoints)
            .build();
       
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
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Pick destination warpship: "
                              ">" + ship_id + "<");
  }
  ;
warpship_drop_source:
  deconflicted_string {
      std::string ship_id(*$1);
      Logger::instance().info("Drop source warpship: "
                              ">" + ship_id + "<");
  }
  ;
pickdrop_cmd:
  // pick
  // p
  TOK_PICK deconflicted_string {
     std::string location(*$2);
     Logger::instance().info("Status of what can be picked up at hex "
                 ">" + location + "<");
  }
  // syntax: drop WARPSHIP_ID
  // pick ...
  // p ...
  | TOK_DROP warpship_drop_source {
     Logger::instance().info("Only list what may be dropped from "
                             "target WarpShip");
  }
  // syntax: pick SOURCE DESTINATION */
  // pick ...
  // p ...
  | TOK_PICK target_systemship warpship_pick_destination {
     Logger::instance().info("Picking up SystemShip to rack "
                             "in target WarpShip");

  }
  // drop ...
  // d ...
  | TOK_DROP target_systemship warpship_drop_source {
     Logger::instance().info("Dropping target SystemShipship from "
                             "target WarpShip");
  }
  ;


rep_cmd:
  // repair
  // rp
  TOK_REPAIR {
     ICmd* pCmd = RepairCommand::Builder().build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
  // repair ...
  // rp ...
  | TOK_REPAIR deconflicted_string repair_attr_spec {
     // g_repair_builder populated by repair_attr_spec
     g_repair_builder->set_ship_code(*$2);
     ICmd* pCmd = g_repair_builder->build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
     delete g_repair_builder;
     g_repair_builder = new RepairCommand::Builder();
  }
  // resupply
  // rs
  | TOK_RESUPPLY {
     ICmd* pCmd = ResupplyCommand::Builder().build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
  // resupply ...
  // rs ...
  | TOK_RESUPPLY deconflicted_string TOK_INT {
     std::string ship(*$2);
     int qty = (int) $3;
     ICmd* pCmd = ResupplyCommand::Builder().set_ship_code(ship).set_missiles(qty).build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
   }
   // retreat <ship> <hex>
   // rt <ship> <hex>
   | TOK_RETREAT deconflicted_string deconflicted_string {
      std::string ship(*$2);
      std::string hex(*$3);
      ICmd* pCmd = new RetreatCommand(ship, hex);
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      delete $2;
      delete $3;
   }
  ;

repair_attr_spec:
  TOK_PD_ASSIGN {
      g_repair_builder->set_attribute("pd");
      g_repair_builder->set_amount($1);
  }
  | TOK_B_ASSIGN {
      g_repair_builder->set_attribute("b");
      g_repair_builder->set_amount($1);
  }
  | TOK_S_ASSIGN {
      g_repair_builder->set_attribute("s");
      g_repair_builder->set_amount($1);
  }
  | TOK_T_ASSIGN {
      g_repair_builder->set_attribute("t");
      g_repair_builder->set_amount($1);
  }
  ; 
 
help_cmd:
  TOK_DEMO {
       ICmd *pCmd = HelpCommand::Builder().set_demo().build();
       pCmd->invoke();
       SafeDelete(pCmd);
  }
  |
  TOK_HELP deconflicted_string {
       std::string topic(*$2);
       ICmd *pCmd = HelpCommand::Builder().set_topic(topic).build();
       pCmd->invoke();
       SafeDelete(pCmd);
       delete $2;
  }
  |
  TOK_HELP {
       ICmd *pCmd = HelpCommand::Builder().build();
       pCmd->invoke();
       SafeDelete(pCmd);
  }
  ;

/* Help topic can be a string OR any keyword that might be a topic name */
deconflicted_string:
    TOK_STRING              { $$ = $1; }
  | TOK_BUILD_COMMIT        { $$ = new std::string("bc"); }
  | TOK_BUILD_DRAFTS        { $$ = new std::string("bd"); }
  | TOK_BUILD_NEW           { $$ = new std::string("bn"); }
  | TOK_BUILD_CANCEL        { $$ = new std::string("bx"); }
  | TOK_BUILD_SET_ATTR      { $$ = new std::string("bs"); }
  | TOK_BUILD               { $$ = new std::string("build"); }
  | TOK_COMBAT              { $$ = new std::string("combat"); }
  | TOK_COMBAT_DRAFTS       { $$ = new std::string("cd"); }
  | TOK_COMBAT_ORDER        { $$ = new std::string("co"); }
  | TOK_COMBAT_APPLY        { $$ = new std::string("ca"); }
  | TOK_COMBAT_COMMIT       { $$ = new std::string("cc"); }
  | TOK_COMBAT_CANCEL       { $$ = new std::string("cx"); }
  | TOK_MOVE                { $$ = new std::string("move"); }
  | TOK_NEXT                { $$ = new std::string("next"); }
  | TOK_DONE                { $$ = new std::string("done"); }
  | TOK_DEPLOY              { $$ = new std::string("deploy"); }
  | TOK_PICK                { $$ = new std::string("pick"); }
  | TOK_DROP                { $$ = new std::string("drop"); }
  | TOK_REPAIR              { $$ = new std::string("repair"); }
  | TOK_RESUPPLY            { $$ = new std::string("resupply"); }
  | TOK_SCORE               { $$ = new std::string("score"); }
  | TOK_FLEET               { $$ = new std::string("fleet"); }
  | TOK_SYSTEM              { $$ = new std::string("system"); }
  | TOK_SURVEY              { $$ = new std::string("survey"); }
  | TOK_STATUS              { $$ = new std::string("status"); }
  | TOK_DRAFTS              { $$ = new std::string("drafts"); }
  | TOK_CRT                 { $$ = new std::string("crt"); }
  | TOK_CLEAR               { $$ = new std::string("clear"); }
  | TOK_EXTRACT             { $$ = new std::string("extract"); }
  | TOK_MARKET              { $$ = new std::string("market"); }
  | TOK_TRADE               { $$ = new std::string("trade"); }
  | TOK_FABRICATE           { $$ = new std::string("fabricate"); }
  | TOK_SALVAGE             { $$ = new std::string("salvage"); }
  | TOK_ATTACK              { $$ = new std::string("attack"); }
  | TOK_DODGE               { $$ = new std::string("dodge"); }
  | TOK_ESCAPE              { $$ = new std::string("escape"); }
  | TOK_HEX                 { $$ = new std::string("hex"); }
  | TOK_SAVE                { $$ = new std::string("save"); }
  | TOK_LOAD                { $$ = new std::string("load"); }
  | TOK_ACCEPT              { $$ = new std::string("accept"); }
  | TOK_REJECT              { $$ = new std::string("reject"); }
  | TOK_DELETE              { $$ = new std::string("delete"); }
  | TOK_DEMO                { $$ = new std::string("demo"); }
  | TOK_HELP                { $$ = new std::string("help"); }
  ;

%%
void yyerror(const char* s)
{
   Logger::instance().error("Parse error: " + std::string(s));
}
