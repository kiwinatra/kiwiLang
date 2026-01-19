#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Runtime/VM.h"
#include "kiwiLang/Runtime/GarbageCollector.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace kiwiLang {
namespace Unreal {

class UObjectRegistry;
class UETypeRegistry;
class UETypeConverter;

enum class UERuntimeMode {
    Editor,
    Standalone,
    Server,
    Client,
    DedicatedServer,
    PIE,
    Test
};

enum class UERuntimeFlags : std::uint32_t {
    None = 0,
    EnableGarbageCollection = 1 << 0,
    EnableHotReload = 1 << 1,
    EnableAsyncLoading = 1 << 2,
    EnableMemoryTracking = 1 << 3,
    EnableProfileMode = 1 << 4,
    EnableDebugSymbols = 1 << 5,
    EnableJITCompilation = 1 << 6,
    EnableMultithreading = 1 << 7,
    Default = EnableGarbageCollection | EnableAsyncLoading | EnableMemoryTracking
};

struct UERuntimeConfig {
    UERuntimeMode runtimeMode;
    UERuntimeFlags flags;
    std::size_t initialHeapSizeMB;
    std::size_t maxHeapSizeMB;
    std::size_t gcThresholdMB;
    std::chrono::milliseconds gcCollectionInterval;
    std::uint32_t maxThreadCount;
    bool enableJustInTimeCompilation;
    bool enableNativeCodeCache;
    
    static UERuntimeConfig getDefault();
    
    bool hasFlag(UERuntimeFlags flag) const {
        return (flags & flag) != UERuntimeFlags::None;
    }
};

struct UERuntimeStats {
    std::size_t totalAllocatedMemory;
    std::size_t usedMemory;
    std::size_t freeMemory;
    std::size_t gcCollections;
    std::size_t liveObjects;
    std::size_t totalObjectsCreated;
    std::size_t totalObjectsDestroyed;
    std::chrono::milliseconds totalGCTime;
    std::chrono::steady_clock::time_point startTime;
    std::uint64_t totalInstructionsExecuted;
    
    std::chrono::milliseconds getUptime() const;
    double getMemoryUsageRatio() const;
    double getGCAverageTime() const;
};

struct UERuntimeObjectHandle {
    void* objectPtr;
    std::string className;
    std::uint64_t objectId;
    std::uint32_t generation;
    bool isManaged;
    bool isGarbage;
    
    bool isValid() const { return objectPtr != nullptr && !isGarbage; }
    bool isSameObject(const UERuntimeObjectHandle& other) const {
        return objectId == other.objectId && generation == other.generation;
    }
};

class KIWI_UNREAL_API UERuntime {
public:
    explicit UERuntime(const UERuntimeConfig& config,
                      UObjectRegistry* objectRegistry = nullptr,
                      UETypeRegistry* typeRegistry = nullptr,
                      UETypeConverter* typeConverter = nullptr);
    ~UERuntime();
    
    bool initialize();
    bool shutdown();
    
    UERuntimeObjectHandle createObject(const std::string& className);
    bool destroyObject(const UERuntimeObjectHandle& handle);
    
    bool invokeMethod(const UERuntimeObjectHandle& handle,
                     const std::string& methodName,
                     const std::vector<Runtime::Value>& args,
                     Runtime::Value& returnValue);
    
    Runtime::Value getProperty(const UERuntimeObjectHandle& handle,
                              const std::string& propertyName);
    
    bool setProperty(const UERuntimeObjectHandle& handle,
                    const std::string& propertyName,
                    const Runtime::Value& value);
    
    bool loadBlueprint(const std::string& blueprintPath);
    bool unloadBlueprint(const std::string& blueprintPath);
    
    bool hotReloadModule(const std::string& moduleName);
    bool unloadModule(const std::string& moduleName);
    
    bool executeScript(const std::string& scriptCode);
    bool executeScriptFile(const std::string& filePath);
    
    bool startAsyncLoading();
    bool stopAsyncLoading();
    
    bool performGarbageCollection();
    bool compactMemory();
    
    UERuntimeStats getRuntimeStats() const;
    void resetRuntimeStats();
    
    bool isInitialized() const { return initialized_; }
    bool isShuttingDown() const { return shuttingDown_; }
    
    const UERuntimeConfig& getConfig() const { return config_; }
    Runtime::VM& getVirtualMachine() { return *vm_; }
    
private:
    struct RuntimeObject {
        void* object;
        std::string className;
        std::uint64_t objectId;
        std::uint32_t generation;
        std::size_t size;
        bool isManaged;
        bool isGarbage;
        std::chrono::steady_clock::time_point creationTime;
        std::chrono::steady_clock::time_point lastAccessTime;
        
        bool isValid() const { return object != nullptr && !isGarbage; }
    };
    
    UERuntimeConfig config_;
    UObjectRegistry* objectRegistry_;
    UETypeRegistry* typeRegistry_;
    UETypeConverter* typeConverter_;
    
    bool initialized_;
    bool shuttingDown_;
    std::atomic<std::uint64_t> nextObjectId_;
    
    std::unique_ptr<Runtime::VM> vm_;
    std::unique_ptr<Runtime::GarbageCollector> garbageCollector_;
    
    std::unordered_map<std::uint64_t, RuntimeObject> objects_;
    std::unordered_map<void*, std::uint64_t> pointerToObjectId_;
    std::unordered_multimap<std::string, std::uint64_t> classNameToObjectIds_;
    
    mutable std::recursive_mutex mutex_;
    std::thread gcThread_;
    std::atomic<bool> gcRunning_;
    
    UERuntimeStats stats_;
    
    bool initializeVirtualMachine();
    bool initializeGarbageCollector();
    bool initializeRuntimeLibraries();
    
    void shutdownVirtualMachine();
    void shutdownGarbageCollector();
    void cleanupAllObjects();
    
    std::uint64_t allocateObjectId();
    RuntimeObject* createRuntimeObject(void* object,
                                      const std::string& className,
                                      std::size_t size,
                                      bool isManaged);
    
    bool destroyRuntimeObject(std::uint64_t objectId);
    RuntimeObject* findObject(std::uint64_t objectId);
    const RuntimeObject* findObject(std::uint64_t objectId) const;
    
    void updateObjectAccessTime(std::uint64_t objectId);
    void markObjectAsGarbage(std::uint64_t objectId);
    
    bool validateObjectHandle(const UERuntimeObjectHandle& handle) const;
    bool validateMethodInvocation(const RuntimeObject& object,
                                 const std::string& methodName) const;
    bool validatePropertyAccess(const RuntimeObject& object,
                               const std::string& propertyName) const;
    
    void collectGarbage();
    void sweepGarbageObjects();
    void compactObjectMemory();
    
    void runGarbageCollectionThread();
    void stopGarbageCollectionThread();
    
    bool loadRuntimeModule(const std::string& moduleName);
    bool unloadRuntimeModule(const std::string& moduleName);
    
    bool registerNativeFunctions();
    bool registerBlueprintFunctions();
    bool registerEngineFunctions();
    
    Runtime::Value convertUObjectToValue(void* uobject, const std::string& className);
    void* convertValueToUObject(const Runtime::Value& value, const std::string& className);
    
    bool executeBlueprintGraph(const std::string& blueprintPath,
                              const std::string& entryPoint,
                              const std::vector<Runtime::Value>& args,
                              Runtime::Value& returnValue);
    
    bool compileAndExecuteScript(const std::string& scriptCode,
                                const std::string& contextName);
    
    void updateRuntimeStats();
    void trackMemoryAllocation(std::size_t size);
    void trackMemoryDeallocation(std::size_t size);
    void trackGarbageCollection(std::size_t collectedBytes,
                               std::chrono::milliseconds collectionTime);
    
    friend class UEGarbageCollector;
};

} // namespace Unreal
} // namespace kiwiLang