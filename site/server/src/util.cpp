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

char owner_for_username(const std::string &u)
{
    std::string lu = u;
    for (size_t i = 0; i < lu.size(); ++i)
        lu[i] = (char)std::tolower((unsigned char)lu[i]);
    if (lu == "alice")
        return 'A';
    if (lu == "bob")
        return 'B';
    return 'A';
}
std::string now_iso()
{
    char buf[32];
    std::time_t t = std::time(NULL);
    std::tm tmv;
    gmtime_r(&t, &tmv);
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return std::string(buf);
}

std::string trim(const std::string &s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
        a++;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
        b--;
    return s.substr(a, b - a);
}

bool starts_with(const std::string &s, const std::string &p)
{
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

std::string to_lower(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] =
            static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    return s;
}

std::string rand_hex_64()
{
    static const char *hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (int i = 0; i < 64; i++)
        out.push_back(hex[std::rand() % 16]);
    return out;
}

std::vector<std::string> split_ws(const std::string &s)
{
    std::istringstream is(s);
    std::vector<std::string> out;
    std::string tok;
    while (is >> tok)
        out.push_back(tok);
    return out;
}
