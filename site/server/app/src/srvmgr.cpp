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

#include "aiagent.h"
#include "comms.h"
#include "srvmgr.h"
#include "typedefs.h"
#include "taskrunner.h"

std::string ServerManager::listen = "127.0.0.1";
unsigned short ServerManager::port = 8080;
int ServerManager::server_socket = -1;

// BUGBUG
extern int internal_command_handler_body(const std::string cmdline, std::string& errmsg);

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
        return AIAgent::instance().next_task(ppNewItem); 
    };

    TaskRunner::instance().start(agentCallback);

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
            TaskRunner::instance().push(task);
        }
    }
    TaskRunner::instance().stop();
}
