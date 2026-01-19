#pragma once

#include "kiwiLang/Unreal/UnrealEngine.h"
#include "kiwiLang/AST/Module.h"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UObjectRegistry;
class UEProjectBuilder;

enum class UEModuleType {
    Runtime,
    Editor,
    Developer,
    ThirdParty,
    Plugin,
    Generated
};

enum class UEModuleLoadPhase {
    PreEngineInit,
    PostEngineInit,
    PostDefaultModule,
    PreLoadingScreen,
    PostLoadingScreen,
    PreBeginPlay,
    PostBeginPlay
};

struct UEModuleInfo {
    std::string name;
    UEModuleType type;
    std::filesystem::path modulePath;
    std::filesystem::path binaryPath;
    std::vector<std::string> dependencies;
    std::vector<std::string> publicHeaders;
    std::vector<std::string> privateHeaders;
    std::vector<std::string> sourceFiles;
    UEModuleLoadPhase loadPhase;
    bool isHotReloadable;
    bool isGameModule;
    bool isEditorModule;
    std::uint32_t version;
    
    bool isValid() const;
    bool isLoaded() const;
    std::string getModuleMacro() const;
};

struct UEModuleLoadResult {
    bool success;
    std::string moduleName;
    void* moduleHandle;
    std::vector<std::string> loadedSymbols;
    std::vector<std::string> errors;
    std::chrono::milliseconds loadTime;
    
    bool hasErrors() const { return !errors.empty(); }
};

class KIWI_UNREAL_API UEModuleLoader {
public:
    explicit UEModuleLoader(const UEConfig& config,
                           UETypeRegistry* typeRegistry = nullptr,
                           UObjectRegistry* objectRegistry = nullptr,
                           UEProjectBuilder* projectBuilder = nullptr);
    ~UEModuleLoader();
    
    bool initialize();
    bool shutdown();
    
    UEModuleLoadResult loadModule(const std::string& moduleName,
                                 UEModuleLoadPhase phase = UEModuleLoadPhase::PostEngineInit);
    
    UEModuleLoadResult loadModuleFromPath(const std::filesystem::path& modulePath,
                                         UEModuleLoadPhase phase = UEModuleLoadPhase::PostEngineInit);
    
    bool unloadModule(const std::string& moduleName);
    bool reloadModule(const std::string& moduleName);
    
    bool registerModule(const UEModuleInfo& moduleInfo);
    bool unregisterModule(const std::string& moduleName);
    
    const UEModuleInfo* findModule(const std::string& moduleName) const;
    std::vector<std::string> getLoadedModules() const;
    std::vector<std::string> getModulesByPhase(UEModuleLoadPhase phase) const;
    
    bool loadAllModules(UEModuleLoadPhase upToPhase = UEModuleLoadPhase::PostBeginPlay);
    bool unloadAllModules();
    
    bool registerSymbol(const std::string& moduleName,
                       const std::string& symbolName,
                       void* address);
    
    void* findSymbol(const std::string& symbolName) const;
    void* findSymbolInModule(const std::string& moduleName,
                            const std::string& symbolName) const;
    
    bool generateModuleStubs(const std::string& moduleName);
    bool generateAllStubs();
    
    bool validateModuleDependencies() const;
    bool checkModuleCompatibility(const UEModuleInfo& moduleInfo) const;
    
    const UEConfig& getConfig() const { return config_; }
    
private:
    struct ModuleState {
        UEModuleInfo info;
        void* handle;
        bool isLoaded;
        std::chrono::system_clock::time_point loadTime;
        std::unordered_map<std::string, void*> symbols;
        std::vector<std::string> dependencyChain;
    };
    
    UEConfig config_;
    UETypeRegistry* typeRegistry_;
    UObjectRegistry* objectRegistry_;
    UEProjectBuilder* projectBuilder_;
    
    std::unordered_map<std::string, ModuleState> modules_;
    std::unordered_map<UEModuleLoadPhase, std::vector<std::string>> phaseModules_;
    std::unordered_map<void*, std::string> handleToModuleMap_;
    std::unordered_map<std::string, void*> globalSymbols_;
    
    mutable std::recursive_mutex mutex_;
    
    bool loadModuleDependencies(ModuleState& state,
                               std::vector<std::string>& loadOrder);
    bool resolveModuleDependencies(const UEModuleInfo& info,
                                  std::vector<std::string>& dependencies) const;
    
    void* loadModuleBinary(const std::filesystem::path& binaryPath,
                          std::vector<std::string>& errors);
    bool unloadModuleBinary(void* handle);
    
    bool registerModuleSymbols(ModuleState& state);
    bool unregisterModuleSymbols(ModuleState& state);
    
    bool executeModuleInitializers(ModuleState& state);
    bool executeModuleFinalizers(ModuleState& state);
    
    bool generateModuleHeaderStub(const UEModuleInfo& info) const;
    bool generateModuleSourceStub(const UEModuleInfo& info) const;
    bool generateModuleBuildFile(const UEModuleInfo& info) const;
    
    std::filesystem::path findModuleBinary(const UEModuleInfo& info,
                                          UEPlatform platform) const;
    std::vector<std::filesystem::path> getModuleSearchPaths() const;
    
    bool checkModuleVersion(const UEModuleInfo& info) const;
    bool verifyModuleSignature(const std::filesystem::path& binaryPath) const;
    
    void collectCyclicDependencies(const std::string& moduleName,
                                  std::unordered_set<std::string>& visited,
                                  std::vector<std::string>& cycle) const;
};

} // namespace Unreal
} // namespace kiwiLang