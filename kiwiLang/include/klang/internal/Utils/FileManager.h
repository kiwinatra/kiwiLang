#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include <system_error>
#include <filesystem>

namespace klang {

namespace fs = std::filesystem;

class FileManager {
public:
    FileManager();
    explicit FileManager(const fs::path& working_directory);
    ~FileManager();
    
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    
    FileManager(FileManager&& other) noexcept;
    FileManager& operator=(FileManager&& other) noexcept;
    
    // File operations
    std::string read_file(const fs::path& path) const;
    std::vector<uint8_t> read_file_binary(const fs::path& path) const;
    std::string read_file_partial(const fs::path& path, size_t offset, size_t size) const;
    
    bool write_file(const fs::path& path, const std::string& content);
    bool write_file(const fs::path& path, const std::vector<uint8_t>& content);
    bool write_file_binary(const fs::path& path, const void* data, size_t size);
    
    bool append_file(const fs::path& path, const std::string& content);
    bool append_file_binary(const fs::path& path, const void* data, size_t size);
    
    bool copy_file(const fs::path& from, const fs::path& to);
    bool move_file(const fs::path& from, const fs::path& to);
    bool rename_file(const fs::path& from, const fs::path& to);
    bool delete_file(const fs::path& path);
    
    // File information
    bool file_exists(const fs::path& path) const;
    bool is_file(const fs::path& path) const;
    bool is_directory(const fs::path& path) const;
    bool is_symlink(const fs::path& path) const;
    bool is_regular_file(const fs::path& path) const;
    
    size_t file_size(const fs::path& path) const;
    uint64_t file_size64(const fs::path& path) const;
    
    std::time_t last_write_time(const fs::path& path) const;
    std::time_t creation_time(const fs::path& path) const;
    std::time_t last_access_time(const fs::path& path) const;
    
    std::string file_extension(const fs::path& path) const;
    std::string file_stem(const fs::path& path) const;
    std::string file_name(const fs::path& path) const;
    fs::path file_directory(const fs::path& path) const;
    fs::path absolute_path(const fs::path& path) const;
    fs::path canonical_path(const fs::path& path) const;
    fs::path relative_path(const fs::path& path, const fs::path& base = fs::current_path()) const;
    
    // Directory operations
    bool create_directory(const fs::path& path);
    bool create_directories(const fs::path& path);
    bool delete_directory(const fs::path& path);
    bool delete_directory_recursive(const fs::path& path);
    
    std::vector<fs::path> list_directory(const fs::path& path) const;
    std::vector<fs::path> list_directory_recursive(const fs::path& path) const;
    std::vector<fs::path> find_files(const fs::path& dir, const std::string& pattern) const;
    std::vector<fs::path> find_files_recursive(const fs::path& dir, const std::string& pattern) const;
    
    // File watching
    using FileChangeCallback = std::function<void(const fs::path&, uint32_t)>;
    
    enum class ChangeType : uint32_t {
        Created = 1 << 0,
        Modified = 1 << 1,
        Deleted = 1 << 2,
        Renamed = 1 << 3,
        Attributes = 1 << 4,
        All = Created | Modified | Deleted | Renamed | Attributes
    };
    
    bool watch_directory(const fs::path& path, FileChangeCallback callback, ChangeType types = ChangeType::All);
    bool watch_file(const fs::path& path, FileChangeCallback callback, ChangeType types = ChangeType::All);
    void unwatch(const fs::path& path);
    void unwatch_all();
    
    // Memory mapped files
    class MemoryMappedFile {
    public:
        MemoryMappedFile();
        MemoryMappedFile(const fs::path& path, size_t offset = 0, size_t length = 0);
        ~MemoryMappedFile();
        
        MemoryMappedFile(const MemoryMappedFile&) = delete;
        MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
        
        MemoryMappedFile(MemoryMappedFile&& other) noexcept;
        MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;
        
        bool open(const fs::path& path, size_t offset = 0, size_t length = 0);
        void close();
        bool is_open() const;
        
        const uint8_t* data() const;
        uint8_t* data();
        size_t size() const;
        size_t offset() const;
        
        std::string_view string_view() const;
        std::string_view substr(size_t pos, size_t count = std::string_view::npos) const;
        
        uint8_t operator[](size_t index) const;
        uint8_t& operator[](size_t index);
        
        bool flush();
        bool sync();
        
    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
    
    MemoryMappedFile map_file(const fs::path& path, size_t offset = 0, size_t length = 0);
    
    // Temporary files
    fs::path create_temp_file(const std::string& prefix = "tmp", const std::string& suffix = "");
    fs::path create_temp_directory(const std::string& prefix = "tmp");
    
    void set_temp_directory(const fs::path& dir);
    fs::path temp_directory() const;
    
    // File locking
    class FileLock {
    public:
        FileLock();
        explicit FileLock(const fs::path& path, bool exclusive = true);
        ~FileLock();
        
        FileLock(const FileLock&) = delete;
        FileLock& operator=(const FileLock&) = delete;
        
        FileLock(FileLock&& other) noexcept;
        FileLock& operator=(FileLock&& other) noexcept;
        
        bool lock(const fs::path& path, bool exclusive = true);
        bool try_lock(const fs::path& path, bool exclusive = true);
        void unlock();
        
        bool is_locked() const;
        bool is_exclusive() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
    
    FileLock lock_file(const fs::path& path, bool exclusive = true);
    bool try_lock_file(const fs::path& path, bool exclusive = true);
    
    // Path manipulation
    fs::path join_path(const fs::path& a, const fs::path& b) const;
    fs::path join_paths(const std::vector<fs::path>& paths) const;
    
    fs::path normalize_path(const fs::path& path) const;
    fs::path make_relative(const fs::path& path, const fs::path& base = fs::current_path()) const;
    
    bool is_absolute_path(const fs::path& path) const;
    bool is_relative_path(const fs::path& path) const;
    
    fs::path resolve_symlink(const fs::path& path) const;
    
    // File system operations
    uintmax_t disk_space_total(const fs::path& path) const;
    uintmax_t disk_space_free(const fs::path& path) const;
    uintmax_t disk_space_available(const fs::path& path) const;
    
    bool create_symlink(const fs::path& target, const fs::path& link);
    bool create_hard_link(const fs::path& target, const fs::path& link);
    
    bool set_permissions(const fs::path& path, fs::perms permissions);
    fs::perms get_permissions(const fs::path& path) const;
    
    // Search paths
    void add_search_path(const fs::path& path);
    void remove_search_path(const fs::path& path);
    void clear_search_paths();
    
    std::vector<fs::path> search_paths() const;
    
    fs::path find_file(const fs::path& filename) const;
    std::vector<fs::path> find_all_files(const fs::path& filename) const;
    
    // Working directory
    void set_working_directory(const fs::path& path);
    fs::path working_directory() const;
    fs::path home_directory() const;
    fs::path config_directory() const;
    fs::path cache_directory() const;
    fs::path data_directory() const;
    
    // Error handling
    std::error_code last_error() const;
    void clear_error();
    void set_error_handler(std::function<void(const std::error_code&)> handler);
    
    // Statistics
    struct Stats {
        size_t files_read = 0;
        size_t files_written = 0;
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        size_t directories_created = 0;
        size_t directories_deleted = 0;
        size_t files_deleted = 0;
        size_t symlinks_created = 0;
        size_t file_locks = 0;
        size_t memory_mapped_files = 0;
        size_t watch_events = 0;
        
        double average_read_speed_mbps = 0.0;
        double average_write_speed_mbps = 0.0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t open_file_handles = 0;
    };
    
    Stats stats() const;
    void reset_stats();
    
    // Caching
    void enable_caching(bool enable);
    bool caching_enabled() const;
    
    void set_cache_size(size_t max_size_mb);
    size_t cache_size() const;
    
    void clear_cache();
    void precache_file(const fs::path& path);
    
    // Virtual file system
    bool mount_virtual(const fs::path& mount_point, std::function<std::string(const fs::path&)> reader);
    bool unmount_virtual(const fs::path& mount_point);
    
    // Compression
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data, int level = 6);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed);
    
    bool compress_file(const fs::path& source, const fs::path& dest, int level = 6);
    bool decompress_file(const fs::path& source, const fs::path& dest);
    
    // Hashing
    std::string file_hash(const fs::path& path, const std::string& algorithm = "sha256");
    std::string data_hash(const std::vector<uint8_t>& data, const std::string& algorithm = "sha256");
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

inline bool FileManager::file_exists(const fs::path& path) const {
    return fs::exists(path);
}

inline bool FileManager::is_file(const fs::path& path) const {
    return fs::is_regular_file(path);
}

inline bool FileManager::is_directory(const fs::path& path) const {
    return fs::is_directory(path);
}

inline bool FileManager::is_symlink(const fs::path& path) const {
    return fs::is_symlink(path);
}

inline bool FileManager::is_regular_file(const fs::path& path) const {
    return fs::is_regular_file(path);
}

inline std::string FileManager::file_extension(const fs::path& path) const {
    return path.extension().string();
}

inline std::string FileManager::file_stem(const fs::path& path) const {
    return path.stem().string();
}

inline std::string FileManager::file_name(const fs::path& path) const {
    return path.filename().string();
}

inline fs::path FileManager::file_directory(const fs::path& path) const {
    return path.parent_path();
}

inline fs::path FileManager::absolute_path(const fs::path& path) const {
    return fs::absolute(path);
}

inline fs::path FileManager::normalize_path(const fs::path& path) const {
    return path.lexically_normal();
}

inline bool FileManager::is_absolute_path(const fs::path& path) const {
    return path.is_absolute();
}

inline bool FileManager::is_relative_path(const fs::path& path) const {
    return path.is_relative();
}

inline fs::path FileManager::join_path(const fs::path& a, const fs::path& b) const {
    return a / b;
}

inline uint8_t FileManager::MemoryMappedFile::operator[](size_t index) const {
    return data()[index];
}

inline uint8_t& FileManager::MemoryMappedFile::operator[](size_t index) {
    return data()[index];
}

inline std::string_view FileManager::MemoryMappedFile::string_view() const {
    return std::string_view(reinterpret_cast<const char*>(data()), size());
}

inline std::string_view FileManager::MemoryMappedFile::substr(size_t pos, size_t count) const {
    if (pos >= size()) return {};
    if (count == std::string_view::npos || pos + count > size()) {
        count = size() - pos;
    }
    return std::string_view(reinterpret_cast<const char*>(data() + pos), count);
}

inline ChangeType operator|(ChangeType a, ChangeType b) {
    return static_cast<ChangeType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ChangeType operator&(ChangeType a, ChangeType b) {
    return static_cast<ChangeType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace klang