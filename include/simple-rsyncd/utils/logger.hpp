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

#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <chrono>

namespace simple_rsyncd {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

enum class LogFormat {
    TEXT,
    JSON
};

/**
 * @brief Log entry structure for structured logging
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::string component;
    std::map<std::string, std::string> fields;

    std::string toJSON() const;
    std::string toText() const;
};

/**
 * @brief Enhanced logger with rotation and structured logging
 */
class Logger {
public:
    Logger();
    ~Logger();

    void setLevel(LogLevel level);
    LogLevel getLevel() const;

    void setFormat(LogFormat format);
    LogFormat getFormat() const;

    void setFile(const std::string& filename);
    void setMaxFileSize(size_t max_size);
    void setMaxFiles(size_t max_files);
    void setCompressOldLogs(bool compress);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    // Structured logging methods
    void log(LogLevel level, const std::string& message,
             const std::string& component = "",
             const std::map<std::string, std::string>& fields = {});

    // Filtering
    void addFilter(const std::string& pattern);
    void removeFilter(const std::string& pattern);
    void clearFilters();

    // Rotation
    void rotateLog();

private:
    void logEntry(const LogEntry& entry);
    void checkRotation();
    std::string getLogFilename(size_t index) const;
    void compressLog(const std::string& filename) const;
    bool shouldLog(const std::string& message) const;

    LogLevel level_;
    LogFormat format_;
    std::string log_file_;
    std::unique_ptr<std::ofstream> log_stream_;
    size_t max_file_size_;
    size_t max_files_;
    bool compress_old_logs_;
    size_t current_file_size_;
    std::vector<std::string> filters_;
    mutable std::mutex log_mutex_;
    bool console_output_;
};

} // namespace simple_rsyncd
