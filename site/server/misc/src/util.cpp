//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "util.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

#include "db.h"

std::string join_vector(const std::vector<int>& v, const std::string delim)
{
    std::stringstream ss;
    for(size_t i  = 0 ; i < v.size(); ++i) {
          ss << std::to_string(v[i]);
          if ( i < v.size() - 1) {
             ss << delim;
          }
    }
    return ss.str();
}
    
std::string trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start))
    {
        start++;
    }

    auto end = s.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

#if 0
// Dynamic seat lookup - checks game_seats table for the user's seat in a game
// Falls back to legacy alice=A, bob=B for backward compatibility
char owner_for_username(const std::string& u)
{
    // Legacy fallback for hardcoded test users
    std::string lower = to_lower(u);
    if (lower == "alice")
        return 'A';
    if (lower == "bob")
        return 'B';
    return 0;
}
#endif

// Dynamic seat lookup - returns 'A' or 'B' based on game_seats table, or 0 if
// not in game
char seat_for_user(int game_id, int user_id)
{
    if (game_id == 0 || user_id == 0)
        return 0;

    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT seat FROM game_seats WHERE game_id=" + std::to_string(game_id) +
        " AND user_id=" + std::to_string(user_id) + " LIMIT 1");
    if (!rows.empty() && !rows[0].empty() && !rows[0][0].empty())
    {
        return rows[0][0][0]; // Return 'A' or 'B'
    }
    return 0;
}

std::string now_iso()
{
    std::time_t t = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return std::string(buf);
}

bool starts_with(const std::string& s, const std::string& p)
{
    return s.rfind(p, 0) == 0;
}

std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string rand_hex_64()
{
    static const char hex[] = "0123456789abcdef";
    std::string s;
    s.reserve(64);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    for (int i = 0; i < 64; ++i)
    {
        s += hex[dis(gen)];
    }
    return s;
}

std::vector<std::string> split_ws(const std::string& s)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (ss >> item)
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string& s, char delim)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::string upper_ascii(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        r[i] = (char)std::toupper((unsigned char)r[i]);
    return r;
}

