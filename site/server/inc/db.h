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
#ifndef __DB_H__
#define __DB_H__

#include "app.h"
#include "db.h"
#include "typs.h"

class Db
{
  public:
    MYSQL *c = NULL;

    void connect(const std::string &host, const std::string &user,
                 const std::string &pass, const std::string &dbname)
    {
        c = mysql_init(NULL);
        if (!c)
            throw std::runtime_error("mysql_init failed");
        if (!mysql_real_connect(c, host.c_str(), user.c_str(), pass.c_str(),
                                dbname.c_str(), 0, NULL, 0))
        {
            throw std::runtime_error(
                std::string("mysql_real_connect failed: ") + mysql_error(c));
        }
        mysql_set_character_set(c, "utf8mb4");
    }

    ~Db()
    {
        if (c)
            mysql_close(c);
    }

    void exec(const std::string &q)
    {
        if (mysql_query(c, q.c_str()))
            throw std::runtime_error(std::string("mysql_query: ") +
                                     mysql_error(c));
    }

    std::vector<std::vector<std::string>> query(const std::string &q)
    {
        if (mysql_query(c, q.c_str()))
            throw std::runtime_error(std::string("mysql_query: ") +
                                     mysql_error(c));
        MYSQL_RES *res = mysql_store_result(c);
        if (!res)
            return {};
        int cols = mysql_num_fields(res);
        std::vector<std::vector<std::string>> out;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)))
        {
            unsigned long *lens = mysql_fetch_lengths(res);
            std::vector<std::string> r;
            for (int i = 0; i < cols; i++)
            {
                if (!row[i])
                    r.push_back("");
                else
                    r.push_back(std::string(row[i], row[i] + lens[i]));
            }
            out.push_back(r);
        }
        mysql_free_result(res);
        return out;
    }

    std::string esc(const std::string &s)
    {
        std::string out;
        out.resize(s.size() * 2 + 1);
        unsigned long n =
            mysql_real_escape_string(c, &out[0], s.c_str(), s.size());
        out.resize(n);
        return out;
    }
};

#endif
