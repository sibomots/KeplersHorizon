#include "logger.h"
#include "util.h"

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::info(const std::string& msg) {
    log("INFO", msg);
}

void Logger::error(const std::string& msg) {
    log("ERROR", msg);
}

void Logger::log(const std::string& level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // [TIMESTAMP] [LEVEL] Message
    std::cout << "[" << now_iso() << "] ";
    if (level != "INFO" && !level.empty()) {
        std::cout << "[" << level << "] ";
    }
    std::cout << msg << std::endl;
}
