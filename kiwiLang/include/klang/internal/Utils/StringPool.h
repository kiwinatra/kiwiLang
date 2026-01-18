#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace kiwiLang {

class StringPool {
public:
    StringPool();
    explicit StringPool(size_t initial_size);
    ~StringPool();

    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;

    StringPool(StringPool&& other) noexcept;
    StringPool& operator=(StringPool&& other) noexcept;

    const char* intern(const char* str);
    const char* intern(const char* str, size_t length);
    const char* intern(std::string_view str);
    const char* intern(const std::string& str);

    const char* intern_no_copy(const char* str);
    const char* intern_no_copy(const char* str, size_t length);
    
    bool contains(const char* str) const;
    bool contains(const char* str, size_t length) const;
    bool contains(std::string_view str) const;
    bool contains(const std::string& str) const;

    size_t size() const;
    size_t capacity() const;
    size_t memory_usage() const;
    size_t bucket_count() const;
    double load_factor() const;
    double max_load_factor() const;

    void clear();
    void shrink_to_fit();
    void reserve(size_t count);

    void set_max_size(size_t max_size);
    size_t max_size() const;

    bool empty() const;

    struct Stats {
        size_t total_strings = 0;
        size_t unique_strings = 0;
        size_t total_length = 0;
        size_t allocated_memory = 0;
        size_t wasted_memory = 0;
        size_t collisions = 0;
        size_t rehashes = 0;
        size_t allocations = 0;
        size_t deallocations = 0;
        double average_length = 0.0;
        double duplication_ratio = 0.0;
        double memory_efficiency = 0.0;
        double hash_efficiency = 0.0;
    };

    Stats stats() const;
    void reset_stats();

    void set_case_sensitive(bool sensitive);
    bool case_sensitive() const;

    void set_thread_safe(bool thread_safe);
    bool thread_safe() const;

    void lock();
    void unlock();
    bool try_lock();

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const char*;
        using difference_type = std::ptrdiff_t;
        using pointer = const char**;
        using reference = const char*&;

        Iterator();
        explicit Iterator(const StringPool* pool, size_t bucket);
        
        const char* operator*() const;
        const char* operator->() const;
        
        Iterator& operator++();
        Iterator operator++(int);
        
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        const StringPool* pool_;
        size_t bucket_;
        const char* current_;
        
        void advance_to_next();
    };

    Iterator begin() const;
    Iterator end() const;

    class ConstIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const char*;
        using difference_type = std::ptrdiff_t;
        using pointer = const char**;
        using reference = const char*&;

        ConstIterator();
        explicit ConstIterator(const StringPool* pool, size_t bucket);
        
        const char* operator*() const;
        const char* operator->() const;
        
        ConstIterator& operator++();
        ConstIterator operator++(int);
        
        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;

    private:
        const StringPool* pool_;
        size_t bucket_;
        const char* current_;
        
        void advance_to_next();
    };

    ConstIterator cbegin() const;
    ConstIterator cend() const;

    struct Hash {
        size_t operator()(const char* str) const;
        size_t operator()(const char* str, size_t length) const;
        size_t operator()(std::string_view str) const;
    };

    struct Equal {
        bool operator()(const char* a, const char* b) const;
        bool operator()(const char* a, const char* b, size_t length) const;
        bool operator()(const char* a, std::string_view b) const;
    };

private:
    struct StringEntry {
        const char* data;
        size_t length;
        size_t hash;
        uint32_t ref_count;
        uint32_t bucket_index;
        
        StringEntry* next;
        StringEntry* prev;
        
        StringEntry(const char* d, size_t len, size_t h)
            : data(d)
            , length(len)
            , hash(h)
            , ref_count(1)
            , bucket_index(0)
            , next(nullptr)
            , prev(nullptr) {}
    };
    
    struct Bucket {
        StringEntry* head;
        StringEntry* tail;
        size_t count;
        
        Bucket() : head(nullptr), tail(nullptr), count(0) {}
    };
    
    class Allocator {
    public:
        Allocator();
        explicit Allocator(size_t block_size);
        ~Allocator();
        
        char* allocate(size_t size);
        void deallocate(char* ptr, size_t size);
        void clear();
        void reset();
        
        size_t allocated() const;
        size_t used() const;
        size_t wasted() const;
        size_t block_count() const;
        
    private:
        struct Block {
            char* memory;
            size_t size;
            size_t used;
            Block* next;
            
            Block(size_t block_size);
            ~Block();
            
            bool can_allocate(size_t size) const;
            char* allocate(size_t size);
        };
        
        size_t block_size_;
        Block* first_;
        Block* current_;
        size_t total_allocated_;
        size_t total_used_;
        size_t total_wasted_;
        
        Block* find_block_with_space(size_t size);
        Block* create_block(size_t size);
    };
    
    Bucket* buckets_;
    size_t bucket_count_;
    size_t element_count_;
    size_t max_elements_;
    size_t rehash_threshold_;
    
    Allocator allocator_;
    
    bool case_sensitive_;
    bool thread_safe_;
    mutable std::mutex mutex_;
    
    Stats stats_;
    
    size_t hash_string(const char* str, size_t length) const;
    size_t hash_string_case_insensitive(const char* str, size_t length) const;
    
    bool compare_strings(const char* a, const char* b, size_t length) const;
    bool compare_strings_case_insensitive(const char* a, const char* b, size_t length) const;
    
    StringEntry* find_entry(const char* str, size_t length, size_t hash) const;
    StringEntry* create_entry(const char* str, size_t length, size_t hash);
    void insert_entry(StringEntry* entry);
    void remove_entry(StringEntry* entry);
    void rehash();
    void cleanup();
    
    char* copy_string(const char* str, size_t length);
    char* copy_string_case_insensitive(const char* str, size_t length);
    
    void update_stats_after_insert(size_t length);
    void update_stats_after_remove(size_t length);
    void update_stats_after_rehash();
    
    size_t calculate_bucket(size_t hash) const;
    size_t next_power_of_two(size_t n) const;
    bool needs_rehash() const;
    
    void lock_guard() const;
    void unlock_guard() const;
};

inline const char* StringPool::intern(const char* str) {
    if (!str) return nullptr;
    return intern(str, std::char_traits<char>::length(str));
}

inline const char* StringPool::intern(const std::string& str) {
    return intern(str.data(), str.size());
}

inline const char* StringPool::intern(std::string_view str) {
    return intern(str.data(), str.size());
}

inline const char* StringPool::intern_no_copy(const char* str) {
    if (!str) return nullptr;
    return intern_no_copy(str, std::char_traits<char>::length(str));
}

inline bool StringPool::contains(const char* str) const {
    if (!str) return false;
    return contains(str, std::char_traits<char>::length(str));
}

inline bool StringPool::contains(const std::string& str) const {
    return contains(str.data(), str.size());
}

inline bool StringPool::contains(std::string_view str) const {
    return contains(str.data(), str.size());
}

inline size_t StringPool::size() const {
    lock_guard();
    size_t result = element_count_;
    unlock_guard();
    return result;
}

inline size_t StringPool::capacity() const {
    lock_guard();
    size_t result = bucket_count_;
    unlock_guard();
    return result;
}

inline bool StringPool::empty() const {
    lock_guard();
    bool result = element_count_ == 0;
    unlock_guard();
    return result;
}

inline StringPool::Iterator StringPool::begin() const {
    return Iterator(this, 0);
}

inline StringPool::Iterator StringPool::end() const {
    return Iterator(this, bucket_count_);
}

inline StringPool::ConstIterator StringPool::cbegin() const {
    return ConstIterator(this, 0);
}

inline StringPool::ConstIterator StringPool::cend() const {
    return ConstIterator(this, bucket_count_);
}

inline size_t StringPool::Hash::operator()(const char* str) const {
    if (!str) return 0;
    size_t length = std::char_traits<char>::length(str);
    return operator()(str, length);
}

inline size_t StringPool::Hash::operator()(std::string_view str) const {
    return operator()(str.data(), str.size());
}

inline bool StringPool::Equal::operator()(const char* a, const char* b) const {
    if (a == b) return true;
    if (!a || !b) return false;
    return std::char_traits<char>::compare(a, b, std::char_traits<char>::length(a)) == 0;
}

inline bool StringPool::Equal::operator()(const char* a, std::string_view b) const {
    return operator()(a, b.data(), b.size());
}

inline const char* StringPool::Iterator::operator*() const {
    return current_;
}

inline const char* StringPool::Iterator::operator->() const {
    return current_;
}

inline bool StringPool::Iterator::operator==(const Iterator& other) const {
    return pool_ == other.pool_ && bucket_ == other.bucket_ && current_ == other.current_;
}

inline bool StringPool::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

inline const char* StringPool::ConstIterator::operator*() const {
    return current_;
}

inline const char* StringPool::ConstIterator::operator->() const {
    return current_;
}

inline bool StringPool::ConstIterator::operator==(const ConstIterator& other) const {
    return pool_ == other.pool_ && bucket_ == other.bucket_ && current_ == other.current_;
}

inline bool StringPool::ConstIterator::operator!=(const ConstIterator& other) const {
    return !(*this == other);
}

} // namespace kiwiLang