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
#include "json.h"

#include "app.h"

std::string json_escape(const std::string &s)
{
    std::ostringstream o;
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c)
        {
        case '\\':
            o << "\\\\";
            break;
        case '"':
            o << "\\\"";
            break;
        case '\b':
            o << "\\b";
            break;
        case '\f':
            o << "\\f";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if (c < 0x20)
            {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                o << buf;
            }
            else
            {
                o << s[i];
            }
        }
    }
    return o.str();
}

// Minimal JSON field extractor for string fields only: {"k":"v"}.
// Not a general JSON parser. Good enough for controlled client.
std::string json_get_string(const std::string &body, const std::string &key)
{
    const std::string pat = "\"" + key + "\"";
    size_t p = body.find(pat);
    if (p == std::string::npos)
        return "";
    p = body.find(':', p + pat.size());
    if (p == std::string::npos)
        return "";
    p++;
    while (p < body.size() && std::isspace(static_cast<unsigned char>(body[p])))
        p++;
    if (p >= body.size() || body[p] != '"')
        return "";
    p++;
    std::ostringstream val;
    while (p < body.size())
    {
        char c = body[p++];
        if (c == '"')
            break;
        if (c == '\\' && p < body.size())
        {
            char e = body[p++];
            switch (e)
            {
            case '"':
                val << '"';
                break;
            case '\\':
                val << '\\';
                break;
            case '/':
                val << '/';
                break;
            case 'b':
                val << '\b';
                break;
            case 'f':
                val << '\f';
                break;
            case 'n':
                val << '\n';
                break;
            case 'r':
                val << '\r';
                break;
            case 't':
                val << '\t';
                break;
            default:
                val << e;
                break;
            }
        }
        else
        {
            val << c;
        }
    }
    return val.str();
}

std::string json_error(const std::string &msg)
{
    return std::string("{\"ok\":false,\"error\":\"") + json_escape(msg) + "\"}";
}
