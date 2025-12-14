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

#include "simple-rsyncd/core/auth.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace simple_rsyncd {

// Simple password file format: username:password (one per line)
// For now, we'll support plain text passwords (insecure, but functional)
// TODO: Add proper password hashing (bcrypt, argon2, etc.)

PasswordFile::PasswordFile(const std::string& filename) : filename_(filename), loaded_(false) {
    load();
}

bool PasswordFile::load() {
    users_.clear();
    loaded_ = false;

    if (filename_.empty()) {
        return false;
    }

    std::ifstream file(filename_);
    if (!file.is_open()) {
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {

        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Parse username:password format
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos || colon_pos == 0) {
            continue; // Invalid format, skip
        }

        std::string username = line.substr(0, colon_pos);
        std::string password = line.substr(colon_pos + 1);

        // Remove whitespace from username
        username.erase(0, username.find_first_not_of(" \t"));
        username.erase(username.find_last_not_of(" \t") + 1);

        // Store password (plain text for now - TODO: add hashing)
        users_[username] = password;
    }

    file.close();
    loaded_ = true;
    return true;
}

bool PasswordFile::authenticate(const std::string& username, const std::string& password) const {
    if (!loaded_) {
        return false;
    }

    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }

    // Simple plain text comparison (insecure - TODO: add proper hashing)
    return it->second == password;
}

bool PasswordFile::userExists(const std::string& username) const {
    if (!loaded_) {
        return false;
    }
    return users_.find(username) != users_.end();
}

std::vector<std::string> PasswordFile::getUsers() const {
    std::vector<std::string> user_list;
    for (const auto& [username, _] : users_) {
        user_list.push_back(username);
    }
    return user_list;
}

size_t PasswordFile::getUserCount() const {
    return users_.size();
}

bool PasswordFile::reload() {
    return load();
}

// Authentication manager implementation
AuthenticationManager::AuthenticationManager(const AuthConfig& config) : config_(config) {
    if (config_.method == "password" && !config_.password_file.empty()) {
        password_file_ = std::make_unique<PasswordFile>(config_.password_file);
    }
}

bool AuthenticationManager::authenticate(const std::string& username, const std::string& password) {
    if (!config_.enabled) {
        // If auth is disabled, allow anonymous access if configured
        return config_.anonymous_access;
    }

    if (config_.method == "password") {
        if (!password_file_) {
            return false;
        }

        // Check if user is explicitly denied
        if (std::find(config_.denied_users.begin(), config_.denied_users.end(), username) != config_.denied_users.end()) {
            return false;
        }

        // Check if user is explicitly allowed (if allowed_users is not empty)
        if (!config_.allowed_users.empty()) {
            if (std::find(config_.allowed_users.begin(), config_.allowed_users.end(), username) == config_.allowed_users.end()) {
                return false;
            }
        }

        return password_file_->authenticate(username, password);
    }

    // Other auth methods not implemented yet
    return false;
}

bool AuthenticationManager::isEnabled() const {
    return config_.enabled;
}

bool AuthenticationManager::allowsAnonymous() const {
    return config_.anonymous_access;
}

std::string AuthenticationManager::getRealm() const {
    return config_.realm;
}

bool AuthenticationManager::reload() {
    if (password_file_) {
        return password_file_->reload();
    }
    return true;
}

} // namespace simple_rsyncd
