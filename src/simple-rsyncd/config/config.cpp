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

#include "simple-rsyncd/config/config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

// JSON support
#ifdef JSONCPP_FOUND
#include <json/json.h>
#else
// Fallback if jsoncpp not found - will need to implement basic JSON parsing
namespace Json {
    class Value {
    public:
        bool isMember(const std::string&) const { return false; }
        const Value& operator[](const std::string&) const { return *this; }
        const Value& operator[](size_t) const { return *this; }
        std::string asString() const { return ""; }
        bool asBool() const { return false; }
        unsigned int asUInt() const { return 0; }
        bool isObject() const { return false; }
        bool isArray() const { return false; }
        std::vector<std::string> getMemberNames() const { return {}; }
    };
    class Reader {
    public:
        bool parse(const std::string&, Value&) { return false; }
        std::string getFormattedErrorMessages() const { return "JSON parsing not available"; }
    };
}
#endif

namespace simple_rsyncd {

Configuration::Configuration() : is_valid_(false), has_changed_(false) {
    setDefaults();
}

bool Configuration::loadFromFile(const std::string& filename) {
    errors_.clear();
    is_valid_ = false;

    // Check if file exists
    if (!std::filesystem::exists(filename)) {
        errors_.push_back("Configuration file does not exist: " + filename);
        return false;
    }

    config_file_path = filename;

    // Detect file format by extension
    std::string ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".json") {
        // Load as JSON
        std::ifstream file(filename);
        if (!file.is_open()) {
            errors_.push_back("Failed to open configuration file: " + filename);
            return false;
        }

        std::string json_content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        file.close();

        return loadFromJSON(json_content);
    } else {
        // Load as INI/config file
        std::ifstream file(filename);
        if (!file.is_open()) {
            errors_.push_back("Failed to open configuration file: " + filename);
            return false;
        }

        config_file_path = filename;

    std::string line;
    std::string current_section;
    int line_number = 0;
    bool in_module = false;
    std::string current_module;

    while (std::getline(file, line)) {
        line_number++;

        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Check for section header [section]
        if (line[0] == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.length() - 2);
            // Remove whitespace from section name
            section.erase(0, section.find_first_not_of(" \t"));
            section.erase(section.find_last_not_of(" \t") + 1);

            if (section == "global") {
                current_section = "global";
                in_module = false;
            } else if (section.find("module:") == 0) {
                current_module = section.substr(7);
                current_section = "module";
                in_module = true;

                // Create module config if it doesn't exist
                if (modules.find(current_module) == modules.end()) {
                    modules[current_module] = ModuleConfig();
                }
            } else {
                current_section = section;
                in_module = false;
            }
            continue;
        }

        // Parse key=value pairs
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            errors_.push_back("Invalid line " + std::to_string(line_number) + ": " + line);
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Remove quotes if present
        if (!value.empty() && ((value[0] == '"' && value.back() == '"') ||
                               (value[0] == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }

        // Parse based on section
        if (current_section == "global" || current_section.empty()) {
            parseGlobalConfig(key, value);
        } else if (in_module && current_section == "module") {
            parseModuleConfig(current_module, key, value);
        }
    }

    file.close();
    updateLastModified();

    // Validate configuration
    if (!validate()) {
        return false;
    }

    is_valid_ = true;
    has_changed_ = false;
    return true;
}

void Configuration::parseGlobalConfig(const std::string& key, const std::string& value) {
    // Network settings
    if (key == "bind_address" || key == "address") {
        network.bind_address = value;
    } else if (key == "bind_port" || key == "port") {
        try {
            network.bind_port = static_cast<uint16_t>(std::stoul(value));
        } catch (...) {
            errors_.push_back("Invalid port value: " + value);
        }
    } else if (key == "max_connections") {
        try {
            network.max_connections = std::stoul(value);
        } catch (...) {
            errors_.push_back("Invalid max_connections value: " + value);
        }
    }
    // SSL settings
    else if (key == "ssl_enabled") {
        ssl.enabled = (value == "true" || value == "1" || value == "yes");
    } else if (key == "ssl_certificate_file" || key == "ssl_cert") {
        ssl.certificate_file = value;
    } else if (key == "ssl_private_key_file" || key == "ssl_key") {
        ssl.private_key_file = value;
    } else if (key == "ssl_ca_file" || key == "ssl_ca") {
        ssl.ca_file = value;
    }
    // Auth settings
    else if (key == "auth_enabled") {
        auth.enabled = (value == "true" || value == "1" || value == "yes");
    } else if (key == "auth_method") {
        auth.method = value;
    } else if (key == "auth_password_file" || key == "password_file") {
        auth.password_file = value;
    } else if (key == "auth_realm") {
        auth.realm = value;
    }
    // Log settings
    else if (key == "log_level" || key == "log") {
        log.level = value;
    } else if (key == "log_file") {
        log.file = value;
        log.file_output = true;
    }
    // Other settings
    else if (key == "pid_file") {
        pid_file = value;
    } else if (key == "user") {
        user = value;
        security.user = value;
    } else if (key == "group") {
        group = value;
        security.group = value;
    }
}

void Configuration::parseModuleConfig(const std::string& module_name, const std::string& key, const std::string& value) {
    if (modules.find(module_name) == modules.end()) {
        modules[module_name] = ModuleConfig();
    }

    auto& module = modules[module_name];

    if (key == "path") {
        module.path = value;
    } else if (key == "comment") {
        module.comment = value;
    } else if (key == "read_only" || key == "read only") {
        module.read_only = (value == "true" || value == "1" || value == "yes");
    } else if (key == "list") {
        module.list = (value == "true" || value == "1" || value == "yes");
    } else if (key == "delete") {
        module.allow_delete = (value == "true" || value == "1" || value == "yes");
    } else if (key == "overwrite") {
        module.overwrite = (value == "true" || value == "1" || value == "yes");
    } else if (key == "exclude") {
        module.exclude_patterns.push_back(value);
    } else if (key == "include") {
        module.include_patterns.push_back(value);
    } else {
        // Store unknown keys as custom options
        module.custom_options[key] = value;
    }
}

bool Configuration::loadFromJSON(const std::string& json) {
    errors_.clear();
    is_valid_ = false;

    if (json.empty()) {
        errors_.push_back("Empty JSON configuration");
        return false;
    }

    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(json, root)) {
        errors_.push_back("Failed to parse JSON: " + reader.getFormattedErrorMessages());
        return false;
    }

    // Parse global configuration
    if (root.isMember("global")) {
        const Json::Value& global = root["global"];
        parseJSONGlobal(global);
    } else {
        // If no "global" section, parse root as global
        parseJSONGlobal(root);
    }

    // Parse network configuration
    if (root.isMember("network")) {
        const Json::Value& net = root["network"];
        if (net.isMember("bind_address")) {
            network.bind_address = net["bind_address"].asString();
        }
        if (net.isMember("bind_port")) {
            network.bind_port = static_cast<uint16_t>(net["bind_port"].asUInt());
        }
        if (net.isMember("max_connections")) {
            network.max_connections = net["max_connections"].asUInt();
        }
        if (net.isMember("backlog")) {
            network.backlog = net["backlog"].asUInt();
        }
        if (net.isMember("worker_threads")) {
            network.worker_threads = net["worker_threads"].asUInt();
        }
    }

    // Parse SSL configuration
    if (root.isMember("ssl")) {
        const Json::Value& ssl_config = root["ssl"];
        if (ssl_config.isMember("enabled")) {
            ssl.enabled = ssl_config["enabled"].asBool();
        }
        if (ssl_config.isMember("certificate_file")) {
            ssl.certificate_file = ssl_config["certificate_file"].asString();
        }
        if (ssl_config.isMember("private_key_file")) {
            ssl.private_key_file = ssl_config["private_key_file"].asString();
        }
        if (ssl_config.isMember("ca_file")) {
            ssl.ca_file = ssl_config["ca_file"].asString();
        }
    }

    // Parse authentication configuration
    if (root.isMember("auth")) {
        const Json::Value& auth_config = root["auth"];
        if (auth_config.isMember("enabled")) {
            auth.enabled = auth_config["enabled"].asBool();
        }
        if (auth_config.isMember("method")) {
            auth.method = auth_config["method"].asString();
        }
        if (auth_config.isMember("password_file")) {
            auth.password_file = auth_config["password_file"].asString();
        }
        if (auth_config.isMember("realm")) {
            auth.realm = auth_config["realm"].asString();
        }
        if (auth_config.isMember("anonymous_access")) {
            auth.anonymous_access = auth_config["anonymous_access"].asBool();
        }
        if (auth_config.isMember("allowed_users")) {
            const Json::Value& users = auth_config["allowed_users"];
            if (users.isArray()) {
                for (const auto& user : users) {
                    auth.allowed_users.push_back(user.asString());
                }
            }
        }
        if (auth_config.isMember("denied_users")) {
            const Json::Value& users = auth_config["denied_users"];
            if (users.isArray()) {
                for (const auto& user : users) {
                    auth.denied_users.push_back(user.asString());
                }
            }
        }
    }

    // Parse modules
    if (root.isMember("modules")) {
        const Json::Value& modules_json = root["modules"];
        if (modules_json.isObject()) {
            for (const auto& module_name : modules_json.getMemberNames()) {
                const Json::Value& module_json = modules_json[module_name];
                parseJSONModule(module_name, module_json);
            }
        }
    }

    updateLastModified();

    // Validate configuration
    if (!validate()) {
        return false;
    }

    is_valid_ = true;
    has_changed_ = false;
    return true;
}

void Configuration::parseJSONGlobal(const Json::Value& global) {
    // Parse global settings that might be at root or in "global" object
    if (global.isMember("bind_address") || global.isMember("address")) {
        network.bind_address = global.isMember("bind_address") 
            ? global["bind_address"].asString() 
            : global["address"].asString();
    }
    if (global.isMember("bind_port") || global.isMember("port")) {
        network.bind_port = static_cast<uint16_t>(
            global.isMember("bind_port") 
                ? global["bind_port"].asUInt() 
                : global["port"].asUInt());
    }
    if (global.isMember("max_connections")) {
        network.max_connections = global["max_connections"].asUInt();
    }
    if (global.isMember("pid_file")) {
        pid_file = global["pid_file"].asString();
    }
    if (global.isMember("user")) {
        user = global["user"].asString();
        security.user = user;
    }
    if (global.isMember("group")) {
        group = global["group"].asString();
        security.group = group;
    }
}

void Configuration::parseJSONModule(const std::string& module_name, const Json::Value& module_json) {
    if (modules.find(module_name) == modules.end()) {
        modules[module_name] = ModuleConfig();
    }

    auto& module = modules[module_name];
    module.name = module_name;

    if (module_json.isMember("path")) {
        module.path = module_json["path"].asString();
    }
    if (module_json.isMember("comment")) {
        module.comment = module_json["comment"].asString();
    }
    if (module_json.isMember("read_only")) {
        module.read_only = module_json["read_only"].asBool();
    }
    if (module_json.isMember("list")) {
        module.list = module_json["list"].asBool();
    }
    if (module_json.isMember("allow_delete")) {
        module.allow_delete = module_json["allow_delete"].asBool();
    }
    if (module_json.isMember("overwrite")) {
        module.overwrite = module_json["overwrite"].asBool();
    }
    if (module_json.isMember("exclude")) {
        const Json::Value& exclude = module_json["exclude"];
        if (exclude.isArray()) {
            for (const auto& pattern : exclude) {
                module.exclude_patterns.push_back(pattern.asString());
            }
        }
    }
    if (module_json.isMember("include")) {
        const Json::Value& include = module_json["include"];
        if (include.isArray()) {
            for (const auto& pattern : include) {
                module.include_patterns.push_back(pattern.asString());
            }
        }
    }
}

bool Configuration::saveToFile(const std::string& filename) const {
    // Basic file saving implementation
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // For now, just write basic configuration
    file << "# simple-rsyncd Configuration\n";
    file << "# Generated configuration file\n";

    return true;
}

std::string Configuration::saveToJSON() const {
    // Basic JSON saving implementation
    return "{\"version\": \"0.2.0\"}";
}

bool Configuration::validate() const {
    errors_.clear();
    bool valid = true;

    // Validate network settings
    if (!validateNetwork()) {
        valid = false;
    }

    // Validate SSL settings if enabled
    if (ssl.enabled && !validateSSL()) {
        valid = false;
    }

    // Validate auth settings if enabled
    if (auth.enabled && !validateAuth()) {
        valid = false;
    }

    // Validate modules
    if (!validateModules()) {
        valid = false;
    }

    return valid;
}

bool Configuration::validateNetwork() const {
    bool valid = true;

    if (network.bind_port == 0 || network.bind_port > 65535) {
        errors_.push_back("Invalid bind_port: " + std::to_string(network.bind_port));
        valid = false;
    }

    if (network.max_connections == 0) {
        errors_.push_back("max_connections must be greater than 0");
        valid = false;
    }

    return valid;
}

bool Configuration::validateSSL() const {
    bool valid = true;

    if (ssl.certificate_file.empty()) {
        errors_.push_back("SSL enabled but certificate_file not specified");
        valid = false;
    } else if (!std::filesystem::exists(ssl.certificate_file)) {
        errors_.push_back("SSL certificate file does not exist: " + ssl.certificate_file);
        valid = false;
    }

    if (ssl.private_key_file.empty()) {
        errors_.push_back("SSL enabled but private_key_file not specified");
        valid = false;
    } else if (!std::filesystem::exists(ssl.private_key_file)) {
        errors_.push_back("SSL private key file does not exist: " + ssl.private_key_file);
        valid = false;
    }

    return valid;
}

bool Configuration::validateAuth() const {
    bool valid = true;

    if (auth.method == "password" && auth.password_file.empty()) {
        errors_.push_back("Password authentication enabled but password_file not specified");
        valid = false;
    } else if (auth.method == "password" && !std::filesystem::exists(auth.password_file)) {
        errors_.push_back("Password file does not exist: " + auth.password_file);
        valid = false;
    }

    return valid;
}

bool Configuration::validateModules() const {
    bool valid = true;

    for (const auto& [name, module] : modules) {
        if (module.path.empty()) {
            errors_.push_back("Module '" + name + "' has no path specified");
            valid = false;
        } else if (!std::filesystem::exists(module.path)) {
            errors_.push_back("Module '" + name + "' path does not exist: " + module.path);
            // Warning, not error - path might be created later
        } else if (!std::filesystem::is_directory(module.path)) {
            errors_.push_back("Module '" + name + "' path is not a directory: " + module.path);
            valid = false;
        }
    }

    return valid;
}

std::vector<std::string> Configuration::getErrors() const {
    return errors_;
}

void Configuration::merge(const Configuration& other, bool overwrite) {
    (void)other; // Suppress unused parameter warning
    (void)overwrite; // Suppress unused parameter warning
    // Basic configuration merging
    if (overwrite) {
        // Overwrite current settings
    } else {
        // Merge settings
    }
}

bool Configuration::reload() {
    if (config_file_path.empty()) {
        return false;
    }

    return loadFromFile(config_file_path);
}

bool Configuration::hasChanged() const {
    return has_changed_;
}

void Configuration::setDefaults() {
    setDefaultSSL();
    setDefaultAuth();
    setDefaultAccess();
    setDefaultRateLimit();
    setDefaultLog();
    setDefaultNetwork();
    setDefaultSecurity();
    setDefaultPerformance();
    setDefaultMonitoring();
}

void Configuration::setDefaultSSL() {
    ssl.enabled = false;
    ssl.certificate_file = "";
    ssl.private_key_file = "";
    ssl.ca_file = "";
    ssl.cipher_suite = "ECDHE-RSA-AES256-GCM-SHA384";
    ssl.require_client_cert = false;
    ssl.tls_version = "1.2";
    ssl.verify_peer = true;
}

void Configuration::setDefaultAuth() {
    auth.enabled = false;
    auth.method = "password";
    auth.realm = "simple-rsyncd";
    auth.password_file = "";
    auth.public_key_file = "";
    auth.oauth2_config_file = "";
    auth.anonymous_access = false;
}

void Configuration::setDefaultAccess() {
    access.enabled = true;
    access.allowed_hosts = {"127.0.0.1", "::1"};
    access.denied_hosts = {};
    access.allowed_networks = {"10.0.0.0/8", "172.16.0.0/12"};
    access.denied_networks = {};
    access.reverse_dns_lookup = false;
    access.access_log_file = "";
    access.access_log_format = "combined";
}

void Configuration::setDefaultRateLimit() {
    rate_limit.enabled = true;
    rate_limit.max_connections_per_minute = 60;
    rate_limit.max_connections_per_hour = 1000;
    rate_limit.max_bandwidth_per_connection = 0;
    rate_limit.max_bandwidth_total = 0;
    rate_limit.connection_timeout = std::chrono::seconds(300);
    rate_limit.idle_timeout = std::chrono::seconds(60);
}

void Configuration::setDefaultLog() {
    log.level = "info";
    log.file = "";
    log.format = "{timestamp} [{level}] {message}";
    log.console_output = true;
    log.file_output = false;
    log.syslog_output = false;
    log.syslog_facility = "daemon";
    log.max_file_size = 10 * 1024 * 1024;
    log.max_files = 5;
    log.compress_old_logs = true;
}

void Configuration::setDefaultNetwork() {
    network.bind_address = "0.0.0.0";
    network.bind_port = 873;
    network.backlog = 128;
    network.reuse_address = true;
    network.keep_alive = true;
    network.keep_alive_time = std::chrono::seconds(7200);
    network.keep_alive_interval = std::chrono::seconds(75);
    network.keep_alive_probes = 9;
    network.max_connections = 100;
    network.worker_threads = 4;
}

void Configuration::setDefaultSecurity() {
    security.chroot_enabled = false;
    security.chroot_directory = "";
    security.user = "";
    security.group = "";
    security.drop_privileges = true;
    security.restrict_to_module_path = true;
    security.allow_symlinks = false;
    security.allow_hardlinks = false;
    security.allow_devices = false;
    security.allow_sockets = false;
    security.umask = "022";
}

void Configuration::setDefaultPerformance() {
    performance.buffer_size = 64 * 1024;
    performance.max_memory_usage = 512 * 1024 * 1024;
    performance.enable_compression = true;
    performance.compression_level = "6";
    performance.enable_checksum_verification = true;
    performance.checksum_algorithm = "md5";
    performance.max_concurrent_transfers = 10;
    performance.enable_pipelining = true;
    performance.pipeline_depth = 5;
}

void Configuration::setDefaultMonitoring() {
    monitoring.enabled = true;
    monitoring.metrics_file = "";
    monitoring.health_check_file = "";
    monitoring.metrics_interval = std::chrono::seconds(60);
    monitoring.health_check_interval = std::chrono::seconds(300);
    monitoring.enable_prometheus_metrics = false;
    monitoring.prometheus_endpoint = "/metrics";
    monitoring.prometheus_port = 9090;
    monitoring.prometheus_address = "127.0.0.1";
}

void Configuration::updateLastModified() {
    if (!config_file_path.empty()) {
        try {
            last_modified = std::filesystem::last_write_time(config_file_path);
        } catch (...) {
            last_modified = std::chrono::system_clock::now();
        }
    } else {
        last_modified = std::chrono::system_clock::now();
    }
    has_changed_ = false;
}

bool Configuration::checkFileChanged() const {
    if (config_file_path.empty()) {
        return false;
    }

    try {
        auto current_time = std::filesystem::last_write_time(config_file_path);
        return current_time > last_modified;
    } catch (...) {
        return false;
    }
}

bool Configuration::validateAccess() const {
    return true; // Basic validation - can be enhanced
}

bool Configuration::validateRateLimit() const {
    return true; // Basic validation - can be enhanced
}

bool Configuration::validateLog() const {
    return true; // Basic validation - can be enhanced
}

bool Configuration::validateSecurity() const {
    return true; // Basic validation - can be enhanced
}

bool Configuration::validatePerformance() const {
    return true; // Basic validation - can be enhanced
}

bool Configuration::validateMonitoring() const {
    return true; // Basic validation - can be enhanced
}

} // namespace simple_rsyncd
