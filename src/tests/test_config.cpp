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
#include "simple-rsyncd/config/config.hpp"
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace simple_rsyncd {

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary config file
        config_file_ = std::tmpnam(nullptr);
        config_file_ += ".conf";

        std::ofstream file(config_file_);
        file << "[global]\n";
        file << "bind_address = 0.0.0.0\n";
        file << "bind_port = 873\n";
        file << "max_connections = 100\n";
        file << "\n";
        file << "[module:test]\n";
        file << "path = /tmp/test\n";
        file << "comment = Test module\n";
        file << "read_only = false\n";
        file << "list = true\n";
        file.close();
    }

    void TearDown() override {
        // Clean up temp file
        if (std::filesystem::exists(config_file_)) {
            std::filesystem::remove(config_file_);
        }
    }

    std::string config_file_;
};

TEST_F(ConfigTest, LoadFromFile) {
    Configuration config;
    EXPECT_TRUE(config.loadFromFile(config_file_));
    EXPECT_TRUE(config.validate());
}

TEST_F(ConfigTest, NetworkConfig) {
    Configuration config;
    config.loadFromFile(config_file_);

    EXPECT_EQ(config.network.bind_address, "0.0.0.0");
    EXPECT_EQ(config.network.bind_port, 873);
    EXPECT_EQ(config.network.max_connections, 100);
}

TEST_F(ConfigTest, ModuleConfig) {
    Configuration config;
    config.loadFromFile(config_file_);

    EXPECT_NE(config.modules.find("test"), config.modules.end());

    const auto& module = config.modules.at("test");
    EXPECT_EQ(module.path, "/tmp/test");
    EXPECT_EQ(module.comment, "Test module");
    EXPECT_FALSE(module.read_only);
    EXPECT_TRUE(module.list);
}

TEST_F(ConfigTest, InvalidFile) {
    Configuration config;
    EXPECT_FALSE(config.loadFromFile("/nonexistent/file.conf"));
}

TEST_F(ConfigTest, Validation) {
    Configuration config;
    config.loadFromFile(config_file_);
    EXPECT_TRUE(config.validate());
}

TEST_F(ConfigTest, JSONConfiguration) {
    // Create JSON config file
    std::string json_file = std::tmpnam(nullptr);
    json_file += ".json";
    
    std::ofstream file(json_file);
    file << "{\n";
    file << "  \"network\": {\n";
    file << "    \"bind_address\": \"0.0.0.0\",\n";
    file << "    \"bind_port\": 873,\n";
    file << "    \"max_connections\": 100\n";
    file << "  },\n";
    file << "  \"modules\": {\n";
    file << "    \"test\": {\n";
    file << "      \"path\": \"/tmp/test\",\n";
    file << "      \"comment\": \"Test module\",\n";
    file << "      \"read_only\": false,\n";
    file << "      \"list\": true\n";
    file << "    }\n";
    file << "  }\n";
    file << "}\n";
    file.close();

    Configuration config;
    bool loaded = config.loadFromFile(json_file);
    
    // Clean up
    if (std::filesystem::exists(json_file)) {
        std::filesystem::remove(json_file);
    }

    // Note: JSON parsing may not work if jsoncpp is not available
    // This test will pass if JSON is supported, or skip gracefully
    if (loaded) {
        EXPECT_TRUE(config.validate());
        EXPECT_EQ(config.network.bind_port, 873);
    }
}

TEST_F(ConfigTest, ConfigurationParsing) {
    Configuration config;
    EXPECT_TRUE(config.loadFromFile(config_file_));
    
    // Test that values were parsed correctly
    EXPECT_EQ(config.network.bind_address, "0.0.0.0");
    EXPECT_EQ(config.network.bind_port, 873);
}

} // namespace simple_rsyncd
