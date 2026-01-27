//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>

#include "ai_command_injector.h"
#include "cmd.h"
#include "logger.h"
#include "statemachine.h"

bool AICommandInjector::inject_batch(int game_id, char ai_player,
                                     const std::vector<std::string>& commands)
{
    // WE DON'T DO BATCHES.
    // If you want to issue more than one command, then use inject multiple times.
    Logger::instance().info("Inside AICommandInjector::inject_batch");
    return false;
}

bool AICommandInjector::inject(int game_id, char ai_player,
                               const std::string& cmdline)
{
    bool bres = false;
    Logger::instance().info("Inside AICommandInjector::inject");

    // CAVEAT -- we really do not want this.
    //  The mutex protects single thread access to the parser,
    //  But we do not want the state machine to be corrupted
    //  there's a chance we corrupt the state here:
    //  then have to wait for another user to invoke command handler
    //  they will have corrupted the statemachine!

    // Therefore:
    // IMPORTANT    
    // What we want is a two step process:
    //    1. Get the mutex for thet state machine
    //    2. Modify the state per moment of "comamnd invocation"
    //    3. Then get the mutex for the command parser, invoke() the command
    //    4. Collect results (error messages or telemetry, or both)
    //    5. release the inner mutex
    //    6. release the outer mutex.

#if 0
    // Set StateMachine context for AI player
    StateMachine::instance().set_game_id(game_id);
    StateMachine::instance().set_current_player(ai_player);
    StateMachine::instance().set_current_user_id(get_ai_user_id());

    std::string errmsg;
    int parse_result = internal_command_handler_body(cmdline, errmsg);

    // Log outcome
    if (parse_result == 0)
    {
        Logger::instance().info("[AI] ✓ Player " + std::string(1, ai_player) +
                                ": " + cmdline);
        bres = true;
    }
    else
    {
        Logger::instance().error("[AI] ✗ Player " + std::string(1, ai_player) +
                                 ": " + cmdline + " | Error: " + errmsg);
        bres = false;
    }
#endif

    return bres;
}

int AICommandInjector::get_ai_user_id()
{
    return 3;
}
