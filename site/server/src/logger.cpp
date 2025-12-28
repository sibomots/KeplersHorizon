#include "logger.h"
#include "util.h" // for now_iso()

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
    // However, user asked for: [timestamp] [Owner] cmd -> msg
    // So if the message itself contains the [Owner] part, we just prepend timestamp.
    // If level is empty or "INFO", maybe we just print timestamp?
    // Let's stick to the requested format: [ISO] Message
    
    // If level is provided and not INFO/special, maybe print it?
    // User request was specific about the command log format.
    // "1. timestamp 2. literal command 3. literal copy..."
    
    std::cout << "[" << now_iso() << "] ";
    if (level != "INFO" && !level.empty()) {
        std::cout << "[" << level << "] ";
    }
    std::cout << msg << std::endl;
}
