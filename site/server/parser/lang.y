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
#include "attributemap.h"
#include "Build.h"
#include "logger.h"
#include "next_command.h"
#include "done_command.h"
#include "deploy_command.h"
#include "move_command.h"

#include "combat_order_actor.h"
#include "combat_apply_actor.h"
#include "combat_drafts_actor.h"
#include "combat_commit_actor.h"
#include "combat_cancel_actor.h"

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
#include "delete_command.h"
#include "quit_command.h"
#include "statemachine.h"
#include "telemetry.h"
// #include "game.h"
#include "db.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

// Forward decl so we can declare yyerror before bison defines YYLTYPE.
typedef struct YYLTYPE YYLTYPE;
extern YYLTYPE yylloc;

// Bison will call yyerror(const char*) for its own diagnostics.
void yyerror(const char* msg);
// Our richer helper (used by actions) that can report source location.
void yyerror(YYLTYPE* loc, const char* msg);

// BUGBUG
//JDW  CombatApplyCommand::Builder* g_combat_apply_builder = new CombatApplyCommand::Builder();

// Global builder for repair command
RepairCommand::Builder* g_repair_builder = new RepairCommand::Builder();

%}

%define parse.error verbose
%locations

%code requires {
#include <string>
#include <vector>
#include "typedefs.h"
}


%union {
   int ival;
   std::string* sval;
   std::vector<std::string>* vec_sval;
   AttributeMap* vec_attr;
   std::pair<AttributeMap*, MissileSet*>* combat_pair;
}

%type <vec_attr> build_attr_spec
%type <combat_pair> new_combat_order_attr_spec
%type <sval> combat_initiator_ship
%type <sval> deployable_ship
%type <sval> building_draft_ship
%type <vec_sval> chain_move_location
%type <vec_sval> chain_resource_words
%type <sval> build_optional_target
%type <ival> combat_tactic;
%type <sval> combat_target_ship

%token <ival> TOK_INT
%token <sval> TOK_STRING
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

%start input

%%

input:
    command
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
   TOK_SAVE TOK_STRING
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
   TOK_LOAD TOK_STRING
   {
        std::string load_name(*$2);
        ICmd *pCmd = LoadCommand::Builder().set_name(load_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_ACCEPT TOK_STRING
   {
        std::string accept_name(*$2);
        ICmd *pCmd = AcceptCommand::Builder().set_name(accept_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_REJECT TOK_STRING
   {
        std::string reject_name(*$2);
        ICmd *pCmd = RejectCommand::Builder().set_name(reject_name).build();
        pCmd->invoke();
        SafeDelete(pCmd);
        delete $2;
   }
   | TOK_DELETE TOK_STRING {
        std::string target_game(*$2);
        ICmd* pCmd = DeleteCommand::Builder().setSaveName(target_game).build();
        if (pCmd && pCmd->invoke()) { /* success */ }
        SafeDelete(pCmd);
        delete $2;
   }
   |
   TOK_QUIT
   {
        ICmd* pCmd = QuitCommand::Builder().build();
        if (pCmd && pCmd->invoke()) { /* success */ }
        SafeDelete(pCmd);
   }
   |
   TOK_CLEAR
   {
        // Bug #6: Clear command sends special marker that client will detect
        // Using ANSI escape sequence ESC[2J which means "clear entire screen"
        Telemetry::getInstance().write("\033[2J");
   }
   
| TOK_SAVE error { yyerror(&@1, "save: usage: save <name>. Note: reserved words cannot be used as names."); YYABORT; }
| TOK_LOAD error { yyerror(&@1, "load: usage: load <name>."); YYABORT; }
| TOK_ACCEPT error { yyerror(&@1, "accept: usage: accept <name>."); YYABORT; }
| TOK_REJECT error { yyerror(&@1, "reject: usage: reject <name>."); YYABORT; }
| TOK_DELETE error { yyerror(&@1, "delete: usage: delete <name>."); YYABORT; }
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
  TOK_HEX TOK_STRING {
      std::string identifier(*$2);
      ICmd* pCmd = HexCommand::Builder().setLocation(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      delete $2;
  }
  | TOK_FLEET {
      ICmd* pCmd = FleetCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM TOK_STRING {
      std::string identifier(*$2);
      ICmd* pCmd = SystemCommand::Builder().setName(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM TOK_STRING TOK_STRING {
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
  | TOK_SURVEY TOK_STRING {
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
  | TOK_EXTRACT TOK_STRING TOK_STRING chain_resource_words {
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
  | TOK_MARKET TOK_STRING {
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
  | TOK_TRADE TOK_BUY TOK_STRING TOK_INT {
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeCommand::Builder().buyMode().resource(res).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_SELL TOK_STRING TOK_INT {
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeCommand::Builder().sellMode().resource(res).quantity(qty).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_TRANSFER TOK_STRING TOK_STRING TOK_STRING TOK_INT {
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
  | TOK_FABRICATE TOK_STRING {
      std::string rec(*$2);
      ICmd* pCmd = FabricateCommand::Builder().recipe(rec).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE TOK_STRING TOK_INT {
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
  | TOK_OUTFIT TOK_STRING TOK_STRING {
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
  | TOK_SALVAGE TOK_STRING {
      std::string ship(*$2);
      ICmd* pCmd = SalvageCommand::Builder().ship(ship).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE TOK_STRING TOK_STRING {
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
  | TOK_CARGO TOK_STRING {
      std::string ship(*$2);
      ICmd* pCmd = CargoCommand::Builder().ship(ship).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      SafeDelete($2);
  }
  
| TOK_HEX error { yyerror(&@1, "hex: usage: hex <hex_id>"); YYABORT; }
| TOK_SYSTEM error { yyerror(&@1, "system: usage: system <name> [subcommand]"); YYABORT; }
| TOK_SURVEY error { yyerror(&@1, "survey: usage: survey [system_name]"); YYABORT; }
| TOK_EXTRACT error { yyerror(&@1, "extract: usage: extract scan | extract <ship> <resource> [resource_words...]"); YYABORT; }
| TOK_MARKET error { yyerror(&@1, "market: usage: market [system_name]"); YYABORT; }
| TOK_TRADE error { yyerror(&@1, "trade: usage: trade buy|sell|transfer ..."); YYABORT; }
| TOK_FABRICATE error { yyerror(&@1, "fabricate: usage: fabricate <ship> <item>"); YYABORT; }
| TOK_OUTFIT error { yyerror(&@1, "outfit: usage: outfit <ship> <spec>"); YYABORT; }
| TOK_SALVAGE error { yyerror(&@1, "salvage: usage: salvage <ship> <target>"); YYABORT; }
| TOK_GALAXY error { yyerror(&@1, "galaxy: usage: galaxy"); YYABORT; }
| TOK_CARGO error { yyerror(&@1, "cargo: usage: cargo <ship> [resource_words...]"); YYABORT; }
| TOK_SCAN error { yyerror(&@1, "scan: usage depends on command (e.g., extract scan)"); YYABORT; }
| TOK_LIST error { yyerror(&@1, "list: usage depends on context"); YYABORT; }
;

turn_cmd:
  TOK_NEXT {
      ICmd* pCmd = NextCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_DONE {
      ICmd* pCmd = DoneCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  
| TOK_NEXT error { yyerror(&@1, "next: usage: next"); YYABORT; }
| TOK_DONE error { yyerror(&@1, "done: usage: done"); YYABORT; }
;

combat_cmd:
   // combat
   // c
   TOK_COMBAT {
      // [LANG] Status of combat situation
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
   | TOK_COMBAT TOK_ORDER combat_initiator_ship combat_tactic combat_target_ship new_combat_order_attr_spec {
       // Build and invoke combat order command
       std::string* attacker_ship = $3;
       std::string* attackee_ship = $5;
       char tact = $4;
       std::pair<AttributeMap*, MissileSet*>* compair = $6;
       AttributeMap* patmap = compair->first;
       MissileSet* pmisset  = compair->second;

       ICmd* pCmd = CombatOrderActor::Builder()
                                .ship_code(*attacker_ship)
                                .target(*attackee_ship)
                                .tactic(tact)
                                .set_missiles(*pmisset)
                                .set_attributes(*patmap)
                                .build(); 
       if (pCmd && pCmd->invoke()) {
       }
       SafeDelete(patmap);
       SafeDelete(pmisset);
       SafeDelete(compair);
       SafeDelete(attacker_ship);
       SafeDelete(attackee_ship);
       SafeDelete(pCmd);
   }
   | TOK_COMBAT_ORDER combat_initiator_ship combat_tactic combat_target_ship new_combat_order_attr_spec {
       // Build and invoke combat order command
       std::string* attacker_ship = $2;
       std::string* attackee_ship = $4;
       char tact = $3;
       std::pair<AttributeMap*, MissileSet*>* compair = $5;
       AttributeMap* patmap = compair->first;
       MissileSet* pmisset  = compair->second;

       ICmd* pCmd = CombatOrderActor::Builder()
                                .ship_code(*attacker_ship)
                                .target(*attackee_ship)
                                .tactic(tact)
                                .set_missiles(*pmisset)
                                .set_attributes(*patmap)
                                .build(); 
       if (pCmd && pCmd->invoke()) {
       }
       SafeDelete(patmap);
       SafeDelete(pmisset);
       SafeDelete(compair);
       SafeDelete(attacker_ship);
       SafeDelete(attackee_ship);
       SafeDelete(pCmd);
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
   | TOK_COMBAT error { yyerror(&@1, "combat: usage: combat (see help combat)"); YYABORT; }
   | TOK_COMBAT_DRAFTS error { yyerror(&@1, "cd: usage: cd"); YYABORT; }
   | TOK_COMBAT_ORDER error { yyerror(&@1, "co: usage: co <attacker> <attack|dodge|escape> <target?> [PD=n B=n S=n T=n M=n SR=n]"); YYABORT; }
   | TOK_COMBAT_APPLY error { yyerror(&@1, "ca: usage: ca <draft_id> <damage_spec...>"); YYABORT; }
   | TOK_COMBAT_COMMIT error { yyerror(&@1, "cc: usage: cc <draft_id>"); YYABORT; }
   | TOK_COMBAT_CANCEL error { yyerror(&@1, "cx: usage: cx <draft_id>"); YYABORT; }
   ;

building_draft_ship:
  TOK_STRING {
      $$ = $1;  // Pass the string up
  }
  ;

combat_initiator_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;

combat_damaged_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      g_combat_apply_builder->ship_code(ship_id);
      delete $1;
  }
  ;

combat_tactic:
  TOK_ATTACK {
      $$ = 'A';
  }
  | TOK_DODGE {
      $$ = 'D';
  }
  | TOK_ESCAPE {
      $$ = 'R';
  }
  ;

combat_target_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;


// jdw
new_combat_order_attr_spec: {  
     $$ = new std::pair<AttributeMap*,  MissileSet*>;
     $$->first = new AttributeMap;
     $$->second = new MissileSet;
  }
  | new_combat_order_attr_spec TOK_PD_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::POWER_DRIVE, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate POWER_DRIVE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_B_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::BEAM, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate BEAM assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_S_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::SCREEN, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate SCREEN assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_T_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::TUBE, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate TUBE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_M_ASSIGN {
      $$ = $1;
      // Missile power per missile is inserted into the vector of MissileSet
      // The AttributeMap[MISSILE] value is left alone. We don't
      // use it for combat per se. 
      $$->second->push_back($2);
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
      ICmd *pCmd = BuildListDraftsCommand::Builder().build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // build new W|S NAME
  | TOK_BUILD TOK_NEW TOK_STRING TOK_STRING {
      std::string code(*$3);
      std::string name(*$4);
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // bn ...
  // bn W|S NAME
  | TOK_BUILD_NEW TOK_STRING TOK_STRING {
      std::string code(*$2);
      std::string name(*$3);
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  // build drafts
  | TOK_BUILD TOK_DRAFTS {
      ICmd *pCmd = BuildListDraftsCommand::Builder().build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bd
  | TOK_BUILD_DRAFTS {
      ICmd *pCmd = BuildListDraftsCommand::Builder().build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // build drafts SHIP_ID
  // build drafts SHIP_NAME
  | TOK_BUILD TOK_DRAFTS building_draft_ship {
      // can also be a name, but we will take it as it comes.
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_target(*$3)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bd SHIP_ID
  // bd SHIP_NAME
  | TOK_BUILD_DRAFTS building_draft_ship {
      // can also be a name, but we will take it as it comes.
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_target(*$2)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }


  // build set ...
  | TOK_BUILD build_optional_target TOK_SET_ATTR build_attr_spec {
      // $2 is the optional target
      //   can be ship_code or  ship name .. dealt with by invoke() later.
      // $3 is "SET" subcommand, we ignore that.
      // $4 is map of ship attributes

      ICmd *pCmd = BuildSetAttributeCommand::Builder()
                    .set_draft_target($2)
                    .set_attributes($4)
                    .build();

      pCmd->invoke(); 

      SafeDelete($4)
      SafeDelete($2)
      SafeDelete(pCmd);
  }

  // bs ...
  | TOK_BUILD_SET_ATTR build_optional_target build_attr_spec {
      // Using alias, already know it's the SET subcommand of BUILD.
      // $2 is the optional target
      //   can be ship_code or
      //   ship name 
      // $3 is vector of ship attributes
      ICmd *pCmd = BuildSetAttributeCommand::Builder()
                    .set_draft_target($2)
                    .set_attributes($3)
                    .build();

      pCmd->invoke();
      SafeDelete($3)
      SafeDelete($2)
      SafeDelete(pCmd);
  }

  // build commit
  // build commit CODE|NAME
  | TOK_BUILD TOK_COMMIT build_optional_target {
      // $3 is the code or name of a ship, or null
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .set_draft_target($3)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bc
  | TOK_BUILD_COMMIT build_optional_target {
      // $2 is the code or name of a ship, or null
      ICmd *pCmd = BuildCommitCommand::Builder()
                   .set_draft_target($2)
                   .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  | TOK_BUILD TOK_CANCEL build_optional_target {
      // $3 is the code or name of a ship, or null
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .set_draft_target($3)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  // bx
  | TOK_BUILD_CANCEL build_optional_target {
      // $2 is the code or name of a ship, or null
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .set_draft_target($2)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  
| TOK_BUILD error { yyerror(&@1, "build: usage: build new W|S name | build drafts | build commit [#] | build cancel [#] | bs [attr=#] ..."); YYABORT; }
| TOK_BUILD_NEW error { yyerror(&@1, "build new: usage: bn W|S name"); YYABORT; }
| TOK_BUILD_DRAFTS error { yyerror(&@1, "build drafts: usage: bd"); YYABORT; }
| TOK_BUILD_COMMIT error { yyerror(&@1, "build commit: usage: bc [#]"); YYABORT; }
| TOK_BUILD_CANCEL error { yyerror(&@1, "build cancel: usage: bx [#]"); YYABORT; }
| TOK_BUILD_SET_ATTR error { yyerror(&@1, "build set: usage: bs [attr=# [attr=#] ... ]"); YYABORT; }
;

build_optional_target:
  {
     $$ = 0;
     // nothing, or
  }
  | TOK_STRING
  {
     $$ = $1;
  }
  ;

build_attr_spec: {  
     $$ = new AttributeMap;
  }
  | build_attr_spec TOK_PD_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::POWER_DRIVE, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate POWER_DRIVE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | build_attr_spec TOK_B_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::BEAM, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate BEAM assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | build_attr_spec TOK_S_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::SCREEN, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate SCREEN assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | build_attr_spec TOK_T_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::TUBE, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate TUBE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | build_attr_spec TOK_M_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::MISSILE, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate MISSILE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | build_attr_spec TOK_SR_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::SYSTEM_RACK, $2});
      if (!inserted) {
        yyerror(&@2, "build set: duplicate SYSTEM_RACK assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  ;


// deployment
// "deploy" { return TOK_DEPLOY; }
deployable_ship:
  TOK_STRING {
      std::string ship_id(*$1);
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
   | TOK_DEPLOY deployable_ship TOK_STRING {
       std::string ship(*$2);
       std::string dest(*$3);
       ICmd* pCmd = DeployCommand::Builder()
                    .target(ship)
                    .system(dest)
                    .build();

       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       delete $2;
       delete $3;
   }
  
| TOK_DEPLOY error { yyerror(&@1, "deploy: usage: deploy <ship> <location>"); YYABORT; }
;

target_systemship:
  TOK_STRING {
      // BUGBUG 
      std::string ship_id(*$1);
  }
  ;

// Movement
chain_move_location:
   // empty - no more waypoints
   {
       $$ = new std::vector<std::string>;
   }
   | TOK_STRING chain_move_location {
       std::string waypoint(*$1);
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
   | TOK_STRING chain_resource_words {
       $$ = $2;
       $$->insert($$->begin(), *$1);
       SafeDelete($1);
   }
   ;

move_cmd:
  // move
  // m
  TOK_MOVE TOK_STRING TOK_STRING chain_move_location {
       std::string ship(*$2);
       std::string first_dest(*$3);
       std::vector<std::string>* waypoints = $4;
       
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
  
| TOK_MOVE error { yyerror(&@1, "move: usage: move <ship> <destination> [via ...]"); YYABORT; }
;


// Pickup, Drop
// "pick" { return TOK_PICK; }
// "drop" { return TOK_DROP; }
warpship_pick_destination:
  TOK_STRING {
      std::string ship_id(*$1);
  }
  ;
warpship_drop_source:
  TOK_STRING {
      std::string ship_id(*$1);
  }
  ;
pickdrop_cmd:
  // pick
  // p
  TOK_PICK TOK_STRING {
     std::string location(*$2);
  }
  // pick ...
  // p ...
  | TOK_DROP warpship_drop_source {
     // Only list what may be dropped from target WarpShip
  }
  // syntax: pick SOURCE DESTINATION */
  // pick ...
  // p ...
  | TOK_PICK target_systemship warpship_pick_destination {
     // Picking up SystemShip to rack in target WarpShip
  }
  // drop ...
  // d ...
  | TOK_DROP target_systemship warpship_drop_source {
     // Dropping target SystemShipship from target WarpShip
  }
  
| TOK_PICK error { yyerror(&@1, "pick: usage: pick <ship> <item>"); YYABORT; }
| TOK_DROP error { yyerror(&@1, "drop: usage: drop <ship> <item>"); YYABORT; }
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
  | TOK_REPAIR TOK_STRING repair_attr_spec {
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
  | TOK_RESUPPLY TOK_STRING TOK_INT {
     std::string ship(*$2);
     int qty = (int) $3;
     ICmd* pCmd = ResupplyCommand::Builder().set_ship_code(ship).set_missiles(qty).build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
   }
   // retreat <ship> <hex>
   // rt <ship> <hex>
   | TOK_RETREAT TOK_STRING TOK_STRING {
      std::string ship(*$2);
      std::string hex(*$3);
      ICmd* pCmd = new RetreatCommand(ship, hex);
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
      delete $2;
      delete $3;
   }
  
| TOK_REPAIR error { yyerror(&@1, "repair: usage: repair <ship> PD=n|B=n|S=n|T=n (one attribute)"); YYABORT; }
| TOK_RESUPPLY error { yyerror(&@1, "resupply: usage: resupply <ship> <amount>"); YYABORT; }
| TOK_RETREAT error { yyerror(&@1, "retreat: usage: retreat <ship> <destination>"); YYABORT; }
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
  TOK_HELP TOK_STRING {
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
  
| TOK_HELP error { yyerror(&@1, "help: usage: help [topic]."); YYABORT; }
;

%%

// for error handling:
typedef struct ParseErrorState {
    int error_count;
    char last_error_msg[2048]; // Or use dynamic allocation
    YYLTYPE last_error_loc;   // The location data structure
    std::vector<std::string> user_errors;
    std::string user_err_msg;

    void clear() {
         error_count = 0;
         bzero(last_error_msg, 2048);
    }
    ParseErrorState() {
         clear();
    }
 
} ParseErrorState;

static ParseErrorState error_state;

bool get_parser_error(std::string& err)
{
    static int err_count = 0;
    bool result = false;

    if ( error_state.error_count > err_count)
    {
       // build up the error string.
       error_state.user_err_msg.clear();

       while ( ! error_state.user_errors.empty() )
       {
          std::string tmp = error_state.user_errors.back();
          error_state.user_errors.pop_back();
          error_state.user_err_msg.append(tmp);
          result = true;
       }

       if (result)
       {
          err = std::string(error_state.user_err_msg);
          error_state.user_errors.clear();
       }
    }
   
    if (!result) 
    {
       // how did we get here?
       Logger::instance().info("[LANG][ERROR] Indeterminite error cause");
       err = std::string("Cannot detect error cause");
    }

    // bump the counter
    err_count = error_state.error_count;
    return result;
}

void yyerror(const char* msg)
{
    // If location tracking is enabled, yylloc is the best available hint.
    // yyerror(&yylloc, msg);
    if (msg) {
       fprintf(stderr, "yyerror: %s\n", msg);
    }
    else {
       fprintf(stderr, "yyerror: (null)\n");
    }
}

// Richer helper used by our grammar actions.
void yyerror(YYLTYPE* loc, const char* msg)
{
    //BUGBUG roll over
    error_state.error_count++;
    std::ostringstream oerr;
    bool hasloc = false;
    if (loc)
    {
        oerr << msg << "\n";
        error_state.user_errors.push_back(oerr.str());
        hasloc = true;
    }
    else
    {
        std::ostringstream oerr;
        oerr  << msg << "\n";
        error_state.user_errors.push_back(oerr.str());
    }
    std::string dbgstr = oerr.str();
    fprintf(stderr, "yyerror %s%s: %s\n",
            (const char*)((hasloc)?"(loc,":"("),
            (const char*)((!hasloc)?"(msg)":",msg)"),
            dbgstr.c_str());
}

