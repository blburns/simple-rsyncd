#pragma once

#include <string>
#include <vector>

namespace simple_rsyncd {

/**
 * @brief IPv4 host/network access matching utilities
 */
class NetworkAccess {
public:
    static bool isValidIpv4(const std::string& address);

    /** Match a client IP against a rule (exact IPv4 or CIDR, e.g. 10.0.0.0/8). */
    static bool hostMatchesRule(const std::string& client_ip, const std::string& rule);

    /**
     * @brief Evaluate allow/deny lists with optional CIDR networks
     * @return true if the client may connect
     */
    static bool isAllowed(const std::string& client_ip,
                          const std::vector<std::string>& denied_hosts,
                          const std::vector<std::string>& denied_networks,
                          const std::vector<std::string>& allowed_hosts,
                          const std::vector<std::string>& allowed_networks);
};

}  // namespace simple_rsyncd
