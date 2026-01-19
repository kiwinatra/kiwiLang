#pragma once

#include "kiwiLang/Runtime/GarbageCollector.h"
#include "kiwiLang/Unreal/Runtime/UERuntime.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace kiwiLang {
namespace Unreal {

class UERuntime;
class UObjectRegistry;

enum class UEGCAlgorithm {
    MarkAndSweep,
    Generational,
    ReferenceCounting,
    Incremental,
    Concurrent
};

enum class UEGCPhase {
    Idle,
    Marking,
    Sweeping,
    Compacting,
    Finalizing
};

enum class UEGCFlags : std::uint32_t {
    None = 0,
    EnableIncremental = 1 << 0,
    EnableConcurrent = 1 << 1,
    EnableGenerational = 1 << 2,
    EnableCompaction = 1 << 3,
    EnableWeakReferences = 1 << 4,
    EnableFinalizationQueue = 1 << 5,
    EnableMemoryPressureDetection = 1 << 6,
    EnableHeapSampling = 1 << 7,
    Default = EnableIncremental | EnableConcurrent | EnableCompaction
};

struct UEGCConfig {
    UEGCAlgorithm algorithm;
    UEGCFlags flags;
    std::size_t initialHeapSizeMB;
    std::size_t maxHeapSizeMB;
    std::size_t nurserySizeMB;
    std::size_t oldGenerationSizeMB;
    std::chrono::milliseconds collectionInterval;
    std::uint32_t maxPauseTimeMS;
    double heapGrowthFactor;
    bool enableAutoTuning;
    
    static UEGCConfig getDefault();
    
    bool hasFlag(UEGCFlags flag) const {
        return (flags & flag) != UEGCFlags::None;
    }
};

struct UEGCStats {
    std::size_t totalCollections;
    std::size_t minorCollections;
    std::size_t majorCollections;
    std::size_t totalBytesCollected;
    std::size_t totalBytesAllocated;
    std::size_t liveBytes;
    std::size_t liveObjects;
    std::chrono::milliseconds totalCollectionTime;
    std::chrono::milliseconds maxPauseTime;
    std::chrono::milliseconds averagePauseTime;
    double heapFragmentation;
    std::chrono::steady_clock::time_point lastCollectionTime;
    
    double getCollectionFrequency() const;
    double getAverageBytesPerCollection() const;
    double getHeapUtilization() const;
};

struct UEGCObjectInfo {
    void* object;
    std::uint64_t objectId;
    std::size_t size;
    std::uint32_t generation;
    std::uint32_t markCount;
    bool isMarked;
    bool isRoot;
    bool isPinned;
    bool isFinalizable;
    std::chrono::steady_clock::time_point allocationTime;
    
    bool isAlive() const { return isMarked || isRoot || isPinned; }
};

class KIWI_UNREAL_API UEGarbageCollector : public Runtime::GarbageCollector {
public:
    explicit UEGarbageCollector(UERuntime* runtime,
                               UObjectRegistry* objectRegistry,
                               const UEGCConfig& config = UEGCConfig::getDefault());
    ~UEGarbageCollector();
    
    // GarbageCollector interface
    void collect() override;
    void collectIncremental(std::size_t budget) override;
    void markRoots() override;
    void sweep() override;
    void compact() override;
    
    void pinObject(void* object) override;
    void unpinObject(void* object) override;
    bool isObjectPinned(void* object) const override;
    
    std::size_t getLiveBytes() const override;
    std::size_t getLiveObjects() const override;
    std::size_t getTotalCollections() const override;
    
    // UE-specific extensions
    bool initialize();
    bool shutdown();
    
    bool startConcurrentCollection();
    bool stopConcurrentCollection();
    
    bool performFullCollection();
    bool performGenerationalCollection();
    
    bool addRootObject(void* object, const std::string& className);
    bool removeRootObject(void* object);
    
    bool addWeakReference(void* object, void** weakRef);
    bool removeWeakReference(void* object, void** weakRef);
    
    bool registerFinalizer(void* object, std::function<void(void*)> finalizer);
    bool unregisterFinalizer(void* object);
    
    UEGCStats getGCStats() const;
    void resetGCStats();
    
    UEGCPhase getCurrentPhase() const { return currentPhase_; }
    bool isCollectionInProgress() const { return collectionInProgress_; }
    
    const UEGCConfig& getConfig() const { return config_; }
    
private:
    struct GCGeneration {
        std::size_t size;
        std::size_t used;
        std::vector<UEGCObjectInfo> objects;
        std::unordered_map<void*, std::size_t> objectToIndex;
        
        bool canAllocate(std::size_t requestedSize) const;
        std::size_t getFreeSpace() const { return size - used; }
    };
    
    struct GCWeakReference {
        void* object;
        void** weakRef;
        bool isAlive;
    };
    
    struct GCFinalizer {
        void* object;
        std::function<void(void*)> callback;
        bool hasRun;
    };
    
    UERuntime* runtime_;
    UObjectRegistry* objectRegistry_;
    UEGCConfig config_;
    
    std::atomic<UEGCPhase> currentPhase_;
    std::atomic<bool> collectionInProgress_;
    std::atomic<bool> shutdownRequested_;
    
    std::vector<GCGeneration> generations_;
    std::unordered_map<void*, bool> pinnedObjects_;
    std::unordered_map<void*, bool> rootObjects_;
    std::vector<GCWeakReference> weakReferences_;
    std::vector<GCFinalizer> finalizers_;
    std::vector<void*> finalizationQueue_;
    
    mutable std::recursive_mutex mutex_;
    std::thread concurrentThread_;
    std::condition_variable gcCondition_;
    
    UEGCStats stats_;
    std::chrono::steady_clock::time_point lastIncrementalStep_;
    
    // Mark-and-sweep implementation
    void markPhase();
    void sweepPhase();
    void compactPhase();
    
    // Generational implementation
    void minorCollection();
    void majorCollection();
    void promoteObject(void* object);
    
    // Incremental implementation
    bool incrementalMark(std::size_t budget);
    bool incrementalSweep(std::size_t budget);
    bool incrementalCompact(std::size_t budget);
    
    // Concurrent implementation
    void concurrentCollectionThread();
    void stopConcurrentCollectionThread();
    
    void markObject(void* object);
    void markFromRoots();
    void markTransitiveClosure(void* object);
    
    void sweepUnmarkedObjects();
    void sweepGeneration(GCGeneration& generation);
    
    void compactHeap();
    void compactGeneration(GCGeneration& generation);
    void updateObjectReferences(void* oldAddress, void* newAddress);
    
    bool isObjectReachable(void* object) const;
    std::vector<void*> getObjectReferences(void* object) const;
    
    void processWeakReferences();
    void processFinalizers();
    
    void runFinalizers();
    void queueFinalizer(void* object);
    
    void updateGCStats(std::size_t collectedBytes,
                      std::chrono::milliseconds collectionTime);
    void updateHeapFragmentation();
    
    bool shouldCollect() const;
    bool shouldCompact() const;
    std::size_t calculateCollectionBudget() const;
    
    GCGeneration& getGenerationForObject(void* object);
    const GCGeneration& getGenerationForObject(void* object) const;
    
    std::size_t estimateObjectSize(void* object) const;
    std::vector<void*> estimateObjectReferences(void* object) const;
    
    void handleMemoryPressure();
    void adjustGCParameters();
    
    bool validateObject(void* object) const;
    bool validateHeap() const;
    
    friend class UERuntime;
};

} // namespace Unreal
} // namespace kiwiLang