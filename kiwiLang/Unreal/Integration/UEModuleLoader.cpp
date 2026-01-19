#include "kiwiLang/Unreal/Integration/UEModuleLoader.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeSystem.h"
#include "kiwiLang/Unreal/Integration/UEProjectBuilder.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <dlfcn.h>
#include <filesystem>
#include <chrono>

namespace kiwiLang {
namespace Unreal {

using namespace std::filesystem;

UEModuleLoader::UEModuleLoader(const UEConfig& config,
                             UETypeRegistry* typeRegistry,
                             UObjectRegistry* objectRegistry,
                             UEProjectBuilder* projectBuilder)
    : config_(config)
    , typeRegistry_(typeRegistry)
    , objectRegistry_(objectRegistry)
    , projectBuilder_(projectBuilder) {
}

UEModuleLoader::~UEModuleLoader() {
    shutdown();
}

bool UEModuleLoader::initialize() {
    DiagnosticEngine::get().info("Initializing UE Module Loader");
    
    // Load core engine modules
    if (!loadCoreModules()) {
        DiagnosticEngine::get().error("Failed to load core modules");
        return false;
    }
    
    return true;
}

bool UEModuleLoader::shutdown() {
    DiagnosticEngine::get().info("Shutting down UE Module Loader");
    
    // Unload all modules
    unloadAllModules();
    
    // Clear all data
    modules_.clear();
    phaseModules_.clear();
    handleToModuleMap_.clear();
    globalSymbols_.clear();
    
    return true;
}

UEModuleLoadResult UEModuleLoader::loadModule(const std::string& moduleName,
                                             UEModuleLoadPhase phase) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    UEModuleLoadResult result;
    result.moduleName = moduleName;
    result.success = false;
    
    DiagnosticEngine::get().info("Loading module: " + moduleName);
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Check if module is already loaded
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt != modules_.end() && moduleIt->second.isLoaded) {
        DiagnosticEngine::get().warning("Module already loaded: " + moduleName);
        result.success = true;
        result.moduleHandle = moduleIt->second.handle;
        return result;
    }
    
    // Find module info
    const UEModuleInfo* moduleInfo = findModule(moduleName);
    if (!moduleInfo) {
        result.errors.push_back("Module not registered: " + moduleName);
        return result;
    }
    
    // Validate module compatibility
    if (!checkModuleCompatibility(*moduleInfo)) {
        result.errors.push_back("Module incompatible: " + moduleName);
        return result;
    }
    
    // Create module state
    ModuleState state;
    state.info = *moduleInfo;
    state.isLoaded = false;
    
    // Load dependencies first
    std::vector<std::string> loadOrder;
    if (!loadModuleDependencies(state, loadOrder)) {
        result.errors.push_back("Failed to load dependencies for: " + moduleName);
        return result;
    }
    
    // Find module binary
    path binaryPath = findModuleBinary(*moduleInfo, config_.targetPlatform);
    if (binaryPath.empty() || !exists(binaryPath)) {
        result.errors.push_back("Module binary not found: " + binaryPath.string());
        return result;
    }
    
    // Verify module signature
    if (!verifyModuleSignature(binaryPath)) {
        result.errors.push_back("Invalid module signature: " + moduleName);
        return result;
    }
    
    // Load the binary
    state.handle = loadModuleBinary(binaryPath, result.errors);
    if (!state.handle) {
        result.errors.push_back("Failed to load module binary: " + moduleName);
        return result;
    }
    
    // Register module
    state.isLoaded = true;
    state.loadTime = std::chrono::system_clock::now();
    modules_[moduleName] = std::move(state);
    handleToModuleMap_[state.handle] = moduleName;
    
    // Register symbols
    if (!registerModuleSymbols(modules_[moduleName])) {
        result.errors.push_back("Failed to register module symbols: " + moduleName);
        unloadModuleBinary(state.handle);
        modules_.erase(moduleName);
        handleToModuleMap_.erase(state.handle);
        return result;
    }
    
    // Execute module initializers
    if (!executeModuleInitializers(modules_[moduleName])) {
        result.errors.push_back("Module initialization failed: " + moduleName);
        unloadModuleBinary(state.handle);
        modules_.erase(moduleName);
        handleToModuleMap_.erase(state.handle);
        return result;
    }
    
    // Add to phase
    phaseModules_[phase].push_back(moduleName);
    
    result.success = true;
    result.moduleHandle = state.handle;
    result.loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);
    
    DiagnosticEngine::get().info("Successfully loaded module: " + moduleName + 
                                " in " + std::to_string(result.loadTime.count()) + "ms");
    
    return result;
}

UEModuleLoadResult UEModuleLoader::loadModuleFromPath(const path& modulePath,
                                                     UEModuleLoadPhase phase) {
    UEModuleLoadResult result;
    
    // Extract module name from path
    std::string moduleName = modulePath.stem().string();
    
    // Create module info
    UEModuleInfo moduleInfo;
    moduleInfo.name = moduleName;
    moduleInfo.modulePath = modulePath;
    moduleInfo.type = UEModuleType::ThirdParty;
    moduleInfo.loadPhase = phase;
    
    // Register module
    if (!registerModule(moduleInfo)) {
        result.errors.push_back("Failed to register module: " + moduleName);
        return result;
    }
    
    // Load the module
    return loadModule(moduleName, phase);
}

bool UEModuleLoader::unloadModule(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Unloading module: " + moduleName);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        DiagnosticEngine::get().warning("Module not loaded: " + moduleName);
        return false;
    }
    
    ModuleState& state = moduleIt->second;
    
    // Execute finalizers
    if (!executeModuleFinalizers(state)) {
        DiagnosticEngine::get().warning("Module finalization failed: " + moduleName);
    }
    
    // Unregister symbols
    if (!unregisterModuleSymbols(state)) {
        DiagnosticEngine::get().warning("Failed to unregister symbols: " + moduleName);
    }
    
    // Unload binary
    if (state.handle) {
        if (!unloadModuleBinary(state.handle)) {
            DiagnosticEngine::get().warning("Failed to unload module binary: " + moduleName);
        }
        handleToModuleMap_.erase(state.handle);
    }
    
    // Remove from phase
    for (auto& [phase, modules] : phaseModules_) {
        modules.erase(std::remove(modules.begin(), modules.end(), moduleName), modules.end());
    }
    
    // Remove module
    modules_.erase(moduleIt);
    
    DiagnosticEngine::get().info("Successfully unloaded module: " + moduleName);
    return true;
}

bool UEModuleLoader::reloadModule(const std::string& moduleName) {
    DiagnosticEngine::get().info("Reloading module: " + moduleName);
    
    // Get module info before unloading
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        DiagnosticEngine::get().error("Module not loaded: " + moduleName);
        return false;
    }
    
    UEModuleLoadPhase phase = moduleIt->second.info.loadPhase;
    
    // Unload module
    if (!unloadModule(moduleName)) {
        return false;
    }
    
    // Load module again
    auto result = loadModule(moduleName, phase);
    return result.success;
}

bool UEModuleLoader::registerModule(const UEModuleInfo& moduleInfo) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!moduleInfo.isValid()) {
        DiagnosticEngine::get().error("Invalid module info");
        return false;
    }
    
    if (modules_.find(moduleInfo.name) != modules_.end()) {
        DiagnosticEngine::get().warning("Module already registered: " + moduleInfo.name);
        return false;
    }
    
    // Create module state (not loaded yet)
    ModuleState state;
    state.info = moduleInfo;
    state.isLoaded = false;
    state.handle = nullptr;
    
    modules_[moduleInfo.name] = std::move(state);
    
    DiagnosticEngine::get().info("Registered module: " + moduleInfo.name);
    return true;
}

bool UEModuleLoader::unregisterModule(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        DiagnosticEngine::get().warning("Module not registered: " + moduleName);
        return false;
    }
    
    // If module is loaded, unload it first
    if (moduleIt->second.isLoaded) {
        if (!unloadModule(moduleName)) {
            DiagnosticEngine::get().error("Failed to unload module before unregistering: " + moduleName);
            return false;
        }
    }
    
    modules_.erase(moduleIt);
    
    DiagnosticEngine::get().info("Unregistered module: " + moduleName);
    return true;
}

const UEModuleInfo* UEModuleLoader::findModule(const std::string& moduleName) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return &it->second.info;
    }
    return nullptr;
}

std::vector<std::string> UEModuleLoader::getLoadedModules() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> loadedModules;
    
    for (const auto& [name, state] : modules_) {
        if (state.isLoaded) {
            loadedModules.push_back(name);
        }
    }
    
    return loadedModules;
}

std::vector<std::string> UEModuleLoader::getModulesByPhase(UEModuleLoadPhase phase) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = phaseModules_.find(phase);
    if (it != phaseModules_.end()) {
        return it->second;
    }
    return {};
}

bool UEModuleLoader::loadAllModules(UEModuleLoadPhase upToPhase) {
    DiagnosticEngine::get().info("Loading all modules up to phase: " + 
        std::to_string(static_cast<int>(upToPhase)));
    
    bool success = true;
    
    for (int phase = static_cast<int>(UEModuleLoadPhase::PreEngineInit);
         phase <= static_cast<int>(upToPhase);
         ++phase) {
        
        UEModuleLoadPhase currentPhase = static_cast<UEModuleLoadPhase>(phase);
        
        // Get modules for this phase
        std::vector<std::string> phaseModuleNames;
        for (const auto& [name, state] : modules_) {
            if (state.info.loadPhase == currentPhase && !state.isLoaded) {
                phaseModuleNames.push_back(name);
            }
        }
        
        // Load modules in this phase
        for (const auto& moduleName : phaseModuleNames) {
            auto result = loadModule(moduleName, currentPhase);
            if (!result.success) {
                DiagnosticEngine::get().error("Failed to load module: " + moduleName);
                success = false;
            }
        }
    }
    
    return success;
}

bool UEModuleLoader::unloadAllModules() {
    DiagnosticEngine::get().info("Unloading all modules");
    
    std::vector<std::string> loadedModules = getLoadedModules();
    bool success = true;
    
    // Unload in reverse order of loading (LIFO)
    for (auto it = loadedModules.rbegin(); it != loadedModules.rend(); ++it) {
        if (!unloadModule(*it)) {
            DiagnosticEngine::get().error("Failed to unload module: " + *it);
            success = false;
        }
    }
    
    return success;
}

bool UEModuleLoader::registerSymbol(const std::string& moduleName,
                                   const std::string& symbolName,
                                   void* address) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        DiagnosticEngine::get().error("Module not found: " + moduleName);
        return false;
    }
    
    ModuleState& state = moduleIt->second;
    state.symbols[symbolName] = address;
    globalSymbols_[symbolName] = address;
    
    DiagnosticEngine::get().info("Registered symbol: " + symbolName + " in module: " + moduleName);
    return true;
}

void* UEModuleLoader::findSymbol(const std::string& symbolName) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = globalSymbols_.find(symbolName);
    if (it != globalSymbols_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void* UEModuleLoader::findSymbolInModule(const std::string& moduleName,
                                        const std::string& symbolName) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        return nullptr;
    }
    
    const ModuleState& state = moduleIt->second;
    auto symbolIt = state.symbols.find(symbolName);
    if (symbolIt != state.symbols.end()) {
        return symbolIt->second;
    }
    
    return nullptr;
}

bool UEModuleLoader::generateModuleStubs(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        DiagnosticEngine::get().error("Module not found: " + moduleName);
        return false;
    }
    
    const UEModuleInfo& info = moduleIt->second.info;
    
    DiagnosticEngine::get().info("Generating stubs for module: " + moduleName);
    
    // Generate header stub
    if (!generateModuleHeaderStub(info)) {
        DiagnosticEngine::get().error("Failed to generate header stub for: " + moduleName);
        return false;
    }
    
    // Generate source stub
    if (!generateModuleSourceStub(info)) {
        DiagnosticEngine::get().error("Failed to generate source stub for: " + moduleName);
        return false;
    }
    
    // Generate build file
    if (!generateModuleBuildFile(info)) {
        DiagnosticEngine::get().error("Failed to generate build file for: " + moduleName);
        return false;
    }
    
    return true;
}

bool UEModuleLoader::generateAllStubs() {
    DiagnosticEngine::get().info("Generating stubs for all modules");
    
    bool success = true;
    
    for (const auto& [moduleName, state] : modules_) {
        if (!generateModuleStubs(moduleName)) {
            DiagnosticEngine::get().error("Failed to generate stubs for: " + moduleName);
            success = false;
        }
    }
    
    return success;
}

bool UEModuleLoader::validateModuleDependencies() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    bool valid = true;
    
    for (const auto& [moduleName, state] : modules_) {
        // Check for circular dependencies
        std::unordered_set<std::string> visited;
        std::vector<std::string> cycle;
        
        collectCyclicDependencies(moduleName, visited, cycle);
        
        if (!cycle.empty()) {
            DiagnosticEngine::get().error("Circular dependency detected involving: " + moduleName);
            valid = false;
        }
    }
    
    return valid;
}

bool UEModuleLoader::checkModuleCompatibility(const UEModuleInfo& moduleInfo) const {
    // Check engine version compatibility
    if (moduleInfo.version > 0) {
        // Simple version check - in reality would check against engine version
        if (moduleInfo.version < 50000) { // UE5 minimum
            DiagnosticEngine::get().warning("Module version may be incompatible: " + moduleInfo.name);
        }
    }
    
    // Check platform compatibility
    if (!moduleInfo.isGameModule && config_.targetPlatform == UEPlatform::Win64) {
        // Most modules support Windows
    }
    
    return true;
}

bool UEModuleLoader::loadModuleDependencies(ModuleState& state,
                                           std::vector<std::string>& loadOrder) {
    // Resolve all dependencies
    std::vector<std::string> dependencies;
    if (!resolveModuleDependencies(state.info, dependencies)) {
        return false;
    }
    
    // Load dependencies first
    for (const auto& depName : dependencies) {
        // Skip if already in load order
        if (std::find(loadOrder.begin(), loadOrder.end(), depName) != loadOrder.end()) {
            continue;
        }
        
        // Check if dependency is already loaded
        auto depIt = modules_.find(depName);
        if (depIt != modules_.end() && depIt->second.isLoaded) {
            loadOrder.push_back(depName);
            continue;
        }
        
        // Load dependency
        auto result = loadModule(depName, state.info.loadPhase);
        if (!result.success) {
            DiagnosticEngine::get().error("Failed to load dependency: " + depName + 
                                         " for module: " + state.info.name);
            return false;
        }
        
        loadOrder.push_back(depName);
    }
    
    // Add this module to load order
    loadOrder.push_back(state.info.name);
    state.dependencyChain = loadOrder;
    
    return true;
}

bool UEModuleLoader::resolveModuleDependencies(const UEModuleInfo& info,
                                              std::vector<std::string>& dependencies) const {
    // Add direct dependencies
    dependencies.insert(dependencies.end(), 
                       info.dependencies.begin(), info.dependencies.end());
    
    // Resolve transitive dependencies
    std::unordered_set<std::string> visited;
    std::vector<std::string> toProcess = info.dependencies;
    
    while (!toProcess.empty()) {
        std::string current = toProcess.back();
        toProcess.pop_back();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        
        visited.insert(current);
        
        // Find module and its dependencies
        auto moduleIt = modules_.find(current);
        if (moduleIt != modules_.end()) {
            for (const auto& dep : moduleIt->second.info.dependencies) {
                if (visited.find(dep) == visited.end()) {
                    toProcess.push_back(dep);
                    dependencies.push_back(dep);
                }
            }
        }
    }
    
    return true;
}

void* UEModuleLoader::loadModuleBinary(const path& binaryPath,
                                      std::vector<std::string>& errors) {
    DiagnosticEngine::get().info("Loading binary: " + binaryPath.string());
    
    // Use platform-specific dynamic loading
#ifdef _WIN32
    HMODULE handle = LoadLibraryW(binaryPath.wstring().c_str());
    if (!handle) {
        DWORD error = GetLastError();
        errors.push_back("Failed to load library: " + binaryPath.string() + 
                        ", error: " + std::to_string(error));
        return nullptr;
    }
    return handle;
#else
    void* handle = dlopen(binaryPath.string().c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char* dlerror_msg = dlerror();
        errors.push_back("Failed to load library: " + binaryPath.string() + 
                        ", error: " + (dlerror_msg ? dlerror_msg : "unknown"));
        return nullptr;
    }
    return handle;
#endif
}

bool UEModuleLoader::unloadModuleBinary(void* handle) {
    if (!handle) {
        return false;
    }
    
#ifdef _WIN32
    return FreeLibrary(static_cast<HMODULE>(handle)) != 0;
#else
    return dlclose(handle) == 0;
#endif
}

bool UEModuleLoader::registerModuleSymbols(ModuleState& state) {
    if (!state.handle) {
        return false;
    }
    
    // Look for registration function
#ifdef _WIN32
    FARPROC registerFunc = GetProcAddress(static_cast<HMODULE>(state.handle), 
                                         "RegisterModuleExports");
#else
    void* registerFunc = dlsym(state.handle, "RegisterModuleExports");
#endif
    
    if (registerFunc) {
        // Call registration function
        using RegisterFunc = void(*)(void*);
        reinterpret_cast<RegisterFunc>(registerFunc)(this);
    } else {
        // Try to auto-discover symbols
        registerCommonSymbols(state);
    }
    
    return true;
}

bool UEModuleLoader::unregisterModuleSymbols(ModuleState& state) {
    // Remove all symbols from this module from global symbol table
    for (const auto& [symbolName, _] : state.symbols) {
        globalSymbols_.erase(symbolName);
    }
    
    state.symbols.clear();
    return true;
}

bool UEModuleLoader::executeModuleInitializers(ModuleState& state) {
    // Look for initialization function
#ifdef _WIN32
    FARPROC initFunc = GetProcAddress(static_cast<HMODULE>(state.handle), 
                                     "InitializeModule");
#else
    void* initFunc = dlsym(state.handle, "InitializeModule");
#endif
    
    if (initFunc) {
        using InitFunc = bool(*)(void*);
        bool success = reinterpret_cast<InitFunc>(initFunc)(this);
        if (!success) {
            DiagnosticEngine::get().error("Module initialization failed: " + state.info.name);
            return false;
        }
    }
    
    return true;
}

bool UEModuleLoader::executeModuleFinalizers(ModuleState& state) {
    // Look for finalization function
#ifdef _WIN32
    FARPROC finiFunc = GetProcAddress(static_cast<HMODULE>(state.handle), 
                                     "ShutdownModule");
#else
    void* finiFunc = dlsym(state.handle, "ShutdownModule");
#endif
    
    if (finiFunc) {
        using FiniFunc = bool(*)(void*);
        bool success = reinterpret_cast<FiniFunc>(finiFunc)(this);
        if (!success) {
            DiagnosticEngine::get().warning("Module finalization failed: " + state.info.name);
        }
    }
    
    return true;
}

bool UEModuleLoader::generateModuleHeaderStub(const UEModuleInfo& info) const {
    if (!projectBuilder_) {
        return false;
    }
    
    path headerPath = info.modulePath / "Public" / (info.name + ".h");
    
    std::string content = "#pragma once\n\n";
    content += "#include \"CoreMinimal.h\"\n";
    content += "#include \"Modules/ModuleManager.h\"\n\n";
    content += "DECLARE_LOG_CATEGORY_EXTERN(Log" + info.name + ", Log, All);\n\n";
    content += "class F" + info.name + "Module : public IModuleInterface\n";
    content += "{\n";
    content += "public:\n";
    content += "    virtual void StartupModule() override;\n";
    content += "    virtual void ShutdownModule() override;\n";
    content += "};\n";
    
    return projectBuilder_->addGeneratedFile(headerPath, content, info.name, true);
}

bool UEModuleLoader::generateModuleSourceStub(const UEModuleInfo& info) const {
    if (!projectBuilder_) {
        return false;
    }
    
    path sourcePath = info.modulePath / "Private" / (info.name + ".cpp");
    
    std::string content = "#include \"" + info.name + ".h\"\n\n";
    content += "DEFINE_LOG_CATEGORY(Log" + info.name + ");\n\n";
    content += "#define LOCTEXT_NAMESPACE \"F" + info.name + "Module\"\n\n";
    content += "void F" + info.name + "Module::StartupModule()\n";
    content += "{\n";
    content += "    UE_LOG(Log" + info.name + ", Log, TEXT(\"" + info.name + " module starting up\"));\n";
    content += "}\n\n";
    content += "void F" + info.name + "Module::ShutdownModule()\n";
    content += "{\n";
    content += "    UE_LOG(Log" + info.name + ", Log, TEXT(\"" + info.name + " module shutting down\"));\n";
    content += "}\n\n";
    content += "#undef LOCTEXT_NAMESPACE\n\n";
    content += "IMPLEMENT_MODULE(F" + info.name + "Module, " + info.name + ");\n";
    
    return projectBuilder_->addGeneratedFile(sourcePath, content, info.name, false);
}

bool UEModuleLoader::generateModuleBuildFile(const UEModuleInfo& info) const {
    if (!projectBuilder_) {
        return false;
    }
    
    path buildCsPath = info.modulePath / (info.name + ".Build.cs");
    
    UEModuleBuildInfo buildInfo;
    buildInfo.name = info.name;
    buildInfo.type = info.isGameModule ? "Runtime" : "Developer";
    buildInfo.publicDependencyModuleNames = {"Core"};
    
    if (info.isEditorModule) {
        buildInfo.publicDependencyModuleNames.push_back("UnrealEd");
    }
    
    return projectBuilder_->addGeneratedFile(buildCsPath, 
                                            buildInfo.generateBuildCsContent(), 
                                            info.name, 
                                            false);
}

path UEModuleLoader::findModuleBinary(const UEModuleInfo& info,
                                     UEPlatform platform) const {
    // Try common binary locations
    std::vector<path> searchPaths = getModuleSearchPaths();
    
    for (const auto& searchPath : searchPaths) {
        path binaryPath = searchPath / info.name;
        
        // Add platform-specific extension
#ifdef _WIN32
        binaryPath += ".dll";
#elif __APPLE__
        binaryPath += ".dylib";
#else
        binaryPath += ".so";
#endif
        
        if (exists(binaryPath)) {
            return binaryPath;
        }
    }
    
    return {};
}

std::vector<path> UEModuleLoader::getModuleSearchPaths() const {
    std::vector<path> searchPaths;
    
    // Engine binaries
    path engineRoot = path(config_.projectPath).parent_path().parent_path().parent_path();
    searchPaths.push_back(engineRoot / "Engine" / "Binaries" / "ThirdParty");
    
    // Game binaries
    searchPaths.push_back(path(config_.projectPath) / "Binaries");
    
    // Platform-specific binaries
    switch (config_.targetPlatform) {
        case UEPlatform::Win64:
            searchPaths.push_back(engineRoot / "Engine" / "Binaries" / "Win64");
            break;
        case UEPlatform::Linux:
            searchPaths.push_back(engineRoot / "Engine" / "Binaries" / "Linux");
            break;
        case UEPlatform::Mac:
            searchPaths.push_back(engineRoot / "Engine" / "Binaries" / "Mac");
            break;
        default:
            break;
    }
    
    return searchPaths;
}

bool UEModuleLoader::checkModuleVersion(const UEModuleInfo& info) const {
    // Simple version check
    return info.version >= 50000; // UE5 compatible
}

bool UEModuleLoader::verifyModuleSignature(const path& binaryPath) const {
    
    
    if (!exists(binaryPath)) {
        return false;
    }
    
    auto fileSize = file_size(binaryPath);
    if (fileSize < 1024 || fileSize > 100 * 1024 * 1024) { // 1KB to 100MB
        DiagnosticEngine::get().warning("Suspicious module size: " + binaryPath.string());
        return false;
    }
    
    return true;
}

void UEModuleLoader::collectCyclicDependencies(const std::string& moduleName,
                                              std::unordered_set<std::string>& visited,
                                              std::vector<std::string>& cycle) const {
    if (visited.find(moduleName) != visited.end()) {
        // Found a cycle
        cycle.push_back(moduleName);
        return;
    }
    
    visited.insert(moduleName);
    
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt != modules_.end()) {
        for (const auto& dep : moduleIt->second.info.dependencies) {
            collectCyclicDependencies(dep, visited, cycle);
            if (!cycle.empty()) {
                cycle.push_back(moduleName);
                return;
            }
        }
    }
    
    visited.erase(moduleName);
}

bool UEModuleLoader::loadCoreModules() {
    // Load essential UE modules
    std::vector<std::pair<std::string, UEModuleLoadPhase>> coreModules = {
        {"Core", UEModuleLoadPhase::PreEngineInit},
        {"CoreUObject", UEModuleLoadPhase::PreEngineInit},
        {"Engine", UEModuleLoadPhase::PostEngineInit}
    };
    
    bool success = true;
    
    for (const auto& [moduleName, phase] : coreModules) {
        auto result = loadModule(moduleName, phase);
        if (!result.success) {
            DiagnosticEngine::get().error("Failed to load core module: " + moduleName);
            success = false;
        }
    }
    
    return success;
}

void UEModuleLoader::registerCommonSymbols(ModuleState& state) {
    // Auto-discover common export patterns
    const char* commonSymbols[] = {
        "GetModule",
        "InitializeModule",
        "ShutdownModule",
        "RegisterExports",
        "GetExports"
    };
    
    for (const char* symbolName : commonSymbols) {
#ifdef _WIN32
        FARPROC address = GetProcAddress(static_cast<HMODULE>(state.handle), symbolName);
#else
        void* address = dlsym(state.handle, symbolName);
#endif
        
        if (address) {
            registerSymbol(state.info.name, symbolName, reinterpret_cast<void*>(address));
        }
    }
}

} // namespace Unreal
} // namespace kiwiLang