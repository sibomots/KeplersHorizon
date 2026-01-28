#ifndef __KH_TASK_RUNNER_H__
#define __KH_TASK_RUNNER_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include "aiagent.h"
#include "comms.h"
#include "srvmgr.h"
#include "typedefs.h"
#include "logger.h"

typedef struct Task
{
    HttpRequest* phtreq;
    HttpResponse* phtresp;
    int sock;
    int seq;
    std::string on_behalf;

    // NEW: AI-specific fields
    bool is_ai_task;        // true if from AI, false if from user HTTP
    int game_id;            // game context (for AI tasks)
    char player;            // 'A' or 'B' (for AI tasks)
    std::string ai_command; // command string (for AI tasks)

    // Default constructor
    Task()
        : seq(0), on_behalf("unknown"), phtreq(nullptr), phtresp(nullptr),
          sock(-1), is_ai_task(false), game_id(0), player('\0'), ai_command("")
    {
    }

    // Constructor for AI tasks (no HTTP involved)
    Task(int gid, char pl, const std::string& cmd, int n)
        : seq(n), on_behalf("AI_AGENT"), phtreq(nullptr), phtresp(nullptr),
          sock(-1), is_ai_task(true), game_id(gid), player(pl), ai_command(cmd)
    {
    }

    // Constructor for user HTTP tasks
    Task(std::string nam, int n, HttpRequest* preq, HttpResponse* presp, int fd)
        : on_behalf(nam), seq(n), phtreq(preq), phtresp(presp), sock(fd),
          is_ai_task(false), game_id(0), player('\0'), ai_command("")
    {
    }

    virtual ~Task()
    {
        SafeDelete(phtreq);
        SafeDelete(phtresp);
        if (sock != -1)
        {
            ::close(sock);
            sock = -1;
        }
    }

    Task(const Task& other)
    {
        deep_copy(this, other);
    }

    Task& operator=(const Task& other)
    {
        if (this != &other)
        {
            deep_copy(this, other);
        }
        return *this;
    }

    Task(Task&& other) noexcept
    {
        deep_move(this, other);
    }

    Task& operator=(Task&& other) noexcept
    {
        if (this != &other)
        {
            deep_move(this, other);
        }
        return *this;
    }

  private:
    void deep_copy(Task* pdst, const Task& src)
    {
        pdst->sock = src.sock;
        pdst->phtreq = src.phtreq;
        pdst->phtresp = src.phtresp;
        pdst->on_behalf = std::string(src.on_behalf);
        pdst->seq = src.seq;
        pdst->is_ai_task = src.is_ai_task;
        pdst->game_id = src.game_id;
        pdst->player = src.player;
        pdst->ai_command = src.ai_command;
    }

    void deep_move(Task* pdst, Task& src)
    {
        pdst->sock = src.sock;
        pdst->phtreq = src.phtreq;
        pdst->phtresp = src.phtresp;
        pdst->on_behalf = std::string(src.on_behalf);
        pdst->seq = src.seq;
        pdst->is_ai_task = src.is_ai_task;
        pdst->game_id = src.game_id;
        pdst->player = src.player;
        pdst->ai_command = src.ai_command;

        src.on_behalf.clear();
        src.seq = 0;
        src.sock = -1;
        src.is_ai_task = false;
        src.game_id = 0;
        src.player = '\0';
        src.ai_command.clear();

        SafeDelete(src.phtreq);
        SafeDelete(src.phtresp);
    }

} Task;

class TaskRunner {
public:
    static TaskRunner& instance() {
        static TaskRunner _instance; 
        return _instance;
    }


    void processItem(Task* item);
    void runLoop();

    // Anyone can push a task onto the queue
    void push(Task* item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item); // std::move(item));
        }
        cv_.notify_one();
    }

    void start(std::function<bool(Task**)> pollCallback)
    {
        pollAgentCallback_ = std::move(pollCallback);
        running_ = true;
        workerThread_ = std::thread(&TaskRunner::runLoop, this);
    }

    void stop()
    {
        running_ = false;
        cv_.notify_all();
        if (workerThread_.joinable())
        {
            workerThread_.join();
        }
    }

private:
    // Private constructor, destructor, copy constructor, and assignment operator
    // to prevent external instantiation, copying, or assignment
    TaskRunner() = default;
    ~TaskRunner()
    {
        stop();
    }
    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;
    TaskRunner(TaskRunner&&) = delete;
    TaskRunner& operator=(TaskRunner&&) = delete;
  
    std::queue<Task*> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;

    // This is the agent callback.  We use this callback
    // to ask the AI Agency for any new task to perform
    std::function<bool(Task**)> pollAgentCallback_;
};

#endif

