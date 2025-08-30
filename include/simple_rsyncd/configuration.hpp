#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include "simple_rsyncd/module.hpp"

namespace simple_rsyncd {

/**
 * @brief SSL/TLS configuration options
 */
struct SSLConfig {
    bool enabled = false;
    std::string certificate_file;
    std::string private_key_file;
    std::string ca_file;
    std::string cipher_suite;
    bool require_client_cert = false;
    std::string tls_version = "1.2";
    bool verify_peer = true;
};

/**
 * @brief Authentication configuration
 */
struct AuthConfig {
    bool enabled = false;
    std::string method = "password"; // password, public_key, oauth2
    std::string realm = "simple-rsyncd";
    std::string password_file;
    std::string public_key_file;
    std::string oauth2_config_file;
    bool anonymous_access = false;
    std::vector<std::string> allowed_users;
    std::vector<std::string> denied_users;
};

/**
 * @brief Access control configuration
 */
struct AccessConfig {
    bool enabled = false;
    std::vector<std::string> allowed_hosts;
    std::vector<std::string> denied_hosts;
    std::vector<std::string> allowed_networks;
    std::vector<std::string> denied_networks;
    bool reverse_dns_lookup = false;
    std::string access_log_file;
    std::string access_log_format = "combined";
};

/**
 * @brief Rate limiting configuration
 */
struct RateLimitConfig {
    bool enabled = false;
    size_t max_connections_per_minute = 60;
    size_t max_connections_per_hour = 1000;
    size_t max_bandwidth_per_connection = 0; // 0 = unlimited
    size_t max_bandwidth_total = 0; // 0 = unlimited
    std::chrono::seconds connection_timeout{300};
    std::chrono::seconds idle_timeout{60};
};

/**
 * @brief Logging configuration
 */
struct LogConfig {
    std::string level = "info";
    std::string file;
    std::string format = "{timestamp} [{level}] {message}";
    bool console_output = true;
    bool file_output = false;
    bool syslog_output = false;
    std::string syslog_facility = "daemon";
    size_t max_file_size = 10 * 1024 * 1024; // 10MB
    size_t max_files = 5;
    bool compress_old_logs = true;
};

/**
 * @brief Network configuration
 */
struct NetworkConfig {
    std::string bind_address = "0.0.0.0";
    uint16_t bind_port = 873;
    int backlog = 128;
    bool reuse_address = true;
    bool keep_alive = true;
    std::chrono::seconds keep_alive_time{7200};
    std::chrono::seconds keep_alive_interval{75};
    int keep_alive_probes{9};
    size_t max_connections = 100;
    size_t worker_threads = 4;
};

// ModuleConfig is defined in module.hpp

/**
 * @brief Security configuration
 */
struct SecurityConfig {
    bool chroot_enabled = false;
    std::string chroot_directory;
    std::string user;
    std::string group;
    bool drop_privileges = true;
    std::vector<std::string> allowed_commands;
    std::vector<std::string> denied_commands;
    bool restrict_to_module_path = true;
    bool allow_symlinks = false;
    bool allow_hardlinks = false;
    bool allow_devices = false;
    bool allow_sockets = false;
    std::string umask = "022";
};

/**
 * @brief Performance configuration
 */
struct PerformanceConfig {
    size_t buffer_size = 64 * 1024; // 64KB
    size_t max_memory_usage = 512 * 1024 * 1024; // 512MB
    bool enable_compression = true;
    std::string compression_level = "6";
    bool enable_checksum_verification = true;
    std::string checksum_algorithm = "md5";
    size_t max_concurrent_transfers = 10;
    bool enable_pipelining = true;
    size_t pipeline_depth = 5;
};

/**
 * @brief Monitoring configuration
 */
struct MonitoringConfig {
    bool enabled = false;
    std::string metrics_file;
    std::string health_check_file;
    std::chrono::seconds metrics_interval{60};
    std::chrono::seconds health_check_interval{300};
    bool enable_prometheus_metrics = false;
    std::string prometheus_endpoint = "/metrics";
    uint16_t prometheus_port = 9090;
    std::string prometheus_address = "127.0.0.1";
};

/**
 * @brief Main configuration class
 */
class Configuration {
public:
    /**
     * @brief Constructor
     */
    Configuration();
    
    /**
     * @brief Destructor
     */
    ~Configuration() = default;
    
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Load configuration from JSON string
     * @param json JSON configuration string
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromJSON(const std::string& json);
    
    /**
     * @brief Save configuration to file
     * @param filename Output file path
     * @return true if saved successfully, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Save configuration as JSON string
     * @return JSON configuration string
     */
    std::string saveToJSON() const;
    
    /**
     * @brief Validate configuration
     * @return true if valid, false otherwise
     */
    bool validate() const;
    
    /**
     * @brief Get configuration errors
     * @return Vector of error messages
     */
    std::vector<std::string> getErrors() const;
    
    /**
     * @brief Merge configuration from another source
     * @param other Configuration to merge
     * @param overwrite Whether to overwrite existing values
     */
    void merge(const Configuration& other, bool overwrite = false);
    
    /**
     * @brief Reload configuration
     * @return true if reloaded successfully, false otherwise
     */
    bool reload();
    
    /**
     * @brief Check if configuration has changed
     * @return true if changed, false otherwise
     */
    bool hasChanged() const;
    
    // Configuration sections
    SSLConfig ssl;
    AuthConfig auth;
    AccessConfig access;
    RateLimitConfig rate_limit;
    LogConfig log;
    NetworkConfig network;
    SecurityConfig security;
    PerformanceConfig performance;
    MonitoringConfig monitoring;
    
    // Module configurations
    std::map<std::string, ModuleConfig> modules;
    
    // Global settings
    std::string pid_file;
    std::string state_file;
    std::string config_file;
    std::string working_directory;
    bool daemonize = false;
    std::string user;
    std::string group;
    std::string umask;
    bool verbose = false;
    bool debug = false;
    std::string version = "0.1.0";
    
    // Configuration file monitoring
    std::string config_file_path;
    std::chrono::system_clock::time_point last_modified;
    bool auto_reload = false;
    std::chrono::seconds reload_interval{30};

private:
    // Internal state
    mutable std::vector<std::string> errors_;
    mutable bool is_valid_;
    bool has_changed_;
    
    // Configuration validation
    bool validateSSL() const;
    bool validateAuth() const;
    bool validateAccess() const;
    bool validateRateLimit() const;
    bool validateLog() const;
    bool validateNetwork() const;
    bool validateSecurity() const;
    bool validatePerformance() const;
    bool validateMonitoring() const;
    bool validateModules() const;
    
    // JSON parsing helpers
    bool parseJSONValue(const std::string& key, const std::string& value);
    bool parseModuleConfig(const std::string& module_name, const std::map<std::string, std::string>& config);
    
    // Configuration file monitoring
    void updateLastModified();
    bool checkFileChanged() const;
    
    // Helper methods
    std::string expandVariables(const std::string& value) const;
    std::string resolvePath(const std::string& path) const;
    bool validatePath(const std::string& path) const;
    bool validatePort(uint16_t port) const;
    bool validateIPAddress(const std::string& address) const;
    bool validateNetworkRange(const std::string& range) const;
    
    // Default values
    void setDefaults();
    void setDefaultSSL();
    void setDefaultAuth();
    void setDefaultAccess();
    void setDefaultRateLimit();
    void setDefaultLog();
    void setDefaultNetwork();
    void setDefaultSecurity();
    void setDefaultPerformance();
    void setDefaultMonitoring();
    
    // Environment variable substitution
    std::string substituteEnvironmentVariables(const std::string& value) const;
    
    // Configuration inheritance
    void inheritFromDefaults();
    void inheritModuleDefaults(ModuleConfig& module) const;
    
    // Configuration validation helpers
    bool validateFileExists(const std::string& file_path, const std::string& description) const;
    bool validateDirectoryExists(const std::string& dir_path, const std::string& description) const;
    bool validateWritableDirectory(const std::string& dir_path, const std::string& description) const;
    bool validateReadableFile(const std::string& file_path, const std::string& description) const;
    bool validateExecutableFile(const std::string& file_path, const std::string& description) const;
};

} // namespace simple_rsyncd
