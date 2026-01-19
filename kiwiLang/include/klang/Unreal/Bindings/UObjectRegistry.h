#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Unreal/Bindings/UClassBinding.h"
#include "kiwiLang/Unreal/Bindings/UFunctionBinding.h"
#include "kiwiLang/Unreal/Bindings/UPropertyBinding.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace kiwiLang {
namespace Unreal {

class UERuntime;
struct UEBindingContext;

enum class UERegistryFlags : std::uint32_t {
    None = 0,
    ThreadSafe = 1 << 0,
    LazyLoad = 1 << 1,
    EnableCaching = 1 << 2,
    ValidateOnRegister = 1 << 3,
    GenerateStubs = 1 << 4,
    Default = ThreadSafe | EnableCaching | ValidateOnRegister
};

struct UERegistryConfig {
    UERegistryFlags flags;
    std::size_t maxCacheSize;
    std::size_t initialCapacity;
    bool enableGarbageCollection;
    std::chrono::milliseconds gcInterval;
    
    static UERegistryConfig getDefault();
};

class KIWI_UNREAL_API UObjectRegistry {
public:
    explicit UObjectRegistry(const UERegistryConfig& config = UERegistryConfig::getDefault());
    ~UObjectRegistry();
    
    bool registerClass(const UEClassInfo& classInfo,
                      const UEBindingContext& context);
    
    bool registerClassBinding(std::unique_ptr<UClassBindingBase> binding);
    bool registerFunctionBinding(std::unique_ptr<UFunctionBindingBase> binding);
    bool registerPropertyBinding(std::unique_ptr<UPropertyBindingBase> binding);
    
    bool unregisterClass(const std::string& className);
    bool unregisterFunction(const std::string& functionName);
    bool unregisterProperty(const std::string& propertyName);
    
    UClassBindingBase* findClassBinding(const std::string& className) const;
    UFunctionBindingBase* findFunctionBinding(const std::string& functionName) const;
    UPropertyBindingBase* findPropertyBinding(const std::string& propertyName) const;
    
    std::vector<std::string> getAllClassNames() const;
    std::vector<std::string> getAllFunctionNames() const;
    std::vector<std::string> getAllPropertyNames() const;
    
    void* createObject(const std::string& className);
    bool destroyObject(void* object, const std::string& className);
    
    Runtime::Value invokeFunction(const std::string& functionName,
                                 void* object,
                                 const std::vector<Runtime::Value>& args);
    
    Runtime::Value getPropertyValue(const std::string& propertyName,
                                   void* object);
    bool setPropertyValue(const std::string& propertyName,
                         void* object,
                         const Runtime::Value& value);
    
    bool generateBindingsCode(const std::string& outputDir);
    bool generateStubs(const std::string& outputDir);
    
    bool loadFromModule(const std::string& modulePath);
    bool saveToCache(const std::string& cachePath);
    bool loadFromCache(const std::string& cachePath);
    
    void clear();
    std::size_t getRegisteredCount() const;
    bool isEmpty() const;
    
    const UERegistryConfig& getConfig() const { return config_; }
    
private:
    struct RegistryEntry {
        std::unique_ptr<UClassBindingBase> classBinding;
        std::unordered_map<std::string, std::unique_ptr<UFunctionBindingBase>> functions;
        std::unordered_map<std::string, std::unique_ptr<UPropertyBindingBase>> properties;
        std::uint64_t lastAccessTime;
        std::uint32_t accessCount;
    };
    
    UERegistryConfig config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RegistryEntry> registry_;
    std::unordered_map<void*, std::string> objectToClassMap_;
    std::vector<std::pair<std::string, std::uint64_t>> accessHistory_;
    
    std::unique_ptr<UERuntime> runtime_;
    
    bool validateClassRegistration(const UEClassInfo& classInfo,
                                  const UEBindingContext& context) const;
    bool validateFunctionRegistration(const UEFunctionInfo& funcInfo,
                                     const UEBindingContext& context) const;
    bool validatePropertyRegistration(const UEPropertyInfo& propInfo,
                                     const UEBindingContext& context) const;
    
    void updateAccessHistory(const std::string& className);
    void cleanupOldEntries();
    void performGarbageCollection();
    
    RegistryEntry* findOrCreateEntry(const std::string& className);
    const RegistryEntry* findEntry(const std::string& className) const;
    
    bool generateClassWrapper(const RegistryEntry& entry,
                             const std::string& outputDir) const;
    bool generateFunctionWrappers(const RegistryEntry& entry,
                                 const std::string& outputDir) const;
    bool generatePropertyAccessors(const RegistryEntry& entry,
                                  const std::string& outputDir) const;
    
    friend class UERuntime;
};

} // namespace Unreal
} // namespace kiwiLang