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
#include <chrono>
#include <optional>
#include <mutex>

namespace simple_rsyncd {

/**
 * @brief User information structure
 */
struct UserInfo {
    std::string username;
    std::string password_hash;  // Hashed password
    bool password_hashed = false;
    std::chrono::system_clock::time_point password_expires;
    std::chrono::system_clock::time_point account_expires;
    bool account_locked = false;
    size_t failed_login_attempts = 0;
    std::chrono::system_clock::time_point last_login;
    std::vector<std::string> permissions;
    std::map<std::string, std::string> metadata;
};

/**
 * @brief Session information
 */
struct SessionInfo {
    std::string session_id;
    std::string username;
    std::string client_address;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_activity;
    std::chrono::seconds timeout;
    bool active = true;
};

/**
 * @brief Password hashing utilities
 */
class PasswordHasher {
public:
    /**
     * @brief Hash a password using SHA-256
     * @param password Plain text password
     * @param salt Optional salt (if empty, generates random salt)
     * @return Hashed password string (format: "sha256:salt:hash")
     */
    static std::string hashPassword(const std::string& password, const std::string& salt = "");

    /**
     * @brief Verify a password against a hash
     * @param password Plain text password
     * @param hash Hashed password string
     * @return true if password matches, false otherwise
     */
    static bool verifyPassword(const std::string& password, const std::string& hash);

    /**
     * @brief Check if a string is a hashed password
     * @param hash Password string to check
     * @return true if hashed, false if plain text
     */
    static bool isHashed(const std::string& hash);

    /**
     * @brief Generate a random salt
     * @param length Salt length in bytes
     * @return Random salt string (hex encoded)
     */
    static std::string generateSalt(size_t length = 16);
};

/**
 * @brief Password policy validator
 */
class PasswordPolicyValidator {
public:
    /**
     * @brief Validate password against policy
     * @param password Password to validate
     * @param policy Password policy
     * @return Pair of (is_valid, error_message)
     */
    static std::pair<bool, std::string> validatePassword(const std::string& password, 
                                                          const PasswordPolicy& policy);

    /**
     * @brief Check password complexity requirements
     * @param password Password to check
     * @param policy Password policy
     * @return true if meets requirements, false otherwise
     */
    static bool meetsComplexityRequirements(const std::string& password, 
                                            const PasswordPolicy& policy);
};

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

    /**
     * @brief Get user info
     * @param username Username
     * @return User info if exists, nullopt otherwise
     */
    std::optional<UserInfo> getUserInfo(const std::string& username) const;

private:
    std::string filename_;
    std::map<std::string, std::string> users_; // username -> password (plain or hashed)
    std::map<std::string, UserInfo> user_info_; // username -> user info
    bool loaded_;
};

/**
 * @brief User database manager
 */
class UserDatabase {
public:
    /**
     * @brief Constructor
     * @param database_file Database file path (optional, uses in-memory if empty)
     */
    explicit UserDatabase(const std::string& database_file = "");

    /**
     * @brief Create a new user
     * @param username Username
     * @param password Password (will be hashed)
     * @param permissions User permissions
     * @return true if created successfully, false otherwise
     */
    bool createUser(const std::string& username, const std::string& password, 
                    const std::vector<std::string>& permissions = {});

    /**
     * @brief Create a new user with password policy validation
     * @param username Username
     * @param password Password (will be hashed)
     * @param permissions User permissions
     * @param policy Password policy to validate against
     * @return true if created successfully, false otherwise
     */
    bool createUserWithPolicy(const std::string& username, const std::string& password,
                              const std::vector<std::string>& permissions,
                              const PasswordPolicy& policy);

    /**
     * @brief Update user password
     * @param username Username
     * @param new_password New password (will be hashed)
     * @return true if updated successfully, false otherwise
     */
    bool updatePassword(const std::string& username, const std::string& new_password);

    /**
     * @brief Delete a user
     * @param username Username
     * @return true if deleted successfully, false otherwise
     */
    bool deleteUser(const std::string& username);

    /**
     * @brief Get user info
     * @param username Username
     * @return User info if exists, nullopt otherwise
     */
    std::optional<UserInfo> getUserInfo(const std::string& username) const;

    /**
     * @brief Check if user exists
     * @param username Username
     * @return true if exists, false otherwise
     */
    bool userExists(const std::string& username) const;

    /**
     * @brief Get all users
     * @return Vector of usernames
     */
    std::vector<std::string> getUsers() const;

    /**
     * @brief Save database to file
     * @return true if saved successfully, false otherwise
     */
    bool save();

    /**
     * @brief Load database from file
     * @return true if loaded successfully, false otherwise
     */
    bool load();

private:
    std::string database_file_;
    std::map<std::string, UserInfo> users_;
};

/**
 * @brief Session manager
 */
class SessionManager {
public:
    /**
     * @brief Constructor
     * @param default_timeout Default session timeout
     */
    explicit SessionManager(std::chrono::seconds default_timeout = std::chrono::seconds(3600));

    /**
     * @brief Create a new session
     * @param username Username
     * @param client_address Client IP address
     * @param timeout Session timeout (optional, uses default if not specified)
     * @return Session ID
     */
    std::string createSession(const std::string& username, const std::string& client_address,
                             std::chrono::seconds timeout = std::chrono::seconds(0));

    /**
     * @brief Get session info
     * @param session_id Session ID
     * @return Session info if exists, nullopt otherwise
     */
    std::optional<SessionInfo> getSession(const std::string& session_id) const;

    /**
     * @brief Update session activity
     * @param session_id Session ID
     * @return true if updated successfully, false otherwise
     */
    bool updateActivity(const std::string& session_id);

    /**
     * @brief Check if session is valid
     * @param session_id Session ID
     * @return true if valid, false otherwise
     */
    bool isValidSession(const std::string& session_id) const;

    /**
     * @brief Delete a session
     * @param session_id Session ID
     * @return true if deleted successfully, false otherwise
     */
    bool deleteSession(const std::string& session_id);

    /**
     * @brief Clean up expired sessions
     * @return Number of sessions cleaned up
     */
    size_t cleanupExpiredSessions();

    /**
     * @brief Get all active sessions
     * @return Vector of session IDs
     */
    std::vector<std::string> getActiveSessions() const;

    /**
     * @brief Get session count
     * @return Number of active sessions
     */
    size_t getSessionCount() const;

private:
    std::chrono::seconds default_timeout_;
    std::map<std::string, SessionInfo> sessions_;
    mutable std::mutex sessions_mutex_;
    std::string generateSessionId() const;
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
     * @return Session ID if authenticated, empty string otherwise
     */
    std::string authenticate(const std::string& username, const std::string& password, 
                            const std::string& client_address = "");

    /**
     * @brief Authenticate user (legacy method, returns bool)
     * @param username Username
     * @param password Password
     * @return true if authenticated, false otherwise
     */
    bool authenticateUser(const std::string& username, const std::string& password);

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

    /**
     * @brief Get session manager
     * @return Reference to session manager
     */
    SessionManager& getSessionManager();

    /**
     * @brief Get user database
     * @return Reference to user database
     */
    UserDatabase& getUserDatabase();

private:
    AuthConfig config_;
    std::unique_ptr<PasswordFile> password_file_;
    std::unique_ptr<UserDatabase> user_database_;
    std::unique_ptr<SessionManager> session_manager_;
    bool use_user_database_ = false;
};

} // namespace simple_rsyncd
