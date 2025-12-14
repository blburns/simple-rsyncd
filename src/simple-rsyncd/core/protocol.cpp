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
#include "simple-rsyncd/core/module.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iomanip>

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
ProtocolHandler::ProtocolHandler(const std::map<std::string, std::shared_ptr<Module>>& module_map)
    : modules_(module_map) {
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

std::string ProtocolHandler::getCurrentModule() const {
    return current_module_;
}

std::shared_ptr<Module> ProtocolHandler::getModule(const std::string& module_name) const {
    auto it = modules_.find(module_name);
    if (it != modules_.end()) {
        return it->second;
    }
    return nullptr;
}

std::string ProtocolHandler::handleList(const ProtocolMessage& message) {
    std::string module_name = message.module.empty() ? current_module_ : message.module;
    auto module = getModule(module_name);

    if (!module) {
        return ProtocolParser().buildErrorResponse(1, "Module not found: " + module_name);
    }

    if (!module->allowsListing()) {
        return ProtocolParser().buildErrorResponse(2, "Listing not allowed for module: " + module_name);
    }

    try {
        bool recursive = message.arguments.find("recursive") != message.arguments.end();
        DirectoryListing listing = module->listDirectory(message.path, recursive);

        // Format directory listing as response
        std::ostringstream response;
        response << "@RSYNCD: OK\n";
        response << "Path: " << listing.path << "\n";
        response << "Files: " << listing.total_files << "\n";
        response << "Directories: " << listing.total_directories << "\n";
        response << "Total size: " << listing.total_size << "\n";

        // List files
        for (const auto& file : listing.files) {
            response << "F " << file.name << " " << file.size << "\n";
        }

        // List directories
        for (const auto& dir : listing.directories) {
            response << "D " << dir.name << "\n";
        }

        return response.str();
    } catch (const std::exception& e) {
        return ProtocolParser().buildErrorResponse(3, "Error listing directory: " + std::string(e.what()));
    }
}

std::string ProtocolHandler::handleGet(const ProtocolMessage& message) {
    std::string module_name = message.module.empty() ? current_module_ : message.module;
    auto module = getModule(module_name);

    if (!module) {
        return ProtocolParser().buildErrorResponse(1, "Module not found: " + module_name);
    }

    if (!module->fileExists(message.path)) {
        return ProtocolParser().buildErrorResponse(4, "File not found: " + message.path);
    }

    try {
        FileInfo info = module->getFileInfo(message.path);

        // Format file info as response - this signals ready for transfer
        std::ostringstream response;
        response << "@RSYNCD: OK\n";
        response << "File: " << info.name << "\n";
        response << "Size: " << info.size << "\n";
        response << "Path: " << info.path << "\n";
        response << "Ready for transfer\n";

        return response.str();
    } catch (const std::exception& e) {
        return ProtocolParser().buildErrorResponse(3, "Error getting file: " + std::string(e.what()));
    }
}

std::string ProtocolHandler::handlePut(const ProtocolMessage& message) {
    std::string module_name = message.module.empty() ? current_module_ : message.module;
    auto module = getModule(module_name);

    if (!module) {
        return ProtocolParser().buildErrorResponse(1, "Module not found: " + module_name);
    }

    if (module->isReadOnly()) {
        return ProtocolParser().buildErrorResponse(5, "Module is read-only: " + module_name);
    }

    try {
        // For PUT, we need to receive the file data
        // This is a simplified response - actual file transfer would happen in session
        std::ostringstream response;
        response << "@RSYNCD: OK\n";
        response << "Ready to receive file: " << message.path << "\n";
        response << "Send file data\n";

        return response.str();
    } catch (const std::exception& e) {
        return ProtocolParser().buildErrorResponse(3, "Error preparing for upload: " + std::string(e.what()));
    }
}

std::string ProtocolHandler::handleDelete(const ProtocolMessage& message) {
    std::string module_name = message.module.empty() ? current_module_ : message.module;
    auto module = getModule(module_name);

    if (!module) {
        return ProtocolParser().buildErrorResponse(1, "Module not found: " + module_name);
    }

    if (module->isReadOnly() || !module->allowsDeletion()) {
        return ProtocolParser().buildErrorResponse(5, "Deletion not allowed for module: " + module_name);
    }

    try {
        bool deleted = false;
        if (module->fileExists(message.path)) {
            deleted = module->deleteFile(message.path);
        } else if (module->directoryExists(message.path)) {
            bool recursive = message.arguments.find("recursive") != message.arguments.end();
            deleted = module->deleteDirectory(message.path, recursive);
        } else {
            return ProtocolParser().buildErrorResponse(4, "Path not found: " + message.path);
        }

        if (deleted) {
            return ProtocolParser().buildResponse(true, "Deleted: " + message.path);
        } else {
            return ProtocolParser().buildErrorResponse(6, "Failed to delete: " + message.path);
        }
    } catch (const std::exception& e) {
        return ProtocolParser().buildErrorResponse(3, "Error deleting: " + std::string(e.what()));
    }
}

std::string ProtocolHandler::handleStat(const ProtocolMessage& message) {
    std::string module_name = message.module.empty() ? current_module_ : message.module;
    auto module = getModule(module_name);

    if (!module) {
        return ProtocolParser().buildErrorResponse(1, "Module not found: " + module_name);
    }

    try {
        FileInfo info = module->getFileInfo(message.path);

        if (info.path.empty()) {
            return ProtocolParser().buildErrorResponse(4, "File not found: " + message.path);
        }

        // Format file stats as response
        std::ostringstream response;
        response << "@RSYNCD: OK\n";
        response << "Name: " << info.name << "\n";
        response << "Path: " << info.path << "\n";
        response << "Size: " << info.size << "\n";
        response << "Type: " << static_cast<int>(info.type) << "\n";
        response << "Is symlink: " << (info.is_symlink ? "yes" : "no") << "\n";

        return response.str();
    } catch (const std::exception& e) {
        return ProtocolParser().buildErrorResponse(3, "Error getting file stats: " + std::string(e.what()));
    }
}

} // namespace simple_rsyncd
