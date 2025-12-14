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
#include "simple-rsyncd/core/module.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace simple_rsyncd {

class ModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for module
        test_dir_ = std::tmpnam(nullptr);
        test_dir_ += "_test";
        std::filesystem::create_directories(test_dir_);

        // Create test file
        test_file_ = test_dir_ + "/test.txt";
        std::ofstream file(test_file_);
        file << "test content\n";
        file.close();

        // Create module config
        config_.name = "test";
        config_.path = test_dir_;
        config_.comment = "Test module";
        config_.read_only = false;
        config_.list = true;
        config_.allow_delete = true;
        config_.overwrite = true;

        // Create module
        module_ = createModule(config_);
    }

    void TearDown() override {
        // Clean up
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::string test_file_;
    ModuleConfig config_;
    std::shared_ptr<Module> module_;
};

TEST_F(ModuleTest, CreateModule) {
    EXPECT_NE(module_, nullptr);
    EXPECT_TRUE(module_->validate());
}

TEST_F(ModuleTest, GetName) {
    EXPECT_EQ(module_->getName(), "test");
}

TEST_F(ModuleTest, GetPath) {
    EXPECT_EQ(module_->getPath(), test_dir_);
}

TEST_F(ModuleTest, FileExists) {
    EXPECT_TRUE(module_->fileExists("test.txt"));
    EXPECT_FALSE(module_->fileExists("nonexistent.txt"));
}

TEST_F(ModuleTest, GetFileInfo) {
    FileInfo info = module_->getFileInfo("test.txt");
    EXPECT_FALSE(info.path.empty());
    EXPECT_EQ(info.name, "test.txt");
    EXPECT_GT(info.size, 0);
}

TEST_F(ModuleTest, ListDirectory) {
    DirectoryListing listing = module_->listDirectory("", false);
    EXPECT_GT(listing.total_files, 0);
    EXPECT_GE(listing.files.size(), 1);
}

TEST_F(ModuleTest, CreateDirectory) {
    EXPECT_TRUE(module_->createDirectory("newdir"));
    EXPECT_TRUE(module_->directoryExists("newdir"));
}

TEST_F(ModuleTest, DeleteFile) {
    // Create a file to delete
    std::string delete_file = test_dir_ + "/delete_me.txt";
    std::ofstream file(delete_file);
    file << "delete me\n";
    file.close();

    EXPECT_TRUE(module_->deleteFile("delete_me.txt"));
    EXPECT_FALSE(module_->fileExists("delete_me.txt"));
}

TEST_F(ModuleTest, ReadOnlyModule) {
    ModuleConfig ro_config = config_;
    ro_config.read_only = true;
    auto ro_module = createModule(ro_config);

    EXPECT_TRUE(ro_module->isReadOnly());
    EXPECT_FALSE(ro_module->createDirectory("test"));
}

TEST_F(ModuleTest, PathValidation) {
    // Test path is within module root
    EXPECT_TRUE(module_->fileExists("test.txt"));
    EXPECT_FALSE(module_->fileExists("../../etc/passwd")); // Path traversal attempt
}

TEST_F(ModuleTest, PermissionChecking) {
    // Test read permission
    EXPECT_TRUE(module_->fileExists("test.txt"));
    
    // Test write permission (module is not read-only)
    EXPECT_FALSE(module_->isReadOnly());
    
    // Test delete permission
    std::string delete_file = test_dir_ + "/delete_test.txt";
    std::ofstream file(delete_file);
    file << "test\n";
    file.close();
    
    EXPECT_TRUE(module_->deleteFile("delete_test.txt"));
}

TEST_F(ModuleTest, ModuleOperations) {
    // Test path validation
    EXPECT_TRUE(module_->fileExists("test.txt"));
    
    // Test directory operations
    EXPECT_TRUE(module_->createDirectory("testdir"));
    EXPECT_TRUE(module_->directoryExists("testdir"));
    
    // Test file operations
    FileInfo info = module_->getFileInfo("test.txt");
    EXPECT_FALSE(info.path.empty());
}

} // namespace simple_rsyncd
