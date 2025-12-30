//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <iostream>
#include <mutex>
#include <string>

class Logger
{
  public:
    static Logger &instance();

    void info(const std::string &msg);
    void error(const std::string &msg);
    void debug(const std::string &msg);

    // Helper to format "[TAG] msg"
    void log(const std::string &level, const std::string &msg);

  private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::mutex m_mutex;
};

#endif
