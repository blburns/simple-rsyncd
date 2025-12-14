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
#include "simple-rsyncd/core/daemon.hpp"
#include "simple-rsyncd/core/session.hpp"
#include "simple-rsyncd/core/protocol.hpp"
#include "simple-rsyncd/core/module.hpp"
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/config/config.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace simple_rsyncd {

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directories
        auto temp_path = std::filesystem::temp_directory_path();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        test_dir_ = (temp_path / ("test_integration_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen)))).string();
        std::filesystem::create_directories(test_dir_);

        server_dir_ = test_dir_ + "/server";
        client_dir_ = test_dir_ + "/client";
        std::filesystem::create_directories(server_dir_);
        std::filesystem::create_directories(client_dir_);

        // Create test files
        test_file_ = client_dir_ + "/test.txt";
        std::ofstream file(test_file_);
        file << "Test file content for integration testing\n";
        file << "Line 2\n";
        file << "Line 3\n";
        file.close();

        // Setup configuration
        config_ = std::make_shared<Configuration>();
        config_->network.bind_address = "127.0.0.1";
        config_->network.bind_port = 0; // Let OS assign port
        config_->network.max_connections = 10;

        // Setup module
        ModuleConfig module_config;
        module_config.name = "test";
        module_config.path = server_dir_;
        module_config.comment = "Test module";
        module_config.read_only = false;
        module_config.list = true;
        module_config.allow_delete = true;
        module_config.overwrite = true;

        modules_["test"] = createModule(module_config);

        // Setup authentication (disabled for basic tests)
        auth_config_.enabled = false;
        auth_config_.anonymous_access = true;
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::string server_dir_;
    std::string client_dir_;
    std::string test_file_;
    std::shared_ptr<Configuration> config_;
    std::map<std::string, std::shared_ptr<Module>> modules_;
    AuthConfig auth_config_;
};

// End-to-end tests
TEST_F(IntegrationTest, FullTransferFlow) {
    // Create a test file to transfer
    std::string source_file = client_dir_ + "/source.txt";
    std::ofstream source(source_file);
    source << "This is a test file for transfer\n";
    source << "Multiple lines\n";
    source << "End of file\n";
    source.close();

    // Get file size
    auto file_size = std::filesystem::file_size(source_file);

    // Use module to transfer file
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Transfer file to server
    TransferResult result = module->transferFile(source_file, "destination.txt", true);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stats.files_transferred, 1);
    EXPECT_EQ(result.transferred_files.size(), 1);

    // Verify file exists on server
    std::string dest_path = server_dir_ + "/destination.txt";
    EXPECT_TRUE(std::filesystem::exists(dest_path));
    EXPECT_EQ(std::filesystem::file_size(dest_path), file_size);

    // Verify content
    std::ifstream dest(dest_path);
    std::string content((std::istreambuf_iterator<char>(dest)),
                        std::istreambuf_iterator<char>());
    dest.close();

    std::ifstream src(source_file);
    std::string expected((std::istreambuf_iterator<char>(src)),
                         std::istreambuf_iterator<char>());
    src.close();

    EXPECT_EQ(content, expected);
}

TEST_F(IntegrationTest, FullDownloadFlow) {
    // Create a file on server
    std::string server_file = server_dir_ + "/download.txt";
    std::ofstream server(server_file);
    server << "File to download\n";
    server << "Content line 2\n";
    server.close();

    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Download file from server
    std::string local_file = client_dir_ + "/downloaded.txt";
    TransferResult result = module->receiveFile("download.txt", local_file);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stats.files_transferred, 1);

    // Verify file exists locally
    EXPECT_TRUE(std::filesystem::exists(local_file));

    // Verify content
    std::ifstream local(local_file);
    std::string content((std::istreambuf_iterator<char>(local)),
                        std::istreambuf_iterator<char>());
    local.close();

    std::ifstream server_read(server_file);
    std::string expected((std::istreambuf_iterator<char>(server_read)),
                         std::istreambuf_iterator<char>());
    server_read.close();

    EXPECT_EQ(content, expected);
}

TEST_F(IntegrationTest, DirectoryListingFlow) {
    // Create multiple files on server
    for (int i = 0; i < 5; ++i) {
        std::string file = server_dir_ + "/file" + std::to_string(i) + ".txt";
        std::ofstream f(file);
        f << "Content " << i << "\n";
        f.close();
    }

    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // List directory
    DirectoryListing listing = module->listDirectory("", false);
    EXPECT_GE(listing.total_files, 5);
    EXPECT_GE(listing.files.size(), 5);

    // Verify files are in listing
    std::set<std::string> found_files;
    for (const auto& file_info : listing.files) {
        found_files.insert(file_info.name);
    }

    for (int i = 0; i < 5; ++i) {
        std::string filename = "file" + std::to_string(i) + ".txt";
        EXPECT_NE(found_files.find(filename), found_files.end());
    }
}

// Authentication flow tests
TEST_F(IntegrationTest, AuthenticationFlow) {
    // Create password file
    std::string password_file = test_dir_ + "/passwd";
    std::ofstream pf(password_file);
    pf << "testuser:testpass\n";
    pf << "admin:admin123\n";
    pf.close();

    // Setup authentication
    AuthConfig auth;
    auth.enabled = true;
    auth.method = "password";
    auth.password_file = password_file;
    auth.anonymous_access = false;

    AuthenticationManager auth_mgr(auth);

    // Test successful authentication
    EXPECT_TRUE(auth_mgr.authenticate("testuser", "testpass"));
    EXPECT_TRUE(auth_mgr.authenticate("admin", "admin123"));

    // Test failed authentication
    EXPECT_FALSE(auth_mgr.authenticate("testuser", "wrongpass"));
    EXPECT_FALSE(auth_mgr.authenticate("nonexistent", "password"));
}

TEST_F(IntegrationTest, AuthenticationWithAllowDenyLists) {
    // Create password file
    std::string password_file = test_dir_ + "/passwd";
    std::ofstream pf(password_file);
    pf << "user1:pass1\n";
    pf << "user2:pass2\n";
    pf << "user3:pass3\n";
    pf.close();

    // Setup authentication with allow/deny lists
    AuthConfig auth;
    auth.enabled = true;
    auth.method = "password";
    auth.password_file = password_file;
    auth.allowed_users.push_back("user1");
    auth.allowed_users.push_back("user2");
    auth.denied_users.push_back("user2"); // Deny overrides allow

    AuthenticationManager auth_mgr(auth);

    // Test allowed user
    EXPECT_TRUE(auth_mgr.authenticate("user1", "pass1"));

    // Test denied user (even with correct password)
    EXPECT_FALSE(auth_mgr.authenticate("user2", "pass2"));

    // Test user not in allowed list
    EXPECT_FALSE(auth_mgr.authenticate("user3", "pass3"));
}

// Error scenario tests
TEST_F(IntegrationTest, ErrorScenario_FileNotFound) {
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Try to get info for non-existent file
    FileInfo info = module->getFileInfo("nonexistent.txt");
    EXPECT_TRUE(info.path.empty());

    // Try to download non-existent file
    TransferResult result = module->receiveFile("nonexistent.txt", client_dir_ + "/out.txt");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(IntegrationTest, ErrorScenario_PathTraversal) {
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Try path traversal attack
    EXPECT_FALSE(module->fileExists("../../etc/passwd"));
    EXPECT_FALSE(module->fileExists("../test.txt"));
    EXPECT_FALSE(module->fileExists("/etc/passwd"));

    // Verify path is sanitized
    FileInfo info = module->getFileInfo("../../etc/passwd");
    EXPECT_TRUE(info.path.empty());
}

TEST_F(IntegrationTest, ErrorScenario_ReadOnlyModule) {
    // Create read-only module
    ModuleConfig ro_config;
    ro_config.name = "readonly";
    ro_config.path = server_dir_;
    ro_config.read_only = true;
    ro_config.list = true;

    auto ro_module = createModule(ro_config);

    // Try to create directory in read-only module
    EXPECT_FALSE(ro_module->createDirectory("newdir"));

    // Try to delete file in read-only module
    std::string test_file = server_dir_ + "/delete_me.txt";
    std::ofstream file(test_file);
    file << "test\n";
    file.close();

    EXPECT_FALSE(ro_module->deleteFile("delete_me.txt"));

    // Try to transfer file to read-only module
    TransferResult result = ro_module->transferFile(test_file_, "upload.txt", true);
    EXPECT_FALSE(result.success);
}

TEST_F(IntegrationTest, ErrorScenario_InvalidPath) {
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Try operations with invalid paths
    EXPECT_FALSE(module->fileExists(""));
    EXPECT_FALSE(module->directoryExists(""));

    DirectoryListing listing = module->listDirectory("invalid/path/that/does/not/exist", false);
    EXPECT_EQ(listing.total_files, 0);
}

TEST_F(IntegrationTest, ErrorScenario_ModuleNotAllowsDelete) {
    // Create module that doesn't allow deletion
    ModuleConfig no_delete_config;
    no_delete_config.name = "nodelete";
    no_delete_config.path = server_dir_;
    no_delete_config.read_only = false;
    no_delete_config.allow_delete = false;

    auto no_delete_module = createModule(no_delete_config);

    // Create a file
    std::string test_file = server_dir_ + "/delete_test.txt";
    std::ofstream file(test_file);
    file << "test\n";
    file.close();

    // Try to delete - should fail
    EXPECT_FALSE(no_delete_module->deleteFile("delete_test.txt"));
    EXPECT_TRUE(std::filesystem::exists(test_file)); // File should still exist
}

// Protocol compatibility tests
TEST_F(IntegrationTest, ProtocolCompatibility_CommandParsing) {
    ProtocolParser parser;

    // Test all command types
    std::vector<std::pair<std::string, ProtocolCommand>> commands = {
        {"LIST test /path", ProtocolCommand::LIST},
        {"GET test /file.txt", ProtocolCommand::GET},
        {"PUT test /file.txt", ProtocolCommand::PUT},
        {"DELETE test /file.txt", ProtocolCommand::DELETE},
        {"STAT test /file.txt", ProtocolCommand::STAT}
    };

    for (const auto& [cmd_str, expected_cmd] : commands) {
        ProtocolMessage msg = parser.parse(cmd_str + "\n");
        EXPECT_TRUE(msg.valid) << "Failed to parse: " << cmd_str;
        EXPECT_EQ(msg.command, expected_cmd) << "Wrong command for: " << cmd_str;
        EXPECT_EQ(msg.module, "test");
    }
}

TEST_F(IntegrationTest, ProtocolCompatibility_ResponseFormat) {
    ProtocolParser parser;

    // Test success response
    std::string success = parser.buildResponse(true, "Operation successful");
    EXPECT_NE(success.find("OK"), std::string::npos);
    EXPECT_NE(success.find("Operation successful"), std::string::npos);

    // Test error response
    std::string error = parser.buildErrorResponse(1, "File not found");
    EXPECT_NE(error.find("ERROR"), std::string::npos);
    EXPECT_NE(error.find("File not found"), std::string::npos);
}

TEST_F(IntegrationTest, ProtocolCompatibility_MessageWithArguments) {
    ProtocolParser parser;

    // Test message with arguments
    std::string msg_str = "LIST test /path recursive=true max_depth=5\n";
    ProtocolMessage msg = parser.parse(msg_str);

    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::LIST);
    EXPECT_EQ(msg.module, "test");
    EXPECT_EQ(msg.path, "/path");
    EXPECT_NE(msg.arguments.find("recursive"), msg.arguments.end());
    EXPECT_NE(msg.arguments.find("max_depth"), msg.arguments.end());
    EXPECT_EQ(msg.arguments.at("recursive"), "true");
    EXPECT_EQ(msg.arguments.at("max_depth"), "5");
}

TEST_F(IntegrationTest, ProtocolCompatibility_ErrorHandling) {
    ProtocolParser parser;

    // Test empty message
    ProtocolMessage msg1 = parser.parse("");
    EXPECT_FALSE(msg1.valid);

    // Test malformed message
    ProtocolMessage msg2 = parser.parse("INVALID COMMAND\n");
    EXPECT_FALSE(msg2.valid);
    EXPECT_EQ(msg2.command, ProtocolCommand::UNKNOWN);

    // Test incomplete message
    ProtocolMessage msg3 = parser.parse("LIST\n");
    // May or may not be valid depending on implementation
    // But should not crash
}

// Full integration scenario
TEST_F(IntegrationTest, FullScenario_UploadDownloadDelete) {
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Step 1: Upload file
    std::string upload_file = client_dir_ + "/upload.txt";
    std::ofstream upload(upload_file);
    upload << "Upload test content\n";
    upload.close();

    TransferResult upload_result = module->transferFile(upload_file, "uploaded.txt", true);
    EXPECT_TRUE(upload_result.success);

    // Step 2: Verify file exists
    EXPECT_TRUE(module->fileExists("uploaded.txt"));
    FileInfo info = module->getFileInfo("uploaded.txt");
    EXPECT_FALSE(info.path.empty());
    EXPECT_GT(info.size, 0);

    // Step 3: Download file
    std::string download_file = client_dir_ + "/downloaded.txt";
    TransferResult download_result = module->receiveFile("uploaded.txt", download_file);
    EXPECT_TRUE(download_result.success);

    // Step 4: Verify downloaded content
    std::ifstream downloaded(download_file);
    std::string content((std::istreambuf_iterator<char>(downloaded)),
                        std::istreambuf_iterator<char>());
    downloaded.close();
    EXPECT_NE(content.find("Upload test content"), std::string::npos);

    // Step 5: Delete file
    EXPECT_TRUE(module->deleteFile("uploaded.txt"));
    EXPECT_FALSE(module->fileExists("uploaded.txt"));
}

TEST_F(IntegrationTest, FullScenario_DirectoryOperations) {
    auto module = modules_["test"];
    ASSERT_NE(module, nullptr);

    // Step 1: Create directory
    EXPECT_TRUE(module->createDirectory("testdir", false));
    EXPECT_TRUE(module->directoryExists("testdir"));

    // Step 2: Create nested directory
    EXPECT_TRUE(module->createDirectory("testdir/nested", true));
    EXPECT_TRUE(module->directoryExists("testdir/nested"));

    // Step 3: List directory
    DirectoryListing listing = module->listDirectory("testdir", false);
    EXPECT_GT(listing.total_files, 0);

    // Step 4: Delete nested directory
    EXPECT_TRUE(module->deleteDirectory("testdir/nested", false));
    EXPECT_FALSE(module->directoryExists("testdir/nested"));

    // Step 5: Delete parent directory
    EXPECT_TRUE(module->deleteDirectory("testdir", true));
    EXPECT_FALSE(module->directoryExists("testdir"));
}

} // namespace simple_rsyncd
