///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// 
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "app.h"
#include "args.h"
#include "comms.h"
#include "db.h"
#include "util.h"
#include <iostream>

int main(int argc, char **argv)
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    Args args;

    try
    {
        args = parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "arg error: %s\n", e.what());
        return 2;
    }

    try
    {
        Db db;
        db.connect(args.dbhost, args.dbuser, args.dbpass, args.dbname);

        int srv = ::socket(AF_INET, SOCK_STREAM, 0);
        if (srv < 0)
            throw std::runtime_error("socket failed");

        int one = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(args.port));
        addr.sin_addr.s_addr = inet_addr(args.listen.c_str());

        if (::bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            throw std::runtime_error(std::string("bind failed: ") +
                                     std::strerror(errno));
        }
        if (::listen(srv, 16) < 0)
            throw std::runtime_error("listen failed");

        std::fprintf(stderr, "[%s] Kepler's Horizon_server listening on %s:%d\n",
                     now_iso().c_str(), args.listen.c_str(), args.port);

        while (true)
        {
            sockaddr_in cli;
            socklen_t clen = sizeof(cli);
            int fd = ::accept(srv, (sockaddr *)&cli, &clen);
            if (fd < 0)
                continue;

            HttpRequest req = http_parse(fd);
            HttpResponse resp;

            try
            {
                dispatch_request((const HttpRequest *)&req, &db,
                                 (HttpResponse *)&resp);
            }
            catch (const std::exception &e)
            {
                resp.status = 500;
                resp.body =
                    json_error(std::string("server error: ") + e.what());
            }

            std::string out = http_serialize(resp);

            // debug
            // std::cout << "serialized output from dispatch result: " << std::endl
            //          << out.c_str()
            //          << std::endl;

            ::send(fd, out.c_str(), out.size(), 0);
            ::close(fd);
        }
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }

    return 0;
}
