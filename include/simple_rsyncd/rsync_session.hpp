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

#include "simple_rsyncd/configuration.hpp"

namespace simple_rsyncd {

class RSyncSession {
public:
    RSyncSession(int client_socket, const std::string& client_address, 
                 std::shared_ptr<Configuration> config);
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
    
    bool readRequest();
    bool writeResponse(const std::string& response);
    bool parseRequest(const std::string& request);
};

} // namespace simple_rsyncd
