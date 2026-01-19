#include "kiwiLang/Unreal/UEConfig.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

namespace kiwiLang {
namespace Unreal {

using namespace std::filesystem;

std::string UEVersion::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

UEConfig UEConfig::loadFromProject(const std::string& uprojectPath) {
    UEConfig config;
    config.projectPath = path(uprojectPath).parent_path().string();
    
    // Parse .uproject file
    std::ifstream file(uprojectPath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open .uproject file: " + uprojectPath);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    // Simple JSON parsing for project name
    std::regex nameRegex(R"(\"ProjectName\"\s*:\s*\"([^\"]+)\")");
    std::smatch match;
    if (std::regex_search(content, match, nameRegex)) {
        config.projectName = match[1];
    }
    
    // Try to detect engine version
    path engineRoot = path(uprojectPath).parent_path().parent_path().parent_path();
    path engineVersionFile = engineRoot / "Engine" / "Build" / "Build.version";
    
    if (exists(engineVersionFile)) {
        std::ifstream versionFile(engineVersionFile);
        std::string versionContent((std::istreambuf_iterator<char>(versionFile)),
                                  std::istreambuf_iterator<char>());
        
        std::regex versionRegex(R"(\"MajorVersion\"\s*:\s*(\d+).*\"MinorVersion\"\s*:\s*(\d+).*\"PatchVersion\"\s*:\s*(\d+))");
        if (std::regex_search(versionContent, match, versionRegex)) {
            config.engineVersion.major = std::stoi(match[1]);
            config.engineVersion.minor = std::stoi(match[2]);
            config.engineVersion.patch = std::stoi(match[3]);
        }
    }
    
    config.intermediatePath = (path(config.projectPath) / "Intermediate" / "kiwiLang").string();
    config.enableHotReload = true;
    config.enableBlueprintIntegration = true;
    config.generateStubs = true;
    config.targetPlatform = UEPlatform::Win64;
    
    return config;
}

bool UEConfig::validate() const {
    if (projectName.empty()) {
        DiagnosticEngine::get().error("Project name cannot be empty");
        return false;
    }
    
    if (projectPath.empty() || !exists(projectPath)) {
        DiagnosticEngine::get().error("Project path does not exist: " + projectPath);
        return false;
    }
    
    if (engineVersion.major == 0) {
        DiagnosticEngine::get().warning("Engine version not detected, using default");
    }
    
    return true;
}

bool UEModuleConfig::validate() const {
    if (name.empty()) {
        DiagnosticEngine::get().error("Module name cannot be empty");
        return false;
    }
    
    if (type.empty()) {
        DiagnosticEngine::get().error("Module type cannot be empty");
        return false;
    }
    
    return true;
}

std::string UEModuleConfig::generateBuildCsContent() const {
    std::ostringstream oss;
    
    oss << "using UnrealBuildTool;\n\n";
    oss << "public class " << name << " : ModuleRules\n";
    oss << "{\n";
    oss << "    public " << name << "(ReadOnlyTargetRules Target) : base(Target)\n";
    oss << "    {\n";
    oss << "        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;\n\n";
    
    oss << "        PublicDependencyModuleNames.AddRange(new string[] {\n";
    for (size_t i = 0; i < publicDependencies.size(); ++i) {
        oss << "            \"" << publicDependencies[i] << "\"";
        if (i != publicDependencies.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "        });\n\n";
    
    oss << "        PrivateDependencyModuleNames.AddRange(new string[] {\n";
    for (size_t i = 0; i < privateDependencies.size(); ++i) {
        oss << "            \"" << privateDependencies[i] << "\"";
        if (i != privateDependencies.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "        });\n\n";
    
    if (!publicIncludePaths.empty()) {
        oss << "        PublicIncludePaths.AddRange(new string[] {\n";
        for (size_t i = 0; i < publicIncludePaths.size(); ++i) {
            oss << "            \"" << publicIncludePaths[i] << "\"";
            if (i != publicIncludePaths.size() - 1) oss << ",";
            oss << "\n";
        }
        oss << "        });\n";
    }
    
    oss << "    }\n";
    oss << "}\n";
    
    return oss.str();
}

bool UEPluginConfig::validate() const {
    if (name.empty()) {
        DiagnosticEngine::get().error("Plugin name cannot be empty");
        return false;
    }
    
    if (version.empty()) {
        DiagnosticEngine::get().error("Plugin version cannot be empty");
        return false;
    }
    
    for (const auto& module : modules) {
        if (!module.validate()) {
            return false;
        }
    }
    
    return true;
}

bool UEPluginConfig::saveToFile(const path& pluginDir) const {
    if (!exists(pluginDir)) {
        create_directories(pluginDir);
    }
    
    path upluginFile = pluginDir / (name + ".uplugin");
    std::ofstream file(upluginFile);
    if (!file.is_open()) {
        DiagnosticEngine::get().error("Cannot create plugin file: " + upluginFile.string());
        return false;
    }
    
    file << "{\n";
    file << "    \"FileVersion\": 3,\n";
    file << "    \"Version\": " << version << ",\n";
    file << "    \"VersionName\": \"" << version << "\",\n";
    file << "    \"FriendlyName\": \"" << name << "\",\n";
    file << "    \"Description\": \"" << description << "\",\n";
    file << "    \"Category\": \"Programming\",\n";
    file << "    \"CreatedBy\": \"kiwiLang\",\n";
    file << "    \"CreatedByURL\": \"\",\n";
    file << "    \"DocsURL\": \"\",\n";
    file << "    \"MarketplaceURL\": \"\",\n";
    file << "    \"SupportURL\": \"\",\n";
    file << "    \"EnabledByDefault\": true,\n";
    file << "    \"CanContainContent\": true,\n";
    file << "    \"IsBetaVersion\": false,\n";
    file << "    \"Installed\": false,\n";
    
    if (!modules.empty()) {
        file << "    \"Modules\": [\n";
        for (size_t i = 0; i < modules.size(); ++i) {
            const auto& module = modules[i];
            file << "        {\n";
            file << "            \"Name\": \"" << module.name << "\",\n";
            file << "            \"Type\": \"" << module.type << "\",\n";
            file << "            \"LoadingPhase\": \"Default\"\n";
            file << "        }";
            if (i != modules.size() - 1) file << ",";
            file << "\n";
        }
        file << "    ],\n";
    }
    
    file << "    \"Plugins\": []\n";
    file << "}\n";
    
    return true;
}

UEPluginConfig UEPluginConfig::loadFromFile(const path& upluginFile) {
    UEPluginConfig config;
    
    if (!exists(upluginFile)) {
        throw std::runtime_error("Plugin file does not exist: " + upluginFile.string());
    }
    
    std::ifstream file(upluginFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    // Simple JSON parsing
    std::regex nameRegex(R"(\"FriendlyName\"\s*:\s*\"([^\"]+)\")");
    std::regex versionRegex(R"(\"Version\"\s*:\s*(\d+))");
    std::regex descRegex(R"(\"Description\"\s*:\s*\"([^\"]+)\")");
    
    std::smatch match;
    
    if (std::regex_search(content, match, nameRegex)) {
        config.name = match[1];
    }
    
    if (std::regex_search(content, match, versionRegex)) {
        config.version = match[1];
    }
    
    if (std::regex_search(content, match, descRegex)) {
        config.description = match[1];
    }
    
    return config;
}

std::vector<std::string> UECompileSettings::getCompilerFlags(UEPlatform platform) const {
    std::vector<std::string> flags;
    
    // Common flags
    flags.push_back("-std=c++20");
    flags.push_back("-fPIC");
    
    if (enableDebugSymbols) {
        flags.push_back("-g");
        flags.push_back("-ggdb");
    }
    
    if (enablePCH) {
        flags.push_back("-fpch-preprocess");
    }
    
    if (optimizationLevel == "Debug") {
        flags.push_back("-O0");
    } else if (optimizationLevel == "Development") {
        flags.push_back("-O2");
    } else if (optimizationLevel == "Shipping") {
        flags.push_back("-O3");
        flags.push_back("-DNDEBUG");
    }
    
    // Platform-specific flags
    switch (platform) {
        case UEPlatform::Win64:
            flags.push_back("-DWIN32");
            flags.push_back("-D_WINDOWS");
            break;
        case UEPlatform::Linux:
            flags.push_back("-DLINUX");
            flags.push_back("-D__LINUX__");
            break;
        case UEPlatform::Mac:
            flags.push_back("-D__APPLE__");
            flags.push_back("-D__MACH__");
            break;
        default:
            break;
    }
    
    // Additional flags
    flags.insert(flags.end(), additionalCompilerFlags.begin(), additionalCompilerFlags.end());
    
    return flags;
}

std::vector<std::string> UECompileSettings::getLinkerFlags(UEPlatform platform) const {
    std::vector<std::string> flags;
    
    if (enableLTO) {
        flags.push_back("-flto");
    }
    
    // Platform-specific linker flags
    switch (platform) {
        case UEPlatform::Win64:
            flags.push_back("-Wl,--subsystem,windows");
            break;
        case UEPlatform::Linux:
            flags.push_back("-Wl,--export-dynamic");
            break;
        case UEPlatform::Mac:
            flags.push_back("-Wl,-dead_strip");
            break;
        default:
            break;
    }
    
    flags.insert(flags.end(), additionalLinkerFlags.begin(), additionalLinkerFlags.end());
    
    return flags;
}

std::vector<std::string> UEHeaderToolConfig::getUHTArguments() const {
    std::vector<std::string> args;
    
    if (generateUHTCompatibleHeaders) {
        args.push_back("-UnrealHeaderTool");
    }
    
    if (generateReflectionData) {
        args.push_back("-Reflection");
    }
    
    if (supportBlueprintType) {
        args.push_back("-BlueprintType");
    }
    
    if (supportInstancedType) {
        args.push_back("-Instanced");
    }
    
    args.insert(args.end(), additionalUHTArguments.begin(), additionalUHTArguments.end());
    
    return args;
}

UERuntimeConfig UERuntimeConfig::getDefault() {
    UERuntimeConfig config;
    config.enableGarbageCollection = true;
    config.enableHotReload = true;
    config.enableAsyncLoading = true;
    config.maxMemoryUsageMB = 4096;
    config.gcCollectionInterval = 5000; // 5 seconds
    return config;
}

UEConfigManager::UEConfigManager(const path& projectRoot)
    : projectRoot_(projectRoot) {
    
    engineConfig_ = UEConfig::loadFromProject(findUProjectFile().string());
    pluginConfig_ = UEPluginConfig::loadFromFile(findPluginDescriptor());
    compileSettings_ = UECompileSettings();
    headerToolConfig_ = UEHeaderToolConfig();
    runtimeConfig_ = UERuntimeConfig::getDefault();
}

bool UEConfigManager::loadConfig() {
    try {
        path configFile = projectRoot_ / "Config" / "kiwiLang.json";
        if (!exists(configFile)) {
            DiagnosticEngine::get().info("No kiwiLang config file found, using defaults");
            return true;
        }
        
        std::ifstream file(configFile);
        nlohmann::json json;
        file >> json;
        
        // Load engine config
        if (json.contains("engine")) {
            const auto& engineJson = json["engine"];
            if (engineJson.contains("version")) {
                const auto& version = engineJson["version"];
                engineConfig_.engineVersion.major = version["major"];
                engineConfig_.engineVersion.minor = version["minor"];
                engineConfig_.engineVersion.patch = version["patch"];
            }
            
            if (engineJson.contains("targetPlatform")) {
                std::string platform = engineJson["targetPlatform"];
                if (platform == "Win64") engineConfig_.targetPlatform = UEPlatform::Win64;
                else if (platform == "Linux") engineConfig_.targetPlatform = UEPlatform::Linux;
                else if (platform == "Mac") engineConfig_.targetPlatform = UEPlatform::Mac;
            }
            
            engineConfig_.enableHotReload = engineJson.value("enableHotReload", true);
            engineConfig_.enableBlueprintIntegration = engineJson.value("enableBlueprintIntegration", true);
            engineConfig_.generateStubs = engineJson.value("generateStubs", true);
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to load config: " + std::string(e.what()));
        return false;
    }
}

bool UEConfigManager::saveConfig() {
    try {
        path configDir = projectRoot_ / "Config";
        create_directories(configDir);
        
        path configFile = configDir / "kiwiLang.json";
        std::ofstream file(configFile);
        
        nlohmann::json json;
        
        // Save engine config
        nlohmann::json engineJson;
        engineJson["version"] = {
            {"major", engineConfig_.engineVersion.major},
            {"minor", engineConfig_.engineVersion.minor},
            {"patch", engineConfig_.engineVersion.patch}
        };
        
        switch (engineConfig_.targetPlatform) {
            case UEPlatform::Win64: engineJson["targetPlatform"] = "Win64"; break;
            case UEPlatform::Linux: engineJson["targetPlatform"] = "Linux"; break;
            case UEPlatform::Mac: engineJson["targetPlatform"] = "Mac"; break;
            default: engineJson["targetPlatform"] = "Win64";
        }
        
        engineJson["enableHotReload"] = engineConfig_.enableHotReload;
        engineJson["enableBlueprintIntegration"] = engineConfig_.enableBlueprintIntegration;
        engineJson["generateStubs"] = engineConfig_.generateStubs;
        
        json["engine"] = engineJson;
        
        // Save compile settings
        nlohmann::json compileJson;
        compileJson["optimizeForSize"] = compileSettings_.optimizeForSize;
        compileJson["enableDebugSymbols"] = compileSettings_.enableDebugSymbols;
        compileJson["enableLTO"] = compileSettings_.enableLTO;
        compileJson["enablePCH"] = compileSettings_.enablePCH;
        compileJson["optimizationLevel"] = compileSettings_.optimizationLevel;
        compileJson["additionalCompilerFlags"] = compileSettings_.additionalCompilerFlags;
        compileJson["additionalLinkerFlags"] = compileSettings_.additionalLinkerFlags;
        
        json["compile"] = compileJson;
        
        // Save runtime config
        nlohmann::json runtimeJson;
        runtimeJson["enableGarbageCollection"] = runtimeConfig_.enableGarbageCollection;
        runtimeJson["enableHotReload"] = runtimeConfig_.enableHotReload;
        runtimeJson["enableAsyncLoading"] = runtimeConfig_.enableAsyncLoading;
        runtimeJson["maxMemoryUsageMB"] = runtimeConfig_.maxMemoryUsageMB;
        runtimeJson["gcCollectionInterval"] = runtimeConfig_.gcCollectionInterval;
        
        json["runtime"] = runtimeJson;
        
        file << json.dump(4);
        return true;
        
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to save config: " + std::string(e.what()));
        return false;
    }
}

bool UEConfigManager::validateConfig() const {
    if (!engineConfig_.validate()) {
        return false;
    }
    
    if (!pluginConfig_.validate()) {
        return false;
    }
    
    return true;
}

path UEConfigManager::getGeneratedHeadersPath() const {
    return path(engineConfig_.intermediatePath) / "Generated" / "Public";
}

path UEConfigManager::getIntermediatePath() const {
    return path(engineConfig_.intermediatePath);
}

path UEConfigManager::getBinariesPath(UEPlatform platform) const {
    path binariesDir = path(engineConfig_.projectPath) / "Binaries";
    
    switch (platform) {
        case UEPlatform::Win64:
            return binariesDir / "Win64";
        case UEPlatform::Linux:
            return binariesDir / "Linux";
        case UEPlatform::Mac:
            return binariesDir / "Mac";
        default:
            return binariesDir;
    }
}

path UEConfigManager::getBuildOutputPath(UEPlatform platform) const {
    return getBinariesPath(platform) / engineConfig_.projectName;
}

bool UEConfigManager::generateProjectFiles() {
    try {
        path projectDir = path(engineConfig_.projectPath);
        path uprojectFile = findUProjectFile();
        
        // Generate .uproject if it doesn't exist
        if (!exists(uprojectFile)) {
            std::ofstream file(uprojectFile);
            file << "{\n";
            file << "    \"FileVersion\": 3,\n";
            file << "    \"EngineAssociation\": \"" << engineConfig_.engineVersion.toString() << "\",\n";
            file << "    \"Category\": \"\",\n";
            file << "    \"Description\": \"\",\n";
            file << "    \"Modules\": []\n";
            file << "}\n";
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to generate project files: " + std::string(e.what()));
        return false;
    }
}

bool UEConfigManager::generatePluginFiles() {
    try {
        path pluginDir = projectRoot_ / "Plugins" / pluginConfig_.name;
        create_directories(pluginDir);
        
        // Save .uplugin file
        if (!pluginConfig_.saveToFile(pluginDir)) {
            return false;
        }
        
        // Create module directories
        for (const auto& module : pluginConfig_.modules) {
            path moduleDir = pluginDir / "Source" / module.name;
            create_directories(moduleDir);
            
            // Generate Build.cs file
            path buildCsFile = moduleDir / (module.name + ".Build.cs");
            std::ofstream buildFile(buildCsFile);
            buildFile << module.generateBuildCsContent();
            
            // Create Public and Private directories
            create_directories(moduleDir / "Public");
            create_directories(moduleDir / "Private");
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to generate plugin files: " + std::string(e.what()));
        return false;
    }
}

path UEConfigManager::findUProjectFile() const {
    for (const auto& entry : directory_iterator(projectRoot_)) {
        if (entry.path().extension() == ".uproject") {
            return entry.path();
        }
    }
    
    // If not found, create a default path
    return projectRoot_ / (engineConfig_.projectName + ".uproject");
}

path UEConfigManager::findPluginDescriptor() const {
    path pluginDir = projectRoot_ / "Plugins" / pluginConfig_.name;
    path upluginFile = pluginDir / (pluginConfig_.name + ".uplugin");
    
    if (exists(upluginFile)) {
        return upluginFile;
    }
    
    return upluginFile;
}

} // namespace Unreal
} // namespace kiwiLang