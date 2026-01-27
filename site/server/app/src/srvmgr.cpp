//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <arpa/inet.h>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <random>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "comms.h"
#include "srvmgr.h"
#include "typedefs.h"

std::string ServerManager::listen = "127.0.0.1";
unsigned short ServerManager::port = 8080;
int ServerManager::server_socket = -1;

typedef struct Task
{
    HttpRequest* phtreq;
    HttpResponse* phtresp;
    int sock;
    int seq;
    std::string on_behalf;

    Task()
        : seq(0), on_behalf("unknown"), phtreq(nullptr), phtresp(nullptr),
          sock(-1)
    {
    }

    Task(std::string nam, int n, HttpRequest* preq, HttpResponse* presp, int fd)
        : on_behalf(nam), seq(n), phtreq(preq), phtresp(presp), sock(fd)
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
    }
    void deep_move(Task* pdst, Task& src)
    {
        pdst->sock = src.sock;
        pdst->phtreq = src.phtreq;
        pdst->phtresp = src.phtresp;
        pdst->on_behalf = std::string(src.on_behalf);
        pdst->seq = src.seq;
        src.on_behalf.clear();
        src.seq = 0;
        src.sock = -1; // BUGBUG  we got the fd, but we don't want the other
                       // to have it.
        SafeDelete(src.phtreq);
        SafeDelete(src.phtresp);
    }
} Task;

class TaskRunner
{
  private:
    std::queue<Task*> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;

    // This is the agent callback.  We use this callback
    // to ask the AI Agency for any new task to perform
    std::function<bool(Task**)> pollAgentCallback_;

    void runLoop()
    {
        while (running_)
        {
#if 0
            if (pollAgentCallback_)
            {
                Task* newItem = nullptr;
                if (pollAgentCallback_(&newItem))
                {
                    push(newItem); // std::move(newItem));
                }
            }
#endif
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
                processItem(item);
                SafeDelete(item);
            }
        }
    }
    void processItem(Task* item)
    {
        bool done = false;
        try
        {
            done = dispatch_request(item->phtreq, item->phtresp);
        }
        catch (const std::exception& ex)
        {
            item->phtresp->status = 500;
            item->phtresp->body =
                 json_error(std::string("server error: ") + ex.what());
        }

        std::string out = http_serialize(item->phtresp);
        ::send(item->sock, out.c_str(), out.size(), 0);
    }

  public:
    TaskRunner() = default;
    ~TaskRunner()
    {
        stop();
    }

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
};

TaskRunner aq;

void ServerManager::connect()
{
    server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        throw std::runtime_error("socket failed");
    }

    int one = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = inet_addr(listen.c_str());
    if (::bind(server_socket, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        throw std::runtime_error(std::string("bind failed: ") +
                                 std::strerror(errno));
    }
    if (::listen(server_socket, 16) < 0)
    {
        throw std::runtime_error("listen failed");
    }
}

void ServerManager::run(void)
{
    int sequence = 0;
    bool done = false;

    auto agentCallback = [](Task** ppNewItem) -> bool
    {
        // go fetch any pending task from the AI Agent.
        // if there is one, populate newItem
        // Here is where we are going to POLL the AI AGENT
        // The AI Agent is going to PUSH new Tasks
        // on the TaskRunner queue.

        // Whenever advance_next() is called,
        // The AI AGENT will have the opportunity to
        // Add tasks to the TaskRunner Queue.

        // This call back is here for two reason:
        //  In the case where the user is the active player
        //  their commands can change the dynamics of the game
        //  and thus, the AI agent has the opportunity to
        //  make responses -- and the opportunity for injecting
        //  commands from the AI Agent occurs when the advance_next
        //  method is called -- meaning only on phase changes
        //  of the user-player.

        // Second, when the AI-Agent is the active player, it
        //  "injects" commands to execute by likewise offering
        //  new tasks to the Queue.

        // The AI-Agent is in a "Command/Response" mode then
        // at the turn phase boundary when the User is the
        // Active Player (again, advance_next() is the place
        // for the AI-Agent to prepare new Tasks to offer here.

        // When the user-player is NOT the active player
        // The Task queue will be LIKELY empty unless a new Task
        // is inserted by the user.
        //  But there is no guarentee..
        //  For the AI-Agent to then marshall new Tasks
        //  it has to do so by stepping through the turn phases
        //   when it is the active player.
        //  That insertion can only happen in that
        // case when
        //   1) the AI_Agent is the active player
        //   2) during the AI_Agent evolution of advance_turn()
        // Doing so does not prohibit the user-player from
        //  running commands.  Both players's Tasks
        //  rendevous in the thread handler:  runLoop

        // We are literally asking the AI-Agent to devise a new
        // Task -- and then we add it here to the queue.
        return true;
    };

    aq.start(agentCallback);

    // Now that the TaskRunner is running, we can now listen
    // for commands from REST endpoints

    while (!done)
    {
        sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int fd = ::accept(server_socket, (sockaddr*)&cli, &clen);
        if (fd < 0)
        {
            continue;
        }
        else
        {
            // Fish on
            HttpRequest* preq = http_parse(fd);
            HttpResponse* presp = new HttpResponse;
            Task* task = new Task("User", ++sequence, preq, presp, fd);
            aq.push(task);
        }
    }
    aq.stop();
}
