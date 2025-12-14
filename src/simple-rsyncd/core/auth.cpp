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
#include <iomanip>
#include <random>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/err.h>

namespace simple_rsyncd {

// Password hashing implementation
std::string PasswordHasher::hashPassword(const std::string& password, const std::string& salt) {
    std::string actual_salt = salt.empty() ? generateSalt() : salt;

    // Create SHA-256 hash using EVP interface (OpenSSL 3.0 compatible)
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return ""; // Error creating context
    }

    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1 ||
        EVP_DigestUpdate(mdctx, actual_salt.c_str(), actual_salt.length()) != 1 ||
        EVP_DigestUpdate(mdctx, password.c_str(), password.length()) != 1 ||
        EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return ""; // Error hashing
    }

    EVP_MD_CTX_free(mdctx);

    // Convert hash to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string hash_str = ss.str();

    // Format: sha256:salt:hash
    return "sha256:" + actual_salt + ":" + hash_str;
}

bool PasswordHasher::verifyPassword(const std::string& password, const std::string& hash) {
    if (!isHashed(hash)) {
        // Plain text comparison (for backward compatibility)
        return password == hash;
    }

    // Parse hash format: sha256:salt:hash
    size_t colon1 = hash.find(':');
    if (colon1 == std::string::npos) {
        return false;
    }

    size_t colon2 = hash.find(':', colon1 + 1);
    if (colon2 == std::string::npos) {
        return false;
    }

    std::string algorithm = hash.substr(0, colon1);
    std::string salt = hash.substr(colon1 + 1, colon2 - colon1 - 1);
    std::string stored_hash = hash.substr(colon2 + 1);

    if (algorithm != "sha256") {
        return false;
    }

    // Compute hash with same salt
    std::string computed_hash = hashPassword(password, salt);
    size_t computed_colon = computed_hash.find(':', computed_hash.find(':') + 1);
    std::string computed_hash_part = computed_hash.substr(computed_colon + 1);

    return stored_hash == computed_hash_part;
}

bool PasswordHasher::isHashed(const std::string& hash) {
    return hash.find("sha256:") == 0;
}

std::string PasswordHasher::generateSalt(size_t length) {
    unsigned char* salt_bytes = new unsigned char[length];
    if (RAND_bytes(salt_bytes, length) != 1) {
        // Fallback to pseudo-random if RAND_bytes fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < length; i++) {
            salt_bytes[i] = static_cast<unsigned char>(dis(gen));
        }
    }

    // Convert to hex string
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(salt_bytes[i]);
    }
    delete[] salt_bytes;
    return ss.str();
}

// PasswordPolicyValidator implementation
std::pair<bool, std::string> PasswordPolicyValidator::validatePassword(const std::string& password,
                                                                        const PasswordPolicy& policy) {
    // Check minimum length
    if (password.length() < policy.min_length) {
        return {false, "Password must be at least " + std::to_string(policy.min_length) + " characters long"};
    }

    // Check complexity requirements
    if (!meetsComplexityRequirements(password, policy)) {
        std::string requirements;
        if (policy.require_uppercase) requirements += "uppercase letter, ";
        if (policy.require_lowercase) requirements += "lowercase letter, ";
        if (policy.require_digits) requirements += "digit, ";
        if (policy.require_special) requirements += "special character, ";
        if (!requirements.empty()) {
            requirements = requirements.substr(0, requirements.length() - 2);
            return {false, "Password must contain: " + requirements};
        }
    }

    return {true, ""};
}

bool PasswordPolicyValidator::meetsComplexityRequirements(const std::string& password,
                                                           const PasswordPolicy& policy) {
    bool has_uppercase = false;
    bool has_lowercase = false;
    bool has_digit = false;
    bool has_special = false;

    for (char c : password) {
        if (std::isupper(c)) has_uppercase = true;
        else if (std::islower(c)) has_lowercase = true;
        else if (std::isdigit(c)) has_digit = true;
        else if (!std::isalnum(c)) has_special = true;
    }

    if (policy.require_uppercase && !has_uppercase) return false;
    if (policy.require_lowercase && !has_lowercase) return false;
    if (policy.require_digits && !has_digit) return false;
    if (policy.require_special && !has_special) return false;

    return true;
}

// Simple password file format: username:password (one per line)
// Supports both plain text and hashed passwords (sha256:salt:hash format)

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

        // Store password (supports both plain text and hashed)
        users_[username] = password;

        // Create user info
        UserInfo user_info;
        user_info.username = username;
        user_info.password_hash = password;
        user_info.password_hashed = PasswordHasher::isHashed(password);
        user_info.password_expires = std::chrono::system_clock::time_point::max();
        user_info.account_expires = std::chrono::system_clock::time_point::max();
        user_info.account_locked = false;
        user_info.failed_login_attempts = 0;
        user_info_[username] = user_info;
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

    // Use password hasher to verify (supports both plain text and hashed)
    return PasswordHasher::verifyPassword(password, it->second);
}

std::optional<UserInfo> PasswordFile::getUserInfo(const std::string& username) const {
    if (!loaded_) {
        return std::nullopt;
    }

    auto it = user_info_.find(username);
    if (it == user_info_.end()) {
        return std::nullopt;
    }

    return it->second;
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

// UserDatabase implementation
UserDatabase::UserDatabase(const std::string& database_file) : database_file_(database_file) {
    if (!database_file_.empty()) {
        load();
    }
}

bool UserDatabase::createUser(const std::string& username, const std::string& password,
                              const std::vector<std::string>& permissions) {
    if (userExists(username)) {
        return false;
    }

    UserInfo user_info;
    user_info.username = username;
    user_info.password_hash = PasswordHasher::hashPassword(password);
    user_info.password_hashed = true;
    user_info.password_expires = std::chrono::system_clock::time_point::max();
    user_info.account_expires = std::chrono::system_clock::time_point::max();
    user_info.account_locked = false;
    user_info.failed_login_attempts = 0;
    user_info.permissions = permissions;

    users_[username] = user_info;

    if (!database_file_.empty()) {
        save();
    }

    return true;
}

bool UserDatabase::createUserWithPolicy(const std::string& username, const std::string& password,
                                        const std::vector<std::string>& permissions,
                                        const PasswordPolicy& policy) {
    if (userExists(username)) {
        return false;
    }

    // Validate password against policy
    auto [valid, error] = PasswordPolicyValidator::validatePassword(password, policy);
    if (!valid) {
        return false; // Password doesn't meet policy requirements
    }

    UserInfo user_info;
    user_info.username = username;
    user_info.password_hash = PasswordHasher::hashPassword(password);
    user_info.password_hashed = true;

    // Set password expiration if policy requires it
    if (policy.expiration_hours.count() > 0) {
        user_info.password_expires = std::chrono::system_clock::now() + policy.expiration_hours;
    } else {
        user_info.password_expires = std::chrono::system_clock::time_point::max();
    }

    user_info.account_expires = std::chrono::system_clock::time_point::max();
    user_info.account_locked = false;
    user_info.failed_login_attempts = 0;
    user_info.permissions = permissions;

    users_[username] = user_info;

    if (!database_file_.empty()) {
        save();
    }

    return true;
}

bool UserDatabase::updatePassword(const std::string& username, const std::string& new_password) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }

    it->second.password_hash = PasswordHasher::hashPassword(new_password);
    it->second.password_hashed = true;
    it->second.failed_login_attempts = 0; // Reset failed attempts on password change

    if (!database_file_.empty()) {
        save();
    }

    return true;
}

bool UserDatabase::deleteUser(const std::string& username) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }

    users_.erase(it);

    if (!database_file_.empty()) {
        save();
    }

    return true;
}

std::optional<UserInfo> UserDatabase::getUserInfo(const std::string& username) const {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool UserDatabase::userExists(const std::string& username) const {
    return users_.find(username) != users_.end();
}

std::vector<std::string> UserDatabase::getUsers() const {
    std::vector<std::string> user_list;
    for (const auto& [username, _] : users_) {
        user_list.push_back(username);
    }
    return user_list;
}

bool UserDatabase::save() {
    if (database_file_.empty()) {
        return false;
    }

    std::ofstream file(database_file_);
    if (!file.is_open()) {
        return false;
    }

    file << "# User database file\n";
    file << "# Format: username:password_hash:permissions:metadata\n";

    for (const auto& [username, user_info] : users_) {
        file << username << ":" << user_info.password_hash;

        // Write permissions
        if (!user_info.permissions.empty()) {
            file << ":";
            for (size_t i = 0; i < user_info.permissions.size(); i++) {
                if (i > 0) file << ",";
                file << user_info.permissions[i];
            }
        }

        file << "\n";
    }

    file.close();
    return true;
}

bool UserDatabase::load() {
    if (database_file_.empty()) {
        return false;
    }

    users_.clear();

    std::ifstream file(database_file_);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Parse: username:password_hash:permissions
        size_t colon1 = line.find(':');
        if (colon1 == std::string::npos) continue;

        size_t colon2 = line.find(':', colon1 + 1);
        if (colon2 == std::string::npos) continue;

        std::string username = line.substr(0, colon1);
        std::string password_hash = line.substr(colon1 + 1, colon2 - colon1 - 1);
        std::string permissions_str = line.substr(colon2 + 1);

        UserInfo user_info;
        user_info.username = username;
        user_info.password_hash = password_hash;
        user_info.password_hashed = PasswordHasher::isHashed(password_hash);
        user_info.password_expires = std::chrono::system_clock::time_point::max();
        user_info.account_expires = std::chrono::system_clock::time_point::max();
        user_info.account_locked = false;
        user_info.failed_login_attempts = 0;

        // Parse permissions
        if (!permissions_str.empty()) {
            std::istringstream iss(permissions_str);
            std::string perm;
            while (std::getline(iss, perm, ',')) {
                user_info.permissions.push_back(perm);
            }
        }

        users_[username] = user_info;
    }

    file.close();
    return true;
}

// SessionManager implementation
SessionManager::SessionManager(std::chrono::seconds default_timeout)
    : default_timeout_(default_timeout) {
}

std::string SessionManager::createSession(const std::string& username, const std::string& client_address,
                                          std::chrono::seconds timeout) {
    if (timeout.count() == 0) {
        timeout = default_timeout_;
    }

    std::string session_id = generateSessionId();
    auto now = std::chrono::system_clock::now();

    SessionInfo session;
    session.session_id = session_id;
    session.username = username;
    session.client_address = client_address;
    session.created_at = now;
    session.last_activity = now;
    session.timeout = timeout;
    session.active = true;

    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_[session_id] = session;

    return session_id;
}

std::optional<SessionInfo> SessionManager::getSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool SessionManager::updateActivity(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    it->second.last_activity = std::chrono::system_clock::now();
    return true;
}

bool SessionManager::isValidSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.active) {
        return false;
    }

    // Check if session has expired
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_activity);
    if (elapsed > it->second.timeout) {
        return false;
    }

    return true;
}

bool SessionManager::deleteSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    sessions_.erase(it);
    return true;
}

size_t SessionManager::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    size_t cleaned = 0;
    auto now = std::chrono::system_clock::now();

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_activity);
        if (elapsed > it->second.timeout || !it->second.active) {
            it = sessions_.erase(it);
            cleaned++;
        } else {
            ++it;
        }
    }

    return cleaned;
}

std::vector<std::string> SessionManager::getActiveSessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    std::vector<std::string> active;
    auto now = std::chrono::system_clock::now();

    for (const auto& [session_id, session] : sessions_) {
        if (session.active) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_activity);
            if (elapsed <= session.timeout) {
                active.push_back(session_id);
            }
        }
    }

    return active;
}

size_t SessionManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

std::string SessionManager::generateSessionId() const {
    // Generate a random session ID
    unsigned char bytes[16];
    if (RAND_bytes(bytes, 16) != 1) {
        // Fallback
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < 16; i++) {
            bytes[i] = static_cast<unsigned char>(dis(gen));
        }
    }

    std::stringstream ss;
    for (int i = 0; i < 16; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

// PublicKey implementation
std::optional<PublicKeyInfo> PublicKey::parse(const std::string& key_string) {
    if (key_string.empty()) {
        return std::nullopt;
    }

    // Parse SSH public key format: "key-type key-data [comment]"
    std::istringstream iss(key_string);
    std::string key_type, key_data, comment;

    if (!(iss >> key_type >> key_data)) {
        return std::nullopt;
    }

    // Read optional comment (rest of the line)
    std::string rest;
    std::getline(iss, rest);
    if (!rest.empty()) {
        comment = rest.substr(1); // Skip leading space
    }

    // Validate key type
    if (!isSupportedKeyType(key_type)) {
        return std::nullopt;
    }

    PublicKeyInfo info;
    info.key_type = key_type;
    info.key_data = key_data;
    info.comment = comment;

    return info;
}

bool PublicKey::verifySignature(const PublicKeyInfo& key_info,
                                const std::string& data,
                                const std::string& signature) {
    if (key_info.key_type == "ssh-rsa") {
        return verifyRSASignature(key_info.key_data, data, signature);
    } else if (key_info.key_type.find("ecdsa-sha2-") == 0) {
        return verifyECDSASignature(key_info.key_data, key_info.key_type, data, signature);
    } else if (key_info.key_type == "ssh-ed25519") {
        return verifyEd25519Signature(key_info.key_data, data, signature);
    }
    return false;
}

bool PublicKey::isSupportedKeyType(const std::string& key_type) {
    return key_type == "ssh-rsa" ||
           key_type == "ssh-dss" ||
           key_type.find("ecdsa-sha2-") == 0 ||
           key_type == "ssh-ed25519";
}

std::vector<std::string> PublicKey::getSupportedKeyTypes() {
    return {"ssh-rsa", "ssh-dss", "ecdsa-sha2-nistp256", "ecdsa-sha2-nistp384",
            "ecdsa-sha2-nistp521", "ssh-ed25519"};
}

bool PublicKey::verifyRSASignature(const std::string& key_data,
                                   const std::string& data,
                                   const std::string& signature) {
    // Decode base64 key data
    std::string decoded_key = base64Decode(key_data);
    if (decoded_key.empty()) {
        return false;
    }

    // Parse SSH RSA public key format
    // Format: [4 bytes: key_type_len][key_type][4 bytes: e_len][e][4 bytes: n_len][n]
    if (decoded_key.length() < 20) {
        return false;
    }

    // For now, we'll use a simplified approach
    // In production, you'd want to properly parse the SSH key format
    // and use OpenSSL RSA functions to verify

    // Note: Full SSH key parsing is complex. This is a placeholder implementation.
    // For production use, consider using libssh or similar library.
    return false; // Placeholder - requires full SSH key format parsing
}

bool PublicKey::verifyECDSASignature(const std::string& key_data,
                                     const std::string& key_type,
                                     const std::string& data,
                                     const std::string& signature) {
    // ECDSA signature verification requires parsing SSH key format
    // This is a placeholder - full implementation requires SSH key format parsing
    return false;
}

bool PublicKey::verifyEd25519Signature(const std::string& key_data,
                                       const std::string& data,
                                       const std::string& signature) {
    // Ed25519 signature verification
    // This is a placeholder - full implementation requires Ed25519 support
    return false;
}

std::string PublicKey::base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.c_str(), encoded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    char buffer[1024];
    std::string decoded;
    int length;
    while ((length = BIO_read(bio, buffer, sizeof(buffer))) > 0) {
        decoded.append(buffer, length);
    }

    BIO_free_all(bio);
    return decoded;
}

// PublicKeyDatabase implementation
PublicKeyDatabase::PublicKeyDatabase(const std::string& key_file)
    : key_file_(key_file), loaded_(false) {
    if (!key_file_.empty()) {
        load();
    }
}

bool PublicKeyDatabase::load() {
    if (key_file_.empty()) {
        return false;
    }

    keys_.clear();
    std::ifstream file(key_file_);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string current_username = "";
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check for username line (format: "username: key-string")
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            current_username = line.substr(0, colon_pos);
            line = line.substr(colon_pos + 1);
            // Trim leading whitespace
            line.erase(0, line.find_first_not_of(" \t"));
        }

        if (current_username.empty()) {
            continue;
        }

        // Parse public key
        auto key_info = PublicKey::parse(line);
        if (key_info) {
            key_info->username = current_username;
            keys_[current_username].push_back(*key_info);
        }
    }

    file.close();
    loaded_ = true;
    return true;
}

bool PublicKeyDatabase::save() {
    if (key_file_.empty()) {
        return false;
    }

    std::ofstream file(key_file_);
    if (!file.is_open()) {
        return false;
    }

    file << "# Public key database\n";
    file << "# Format: username: key-type key-data [comment]\n";

    for (const auto& [username, user_keys] : keys_) {
        for (const auto& key_info : user_keys) {
            file << username << ": " << key_info.key_type << " " << key_info.key_data;
            if (!key_info.comment.empty()) {
                file << " " << key_info.comment;
            }
            file << "\n";
        }
    }

    file.close();
    return true;
}

bool PublicKeyDatabase::addKey(const std::string& username, const std::string& key_string) {
    auto key_info = PublicKey::parse(key_string);
    if (!key_info) {
        return false;
    }

    key_info->username = username;
    keys_[username].push_back(*key_info);

    if (!key_file_.empty()) {
        return save();
    }
    return true;
}

bool PublicKeyDatabase::removeKey(const std::string& username, const std::string& key_data) {
    auto it = keys_.find(username);
    if (it == keys_.end()) {
        return false;
    }

    auto& user_keys = it->second;
    user_keys.erase(
        std::remove_if(user_keys.begin(), user_keys.end(),
                      [&key_data](const PublicKeyInfo& key) {
                          return key.key_data == key_data;
                      }),
        user_keys.end()
    );

    if (user_keys.empty()) {
        keys_.erase(it);
    }

    if (!key_file_.empty()) {
        return save();
    }
    return true;
}

std::vector<PublicKeyInfo> PublicKeyDatabase::getUserKeys(const std::string& username) const {
    auto it = keys_.find(username);
    if (it == keys_.end()) {
        return {};
    }
    return it->second;
}

bool PublicKeyDatabase::userHasKeys(const std::string& username) const {
    return keys_.find(username) != keys_.end() && !keys_.at(username).empty();
}

std::vector<std::string> PublicKeyDatabase::getUsers() const {
    std::vector<std::string> users;
    for (const auto& [username, _] : keys_) {
        users.push_back(username);
    }
    return users;
}

bool PublicKeyDatabase::verifySignature(const std::string& username,
                                        const std::string& data,
                                        const std::string& signature) const {
    auto it = keys_.find(username);
    if (it == keys_.end()) {
        return false;
    }

    // Try to verify with any of the user's keys
    for (const auto& key_info : it->second) {
        if (PublicKey::verifySignature(key_info, data, signature)) {
            return true;
        }
    }

    return false;
}

bool PublicKeyDatabase::reload() {
    return load();
}

// Authentication manager implementation
AuthenticationManager::AuthenticationManager(const AuthConfig& config) : config_(config) {
    if (config_.method == "password") {
        if (!config_.password_file.empty()) {
            password_file_ = std::make_unique<PasswordFile>(config_.password_file);
        }
        // Initialize user database if configured (future enhancement)
        user_database_ = std::make_unique<UserDatabase>();
        use_user_database_ = false; // Use password file by default
    } else if (config_.method == "public_key") {
        if (!config_.public_key_file.empty()) {
            public_key_database_ = std::make_unique<PublicKeyDatabase>(config_.public_key_file);
        }
    }

    // Initialize session manager with default timeout (1 hour)
    session_manager_ = std::make_unique<SessionManager>(std::chrono::seconds(3600));
}

std::string AuthenticationManager::authenticate(const std::string& username, const std::string& password,
                                               const std::string& client_address) {
    if (!config_.enabled) {
        // If auth is disabled, allow anonymous access if configured
        if (config_.anonymous_access) {
            return session_manager_->createSession("anonymous", client_address);
        }
        return "";
    }

    if (config_.method == "password") {
        bool authenticated = false;

        // Check if user is explicitly denied
        if (std::find(config_.denied_users.begin(), config_.denied_users.end(), username) != config_.denied_users.end()) {
            return "";
        }

        // Check if user is explicitly allowed (if allowed_users is not empty)
        if (!config_.allowed_users.empty()) {
            if (std::find(config_.allowed_users.begin(), config_.allowed_users.end(), username) == config_.allowed_users.end()) {
                return "";
            }
        }

        // Authenticate using password file or user database
        if (use_user_database_ && user_database_) {
            auto user_info = user_database_->getUserInfo(username);
            if (user_info) {
                authenticated = PasswordHasher::verifyPassword(password, user_info->password_hash);
                if (authenticated) {
                    // Update last login
                    // Note: This would require making user_info mutable or updating the database
                }
            }
        } else if (password_file_) {
            authenticated = password_file_->authenticate(username, password);
        }

        if (authenticated) {
            return session_manager_->createSession(username, client_address);
        }
    }

    if (config_.method == "public_key") {
        // Public key authentication handled separately via authenticateWithKey
        return "";
    }

    // Other auth methods not implemented yet
    return "";
}

std::string AuthenticationManager::authenticateWithKey(const std::string& username,
                                                       const std::string& data,
                                                       const std::string& signature,
                                                       const std::string& client_address) {
    if (!config_.enabled) {
        if (config_.anonymous_access) {
            return session_manager_->createSession("anonymous", client_address);
        }
        return "";
    }

    if (config_.method != "public_key") {
        return "";
    }

    // Check if user is explicitly denied
    if (std::find(config_.denied_users.begin(), config_.denied_users.end(), username) != config_.denied_users.end()) {
        return "";
    }

    // Check if user is explicitly allowed (if allowed_users is not empty)
    if (!config_.allowed_users.empty()) {
        if (std::find(config_.allowed_users.begin(), config_.allowed_users.end(), username) == config_.allowed_users.end()) {
            return "";
        }
    }

    // Verify signature using public key database
    if (public_key_database_) {
        if (public_key_database_->verifySignature(username, data, signature)) {
            return session_manager_->createSession(username, client_address);
        }
    }

    return "";
}

bool AuthenticationManager::authenticateUser(const std::string& username, const std::string& password) {
    std::string session_id = authenticate(username, password);
    return !session_id.empty();
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
    if (user_database_ && !use_user_database_) {
        return user_database_->load();
    }
    if (public_key_database_) {
        return public_key_database_->reload();
    }
    return true;
}

SessionManager& AuthenticationManager::getSessionManager() {
    return *session_manager_;
}

UserDatabase& AuthenticationManager::getUserDatabase() {
    return *user_database_;
}

PublicKeyDatabase& AuthenticationManager::getPublicKeyDatabase() {
    if (!public_key_database_) {
        // Initialize if not already initialized
        public_key_database_ = std::make_unique<PublicKeyDatabase>(config_.public_key_file);
    }
    return *public_key_database_;
}

} // namespace simple_rsyncd
