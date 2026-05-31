#include "simple-rsyncd/security/rate_limiter.hpp"

namespace simple_rsyncd {

ConnectionRateLimiter::ConnectionRateLimiter(size_t max_per_minute, size_t max_per_hour)
    : max_per_minute_(max_per_minute), max_per_hour_(max_per_hour) {}

bool ConnectionRateLimiter::incrementWindow(Window& window,
                                            size_t max_count,
                                            std::chrono::seconds period) {
    const auto now = std::chrono::steady_clock::now();
    if (window.start.time_since_epoch().count() == 0 ||
        (now - window.start) >= period) {
        window.start = now;
        window.count = 0;
    }

    if (max_count > 0 && window.count >= max_count) {
        return false;
    }

    ++window.count;
    return true;
}

bool ConnectionRateLimiter::allowConnection(const std::string& client_ip) {
    if (max_per_minute_ == 0 && max_per_hour_ == 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();

    if (max_per_minute_ > 0) {
        auto& minute = minute_windows_[client_ip];
        if (minute.start.time_since_epoch().count() == 0 ||
            (now - minute.start) >= std::chrono::seconds(60)) {
            minute.start = now;
            minute.count = 0;
        }
        if (minute.count >= max_per_minute_) {
            return false;
        }
    }

    if (max_per_hour_ > 0) {
        auto& hour = hour_windows_[client_ip];
        if (hour.start.time_since_epoch().count() == 0 ||
            (now - hour.start) >= std::chrono::seconds(3600)) {
            hour.start = now;
            hour.count = 0;
        }
        if (hour.count >= max_per_hour_) {
            return false;
        }
    }

    return true;
}

void ConnectionRateLimiter::recordConnection(const std::string& client_ip) {
    if (max_per_minute_ == 0 && max_per_hour_ == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (max_per_minute_ > 0) {
        incrementWindow(minute_windows_[client_ip], max_per_minute_, std::chrono::seconds(60));
    }
    if (max_per_hour_ > 0) {
        incrementWindow(hour_windows_[client_ip], max_per_hour_, std::chrono::seconds(3600));
    }
}

}  // namespace simple_rsyncd
