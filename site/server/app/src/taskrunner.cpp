#include "taskrunner.h"
#include "cmd.h"

void TaskRunner::processItem(Task* item)
{
    bool done = false;

    try
    {
        if (item->is_ai_task)
        {
            // AI task - execute command through parser
            Logger::instance().info("[TASK] Processing AI task: " +
                                    item->ai_command);

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
                    "[AI] Command failed: " + item->ai_command +
                    " | Error: " + errmsg);
            }
            else
            {
                Logger::instance().info("[AI] ✓ " + item->ai_command);
            }
        }
        else
        {
            // User HTTP task - existing code
            Logger::instance().info("[TASK] Processing user HTTP task");
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
        if (pollAgentCallback_)
        {
            Task* newItem = nullptr;
            if (pollAgentCallback_(&newItem) && newItem != nullptr)
            {
                push(newItem);
            }
        }
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
            SafeDelete(item);
        }
    }
}
