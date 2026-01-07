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
    StateMachine::getInstance().set_game_id(a.game_id);
    StateMachine::getInstance().set_current_player(a.player);
    StateMachine::getInstance().set_current_user_id(a.user_id);

    // Clear telemetry message buffer before command execution
    Telemetry::getInstance().clear_messages();

    // Try parser first (handles migrated commands)
    YY_BUFFER_STATE buffer = yy_scan_string(cmdline.c_str());

    int parse_result = yyparse();
    yy_delete_buffer(buffer);

    // If parser succeeded, return response with Telemetry
    if (parse_result == 0)
    {
        Logger::instance().info("Command handled by parser: " + cmdline);

        // Check if game_id changed (e.g., after 'start' command)
        int new_game_id = StateMachine::getInstance().get_game_id();
        if (new_game_id != a.game_id)
        {
            // Update session with new game_id
            DatabaseManager& db = DatabaseManager::getInstance();
            db.exec(
                "UPDATE sessions SET game_id=" + std::to_string(new_game_id) +
                " WHERE token='" + db.esc(a.token) + "'");
            Logger::instance().info("Updated session game_id to " +
                                    std::to_string(new_game_id));
        }

        // Only load game state if a game has been started
        // (game_id will be 0 until 'start' command is run)
        if (new_game_id != 0)
        {
            GameState s = StateMachine::getInstance().load_game(new_game_id);
        }

        // Get accumulated telemetry messages and combine them
        auto messages = Telemetry::getInstance().get_messages();
        std::ostringstream combined;
        for (size_t i = 0; i < messages.size(); ++i)
        {
            if (i > 0)
                combined << "\n";
            combined << messages[i];
        }
        std::string event_msg =
            combined.str().empty() ? "Command executed\n" : combined.str();

        resp->body = Telemetry::getInstance().write(event_msg);

        /* std::ostringstream err; */
        /* err << "{\"ok\":false,\"event\":\"Command failed\",\"state\":" <<
         * s.to_json() << "}"; */
        /* resp->body = err.str(); */

        return;
    }

    // Parser failed
    Logger::instance().info("Falling back to legacy handler: " + cmdline);
    resp->status = 400;
    resp->body = json_error("unknown command");
    Logger::instance().error("Unknown command attempted");
    return;
}
