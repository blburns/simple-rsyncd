/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simple-rsyncd/utils/logger.hpp"
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
