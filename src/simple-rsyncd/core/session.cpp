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
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <vector>

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
    , handler_(std::make_unique<ProtocolHandler>(modules)) {
}

RSyncSession::~RSyncSession() {
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

} // namespace simple_rsyncd
