///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "taskrunner.h"

#include <format>

#include "autonomy_agency.h"
#include "cmd.h"

void TaskRunner::processItem(Task* item)
{
    bool done = false;

    try
    {
        if (item->is_ai_task)
        {
            // AI task - execute command through parser
            Logger::instance().info(std::format("[AI-TASK] {}", item->ai_command));

            // Set StateMachine context
            StateMachine::instance().set_game_id(item->game_id);
            StateMachine::instance().set_current_player(item->player);
            StateMachine::instance().set_current_user_id(3); // AI user

            // Execute command through parser
            std::string errmsg;
            int result =
                internal_command_handler_body(item->ai_command, errmsg);

            if (result != 0)
            {
                Logger::instance().error(
                    std::format("[AI] FAILED CMD: {} | Error: {}",
                                item->ai_command, errmsg));
            }
            else
            {
                Logger::instance().ai(
                    std::format("PASSED CMD: {} ", item->ai_command));
            }

            // Signal AI thread that task is complete
            AutonomyAgency::instance().notify_task_complete();
        }
        else
        {
            // User HTTP task
            done = dispatch_request(item->phtreq, item->phtresp);
            std::string out = http_serialize(item->phtresp);
            ::send(item->sock, out.c_str(), out.size(), 0);
        }
    }
    catch (const std::exception& ex)
    {
        if (item->is_ai_task)
        {
            Logger::instance().error("[AI] Exception: " +
                                     std::string(ex.what()));
        }
        else
        {
            item->phtresp->status = 500;
            item->phtresp->body =
                json_error(std::string("server error: ") + ex.what());
            std::string out = http_serialize(item->phtresp);
            ::send(item->sock, out.c_str(), out.size(), 0);
        }
    }
}

void TaskRunner::runLoop()
{
    MySqlThreadGuard mysql_guard;

    while (running_)
    {
        Task* item = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

            if (!running_ && queue_.empty())
            {
                break;
            }

            if (queue_.empty())
            {
                continue;
            }

            item = queue_.front();
            queue_.pop();
        }

        if (item != nullptr)
        {
            processItem(item);

            // After any task: check if AI has initiative and should be pumped
            // This handles both:
            //   1. AI tasks where AI continues to next phase
            //   2. HTTP tasks where user yields turn to AI
            int game_id = item->is_ai_task
                              ? item->game_id
                              : StateMachine::instance().get_game_id();
            if (game_id > 0)
            {
                GameState s = StateMachine::instance().load_game(game_id);
                if (StateMachine::instance().is_ai_player(s.active_player) &&
                    !s.game_over)
                {
                    // Configure AA if not yet configured for this game
                    AutonomyAgency::instance().configure(game_id,
                                                         s.active_player[0]);
                    if (!AutonomyAgency::instance().is_running())
                    {
                        AutonomyAgency::instance().start();
                    }
                    AutonomyAgency::instance().pump();
                }
            }

            SafeDelete(item);
        }
    }
}
