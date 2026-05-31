#include "simple-rsyncd/security/network_access.hpp"

#include <arpa/inet.h>
#include <cstring>

namespace simple_rsyncd {

namespace {

bool parseIpv4(const std::string& address, uint32_t& out) {
    struct in_addr addr {};
    if (inet_pton(AF_INET, address.c_str(), &addr) != 1) {
        return false;
    }
    out = ntohl(addr.s_addr);
    return true;
}

bool parseCidr(const std::string& rule, uint32_t& network, uint8_t& prefix_len) {
    const auto slash = rule.find('/');
    if (slash == std::string::npos) {
        if (!parseIpv4(rule, network)) {
            return false;
        }
        prefix_len = 32;
        return true;
    }

    const std::string ip_part = rule.substr(0, slash);
    const std::string prefix_part = rule.substr(slash + 1);
    if (!parseIpv4(ip_part, network)) {
        return false;
    }

    try {
        const int prefix = std::stoi(prefix_part);
        if (prefix < 0 || prefix > 32) {
            return false;
        }
        prefix_len = static_cast<uint8_t>(prefix);
    } catch (...) {
        return false;
    }

    return true;
}

bool ipInCidr(uint32_t ip, uint32_t network, uint8_t prefix_len) {
    if (prefix_len == 0) {
        return true;
    }
    const uint32_t mask = prefix_len == 32 ? 0xFFFFFFFFu
                                           : (0xFFFFFFFFu << (32 - prefix_len));
    return (ip & mask) == (network & mask);
}

bool ipMatchesRule(const std::string& client_ip, const std::string& rule) {
    uint32_t client = 0;
    if (!parseIpv4(client_ip, client)) {
        return false;
    }

    uint32_t network = 0;
    uint8_t prefix_len = 32;
    if (!parseCidr(rule, network, prefix_len)) {
        return false;
    }

    return ipInCidr(client, network, prefix_len);
}

bool matchesAny(const std::string& client_ip,
                const std::vector<std::string>& rules) {
    for (const auto& rule : rules) {
        if (rule.empty()) {
            continue;
        }
        if (ipMatchesRule(client_ip, rule)) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool NetworkAccess::isValidIpv4(const std::string& address) {
    uint32_t ip = 0;
    return parseIpv4(address, ip);
}

bool NetworkAccess::hostMatchesRule(const std::string& client_ip, const std::string& rule) {
    return ipMatchesRule(client_ip, rule);
}

bool NetworkAccess::isAllowed(const std::string& client_ip,
                              const std::vector<std::string>& denied_hosts,
                              const std::vector<std::string>& denied_networks,
                              const std::vector<std::string>& allowed_hosts,
                              const std::vector<std::string>& allowed_networks) {
    if (matchesAny(client_ip, denied_hosts) || matchesAny(client_ip, denied_networks)) {
        return false;
    }

    const bool has_allow_list = !allowed_hosts.empty() || !allowed_networks.empty();
    if (!has_allow_list) {
        return true;
    }

    return matchesAny(client_ip, allowed_hosts) || matchesAny(client_ip, allowed_networks);
}

}  // namespace simple_rsyncd
