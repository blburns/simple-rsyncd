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
#include <map>
#include <vector>
#include <exception>
#include <chrono>
#include <memory>

namespace simple_rsyncd {

/**
 * @brief Error categories
 */
enum class ErrorCategory {
    CONFIGURATION,
    AUTHENTICATION,
    AUTHORIZATION,
    NETWORK,
    FILE_SYSTEM,
    PROTOCOL,
    MODULE,
    INTERNAL,
    EXTERNAL
};

/**
 * @brief Error codes
 */
enum class ErrorCode {
    // Configuration errors (1000-1999)
    CONFIG_FILE_NOT_FOUND = 1001,
    CONFIG_INVALID_FORMAT = 1002,
    CONFIG_VALIDATION_FAILED = 1003,
    CONFIG_RELOAD_FAILED = 1004,

    // Authentication errors (2000-2999)
    AUTH_FAILED = 2001,
    AUTH_USER_NOT_FOUND = 2002,
    AUTH_PASSWORD_INVALID = 2003,
    AUTH_ACCOUNT_LOCKED = 2004,
    AUTH_PASSWORD_EXPIRED = 2005,
    AUTH_SESSION_EXPIRED = 2006,

    // Authorization errors (3000-3999)
    AUTHZ_ACCESS_DENIED = 3001,
    AUTHZ_INSUFFICIENT_PERMISSIONS = 3002,
    AUTHZ_IP_DENIED = 3003,

    // Network errors (4000-4999)
    NETWORK_CONNECTION_FAILED = 4001,
    NETWORK_BIND_FAILED = 4002,
    NETWORK_LISTEN_FAILED = 4003,
    NETWORK_ACCEPT_FAILED = 4004,
    NETWORK_TIMEOUT = 4005,

    // File system errors (5000-5999)
    FS_FILE_NOT_FOUND = 5001,
    FS_PERMISSION_DENIED = 5002,
    FS_DISK_FULL = 5003,
    FS_PATH_TRAVERSAL = 5004,
    FS_INVALID_PATH = 5005,

    // Protocol errors (6000-6999)
    PROTOCOL_INVALID_MESSAGE = 6001,
    PROTOCOL_UNSUPPORTED_VERSION = 6002,
    PROTOCOL_PARSE_ERROR = 6003,

    // Module errors (7000-7999)
    MODULE_NOT_FOUND = 7001,
    MODULE_INVALID_CONFIG = 7002,
    MODULE_OPERATION_FAILED = 7003,

    // Internal errors (8000-8999)
    INTERNAL_ERROR = 8001,
    INTERNAL_MEMORY_ERROR = 8002,
    INTERNAL_THREAD_ERROR = 8003,

    // External errors (9000-9999)
    EXTERNAL_DEPENDENCY_ERROR = 9001,
    EXTERNAL_SSL_ERROR = 9002
};

/**
 * @brief Comprehensive error class
 */
class RSyncError : public std::exception {
public:
    RSyncError(ErrorCode code, const std::string& message, ErrorCategory category = ErrorCategory::INTERNAL);
    RSyncError(ErrorCode code, const std::string& message, const std::string& context, ErrorCategory category = ErrorCategory::INTERNAL);

    virtual ~RSyncError() = default;

    ErrorCode getCode() const;
    ErrorCategory getCategory() const;
    std::string getMessage() const;
    std::string getContext() const;
    std::map<std::string, std::string> getFields() const;

    void addField(const std::string& key, const std::string& value);
    void setContext(const std::string& context);

    const char* what() const noexcept override;

    std::string toString() const;
    std::string toJSON() const;

    // Error recovery
    bool isRecoverable() const;
    std::string getRecoverySuggestion() const;

private:
    ErrorCode code_;
    ErrorCategory category_;
    std::string message_;
    std::string context_;
    std::map<std::string, std::string> fields_;
    std::chrono::system_clock::time_point timestamp_;

    static std::string getErrorCodeString(ErrorCode code);
    static std::string getCategoryString(ErrorCategory category);
    static bool isRecoverableError(ErrorCode code);
    static std::string getRecoverySuggestionForCode(ErrorCode code);
};

/**
 * @brief Error context builder
 */
class ErrorContext {
public:
    ErrorContext();

    ErrorContext& addField(const std::string& key, const std::string& value);
    ErrorContext& setComponent(const std::string& component);
    ErrorContext& setOperation(const std::string& operation);
    ErrorContext& setPath(const std::string& path);
    ErrorContext& setUser(const std::string& user);
    ErrorContext& setClientAddress(const std::string& address);

    std::string build() const;
    std::map<std::string, std::string> getFields() const;

private:
    std::string component_;
    std::string operation_;
    std::string path_;
    std::string user_;
    std::string client_address_;
    std::map<std::string, std::string> fields_;
};

} // namespace simple_rsyncd
