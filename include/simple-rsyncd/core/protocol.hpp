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
#include <vector>
#include <map>
#include <cstdint>

namespace simple_rsyncd {

/**
 * @brief RSync protocol version
 */
enum class ProtocolVersion {
    VERSION_30 = 30,  // rsync 3.0
    VERSION_29 = 29,  // rsync 2.9
    VERSION_27 = 27   // rsync 2.7
};

/**
 * @brief RSync protocol command types
 */
enum class ProtocolCommand {
    LIST,      // List directory contents
    GET,       // Get file
    PUT,       // Put file
    DELETE,    // Delete file
    STAT,      // Get file stats
    UNKNOWN
};

/**
 * @brief RSync protocol message
 */
struct ProtocolMessage {
    ProtocolCommand command = ProtocolCommand::UNKNOWN;
    std::string module;
    std::string path;
    std::map<std::string, std::string> arguments;
    std::vector<uint8_t> data;
    bool valid = false;
    std::string error_message;
};

/**
 * @brief RSync protocol parser
 */
class ProtocolParser {
public:
    /**
     * @brief Constructor
     */
    ProtocolParser();

    /**
     * @brief Parse protocol message from buffer
     * @param buffer Input buffer
     * @param size Buffer size
     * @return Parsed protocol message
     */
    ProtocolMessage parse(const uint8_t* buffer, size_t size);

    /**
     * @brief Parse protocol message from string
     * @param data Input string
     * @return Parsed protocol message
     */
    ProtocolMessage parse(const std::string& data);

    /**
     * @brief Build protocol response
     * @param success Whether operation succeeded
     * @param message Response message
     * @param data Optional response data
     * @return Response string
     */
    std::string buildResponse(bool success, const std::string& message, const std::vector<uint8_t>& data = {});

    /**
     * @brief Build error response
     * @param error_code Error code
     * @param error_message Error message
     * @return Error response string
     */
    std::string buildErrorResponse(int error_code, const std::string& error_message);

private:
    ProtocolVersion version_;
    
    // Parsing helpers
    ProtocolCommand parseCommand(const std::string& command_str);
    std::string parseModule(const std::string& line);
    std::string parsePath(const std::string& line);
    std::map<std::string, std::string> parseArguments(const std::string& line);
    
    // Response building helpers
    std::string formatSuccess(const std::string& message);
    std::string formatError(int code, const std::string& message);
};

/**
 * @brief RSync protocol handler
 */
class ProtocolHandler {
public:
    /**
     * @brief Constructor
     */
    ProtocolHandler();

    /**
     * @brief Handle protocol message
     * @param message Protocol message
     * @return Response string
     */
    std::string handle(const ProtocolMessage& message);

    /**
     * @brief Set module for operations
     * @param module_name Module name
     */
    void setModule(const std::string& module_name);

    /**
     * @brief Get current module name
     * @return Module name
     */
    std::string getModule() const;

private:
    std::string current_module_;
    
    // Command handlers
    std::string handleList(const ProtocolMessage& message);
    std::string handleGet(const ProtocolMessage& message);
    std::string handlePut(const ProtocolMessage& message);
    std::string handleDelete(const ProtocolMessage& message);
    std::string handleStat(const ProtocolMessage& message);
};

} // namespace simple_rsyncd
