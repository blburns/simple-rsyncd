#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "simple_rsyncd/configuration.hpp"
#include "simple_rsyncd/rsync_session.hpp"
#include "simple_rsyncd/logger.hpp"
#include "simple_rsyncd/ssl_context.hpp"

namespace simple_rsyncd {

/**
 * @brief RSync Daemon - Main server class for handling rsync connections
 * 
 * This class implements a full-featured rsync daemon that can:
 * - Handle multiple concurrent connections
 * - Support SSL/TLS encryption
 * - Manage multiple modules and paths
 * - Provide authentication and access control
 * - Support configuration overlays and hot-reloading
 */
class RSyncDaemon {
public:
    /**
     * @brief Constructor
     * @param config Configuration object for the daemon
     */
    explicit RSyncDaemon(std::shared_ptr<Configuration> config);
    
    /**
     * @brief Destructor
     */
    ~RSyncDaemon();
    
    /**
     * @brief Start the daemon
     * @return true if started successfully, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop the daemon
     */
    void stop();
    
    /**
     * @brief Check if daemon is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Reload configuration
     * @return true if reloaded successfully, false otherwise
     */
    bool reloadConfig();
    
    /**
     * @brief Get daemon status information
     * @return Status information as a string
     */
    std::string getStatus() const;
    
    /**
     * @brief Get active connection count
     * @return Number of active connections
     */
    size_t getActiveConnections() const;
    
    /**
     * @brief Get total connection count since start
     * @return Total number of connections handled
     */
    size_t getTotalConnections() const;
    
    /**
     * @brief Get daemon uptime
     * @return Uptime in seconds
     */
    time_t getUptime() const;
    
    /**
     * @brief Get daemon version
     * @return Version string
     */
    std::string getVersion() const;

private:
    // Configuration
    std::shared_ptr<Configuration> config_;
    
    // SSL context
    std::unique_ptr<SSLContext> ssl_context_;
    
    // Network components
    int listen_socket_;
    std::string bind_address_;
    uint16_t bind_port_;
    
    // Threading
    std::thread accept_thread_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_requested_;
    
    // Connection management
    std::vector<std::unique_ptr<RSyncSession>> active_sessions_;
    mutable std::mutex sessions_mutex_;
    std::atomic<size_t> total_connections_;
    
    // Timing
    std::chrono::steady_clock::time_point start_time_;
    
    // Logger
    std::shared_ptr<Logger> logger_;
    
    // Internal methods
    bool initializeNetwork();
    bool initializeSSL();
    void acceptLoop();
    void workerLoop();
    void handleConnection(int client_socket, const std::string& client_address);
    void cleanupSessions();
    bool validateClient(const std::string& client_address);
    void logAccess(const std::string& client_address, const std::string& module, bool allowed);
    
    // Configuration validation
    bool validateConfiguration() const;
    bool validateModules() const;
    bool validatePaths() const;
    bool validateSSL() const;
    
    // Signal handling
    void setupSignalHandlers();
    static void signalHandler(int signal);
    
    // Statistics
    mutable std::mutex stats_mutex_;
    struct Statistics {
        size_t bytes_transferred;
        size_t files_transferred;
        size_t failed_transfers;
        size_t authentication_failures;
        size_t access_denied;
        std::map<std::string, size_t> module_usage;
        std::map<std::string, size_t> client_usage;
    } stats_;
    
    // Update statistics
    void updateStats(const std::string& module, const std::string& client, 
                    size_t bytes, size_t files, bool success);
    
    // Configuration hot-reload
    std::thread config_watcher_thread_;
    void configWatcherLoop();
    bool hasConfigChanged() const;
    bool reloadModules();
    
    // Thread management
    void startWorkerThreads();
    void startConfigWatcher();
    void startHealthMonitor();
    
    // Module management
    std::map<std::string, std::shared_ptr<Module>> modules_;
    mutable std::mutex modules_mutex_;
    bool loadModules();
    bool unloadModules();
    std::shared_ptr<Module> findModule(const std::string& name) const;
    
    // Access control
    bool checkAccess(const std::string& client_address, const std::string& module_name) const;
    bool checkAuthentication(const std::string& username, const std::string& password) const;
    bool checkPermissions(const std::string& username, const std::string& module_name, 
                        const std::string& operation) const;
    
    // Rate limiting
    struct RateLimit {
        std::chrono::steady_clock::time_point last_check;
        size_t request_count;
        size_t max_requests;
        std::chrono::seconds window;
    };
    mutable std::map<std::string, RateLimit> rate_limits_;
    mutable std::mutex rate_limit_mutex_;
    bool checkRateLimit(const std::string& client_address) const;
    void updateRateLimit(const std::string& client_address);
    
    // Health monitoring
    std::thread health_monitor_thread_;
    void healthMonitorLoop();
    bool checkHealth() const;
    void reportHealth();
    
    // Metrics collection
    void collectMetrics();
    std::string getMetricsJSON() const;
    
    // Cleanup
    void cleanup();
    void joinThreads();
    void closeSessions();
    
    // Disable copy and assignment
    RSyncDaemon(const RSyncDaemon&) = delete;
    RSyncDaemon& operator=(const RSyncDaemon&) = delete;
};

} // namespace simple_rsyncd
