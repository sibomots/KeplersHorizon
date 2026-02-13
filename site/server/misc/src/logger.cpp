///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "configr.h"
#include "logger.h"
#include "util.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <ranges>
#include <vector>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::info(const std::string& msg)
{
    log("", msg);
}

void Logger::error(const std::string& msg)
{
    log("ERROR", msg);
}

void Logger::debug(const std::string& msg)
{
    log("DEBUG", msg);
}

void Logger::user(const std::string& msg)
{
    log("USER", msg);
}

void Logger::ai(const std::string& msg)
{
    log("AI", msg);
}

void Logger::log(const std::string& level, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string log_file = Configr::instance().get<Key::log>();
    if (log_file.empty())
    {
        log_file = "kh.log";
    }

    std::ofstream os(log_file, std::ios::app);
    if (os.is_open())
    {
        bool has_level = !level.empty();

        // Helper function (lambda) to convert a subrange to a std::string in
        // C++20
        auto to_string = [](auto&& rng)
        { return std::string(rng.begin(), rng.end()); };

        // Use ranges to split the string and transform the results
        auto lines_view =
            msg | std::views::split('\n') | std::views::transform(to_string);

        // Convert the resulting view into a std::vector<std::string>
        std::vector<std::string> lines(lines_view.begin(), lines_view.end());

        for (const auto& line : lines)
        {
            os << "[" << now_iso() << "] ";
            if (has_level)
            {
                os << "[" << level << "] ";
            }
            os << line << std::endl;
        }
    }
}
