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

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <filesystem>

namespace simple_rsyncd {

/**
 * @brief File transfer statistics
 */
struct TransferStats {
    size_t bytes_transferred = 0;
    size_t files_transferred = 0;
    size_t directories_created = 0;
    size_t symlinks_created = 0;
    size_t hardlinks_created = 0;
    size_t failed_transfers = 0;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;

    /**
     * @brief Get transfer duration in seconds
     * @return Duration in seconds
     */
    double getDuration() const;

    /**
     * @brief Get transfer rate in bytes per second
     * @return Transfer rate in bytes per second
     */
    double getTransferRate() const;

    /**
     * @brief Reset statistics
     */
    void reset();
};

// TransferStats implementation
inline double TransferStats::getDuration() const {
    if (end_time.time_since_epoch().count() == 0) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        return duration.count() / 1000.0;
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return duration.count() / 1000.0;
}

inline double TransferStats::getTransferRate() const {
    double duration = getDuration();
    if (duration <= 0) return 0.0;
    return static_cast<double>(bytes_transferred) / duration;
}

inline void TransferStats::reset() {
    bytes_transferred = 0;
    files_transferred = 0;
    directories_created = 0;
    symlinks_created = 0;
    hardlinks_created = 0;
    failed_transfers = 0;
    start_time = std::chrono::steady_clock::now();
    end_time = start_time;
}

/**
 * @brief File information for rsync operations
 */
struct FileInfo {
    std::string name;
    std::string path;
    std::string relative_path;
    std::filesystem::file_type type;
    std::uintmax_t size;
    std::chrono::system_clock::time_point last_modified;
    std::chrono::system_clock::time_point last_access;
    std::chrono::system_clock::time_point created;
    std::string permissions;
    std::string owner;
    std::string group;
    bool is_symlink = false;
    bool is_hardlink = false;
    bool is_device = false;
    bool is_socket = false;
    std::string symlink_target;
    std::string checksum;
    std::string checksum_algorithm;

    /**
     * @brief Check if file is readable
     * @return true if readable, false otherwise
     */
    bool isReadable() const;

    /**
     * @brief Check if file is writable
     * @return true if writable, false otherwise
     */
    bool isWritable() const;

    /**
     * @brief Check if file is executable
     * @return true if executable, false otherwise
     */
    bool isExecutable() const;

    /**
     * @brief Get file size in human readable format
     * @return Human readable size string
     */
    std::string getHumanReadableSize() const;

    /**
     * @brief Get last modified time as string
     * @return Last modified time string
     */
    std::string getLastModifiedString() const;
};

/**
 * @brief Directory listing result
 */
struct DirectoryListing {
    std::string path;
    std::vector<FileInfo> files;
    std::vector<FileInfo> directories;
    std::vector<FileInfo> symlinks;
    std::vector<FileInfo> other;
    size_t total_files = 0;
    size_t total_directories = 0;
    size_t total_symlinks = 0;
    size_t total_other = 0;
    std::uintmax_t total_size = 0;
    bool recursive = false;
    std::chrono::system_clock::time_point listing_time;

    /**
     * @brief Get all items combined
     * @return Vector of all file info items
     */
    std::vector<FileInfo> getAllItems() const;

    /**
     * @brief Sort items by name
     */
    void sortByName();

    /**
     * @brief Sort items by size
     */
    void sortBySize();

    /**
     * @brief Sort items by modification time
     */
    void sortByModificationTime();

    /**
     * @brief Filter items by pattern
     * @param pattern Glob pattern to filter by
     * @return Filtered directory listing
     */
    DirectoryListing filterByPattern(const std::string& pattern) const;

    /**
     * @brief Filter items by type
     * @param type File type to filter by
     * @return Filtered directory listing
     */
    DirectoryListing filterByType(std::filesystem::file_type type) const;
};

/**
 * @brief Transfer operation result
 */
struct TransferResult {
    bool success = false;
    std::string error_message;
    TransferStats stats;
    std::vector<std::string> transferred_files;
    std::vector<std::string> failed_files;
    std::vector<std::string> skipped_files;
    std::chrono::steady_clock::time_point completion_time;

    /**
     * @brief Check if transfer was successful
     * @return true if successful, false otherwise
     */
    bool isSuccessful() const;

    /**
     * @brief Get summary of transfer
     * @return Summary string
     */
    std::string getSummary() const;
};

/**
 * @brief Module configuration
 */
struct ModuleConfig {
    std::string name;
    std::string path;
    std::string comment;
    bool read_only = false;
    bool list = true;
    bool allow_delete = false;
    bool overwrite = false;
    std::vector<std::string> exclude_patterns;
    std::vector<std::string> include_patterns;
    std::map<std::string, std::string> environment_variables;
    std::string pre_transfer_script;
    std::string post_transfer_script;
    std::string pre_delete_script;
    std::string post_delete_script;
    std::string pre_list_script;
    std::string post_list_script;
    std::map<std::string, std::string> custom_options;

    /**
     * @brief Check if file should be excluded
     * @param filename File name to check
     * @return true if excluded, false otherwise
     */
    bool isExcluded(const std::string& filename) const {
        for (const auto& pattern : exclude_patterns) {
            if (matchesPattern(filename, pattern)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check if file should be included
     * @param filename File name to check
     * @return true if included, false otherwise
     */
    bool isIncluded(const std::string& filename) const {
        if (include_patterns.empty()) {
            return true; // No include patterns means include all
        }
        for (const auto& pattern : include_patterns) {
            if (matchesPattern(filename, pattern)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get environment variable value
     * @param name Variable name
     * @param default_value Default value if not found
     * @return Environment variable value
     */
    std::string getEnvironmentVariable(const std::string& name, const std::string& default_value = "") const {
        auto it = environment_variables.find(name);
        if (it != environment_variables.end()) {
            return it->second;
        }
        return default_value;
    }

    /**
     * @brief Get custom option value
     * @param name Option name
     * @param default_value Default value if not found
     * @return Custom option value
     */
    std::string getCustomOption(const std::string& name, const std::string& default_value = "") const {
        auto it = custom_options.find(name);
        if (it != custom_options.end()) {
            return it->second;
        }
        return default_value;
    }

private:
    // Simple pattern matching helper
    static bool matchesPattern(const std::string& filename, const std::string& pattern) {
        // Simple glob matching - * matches anything
        if (pattern == "*") return true;
        if (pattern == filename) return true;

        // Check if pattern contains wildcard
        size_t star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            std::string prefix = pattern.substr(0, star_pos);
            std::string suffix = pattern.substr(star_pos + 1);

            if (filename.length() < prefix.length() + suffix.length()) {
                return false;
            }

            return filename.substr(0, prefix.length()) == prefix &&
                   filename.substr(filename.length() - suffix.length()) == suffix;
        }

        return false;
    }
};

/**
 * @brief Module interface for rsync operations
 */
class Module {
public:
    /**
     * @brief Constructor
     * @param config Module configuration
     */
    explicit Module(const ModuleConfig& config);

    /**
     * @brief Destructor
     */
    virtual ~Module() = default;

    /**
     * @brief Get module name
     * @return Module name
     */
    std::string getName() const;

    /**
     * @brief Get module path
     * @return Module path
     */
    std::string getPath() const;

    /**
     * @brief Get module comment
     * @return Module comment
     */
    std::string getComment() const;

    /**
     * @brief Check if module is read-only
     * @return true if read-only, false otherwise
     */
    bool isReadOnly() const;

    /**
     * @brief Check if module allows listing
     * @return true if listing allowed, false otherwise
     */
    bool allowsListing() const;

    /**
     * @brief Check if module allows deletion
     * @return true if deletion allowed, false otherwise
     */
    bool allowsDeletion() const;

    /**
     * @brief Check if module allows overwriting
     * @return true if overwriting allowed, false otherwise
     */
    bool allowsOverwriting() const;

    /**
     * @brief List directory contents
     * @param path Directory path (relative to module root)
     * @param recursive Whether to list recursively
     * @return Directory listing result
     */
    virtual DirectoryListing listDirectory(const std::string& path = "", bool recursive = false) = 0;

    /**
     * @brief Get file information
     * @param path File path (relative to module root)
     * @return File information
     */
    virtual FileInfo getFileInfo(const std::string& path) = 0;

    /**
     * @brief Check if file exists
     * @param path File path (relative to module root)
     * @return true if exists, false otherwise
     */
    virtual bool fileExists(const std::string& path) = 0;

    /**
     * @brief Check if directory exists
     * @param path Directory path (relative to module root)
     * @return true if exists, false otherwise
     */
    virtual bool directoryExists(const std::string& path) = 0;

    /**
     * @brief Create directory
     * @param path Directory path (relative to module root)
     * @param recursive Whether to create parent directories
     * @return true if created successfully, false otherwise
     */
    virtual bool createDirectory(const std::string& path, bool recursive = false) = 0;

    /**
     * @brief Delete file
     * @param path File path (relative to module root)
     * @return true if deleted successfully, false otherwise
     */
    virtual bool deleteFile(const std::string& path) = 0;

    /**
     * @brief Delete directory
     * @param path Directory path (relative to module root)
     * @param recursive Whether to delete contents recursively
     * @return true if deleted successfully, false otherwise
     */
    virtual bool deleteDirectory(const std::string& path, bool recursive = false) = 0;

    /**
     * @brief Transfer file to module
     * @param local_path Local file path
     * @param remote_path Remote file path (relative to module root)
     * @param overwrite Whether to overwrite existing file
     * @return Transfer result
     */
    virtual TransferResult transferFile(const std::string& local_path, const std::string& remote_path, bool overwrite = false) = 0;

    /**
     * @brief Transfer file from module
     * @param remote_path Remote file path (relative to module root)
     * @param local_path Local file path
     * @return Transfer result
     */
    virtual TransferResult receiveFile(const std::string& remote_path, const std::string& local_path) = 0;

    /**
     * @brief Get module statistics
     * @return Module statistics
     */
    virtual TransferStats getStats() const = 0;

    /**
     * @brief Reset module statistics
     */
    virtual void resetStats() = 0;

    /**
     * @brief Validate module configuration
     * @return true if valid, false otherwise
     */
    virtual bool validate() const = 0;

    /**
     * @brief Get module errors
     * @return Vector of error messages
     */
    virtual std::vector<std::string> getErrors() const = 0;

    /**
     * @brief Reload module configuration
     * @return true if reloaded successfully, false otherwise
     */
    virtual bool reload() = 0;

protected:
    // Module configuration
    ModuleConfig config_;

    // Module statistics
    mutable TransferStats stats_;
    mutable std::mutex stats_mutex_;

    // Helper methods
    std::string resolvePath(const std::string& path) const;
    bool validatePath(const std::string& path) const;
    bool isPathAllowed(const std::string& path) const;
    std::string sanitizePath(const std::string& path) const;

    // Script execution
    bool executeScript(const std::string& script_path, const std::map<std::string, std::string>& environment) const;
    std::map<std::string, std::string> buildEnvironment(const std::string& operation, const std::string& path) const;

    // File operations
    bool copyFile(const std::string& source, const std::string& destination) const;
    bool moveFile(const std::string& source, const std::string& destination) const;
    bool setFilePermissions(const std::string& path, const std::string& permissions) const;
    bool setFileOwnership(const std::string& path, const std::string& owner, const std::string& group) const;

    // Pattern matching
    bool matchesPattern(const std::string& filename, const std::string& pattern) const;
    bool matchesPatterns(const std::string& filename, const std::vector<std::string>& patterns) const;

    // Error handling
    void addError(const std::string& error) const;
    void clearErrors() const;

    // Statistics update
    void updateStats(const TransferStats& transfer_stats, bool success);

    // Path validation
    bool isPathTraversal(const std::string& path) const;
    bool isPathSafe(const std::string& path) const;
    std::string normalizePath(const std::string& path) const;

    // File system operations
    bool ensureDirectoryExists(const std::string& path) const;
    bool removeDirectoryContents(const std::string& path) const;
    bool copyDirectory(const std::string& source, const std::string& destination) const;
    bool moveDirectory(const std::string& source, const std::string& destination) const;

    // Checksum calculation
    std::string calculateChecksum(const std::string& file_path, const std::string& algorithm = "md5") const;
    bool verifyChecksum(const std::string& file_path, const std::string& expected_checksum, const std::string& algorithm = "md5") const;

    // Compression
    bool compressFile(const std::string& source, const std::string& destination, int level = 6) const;
    bool decompressFile(const std::string& source, const std::string& destination) const;

    // Logging
    void logOperation(const std::string& operation, const std::string& path, bool success, const std::string& details = "") const;

    // Security
    bool checkFilePermissions(const std::string& path, const std::string& operation) const;
    bool checkReadPermission(const std::string& path) const;
    bool checkWritePermission(const std::string& path) const;
    bool checkDeletePermission(const std::string& path) const;
    bool validateFileType(const std::string& path) const;
    bool isExecutableFile(const std::string& path) const;
    bool isSymbolicLink(const std::string& path) const;

    // Performance
    size_t getOptimalBufferSize(const std::string& file_path) const;
    bool useDirectIO(const std::string& file_path) const;
    bool useAsyncIO(const std::string& file_path) const;

    // Error recovery
    bool retryOperation(const std::function<bool()>& operation, int max_retries = 3, int delay_ms = 100) const;
    bool cleanupOnFailure(const std::string& path) const;

    // Monitoring
    void updateMetrics(const std::string& operation, const std::string& path, size_t bytes, bool success) const;
    std::string getMetricsJSON() const;

    // Configuration
    bool loadConfiguration() const;
    bool saveConfiguration() const;
    bool validateConfiguration() const;

    // Module state
    mutable std::vector<std::string> errors_;
    mutable bool is_valid_;
    mutable std::chrono::steady_clock::time_point last_activity_;

    // Module locking
    mutable std::mutex module_mutex_;
    bool lockModule() const;
    void unlockModule() const;

    // Resource management
    bool checkResourceLimits() const;
    bool updateResourceUsage(size_t bytes, size_t files) const;
    void cleanupResources() const;

    // Health checking
    bool checkModuleHealth() const;
    std::string getModuleStatus() const;

    // Backup and restore
    bool createBackup(const std::string& path) const;
    bool restoreFromBackup(const std::string& path) const;
    bool cleanupBackups() const;

    // Versioning
    bool createVersion(const std::string& path, const std::string& version) const;
    bool listVersions(const std::string& path) const;
    bool restoreVersion(const std::string& path, const std::string& version) const;

    // Synchronization
    bool lockFile(const std::string& path) const;
    bool unlockFile(const std::string& path) const;
    bool isFileLocked(const std::string& path) const;

    // Notifications
    void notifyFileChange(const std::string& path, const std::string& operation) const;
    void notifyDirectoryChange(const std::string& path, const std::string& operation) const;

    // Audit trail
    void auditOperation(const std::string& operation, const std::string& path, const std::string& user, bool success) const;
    std::string getAuditLog() const;

    // Maintenance
    bool performMaintenance() const;
    bool cleanupTempFiles() const;
    bool optimizeStorage() const;
    bool checkIntegrity() const;

    // Reporting
    std::string generateReport() const;
    std::string exportData(const std::string& format) const;
    bool importData(const std::string& data, const std::string& format) const;
};

/**
 * @brief Factory function to create a module instance
 * @param config Module configuration
 * @return Shared pointer to module instance
 */
std::shared_ptr<Module> createModule(const ModuleConfig& config);

} // namespace simple_rsyncd
