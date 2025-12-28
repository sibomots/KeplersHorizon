#pragma once
#include <string>
#include <iostream>
#include <mutex>

class Logger {
public:
    static Logger& instance();

    void info(const std::string& msg);
    void error(const std::string& msg);
    
    // Helper to format "[TAG] msg"
    void log(const std::string& level, const std::string& msg);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex m_mutex;
};
