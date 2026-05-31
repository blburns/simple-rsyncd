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

#include "simple-rsyncd/core/session.hpp"
#include "simple-rsyncd/core/protocol.hpp"
#include "simple-rsyncd/core/module.hpp"
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/security/ssl_context.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <filesystem>

namespace simple_rsyncd {

RSyncSession::RSyncSession(int client_socket, const std::string& client_address,
                           std::shared_ptr<Configuration> config,
                           const std::map<std::string, std::shared_ptr<Module>>& modules,
                           AuthenticationManager* auth_manager,
                           std::unique_ptr<SSLConnection> ssl_connection)
    : client_socket_(client_socket)
    , ssl_connection_(std::move(ssl_connection))
    , client_address_(client_address)
    , config_(config)
    , auth_manager_(auth_manager)
    , active_(true)
    , start_time_(std::chrono::steady_clock::now())
    , parser_(std::make_unique<ProtocolParser>())
    , handler_(std::make_unique<ProtocolHandler>(modules))
    , transfer_state_(TransferState::IDLE)
    , transfer_bytes_remaining_(0)
    , transfer_bytes_sent_(0) {
}

RSyncSession::~RSyncSession() {
    endFileTransfer();
    close();
}

std::string RSyncSession::getClientAddress() const {
    return client_address_;
}

int RSyncSession::getClientSocket() const {
    return client_socket_;
}

bool RSyncSession::isActive() const {
    return active_;
}

void RSyncSession::close() {
    if (ssl_connection_) {
        ssl_connection_->close();
        client_socket_ = -1;
        active_ = false;
        return;
    }
    if (active_ && client_socket_ >= 0) {
        ::close(client_socket_);
        client_socket_ = -1;
        active_ = false;
    }
}

ssize_t RSyncSession::readSocket(void* buffer, size_t length) {
    if (ssl_connection_ && ssl_connection_->isOpen()) {
        return ssl_connection_->read(buffer, length);
    }
    if (client_socket_ < 0) {
        return -1;
    }
    return recv(client_socket_, buffer, length, 0);
}

ssize_t RSyncSession::writeSocket(const void* buffer, size_t length) {
    if (ssl_connection_ && ssl_connection_->isOpen()) {
        return ssl_connection_->write(buffer, length);
    }
    if (client_socket_ < 0) {
        return -1;
    }
    return send(client_socket_, buffer, length, 0);
}

bool RSyncSession::processRequest() {
    if (!active_) {
        return false;
    }

    // If we're in the middle of a file transfer, continue it
    if (transfer_state_ != TransferState::IDLE) {
        return continueFileTransfer();
    }

    // Read request from socket
    std::vector<uint8_t> buffer(4096);
    ssize_t bytes_read = readSocket(buffer.data(), buffer.size() - 1);

    if (bytes_read <= 0) {
        active_ = false;
        return false;
    }

    // Convert to string for parsing
    std::string request(reinterpret_cast<const char*>(buffer.data()), bytes_read);

    // Parse protocol message
    ProtocolMessage message = parser_->parse(request);
    if (!message.valid) {
        std::string error = parser_->buildErrorResponse(-1, "Invalid protocol message: " + message.error_message);
        return writeResponse(error);
    }

    // Handle protocol message
    std::string response = handleProtocolMessage(message);

    // If response indicates file transfer, handle it
    if (response.find("Ready for transfer") != std::string::npos && message.command == ProtocolCommand::GET) {
        // Start file download
        if (startFileDownload(message.module, message.path)) {
            return continueFileTransfer();
        }
    } else if (response.find("Ready to receive file") != std::string::npos && message.command == ProtocolCommand::PUT) {
        // Start file upload
        if (startFileUpload(message.module, message.path)) {
            // Send response and wait for file data
            if (writeResponse(response)) {
                transfer_state_ = TransferState::RECEIVING_FILE;
                return true;
            }
        }
    }

    return writeResponse(response);
}

bool RSyncSession::authenticate() {
    if (!config_ || !config_->auth.enabled) {
        authenticated_user_ = "anonymous";
        return true; // No authentication required
    }

    // Basic authentication implementation
    // TODO: Implement proper authentication handshake with client
    // For now, allow anonymous access if configured
    if (config_->auth.anonymous_access) {
        authenticated_user_ = "anonymous";
        return true;
    }

    // Authentication required but not implemented yet
    return false;
}

bool RSyncSession::authorize(const std::string& module_name, const std::string& operation) {
    (void)module_name; // Suppress unused parameter warning
    (void)operation; // Suppress unused parameter warning
    if (!config_ || !config_->access.enabled) {
        return true; // No access control required
    }

    // Basic authorization implementation
    // For now, always return true
    return true;
}

bool RSyncSession::readRequest() {
    // Reading is now done in processRequest()
    return true;
}

bool RSyncSession::writeResponse(const std::string& response) {
    if (client_socket_ < 0 || !active_) {
        return false;
    }

    // Send response to client
    ssize_t bytes_sent = writeSocket(response.c_str(), response.length());
    return bytes_sent > 0;
}

bool RSyncSession::parseRequest(const std::string& request) {
    // Request is already in string format, parsing happens in processRequest
    return !request.empty();
}

std::string RSyncSession::handleProtocolMessage(const ProtocolMessage& message) {
    // Set module for handler
    if (!message.module.empty()) {
        handler_->setModule(message.module);
    }

    // Public-key auth must complete before other commands (except AUTH).
    if (config_ && config_->auth.enabled && config_->auth.method == "public_key" &&
        authenticated_user_.empty() && message.command != ProtocolCommand::AUTH) {
        return parser_->buildErrorResponse(401, "Authentication required: send AUTH command");
    }

    if (message.command == ProtocolCommand::AUTH) {
        return handleAuthMessage(message);
    }
    // Handle the protocol message
    return handler_->handle(message);
}

std::string RSyncSession::handleAuthMessage(const ProtocolMessage& message) {
    if (!config_ || !config_->auth.enabled) {
        authenticated_user_ = "anonymous";
        return parser_->buildResponse(true, "Authentication disabled");
    }

    if (config_->auth.method == "public_key") {
        if (!auth_manager_) {
            return parser_->buildErrorResponse(500, "Authentication manager unavailable");
        }

        const auto sig_it = message.arguments.find("signature");
        if (message.module.empty() || message.path.empty() || sig_it == message.arguments.end()) {
            return parser_->buildErrorResponse(400,
                "AUTH requires: AUTH <user> <challenge> signature=<base64_ssh_sig>");
        }

        if (auth_manager_->authenticateUserWithKey(message.module, message.path, sig_it->second)) {
            authenticated_user_ = message.module;
            return parser_->buildResponse(true, "Authenticated: " + message.module);
        }
        return parser_->buildErrorResponse(403, "Public key authentication failed");
    }

    if (config_->auth.method == "password") {
        const auto pass_it = message.arguments.find("password");
        if (message.module.empty() || pass_it == message.arguments.end()) {
            return parser_->buildErrorResponse(400, "AUTH requires: AUTH <user> _ password=<secret>");
        }
        if (auth_manager_ && auth_manager_->authenticateUser(message.module, pass_it->second)) {
            authenticated_user_ = message.module;
            return parser_->buildResponse(true, "Authenticated: " + message.module);
        }
        return parser_->buildErrorResponse(403, "Password authentication failed");
    }

    return parser_->buildErrorResponse(501, "Unsupported authentication method");
}

bool RSyncSession::startFileUpload(const std::string& module_name, const std::string& path) {
    transfer_module_ = module_name;
    transfer_path_ = path;

    // Get module using handler's getCurrentModule method
    std::string module_name_to_use = module_name.empty() ? handler_->getCurrentModule() : module_name;
    auto module = handler_->getModule(module_name_to_use);
    if (!module) {
        return false;
    }

    // Create temporary file for upload
    std::string temp_path = path + ".tmp";

    // Resolve path manually since resolvePath is protected
    std::string module_base = module->getPath();
    std::filesystem::path base_path(module_base);
    std::filesystem::path relative_path(temp_path);
    std::filesystem::path resolved = base_path / relative_path;
    resolved = resolved.lexically_normal();
    std::string full_path = std::filesystem::absolute(resolved).string();

    if (full_path.empty()) {
        return false;
    }

    // Create parent directory if needed
    std::filesystem::path parent = std::filesystem::path(full_path).parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }

    upload_file_.open(full_path, std::ios::binary | std::ios::out);
    if (!upload_file_.is_open()) {
        return false;
    }

    transfer_state_ = TransferState::RECEIVING_FILE;
    transfer_bytes_remaining_ = 0; // Will be set when we receive size
    return true;
}

bool RSyncSession::startFileDownload(const std::string& module_name, const std::string& path) {
    transfer_module_ = module_name;
    transfer_path_ = path;

    // Get module
    std::string module_name_to_use = module_name.empty() ? handler_->getCurrentModule() : module_name;
    auto module = handler_->getModule(module_name_to_use);
    if (!module) {
        return false;
    }

    // Get file info
    FileInfo info = module->getFileInfo(path);
    if (info.path.empty() || !module->fileExists(path)) {
        return false;
    }

    // Open file for reading
    download_file_.open(info.path, std::ios::binary | std::ios::in);
    if (!download_file_.is_open()) {
        return false;
    }

    transfer_state_ = TransferState::SENDING_FILE;
    transfer_bytes_remaining_ = info.size;
    transfer_bytes_sent_ = 0;
    return true;
}

bool RSyncSession::continueFileTransfer() {
    if (transfer_state_ == TransferState::SENDING_FILE) {
        // Send file data
        std::vector<uint8_t> buffer(8192); // 8KB chunks
        size_t chunk_size = std::min(buffer.size(), transfer_bytes_remaining_);

        download_file_.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
        size_t bytes_read = download_file_.gcount();

        if (bytes_read > 0) {
            ssize_t bytes_sent = writeSocket(buffer.data(), bytes_read);
            if (bytes_sent < 0) {
                endFileTransfer();
                return false;
            }

            transfer_bytes_sent_ += bytes_sent;
            transfer_bytes_remaining_ -= bytes_sent;

            if (transfer_bytes_remaining_ == 0) {
                // Transfer complete
                endFileTransfer();
                std::string complete_msg = "@RSYNCD: OK\nTransfer complete\n";
                writeResponse(complete_msg);
                return true;
            }
        } else {
            // File read error or EOF
            endFileTransfer();
            return false;
        }

        return true;
    } else if (transfer_state_ == TransferState::RECEIVING_FILE) {
        // Receive file data
        std::vector<uint8_t> buffer(8192); // 8KB chunks
        ssize_t bytes_received = readSocket(buffer.data(), buffer.size());

        if (bytes_received > 0) {
            upload_file_.write(reinterpret_cast<const char*>(buffer.data()), bytes_received);
            upload_file_.flush();

            transfer_bytes_sent_ += bytes_received;

            // Check if transfer is complete (we don't know size ahead of time, so check for protocol message)
            // For now, continue receiving until connection closes or we get a protocol message
            return true;
        } else if (bytes_received == 0) {
            // Connection closed, transfer complete
            endFileTransfer();

            // Move temp file to final location
            if (!transfer_module_.empty() && !transfer_path_.empty()) {
                auto module = handler_->getModule(transfer_module_);
                if (module) {
                    std::string module_base = module->getPath();
                    std::string temp_path = transfer_path_ + ".tmp";

                    std::filesystem::path base_path(module_base);
                    std::filesystem::path temp_rel(temp_path);
                    std::filesystem::path final_rel(transfer_path_);

                    std::filesystem::path full_temp = std::filesystem::absolute(base_path / temp_rel);
                    std::filesystem::path full_final = std::filesystem::absolute(base_path / final_rel);

                    if (std::filesystem::exists(full_temp)) {
                        std::filesystem::rename(full_temp, full_final);
                    }
                }
            }

            std::string complete_msg = "@RSYNCD: OK\nFile uploaded successfully\n";
            writeResponse(complete_msg);
            return true;
        } else {
            // Error receiving
            endFileTransfer();
            return false;
        }
    }

    return false;
}

void RSyncSession::endFileTransfer() {
    if (upload_file_.is_open()) {
        upload_file_.close();
    }

    if (download_file_.is_open()) {
        download_file_.close();
    }

    transfer_state_ = TransferState::IDLE;
    transfer_module_.clear();
    transfer_path_.clear();
    transfer_bytes_remaining_ = 0;
    transfer_bytes_sent_ = 0;
}

} // namespace simple_rsyncd
