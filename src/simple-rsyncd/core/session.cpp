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
                           const std::map<std::string, std::shared_ptr<Module>>& modules)
    : client_socket_(client_socket)
    , client_address_(client_address)
    , config_(config)
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
    if (active_ && client_socket_ >= 0) {
        ::close(client_socket_);
        client_socket_ = -1;
        active_ = false;
    }
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
    ssize_t bytes_read = recv(client_socket_, buffer.data(), buffer.size() - 1, 0);

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
    ssize_t bytes_sent = send(client_socket_, response.c_str(), response.length(), 0);
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

    // Check authentication if required
    if (config_ && config_->auth.enabled && authenticated_user_.empty()) {
        // Authentication required but not yet authenticated
        // For now, allow and set a default user
        // TODO: Implement proper authentication handshake
        authenticated_user_ = "anonymous";
    }

    // Handle the protocol message
    return handler_->handle(message);
}

bool RSyncSession::startFileUpload(const std::string& module_name, const std::string& path) {
    transfer_module_ = module_name;
    transfer_path_ = path;

    // Get module using handler's getModule method
    std::string module_name_to_use = module_name.empty() ? handler_->getModule() : module_name;
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
    std::string module_name_to_use = module_name.empty() ? handler_->getModule() : module_name;
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
            ssize_t bytes_sent = send(client_socket_, buffer.data(), bytes_read, 0);
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
        ssize_t bytes_received = recv(client_socket_, buffer.data(), buffer.size(), 0);

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
