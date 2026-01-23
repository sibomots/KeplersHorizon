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
#include "ce.h"
#include "comms.h"
#include "db.h"
#include "events.h"
#include "logger.h"
#include "mapgraph.h"
#include "ships.h"
#include "state.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"
#include "util.h"

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern "C" int yyparse();
extern bool get_parser_error(std::string& err);

bool handle_usr_command(const HttpRequest* req, HttpResponse* resp)
{
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return false; // not done
    }

    AuthContext a = require_auth((const HttpRequest*)req, resp);

    if (resp->status != 200)
    {
        return false; // not done
    }

    std::string cmdline = trim(json_get_string(req->body, "command"));

    if (cmdline.empty())
    {
        resp->status = 400;
        resp->body = json_error("empty command");
        return false; // not done
    }

    // DEBUG BUGBUG
    if (cmdline.compare("malloc") == 0) {
        // forcing the mtrace to account
        return true; // is done
    }
    // Configure StateMachine with DB context
    StateMachine::getInstance().set_game_id(a.game_id);

    // BEGIN BUGBUG
    // This is probably OK --- the server is single threaded -
    //   When a new command comes in from either player, we need to know
    //   which player is issuing _that_ command and that's why we set
    //   the statemachine to the 'current' player to this player.
    // The thread of control completes and the next command that is
    //  handled by this routine will set the 'current' player to whomever
    //  is sending the command.
    //
    // This does not affect the state of the "active player" -- the player who
    //  owns the turn being played.
    // This game loop will take commands from either player at any time.
    //   ==> Handling per user commands is tied to the current_player
    //   ==> The state machine keeps track of the active player (the turn holding
    //       player).  All State Machine inhibits are based on the turn-holding
    //       player compared to the 'current' player issuing the candidate command.

    StateMachine::getInstance().set_current_player(a.player);
    StateMachine::getInstance().set_current_user_id(a.user_id);
    // END BUGBUG

    // Clear telemetry message buffer before command execution
    Telemetry::getInstance().clear_messages();

    // Try parser first (handles migrated commands)
    YY_BUFFER_STATE buffer = yy_scan_string(cmdline.c_str());

    int parse_result = yyparse();
    yy_delete_buffer(buffer);

    // If parser succeeded, return response with Telemetry
    if (parse_result == 0)
    {
        // Check if game_id changed (e.g., after 'start' command)
        int new_game_id = StateMachine::getInstance().get_game_id();
        if (new_game_id != a.game_id)
        {
            // Update session with new game_id
            DatabaseManager& db = DatabaseManager::getInstance();
            db.exec(
                "UPDATE sessions SET game_id=" + std::to_string(new_game_id) +
                " WHERE token='" + db.esc(a.token) + "'");
            Logger::instance().info("[CMD] Updated session game_id to " +
                                    std::to_string(new_game_id));
        }

        // Only load game state if a game has been started
        // (game_id will be 0 until 'start' command is run)
        if (new_game_id != 0)
        {
            GameState s = StateMachine::getInstance().load_game(new_game_id);
        }

        std::string event_msg;
        Telemetry::getInstance().source_messages(event_msg);
        resp->body = event_msg;
    }
    else {
          std::string err;
          bool found_cause = get_parser_error(err);
          if (!found_cause && err.empty()) {
             err = "Parser provided no error message";
          }
          // Parser failed
          resp->status = 400;
          resp->body = json_error(err);
          Logger::instance().info("[CMD] User command: " + cmdline);
          Logger::instance().info("[CMD] Parser error: " + err);
    }

    return false; // not done
}
