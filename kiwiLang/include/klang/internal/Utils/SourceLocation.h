#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <ostream>
#include <functional>
#include <memory>

namespace klang {

class SourceManager;

class SourceLocation {
public:
    SourceLocation();
    SourceLocation(const SourceManager* manager, uint32_t file_id, uint32_t offset);
    SourceLocation(const SourceManager* manager, uint32_t file_id, uint32_t line, uint32_t column);
    SourceLocation(const SourceManager* manager, const std::string& filename, uint32_t offset);
    SourceLocation(const SourceManager* manager, const std::string& filename, uint32_t line, uint32_t column);
    
    bool is_valid() const;
    bool is_invalid() const;
    
    const SourceManager* manager() const;
    uint32_t file_id() const;
    uint32_t offset() const;
    uint32_t line() const;
    uint32_t column() const;
    
    std::string filename() const;
    std::string file_path() const;
    std::string directory() const;
    
    SourceLocation get_begin() const;
    SourceLocation get_end() const;
    
    SourceLocation get_line_begin() const;
    SourceLocation get_line_end() const;
    
    SourceLocation advance(size_t chars) const;
    SourceLocation retreat(size_t chars) const;
    
    SourceLocation next_line() const;
    SourceLocation prev_line() const;
    
    bool same_file(const SourceLocation& other) const;
    bool same_line(const SourceLocation& other) const;
    
    bool operator==(const SourceLocation& other) const;
    bool operator!=(const SourceLocation& other) const;
    bool operator<(const SourceLocation& other) const;
    bool operator<=(const SourceLocation& other) const;
    bool operator>(const SourceLocation& other) const;
    bool operator>=(const SourceLocation& other) const;
    
    std::string to_string() const;
    std::string to_short_string() const;
    std::string to_user_string() const;
    
    size_t hash() const;
    
    struct Hasher {
        size_t operator()(const SourceLocation& loc) const;
    };
    
private:
    const SourceManager* manager_ = nullptr;
    uint32_t file_id_ = 0;
    uint32_t offset_ = 0;
    uint32_t line_ = 0;
    uint32_t column_ = 0;
    mutable bool computed_line_col_ = false;
    
    void compute_line_col() const;
};

class SourceRange {
public:
    SourceRange();
    SourceRange(const SourceLocation& begin, const SourceLocation& end);
    SourceRange(const SourceManager* manager, uint32_t file_id, uint32_t begin_offset, uint32_t end_offset);
    SourceRange(const SourceManager* manager, const std::string& filename, uint32_t begin_offset, uint32_t end_offset);
    
    bool is_valid() const;
    bool is_invalid() const;
    bool empty() const;
    
    const SourceLocation& begin() const;
    const SourceLocation& end() const;
    
    void set_begin(const SourceLocation& begin);
    void set_end(const SourceLocation& end);
    
    SourceLocation get_center() const;
    
    bool contains(const SourceLocation& loc) const;
    bool contains(const SourceRange& range) const;
    bool overlaps(const SourceRange& range) const;
    bool adjacent(const SourceRange& range) const;
    
    SourceRange union_with(const SourceRange& other) const;
    SourceRange intersection_with(const SourceRange& other) const;
    
    SourceRange expand_to_include(const SourceLocation& loc) const;
    SourceRange expand_lines(uint32_t before, uint32_t after) const;
    SourceRange expand_chars(uint32_t before, uint32_t after) const;
    
    uint32_t length() const;
    uint32_t line_count() const;
    
    bool same_file(const SourceRange& other) const;
    
    bool operator==(const SourceRange& other) const;
    bool operator!=(const SourceRange& other) const;
    
    std::string to_string() const;
    std::string to_short_string() const;
    std::string to_user_string() const;
    
    size_t hash() const;
    
    struct Hasher {
        size_t operator()(const SourceRange& range) const;
    };
    
private:
    SourceLocation begin_;
    SourceLocation end_;
};

class SourceManager {
public:
    SourceManager();
    ~SourceManager();
    
    SourceManager(const SourceManager&) = delete;
    SourceManager& operator=(const SourceManager&) = delete;
    
    uint32_t create_file_id(const std::string& filename, const std::string& content);
    uint32_t get_file_id(const std::string& filename) const;
    bool has_file(const std::string& filename) const;
    bool has_file(uint32_t file_id) const;
    
    const std::string& get_filename(uint32_t file_id) const;
    const std::string& get_file_content(uint32_t file_id) const;
    std::string_view get_file_view(uint32_t file_id) const;
    
    size_t get_file_size(uint32_t file_id) const;
    size_t get_line_count(uint32_t file_id) const;
    
    SourceLocation get_location(uint32_t file_id, uint32_t offset) const;
    SourceLocation get_location(uint32_t file_id, uint32_t line, uint32_t column) const;
    SourceLocation get_location(const std::string& filename, uint32_t offset) const;
    SourceLocation get_location(const std::string& filename, uint32_t line, uint32_t column) const;
    
    std::string get_line(uint32_t file_id, uint32_t line) const;
    std::string get_line(const SourceLocation& loc) const;
    
    SourceRange get_line_range(uint32_t file_id, uint32_t line) const;
    SourceRange get_line_range(const SourceLocation& loc) const;
    
    void compute_line_col(uint32_t file_id, uint32_t offset, uint32_t& line, uint32_t& column) const;
    uint32_t compute_offset(uint32_t file_id, uint32_t line, uint32_t column) const;
    
    SourceLocation get_begin(uint32_t file_id) const;
    SourceLocation get_end(uint32_t file_id) const;
    
    bool is_valid_location(const SourceLocation& loc) const;
    bool is_valid_range(const SourceRange& range) const;
    
    char get_char(const SourceLocation& loc) const;
    std::string_view get_text(const SourceRange& range) const;
    std::string_view get_text(const SourceLocation& begin, const SourceLocation& end) const;
    
    SourceLocation find_char(uint32_t file_id, char c, uint32_t start_offset = 0) const;
    SourceLocation find_string(uint32_t file_id, const std::string& str, uint32_t start_offset = 0) const;
    SourceLocation find_pattern(uint32_t file_id, const std::string& pattern, uint32_t start_offset = 0) const;
    
    SourceLocation find_next_line(uint32_t file_id, uint32_t offset) const;
    SourceLocation find_prev_line(uint32_t file_id, uint32_t offset) const;
    
    uint32_t get_line_start_offset(uint32_t file_id, uint32_t line) const;
    uint32_t get_line_end_offset(uint32_t file_id, uint32_t line) const;
    
    void build_line_table(uint32_t file_id);
    bool has_line_table(uint32_t file_id) const;
    
    size_t file_count() const;
    size_t total_size() const;
    
    void clear();
    void remove_file(uint32_t file_id);
    void remove_file(const std::string& filename);
    
    struct FileInfo {
        uint32_t id;
        std::string filename;
        std::string content;
        std::vector<uint32_t> line_offsets;
        size_t size;
        uint32_t line_count;
        
        FileInfo(uint32_t id, std::string name, std::string data);
    };
    
    const FileInfo* get_file_info(uint32_t file_id) const;
    const FileInfo* get_file_info(const std::string& filename) const;
    
    std::vector<uint32_t> get_file_ids() const;
    std::vector<std::string> get_filenames() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

inline bool SourceLocation::is_valid() const {
    return manager_ != nullptr && file_id_ != 0;
}

inline bool SourceLocation::is_invalid() const {
    return !is_valid();
}

inline const SourceManager* SourceLocation::manager() const {
    return manager_;
}

inline uint32_t SourceLocation::file_id() const {
    return file_id_;
}

inline uint32_t SourceLocation::offset() const {
    return offset_;
}

inline bool SourceLocation::operator==(const SourceLocation& other) const {
    return manager_ == other.manager_ && file_id_ == other.file_id_ && offset_ == other.offset_;
}

inline bool SourceLocation::operator!=(const SourceLocation& other) const {
    return !(*this == other);
}

inline bool SourceLocation::operator<(const SourceLocation& other) const {
    if (manager_ != other.manager_) return manager_ < other.manager_;
    if (file_id_ != other.file_id_) return file_id_ < other.file_id_;
    return offset_ < other.offset_;
}

inline bool SourceLocation::operator<=(const SourceLocation& other) const {
    return !(other < *this);
}

inline bool SourceLocation::operator>(const SourceLocation& other) const {
    return other < *this;
}

inline bool SourceLocation::operator>=(const SourceLocation& other) const {
    return !(*this < other);
}

inline size_t SourceLocation::Hasher::operator()(const SourceLocation& loc) const {
    size_t seed = 0;
    seed ^= std::hash<const void*>{}(loc.manager_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint32_t>{}(loc.file_id_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint32_t>{}(loc.offset_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

inline const SourceLocation& SourceRange::begin() const {
    return begin_;
}

inline const SourceLocation& SourceRange::end() const {
    return end_;
}

inline void SourceRange::set_begin(const SourceLocation& begin) {
    begin_ = begin;
}

inline void SourceRange::set_end(const SourceLocation& end) {
    end_ = end;
}

inline bool SourceRange::is_valid() const {
    return begin_.is_valid() && end_.is_valid();
}

inline bool SourceRange::is_invalid() const {
    return !is_valid();
}

inline bool SourceRange::empty() const {
    return begin_ == end_;
}

inline bool SourceRange::operator==(const SourceRange& other) const {
    return begin_ == other.begin_ && end_ == other.end_;
}

inline bool SourceRange::operator!=(const SourceRange& other) const {
    return !(*this == other);
}

inline size_t SourceRange::Hasher::operator()(const SourceRange& range) const {
    size_t seed = 0;
    seed ^= SourceLocation::Hasher{}(range.begin_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= SourceLocation::Hasher{}(range.end_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

inline std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
    return os << loc.to_string();
}

inline std::ostream& operator<<(std::ostream& os, const SourceRange& range) {
    return os << range.to_string();
}

} // namespace klang