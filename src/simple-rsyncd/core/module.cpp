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

#include "simple-rsyncd/core/module.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

namespace simple_rsyncd {


// FileSystemModule - Concrete implementation of Module
class FileSystemModule : public Module {
public:
    explicit FileSystemModule(const ModuleConfig& config) : Module(config) {
        config_.name = config.name;
    }

    DirectoryListing listDirectory(const std::string& path, bool recursive) override {
        DirectoryListing listing;
        listing.path = path;
        listing.recursive = recursive;
        listing.listing_time = std::chrono::system_clock::now();

        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return listing;
        }

        try {
            if (!std::filesystem::exists(full_path) || !std::filesystem::is_directory(full_path)) {
                return listing;
            }

            if (recursive) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(full_path)) {
                    addFileInfo(entry, listing, full_path);
                }
            } else {
                for (const auto& entry : std::filesystem::directory_iterator(full_path)) {
                    addFileInfo(entry, listing, full_path);
                }
            }
        } catch (const std::exception& e) {
            // Error handling
        }

        return listing;
    }

    FileInfo getFileInfo(const std::string& path) override {
        FileInfo info;
        std::string full_path = resolvePath(path);

        if (!validatePath(full_path)) {
            return info;
        }

        try {
            if (!std::filesystem::exists(full_path)) {
                return info;
            }

            auto status = std::filesystem::status(full_path);
            auto file_status = std::filesystem::status(full_path);

            info.name = std::filesystem::path(full_path).filename().string();
            info.path = full_path;
            info.relative_path = path;
            info.type = file_status.type();
            info.size = std::filesystem::is_regular_file(full_path) ? std::filesystem::file_size(full_path) : 0;
            info.last_modified = std::chrono::system_clock::from_time_t(
                std::filesystem::last_write_time(full_path).time_since_epoch().count());
            info.is_symlink = std::filesystem::is_symlink(full_path);
            info.is_hardlink = file_status.type() == std::filesystem::file_type::regular &&
                              std::filesystem::hard_link_count(full_path) > 1;

            if (info.is_symlink) {
                info.symlink_target = std::filesystem::read_symlink(full_path).string();
            }
        } catch (const std::exception& e) {
            // Error handling
        }

        return info;
    }

    bool fileExists(const std::string& path) override {
        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return false;
        }
        return std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path);
    }

    bool directoryExists(const std::string& path) override {
        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return false;
        }
        return std::filesystem::exists(full_path) && std::filesystem::is_directory(full_path);
    }

    bool createDirectory(const std::string& path, bool recursive) override {
        if (isReadOnly()) {
            return false;
        }

        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return false;
        }

        try {
            if (recursive) {
                return std::filesystem::create_directories(full_path);
            } else {
                return std::filesystem::create_directory(full_path);
            }
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool deleteFile(const std::string& path) override {
        if (isReadOnly() || !allowsDeletion()) {
            return false;
        }

        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return false;
        }

        try {
            if (std::filesystem::is_regular_file(full_path)) {
                return std::filesystem::remove(full_path);
            }
        } catch (const std::exception& e) {
            return false;
        }

        return false;
    }

    bool deleteDirectory(const std::string& path, bool recursive) override {
        if (isReadOnly() || !allowsDeletion()) {
            return false;
        }

        std::string full_path = resolvePath(path);
        if (!validatePath(full_path)) {
            return false;
        }

        try {
            if (recursive) {
                return std::filesystem::remove_all(full_path) > 0;
            } else {
                return std::filesystem::remove(full_path);
            }
        } catch (const std::exception& e) {
            return false;
        }
    }

    TransferResult transferFile(const std::string& local_path, const std::string& remote_path, bool overwrite) override {
        TransferResult result;
        result.stats.start_time = std::chrono::steady_clock::now();

        if (isReadOnly()) {
            result.error_message = "Module is read-only";
            return result;
        }

        if (!overwrite && fileExists(remote_path)) {
            result.error_message = "File already exists and overwrite not allowed";
            return result;
        }

        std::string full_remote_path = resolvePath(remote_path);
        if (!validatePath(full_remote_path)) {
            result.error_message = "Invalid path";
            return result;
        }

        try {
            // Create parent directory if needed
            std::filesystem::path parent = std::filesystem::path(full_remote_path).parent_path();
            if (!parent.empty() && !std::filesystem::exists(parent)) {
                std::filesystem::create_directories(parent);
            }

            // Copy file
            std::filesystem::copy_file(local_path, full_remote_path,
                                      overwrite ? std::filesystem::copy_options::overwrite_existing
                                                : std::filesystem::copy_options::none);

            result.success = true;
            result.stats.files_transferred = 1;
            result.transferred_files.push_back(remote_path);
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.stats.failed_transfers = 1;
            result.failed_files.push_back(remote_path);
        }

        result.stats.end_time = std::chrono::steady_clock::now();
        return result;
    }

    TransferResult receiveFile(const std::string& remote_path, const std::string& local_path) override {
        TransferResult result;
        result.stats.start_time = std::chrono::steady_clock::now();

        std::string full_remote_path = resolvePath(remote_path);
        if (!validatePath(full_remote_path) || !fileExists(remote_path)) {
            result.error_message = "File does not exist or invalid path";
            return result;
        }

        try {
            // Create parent directory if needed
            std::filesystem::path parent = std::filesystem::path(local_path).parent_path();
            if (!parent.empty() && !std::filesystem::exists(parent)) {
                std::filesystem::create_directories(parent);
            }

            // Copy file
            std::filesystem::copy_file(full_remote_path, local_path,
                                      std::filesystem::copy_options::overwrite_existing);

            result.success = true;
            result.stats.files_transferred = 1;
            result.transferred_files.push_back(remote_path);
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.stats.failed_transfers = 1;
            result.failed_files.push_back(remote_path);
        }

        result.stats.end_time = std::chrono::steady_clock::now();
        return result;
    }

    TransferStats getStats() const override {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_;
    }

    void resetStats() override {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.reset();
    }

    bool validate() const override {
        if (config_.path.empty()) {
            return false;
        }
        return std::filesystem::exists(config_.path) && std::filesystem::is_directory(config_.path);
    }

    std::vector<std::string> getErrors() const override {
        std::vector<std::string> errors;
        if (config_.path.empty()) {
            errors.push_back("Module path is empty");
        } else if (!std::filesystem::exists(config_.path)) {
            errors.push_back("Module path does not exist: " + config_.path);
        } else if (!std::filesystem::is_directory(config_.path)) {
            errors.push_back("Module path is not a directory: " + config_.path);
        }
        return errors;
    }

    bool reload() override {
        // Reload would re-read configuration
        return validate();
    }

private:
    void addFileInfo(const std::filesystem::directory_entry& entry, DirectoryListing& listing, const std::string& base_path) {
        FileInfo info;
        try {
            auto status = entry.status();
            info.path = entry.path().string();
            info.relative_path = std::filesystem::relative(entry.path(), base_path).string();
            info.name = entry.path().filename().string();
            info.type = status.type();

            if (std::filesystem::is_regular_file(entry.path())) {
                info.size = std::filesystem::file_size(entry.path());
                listing.files.push_back(info);
                listing.total_files++;
            } else if (std::filesystem::is_directory(entry.path())) {
                listing.directories.push_back(info);
                listing.total_directories++;
            } else if (std::filesystem::is_symlink(entry.path())) {
                info.is_symlink = true;
                listing.symlinks.push_back(info);
                listing.total_symlinks++;
            } else {
                listing.other.push_back(info);
                listing.total_other++;
            }

            listing.total_size += info.size;
        } catch (const std::exception& e) {
            // Skip entries that can't be accessed
        }
    }
};

// Module base class implementation
Module::Module(const ModuleConfig& config) : config_(config) {
    stats_.reset();
}

std::string Module::getName() const {
    return config_.name;
}

std::string Module::getPath() const {
    return config_.path;
}

std::string Module::getComment() const {
    return config_.comment;
}

bool Module::isReadOnly() const {
    return config_.read_only;
}

bool Module::allowsListing() const {
    return config_.list;
}

bool Module::allowsDeletion() const {
    return config_.allow_delete;
}

bool Module::allowsOverwriting() const {
    return config_.overwrite;
}

std::string Module::resolvePath(const std::string& path) const {
    if (path.empty()) {
        return config_.path;
    }

    std::filesystem::path base_path(config_.path);
    std::filesystem::path relative_path(path);

    // Normalize the path to prevent directory traversal
    std::filesystem::path resolved = base_path / relative_path;
    resolved = resolved.lexically_normal();

    // Get absolute paths for comparison
    std::filesystem::path base_absolute = std::filesystem::absolute(base_path);
    std::filesystem::path resolved_absolute = std::filesystem::absolute(resolved);

    // Ensure resolved path is within module root
    std::string resolved_str = resolved_absolute.string();
    std::string base_str = base_absolute.string();

    // Check if resolved path starts with base path
    if (resolved_str.length() < base_str.length() || 
        resolved_str.substr(0, base_str.length()) != base_str) {
        return ""; // Path outside module root
    }

    // Additional check: ensure no ".." escaped
    if (resolved_str.find("..") != std::string::npos) {
        return ""; // Directory traversal attempt
    }

    return resolved_absolute.string();
}

bool Module::validatePath(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    std::string resolved = resolvePath(path);
    if (resolved.empty()) {
        return false;
    }

    // Check if path is excluded
    std::string filename = std::filesystem::path(path).filename().string();
    if (config_.isExcluded(filename)) {
        return false;
    }

    // Check if path is included (if include patterns exist)
    if (!config_.isIncluded(filename)) {
        return false;
    }

    return true;
}


// Factory function to create modules
std::shared_ptr<Module> createModule(const ModuleConfig& config) {
    return std::make_shared<FileSystemModule>(config);
}

} // namespace simple_rsyncd
