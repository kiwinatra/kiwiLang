include/kiwiLang/internal/Runtime/Memory.h
#pragma once
#include "Object.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <type_traits>

namespace kiwiLang {
namespace internal {
namespace runtime {

class VM;

class MemoryArena {
public:
    explicit MemoryArena(std::size_t chunkSize = 64 * 1024);
    ~MemoryArena();
    
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    
    void* allocate(std::size_t size, std::size_t alignment);
    void deallocateAll();
    
    std::size_t allocatedBytes() const { return allocated_; }
    std::size_t totalCapacity() const { return chunks_.size() * chunkSize_; }

private:
    struct Chunk {
        std::unique_ptr<std::byte[]> data;
        std::byte* current;
        std::byte* end;
        
        Chunk(std::size_t size);
    };
    
    std::size_t chunkSize_;
    std::vector<Chunk> chunks_;
    std::size_t allocated_;
};

template<typename T>
class FreeListAllocator {
public:
    using value_type = T;
    
    FreeListAllocator() = default;
    
    template<typename U>
    FreeListAllocator(const FreeListAllocator<U>&) noexcept {}
    
    T* allocate(std::size_t n);
    void deallocate(T* p, std::size_t n) noexcept;
    
    bool operator==(const FreeListAllocator&) const { return true; }
    bool operator!=(const FreeListAllocator&) const { return false; }

private:
    union FreeNode {
        FreeNode* next;
        alignas(T) char data[sizeof(T)];
    };
    
    FreeNode* freeList_ = nullptr;
};

class GCPage {
public:
    static constexpr std::size_t PAGE_SIZE = 64 * 1024;
    static constexpr std::size_t OBJECT_SIZE_LIMIT = PAGE_SIZE / 4;
    
    GCPage(std::size_t objectSize);
    ~GCPage();
    
    bool contains(const void* ptr) const;
    void* allocate();
    void sweep(std::function<void(HeapObject*)> finalizer);
    
    std::size_t allocatedObjects() const { return allocated_; }
    std::size_t freeObjects() const { return free_; }
    
private:
    struct ObjectSlot {
        HeapObject* object;
        bool marked;
    };
    
    std::unique_ptr<std::byte[]> memory_;
    std::vector<ObjectSlot> slots_;
    std::size_t objectSize_;
    std::size_t allocated_;
    std::size_t free_;
    std::size_t nextFree_;
};

class GCHeap {
public:
    GCHeap();
    ~GCHeap();
    
    void* allocate(std::size_t size);
    void collectGarbage(VM& vm);
    
    std::size_t totalAllocated() const { return totalAllocated_; }
    std::size_t totalFreed() const { return totalFreed_; }
    
    void addRoot(HeapObject* object);
    void removeRoot(HeapObject* object);
    
    void registerFinalizer(HeapObject* object, std::function<void()> finalizer);

private:
    struct LargeAllocation {
        std::unique_ptr<std::byte[]> memory;
        HeapObject* object;
        std::size_t size;
        std::function<void()> finalizer;
    };
    
    std::vector<GCPage> smallPages_;
    std::vector<LargeAllocation> largeAllocations_;
    std::vector<HeapObject*> roots_;
    std::size_t totalAllocated_;
    std::size_t totalFreed_;
    std::size_t nextGC_;
    
    static constexpr std::size_t GC_THRESHOLD = 16 * 1024 * 1024;
    static constexpr std::size_t GROWTH_FACTOR = 2;
    
    void mark(VM& vm);
    void sweep();
    void markObject(HeapObject* object);
};

template<typename T, typename... Args>
T* constructInPlace(void* memory, Args&&... args) {
    static_assert(std::is_base_of_v<HeapObject, T>, "T must inherit from HeapObject");
    static_assert(alignof(T) <= alignof(std::max_align_t), "Alignment requirement too high");
    
    return new (memory) T(std::forward<Args>(args)...);
}

template<typename T>
void destroyInPlace(T* object) {
    static_assert(std::is_base_of_v<HeapObject, T>, "T must inherit from HeapObject");
    object->~T();
}

class AllocationTracker {
public:
    struct Stats {
        std::size_t totalAllocations;
        std::size_t totalBytes;
        std::size_t peakBytes;
        std::size_t liveObjects;
        std::size_t liveBytes;
    };
    
    static AllocationTracker& instance();
    
    void recordAllocation(std::size_t size, const char* typeName);
    void recordDeallocation(std::size_t size, const char* typeName);
    
    Stats getStats() const;
    void resetStats();

private:
    AllocationTracker() = default;
    
    mutable std::mutex mutex_;
    Stats stats_;
    std::unordered_map<const char*, std::size_t> typeCounts_;
};

} // namespace runtime
} // namespace internal
} // namespace kiwiLang