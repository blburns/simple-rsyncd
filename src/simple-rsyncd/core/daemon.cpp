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

#include "simple-rsyncd/core/daemon.hpp"
#include "simple-rsyncd/config/config.hpp"
#include "simple-rsyncd/utils/logger.hpp"
#include "simple-rsyncd/core/session.hpp"
#include "simple-rsyncd/core/module.hpp"
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/security/network_access.hpp"
#include "simple-rsyncd/security/privileges.hpp"
#include "simple-rsyncd/security/rate_limiter.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

namespace simple_rsyncd {

RSyncDaemon::RSyncDaemon(std::shared_ptr<Configuration> config)
    : config_(config)
    , listen_socket_(-1)
    , bind_address_("0.0.0.0")
    , bind_port_(873)
    , running_(false)
    , shutdown_requested_(false)
    , total_connections_(0)
    , start_time_(std::chrono::steady_clock::now())
    , logger_(std::make_shared<Logger>()) {

    if (config_) {
        bind_address_ = config_->network.bind_address;
        bind_port_ = config_->network.bind_port;

        // Initialize authentication manager
        if (config_->auth.enabled) {
            auth_manager_ = std::make_unique<AuthenticationManager>(config_->auth);
            logger_->info("Authentication enabled: " + config_->auth.method);
        }
    }

    // Initialize statistics
    stats_.bytes_transferred = 0;
    stats_.files_transferred = 0;
    stats_.failed_transfers = 0;
    stats_.authentication_failures = 0;
    stats_.access_denied = 0;
}

RSyncDaemon::~RSyncDaemon() {
    stop();
}

bool RSyncDaemon::start() {
    if (running_) {
        logger_->warn("Daemon is already running");
        return true;
    }

    logger_->info("Starting simple-rsyncd daemon");

    // Validate configuration
    if (!validateConfiguration()) {
        logger_->error("Configuration validation failed");
        return false;
    }

    // Load modules
    if (!loadModules()) {
        logger_->error("Failed to load modules");
        return false;
    }

    // Initialize SSL when enabled
    if (!initializeSSL()) {
        logger_->error("Failed to initialize SSL/TLS");
        return false;
    }

    // Initialize network
    if (!initializeNetwork()) {
        logger_->error("Failed to initialize network");
        return false;
    }

    // Optional chroot (requires root, before privilege drop)
    if (config_ && config_->security.chroot_enabled && !config_->security.chroot_directory.empty()) {
        if (!enterChroot(config_->security.chroot_directory)) {
            logger_->error("Failed to enter chroot: " + config_->security.chroot_directory);
            return false;
        }
        logger_->info("Entered chroot: " + config_->security.chroot_directory);
    }

    // Drop privileges after bind (requires root for setuid)
    if (config_ && config_->security.drop_privileges) {
        if (!dropProcessPrivileges(config_->security.user, config_->security.group)) {
            logger_->error("Failed to drop privileges");
            return false;
        }
        if (!config_->security.user.empty() || !config_->security.group.empty()) {
            logger_->info("Dropped privileges to user=" + config_->security.user +
                          " group=" + config_->security.group);
        }
    }

    if (config_ && config_->rate_limit.enabled) {
        rate_limiter_ = std::make_unique<ConnectionRateLimiter>(
            config_->rate_limit.max_connections_per_minute,
            config_->rate_limit.max_connections_per_hour);
        logger_->info("Connection rate limiting enabled");
    }

    // Set up signal handlers
    setupSignalHandlers();

    // Start accept thread
    running_ = true;
    accept_thread_ = std::thread(&RSyncDaemon::acceptLoop, this);

    // Start worker threads
    startWorkerThreads();

    // Start configuration monitor thread if enabled
    if (config_ && config_->auto_reload) {
        config_monitor_thread_ = std::thread(&RSyncDaemon::configMonitorLoop, this);
        logger_->info("Configuration auto-reload enabled");
    }

    logger_->info("Daemon started successfully");
    return true;
}

void RSyncDaemon::stop() {
    if (!running_) {
        return;
    }

    logger_->info("Stopping simple-rsyncd daemon");

    shutdown_requested_ = true;
    running_ = false;

    // Close listen socket
    if (listen_socket_ >= 0) {
        close(listen_socket_);
        listen_socket_ = -1;
        logger_->info("Listen socket closed");
    }

    // Join accept thread
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Join config monitor thread
    if (config_monitor_thread_.joinable()) {
        config_monitor_thread_.join();
    }

    // Stop all threads
    joinThreads();

    // Close all sessions
    closeSessions();

    // Clean up sessions
    cleanupSessions();

    // Unload modules
    unloadModules();

    logger_->info("Daemon stopped");
}

bool RSyncDaemon::isRunning() const {
    return running_;
}

bool RSyncDaemon::reloadConfig() {
    logger_->info("Reloading configuration");

    if (!config_) {
        logger_->error("No configuration available for reload");
        return false;
    }

    // Reload configuration
    if (!config_->reload()) {
        logger_->error("Failed to reload configuration");
        return false;
    }

    // Validate new configuration
    if (!validateConfiguration()) {
        logger_->error("New configuration validation failed");
        return false;
    }

    // Reload modules
    if (!reloadModules()) {
        logger_->error("Failed to reload modules");
        return false;
    }

    logger_->info("Configuration reloaded successfully");
    return true;
}

std::string RSyncDaemon::getStatus() const {
    std::stringstream ss;
    ss << "Simple RSync Daemon Status\n";
    ss << "==========================\n";
    ss << "Running: " << (running_ ? "Yes" : "No") << "\n";
    ss << "Bind Address: " << bind_address_ << ":" << bind_port_ << "\n";
    ss << "Active Connections: " << getActiveConnections() << "\n";
    ss << "Total Connections: " << getTotalConnections() << "\n";
    ss << "Uptime: " << getUptime() << " seconds\n";
    ss << "Version: " << getVersion() << "\n";

    // Module status
    ss << "\nModules:\n";
    std::lock_guard<std::mutex> lock(modules_mutex_);
    for (const auto& [name, module] : modules_) {
        ss << "  " << name << ": " << (module ? "Active" : "Inactive") << "\n";
    }

    return ss.str();
}

size_t RSyncDaemon::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return active_sessions_.size();
}

size_t RSyncDaemon::getTotalConnections() const {
    return total_connections_;
}

time_t RSyncDaemon::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return duration.count();
}

std::string RSyncDaemon::getVersion() const {
    return "0.4.0";
}

bool RSyncDaemon::initializeSSL() {
    if (!config_ || !config_->ssl.enabled) {
        return true;
    }

    ssl_context_ = std::make_unique<SSLContext>();
    if (!ssl_context_->initialize(config_->ssl.certificate_file,
                                  config_->ssl.private_key_file,
                                  config_->ssl.ca_file,
                                  config_->ssl.tls_version,
                                  config_->ssl.cipher_suite)) {
        logger_->error("SSL initialization failed: " + ssl_context_->lastError());
        return false;
    }

    logger_->info("SSL/TLS enabled (min TLS " + config_->ssl.tls_version + ")");
    return true;
}

bool RSyncDaemon::checkRateLimit(const std::string& client_address) const {
    if (!config_ || !config_->rate_limit.enabled || !rate_limiter_) {
        return true;
    }
    return rate_limiter_->allowConnection(client_address);
}

void RSyncDaemon::updateRateLimit(const std::string& client_address) {
    if (!config_ || !config_->rate_limit.enabled || !rate_limiter_) {
        return;
    }
    rate_limiter_->recordConnection(client_address);
}

// Private methods implementation

bool RSyncDaemon::validateConfiguration() const {
    if (!config_) {
        return false;
    }

    return config_->validate();
}

bool RSyncDaemon::loadModules() {
    if (!config_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(modules_mutex_);
    modules_.clear();

    for (const auto& [name, module_config] : config_->modules) {
        try {
            ModuleConfig secured_config = module_config;
            secured_config.allow_symlinks = config_->security.allow_symlinks;
            secured_config.allow_hardlinks = config_->security.allow_hardlinks;
            auto module = createModule(secured_config);
            if (module && module->validate()) {
                modules_[name] = module;
                logger_->info("Loaded module: " + name + " at " + module_config.path);
            } else {
                logger_->warn("Failed to load module: " + name);
                auto errors = module->getErrors();
                for (const auto& error : errors) {
                    logger_->warn("  " + error);
                }
            }
        } catch (const std::exception& e) {
            logger_->error("Exception loading module " + name + ": " + e.what());
        }
    }

    return !modules_.empty();
}

bool RSyncDaemon::unloadModules() {
    std::lock_guard<std::mutex> lock(modules_mutex_);
    modules_.clear();
    logger_->info("All modules unloaded");
    return true;
}

bool RSyncDaemon::reloadModules() {
    return loadModules();
}

void RSyncDaemon::setupSignalHandlers() {
    // Signal handling setup would go here
    logger_->info("Signal handlers configured");
}

void RSyncDaemon::startWorkerThreads() {
    size_t num_workers = config_ ? config_->network.worker_threads : 4;

    for (size_t i = 0; i < num_workers; ++i) {
        worker_threads_.emplace_back(&RSyncDaemon::workerLoop, this);
    }

    logger_->info("Started " + std::to_string(num_workers) + " worker threads");
}

void RSyncDaemon::workerLoop() {
    while (running_ && !shutdown_requested_) {
        // Process sessions
        std::vector<std::unique_ptr<RSyncSession>> sessions_to_process;

        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            // Move active sessions to processing list
            for (auto it = active_sessions_.begin(); it != active_sessions_.end();) {
                if (*it && (*it)->isActive()) {
                    sessions_to_process.push_back(std::move(*it));
                    it = active_sessions_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Process each session
        for (auto& session : sessions_to_process) {
            if (session && session->isActive()) {
                try {
                    if (!session->processRequest()) {
                        session->close();
                    }
                } catch (const std::exception& e) {
                    logger_->error("Error processing session: " + std::string(e.what()));
                    session->close();
                }
            }
        }

        // Return processed sessions (if still active)
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& session : sessions_to_process) {
                if (session && session->isActive()) {
                    active_sessions_.push_back(std::move(session));
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RSyncDaemon::startConfigWatcher() {
    config_watcher_thread_ = std::thread(&RSyncDaemon::configWatcherLoop, this);
    logger_->info("Configuration watcher started");
}

void RSyncDaemon::configWatcherLoop() {
    while (running_ && !shutdown_requested_) {
        if (hasConfigChanged()) {
            logger_->info("Configuration change detected, reloading...");
            reloadConfig();
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void RSyncDaemon::startHealthMonitor() {
    health_monitor_thread_ = std::thread(&RSyncDaemon::healthMonitorLoop, this);
    logger_->info("Health monitor started");
}

void RSyncDaemon::healthMonitorLoop() {
    while (running_ && !shutdown_requested_) {
        if (!checkHealth()) {
            logger_->warn("Health check failed");
        }

        std::this_thread::sleep_for(std::chrono::seconds(300));
    }
}

bool RSyncDaemon::hasConfigChanged() const {
    // Configuration change detection would go here
    return false;
}

bool RSyncDaemon::checkHealth() const {
    // Health checking would go here
    return true;
}

void RSyncDaemon::joinThreads() {
    // Join all threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    if (config_watcher_thread_.joinable()) {
        config_watcher_thread_.join();
    }

    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }

    logger_->info("All threads joined");
}

void RSyncDaemon::cleanupSessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    active_sessions_.clear();
    logger_->info("All sessions cleaned up");
}

void RSyncDaemon::closeSessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& session : active_sessions_) {
        if (session) {
            session->close();
        }
    }
    logger_->info("All sessions closed");
}

bool RSyncDaemon::checkAuthentication(const std::string& username, const std::string& password) const {
    if (!config_ || !config_->auth.enabled) {
        // If auth is disabled, allow if anonymous access is enabled
        return config_ && config_->auth.anonymous_access;
    }

    if (!auth_manager_) {
        return false;
    }

    bool authenticated = auth_manager_->authenticateUser(username, password);

    if (!authenticated) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.authentication_failures++;
    }

    return authenticated;
}

bool RSyncDaemon::checkAccess(const std::string& client_address, const std::string& module_name) const {
    (void)module_name;
    if (!config_ || !config_->access.enabled) {
        return true;
    }

    if (!NetworkAccess::isAllowed(client_address,
                                  config_->access.denied_hosts,
                                  config_->access.denied_networks,
                                  config_->access.allowed_hosts,
                                  config_->access.allowed_networks)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.access_denied++;
        logAccess(client_address, module_name, false);
        return false;
    }

    logAccess(client_address, module_name, true);
    return true;
}

void RSyncDaemon::logAccess(const std::string& client_address,
                            const std::string& module,
                            bool allowed) const {
    if (!logger_) {
        return;
    }
    logger_->info(std::string("access ") + (allowed ? "allowed" : "denied") +
                  " client=" + client_address +
                  (module.empty() ? "" : " module=" + module));
}

bool RSyncDaemon::validateClient(const std::string& client_address) {
    return checkAccess(client_address, "") && checkRateLimit(client_address);
}

bool RSyncDaemon::checkPermissions(const std::string& username, const std::string& module_name,
                                   const std::string& operation) const {
    // Basic permission checking
    // TODO: Implement more sophisticated permission system

    std::lock_guard<std::mutex> lock(modules_mutex_);
    auto it = modules_.find(module_name);
    if (it == modules_.end()) {
        return false;
    }

    auto module = it->second;
    if (!module) {
        return false;
    }

    // Check operation-specific permissions
    if (operation == "read" || operation == "list") {
        return module->allowsListing();
    } else if (operation == "write" || operation == "upload") {
        return !module->isReadOnly() && module->allowsOverwriting();
    } else if (operation == "delete") {
        return !module->isReadOnly() && module->allowsDeletion();
    }

    return false;
}

bool RSyncDaemon::initializeNetwork() {
    if (!config_) {
        logger_->error("Configuration not set");
        return false;
    }

    bind_address_ = config_->network.bind_address;
    bind_port_ = config_->network.bind_port;

    // Create socket
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        logger_->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_->error("Failed to set socket options: " + std::string(strerror(errno)));
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    // Set non-blocking
    int flags = fcntl(listen_socket_, F_GETFL, 0);
    if (flags < 0 || fcntl(listen_socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
        logger_->error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(bind_port_);

    if (bind_address_ == "0.0.0.0" || bind_address_.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_aton(bind_address_.c_str(), &addr.sin_addr) == 0) {
            logger_->error("Invalid bind address: " + bind_address_);
            close(listen_socket_);
            listen_socket_ = -1;
            return false;
        }
    }

    if (bind(listen_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        logger_->error("Failed to bind socket: " + std::string(strerror(errno)));
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    // Listen
    int backlog = config_->network.backlog > 0 ? config_->network.backlog : 128;
    if (listen(listen_socket_, backlog) < 0) {
        logger_->error("Failed to listen on socket: " + std::string(strerror(errno)));
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    logger_->info("Network initialized: listening on " + bind_address_ + ":" + std::to_string(bind_port_));
    return true;
}

void RSyncDaemon::acceptLoop() {
    logger_->info("Accept loop started");

    while (running_ && !shutdown_requested_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        memset(&client_addr, 0, client_len);

        int client_socket = accept(listen_socket_, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection available, continue
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } else if (errno == EINTR) {
                // Interrupted, continue
                continue;
            } else {
                if (running_ && !shutdown_requested_) {
                    logger_->error("Accept failed: " + std::string(strerror(errno)));
                }
                break;
            }
        }

        // Get client address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::string client_address = std::string(client_ip);

        // Check access control and rate limits
        if (!checkAccess(client_address, "")) {
            logger_->warn("Access denied for " + client_address);
            close(client_socket);
            continue;
        }

        if (!checkRateLimit(client_address)) {
            logger_->warn("Rate limit exceeded for " + client_address);
            close(client_socket);
            continue;
        }

        // Create session
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            if (active_sessions_.size() >= config_->network.max_connections) {
                logger_->warn("Max connections reached, rejecting " + client_address);
                close(client_socket);
                continue;
            }
        }

        updateRateLimit(client_address);

        // Get modules map (need to lock modules_mutex_)
        std::map<std::string, std::shared_ptr<Module>> modules_copy;
        {
            std::lock_guard<std::mutex> lock(modules_mutex_);
            modules_copy = modules_;
        }

        AuthenticationManager* auth_ptr = auth_manager_ ? auth_manager_.get() : nullptr;

        try {
            std::unique_ptr<RSyncSession> session;
            if (config_->ssl.enabled && ssl_context_ && ssl_context_->isInitialized()) {
                auto ssl_conn = ssl_context_->acceptConnection(client_socket);
                if (!ssl_conn) {
                    logger_->error("TLS handshake failed for " + client_address + ": " +
                                   ssl_context_->lastError());
                    close(client_socket);
                    continue;
                }
                session = std::make_unique<RSyncSession>(client_socket, client_address, config_,
                                                         modules_copy, auth_ptr, std::move(ssl_conn));
            } else {
                session = std::make_unique<RSyncSession>(client_socket, client_address, config_,
                                                         modules_copy, auth_ptr);
            }
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                active_sessions_.push_back(std::move(session));
                total_connections_++;
            }
            logger_->info("Accepted connection from " + client_address);
        } catch (const std::exception& e) {
            logger_->error("Failed to create session: " + std::string(e.what()));
            close(client_socket);
        }
    }

    logger_->info("Accept loop stopped");
}

void RSyncDaemon::configMonitorLoop() {
    if (!config_) {
        return;
    }

    logger_->info("Configuration monitor thread started");

    while (running_ && !shutdown_requested_) {
        std::this_thread::sleep_for(config_->reload_interval);

        if (shutdown_requested_) {
            break;
        }

        // Check if configuration file has changed
        if (config_->hasChanged()) {
            logger_->info("Configuration file changed, reloading...");

            if (reloadConfig()) {
                logger_->info("Configuration reloaded successfully");
            } else {
                logger_->error("Failed to reload configuration, keeping existing configuration");
            }
        }
    }

    logger_->info("Configuration monitor thread stopped");
}

} // namespace simple_rsyncd
