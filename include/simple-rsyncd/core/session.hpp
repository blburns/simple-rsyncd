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
#include <memory>
#include <chrono>

#include "simple-rsyncd/config/config.hpp"
#include "simple-rsyncd/core/protocol.hpp"

namespace simple_rsyncd {

// Forward declaration
class RSyncDaemon;

class RSyncSession {
public:
    RSyncSession(int client_socket, const std::string& client_address,
                 std::shared_ptr<Configuration> config,
                 const std::map<std::string, std::shared_ptr<Module>>& modules);
    ~RSyncSession();

    std::string getClientAddress() const;
    int getClientSocket() const;
    bool isActive() const;
    void close();

    bool processRequest();
    bool authenticate();
    bool authorize(const std::string& module_name, const std::string& operation);

private:
    int client_socket_;
    std::string client_address_;
    std::shared_ptr<Configuration> config_;
    bool active_;
    std::chrono::steady_clock::time_point start_time_;

    // Protocol handling
    std::unique_ptr<ProtocolParser> parser_;
    std::unique_ptr<ProtocolHandler> handler_;
    std::string authenticated_user_;

    // File transfer state
    enum class TransferState {
        IDLE,
        RECEIVING_FILE,  // Receiving file data (PUT)
        SENDING_FILE     // Sending file data (GET)
    };
    TransferState transfer_state_;
    std::string transfer_module_;
    std::string transfer_path_;
    std::ofstream upload_file_;
    std::ifstream download_file_;
    size_t transfer_bytes_remaining_;
    size_t transfer_bytes_sent_;

    bool readRequest();
    bool writeResponse(const std::string& response);
    bool parseRequest(const std::string& request);
    std::string handleProtocolMessage(const ProtocolMessage& message);

    // File transfer methods
    bool startFileUpload(const std::string& module_name, const std::string& path);
    bool startFileDownload(const std::string& module_name, const std::string& path);
    bool continueFileTransfer();
    void endFileTransfer();
};

} // namespace simple_rsyncd
