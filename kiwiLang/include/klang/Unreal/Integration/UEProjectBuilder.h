#pragma once

#include "kiwiLang/Unreal/UnrealEngine.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UObjectRegistry;
struct UECompileSettings;

enum class UEProjectType {
    Game,
    Editor,
    Server,
    Client,
    Program,
    Plugin
};

enum class UEBuildConfiguration {
    Debug,
    DebugGame,
    Development,
    Shipping,
    Test
};

enum class UEBuildTarget {
    Game,
    Editor,
    Server,
    Client,
    Program
};

struct UEProjectFile {
    std::filesystem::path sourcePath;
    std::filesystem::path generatedPath;
    std::string moduleName;
    std::vector<std::string> dependencies;
    bool isPublicAPI;
    bool isGenerated;
    std::chrono::system_clock::time_point lastModified;
    
    bool needsRegeneration() const;
    std::string getIncludePath() const;
};

struct UEModuleBuildInfo {
    std::string name;
    UEProjectType type;
    std::vector<std::string> publicDependencyModuleNames;
    std::vector<std::string> privateDependencyModuleNames;
    std::vector<std::filesystem::path> publicIncludePaths;
    std::vector<std::filesystem::path> privateIncludePaths;
    std::vector<std::string> publicDefinitions;
    std::vector<std::string> privateDefinitions;
    std::vector<std::filesystem::path> additionalLibraries;
    
    std::string generateBuildCsContent() const;
    bool validate() const;
};

struct UEBuildResult {
    bool success;
    std::chrono::milliseconds buildTime;
    std::vector<std::string> outputFiles;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::uint32_t exitCode;
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
};

class KIWI_UNREAL_API UEProjectBuilder {
public:
    explicit UEProjectBuilder(const UEConfig& config,
                             UETypeRegistry* typeRegistry = nullptr,
                             UObjectRegistry* objectRegistry = nullptr);
    ~UEProjectBuilder();
    
    bool initialize();
    bool shutdown();
    
    UEBuildResult buildProject(UEBuildTarget target,
                              UEBuildConfiguration configuration,
                              UEPlatform platform);
    
    UEBuildResult buildPlugin(const std::string& pluginName,
                             UEBuildConfiguration configuration,
                             UEPlatform platform);
    
    bool generateProjectFiles();
    bool generateModuleFiles(const std::string& moduleName);
    bool generatePluginFiles(const std::string& pluginName);
    
    bool addSourceFile(const std::filesystem::path& sourcePath,
                      const std::string& moduleName,
                      bool isPublicAPI = false);
    
    bool addGeneratedFile(const std::filesystem::path& generatedPath,
                         const std::string& content,
                         const std::string& moduleName,
                         bool isPublicAPI = false);
    
    bool registerModule(const UEModuleBuildInfo& moduleInfo);
    bool unregisterModule(const std::string& moduleName);
    
    std::vector<std::string> getRegisteredModules() const;
    std::vector<UEProjectFile> getModuleFiles(const std::string& moduleName) const;
    
    bool cleanBuildArtifacts();
    bool cleanGeneratedFiles();
    
    const UEConfig& getConfig() const { return config_; }
    bool isInitialized() const { return initialized_; }
    
private:
    UEConfig config_;
    UETypeRegistry* typeRegistry_;
    UObjectRegistry* objectRegistry_;
    bool initialized_;
    
    std::unordered_map<std::string, UEModuleBuildInfo> modules_;
    std::unordered_map<std::string, std::vector<UEProjectFile>> moduleFiles_;
    std::filesystem::path intermediateDir_;
    std::filesystem::path binariesDir_;
    std::filesystem::path generatedDir_;
    
    std::unique_ptr<class UEBuildProcess> buildProcess_;
    std::unique_ptr<class UEFileGenerator> fileGenerator_;
    
    bool setupDirectories();
    bool loadExistingModules();
    bool saveModuleRegistry();
    
    std::filesystem::path getModulePath(const std::string& moduleName) const;
    std::filesystem::path getModuleBuildCsPath(const std::string& moduleName) const;
    std::filesystem::path getModuleIntermediatePath(const std::string& moduleName) const;
    
    UEBuildResult executeBuildCommand(const std::vector<std::string>& args,
                                     const std::filesystem::path& workingDir);
    
    bool generateBuildCsFile(const UEModuleBuildInfo& moduleInfo);
    bool generateTargetCsFiles();
    bool generateUProjectFile();
    bool generateUPluginFile();
    
    bool compileGeneratedCode(const std::string& moduleName);
    bool linkModule(const std::string& moduleName,
                   UEBuildConfiguration configuration,
                   UEPlatform platform);
    
    void collectBuildDependencies(const std::string& moduleName,
                                 std::vector<std::string>& dependencies) const;
    void validateModuleDependencies() const;
};

} // namespace Unreal
} // namespace kiwiLang