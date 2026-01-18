include/klang/internal/Runtime/HashTable.h
#pragma once
#include "Object.h"
#include "Value.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace klang {
namespace internal {
namespace runtime {

class VM;

class HashTable {
public:
    static constexpr double MAX_LOAD = 0.75;
    static constexpr std::size_t INITIAL_CAPACITY = 8;
    
    explicit HashTable(VM& vm);
    ~HashTable();
    
    bool set(Value key, Value value);
    bool get(const Value& key, Value& value) const;
    bool remove(const Value& key);
    bool contains(const Value& key) const;
    
    void clear();
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return capacity_; }
    
    void visit(GCVisitor& visitor);
    
    class Iterator {
    public:
        Iterator(const HashTable* table, std::size_t index);
        
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        
        Iterator& operator++();
        std::pair<Value, Value> operator*() const;
        
    private:
        const HashTable* table_;
        std::size_t index_;
        void advanceToNextEntry();
    };
    
    Iterator begin() const;
    Iterator end() const;
    
private:
    struct Entry {
        Value key;
        Value value;
        bool occupied;
        bool tombstone;
        
        Entry() : occupied(false), tombstone(false) {}
    };
    
    VM& vm_;
    Entry* entries_;
    std::size_t capacity_;
    std::size_t count_;
    std::size_t tombstones_;
    
    std::size_t findEntry(const Value& key) const;
    void adjustCapacity(std::size_t newCapacity);
    
    static std::uint32_t hashValue(const Value& value);
    static bool valuesEqual(const Value& a, const Value& b);
};

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class ConcurrentHashTable {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    
    explicit ConcurrentHashTable(std::size_t initialCapacity = 16);
    
    bool insert(const Key& key, const Value& value);
    bool find(const Key& key, Value& value) const;
    bool erase(const Key& key);
    void clear();
    
    std::size_t size() const;
    std::size_t capacity() const;
    
private:
    struct Bucket {
        mutable std::mutex mutex;
        std::vector<value_type> entries;
    };
    
    std::vector<std::unique_ptr<Bucket>> buckets_;
    std::atomic<std::size_t> size_;
    Hash hash_;
    Equal equal_;
    
    Bucket& getBucket(const Key& key) const;
    std::size_t bucketIndex(const Key& key) const;
    
    static bool isPrime(std::size_t n);
    static std::size_t nextPrime(std::size_t n);
};

template<typename T>
class IdentityHashTable {
public:
    static constexpr std::size_t INITIAL_SIZE = 1024;
    
    IdentityHashTable();
    
    T* get(HeapObject* object) const;
    void set(HeapObject* object, T* value);
    void remove(HeapObject* object);
    void clear();
    
    std::size_t size() const { return count_; }
    
    void visit(GCVisitor& visitor);
    
private:
    struct Slot {
        HeapObject* key;
        T* value;
        std::uint32_t hash;
        bool occupied;
        
        Slot() : key(nullptr), value(nullptr), hash(0), occupied(false) {}
    };
    
    Slot* slots_;
    std::size_t capacity_;
    std::size_t count_;
    std::size_t mask_;
    
    void rehash();
    std::size_t findSlot(HeapObject* object, std::uint32_t hash) const;
    
    static std::uint32_t identityHash(HeapObject* object);
};

class StringTable {
public:
    explicit StringTable(VM& vm);
    
    String* intern(std::string_view str);
    String* get(std::string_view str) const;
    
    void sweep();
    void clear();
    
    std::size_t size() const { return table_.size(); }
    std::size_t capacity() const { return table_.capacity(); }
    
private:
    VM& vm_;
    HashTable table_;
    
    static Value stringToKey(String* str);
    static String* keyToString(const Value& key);
};

} // namespace runtime
} // namespace internal
} // namespace klang