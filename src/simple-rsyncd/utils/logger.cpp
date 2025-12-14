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
#include <filesystem>
#include <algorithm>
#include <regex>

namespace simple_rsyncd {

// LogEntry implementation
std::string LogEntry::toJSON() const {
    std::stringstream ss;
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto tm = *std::localtime(&time_t);
    
    ss << "{";
    ss << "\"timestamp\":\"" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\",";
    
    std::string level_str;
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO: level_str = "INFO"; break;
        case LogLevel::WARN: level_str = "WARN"; break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
        case LogLevel::FATAL: level_str = "FATAL"; break;
    }
    ss << "\"level\":\"" << level_str << "\",";
    ss << "\"message\":\"" << message << "\"";
    
    if (!component.empty()) {
        ss << ",\"component\":\"" << component << "\"";
    }
    
    if (!fields.empty()) {
        ss << ",\"fields\":{";
        bool first = true;
        for (const auto& [key, value] : fields) {
            if (!first) ss << ",";
            ss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        ss << "}";
    }
    
    ss << "}";
    return ss.str();
}

std::string LogEntry::toText() const {
    std::stringstream ss;
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto tm = *std::localtime(&time_t);
    
    ss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
    
    std::string level_str;
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO: level_str = "INFO"; break;
        case LogLevel::WARN: level_str = "WARN"; break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
        case LogLevel::FATAL: level_str = "FATAL"; break;
    }
    ss << " [" << level_str << "]";
    
    if (!component.empty()) {
        ss << " [" << component << "]";
    }
    
    ss << " " << message;
    
    if (!fields.empty()) {
        ss << " {";
        bool first = true;
        for (const auto& [key, value] : fields) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << "}";
    }
    
    return ss.str();
}

// Logger implementation
Logger::Logger() 
    : level_(LogLevel::INFO)
    , format_(LogFormat::TEXT)
    , max_file_size_(10 * 1024 * 1024) // 10MB
    , max_files_(5)
    , compress_old_logs_(true)
    , current_file_size_(0)
    , console_output_(true) {
}

Logger::~Logger() {
    if (log_stream_ && log_stream_->is_open()) {
        log_stream_->close();
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    level_ = level;
}

LogLevel Logger::getLevel() const {
    return level_;
}

void Logger::setFormat(LogFormat format) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    format_ = format;
}

LogFormat Logger::getFormat() const {
    return format_;
}

void Logger::setFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_file_ = filename;
    if (log_stream_ && log_stream_->is_open()) {
        log_stream_->close();
    }
    if (!filename.empty()) {
        log_stream_ = std::make_unique<std::ofstream>(filename, std::ios::app);
        if (log_stream_->is_open()) {
            log_stream_->seekp(0, std::ios::end);
            current_file_size_ = log_stream_->tellp();
        }
    }
}

void Logger::setMaxFileSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    max_file_size_ = max_size;
}

void Logger::setMaxFiles(size_t max_files) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    max_files_ = max_files;
}

void Logger::setCompressOldLogs(bool compress) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    compress_old_logs_ = compress;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& component,
                 const std::map<std::string, std::string>& fields) {
    if (level < level_) {
        return;
    }
    
    if (!shouldLog(message)) {
        return;
    }
    
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.message = message;
    entry.component = component;
    entry.fields = fields;
    
    logEntry(entry);
}

void Logger::addFilter(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    filters_.push_back(pattern);
}

void Logger::removeFilter(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    filters_.erase(std::remove(filters_.begin(), filters_.end(), pattern), filters_.end());
}

void Logger::clearFilters() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    filters_.clear();
}

void Logger::rotateLog() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (log_file_.empty() || !log_stream_ || !log_stream_->is_open()) {
        return;
    }
    
    log_stream_->close();
    
    // Rotate existing logs
    for (size_t i = max_files_ - 1; i > 0; i--) {
        std::string old_file = getLogFilename(i - 1);
        std::string new_file = getLogFilename(i);
        
        if (std::filesystem::exists(old_file)) {
            if (i == max_files_ - 1 && compress_old_logs_) {
                compressLog(old_file);
            } else {
                std::filesystem::rename(old_file, new_file);
            }
        }
    }
    
    // Rename current log
    std::string current_log = getLogFilename(0);
    if (std::filesystem::exists(log_file_)) {
        std::filesystem::rename(log_file_, current_log);
    }
    
    // Open new log file
    log_stream_ = std::make_unique<std::ofstream>(log_file_, std::ios::trunc);
    current_file_size_ = 0;
}

void Logger::logEntry(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    std::string log_line;
    if (format_ == LogFormat::JSON) {
        log_line = entry.toJSON();
    } else {
        log_line = entry.toText();
    }
    log_line += "\n";
    
    // Console output
    if (console_output_) {
        std::cout << log_line;
    }
    
    // File output
    if (log_stream_ && log_stream_->is_open()) {
        *log_stream_ << log_line;
        log_stream_->flush();
        current_file_size_ += log_line.length();
        
        // Check if rotation is needed
        if (current_file_size_ >= max_file_size_) {
            checkRotation();
        }
    }
}

void Logger::checkRotation() {
    if (current_file_size_ >= max_file_size_) {
        rotateLog();
    }
}

std::string Logger::getLogFilename(size_t index) const {
    if (log_file_.empty()) {
        return "";
    }
    
    std::filesystem::path path(log_file_);
    std::string stem = path.stem().string();
    std::string ext = path.extension().string();
    std::string dir = path.parent_path().string();
    
    std::string rotated_file = stem + "." + std::to_string(index) + ext;
    if (!dir.empty()) {
        return dir + "/" + rotated_file;
    }
    return rotated_file;
}

void Logger::compressLog(const std::string& filename) const {
    // Compression would require zlib or similar library
    // For now, just mark it as compressed by renaming
    if (std::filesystem::exists(filename)) {
        std::filesystem::path path(filename);
        std::string compressed_name = path.string() + ".gz";
        // In a real implementation, we would compress the file here
        // For now, just rename it to indicate it should be compressed
        std::filesystem::rename(filename, compressed_name);
    }
}

bool Logger::shouldLog(const std::string& message) const {
    if (filters_.empty()) {
        return true;
    }
    
    for (const auto& pattern : filters_) {
        try {
            std::regex re(pattern);
            if (std::regex_search(message, re)) {
                return false; // Filter matched, don't log
            }
        } catch (...) {
            // Invalid regex, skip this filter
        }
    }
    
    return true;
}

} // namespace simple_rsyncd
