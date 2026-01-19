#pragma once

#include "UnrealEngine.h"
#include <string>
#include <vector>
#include <filesystem>

namespace kiwiLang {
namespace Unreal {

struct UEModuleConfig {
    std::string name;
    std::string type; // Runtime, Editor, Developer, etc.
    std::vector<std::string> publicDependencies;
    std::vector<std::string> privateDependencies;
    std::vector<std::string> publicIncludePaths;
    bool loadAutomatically;
    
    bool validate() const;
    std::string generateBuildCsContent() const;
};

struct UEPluginConfig {
    std::string name;
    std::string version;
    std::string description;
    std::vector<UEModuleConfig> modules;
    std::vector<std::string> supportedTargetPlatforms;
    std::vector<std::string> supportedTargetConfigurations;
    
    bool validate() const;
    bool saveToFile(const std::filesystem::path& pluginDir) const;
    static UEPluginConfig loadFromFile(const std::filesystem::path& upluginFile);
};

struct UECompileSettings {
    bool optimizeForSize;
    bool enableDebugSymbols;
    bool enableLTO;
    bool enablePCH;
    std::vector<std::string> additionalCompilerFlags;
    std::vector<std::string> additionalLinkerFlags;
    std::string optimizationLevel; // "Debug", "Development", "Shipping", "Test"
    
    std::vector<std::string> getCompilerFlags(UEPlatform platform) const;
    std::vector<std::string> getLinkerFlags(UEPlatform platform) const;
};

struct UEHeaderToolConfig {
    bool generateUHTCompatibleHeaders;
    bool generateReflectionData;
    bool supportBlueprintType;
    bool supportInstancedType;
    std::vector<std::string> additionalUHTArguments;
    
    std::vector<std::string> getUHTArguments() const;
};

struct UERuntimeConfig {
    bool enableGarbageCollection;
    bool enableHotReload;
    bool enableAsyncLoading;
    size_t maxMemoryUsageMB;
    size_t gcCollectionInterval;
    
    static UERuntimeConfig getDefault();
};

class KIWI_UNREAL_API UEConfigManager {
public:
    explicit UEConfigManager(const std::filesystem::path& projectRoot);
    
    bool loadConfig();
    bool saveConfig();
    bool validateConfig() const;
    
    UEConfig& getEngineConfig() { return engineConfig_; }
    const UEConfig& getEngineConfig() const { return engineConfig_; }
    
    UEPluginConfig& getPluginConfig() { return pluginConfig_; }
    const UEPluginConfig& getPluginConfig() const { return pluginConfig_; }
    
    UECompileSettings& getCompileSettings() { return compileSettings_; }
    const UECompileSettings& getCompileSettings() const { return compileSettings_; }
    
    UEHeaderToolConfig& getHeaderToolConfig() { return headerToolConfig_; }
    const UEHeaderToolConfig& getHeaderToolConfig() const { return headerToolConfig_; }
    
    UERuntimeConfig& getRuntimeConfig() { return runtimeConfig_; }
    const UERuntimeConfig& getRuntimeConfig() const { return runtimeConfig_; }
    
    std::filesystem::path getGeneratedHeadersPath() const;
    std::filesystem::path getIntermediatePath() const;
    std::filesystem::path getBinariesPath(UEPlatform platform) const;
    std::filesystem::path getBuildOutputPath(UEPlatform platform) const;
    
    bool generateProjectFiles();
    bool generatePluginFiles();
    
private:
    std::filesystem::path projectRoot_;
    UEConfig engineConfig_;
    UEPluginConfig pluginConfig_;
    UECompileSettings compileSettings_;
    UEHeaderToolConfig headerToolConfig_;
    UERuntimeConfig runtimeConfig_;
    
    bool loadEngineConfigFromJson(const std::filesystem::path& configFile);
    bool loadPluginConfigFromJson(const std::filesystem::path& configFile);
    bool saveEngineConfigToJson(const std::filesystem::path& configFile) const;
    bool savePluginConfigToJson(const std::filesystem::path& configFile) const;
    
    std::filesystem::path findUProjectFile() const;
    std::filesystem::path findPluginDescriptor() const;
};

} // namespace Unreal
} // namespace kiwiLang