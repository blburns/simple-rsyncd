#include "simple_rsyncd/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace simple_rsyncd {

Logger::Logger() : level_(LogLevel::INFO) {
}

Logger::~Logger() = default;

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

LogLevel Logger::getLevel() const {
    return level_;
}

void Logger::debug(const std::string& message) {
    if (level_ <= LogLevel::DEBUG) {
        log("DEBUG", message);
    }
}

void Logger::info(const std::string& message) {
    if (level_ <= LogLevel::INFO) {
        log("INFO", message);
    }
}

void Logger::warn(const std::string& message) {
    if (level_ <= LogLevel::WARN) {
        log("WARN", message);
    }
}

void Logger::error(const std::string& message) {
    if (level_ <= LogLevel::ERROR) {
        log("ERROR", message);
    }
}

void Logger::fatal(const std::string& message) {
    if (level_ <= LogLevel::FATAL) {
        log("FATAL", message);
    }
}

void Logger::log(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    std::cout << "[" << ss.str() << "] [" << level << "] " << message << std::endl;
}

} // namespace simple_rsyncd
