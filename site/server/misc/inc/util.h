//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <vector>

std::string trim(const std::string& s);
char seat_for_user(int game_id,
                   int user_id); // Dynamic seat lookup from game_seats table
std::string now_iso();
bool starts_with(const std::string& s, const std::string& p);
std::string to_lower(std::string s);
std::string rand_hex_64();
std::vector<std::string> split_ws(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
std::string upper_ascii(const std::string& s);
std::string join_vector(const std::vector<int>& v, const std::string delim);
#endif
