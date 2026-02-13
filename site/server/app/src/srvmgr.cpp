///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "srvmgr.h"

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
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "aiagent.h"
#include "autonomy_agency.h"
#include "comms.h"
#include "logger.h"
#include "taskrunner.h"
#include "typedefs.h"

std::string ServerManager::listen = "127.0.0.1";
unsigned short ServerManager::port = 8080;
int ServerManager::server_socket = -1;
std::atomic<bool> ServerManager::shutdown_requested(false);

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

void ServerManager::request_shutdown()
{
    shutdown_requested.store(true);
    // Close socket to unblock accept()
    if (server_socket >= 0)
    {
        ::shutdown(server_socket, SHUT_RDWR);
    }
}

void ServerManager::run(void)
{
    int sequence = 0;

    // TaskRunner is the single consumer of Tasks
    // Producers: HTTP thread (user commands), AI thread (via inject())
    // TaskRunner executes commands and pumps AI when it still has initiative
    TaskRunner::instance().start();

    // Now that the TaskRunner is running, we can now listen
    // for commands from REST endpoints

    while (!shutdown_requested.load())
    {
        // Use select() with timeout so we can check shutdown flag
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ready = select(server_socket + 1, &readfds, NULL, NULL, &tv);
        if (ready <= 0)
        {
            continue;
        }

        sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int fd = ::accept(server_socket, (sockaddr*)&cli, &clen);
        if (fd < 0)
        {
            continue;
        }

        // Fish on
        HttpRequest* preq = http_parse(fd);
        HttpResponse* presp = new HttpResponse;
        Task* task = new Task("User", ++sequence, preq, presp, fd);
        TaskRunner::instance().push(task);
    }

    // Graceful shutdown
    Logger::instance().debug("[SRV] Shutdown requested, cleaning up...");
    TaskRunner::instance().stop();
    AutonomyAgency::instance().stop();
    AutonomyAgency::instance().join();
    Logger::instance().debug("[SRV] Shutdown complete");
}
