#pragma once

#include <chrono>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>

namespace simple_rsyncd {

/**
 * @brief Per-client connection rate limiter (minute and hour windows)
 */
class ConnectionRateLimiter {
public:
    ConnectionRateLimiter(size_t max_per_minute, size_t max_per_hour);

    /** @return true if a new connection from @p client_ip is allowed */
    bool allowConnection(const std::string& client_ip);

    void recordConnection(const std::string& client_ip);

private:
    struct Window {
        size_t count = 0;
        std::chrono::steady_clock::time_point start{};
    };

    size_t max_per_minute_;
    size_t max_per_hour_;
    mutable std::mutex mutex_;
    std::map<std::string, Window> minute_windows_;
    std::map<std::string, Window> hour_windows_;

    static bool incrementWindow(Window& window,
                                size_t max_count,
                                std::chrono::seconds period);
};

}  // namespace simple_rsyncd
