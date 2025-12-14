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

#pragma once

#include "simple-rsyncd/config/config.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace simple_rsyncd {

/**
 * @brief Password file manager
 */
class PasswordFile {
public:
    /**
     * @brief Constructor
     * @param filename Password file path
     */
    explicit PasswordFile(const std::string& filename);

    /**
     * @brief Load password file
     * @return true if loaded successfully, false otherwise
     */
    bool load();

    /**
     * @brief Authenticate user with password
     * @param username Username
     * @param password Password
     * @return true if authenticated, false otherwise
     */
    bool authenticate(const std::string& username, const std::string& password) const;

    /**
     * @brief Check if user exists
     * @param username Username
     * @return true if exists, false otherwise
     */
    bool userExists(const std::string& username) const;

    /**
     * @brief Get list of all users
     * @return Vector of usernames
     */
    std::vector<std::string> getUsers() const;

    /**
     * @brief Get user count
     * @return Number of users
     */
    size_t getUserCount() const;

    /**
     * @brief Reload password file
     * @return true if reloaded successfully, false otherwise
     */
    bool reload();

private:
    std::string filename_;
    std::map<std::string, std::string> users_; // username -> password
    bool loaded_;
};

/**
 * @brief Authentication manager
 */
class AuthenticationManager {
public:
    /**
     * @brief Constructor
     * @param config Authentication configuration
     */
    explicit AuthenticationManager(const AuthConfig& config);

    /**
     * @brief Authenticate user
     * @param username Username
     * @param password Password
     * @return true if authenticated, false otherwise
     */
    bool authenticate(const std::string& username, const std::string& password);

    /**
     * @brief Check if authentication is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;

    /**
     * @brief Check if anonymous access is allowed
     * @return true if allowed, false otherwise
     */
    bool allowsAnonymous() const;

    /**
     * @brief Get authentication realm
     * @return Realm string
     */
    std::string getRealm() const;

    /**
     * @brief Reload authentication configuration
     * @return true if reloaded successfully, false otherwise
     */
    bool reload();

private:
    AuthConfig config_;
    std::unique_ptr<PasswordFile> password_file_;
};

} // namespace simple_rsyncd
