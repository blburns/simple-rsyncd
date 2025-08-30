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
