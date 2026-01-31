//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <format>
#include <iostream>
#include <queue>
#include <unordered_map>

#include "ai_db_mutex.h"
#include "app.h"
#include "ce.h"
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
extern bool get_parser_error(std::string& err);

// input: command line from user
// output: modified errmsg (if any)
int internal_command_handler_body(const std::string cmdline,
                                  std::string& errmsg)
{
    // Clear telemetry message buffer before command execution
    Telemetry::instance().clear_messages();

    // Try parser first (handles migrated commands)
    YY_BUFFER_STATE buffer = yy_scan_string(cmdline.c_str());
    int parse_result = yyparse();
    yy_delete_buffer(buffer);

    const std::string parsediag = std::format(
        "[INTERNAL] Parse result for >{}< is: {}", cmdline, parse_result);
    Logger::instance().info(parsediag);
    // if error with parse
    if (parse_result != 0)
    {

        bool found_cause = get_parser_error(errmsg);
        if (!found_cause && errmsg.empty())
        {
            errmsg = "Parser provided no error message";
        }
        const std::string parseerr = std::format(
            "[INTERNAL] Parse error for >{}< is: >{}<", cmdline, errmsg);
        Logger::instance().info(parseerr);
    }
    return parse_result;
}

bool handle_usr_command(const HttpRequest* req, HttpResponse* resp)
{

    // We only handle POST method
    if (req->method != "POST")
    {
        resp->status = 405;
        resp->body = json_error("method");
        return false; // not done
    }

    // We need this from the HttpRequest to fulfill the command handling
    // because it contains context for which player is invoking a command.
    AuthContext context = require_auth((const HttpRequest*)req, resp);

    // Part of the handshake, we want result 200
    if (resp->status != 200)
    {
        return false; // not done
    }

    // This command came through the REST endpoint, so we're going to
    // set context with that information from the endpoint.

    // Configure StateMachine with DB context
    StateMachine::instance().set_game_id(context.game_id);

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
    //   ==> The state machine keeps track of the active player (the turn
    //   holding
    //       player).  All State Machine inhibits are based on the turn-holding
    //       player compared to the 'current' player issuing the candidate
    //       command.
    StateMachine::instance().set_current_player(context.player);
    StateMachine::instance().set_current_user_id(context.user_id);
    // END BUGBUG

    // Ok, let's go..
    std::string cmdline = trim(json_get_string(req->body, "command"));

    if (cmdline.empty())
    {
        Logger::instance().info("cmdline: " + cmdline);
        resp->status = 400;
        resp->body = json_error("empty command");
        return false; // not done
    }

    // we don't know what the parser will do yet, so we anticipate
    // possible error msg
    std::string errmsg;

    // now, give the parser the command line (from any player or AI)
    int parse_result = internal_command_handler_body(cmdline, errmsg);

    // If parser succeeded, return response with Telemetry

    // We've already done the inside work, so now let's disposition
    // the HTTP response
    if (parse_result == 0)
    {
        // Parse error was 0 -- this is successful
        // Check if game_id changed (e.g., after 'start' command)
        int new_game_id = StateMachine::instance().get_game_id();
        if (new_game_id != context.game_id)
        {
            // Update session with new game_id
            DatabaseManager& db = DatabaseManager::instance();
            db.Exec("UPDATE sessions SET game_id=? WHERE token=?",
                    {new_game_id, context.token});
            Logger::instance().info("[CMD] Updated session game_id to " +
                                    std::to_string(new_game_id));
        }

        // Only load game state if a game has been started
        // (game_id will be 0 until 'start' command is run)
        if (new_game_id != 0)
        {
            GameState s = StateMachine::instance().load_game(new_game_id);
        }
        // Get the message for sinking to the HTTP Response
        std::string event_msg;
        Telemetry::instance().source_messages(event_msg);
        resp->body = event_msg;
    }
    else
    {
        resp->status = 400;
        // Get the error message for sinking to the HTTP Response
        resp->body = json_error(errmsg);
        Logger::instance().info("[CMD] User command: " + cmdline);
        Logger::instance().info("[CMD] Parser error: " + errmsg);
    }
    return false; // not done
}
