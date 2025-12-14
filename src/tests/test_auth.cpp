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

#include <gtest/gtest.h>
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/config/config.hpp"
#include <fstream>
#include <cstdio>
#include <filesystem>

namespace simple_rsyncd {

class AuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary password file
        password_file_ = std::tmpnam(nullptr);
        password_file_ += ".passwd";

        std::ofstream file(password_file_);
        file << "# Password file\n";
        file << "user1:password1\n";
        file << "user2:password2\n";
        file << "admin:admin123\n";
        file.close();
    }

    void TearDown() override {
        if (std::filesystem::exists(password_file_)) {
            std::filesystem::remove(password_file_);
        }
    }

    std::string password_file_;
};

TEST_F(AuthTest, PasswordFileParsing) {
    PasswordFile pf(password_file_);
    EXPECT_TRUE(pf.getUserCount() > 0);
}

TEST_F(AuthTest, AuthenticationFlow) {
    PasswordFile pf(password_file_);
    
    EXPECT_TRUE(pf.authenticate("user1", "password1"));
    EXPECT_TRUE(pf.authenticate("user2", "password2"));
    EXPECT_FALSE(pf.authenticate("user1", "wrongpassword"));
    EXPECT_FALSE(pf.authenticate("nonexistent", "password"));
}

TEST_F(AuthTest, UserManagement) {
    PasswordFile pf(password_file_);
    
    EXPECT_TRUE(pf.userExists("user1"));
    EXPECT_TRUE(pf.userExists("user2"));
    EXPECT_FALSE(pf.userExists("nonexistent"));
    
    auto users = pf.getUsers();
    EXPECT_GE(users.size(), 3);
}

TEST_F(AuthTest, AuthenticationManager) {
    AuthConfig auth_config;
    auth_config.enabled = true;
    auth_config.method = "password";
    auth_config.password_file = password_file_;
    auth_config.anonymous_access = false;

    AuthenticationManager auth_mgr(auth_config);
    
    EXPECT_TRUE(auth_mgr.isEnabled());
    EXPECT_FALSE(auth_mgr.allowsAnonymous());
    EXPECT_TRUE(auth_mgr.authenticate("user1", "password1"));
    EXPECT_FALSE(auth_mgr.authenticate("user1", "wrongpassword"));
}

TEST_F(AuthTest, AllowDenyLists) {
    AuthConfig auth_config;
    auth_config.enabled = true;
    auth_config.method = "password";
    auth_config.password_file = password_file_;
    auth_config.allowed_users.push_back("user1");
    auth_config.denied_users.push_back("user2");

    AuthenticationManager auth_mgr(auth_config);
    
    EXPECT_TRUE(auth_mgr.authenticate("user1", "password1"));
    EXPECT_FALSE(auth_mgr.authenticate("user2", "password2")); // Denied
    EXPECT_FALSE(auth_mgr.authenticate("admin", "admin123")); // Not in allowed list
}

} // namespace simple_rsyncd
