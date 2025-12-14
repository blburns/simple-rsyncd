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

#include "simple-rsyncd/core/protocol.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace simple_rsyncd {

// ProtocolParser implementation
ProtocolParser::ProtocolParser() : version_(ProtocolVersion::VERSION_30) {
}

ProtocolMessage ProtocolParser::parse(const uint8_t* buffer, size_t size) {
    ProtocolMessage message;
    
    if (size == 0 || buffer == nullptr) {
        message.error_message = "Empty buffer";
        return message;
    }

    // Convert buffer to string for parsing
    std::string data(reinterpret_cast<const char*>(buffer), size);
    return parse(data);
}

ProtocolMessage ProtocolParser::parse(const std::string& data) {
    ProtocolMessage message;
    
    if (data.empty()) {
        message.error_message = "Empty data";
        return message;
    }

    // Basic rsync protocol parsing
    // Format: @RSYNCD: <version>\n<command> <module> <path> [args]\n
    
    std::istringstream stream(data);
    std::string line;
    
    // Parse first line - protocol header
    if (std::getline(stream, line)) {
        // Check for rsync protocol header
        if (line.find("@RSYNCD:") == 0) {
            // Extract version
            size_t version_pos = line.find(":");
            if (version_pos != std::string::npos) {
                std::string version_str = line.substr(version_pos + 1);
                // Remove whitespace
                version_str.erase(0, version_str.find_first_not_of(" \t"));
                version_str.erase(version_str.find_last_not_of(" \t") + 1);
                
                try {
                    int version_num = std::stoi(version_str);
                    if (version_num == 30) version_ = ProtocolVersion::VERSION_30;
                    else if (version_num == 29) version_ = ProtocolVersion::VERSION_29;
                    else if (version_num == 27) version_ = ProtocolVersion::VERSION_27;
                } catch (...) {
                    // Use default version
                }
            }
        } else {
            // No header, assume it's a command line
            // Parse as command
            message.command = parseCommand(line);
            if (message.command != ProtocolCommand::UNKNOWN) {
                // Try to extract module and path
                std::istringstream cmd_stream(line);
                std::string cmd, module, path;
                if (cmd_stream >> cmd >> module >> path) {
                    message.module = module;
                    message.path = path;
                    message.valid = true;
                }
            }
            return message;
        }
    }

    // Parse command line
    if (std::getline(stream, line)) {
        message.command = parseCommand(line);
        message.module = parseModule(line);
        message.path = parsePath(line);
        message.arguments = parseArguments(line);
        message.valid = (message.command != ProtocolCommand::UNKNOWN);
    }

    return message;
}

ProtocolCommand ProtocolParser::parseCommand(const std::string& line) {
    // Extract first word as command
    std::istringstream stream(line);
    std::string cmd;
    stream >> cmd;

    // Convert to uppercase for comparison
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "LIST" || cmd == "LS") {
        return ProtocolCommand::LIST;
    } else if (cmd == "GET" || cmd == "DOWNLOAD") {
        return ProtocolCommand::GET;
    } else if (cmd == "PUT" || cmd == "UPLOAD" || cmd == "SEND") {
        return ProtocolCommand::PUT;
    } else if (cmd == "DELETE" || cmd == "DEL" || cmd == "RM") {
        return ProtocolCommand::DELETE;
    } else if (cmd == "STAT" || cmd == "STATUS" || cmd == "INFO") {
        return ProtocolCommand::STAT;
    }

    return ProtocolCommand::UNKNOWN;
}

std::string ProtocolParser::parseModule(const std::string& line) {
    std::istringstream stream(line);
    std::string cmd, module;
    stream >> cmd >> module;
    return module;
}

std::string ProtocolParser::parsePath(const std::string& line) {
    std::istringstream stream(line);
    std::string cmd, module, path;
    stream >> cmd >> module >> path;
    return path;
}

std::map<std::string, std::string> ProtocolParser::parseArguments(const std::string& line) {
    std::map<std::string, std::string> args;
    
    // Simple argument parsing - look for key=value pairs
    std::istringstream stream(line);
    std::string token;
    
    // Skip command, module, path
    std::string dummy;
    stream >> dummy >> dummy >> dummy;
    
    while (stream >> token) {
        size_t eq_pos = token.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = token.substr(0, eq_pos);
            std::string value = token.substr(eq_pos + 1);
            args[key] = value;
        }
    }
    
    return args;
}

std::string ProtocolParser::buildResponse(bool success, const std::string& message, const std::vector<uint8_t>& data) {
    if (success) {
        return formatSuccess(message);
    } else {
        return formatError(-1, message);
    }
}

std::string ProtocolParser::buildErrorResponse(int error_code, const std::string& error_message) {
    return formatError(error_code, error_message);
}

std::string ProtocolParser::formatSuccess(const std::string& message) {
    return "@RSYNCD: OK\n" + message + "\n";
}

std::string ProtocolParser::formatError(int code, const std::string& message) {
    return "@RSYNCD: ERROR " + std::to_string(code) + "\n" + message + "\n";
}

// ProtocolHandler implementation
ProtocolHandler::ProtocolHandler() {
}

std::string ProtocolHandler::handle(const ProtocolMessage& message) {
    if (!message.valid) {
        return ProtocolParser().buildErrorResponse(-1, "Invalid protocol message");
    }

    switch (message.command) {
        case ProtocolCommand::LIST:
            return handleList(message);
        case ProtocolCommand::GET:
            return handleGet(message);
        case ProtocolCommand::PUT:
            return handlePut(message);
        case ProtocolCommand::DELETE:
            return handleDelete(message);
        case ProtocolCommand::STAT:
            return handleStat(message);
        default:
            return ProtocolParser().buildErrorResponse(-1, "Unknown command");
    }
}

void ProtocolHandler::setModule(const std::string& module_name) {
    current_module_ = module_name;
}

std::string ProtocolHandler::getModule() const {
    return current_module_;
}

std::string ProtocolHandler::handleList(const ProtocolMessage& message) {
    // Basic list response
    // TODO: Integrate with Module class to get actual directory listing
    return ProtocolParser().buildResponse(true, "Directory listing not yet implemented");
}

std::string ProtocolHandler::handleGet(const ProtocolMessage& message) {
    // Basic get response
    // TODO: Integrate with Module class to get actual file
    return ProtocolParser().buildResponse(true, "File transfer not yet implemented");
}

std::string ProtocolHandler::handlePut(const ProtocolMessage& message) {
    // Basic put response
    // TODO: Integrate with Module class to save file
    return ProtocolParser().buildResponse(true, "File upload not yet implemented");
}

std::string ProtocolHandler::handleDelete(const ProtocolMessage& message) {
    // Basic delete response
    // TODO: Integrate with Module class to delete file
    return ProtocolParser().buildResponse(true, "File deletion not yet implemented");
}

std::string ProtocolHandler::handleStat(const ProtocolMessage& message) {
    // Basic stat response
    // TODO: Integrate with Module class to get file info
    return ProtocolParser().buildResponse(true, "File stats not yet implemented");
}

} // namespace simple_rsyncd
