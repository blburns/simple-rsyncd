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

#include "simple-rsyncd/core/error.hpp"
#include <sstream>
#include <iomanip>

namespace simple_rsyncd {

RSyncError::RSyncError(ErrorCode code, const std::string& message, ErrorCategory category)
    : code_(code)
    , category_(category)
    , message_(message)
    , timestamp_(std::chrono::system_clock::now()) {
}

RSyncError::RSyncError(ErrorCode code, const std::string& message, const std::string& context, ErrorCategory category)
    : code_(code)
    , category_(category)
    , message_(message)
    , context_(context)
    , timestamp_(std::chrono::system_clock::now()) {
}

ErrorCode RSyncError::getCode() const {
    return code_;
}

ErrorCategory RSyncError::getCategory() const {
    return category_;
}

std::string RSyncError::getMessage() const {
    return message_;
}

std::string RSyncError::getContext() const {
    return context_;
}

std::map<std::string, std::string> RSyncError::getFields() const {
    return fields_;
}

void RSyncError::addField(const std::string& key, const std::string& value) {
    fields_[key] = value;
}

void RSyncError::setContext(const std::string& context) {
    context_ = context;
}

const char* RSyncError::what() const noexcept {
    return message_.c_str();
}

std::string RSyncError::toString() const {
    std::stringstream ss;
    ss << "[" << getCategoryString(category_) << "] ";
    ss << "[" << getErrorCodeString(code_) << "] ";
    ss << message_;
    if (!context_.empty()) {
        ss << " (Context: " << context_ << ")";
    }
    if (!fields_.empty()) {
        ss << " {";
        bool first = true;
        for (const auto& [key, value] : fields_) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << "}";
    }
    return ss.str();
}

std::string RSyncError::toJSON() const {
    std::stringstream ss;
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    auto tm = *std::localtime(&time_t);

    ss << "{";
    ss << "\"timestamp\":\"" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\",";
    ss << "\"code\":" << static_cast<int>(code_) << ",";
    ss << "\"code_string\":\"" << getErrorCodeString(code_) << "\",";
    ss << "\"category\":\"" << getCategoryString(category_) << "\",";
    ss << "\"message\":\"" << message_ << "\"";

    if (!context_.empty()) {
        ss << ",\"context\":\"" << context_ << "\"";
    }

    if (!fields_.empty()) {
        ss << ",\"fields\":{";
        bool first = true;
        for (const auto& [key, value] : fields_) {
            if (!first) ss << ",";
            ss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        ss << "}";
    }

    ss << "}";
    return ss.str();
}

bool RSyncError::isRecoverable() const {
    return isRecoverableError(code_);
}

std::string RSyncError::getRecoverySuggestion() const {
    return getRecoverySuggestionForCode(code_);
}

std::string RSyncError::getErrorCodeString(ErrorCode code) {
    switch (code) {
        case ErrorCode::CONFIG_FILE_NOT_FOUND: return "CONFIG_FILE_NOT_FOUND";
        case ErrorCode::CONFIG_INVALID_FORMAT: return "CONFIG_INVALID_FORMAT";
        case ErrorCode::CONFIG_VALIDATION_FAILED: return "CONFIG_VALIDATION_FAILED";
        case ErrorCode::CONFIG_RELOAD_FAILED: return "CONFIG_RELOAD_FAILED";
        case ErrorCode::AUTH_FAILED: return "AUTH_FAILED";
        case ErrorCode::AUTH_USER_NOT_FOUND: return "AUTH_USER_NOT_FOUND";
        case ErrorCode::AUTH_PASSWORD_INVALID: return "AUTH_PASSWORD_INVALID";
        case ErrorCode::AUTH_ACCOUNT_LOCKED: return "AUTH_ACCOUNT_LOCKED";
        case ErrorCode::AUTH_PASSWORD_EXPIRED: return "AUTH_PASSWORD_EXPIRED";
        case ErrorCode::AUTH_SESSION_EXPIRED: return "AUTH_SESSION_EXPIRED";
        case ErrorCode::AUTHZ_ACCESS_DENIED: return "AUTHZ_ACCESS_DENIED";
        case ErrorCode::AUTHZ_INSUFFICIENT_PERMISSIONS: return "AUTHZ_INSUFFICIENT_PERMISSIONS";
        case ErrorCode::AUTHZ_IP_DENIED: return "AUTHZ_IP_DENIED";
        case ErrorCode::NETWORK_CONNECTION_FAILED: return "NETWORK_CONNECTION_FAILED";
        case ErrorCode::NETWORK_BIND_FAILED: return "NETWORK_BIND_FAILED";
        case ErrorCode::NETWORK_LISTEN_FAILED: return "NETWORK_LISTEN_FAILED";
        case ErrorCode::NETWORK_ACCEPT_FAILED: return "NETWORK_ACCEPT_FAILED";
        case ErrorCode::NETWORK_TIMEOUT: return "NETWORK_TIMEOUT";
        case ErrorCode::FS_FILE_NOT_FOUND: return "FS_FILE_NOT_FOUND";
        case ErrorCode::FS_PERMISSION_DENIED: return "FS_PERMISSION_DENIED";
        case ErrorCode::FS_DISK_FULL: return "FS_DISK_FULL";
        case ErrorCode::FS_PATH_TRAVERSAL: return "FS_PATH_TRAVERSAL";
        case ErrorCode::FS_INVALID_PATH: return "FS_INVALID_PATH";
        case ErrorCode::PROTOCOL_INVALID_MESSAGE: return "PROTOCOL_INVALID_MESSAGE";
        case ErrorCode::PROTOCOL_UNSUPPORTED_VERSION: return "PROTOCOL_UNSUPPORTED_VERSION";
        case ErrorCode::PROTOCOL_PARSE_ERROR: return "PROTOCOL_PARSE_ERROR";
        case ErrorCode::MODULE_NOT_FOUND: return "MODULE_NOT_FOUND";
        case ErrorCode::MODULE_INVALID_CONFIG: return "MODULE_INVALID_CONFIG";
        case ErrorCode::MODULE_OPERATION_FAILED: return "MODULE_OPERATION_FAILED";
        case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
        case ErrorCode::INTERNAL_MEMORY_ERROR: return "INTERNAL_MEMORY_ERROR";
        case ErrorCode::INTERNAL_THREAD_ERROR: return "INTERNAL_THREAD_ERROR";
        case ErrorCode::EXTERNAL_DEPENDENCY_ERROR: return "EXTERNAL_DEPENDENCY_ERROR";
        case ErrorCode::EXTERNAL_SSL_ERROR: return "EXTERNAL_SSL_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

std::string RSyncError::getCategoryString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::CONFIGURATION: return "CONFIGURATION";
        case ErrorCategory::AUTHENTICATION: return "AUTHENTICATION";
        case ErrorCategory::AUTHORIZATION: return "AUTHORIZATION";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::FILE_SYSTEM: return "FILE_SYSTEM";
        case ErrorCategory::PROTOCOL: return "PROTOCOL";
        case ErrorCategory::MODULE: return "MODULE";
        case ErrorCategory::INTERNAL: return "INTERNAL";
        case ErrorCategory::EXTERNAL: return "EXTERNAL";
        default: return "UNKNOWN";
    }
}

bool RSyncError::isRecoverableError(ErrorCode code) {
    switch (code) {
        case ErrorCode::NETWORK_TIMEOUT:
        case ErrorCode::NETWORK_CONNECTION_FAILED:
        case ErrorCode::AUTH_SESSION_EXPIRED:
        case ErrorCode::CONFIG_RELOAD_FAILED:
            return true;
        default:
            return false;
    }
}

std::string RSyncError::getRecoverySuggestionForCode(ErrorCode code) {
    switch (code) {
        case ErrorCode::CONFIG_FILE_NOT_FOUND:
            return "Check that the configuration file exists and is readable";
        case ErrorCode::CONFIG_INVALID_FORMAT:
            return "Verify the configuration file format (INI or JSON)";
        case ErrorCode::AUTH_FAILED:
            return "Check username and password";
        case ErrorCode::AUTH_ACCOUNT_LOCKED:
            return "Account is locked. Contact administrator";
        case ErrorCode::AUTH_PASSWORD_EXPIRED:
            return "Password has expired. Please change your password";
        case ErrorCode::NETWORK_BIND_FAILED:
            return "Check if the port is already in use or if you have permission to bind";
        case ErrorCode::FS_PERMISSION_DENIED:
            return "Check file permissions and ownership";
        case ErrorCode::FS_DISK_FULL:
            return "Free up disk space";
        default:
            return "Check logs for more details";
    }
}

// ErrorContext implementation
ErrorContext::ErrorContext() {
}

ErrorContext& ErrorContext::addField(const std::string& key, const std::string& value) {
    fields_[key] = value;
    return *this;
}

ErrorContext& ErrorContext::setComponent(const std::string& component) {
    component_ = component;
    return *this;
}

ErrorContext& ErrorContext::setOperation(const std::string& operation) {
    operation_ = operation;
    return *this;
}

ErrorContext& ErrorContext::setPath(const std::string& path) {
    path_ = path;
    return *this;
}

ErrorContext& ErrorContext::setUser(const std::string& user) {
    user_ = user;
    return *this;
}

ErrorContext& ErrorContext::setClientAddress(const std::string& address) {
    client_address_ = address;
    return *this;
}

std::string ErrorContext::build() const {
    std::stringstream ss;
    bool first = true;

    if (!component_.empty()) {
        ss << "component=" << component_;
        first = false;
    }
    if (!operation_.empty()) {
        if (!first) ss << ", ";
        ss << "operation=" << operation_;
        first = false;
    }
    if (!path_.empty()) {
        if (!first) ss << ", ";
        ss << "path=" << path_;
        first = false;
    }
    if (!user_.empty()) {
        if (!first) ss << ", ";
        ss << "user=" << user_;
        first = false;
    }
    if (!client_address_.empty()) {
        if (!first) ss << ", ";
        ss << "client=" << client_address_;
        first = false;
    }
    for (const auto& [key, value] : fields_) {
        if (!first) ss << ", ";
        ss << key << "=" << value;
        first = false;
    }

    return ss.str();
}

std::map<std::string, std::string> ErrorContext::getFields() const {
    std::map<std::string, std::string> all_fields = fields_;
    if (!component_.empty()) all_fields["component"] = component_;
    if (!operation_.empty()) all_fields["operation"] = operation_;
    if (!path_.empty()) all_fields["path"] = path_;
    if (!user_.empty()) all_fields["user"] = user_;
    if (!client_address_.empty()) all_fields["client_address"] = client_address_;
    return all_fields;
}

} // namespace simple_rsyncd
