//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "srvmgr.h"

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "comms.h"

std::string ServerManager::listen = "127.0.0.1";
unsigned short ServerManager::port = 8080;
int ServerManager::server_socket = -1;

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
    while (1)
    {
        sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int fd = ::accept(server_socket, (sockaddr*)&cli, &clen);
        if (fd < 0)
        {
            continue;
        }

        // Fish on
        HttpRequest req = http_parse(fd);
        HttpResponse resp;

        try
        {
            // Do something with this request
            dispatch_request((const HttpRequest*)&req, (HttpResponse*)&resp);
        }
        catch (const std::exception& ex)
        {
            // BUGBUG  magic number
            resp.status = 500;
            resp.body = json_error(std::string("server error: ") + ex.what());
        }

        // After forming a reply, send it over the socket.
        std::string out = http_serialize(resp);
        ::send(fd, out.c_str(), out.size(), 0);
        ::close(fd);
    }
}
