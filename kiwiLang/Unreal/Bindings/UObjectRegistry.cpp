#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Runtime/UERuntime.h"
#include "kiwiLang/Unreal/Types/UETypeConverter.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include "kiwiLang/Compiler/Compiler.h"
#include <fstream>
#include <chrono>

namespace kiwiLang {
namespace Unreal {

UERegistryConfig UERegistryConfig::getDefault() {
    UERegistryConfig config;
    config.flags = UERegistryFlags::Default;
    config.maxCacheSize = 1000;
    config.initialCapacity = 100;
    config.enableGarbageCollection = true;
    config.gcInterval = std::chrono::seconds(30);
    return config;
}

UObjectRegistry::UObjectRegistry(const UERegistryConfig& config)
    : config_(config) {
    
    if (config.hasFlag(UERegistryFlags::ThreadSafe)) {
        // mutex_ is already initialized
    }
    
    if (config.enableGarbageCollection) {
        runtime_ = std::make_unique<UERuntime>(
            UERuntimeConfig::getDefault(),
            this,
            nullptr,
            nullptr);
    }
}

UObjectRegistry::~UObjectRegistry() {
    clear();
}

bool UObjectRegistry::registerClass(const UEClassInfo& classInfo,
                                   const UEBindingContext& context) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!validateClassRegistration(classInfo, context)) {
        return false;
    }
    
    // Create binding using factory
    auto binding = UClassBindingFactory::createBinding(classInfo, context);
    if (!binding) {
        DiagnosticEngine::get().error("Failed to create class binding for: " + classInfo.name);
        return false;
    }
    
    return registerClassBinding(std::move(binding));
}

bool UObjectRegistry::registerClassBinding(std::unique_ptr<UClassBindingBase> binding) {
    if (!binding) {
        return false;
    }
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const std::string& className = binding->getClassInfo().name;
    RegistryEntry& entry = *findOrCreateEntry(className);
    
    if (entry.classBinding) {
        DiagnosticEngine::get().warning("Class binding already exists: " + className);
        return false;
    }
    
    entry.classBinding = std::move(binding);
    entry.lastAccessTime = std::chrono::steady_clock::now().time_since_epoch().count();
    entry.accessCount = 1;
    
    DiagnosticEngine::get().info("Registered class binding: " + className);
    updateAccessHistory(className);
    
    return true;
}

bool UObjectRegistry::registerFunctionBinding(std::unique_ptr<UFunctionBindingBase> binding) {
    if (!binding) {
        return false;
    }
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const std::string& functionName = binding->getParams().functionName;
    std::string className;
    
    // Extract class name from function name (assuming ClassName::FunctionName format)
    size_t separator = functionName.find("::");
    if (separator != std::string::npos) {
        className = functionName.substr(0, separator);
    } else {
        DiagnosticEngine::get().error("Function name must include class: " + functionName);
        return false;
    }
    
    RegistryEntry& entry = *findOrCreateEntry(className);
    
    if (entry.functions.find(functionName) != entry.functions.end()) {
        DiagnosticEngine::get().warning("Function binding already exists: " + functionName);
        return false;
    }
    
    entry.functions[functionName] = std::move(binding);
    entry.lastAccessTime = std::chrono::steady_clock::now().time_since_epoch().count();
    
    DiagnosticEngine::get().info("Registered function binding: " + functionName);
    updateAccessHistory(className);
    
    return true;
}

bool UObjectRegistry::registerPropertyBinding(std::unique_ptr<UPropertyBindingBase> binding) {
    if (!binding) {
        return false;
    }
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const std::string& propertyName = binding->getParams().propertyName;
    std::string className;
    
    // Extract class name from property name (assuming ClassName::PropertyName format)
    size_t separator = propertyName.find("::");
    if (separator != std::string::npos) {
        className = propertyName.substr(0, separator);
    } else {
        DiagnosticEngine::get().error("Property name must include class: " + propertyName);
        return false;
    }
    
    RegistryEntry& entry = *findOrCreateEntry(className);
    
    if (entry.properties.find(propertyName) != entry.properties.end()) {
        DiagnosticEngine::get().warning("Property binding already exists: " + propertyName);
        return false;
    }
    
    entry.properties[propertyName] = std::move(binding);
    entry.lastAccessTime = std::chrono::steady_clock::now().time_since_epoch().count();
    
    DiagnosticEngine::get().info("Registered property binding: " + propertyName);
    updateAccessHistory(className);
    
    return true;
}

bool UObjectRegistry::unregisterClass(const std::string& className) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = registry_.find(className);
    if (it == registry_.end()) {
        DiagnosticEngine::get().warning("Class not registered: " + className);
        return false;
    }
    
    // Clean up any objects of this class
    for (auto objIt = objectToClassMap_.begin(); objIt != objectToClassMap_.end(); ) {
        if (objIt->second == className) {
            if (runtime_) {
                runtime_->destroyObject(objIt->first);
            }
            objIt = objectToClassMap_.erase(objIt);
        } else {
            ++objIt;
        }
    }
    
    registry_.erase(it);
    
    // Remove from access history
    accessHistory_.erase(
        std::remove_if(accessHistory_.begin(), accessHistory_.end(),
            [&](const auto& pair) { return pair.first == className; }),
        accessHistory_.end());
    
    DiagnosticEngine::get().info("Unregistered class: " + className);
    return true;
}

bool UObjectRegistry::unregisterFunction(const std::string& functionName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string className;
    size_t separator = functionName.find("::");
    if (separator != std::string::npos) {
        className = functionName.substr(0, separator);
    } else {
        return false;
    }
    
    auto it = registry_.find(className);
    if (it == registry_.end()) {
        return false;
    }
    
    auto funcIt = it->second.functions.find(functionName);
    if (funcIt == it->second.functions.end()) {
        return false;
    }
    
    it->second.functions.erase(funcIt);
    DiagnosticEngine::get().info("Unregistered function: " + functionName);
    return true;
}

bool UObjectRegistry::unregisterProperty(const std::string& propertyName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string className;
    size_t separator = propertyName.find("::");
    if (separator != std::string::npos) {
        className = propertyName.substr(0, separator);
    } else {
        return false;
    }
    
    auto it = registry_.find(className);
    if (it == registry_.end()) {
        return false;
    }
    
    auto propIt = it->second.properties.find(propertyName);
    if (propIt == it->second.properties.end()) {
        return false;
    }
    
    it->second.properties.erase(propIt);
    DiagnosticEngine::get().info("Unregistered property: " + propertyName);
    return true;
}

UClassBindingBase* UObjectRegistry::findClassBinding(const std::string& className) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const RegistryEntry* entry = findEntry(className);
    if (entry && entry->classBinding) {
        updateAccessHistory(className);
        return entry->classBinding.get();
    }
    
    return nullptr;
}

UFunctionBindingBase* UObjectRegistry::findFunctionBinding(const std::string& functionName) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string className;
    size_t separator = functionName.find("::");
    if (separator != std::string::npos) {
        className = functionName.substr(0, separator);
    } else {
        return nullptr;
    }
    
    const RegistryEntry* entry = findEntry(className);
    if (!entry) {
        return nullptr;
    }
    
    auto it = entry->functions.find(functionName);
    if (it != entry->functions.end()) {
        updateAccessHistory(className);
        return it->second.get();
    }
    
    return nullptr;
}

UPropertyBindingBase* UObjectRegistry::findPropertyBinding(const std::string& propertyName) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string className;
    size_t separator = propertyName.find("::");
    if (separator != std::string::npos) {
        className = propertyName.substr(0, separator);
    } else {
        return nullptr;
    }
    
    const RegistryEntry* entry = findEntry(className);
    if (!entry) {
        return nullptr;
    }
    
    auto it = entry->properties.find(propertyName);
    if (it != entry->properties.end()) {
        updateAccessHistory(className);
        return it->second.get();
    }
    
    return nullptr;
}

std::vector<std::string> UObjectRegistry::getAllClassNames() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> classNames;
    classNames.reserve(registry_.size());
    
    for (const auto& [className, entry] : registry_) {
        if (entry.classBinding) {
            classNames.push_back(className);
        }
    }
    
    return classNames;
}

std::vector<std::string> UObjectRegistry::getAllFunctionNames() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> functionNames;
    
    for (const auto& [className, entry] : registry_) {
        for (const auto& [funcName, _] : entry.functions) {
            functionNames.push_back(funcName);
        }
    }
    
    return functionNames;
}

std::vector<std::string> UObjectRegistry::getAllPropertyNames() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> propertyNames;
    
    for (const auto& [className, entry] : registry_) {
        for (const auto& [propName, _] : entry.properties) {
            propertyNames.push_back(propName);
        }
    }
    
    return propertyNames;
}

void* UObjectRegistry::createObject(const std::string& className) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    UClassBindingBase* binding = findClassBinding(className);
    if (!binding) {
        DiagnosticEngine::get().error("Class not found: " + className);
        return nullptr;
    }
    
    void* object = binding->createInstance();
    if (!object) {
        DiagnosticEngine::get().error("Failed to create instance of: " + className);
        return nullptr;
    }
    
    // Initialize the object
    if (!binding->initializeInstance(object)) {
        DiagnosticEngine::get().warning("Failed to initialize instance of: " + className);
        // Continue anyway
    }
    
    // Track the object
    objectToClassMap_[object] = className;
    
    if (runtime_) {
        runtime_->trackObject(object, className);
    }
    
    DiagnosticEngine::get().info("Created object of class: " + className);
    return object;
}

bool UObjectRegistry::destroyObject(void* object, const std::string& className) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = objectToClassMap_.find(object);
    if (it == objectToClassMap_.end() || it->second != className) {
        DiagnosticEngine::get().error("Object not found or class mismatch");
        return false;
    }
    
    UClassBindingBase* binding = findClassBinding(className);
    if (!binding) {
        DiagnosticEngine::get().error("Class binding not found: " + className);
        return false;
    }
    
    binding->destroyInstance(object);
    objectToClassMap_.erase(it);
    
    if (runtime_) {
        runtime_->untrackObject(object);
    }
    
    DiagnosticEngine::get().info("Destroyed object of class: " + className);
    return true;
}

Runtime::Value UObjectRegistry::invokeFunction(const std::string& functionName,
                                              void* object,
                                              const std::vector<Runtime::Value>& args) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    UFunctionBindingBase* binding = findFunctionBinding(functionName);
    if (!binding) {
        DiagnosticEngine::get().error("Function not found: " + functionName);
        return Runtime::Value::null();
    }
    
    if (!object) {
        DiagnosticEngine::get().error("Null object for function: " + functionName);
        return Runtime::Value::null();
    }
    
    // Convert args to void* array for invocation
    std::vector<void*> rawArgs;
    rawArgs.reserve(args.size());
    
    for (const auto& arg : args) {
     
        rawArgs.push_back(const_cast<void*>(static_cast<const void*>(&arg)));
    }
    
    void* result = binding->invoke(object, rawArgs.data());
    
    
    if (result) {
        return Runtime::Value::pointer(result);
    }
    
    return Runtime::Value::null();
}

Runtime::Value UObjectRegistry::getPropertyValue(const std::string& propertyName,
                                                void* object) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    UPropertyBindingBase* binding = findPropertyBinding(propertyName);
    if (!binding) {
        DiagnosticEngine::get().error("Property not found: " + propertyName);
        return Runtime::Value::null();
    }
    
    if (!object) {
        DiagnosticEngine::get().error("Null object for property: " + propertyName);
        return Runtime::Value::null();
    }
    
    return binding->getValue(object);
}

bool UObjectRegistry::setPropertyValue(const std::string& propertyName,
                                      void* object,
                                      const Runtime::Value& value) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    UPropertyBindingBase* binding = findPropertyBinding(propertyName);
    if (!binding) {
        DiagnosticEngine::get().error("Property not found: " + propertyName);
        return false;
    }
    
    if (!object) {
        DiagnosticEngine::get().error("Null object for property: " + propertyName);
        return false;
    }
    
    return binding->setValue(object, value);
}

bool UObjectRegistry::generateBindingsCode(const std::string& outputDir) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Generating bindings code to: " + outputDir);
    
    bool success = true;
    
    for (const auto& [className, entry] : registry_) {
        if (!generateClassWrapper(entry, outputDir)) {
            DiagnosticEngine::get().error("Failed to generate wrapper for: " + className);
            success = false;
        }
        
        if (!generateFunctionWrappers(entry, outputDir)) {
            DiagnosticEngine::get().error("Failed to generate function wrappers for: " + className);
            success = false;
        }
        
        if (!generatePropertyAccessors(entry, outputDir)) {
            DiagnosticEngine::get().error("Failed to generate property accessors for: " + className);
            success = false;
        }
    }
    
    return success;
}

bool UObjectRegistry::generateStubs(const std::string& outputDir) {
    if (!config_.hasFlag(UERegistryFlags::GenerateStubs)) {
        return true;
    }
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Generating stubs to: " + outputDir);
    
    // Create output directory
    std::filesystem::path dirPath(outputDir);
    std::filesystem::create_directories(dirPath);
    
    // Generate header with all bindings
    std::ofstream headerFile(dirPath / "kiwiLangBindings.h");
    if (!headerFile.is_open()) {
        DiagnosticEngine::get().error("Cannot create bindings header file");
        return false;
    }
    
    headerFile << "#pragma once\n\n";
    headerFile << "// Generated by kiwiLang - DO NOT EDIT\n\n";
    
    for (const auto& [className, entry] : registry_) {
        if (entry.classBinding) {
            headerFile << "// Class: " << className << "\n";
            headerFile << entry.classBinding->generateHeaderCode() << "\n\n";
        }
    }
    
    return true;
}

bool UObjectRegistry::loadFromModule(const std::string& modulePath) {
    DiagnosticEngine::get().info("Loading bindings from module: " + modulePath);
    
   
    return true;
}

bool UObjectRegistry::saveToCache(const std::string& cachePath) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Saving registry cache to: " + cachePath);
    
    // Create cache directory
    std::filesystem::path dirPath(cachePath);
    std::filesystem::create_directories(dirPath);
    
   
    
    return true;
}

bool UObjectRegistry::loadFromCache(const std::string& cachePath) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Loading registry cache from: " + cachePath);
    
    if (!std::filesystem::exists(cachePath)) {
        DiagnosticEngine::get().warning("Cache file does not exist: " + cachePath);
        return false;
    }
    
    // Deserialize registry state
    // This would involve reading the cache file and reconstructing bindings
    
    return true;
}

void UObjectRegistry::clear() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Destroy all tracked objects
    for (const auto& [object, className] : objectToClassMap_) {
        UClassBindingBase* binding = findClassBinding(className);
        if (binding) {
            binding->destroyInstance(object);
        }
    }
    
    registry_.clear();
    objectToClassMap_.clear();
    accessHistory_.clear();
    
    if (runtime_) {
        runtime_->clear();
    }
    
    DiagnosticEngine::get().info("Cleared object registry");
}

std::size_t UObjectRegistry::getRegisteredCount() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return registry_.size();
}

bool UObjectRegistry::isEmpty() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return registry_.empty();
}

bool UObjectRegistry::validateClassRegistration(const UEClassInfo& classInfo,
                                               const UEBindingContext& context) const {
    if (classInfo.name.empty()) {
        DiagnosticEngine::get().error("Class name cannot be empty");
        return false;
    }
    
    // Check if class is already registered
    if (registry_.find(classInfo.name) != registry_.end()) {
        DiagnosticEngine::get().warning("Class already registered: " + classInfo.name);
        return false;
    }
    
    // Validate properties
    for (const auto& prop : classInfo.properties) {
        if (!validatePropertyRegistration(prop, context)) {
            return false;
        }
    }
    
    // Validate functions
    for (const auto& func : classInfo.functions) {
        if (!validateFunctionRegistration(func, context)) {
            return false;
        }
    }
    
    return true;
}

bool UObjectRegistry::validateFunctionRegistration(const UEFunctionInfo& funcInfo,
                                                  const UEBindingContext& context) const {
    if (funcInfo.name.empty()) {
        DiagnosticEngine::get().error("Function name cannot be empty");
        return false;
    }
    
    // Check for name conflicts
    std::string fullName = funcInfo.name; // Should include class name
    // Implementation would check for conflicts
    
    return true;
}

bool UObjectRegistry::validatePropertyRegistration(const UEPropertyInfo& propInfo,
                                                  const UEBindingContext& context) const {
    if (propInfo.name.empty()) {
        DiagnosticEngine::get().error("Property name cannot be empty");
        return false;
    }
    
    if (!propInfo.kiwiType) {
        DiagnosticEngine::get().error("Property must have kiwi type: " + propInfo.name);
        return false;
    }
    
    // Check type compatibility with UE
    if (context.typeRegistry) {
        std::string ueTypeName = context.typeRegistry->findUETypeName(propInfo.kiwiType);
        if (ueTypeName.empty()) {
            DiagnosticEngine::get().warning("No UE type mapping for property: " + propInfo.name);
        }
    }
    
    return true;
}

void UObjectRegistry::updateAccessHistory(const std::string& className) {
    if (!config_.hasFlag(UERegistryFlags::EnableCaching)) {
        return;
    }
    
    auto currentTime = std::chrono::steady_clock::now().time_since_epoch().count();
    
    // Update or add entry
    auto it = std::find_if(accessHistory_.begin(), accessHistory_.end(),
        [&](const auto& pair) { return pair.first == className; });
    
    if (it != accessHistory_.end()) {
        it->second = currentTime;
    } else {
        accessHistory_.emplace_back(className, currentTime);
    }
    
    // Keep history size limited
    if (accessHistory_.size() > config_.maxCacheSize * 2) {
        // Remove oldest entries
        std::sort(accessHistory_.begin(), accessHistory_.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        accessHistory_.resize(config_.maxCacheSize);
    }
    
    // Clean up old entries if enabled
    if (config_.enableGarbageCollection) {
        cleanupOldEntries();
    }
}

void UObjectRegistry::cleanupOldEntries() {
    auto currentTime = std::chrono::steady_clock::now().time_since_epoch().count();
    auto cutoffTime = currentTime - std::chrono::duration_cast<std::chrono::nanoseconds>(
        config_.gcInterval).count();
    
    for (auto it = registry_.begin(); it != registry_.end(); ) {
        if (it->second.lastAccessTime < cutoffTime && 
            it->second.accessCount < 10) { // Rarely used
            DiagnosticEngine::get().info("Cleaning up unused class: " + it->first);
            it = registry_.erase(it);
        } else {
            ++it;
        }
    }
}

void UObjectRegistry::performGarbageCollection() {
    if (!config_.enableGarbageCollection || !runtime_) {
        return;
    }
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    runtime_->performGarbageCollection();
    cleanupOldEntries();
}

UObjectRegistry::RegistryEntry* UObjectRegistry::findOrCreateEntry(const std::string& className) {
    auto it = registry_.find(className);
    if (it == registry_.end()) {
        auto [newIt, inserted] = registry_.emplace(className, RegistryEntry{});
        if (inserted) {
            newIt->second.lastAccessTime = std::chrono::steady_clock::now().time_since_epoch().count();
            newIt->second.accessCount = 0;
        }
        return &newIt->second;
    }
    return &it->second;
}

const UObjectRegistry::RegistryEntry* UObjectRegistry::findEntry(const std::string& className) const {
    auto it = registry_.find(className);
    if (it != registry_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool UObjectRegistry::generateClassWrapper(const RegistryEntry& entry,
                                          const std::string& outputDir) const {
    if (!entry.classBinding) {
        return true; // Nothing to generate
    }
    
    std::filesystem::path dirPath(outputDir);
    std::filesystem::create_directories(dirPath);
    
    std::string className = entry.classBinding->getClassInfo().name;
    std::filesystem::path headerPath = dirPath / (className + ".h");
    std::filesystem::path sourcePath = dirPath / (className + ".cpp");
    
    // Generate header
    std::ofstream headerFile(headerPath);
    if (!headerFile.is_open()) {
        return false;
    }
    
    headerFile << entry.classBinding->generateHeaderCode();
    
    // Generate source
    std::ofstream sourceFile(sourcePath);
    if (!sourceFile.is_open()) {
        return false;
    }
    
    sourceFile << entry.classBinding->generateWrapperCode();
    
    return true;
}

bool UObjectRegistry::generateFunctionWrappers(const RegistryEntry& entry,
                                              const std::string& outputDir) const {
    if (entry.functions.empty()) {
        return true;
    }
    
    std::filesystem::path dirPath(outputDir);
    std::filesystem::create_directories(dirPath);
    
    // Generate function wrappers file
    std::string className = entry.classBinding ? entry.classBinding->getClassInfo().name : "Unknown";
    std::filesystem::path filePath = dirPath / (className + "_Functions.cpp");
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "// Function wrappers for class: " << className << "\n";
    file << "// Generated by kiwiLang - DO NOT EDIT\n\n";
    
    for (const auto& [funcName, funcBinding] : entry.functions) {
        file << funcBinding->generateWrapperCode() << "\n\n";
    }
    
    return true;
}

bool UObjectRegistry::generatePropertyAccessors(const RegistryEntry& entry,
                                               const std::string& outputDir) const {
    if (entry.properties.empty()) {
        return true;
    }
    
    std::filesystem::path dirPath(outputDir);
    std::filesystem::create_directories(dirPath);
    
    // Generate property accessors file
    std::string className = entry.classBinding ? entry.classBinding->getClassInfo().name : "Unknown";
    std::filesystem::path filePath = dirPath / (className + "_Properties.cpp");
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "// Property accessors for class: " << className << "\n";
    file << "// Generated by kiwiLang - DO NOT EDIT\n\n";
    
    for (const auto& [propName, propBinding] : entry.properties) {
        file << propBinding->generateAccessorCode() << "\n\n";
    }
    
    return true;
}

} // namespace Unreal
} // namespace kiwiLang