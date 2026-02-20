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
#include <format>
#include <string>
#include <vector>
#include <map>
#include "typedefs.h"
#include "logger.h"

#include "attributemap.h"

#include "license_command.h"

// Building ship actors
#include "build_commit_actor.h"
#include "build_new_actor.h"
#include "build_set_actor.h"
#include "build_commit_actor.h"
#include "build_cancel_actor.h"
#include "build_show_draft_actor.h"
#include "build_drafts_actor.h"
#include "fleet_actor.h"

// Combat actors
#include "combat_order_actor.h"
#include "combat_apply_actor.h"
#include "combat_drafts_actor.h"
#include "combat_commit_actor.h"
#include "combat_cancel_actor.h"

#include "next_command.h"
#include "done_command.h"
#include "deploy_command.h"
#include "move_command.h"
#include "chart_command.h"
#include "pick_drop_command.h"

#include "cargo_actor.h"
#include "system_actor.h"
#include "survey_actor.h"
#include "repair_command.h"
#include "resupply_command.h"
#include "recap_command.h"
#include "score_command.h"
#include "extract_actor.h"
#include "market_actor.h"
#include "trade_actor.h"
#include "fabricate_actor.h"
#include "outfit_actor.h"
#include "salvage_actor.h"
#include "retreat_command.h"
#include "save_command.h"
#include "help_command.h"
#include "crt_command.h"
#include "status_command.h"
#include "hex_command.h"
#include "delete_command.h"
#include "tuning_command.h"
#include "statemachine.h"
#include "telemetry.h"
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
   std::pair<AttributeMap*, TorpedoSet*>* combat_pair;
}

%type <ival> combat_tactic;
%type <sval> build_target
%type <sval> combat_initiator_ship
%type <sval> deployable_ship
%type <sval> building_draft_ship
%type <sval> combat_target_ship
%type <sval> combat_damaged_ship
%type <sval> ship_class
%type <vec_sval> chain_move_location
%type <vec_sval> chain_resource_words
%type <vec_attr> combat_apply_spec
%type <vec_attr> build_attr_spec_nonempty
%type <combat_pair> new_combat_order_attr_spec


%token <ival> TOK_INT
%token <sval> TOK_STRING

%token TOK_APPLY
%token TOK_ATTACK
%token TOK_PHASIC
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
%token TOK_DROP
%token TOK_ESCAPE
%token TOK_FLEET
%token TOK_CARGO
%token TOK_HELP
%token TOK_HEX
%token TOK_LICENSE
%token TOK_LIST
%token TOK_LOAD
%token TOK_TORPEDO
%token TOK_MOVE
%token TOK_CHART
%token TOK_NEW
%token TOK_NEXT
%token TOK_NEXT_SHORT
%token TOK_ORDER
%token TOK_PICK
%token TOK_POWER_DRIVE
%token TOK_REPAIR
%token TOK_RESUPPLY
%token TOK_RETREAT
%token TOK_SAVE
%token TOK_ACCEPT
%token TOK_REJECT
%token TOK_SHIELD
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
%token TOK_ANOMALIES
%token TOK_FACILITIES
%token TOK_RESOURCES
%token TOK_PLANETS
%token TOK_SURVEY
%token TOK_LAUNCHER
%token TOK_CLEAR
%token TOK_CONFIGURE
%token TOK_RELOAD
%token TOK_CONF
%token TOK_AI
%token TOK_RECAP
%token TOK_LRS

/* Attribute assignment tokens - value in yylval.ival */
%token <ival> TOK_PD_ASSIGN
%token <ival> TOK_P_ASSIGN
%token <ival> TOK_S_ASSIGN
%token <ival> TOK_L_ASSIGN
%token <ival> TOK_T_ASSIGN
%token <ival> TOK_H_ASSIGN

%token <sval> TOK_PD_ASSIGN_INVALID TOK_P_ASSIGN_INVALID TOK_S_ASSIGN_INVALID
%token <sval> TOK_L_ASSIGN_INVALID TOK_T_ASSIGN_INVALID TOK_H_ASSIGN_INVALID

%token TOK_UNKNOWN

%start input

%%

input:
    command
  ;

command:
    help_cmd
  | license_cmd
  | session_cmd
  | info_cmd
  | looking_cmd
  | turn_cmd
  | combat_cmd
  | build_cmd
  | deploy_cmd
  | move_cmd
  | chart_cmd
  | pickdrop_cmd
  | rep_cmd
  | configure_cmd
  ;


license_cmd:
   TOK_LICENSE
   {
       // BUGBUG OK
       ICmd* pCmd = LicenseCommand::Builder().build();
       pCmd->invoke();
       SafeDelete(pCmd);
   }
   ;
session_cmd:
   TOK_SAVE
   {
       // BUGBUG OK
       ICmd *pCmd = SaveCommand::Builder().set_show_usage().build();
       pCmd->invoke();
       SafeDelete(pCmd);
   }
   |
   TOK_SAVE TOK_STRING
   {
       // BUGBUG OK
       std::string save_name(*$2);
       ICmd *pCmd = SaveCommand::Builder().set_name(save_name).build();
       pCmd->invoke();
       SafeDelete(pCmd);
       SafeDelete($2);
   }
   |
   TOK_LOAD
   {
        // BUGBUG OK
        ICmd *pCmd = LoadCommand::Builder().set_list_saves().build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
   |
   TOK_LOAD TOK_STRING
   {
        // BUGBUG OK
        std::string load_name(*$2);
        ICmd *pCmd = LoadCommand::Builder().set_name(load_name).build();
        pCmd->invoke();
        SafeDelete($2);
        SafeDelete(pCmd);
   }
   |
   TOK_ACCEPT TOK_STRING
   {
        // BUGBUG OK
        std::string accept_name(*$2);
        ICmd *pCmd = AcceptCommand::Builder().set_name(accept_name).build();
        pCmd->invoke();
        SafeDelete($2);
        SafeDelete(pCmd);
   }
   |
   TOK_REJECT TOK_STRING
   {
        // BUGBUG OK
        std::string reject_name(*$2);
        ICmd *pCmd = RejectCommand::Builder().set_name(reject_name).build();
        pCmd->invoke();
        SafeDelete($2);
        SafeDelete(pCmd);
   }
   | TOK_DELETE TOK_STRING {
        // BUGBUG OK
        std::string target_game(*$2);
        ICmd* pCmd = DeleteCommand::Builder().setSaveName(target_game).build();
        if (pCmd && pCmd->invoke()) { /* success */ }
        SafeDelete($2);
        SafeDelete(pCmd);
   }
   |
   TOK_CLEAR
   {
        // BUGBUG OK
        // Using ANSI escape sequence ESC[2J which means "clear entire shield"
        Telemetry::instance().write("\033[2J");
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
      // Status is now an alias for score
      ICmd* pCmd = ScoreCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
   }
   | TOK_CRT {
      // BUGBUG OK
      ICmd* pCmd = CrtCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
   }
   | TOK_RECAP {
      // BUGBUG BUGBUG should be cleaned up
      ICmd* pCmd = RecapCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
   }
   ;

looking_cmd:
  TOK_FLEET TOK_STRING error {
      yyerror(&@3, "Too many arguments. Usage: fleet > HELP FLEET");
      SafeDelete($2);
  }
  | TOK_FLEET TOK_STRING {
      yyerror(&@2, "fleet takes no arguments. Usage: fleet > HELP FLEET");
      SafeDelete($2);
      YYERROR;
  }
  | TOK_FLEET {
      ICmd* pCmd = BuildFleetListActor::Builder().build();
      if (pCmd) {
          pCmd->invoke();
      }
      SafeDelete(pCmd);
  }
  | TOK_HEX TOK_STRING {
      // BUGBUG OK
      std::string identifier(*$2);
      ICmd* pCmd = HexCommand::Builder().setLocation(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }

  | TOK_SYSTEM TOK_STRING TOK_ANOMALIES {
      // BUGBUG - depends on the star-hex involved
      // if it's a base star-hex, we should already have full knowledge
      std::string identifier(*$2);
      ICmd* pCmd = SystemActor::Builder()
                      .set_system_name(identifier)
                      .set_anomalies_mode()
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }

  | TOK_SYSTEM TOK_STRING TOK_FACILITIES {
      // BUGBUG - depends on the star-hex involved
      // if it's a base star-hex, we should already have full knowledge
      std::string identifier(*$2);
      ICmd* pCmd = SystemActor::Builder()
                      .set_system_name(identifier)
                      .set_facilities_mode()
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM TOK_STRING TOK_RESOURCES {
      // BUGBUG - depends on the star-hex involved
      // if it's a base star-hex, we should already have full knowledge
      std::string identifier(*$2);
      ICmd* pCmd = SystemActor::Builder()
                      .set_system_name(identifier)
                      .set_resources_mode()
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM TOK_STRING TOK_PLANETS {
      // BUGBUG - depends on the star-hex involved
      // if it's a base star-hex, we should already have full knowledge
      std::string identifier(*$2);
      ICmd* pCmd = SystemActor::Builder()
                      .set_system_name(identifier)
                      .set_planets_mode()
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_SYSTEM TOK_STRING {
      // BUGBUG -- once a base-star hex is claimed
      // the rumors should include (for this game) that the player has
      // control over it.
      std::string identifier(*$2);
      ICmd* pCmd = SystemActor::Builder()
                     .set_system_name(identifier)
                     .set_show_mode()
                     .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }

  | TOK_SURVEY {
      // BUGBUG but there is some cumbersome ways to ladder-up knowledge
      ICmd* pCmd = SurveyActor::Builder()
                      .set_mode(SurveyMode::SURV_NONE)
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SURVEY TOK_STRING {
      // BUGBUG but there is some cumbersome ways to ladder-up knowledge
      std::string identifier(*$2);
      ICmd* pCmd = SurveyActor::Builder()
                     .set_system_name(identifier)
                     .set_mode(SurveyMode::SURV_BASIC)
                     .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_SCORE {
      // BUGBUG - should be part of STATUS
      ICmd* pCmd = ScoreCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_EXTRACT TOK_STRING TOK_STRING chain_resource_words {

      // BUGBUG OK, it works.
      std::string ship(*$2);

      // Join words with underscore: "rare" + "earth" -> "RARE_EARTH"
      std::string res = *$3;

      for (auto& c : res)
      {
          c = std::toupper(c);
      }

      for (const auto& word : *$4)
      {
          std::string upper = word;
          for (auto& c : upper)
          {
              c = std::toupper(c);
          }
          res += "_" + upper;
      }

      ICmd* pCmd = ExtractActor::Builder()
                                   .set_ship_code(ship)
                                   .set_extract_mode()
                                   .set_resource(res)
                                   .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete($3);
      SafeDelete($4);
      SafeDelete(pCmd);
  }
  | TOK_EXTRACT TOK_SCAN {
      // BUGBUG - but it doesn't cost anything??
      ICmd* pCmd = ExtractActor::Builder()
                                   .set_scan_mode()
                                   .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_MARKET TOK_STRING {
      // BADBUG:
      //   typed in "market rare earth" 
      // and got "too many arguments. Usage Fleet > help fleet"
      std::string res(*$2);
      ICmd* pCmd = MarketActor::Builder()
                                .set_history_mode()
                                .set_resource(res)
                                .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_MARKET {
      // BUGBUG - no primed data.
      ICmd* pCmd = MarketActor::Builder()
                                .set_list_mode()
                                .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_BUY TOK_STRING TOK_INT {
      // UNTESTED
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeActor::Builder()
                               .set_buy_mode()
                               .set_resource(res)
                               .set_qty(qty)
                               .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($3);
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_SELL TOK_STRING TOK_INT {
      // UNTESTED
      std::string res(*$3);
      int qty = $4;
      ICmd* pCmd = TradeActor::Builder()
                              .set_sell_mode()
                              .set_resource(res)
                              .set_qty(qty)
                              .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($3);
      SafeDelete(pCmd);
  }

  | TOK_TRADE TOK_TRANSFER TOK_STRING TOK_STRING TOK_STRING TOK_INT {
      // UNTESTED
      std::string ship1(*$3);
      std::string ship2(*$4);
      std::string res(*$5);
      int qty = $6;
      ICmd* pCmd = TradeActor::Builder()
                     .set_transfer_mode()
                     .set_from_ship(ship1)
                     .set_to_ship(ship2)
                     .set_resource(res)
                     .set_qty(qty)
                     .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($3);
      SafeDelete($4);
      SafeDelete($5);
      SafeDelete(pCmd);
  }
  | TOK_TRADE TOK_LIST {
      // BADBUG:  the trade list gives list of prices
      // on the exchange, but the market command yielded nothing.
      // That is inconsistent.
      ICmd* pCmd = TradeActor::Builder().set_list_mode().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }

  | TOK_FABRICATE TOK_STRING {
      // UNTESTED
      std::string fabricate_plan_requested(*$2);
      ICmd* pCmd = FabricateActor::Builder()
                     .set_plan(fabricate_plan_requested)
                     .set_qty(1)
                     .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE TOK_INT TOK_STRING {
      std::string fabricate_plan_requested(*$3);
      int qty = $2;
      ICmd* pCmd = FabricateActor::Builder()
                     .set_plan(fabricate_plan_requested)
                     .set_qty(qty)
                     .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($3);
      SafeDelete(pCmd);
  }
  | TOK_FABRICATE TOK_LIST {
      // BUGBUG the list is too tall.  Make it wider.
      ICmd* pCmd = FabricateActor::Builder()
                                   .show_plans()
                                   .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT TOK_STRING TOK_LRS {
      // UNTESTED
      std::string ship(*$2);
      ICmd* pCmd = OutfitActor::Builder()
                                .set_ship(ship)
                                .set_lrs()
                                .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT TOK_LIST {
      // UNTESTED
      ICmd* pCmd = OutfitActor::Builder()
                               .set_list()
                               .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_OUTFIT {
      // UNTESTED
      ICmd* pCmd = OutfitActor::Builder()
                                .set_list()
                                .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE TOK_SCAN {
      // BADBUG: "salvage w1 <system_name>" gave me "command executed"
      ICmd* pCmd = SalvageActor::Builder().set_scan_mode().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE TOK_STRING {
      // BADBUG: "salvage w1" gave me "command executed"
      std::string ship(*$2);
      ICmd* pCmd = SalvageActor::Builder()
                                .set_ship_code(ship)
                                .set_salvage_mode() 
                                .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_SALVAGE TOK_STRING TOK_STRING {
      // UNTESTED
      std::string ship(*$2);
      std::string resource(*$3);
      ICmd* pCmd = SalvageActor::Builder()
                             .set_ship_code(ship)
                             .set_salvage_mode()
                             .set_resource(resource)
                             .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($3);
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  | TOK_CARGO TOK_STRING {
      // BUGBUG OK
      std::string ship(*$2);
      ICmd* pCmd = CargoActor::Builder()
                      .ship(ship)
                      .build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete($2);
      SafeDelete(pCmd);
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
| TOK_CARGO error { yyerror(&@1, "cargo: usage: cargo <ship> [resource_words...]"); YYABORT; }
| TOK_SCAN error { yyerror(&@1, "scan: usage depends on command (e.g., extract scan)"); YYABORT; }
| TOK_LIST error { yyerror(&@1, "list: usage depends on context"); YYABORT; }
;

turn_cmd:
  TOK_NEXT {
      // BUGBUG OK
      ICmd* pCmd = NextCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  | TOK_DONE {
      // BUGBUG OK
      ICmd* pCmd = DoneCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
  
| TOK_NEXT error { yyerror(&@1, "next: usage: next"); YYABORT; }
| TOK_DONE error { yyerror(&@1, "done: usage: done"); YYABORT; }
;

combat_cmd:
   TOK_COMBAT TOK_ORDER combat_initiator_ship combat_tactic combat_target_ship new_combat_order_attr_spec {
       // BUGBUG OK
       // Build and invoke combat order command
       std::string* attacker_ship = $3;
       std::string* attackee_ship = $5;
       char tact = $4;
       std::pair<AttributeMap*, TorpedoSet*>* compair = $6;
       AttributeMap* patmap = compair->first;
       TorpedoSet* pmisset  = compair->second;

       ICmd* pCmd = CombatOrderActor::Builder()
                                .ship_code(*attacker_ship)
                                .target(*attackee_ship)
                                .tactic(tact)
                                .set_torpedoes(*pmisset)
                                .set_attributes(*patmap)
                                .build(); 
       if (pCmd)
       { 
          pCmd->invoke();
       }
       SafeDelete(patmap);
       SafeDelete(pmisset);
       SafeDelete(compair);
       SafeDelete(attacker_ship);
       SafeDelete(attackee_ship);
       SafeDelete(pCmd);
   }
   | TOK_COMBAT_ORDER combat_initiator_ship combat_tactic combat_target_ship new_combat_order_attr_spec {
       // BUGBUG OK
       // Build and invoke combat order command
       std::string* attacker_ship = $2;
       std::string* attackee_ship = $4;
       char tact = $3;
       std::pair<AttributeMap*, TorpedoSet*>* compair = $5;
       AttributeMap* patmap = compair->first;
       TorpedoSet* pmisset  = compair->second;

       ICmd* pCmd = CombatOrderActor::Builder()
                                .ship_code(*attacker_ship)
                                .target(*attackee_ship)
                                .tactic(tact)
                                .set_torpedoes(*pmisset)
                                .set_attributes(*patmap)
                                .build(); 
       if (pCmd)
       {
          pCmd->invoke();
       }
       SafeDelete(patmap);
       SafeDelete(pmisset);
       SafeDelete(compair);
       SafeDelete(attacker_ship);
       SafeDelete(attackee_ship);
       SafeDelete(pCmd);
   }
   // combat apply ...
   | TOK_COMBAT TOK_APPLY combat_damaged_ship combat_apply_spec {
       // BUGBUG OK
       // $3 is target ship to apply on
       // $4 is AttributeMap to use.  We do allow repairing "M"
       std::string* target_ship = $3;
       AttributeMap* patmap = $4;
       ICmd* pCmd = CombatApplyActor::Builder()
                              .ship_code(*target_ship)
                              .set_assignments(*patmap)
                              .build();
       if (pCmd) {
           pCmd->invoke();
       }
       SafeDelete(target_ship);
       SafeDelete(patmap);
       SafeDelete(pCmd);
   }
   // ca ...
   | TOK_COMBAT_APPLY combat_damaged_ship combat_apply_spec {
       // BUGBUG OK
       // $2 is target ship to apply on
       // $3 is AttributeMap to use.  We do allow repairing "M"
       std::string* target_ship = $2;
       AttributeMap* patmap = $3;
       ICmd* pCmd = CombatApplyActor::Builder()
                              .ship_code(*target_ship)
                              .set_assignments(*patmap)
                              .build();
       if (pCmd) {
           pCmd->invoke();
       }
       SafeDelete(target_ship);
       SafeDelete(patmap);
       SafeDelete(pCmd);
   }
   | TOK_COMBAT_DRAFTS error {
          yyerror(&@1, "cd: usage: cd"); YYABORT;
   }
   | TOK_COMBAT_ORDER error {
          yyerror(&@1, "co: usage: co <attacker> <attack|dodge|escape> <target?> [PD=n P=n S=n L=n T=n H=n]");
          YYABORT;
   }
   | TOK_COMBAT_APPLY error {
          yyerror(&@1, "ca: usage: ca <draft_id> <damage_spec...>");
          YYABORT;
   }
   | TOK_COMBAT_COMMIT error {
          yyerror(&@1, "cc: usage: cc <draft_id>"); YYABORT;
   }
   | TOK_COMBAT_CANCEL error { 
          yyerror(&@1, "cx: usage: cx <draft_id>"); YYABORT;
   }
   //  cd
   | TOK_COMBAT_DRAFTS {
       // BUGBUG OK
       ICmd* pCmd = CombatDraftsActor::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   // combat commit
   | TOK_COMBAT TOK_COMMIT {
       // BUGBUG OK
       ICmd* pCmd = CombatCommitActor::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   // cc
   | TOK_COMBAT_COMMIT {
       // BUGBUG OK
       ICmd* pCmd = CombatCommitActor::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   // combat cancel
   | TOK_COMBAT TOK_CANCEL {
       // BUGBUG OK
       ICmd* pCmd = CombatCancelActor::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   // cx
   | TOK_COMBAT_CANCEL {
       // BUGBUG OK
       ICmd* pCmd = CombatCancelActor::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
   ;

building_draft_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;

combat_initiator_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;

combat_damaged_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;

combat_target_ship:
  TOK_STRING {
      $$ = $1;
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
      $$ = 'E';
  }
  ;


new_combat_order_attr_spec: {  
     $$ = new std::pair<AttributeMap*,  TorpedoSet*>;
     $$->first = new AttributeMap;
     $$->second = new TorpedoSet;
  }
  | new_combat_order_attr_spec TOK_PD_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::POWER_DRIVE, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate POWER_DRIVE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_P_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::PHASIC, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate PHASIC assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_S_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::SHIELD, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate SHIELD assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_L_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->first->insert({AttributeID::LAUNCHER, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate LAUNCHER assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | new_combat_order_attr_spec TOK_T_ASSIGN {
      $$ = $1;
      // Torpedo power per torpedo is inserted into the vector of TorpedoSet
      // The AttributeMap[TORPEDO] value is left alone. We don't
      // use it for combat per se. 
      $$->second->push_back($2);
  }
  ;


combat_apply_spec: {
     $$ = new AttributeMap;
  }
  | combat_apply_spec TOK_PD_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::POWER_DRIVE, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate POWER_DRIVE assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | combat_apply_spec TOK_P_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::PHASIC, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate PHASIC assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | combat_apply_spec TOK_S_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::SHIELD, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate SHIELD assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | combat_apply_spec TOK_L_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::LAUNCHER, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate LAUNCHER assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  | combat_apply_spec TOK_T_ASSIGN {
      $$ = $1;
      $$ = $1;
      // Allow the damage to hit the "Torpedo" enterprise of the
      // ship. Effectively destroys torpedoes.  BIGBUG -- should
      // be somewhat more catastrophic than this.
      auto [it, inserted] = $$->insert({AttributeID::TORPEDO, $2});
      if (!inserted) {
        yyerror(&@2, "duplicate LAUNCHER assignment");
        it->second = $2; // choose: overwrite, or keep old
      }
  }
  ;

ship_class: TOK_STRING {
    if (!$1 || $1->empty()) {
        yyerror(&@1, "Invalid ship class: empty string > HELP BN");
        SafeDelete($1);
        YYERROR;
    }
    char ship_type = std::toupper((*$1)[0]);
    if (ship_type != 'W' && ship_type != 'S') {
        std::string err = "Invalid ship class: "
                        + *$1
                        + ". Must start with 'W' or 'S'.";
        yyerror(&@1, err.c_str());
        SafeDelete($1);
        YYERROR;
    }
    
    std::string rest = $1->substr(1);
    if (!rest.empty()
        && (rest.length() > 3
        || !std::all_of(rest.begin(), rest.end(), ::isdigit))) {
        std::string err = "Invalid custom ship class ID format: '"
                        + *$1
                        + "'. Number must be 1-3 digits.";
        yyerror(&@1, err.c_str());
        SafeDelete($1);
        YYERROR;
    }
    $$ = $1;
}
;

build_attr_spec_nonempty:
  TOK_PD_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::POWER_DRIVE, $1});
  }
  | TOK_P_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::PHASIC, $1});
  }
  | TOK_S_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::SHIELD, $1});
  }
  | TOK_L_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::LAUNCHER, $1});
  }
  | TOK_T_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::TORPEDO, $1});
  }
  | TOK_H_ASSIGN {
      $$ = new AttributeMap;
      $$->insert({AttributeID::HANGAR, $1});
  }
  
  | TOK_PD_ASSIGN_INVALID {
      std::string err = "Invalid value for PD: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  | TOK_P_ASSIGN_INVALID {
      std::string err = "Invalid value for P: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  | TOK_S_ASSIGN_INVALID {
      std::string err = "Invalid value for S: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  | TOK_L_ASSIGN_INVALID {
      std::string err = "Invalid value for L: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  | TOK_T_ASSIGN_INVALID {
      std::string err = "Invalid value for T: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  | TOK_H_ASSIGN_INVALID {
      std::string err = "Invalid value for H: '" + *$1 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@1, err.c_str());
      SafeDelete($1);
      YYABORT;
  }
  
  | build_attr_spec_nonempty TOK_PD_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::POWER_DRIVE, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  | build_attr_spec_nonempty TOK_P_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::PHASIC, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  | build_attr_spec_nonempty TOK_S_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::SHIELD, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  | build_attr_spec_nonempty TOK_L_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::LAUNCHER, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  | build_attr_spec_nonempty TOK_T_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::TORPEDO, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  | build_attr_spec_nonempty TOK_H_ASSIGN {
      $$ = $1;
      auto [it, inserted] = $$->insert({AttributeID::HANGAR, $2});
      if (!inserted) {
        it->second = $2;
      }
  }
  
  // ADD: Continuation rules - invalid (catch errors in middle/end of list)
  | build_attr_spec_nonempty TOK_PD_ASSIGN_INVALID {
      std::string err = "Invalid value for PD: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
  | build_attr_spec_nonempty TOK_P_ASSIGN_INVALID {
      std::string err = "Invalid value for P: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
  | build_attr_spec_nonempty TOK_S_ASSIGN_INVALID {
      std::string err = "Invalid value for S: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
  | build_attr_spec_nonempty TOK_L_ASSIGN_INVALID {
      std::string err = "Invalid value for L: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
  | build_attr_spec_nonempty TOK_T_ASSIGN_INVALID {
      std::string err = "Invalid value for T: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
  | build_attr_spec_nonempty TOK_H_ASSIGN_INVALID {
      std::string err = "Invalid value for H: '" + *$2 + "'. Must be a positive integer. > HELP ATTR";
      yyerror(&@2, err.c_str());
      SafeDelete($1);
      SafeDelete($2);
      YYABORT;
  }
;

build_cmd:
  TOK_BUILD_NEW ship_class TOK_STRING {
      // BUGBUG OK
      // build new W|S NAME
      // Example:  BN W FooShip
      // Example:  BN S BarShip
      // Example:  BN W4 AnotherShip
      // Example:  BN S11 yetAnotherShip
      if ($2 == nullptr) {
            // ship_class validation failed. already reported error
            SafeDelete($3);
      }
      else {
         ICmd* pCmd = BuildNewActor::Builder()
                     .set_ship_code(*$2)
                     .set_ship_name(*$3)
                     .build();
         pCmd->invoke();
         SafeDelete($2);
         SafeDelete($3);
         SafeDelete(pCmd);
      }
  }
  | TOK_BUILD_NEW ship_class TOK_STRING error {
      // BUGBUG OK
      yyerror(&@4, "Too many arguments. Usage: bn {W|S} name > HELP BN");
      SafeDelete($2);
      SafeDelete($3);
      YYABORT;
  }
  | TOK_BUILD_NEW ship_class error {
      // BUGBUG OK
      // ERROR: BN W (missing ship name)
      yyerror(&@3, "Missing ship name. Usage: bn {W|S} name > HELP BN");
      SafeDelete($2);
      YYABORT;
  }
  | TOK_BUILD_NEW error {
      // BUGBUG OK
      // ERROR: BN (no arguments at all)
      yyerror(&@2, "Missing arguments. Usage: bn {W|S} name > HELP BN");
      YYABORT;
  }

  // build new ...
  // build new W|S NAME
  | TOK_BUILD TOK_NEW ship_class TOK_STRING {
      // BUGBUG OK
      if ($3 == nullptr) {
            // ship_class validation failed. already reported error
            SafeDelete($4);
      }
      else {  
         ICmd *pCmd = BuildNewActor::Builder()
                     .set_ship_code(*$3)
                     .set_ship_name(*$4)
                     .build();
         pCmd->invoke();
         SafeDelete($3);
         SafeDelete($4);
         SafeDelete(pCmd);
      }
  }
  | TOK_BUILD TOK_NEW ship_class TOK_STRING error {
      // BUGBUG OK
      yyerror(&@5, "Too many arguments. Usage: build new {W|S} name > HELP BUILD");
      SafeDelete($3);
      SafeDelete($4);
      YYABORT;
  }
  | TOK_BUILD TOK_NEW ship_class error {
      // BUGBUG OK
      // ERROR: BUILD NEW W (missing name)
      yyerror(&@4, "Missing ship name. Usage: build new {W|S} name > HELP BUILD");
      SafeDelete($3);
      YYABORT;
  }

  | TOK_BUILD TOK_NEW error {
      // BUGBUG OK
      // ERROR: BUILD NEW (missing ship class and name)
      yyerror(&@3, "Missing arguments. Usage: build new {W|S} name > HELP BUILD");
      YYABORT;
  }
  // bd SHIP_ID
  // bd SHIP_NAME
  | TOK_BUILD_DRAFTS building_draft_ship {
      // BUGBUG OK
      // can also be a name, but we will take it as it comes.
      ICmd *pCmd = BuildShowDraftActor::Builder()
                  .set_target(*$2)
                  .build();
      pCmd->invoke();
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  // bd
  | TOK_BUILD_DRAFTS {
      // BUGBUG OK
      ICmd *pCmd = BuildDraftsActor::Builder().build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

  | TOK_BUILD TOK_SET_ATTR build_target build_attr_spec_nonempty {
      // BUGBUG OK
      // SUCCESS: BUILD SET target attr=# [attr=# ...]
      ICmd *pCmd = BuildSetActor::Builder()
                    .set_target(*$3)
                    .set_attributes(*$4)
                    .build();
      pCmd->invoke();
      SafeDelete($4)
      SafeDelete($3)
      SafeDelete(pCmd);
  }

  | TOK_BUILD TOK_SET_ATTR build_target {
      // BUGBUG OK
      // ERROR: BUILD SET target (missing attributes)
      yyerror(&@3, "Missing attributes. Usage: build set {NAME|HULL} attr=# ... > HELP BS");
      SafeDelete($3);
      YYABORT;
  }

  | TOK_BUILD TOK_SET_ATTR build_target TOK_STRING {
      // BUGBUG OK
      // ERROR: BUILD SET target X=1 (invalid attribute key)
      std::string err = "Invalid attribute: " + *$4;
      err += ". Valid attributes: PD, P, S, L, T, H > HELP ATTR";
      yyerror(&@4, err.c_str());
      SafeDelete($3);
      SafeDelete($4);
      YYABORT;
  }

  | TOK_BUILD TOK_SET_ATTR build_target build_attr_spec_nonempty error {
      // BUGBUG OK
      // ERROR: BUILD SET target attr=# ... extra junk
      yyerror(&@5, "Unexpected tokens after attributes. > HELP BS");
      SafeDelete($3);
      SafeDelete($4);
      YYABORT;
  }

  | TOK_BUILD TOK_SET_ATTR error {
      // BUGBUG OK
      // ERROR: BUILD SET (missing target and attributes)
      yyerror(&@3, "Missing ship name/hull and attributes. Usage: build set {NAME|HULL} attr=# ... > HELP BS");
      YYABORT;
  }

  // =========================================================================
  // BS (alias) with comprehensive error handling
  // =========================================================================

  | TOK_BUILD_SET_ATTR build_target build_attr_spec_nonempty {
      // BUGBUG OK
      // SUCCESS: BS target attr=# [attr=# ...]
      ICmd *pCmd = BuildSetActor::Builder()
                    .set_target(*$2)
                    .set_attributes(*$3)
                    .build();
      pCmd->invoke();
      SafeDelete($3)
      SafeDelete($2)
      SafeDelete(pCmd);
  }

  | TOK_BUILD_SET_ATTR build_target {
      // BUGBUG OK
      // ERROR: BS target (missing attributes)
      yyerror(&@2, "Missing attributes. Usage: bs {NAME|HULL} attr=# ... > HELP BS");
      SafeDelete($2);
      YYABORT;
  }

  | TOK_BUILD_SET_ATTR build_target TOK_STRING {
      // BUGBUG OK
      // ERROR: BS target X=1 (invalid attribute - caught as TOK_STRING)
      std::string err = "Invalid attribute: " + *$3;
      err += ". Valid attributes: PD, P, S, L, T, H > HELP BS";
      yyerror(&@3, err.c_str());
      SafeDelete($2);
      SafeDelete($3);
      YYABORT;
  }

  | TOK_BUILD_SET_ATTR build_target build_attr_spec_nonempty error {
      // BUGBUG OK
      // ERROR: BS target attr=# ... extra junk
      yyerror(&@4, "Unexpected tokens after attributes. > HELP BS");
      SafeDelete($2);
      SafeDelete($3);
      YYABORT;
  }

  | TOK_BUILD_SET_ATTR error {
      // BUGBUG OK
      // ERROR: BS (missing everything)
      yyerror(&@2, "Missing ship name/hull and attributes. Usage: bs {NAME|HULL} attr=# ... > HELP BS");
      YYABORT;
  }

  // build commit
  // build commit CODE|NAME
  | TOK_BUILD TOK_COMMIT build_target {
      // BUGBUG OK
      ICmd *pCmd = BuildCommitActor::Builder()
                  .set_target(*$3)
                  .build();
      pCmd->invoke();
      SafeDelete($3);
      SafeDelete(pCmd);
  }

  // bc
  | TOK_BUILD_COMMIT build_target {
      // BUGBUG OK
      ICmd *pCmd = BuildCommitActor::Builder()
                   .set_target(*$2)
                   .build();
      pCmd->invoke();
      SafeDelete($2);
      SafeDelete(pCmd);
  }

  | TOK_BUILD TOK_CANCEL build_target {
      // BUGBUG OK
      ICmd *pCmd = BuildCancelActor::Builder()
                  .set_target(*$3)
                  .build();
      pCmd->invoke();
      SafeDelete($3);
      SafeDelete(pCmd);
  }

  // bx
  | TOK_BUILD_CANCEL build_target {
      // BUGBUG OK
      ICmd *pCmd = BuildCancelActor::Builder()
                  .set_target(*$2)
                  .build();
      pCmd->invoke();
      SafeDelete($2);
      SafeDelete(pCmd);
  }
  |
  // With no arguments, treat this as 'build drafts' 
  // build
  // b
  TOK_BUILD {
      // BUGBUG OK
      ICmd *pCmd = BuildDraftsActor::Builder().build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }

| TOK_BUILD error { yyerror(&@1,          "build syntax               > HELP BUILD"); YYABORT; }
| TOK_BUILD_DRAFTS error { yyerror(&@1,   "usage: bd {NAME|HULL}      > HELP BD"); YYABORT; }
| TOK_BUILD_COMMIT error { yyerror(&@1,   "usage: bc {NAME|HULL}      > HELP BC"); YYABORT; }
| TOK_BUILD_CANCEL error { yyerror(&@1,   "usage: bx {NAME|HULL}      > HELP BX"); YYABORT; }
;

build_target:
  TOK_STRING
  {
     $$ = $1;
  }
  ;

// deployment
// "deploy" { return TOK_DEPLOY; }
deployable_ship:
  TOK_STRING {
      $$ = $1;
  }
  ;

deploy_cmd:
    TOK_DEPLOY deployable_ship TOK_STRING {
       // BUGBUG OK
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
   | TOK_DEPLOY error {
       yyerror(&@1, "deploy: usage: deploy <ship> <location>");
       YYABORT;
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
       // BUGBUG OK
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
  | TOK_MOVE error {
       yyerror(&@1, "move: usage: move <ship> <destination> [via ...]");
       YYABORT;
  }
  ;

chart_cmd:
  // chart <ship> <destination> [waypoints...]
  TOK_CHART TOK_STRING TOK_STRING chain_move_location {
       std::string ship(*$2);
       std::string first_dest(*$3);
       std::vector<std::string>* waypoints = $4;

       ICmd* pCmd = ChartCommand::Builder()
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
  | TOK_CHART error {
       yyerror(&@1, "chart: usage: chart <ship> <destination> [via ...]");
       YYABORT;
  }
  ;

// Pickup, Drop
// "pick" { return TOK_PICK; }
// "drop" { return TOK_DROP; }
pickdrop_cmd:
  TOK_PICK TOK_STRING TOK_STRING {
     // BUGBUG OK
     // pick <systemship> <warpship>
     std::string sysship(*$2);
     std::string warpship(*$3);
     ICmd* pCmd = PickCommand::Builder()
                      .systemship(sysship)
                      .warpship(warpship)
                      .build();
     pCmd->invoke();
     delete pCmd;
  }
  | TOK_DROP TOK_STRING TOK_STRING {
     // UNTESTED
     // drop <systemship> <warpship>
     std::string sysship(*$2);
     std::string warpship(*$3);
     ICmd* pCmd = DropCommand::Builder()
                      .systemship(sysship)
                      .warpship(warpship)
                      .build();
     pCmd->invoke();
     delete pCmd;
  }
  | TOK_PICK error {
      yyerror(&@1, "pick: usage: pick <systemship> <warpship>");
      YYABORT;
  }
  | TOK_DROP error {
      yyerror(&@1, "drop: usage: drop <systemship> <warpship>");
      YYABORT;
  }
  ;


rep_cmd:
  // repair
  // rp
  TOK_REPAIR {
     // UNTESTED
     ICmd* pCmd = RepairCommand::Builder().build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
  // repair ...
  // rp ...
  | TOK_REPAIR TOK_STRING repair_attr_spec {
     // UNTESTED
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
     ICmd* pCmd = ResupplyCommand::Builder().set_ship_code(ship).set_torpedoes(qty).build();
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
  
| TOK_REPAIR error { yyerror(&@1, "repair: usage: repair <ship> PD=n|P=n|S=n|L=n (one attribute)"); YYABORT; }
| TOK_RESUPPLY error { yyerror(&@1, "resupply: usage: resupply <ship> <amount>"); YYABORT; }
| TOK_RETREAT error { yyerror(&@1, "retreat: usage: retreat <ship> <destination>"); YYABORT; }
;

repair_attr_spec:
  TOK_PD_ASSIGN {
      g_repair_builder->set_attribute("pd");
      g_repair_builder->set_amount($1);
  }
  | TOK_P_ASSIGN {
      g_repair_builder->set_attribute("p");
      g_repair_builder->set_amount($1);
  }
  | TOK_S_ASSIGN {
      g_repair_builder->set_attribute("s");
      g_repair_builder->set_amount($1);
  }
  | TOK_L_ASSIGN {
      g_repair_builder->set_attribute("l");
      g_repair_builder->set_amount($1);
  }
  ; 
 
configure_cmd:
  TOK_CONFIGURE TOK_RELOAD error {
      yyerror(&@1, "configure: usage: configure [reload conf|reload ai]");
      YYABORT;
  }
  | TOK_CONFIGURE TOK_RELOAD TOK_CONF error {
      yyerror(&@1, "configure: usage: configure [reload conf|reload ai]");
      YYABORT;
  }
  | TOK_CONFIGURE TOK_RELOAD TOK_AI error {
      yyerror(&@1, "configure: usage: configure [reload conf|reload ai]");
      YYABORT;
  }
  | TOK_CONFIGURE TOK_RELOAD TOK_CONF {
      // configure reload conf — reload kh.conf
      ICmd* pCmd = TuningCommand::Builder()
          .set_reload_conf_mode()
          .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_CONFIGURE TOK_RELOAD TOK_AI {
      // configure reload ai — reload AI DSL
      ICmd* pCmd = TuningCommand::Builder()
          .set_reload_ai_mode()
          .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
  | TOK_CONFIGURE error {
      yyerror(&@1, "configure: usage: configure [reload conf|reload ai]");
      YYABORT;
  }
  | TOK_CONFIGURE {
      // configure — show current settings
      ICmd* pCmd = TuningCommand::Builder()
          .set_show_mode()
          .build();
      pCmd->invoke();
      SafeDelete(pCmd);
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
       SafeDelete($2);
       SafeDelete(pCmd);
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
       Logger::instance().error("[PARSER] Indeterminate error cause");
       err = std::string(LC_PARSER_ROOT_ERROR);
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
       const std::string yyerr = std::format("[YACC] Error: >{}<", msg);
       Logger::instance().debug(yyerr);
    }
    else {
       const std::string yyerr("[YACC] Error");
       Logger::instance().debug(yyerr);
    }
}

// Richer helper used by our grammar actions.
void yyerror(YYLTYPE* loc, const char* msg)
{
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
    const std::string yyerr = std::format("[YACC] yyerror {}{}: {}",
       (const char*)((hasloc)?"(loc,":"("),
       (const char*)((!hasloc)?"(msg)":",msg)"),
       dbgstr.c_str());
    Logger::instance().debug(yyerr);
}

