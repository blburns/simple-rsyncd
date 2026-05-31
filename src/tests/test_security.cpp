/*
 * Copyright 2024 SimpleDaemons
 * Licensed under the Apache License, Version 2.0
 */

#include <gtest/gtest.h>
#include "simple-rsyncd/security/network_access.hpp"
#include "simple-rsyncd/security/rate_limiter.hpp"
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/core/module.hpp"
#include <filesystem>
#include <fstream>

namespace simple_rsyncd {

TEST(NetworkAccessTest, ExactIpMatch) {
    EXPECT_TRUE(NetworkAccess::hostMatchesRule("192.168.1.10", "192.168.1.10"));
    EXPECT_FALSE(NetworkAccess::hostMatchesRule("192.168.1.11", "192.168.1.10"));
}

TEST(NetworkAccessTest, CidrMatch) {
    EXPECT_TRUE(NetworkAccess::hostMatchesRule("10.0.5.1", "10.0.0.0/8"));
    EXPECT_FALSE(NetworkAccess::hostMatchesRule("172.16.0.1", "10.0.0.0/8"));
}

TEST(NetworkAccessTest, AllowDenyLists) {
    EXPECT_TRUE(NetworkAccess::isAllowed("10.0.0.5", {}, {}, {}, {}));
    EXPECT_FALSE(NetworkAccess::isAllowed("10.0.0.5", {}, {"10.0.0.0/8"}, {}, {}));
    EXPECT_FALSE(NetworkAccess::isAllowed("10.0.0.5", {}, {}, {"192.168.0.0/16"}, {}));
    EXPECT_TRUE(NetworkAccess::isAllowed("192.168.1.2", {}, {}, {"192.168.1.0/24"}, {}));
}

TEST(RateLimiterTest, EnforcesPerMinuteLimit) {
    ConnectionRateLimiter limiter(2, 0);
    EXPECT_TRUE(limiter.allowConnection("1.2.3.4"));
    limiter.recordConnection("1.2.3.4");
    EXPECT_TRUE(limiter.allowConnection("1.2.3.4"));
    limiter.recordConnection("1.2.3.4");
    EXPECT_FALSE(limiter.allowConnection("1.2.3.4"));
}

TEST(AuthSecurityTest, RejectsPlaintextPasswordsWhenConfigured) {
    const auto path = std::filesystem::temp_directory_path() / "reject_plain_test.passwd";
    {
        std::ofstream file(path);
        file << "user:plainsecret\n";
        file << "hashed:" << PasswordHasher::hashPassword("secret") << "\n";
    }

    PasswordFile plain_ok(path.string(), false);
    EXPECT_EQ(plain_ok.getUserCount(), 2u);
    EXPECT_TRUE(plain_ok.authenticate("user", "plainsecret"));

    PasswordFile plain_reject(path.string(), true);
    EXPECT_EQ(plain_reject.getUserCount(), 1u);
    EXPECT_FALSE(plain_reject.authenticate("user", "plainsecret"));
    EXPECT_TRUE(plain_reject.authenticate("hashed", "secret"));

    std::filesystem::remove(path);
}

TEST(ModuleSecurityTest, BlocksSymlinkEscape) {
    const auto base = std::filesystem::temp_directory_path() / "rsyncd_symlink_test";
    const auto outside = std::filesystem::temp_directory_path() / "rsyncd_outside_secret.txt";
    std::filesystem::remove_all(base);
    std::filesystem::remove(outside);

    std::filesystem::create_directories(base);
    std::ofstream(outside).put('x');
    std::filesystem::create_directory_symlink(outside, base / "escape.link");

    ModuleConfig config;
    config.name = "test";
    config.path = base.string();
    config.allow_symlinks = false;
    config.list = true;

    auto module = createModule(config);
    ASSERT_NE(module, nullptr);
    EXPECT_FALSE(module->fileExists("escape.link"));
    EXPECT_TRUE(module->getFileInfo("escape.link").path.empty());

    std::filesystem::remove_all(base);
    std::filesystem::remove(outside);
}

}  // namespace simple_rsyncd
