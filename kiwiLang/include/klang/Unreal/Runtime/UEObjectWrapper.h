#pragma once

#include "kiwiLang/Runtime/Value.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UERuntime;
class UObjectRegistry;
class UETypeConverter;

enum class UEObjectWrapperFlags : std::uint32_t {
    None = 0,
    ManagedByGC = 1 << 0,
    WeakReference = 1 << 1,
    Pinned = 1 << 2,
    Finalizable = 1 << 3,
    AutoSerialize = 1 << 4,
    AutoReplicate = 1 << 5,
    BlueprintAccessible = 1 << 6,
    EditorVisible = 1 << 7,
    Transient = 1 << 8,
    Default = ManagedByGC | BlueprintAccessible
};

struct UEObjectWrapperConfig {
    UEObjectWrapperFlags flags;
    std::string className;
    std::unordered_map<std::string, std::string> metadata;
    std::function<void(void*)> destructorCallback;
    std::function<void(void*, const std::string&, const Runtime::Value&)> propertySetter;
    std::function<Runtime::Value(void*, const std::string&)> propertyGetter;
    
    static UEObjectWrapperConfig getDefault();
    
    bool hasFlag(UEObjectWrapperFlags flag) const {
        return (flags & flag) != UEObjectWrapperFlags::None;
    }
};

class KIWI_UNREAL_API UEObjectWrapper {
public:
    explicit UEObjectWrapper(void* uobject,
                            const std::string& className,
                            const UEObjectWrapperConfig& config = UEObjectWrapperConfig::getDefault());
    ~UEObjectWrapper();
    
    static std::shared_ptr<UEObjectWrapper> create(void* uobject,
                                                  const std::string& className,
                                                  const UEObjectWrapperConfig& config = UEObjectWrapperConfig::getDefault());
    
    bool initialize();
    bool destroy();
    
    void* getUObject() const { return uobject_; }
    const std::string& getClassName() const { return className_; }
    std::uint64_t getObjectId() const { return objectId_; }
    
    Runtime::Value getProperty(const std::string& propertyName);
    bool setProperty(const std::string& propertyName, const Runtime::Value& value);
    
    Runtime::Value invokeMethod(const std::string& methodName,
                               const std::vector<Runtime::Value>& args);
    
    bool hasProperty(const std::string& propertyName) const;
    bool hasMethod(const std::string& methodName) const;
    
    bool serializeToBuffer(std::vector<std::uint8_t>& buffer) const;
    bool deserializeFromBuffer(const std::vector<std::uint8_t>& buffer);
    
    bool replicateToClients();
    bool receiveReplication(const std::vector<std::uint8_t>& data);
    
    void pin();
    void unpin();
    bool isPinned() const;
    
    void addRef();
    void releaseRef();
    std::uint32_t getRefCount() const;
    
    bool isValid() const;
    bool isManaged() const;
    
    const UEObjectWrapperConfig& getConfig() const { return config_; }
    
private:
    struct PropertyCache {
        std::string name;
        std::string typeName;
        std::function<Runtime::Value(void*)> getter;
        std::function<bool(void*, const Runtime::Value&)> setter;
        std::uint32_t offset;
        std::uint32_t size;
        bool isReadOnly;
        bool isBlueprintVisible;
    };
    
    struct MethodCache {
        std::string name;
        std::function<Runtime::Value(void*, const std::vector<Runtime::Value>&)> invoker;
        std::vector<std::string> parameterTypes;
        std::string returnType;
        bool isBlueprintCallable;
        bool isConst;
        bool isStatic;
    };
    
    void* uobject_;
    std::string className_;
    std::uint64_t objectId_;
    UEObjectWrapperConfig config_;
    
    std::atomic<std::uint32_t> refCount_;
    std::atomic<bool> isPinned_;
    std::atomic<bool> isDestroyed_;
    
    std::unordered_map<std::string, PropertyCache> propertyCache_;
    std::unordered_map<std::string, MethodCache> methodCache_;
    
    UERuntime* runtime_;
    UObjectRegistry* objectRegistry_;
    UETypeConverter* typeConverter_;
    
    static std::atomic<std::uint64_t> nextObjectId_;
    
    bool initializePropertyCache();
    bool initializeMethodCache();
    bool initializeTypeInformation();
    
    bool populatePropertyCache();
    bool populateMethodCache();
    
    PropertyCache createPropertyCache(const std::string& propertyName);
    MethodCache createMethodCache(const std::string& methodName);
    
    Runtime::Value getPropertyDirect(const PropertyCache& cache);
    bool setPropertyDirect(const PropertyCache& cache, const Runtime::Value& value);
    
    Runtime::Value invokeMethodDirect(const MethodCache& cache,
                                     const std::vector<Runtime::Value>& args);
    
    bool validatePropertyAccess(const PropertyCache& cache) const;
    bool validateMethodInvocation(const MethodCache& cache,
                                 const std::vector<Runtime::Value>& args) const;
    
    std::vector<std::uint8_t> serializeProperty(const PropertyCache& cache) const;
    bool deserializeProperty(const PropertyCache& cache, const std::vector<std::uint8_t>& data);
    
    bool replicateProperty(const PropertyCache& cache);
    bool receivePropertyReplication(const PropertyCache& cache, const std::vector<std::uint8_t>& data);
    
    void cleanupPropertyCache();
    void cleanupMethodCache();
    
    void finalizeObject();
    void notifyDestruction();
    
    bool checkObjectValidity() const;
    bool updateObjectReference(void* newUObject);
    
    friend class UERuntime;
    friend class UEGarbageCollector;
};

class KIWI_UNREAL_API UEObjectWrapperFactory {
public:
    explicit UEObjectWrapperFactory(UERuntime* runtime,
                                   UObjectRegistry* objectRegistry = nullptr,
                                   UETypeConverter* typeConverter = nullptr);
    
    std::shared_ptr<UEObjectWrapper> createWrapper(void* uobject,
                                                  const std::string& className,
                                                  const UEObjectWrapperConfig& config = UEObjectWrapperConfig::getDefault());
    
    std::shared_ptr<UEObjectWrapper> findWrapper(void* uobject) const;
    std::shared_ptr<UEObjectWrapper> findWrapper(std::uint64_t objectId) const;
    
    bool destroyWrapper(void* uobject);
    bool destroyWrapper(std::uint64_t objectId);
    
    bool registerWrapperType(const std::string& className,
                            std::function<std::shared_ptr<UEObjectWrapper>(void*, const UEObjectWrapperConfig&)> factory);
    
    bool unregisterWrapperType(const std::string& className);
    
    bool hasWrapper(void* uobject) const;
    bool hasWrapper(std::uint64_t objectId) const;
    
    std::vector<std::shared_ptr<UEObjectWrapper>> getAllWrappers() const;
    std::vector<std::shared_ptr<UEObjectWrapper>> getWrappersByClass(const std::string& className) const;
    
    void cleanupOrphanedWrappers();
    void destroyAllWrappers();
    
private:
    UERuntime* runtime_;
    UObjectRegistry* objectRegistry_;
    UETypeConverter* typeConverter_;
    
    std::unordered_map<void*, std::weak_ptr<UEObjectWrapper>> objectToWrapper_;
    std::unordered_map<std::uint64_t, std::weak_ptr<UEObjectWrapper>> idToWrapper_;
    std::unordered_multimap<std::string, std::weak_ptr<UEObjectWrapper>> classToWrappers_;
    std::unordered_map<std::string, std::function<std::shared_ptr<UEObjectWrapper>(void*, const UEObjectWrapperConfig&)>> wrapperFactories_;
    
    mutable std::recursive_mutex mutex_;
    
    std::shared_ptr<UEObjectWrapper> createDefaultWrapper(void* uobject,
                                                         const std::string& className,
                                                         const UEObjectWrapperConfig& config);
    
    bool registerDefaultWrapperTypes();
    
    void cleanupExpiredWrappers();
    void removeWrapperFromMaps(const std::shared_ptr<UEObjectWrapper>& wrapper);
    
    bool validateUObject(void* uobject, const std::string& className) const;
    bool validateWrapperConfig(const UEObjectWrapperConfig& config) const;
};

} // namespace Unreal
} // namespace kiwiLang