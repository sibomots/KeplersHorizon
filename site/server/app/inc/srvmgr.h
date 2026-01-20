//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __SERVER_MANAGER_H__
#define __SERVER_MANAGER_H__

#include "app.h"
#include "configr.h"
#include "typedefs.h"

class ServerManager
{
  public:
    static ServerManager& getInstance()
    {
        // The static instance is created upon the first call to this function.
        static ServerManager instance;
        return instance;
    }

    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;
    ServerManager(ServerManager&&) noexcept = delete;
    ServerManager& operator=(ServerManager&&) noexcept = delete;

    bool socket_invalid()
    {
        if (server_socket < 0)
        {
            throw std::runtime_error("ServerManager socket invalid");
        }
        return false;
    }

    void connect();

    void configure()
    {
        port = Configr::instance().get<Key::port>();
    }

    void run(void);

    static std::string listen;
    static unsigned short port;
    static int server_socket;

  private:
    ServerManager()
    {
    }

    ~ServerManager()
    {
        if (server_socket > 0)
        {
            close(server_socket);
            server_socket = -1;
        }
    }
};

#endif
