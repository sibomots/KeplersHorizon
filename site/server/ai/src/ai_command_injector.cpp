///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "ai_command_injector.h"

#include <string>
#include <vector>

#include "logger.h"
#include "taskrunner.h"

bool AICommandInjector::inject_batch(int game_id, char ai_player,
                                     const std::vector<std::string>& commands)
{
    // WE DON'T DO BATCHES.
    // If you want to issue more than one command, then use inject multiple
    // times.
    Logger::instance().ai("Inside AICommandInjector::inject_batch");
    return false;
}

void AICommandInjector::inject(int game_id, char ai_player,
                               const std::string& cmdline)
{
    Logger::instance().ai(std::format(
           "Enqueue: {}", cmdline));

    // Create AI task - DO NOT EXECUTE, just enqueue
    Task* pTask = new Task(game_id, ai_player, cmdline, 0);

    // Enqueue for TaskRunner to execute
    TaskRunner::instance().push(pTask);

    // Fire and forget - no return value, no waiting
}

int AICommandInjector::get_ai_user_id()
{
    // AI user ID - used by TaskRunner when executing AI tasks
    return 3;
}
