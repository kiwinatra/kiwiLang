include/klang/internal/Runtime/GarbageCollector.h
#pragma once
#include "Object.h"
#include "Memory.h"
#include "HashTable.h"
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

namespace klang {
namespace internal {
namespace runtime {

class VM;
class HeapObject;
class GCVisitor;

enum class GCPhase {
    IDLE,
    MARK,
    SWEEP,
    COMPACT,
    FINALIZE
};

enum class GCStrategy {
    GENERATIONAL,
    INCREMENTAL,
    CONCURRENT,
    REAL_TIME
};

struct GCConfig {
    GCStrategy strategy = GCStrategy::GENERATIONAL;
    std::size_t heapSize = 64 * 1024 * 1024; // 64 MB
    std::size_t nurserySize = 16 * 1024 * 1024; // 16 MB
    double growthFactor = 1.5;
    std::size_t incrementalStep = 1024 * 1024; // 1 MB
    bool enableCompaction = true;
    bool enableFinalization = true;
    std::size_t maxPauseMs = 10;
};

class GCWorklist {
public:
    GCWorklist();
    ~GCWorklist();
    
    void push(HeapObject* object);
    HeapObject* pop();
    bool empty() const;
    void clear();
    
    std::size_t size() const;

private:
    struct Chunk {
        static constexpr std::size_t SIZE = 4096;
        HeapObject* objects[SIZE];
        std::size_t count;
        Chunk* next;
        
        Chunk();
    };
    
    Chunk* head_;
    Chunk* tail_;
    std::atomic<std::size_t> size_;
    std::mutex mutex_;
};

class GCBarrier {
public:
    virtual ~GCBarrier() = default;
    
    virtual void writeBarrier(HeapObject* obj, HeapObject* field) = 0;
    virtual void readBarrier(HeapObject* obj) = 0;
    virtual void onPointerStore(HeapObject* dest, std::size_t offset, HeapObject* value) = 0;
    
    static GCBarrier* create(GCStrategy strategy, VM& vm);
};

class GenerationalGCBarrier final : public GCBarrier {
public:
    explicit GenerationalGCBarrier(VM& vm);
    
    void writeBarrier(HeapObject* obj, HeapObject* field) override;
    void readBarrier(HeapObject* obj) override;
    void onPointerStore(HeapObject* dest, std::size_t offset, HeapObject* value) override;

private:
    VM& vm_;
    
    void remember(HeapObject* oldObj, HeapObject* newObj);
};

class GarbageCollector {
public:
    explicit GarbageCollector(VM& vm, const GCConfig& config = GCConfig{});
    ~GarbageCollector();
    
    void* allocate(std::size_t size, ObjectType type);
    template<typename T, typename... Args>
    T* allocate(Args&&... args);
    
    void collect();
    void collectIncremental();
    void collectConcurrent();
    
    void startGC();
    void finishGC();
    bool isCollecting() const;
    
    void addRoot(HeapObject* object);
    void removeRoot(HeapObject* object);
    
    void addWeakRef(HeapObject* object, std::function<void()> callback);
    void addFinalizer(HeapObject* object, std::function<void()> finalizer);
    
    std::size_t allocatedBytes() const;
    std::size_t totalBytes() const;
    std::size_t collections() const;
    
    void visitRoots(GCVisitor& visitor);
    void visitWeakRefs(GCVisitor& visitor);
    
    GCPhase currentPhase() const { return phase_; }
    GCConfig config() const { return config_; }

private:
    struct Generation {
        MemoryArena arena;
        std::vector<HeapObject*> rememberedSet;
        std::size_t threshold;
        std::size_t allocated;
        
        explicit Generation(std::size_t size);
    };
    
    VM& vm_;
    GCConfig config_;
    std::atomic<GCPhase> phase_;
    std::atomic<bool> collecting_;
    
    Generation nursery_;
    Generation oldSpace_;
    Generation largeObjectSpace_;
    
    std::vector<HeapObject*> roots_;
    std::vector<std::pair<HeapObject*, std::function<void()>>> finalizers_;
    std::vector<std::pair<HeapObject*, std::function<void()>>> weakRefs_;
    
    GCWorklist worklist_;
    std::unique_ptr<GCBarrier> barrier_;
    
    std::size_t totalCollections_;
    std::size_t totalBytesAllocated_;
    std::size_t bytesSinceLastGC_;
    
    std::mutex allocationMutex_;
    std::mutex collectionMutex_;
    
    void markPhase();
    void sweepPhase();
    void compactPhase();
    void finalizePhase();
    
    void markRoots();
    void markFromWorklist();
    
    void promote(HeapObject* object);
    void evacuate(Generation& from, Generation& to);
    
    bool shouldCollect() const;
    void updateThresholds();
    
    void processWeakRefs();
    void processFinalizers();
    
    void* allocateInGeneration(Generation& gen, std::size_t size, ObjectType type);
    
    static constexpr double COLLECTION_THRESHOLD = 0.85;
    static constexpr std::size_t MIN_PROMOTION_AGE = 2;
};

class GCVisitorImpl final : public GCVisitor {
public:
    explicit GCVisitorImpl(GarbageCollector& gc);
    
    void visit(HeapObject* object) override;
    
    void setWorklist(GCWorklist* worklist) { worklist_ = worklist; }

private:
    GarbageCollector& gc_;
    GCWorklist* worklist_;
};

template<typename T, typename... Args>
T* GarbageCollector::allocate(Args&&... args) {
    static_assert(std::is_base_of_v<HeapObject, T>, "T must inherit from HeapObject");
    
    constexpr std::size_t size = sizeof(T);
    void* memory = allocate(size, T::TYPE);
    
    if (!memory) {
        collect();
        memory = allocate(size, T::TYPE);
        if (!memory) {
            throw std::bad_alloc();
        }
    }
    
    T* object = new (memory) T(std::forward<Args>(args)...);
    
    if constexpr (std::is_base_of_v<GCObject, T>) {
        if (barrier_) {
            barrier_->onPointerStore(object, 0, nullptr);
        }
    }
    
    return object;
}

class GCProfiler {
public:
    struct Stats {
        std::size_t collections;
        std::size_t totalPauseMs;
        std::size_t maxPauseMs;
        std::size_t bytesFreed;
        std::size_t bytesPromoted;
        std::size_t weakRefsCleared;
        std::size_t finalizersCalled;
    };
    
    GCProfiler();
    
    void startCollection();
    void endCollection();
    
    void recordAllocation(std::size_t size);
    void recordPromotion(std::size_t size);
    void recordWeakRefCleared();
    void recordFinalizerCalled();
    
    Stats getStats() const;
    void reset();

private:
    mutable std::mutex mutex_;
    Stats stats_;
    std::chrono::steady_clock::time_point collectionStart_;
    std::size_t allocationsSinceLastGC_;
    std::size_t bytesAllocatedSinceLastGC_;
};

} // namespace runtime
} // namespace internal
} // namespace klang