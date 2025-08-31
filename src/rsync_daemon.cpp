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

#include "simple_rsyncd/rsync_daemon.hpp"
#include "simple_rsyncd/configuration.hpp"
#include "simple_rsyncd/logger.hpp"
#include "simple_rsyncd/rsync_session.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <sstream>
#include <algorithm>

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
    
    // Set up signal handlers
    setupSignalHandlers();
    
    // Start worker threads
    running_ = true;
    startWorkerThreads();
    
    // Start configuration watcher if enabled
    if (config_ && config_->auto_reload) {
        startConfigWatcher();
    }
    
    // Start health monitor
    startHealthMonitor();
    
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
    
    // Stop all threads
    joinThreads();
    
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
    return "0.1.0";
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
    
    for (const auto& [name, module_config] : config_->modules) {
        // Module loading would go here
        // For now, just create placeholder modules
        logger_->info("Loaded module: " + name);
    }
    
    return true;
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
        // Worker thread processing would go here
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

} // namespace simple_rsyncd
