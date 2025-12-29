%{
#include <cstdio>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "logger.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);
%}

%union {
   int ival;
   std::string* sval;
}

%token <ival> TOK_INT
%token <sval> TOK_STRING
%type  <sval> deployable_ship

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
%token TOK_EQUAL
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
%token TOK_RESET
%token TOK_SAVE
%token TOK_SCREEN
%token TOK_SET_ATTR
%token TOK_START_GAME
%token TOK_STATS
%token TOK_STATUS
%token TOK_SYSTEM
%token TOK_TUBE

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
  | commitment_cmd
  | combat_cmd
  | build_cmd
  | deploy_cmd
  | move_cmd
  | pickdrop_cmd
  ;

session_cmd:
   TOK_START_GAME TOK_LEARNING
   {
        Logger::instance().info("Start a learning game scenario");
   }
   |
   TOK_START_GAME TOK_BASIC
   {
        Logger::instance().info("Start a basic game scenario");
   }
   |
   TOK_START_GAME TOK_ADVANCED
   {
        Logger::instance().info("Start an advanced game scenario");
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
      Logger::instance().info("Advance active player to next  phase");
  }
  | TOK_DONE {
      Logger::instance().info("Advance active player to first phase "
                      "of opponent, if possible. (do not ignore "
                      "required combat)");
  }
  ;

// Phase evolution
// "cancel" { return TOK_CANCEL; }
// "commit" { return TOK_COMMIT; }
commitment_cmd:
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
      Logger::instance().info("Combat application spec: ");
   }
  ;


building_draft_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Drafting ship: "
                              ">" + ship_id + "<" );
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
  TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec PD ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec BEAM ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec SCREEN ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec TUBE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Combat Order update spec MISSILE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  ;

additional_combat_order_spec:
  // no spec
  | TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec PD ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec BEAM ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec SCREEN ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec TUBE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_combat_order_spec {
      std::stringstream ss;
      ss << "Additional Combat Order update spec MISSILE ";
      ss << (int) ($3);
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
  TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec PD ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec BEAM ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec SCREEN ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec TUBE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Combat Application update spec MISSILE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  ;

additional_combat_application_spec:
  // no spec
  | TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec PD ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec BEAM ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec SCREEN ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec TUBE ";
      ss << (int) ($3);
      Logger::instance().info(ss.str());
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_combat_application_spec {
      std::stringstream ss;
      ss << "Additional Combat Application update spec MISSILE ";
      ss << (int) ($3);
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
  | TOK_BUILD TOK_DRAFTS {
      Logger::instance().info("List of all pending build drafts by self");
  }
  | TOK_BUILD TOK_DRAFTS building_draft_ship {
      Logger::instance().info("Detail about this particular ship "
                              "build draft: ");
  }
  | TOK_BUILD TOK_SET_ATTR build_attr_spec {
      Logger::instance().info("Set (or overwrite) attribute to build draft");
  }
  ;

build_attr_spec:
  TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec PD");
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec BEAM");
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec SCREEN");
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec TUBE");
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec MISSILE");
  }
  ;

additional_build_attr_spec:
  // or no more spec
  | TOK_POWER_DRIVE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec PD");
  }
  | TOK_BEAM TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec BEAM");
  }
  | TOK_SCREEN TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec SCREEN");
  }
  | TOK_TUBE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec TUBE");
  }
  | TOK_MISSILE TOK_EQUAL TOK_INT additional_build_attr_spec {
      Logger::instance().info("Building, update spec MISSILE");
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
   // or no more location
   | TOK_STRING {
       std::string destination(*$1);
       Logger::instance().info("An additional hex on chain for move "
                     " to destination >" + destination + "<");
   }
   ;
moveable_ship:
  TOK_STRING {
      std::string ship_id(*$1);
      Logger::instance().info("Moveable ship: "
                              ">" + ship_id + "<");
  }
  ;

move_cmd:
  TOK_MOVE {
       Logger::instance().info("Show move status");
  }
  | TOK_MOVE moveable_ship TOK_STRING chain_move_location {
       std::string destination(*$3);
       Logger::instance().info("Moving a ship to a hex or star hex, "
              "or base star hex, through other hexes. "
              "Starting at >" + destination + "<");
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
    | TOK_HELP TOK_EQUAL
    {
        Logger::instance().info("Help about TOK_EQUAL");
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
