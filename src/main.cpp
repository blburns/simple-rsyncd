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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "simple_rsyncd/rsync_daemon.hpp"
#include "simple_rsyncd/configuration.hpp"
#include "simple_rsyncd/logger.hpp"

// Global daemon instance for signal handling
std::unique_ptr<simple_rsyncd::RSyncDaemon> g_daemon;
std::atomic<bool> g_shutdown_requested(false);

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number
 */
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    g_shutdown_requested = true;
    
    if (g_daemon) {
        g_daemon->stop();
    }
}

/**
 * @brief Print usage information
 */
void printUsage() {
    std::cout << "\nUsage: simple-rsyncd [OPTIONS] [COMMAND] [ARGS...]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --help, -h           Show this help message" << std::endl;
    std::cout << "  --version, -v        Show version information" << std::endl;
    std::cout << "  --config, -c FILE    Use specified configuration file" << std::endl;
    std::cout << "  --verbose, -V        Enable verbose logging" << std::endl;
    std::cout << "  --daemon, -d         Run as daemon" << std::endl;
    std::cout << "  --foreground, -f     Run in foreground (default)" << std::endl;
    std::cout << "  --pid-file FILE      Write PID to specified file" << std::endl;
    std::cout << "  --user USER          Run as specified user" << std::endl;
    std::cout << "  --group GROUP        Run as specified group" << std::endl;
    
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start                Start the rsync daemon" << std::endl;
    std::cout << "  stop                 Stop the rsync daemon" << std::endl;
    std::cout << "  restart              Restart the rsync daemon" << std::endl;
    std::cout << "  status               Show daemon status" << std::endl;
    std::cout << "  reload               Reload configuration" << std::endl;
    std::cout << "  test                 Test configuration" << std::endl;
    std::cout << "  modules              List available modules" << std::endl;
    std::cout << "  logs                 Show recent logs" << std::endl;
    std::cout << "  metrics              Show metrics" << std::endl;
    
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf" << std::endl;
    std::cout << "  simple-rsyncd start --daemon --config /etc/simple-rsyncd/rsyncd.conf" << std::endl;
    std::cout << "  simple-rsyncd status" << std::endl;
    std::cout << "  simple-rsyncd reload" << std::endl;
    std::cout << "  simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf" << std::endl;
    std::cout << "  simple-rsyncd modules" << std::endl;
    std::cout << "  simple-rsyncd logs --tail 100" << std::endl;
    std::cout << "  simple-rsyncd metrics --format json" << std::endl;
}

/**
 * @brief Print version information
 */
void printVersion() {
    std::cout << "simple-rsyncd v0.1.0" << std::endl;
    std::cout << "Simple RSync Daemon - A lightweight and secure rsync server" << std::endl;
    std::cout << "Copyright (c) 2024 simple-rsyncd contributors" << std::endl;
    std::cout << "License: Apache 2.0" << std::endl;
}

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param config_file Output configuration file path
 * @param command Output command
 * @param daemon_mode Output daemon mode flag
 * @param verbose Output verbose flag
 * @param pid_file Output PID file path
 * @param user Output user name
 * @param group Output group name
 * @return true if parsed successfully, false otherwise
 */
bool parseArguments(int argc, char* argv[], std::string& config_file, std::string& command,
                   bool& daemon_mode, bool& verbose, std::string& pid_file,
                   std::string& user, std::string& group) {
    config_file.clear();
    command.clear();
    daemon_mode = false;
    verbose = false;
    pid_file.clear();
    user.clear();
    group.clear();
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return false;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return false;
            }
        } else if (arg == "--verbose" || arg == "-V") {
            verbose = true;
        } else if (arg == "--daemon" || arg == "-d") {
            daemon_mode = true;
        } else if (arg == "--foreground" || arg == "-f") {
            daemon_mode = false;
        } else if (arg == "--pid-file") {
            if (i + 1 < argc) {
                pid_file = argv[++i];
            } else {
                std::cerr << "Error: --pid-file requires a file path" << std::endl;
                return false;
            }
        } else if (arg == "--user") {
            if (i + 1 < argc) {
                user = argv[++i];
            } else {
                std::cerr << "Error: --user requires a username" << std::endl;
                return false;
            }
        } else if (arg == "--group") {
            if (i + 1 < argc) {
                group = argv[++i];
            } else {
                std::cerr << "Error: --group requires a group name" << std::endl;
                return false;
            }
        } else if (arg[0] != '-') {
            // This is a command
            command = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage();
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Set default configuration file path
 * @param config_file Configuration file path (can be empty)
 * @return Default configuration file path
 */
std::string getDefaultConfigFile(const std::string& config_file) {
    if (!config_file.empty()) {
        return config_file;
    }
    
    // Try common configuration locations
    std::vector<std::string> config_paths = {
        "/etc/simple-rsyncd/rsyncd.conf",
        "/usr/local/etc/simple-rsyncd/rsyncd.conf",
        "/opt/simple-rsyncd/etc/rsyncd.conf",
        "./rsyncd.conf",
        "./config/rsyncd.conf"
    };
    
    for (const auto& path : config_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    // Return default path
    return "/etc/simple-rsyncd/rsyncd.conf";
}

/**
 * @brief Write PID to file
 * @param pid_file PID file path
 * @return true if written successfully, false otherwise
 */
bool writePidFile(const std::string& pid_file) {
    if (pid_file.empty()) {
        return true;
    }
    
    try {
        std::ofstream file(pid_file);
        if (file.is_open()) {
            file << getpid() << std::endl;
            file.close();
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing PID file: " << e.what() << std::endl;
    }
    
    return false;
}

/**
 * @brief Remove PID file
 * @param pid_file PID file path
 */
void removePidFile(const std::string& pid_file) {
    if (!pid_file.empty() && std::filesystem::exists(pid_file)) {
        try {
            std::filesystem::remove(pid_file);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not remove PID file: " << e.what() << std::endl;
        }
    }
}

/**
 * @brief Drop privileges to specified user/group
 * @param user User name
 * @param group Group name
 * @return true if successful, false otherwise
 */
bool dropPrivileges(const std::string& user, const std::string& group) {
    if (user.empty() && group.empty()) {
        return true;
    }
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    if (!user.empty()) {
        std::cout << "Would drop privileges to user: " << user << std::endl;
    }
    if (!group.empty()) {
        std::cout << "Would drop privileges to group: " << group << std::endl;
    }
    
    return true;
}

/**
 * @brief Daemonize the process
 * @return true if successful, false otherwise
 */
bool daemonize() {
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would daemonize the process" << std::endl;
    return true;
}

/**
 * @brief Handle start command
 * @param config_file Configuration file path
 * @param daemon_mode Whether to run in daemon mode
 * @param verbose Whether to enable verbose logging
 * @param pid_file PID file path
 * @param user User name
 * @param group Group name
 * @return Exit code
 */
int handleStart(const std::string& config_file, bool daemon_mode, bool verbose,
                const std::string& pid_file, const std::string& user, const std::string& group) {
    (void)verbose; // Suppress unused parameter warning
    std::cout << "Starting simple-rsyncd..." << std::endl;
    
    // Load configuration
    auto config = std::make_shared<simple_rsyncd::Configuration>();
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration from " << config_file << std::endl;
        std::cerr << "Configuration errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    // Validate configuration
    if (!config->validate()) {
        std::cerr << "Error: Configuration validation failed" << std::endl;
        std::cerr << "Configuration errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    std::cout << "Configuration loaded successfully" << std::endl;
    
    // Drop privileges if specified
    if (!dropPrivileges(user, group)) {
        std::cerr << "Error: Failed to drop privileges" << std::endl;
        return 1;
    }
    
    // Daemonize if requested
    if (daemon_mode) {
        if (!daemonize()) {
            std::cerr << "Error: Failed to daemonize" << std::endl;
            return 1;
        }
    }
    
    // Write PID file
    if (!writePidFile(pid_file)) {
        std::cerr << "Warning: Failed to write PID file" << std::endl;
    }
    
    // Create and start daemon
    try {
        g_daemon = std::make_unique<simple_rsyncd::RSyncDaemon>(config);
        
        if (!g_daemon->start()) {
            std::cerr << "Error: Failed to start daemon" << std::endl;
            removePidFile(pid_file);
            return 1;
        }
        
        std::cout << "Daemon started successfully" << std::endl;
        
        // Main event loop
        while (!g_shutdown_requested && g_daemon->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Shutting down daemon..." << std::endl;
        g_daemon->stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        removePidFile(pid_file);
        return 1;
    }
    
    removePidFile(pid_file);
    std::cout << "Daemon stopped" << std::endl;
    
    return 0;
}

/**
 * @brief Handle stop command
 * @return Exit code
 */
int handleStop() {
    std::cout << "Stopping simple-rsyncd..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would stop the daemon" << std::endl;
    
    return 0;
}

/**
 * @brief Handle restart command
 * @param config_file Configuration file path
 * @param daemon_mode Whether to run in daemon mode
 * @param verbose Whether to enable verbose logging
 * @param pid_file PID file path
 * @param user User name
 * @param group Group name
 * @return Exit code
 */
int handleRestart(const std::string& config_file, bool daemon_mode, bool verbose,
                  const std::string& pid_file, const std::string& user, const std::string& group) {
    std::cout << "Restarting simple-rsyncd..." << std::endl;
    
    // Stop first
    int stop_result = handleStop();
    if (stop_result != 0) {
        std::cerr << "Warning: Failed to stop daemon, continuing with restart" << std::endl;
    }
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Start again
    return handleStart(config_file, daemon_mode, verbose, pid_file, user, group);
}

/**
 * @brief Handle status command
 * @return Exit code
 */
int handleStatus() {
    std::cout << "Checking simple-rsyncd status..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would check daemon status" << std::endl;
    
    return 0;
}

/**
 * @brief Handle reload command
 * @return Exit code
 */
int handleReload() {
    std::cout << "Reloading simple-rsyncd configuration..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would reload configuration" << std::endl;
    
    return 0;
}

/**
 * @brief Handle test command
 * @param config_file Configuration file path
 * @return Exit code
 */
int handleTest(const std::string& config_file) {
    std::cout << "Testing simple-rsyncd configuration..." << std::endl;
    
    // Load configuration
    auto config = std::make_shared<simple_rsyncd::Configuration>();
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration from " << config_file << std::endl;
        std::cerr << "Configuration errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    // Validate configuration
    if (!config->validate()) {
        std::cerr << "Error: Configuration validation failed" << std::endl;
        std::cerr << "Configuration errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    std::cout << "Configuration is valid" << std::endl;
    
    // Test modules
    std::cout << "Testing modules..." << std::endl;
    for (const auto& [name, module_config] : config->modules) {
        std::cout << "  Module: " << name << std::endl;
        std::cout << "    Path: " << module_config.path << std::endl;
        std::cout << "    Comment: " << module_config.comment << std::endl;
        std::cout << "    Read-only: " << (module_config.read_only ? "yes" : "no") << std::endl;
        std::cout << "    List: " << (module_config.list ? "yes" : "no") << std::endl;
        std::cout << "    Delete: " << (module_config.allow_delete ? "yes" : "no") << std::endl;
        std::cout << "    Overwrite: " << (module_config.overwrite ? "yes" : "no") << std::endl;
        
        // Check if path exists
        if (std::filesystem::exists(module_config.path)) {
            std::cout << "    Path exists: yes" << std::endl;
            std::cout << "    Path type: " << (std::filesystem::is_directory(module_config.path) ? "directory" : "file") << std::endl;
        } else {
            std::cout << "    Path exists: no" << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "Configuration test completed successfully" << std::endl;
    return 0;
}

/**
 * @brief Handle modules command
 * @return Exit code
 */
int handleModules() {
    std::cout << "Listing simple-rsyncd modules..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would list available modules" << std::endl;
    
    return 0;
}

/**
 * @brief Handle logs command
 * @return Exit code
 */
int handleLogs() {
    std::cout << "Showing simple-rsyncd logs..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would show recent logs" << std::endl;
    
    return 0;
}

/**
 * @brief Handle metrics command
 * @return Exit code
 */
int handleMetrics() {
    std::cout << "Showing simple-rsyncd metrics..." << std::endl;
    
    // Implementation would depend on platform-specific code
    // For now, just log the intention
    std::cout << "Would show metrics" << std::endl;
    
    return 0;
}

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string config_file, command, pid_file, user, group;
    bool daemon_mode = false, verbose = false;
    
    if (!parseArguments(argc, argv, config_file, command, daemon_mode, verbose, pid_file, user, group)) {
        return 0; // Help or version was printed
    }
    
    // Set default configuration file if none specified
    config_file = getDefaultConfigFile(config_file);
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGHUP, signalHandler);
    
    // Handle commands
    if (command == "start") {
        return handleStart(config_file, daemon_mode, verbose, pid_file, user, group);
    } else if (command == "stop") {
        return handleStop();
    } else if (command == "restart") {
        return handleRestart(config_file, daemon_mode, verbose, pid_file, user, group);
    } else if (command == "status") {
        return handleStatus();
    } else if (command == "reload") {
        return handleReload();
    } else if (command == "test") {
        return handleTest(config_file);
    } else if (command == "modules") {
        return handleModules();
    } else if (command == "logs") {
        return handleLogs();
    } else if (command == "metrics") {
        return handleMetrics();
    } else if (command.empty()) {
        std::cerr << "Error: No command specified" << std::endl;
        printUsage();
        return 1;
    } else {
        std::cerr << "Error: Unknown command: " << command << std::endl;
        printUsage();
        return 1;
    }
    
    return 0;
}
