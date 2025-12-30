//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __JSON_H__
#define __JSON_H__

#include <string>

std::string json_escape(const std::string &s);
std::string json_error(const std::string &msg);
std::string json_get_string(const std::string &body, const std::string &key);

#endif
