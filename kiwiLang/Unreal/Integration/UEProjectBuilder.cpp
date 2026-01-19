#include "kiwiLang/Unreal/Integration/UEProjectBuilder.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeSystem.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

namespace kiwiLang {
namespace Unreal {

using namespace std::filesystem;

UEProjectBuilder::UEProjectBuilder(const UEConfig& config,
                                 UETypeRegistry* typeRegistry,
                                 UObjectRegistry* objectRegistry)
    : config_(config)
    , typeRegistry_(typeRegistry)
    , objectRegistry_(objectRegistry)
    , initialized_(false) {
}

UEProjectBuilder::~UEProjectBuilder() {
    shutdown();
}

bool UEProjectBuilder::initialize() {
    if (initialized_) {
        return true;
    }
    
    DiagnosticEngine::get().info("Initializing UE Project Builder");
    
    if (!setupDirectories()) {
        DiagnosticEngine::get().error("Failed to setup directories");
        return false;
    }
    
    if (!loadExistingModules()) {
        DiagnosticEngine::get().warning("Failed to load existing modules");
    }
    
    initialized_ = true;
    return true;
}

bool UEProjectBuilder::shutdown() {
    if (!initialized_) {
        return true;
    }
    
    DiagnosticEngine::get().info("Shutting down UE Project Builder");
    
    buildProcess_.reset();
    fileGenerator_.reset();
    modules_.clear();
    moduleFiles_.clear();
    
    initialized_ = false;
    return true;
}

UEBuildResult UEProjectBuilder::buildProject(UEBuildTarget target,
                                           UEBuildConfiguration configuration,
                                           UEPlatform platform) {
    UEBuildResult result;
    result.success = false;
    
    if (!initialized_) {
        result.errors.push_back("Project builder not initialized");
        return result;
    }
    
    DiagnosticEngine::get().info("Building project for platform: " + 
        std::to_string(static_cast<int>(platform)) + 
        ", configuration: " + std::to_string(static_cast<int>(configuration)));
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // Generate project files first
        if (!generateProjectFiles()) {
            result.errors.push_back("Failed to generate project files");
            return result;
        }
        
        // Prepare build arguments
        std::vector<std::string> args;
        args.push_back("-project=\"" + config_.projectPath + "\"");
        
        // Target
        switch (target) {
            case UEBuildTarget::Game:
                args.push_back("-target=Game");
                break;
            case UEBuildTarget::Editor:
                args.push_back("-target=Editor");
                break;
            case UEBuildTarget::Server:
                args.push_back("-target=Server");
                break;
            case UEBuildTarget::Client:
                args.push_back("-target=Client");
                break;
            default:
                args.push_back("-target=Game");
        }
        
        // Configuration
        switch (configuration) {
            case UEBuildConfiguration::Debug:
                args.push_back("-configuration=Debug");
                break;
            case UEBuildConfiguration::DebugGame:
                args.push_back("-configuration=DebugGame");
                break;
            case UEBuildConfiguration::Development:
                args.push_back("-configuration=Development");
                break;
            case UEBuildConfiguration::Shipping:
                args.push_back("-configuration=Shipping");
                break;
            case UEBuildConfiguration::Test:
                args.push_back("-configuration=Test");
                break;
            default:
                args.push_back("-configuration=Development");
        }
        
        // Platform
        switch (platform) {
            case UEPlatform::Win64:
                args.push_back("-platform=Win64");
                break;
            case UEPlatform::Linux:
                args.push_back("-platform=Linux");
                break;
            case UEPlatform::Mac:
                args.push_back("-platform=Mac");
                break;
            case UEPlatform::Android:
                args.push_back("-platform=Android");
                break;
            case UEPlatform::IOS:
                args.push_back("-platform=IOS");
                break;
            default:
                args.push_back("-platform=Win64");
        }
        
        // Additional arguments
        args.push_back("-build");
        args.push_back("-nop4");
        args.push_back("-nodebuginfo");
        
        // Execute build
        path engineRoot = path(config_.projectPath).parent_path().parent_path().parent_path();
        path buildBat = engineRoot / "Engine" / "Build" / "BatchFiles" / "Build.bat";
        
        if (!exists(buildBat)) {
            // Try Linux/Mac build script
            buildBat = engineRoot / "Engine" / "Build" / "BatchFiles" / "Build.sh";
        }
        
        if (!exists(buildBat)) {
            result.errors.push_back("Build script not found: " + buildBat.string());
            return result;
        }
        
        result = executeBuildCommand(args, engineRoot);
        result.buildTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (result.success) {
            DiagnosticEngine::get().info("Build completed successfully in " + 
                std::to_string(result.buildTime.count()) + "ms");
        } else {
            DiagnosticEngine::get().error("Build failed");
        }
        
    } catch (const std::exception& e) {
        result.errors.push_back("Build exception: " + std::string(e.what()));
    }
    
    return result;
}

UEBuildResult UEProjectBuilder::buildPlugin(const std::string& pluginName,
                                          UEBuildConfiguration configuration,
                                          UEPlatform platform) {
    UEBuildResult result;
    result.success = false;
    
    DiagnosticEngine::get().info("Building plugin: " + pluginName);
    
    auto it = modules_.find(pluginName);
    if (it == modules_.end()) {
        result.errors.push_back("Plugin not found: " + pluginName);
        return result;
    }
    
    // Generate plugin files
    if (!generatePluginFiles(pluginName)) {
        result.errors.push_back("Failed to generate plugin files");
        return result;
    }
    
    // Build the module
    return buildProject(UEBuildTarget::Game, configuration, platform);
}

bool UEProjectBuilder::generateProjectFiles() {
    DiagnosticEngine::get().info("Generating project files");
    
    try {
        // Generate .uproject file
        if (!generateUProjectFile()) {
            return false;
        }
        
        // Generate target.cs files
        if (!generateTargetCsFiles()) {
            return false;
        }
        
        // Generate build files for all modules
        for (const auto& [moduleName, moduleInfo] : modules_) {
            if (!generateBuildCsFile(moduleInfo)) {
                DiagnosticEngine::get().error("Failed to generate Build.cs for: " + moduleName);
                return false;
            }
        }
        
        // Run UnrealBuildTool to generate solution/project files
        path engineRoot = path(config_.projectPath).parent_path().parent_path().parent_path();
        path ubtPath = engineRoot / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe";
        
        if (!exists(ubtPath)) {
            // Try Linux/Mac path
            ubtPath = engineRoot / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool";
        }
        
        if (exists(ubtPath)) {
            std::vector<std::string> args = {
                "-projectfiles",
                "-project=\"" + config_.projectPath + "\"",
                "-game",
                "-engine",
                "-progress"
            };
            
            auto result = executeBuildCommand(args, engineRoot);
            if (!result.success) {
                DiagnosticEngine::get().error("Failed to generate project files");
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Exception generating project files: " + std::string(e.what()));
        return false;
    }
}

bool UEProjectBuilder::generateModuleFiles(const std::string& moduleName) {
    DiagnosticEngine::get().info("Generating files for module: " + moduleName);
    
    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        DiagnosticEngine::get().error("Module not found: " + moduleName);
        return false;
    }
    
    const auto& moduleInfo = it->second;
    
    // Generate Build.cs file
    if (!generateBuildCsFile(moduleInfo)) {
        return false;
    }
    
    // Generate module header
    path modulePath = getModulePath(moduleName);
    path headerPath = modulePath / "Public" / (moduleName + ".h");
    
    std::ofstream headerFile(headerPath);
    if (!headerFile.is_open()) {
        DiagnosticEngine::get().error("Cannot create header file: " + headerPath.string());
        return false;
    }
    
    headerFile << "#pragma once\n\n";
    headerFile << "#include \"CoreMinimal.h\"\n";
    headerFile << "#include \"Modules/ModuleManager.h\"\n\n";
    headerFile << "DECLARE_LOG_CATEGORY_EXTERN(Log" << moduleName << ", Log, All);\n\n";
    headerFile << "class F" << moduleName << "Module : public IModuleInterface\n";
    headerFile << "{\n";
    headerFile << "public:\n";
    headerFile << "    virtual void StartupModule() override;\n";
    headerFile << "    virtual void ShutdownModule() override;\n";
    headerFile << "};\n";
    
    // Generate module source
    path sourcePath = modulePath / "Private" / (moduleName + ".cpp");
    
    std::ofstream sourceFile(sourcePath);
    if (!sourceFile.is_open()) {
        DiagnosticEngine::get().error("Cannot create source file: " + sourcePath.string());
        return false;
    }
    
    sourceFile << "#include \"" << moduleName << ".h\"\n\n";
    sourceFile << "DEFINE_LOG_CATEGORY(Log" << moduleName << ");\n\n";
    sourceFile << "#define LOCTEXT_NAMESPACE \"F" << moduleName << "Module\"\n\n";
    sourceFile << "void F" << moduleName << "Module::StartupModule()\n";
    sourceFile << "{\n";
    sourceFile << "    UE_LOG(Log" << moduleName << ", Log, TEXT(\"" << moduleName << " module starting up\"));\n";
    sourceFile << "}\n\n";
    sourceFile << "void F" << moduleName << "Module::ShutdownModule()\n";
    sourceFile << "{\n";
    sourceFile << "    UE_LOG(Log" << moduleName << ", Log, TEXT(\"" << moduleName << " module shutting down\"));\n";
    sourceFile << "}\n\n";
    sourceFile << "#undef LOCTEXT_NAMESPACE\n\n";
    sourceFile << "IMPLEMENT_MODULE(F" << moduleName << "Module, " << moduleName << ");\n";
    
    return true;
}

bool UEProjectBuilder::generatePluginFiles(const std::string& pluginName) {
    DiagnosticEngine::get().info("Generating plugin files: " + pluginName);
    
    path pluginDir = path(config_.projectPath) / "Plugins" / pluginName;
    create_directories(pluginDir);
    
    // Generate .uplugin file
    if (!generateUPluginFile()) {
        return false;
    }
    
    // Generate module files for the plugin
    return generateModuleFiles(pluginName);
}

bool UEProjectBuilder::addSourceFile(const path& sourcePath,
                                    const std::string& moduleName,
                                    bool isPublicAPI) {
    if (!exists(sourcePath)) {
        DiagnosticEngine::get().error("Source file does not exist: " + sourcePath.string());
        return false;
    }
    
    auto it = moduleFiles_.find(moduleName);
    if (it == moduleFiles_.end()) {
        moduleFiles_[moduleName] = {};
    }
    
    UEProjectFile file;
    file.sourcePath = sourcePath;
    file.moduleName = moduleName;
    file.isPublicAPI = isPublicAPI;
    file.isGenerated = false;
    file.lastModified = last_write_time(sourcePath);
    
    moduleFiles_[moduleName].push_back(file);
    return true;
}

bool UEProjectBuilder::addGeneratedFile(const path& generatedPath,
                                       const std::string& content,
                                       const std::string& moduleName,
                                       bool isPublicAPI) {
    // Create directory if it doesn't exist
    create_directories(generatedPath.parent_path());
    
    std::ofstream file(generatedPath);
    if (!file.is_open()) {
        DiagnosticEngine::get().error("Cannot create generated file: " + generatedPath.string());
        return false;
    }
    
    file << content;
    
    UEProjectFile projectFile;
    projectFile.generatedPath = generatedPath;
    projectFile.moduleName = moduleName;
    projectFile.isPublicAPI = isPublicAPI;
    projectFile.isGenerated = true;
    projectFile.lastModified = std::chrono::file_clock::now();
    
    moduleFiles_[moduleName].push_back(projectFile);
    return true;
}

bool UEProjectBuilder::registerModule(const UEModuleBuildInfo& moduleInfo) {
    if (!moduleInfo.validate()) {
        DiagnosticEngine::get().error("Invalid module info");
        return false;
    }
    
    if (modules_.find(moduleInfo.name) != modules_.end()) {
        DiagnosticEngine::get().warning("Module already registered: " + moduleInfo.name);
        return false;
    }
    
    modules_[moduleInfo.name] = moduleInfo;
    DiagnosticEngine::get().info("Registered module: " + moduleInfo.name);
    
    return true;
}

bool UEProjectBuilder::unregisterModule(const std::string& moduleName) {
    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        DiagnosticEngine::get().warning("Module not found: " + moduleName);
        return false;
    }
    
    modules_.erase(it);
    moduleFiles_.erase(moduleName);
    
    DiagnosticEngine::get().info("Unregistered module: " + moduleName);
    return true;
}

std::vector<std::string> UEProjectBuilder::getRegisteredModules() const {
    std::vector<std::string> moduleNames;
    moduleNames.reserve(modules_.size());
    
    for (const auto& [name, _] : modules_) {
        moduleNames.push_back(name);
    }
    
    return moduleNames;
}

std::vector<UEProjectFile> UEProjectBuilder::getModuleFiles(const std::string& moduleName) const {
    auto it = moduleFiles_.find(moduleName);
    if (it != moduleFiles_.end()) {
        return it->second;
    }
    return {};
}

bool UEProjectBuilder::cleanBuildArtifacts() {
    DiagnosticEngine::get().info("Cleaning build artifacts");
    
    try {
        path binariesDir = getBinariesPath(config_.targetPlatform);
        if (exists(binariesDir)) {
            remove_all(binariesDir);
        }
        
        path intermediateDir = getIntermediatePath(config_.targetPlatform);
        if (exists(intermediateDir)) {
            remove_all(intermediateDir);
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to clean build artifacts: " + std::string(e.what()));
        return false;
    }
}

bool UEProjectBuilder::cleanGeneratedFiles() {
    DiagnosticEngine::get().info("Cleaning generated files");
    
    try {
        if (exists(generatedDir_)) {
            remove_all(generatedDir_);
        }
        
        // Clear module files cache
        for (auto& [moduleName, files] : moduleFiles_) {
            files.erase(
                std::remove_if(files.begin(), files.end(),
                    [](const UEProjectFile& file) { return file.isGenerated; }),
                files.end());
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to clean generated files: " + std::string(e.what()));
        return false;
    }
}

bool UEProjectBuilder::setupDirectories() {
    try {
        // Create intermediate directory
        intermediateDir_ = path(config_.intermediatePath);
        create_directories(intermediateDir_);
        
        // Create binaries directory
        binariesDir_ = getBinariesPath(config_.targetPlatform);
        create_directories(binariesDir_);
        
        // Create generated directory
        generatedDir_ = intermediateDir_ / "Generated";
        create_directories(generatedDir_ / "Public");
        create_directories(generatedDir_ / "Private");
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to setup directories: " + std::string(e.what()));
        return false;
    }
}

bool UEProjectBuilder::loadExistingModules() {
    path sourceDir = path(config_.projectPath) / "Source";
    if (!exists(sourceDir)) {
        return true; // No source directory yet
    }
    
    try {
        for (const auto& entry : directory_iterator(sourceDir)) {
            if (!entry.is_directory()) {
                continue;
            }
            
            path buildCsFile = entry.path() / (entry.path().filename().string() + ".Build.cs");
            if (exists(buildCsFile)) {
                // Parse existing module
                std::ifstream file(buildCsFile);
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                UEModuleBuildInfo moduleInfo;
                moduleInfo.name = entry.path().filename().string();
                
                // Simple parsing to extract dependencies
                // In a real implementation, this would use proper C# parsing
                std::regex depsRegex(R"(DependencyModuleNames\.AddRange\(new string\[\] \{(.*?)\}\))");
                std::smatch match;
                
                if (std::regex_search(content, match, depsRegex)) {
                    std::string deps = match[1];
                    std::regex nameRegex(R"(\"([^\"]+)\")");
                    auto words_begin = std::sregex_iterator(deps.begin(), deps.end(), nameRegex);
                    auto words_end = std::sregex_iterator();
                    
                    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                        moduleInfo.publicDependencyModuleNames.push_back((*i)[1]);
                    }
                }
                
                modules_[moduleInfo.name] = moduleInfo;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().warning("Failed to load existing modules: " + std::string(e.what()));
        return false;
    }
}

bool UEProjectBuilder::saveModuleRegistry() {
    path registryFile = intermediateDir_ / "ModuleRegistry.json";
    
    try {
        std::ofstream file(registryFile);
        nlohmann::json json;
        
        for (const auto& [name, moduleInfo] : modules_) {
            nlohmann::json moduleJson;
            moduleJson["name"] = moduleInfo.name;
            moduleJson["type"] = moduleInfo.type;
            moduleJson["publicDependencies"] = moduleInfo.publicDependencyModuleNames;
            moduleJson["privateDependencies"] = moduleInfo.privateDependencyModuleNames;
            
            json[name] = moduleJson;
        }
        
        file << json.dump(4);
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to save module registry: " + std::string(e.what()));
        return false;
    }
}

path UEProjectBuilder::getModulePath(const std::string& moduleName) const {
    return path(config_.projectPath) / "Source" / moduleName;
}

path UEProjectBuilder::getModuleBuildCsPath(const std::string& moduleName) const {
    return getModulePath(moduleName) / (moduleName + ".Build.cs");
}

path UEProjectBuilder::getModuleIntermediatePath(const std::string& moduleName) const {
    return intermediateDir_ / moduleName;
}

UEBuildResult UEProjectBuilder::executeBuildCommand(const std::vector<std::string>& args,
                                                   const path& workingDir) {
    UEBuildResult result;
    
    // In a real implementation, this would spawn the build process
    // and capture its output. For now, simulate success.
    
    result.success = true;
    result.exitCode = 0;
    
    // Simulate build output
    result.outputFiles.push_back("Game.exe");
    result.outputFiles.push_back("Game.pdb");
    
    DiagnosticEngine::get().info("Build command executed: " + args[0]);
    
    return result;
}

bool UEProjectBuilder::generateBuildCsFile(const UEModuleBuildInfo& moduleInfo) {
    path buildCsPath = getModuleBuildCsPath(moduleInfo.name);
    create_directories(buildCsPath.parent_path());
    
    std::ofstream file(buildCsPath);
    if (!file.is_open()) {
        DiagnosticEngine::get().error("Cannot create Build.cs file: " + buildCsPath.string());
        return false;
    }
    
    file << moduleInfo.generateBuildCsContent();
    return true;
}

bool UEProjectBuilder::generateTargetCsFiles() {
    path targetDir = path(config_.projectPath) / "Source";
    create_directories(targetDir);
    
    // Generate Game target
    path gameTargetPath = targetDir / (config_.projectName + ".Target.cs");
    std::ofstream gameFile(gameTargetPath);
    
    if (gameFile.is_open()) {
        gameFile << "using UnrealBuildTool;\n\n";
        gameFile << "public class " << config_.projectName << "Target : TargetRules\n";
        gameFile << "{\n";
        gameFile << "    public " << config_.projectName << "Target(TargetInfo Target) : base(Target)\n";
        gameFile << "    {\n";
        gameFile << "        Type = TargetType.Game;\n";
        gameFile << "        DefaultBuildSettings = BuildSettingsVersion.V2;\n";
        gameFile << "        ExtraModuleNames.AddRange(new string[] { \"" << config_.projectName << "\" });\n";
        gameFile << "    }\n";
        gameFile << "}\n";
    }
    
    // Generate Editor target
    path editorTargetPath = targetDir / (config_.projectName + "Editor.Target.cs");
    std::ofstream editorFile(editorTargetPath);
    
    if (editorFile.is_open()) {
        editorFile << "using UnrealBuildTool;\n\n";
        editorFile << "public class " << config_.projectName << "EditorTarget : TargetRules\n";
        editorFile << "{\n";
        editorFile << "    public " << config_.projectName << "EditorTarget(TargetInfo Target) : base(Target)\n";
        editorFile << "    {\n";
        editorFile << "        Type = TargetType.Editor;\n";
        editorFile << "        DefaultBuildSettings = BuildSettingsVersion.V2;\n";
        editorFile << "        ExtraModuleNames.AddRange(new string[] { \"" << config_.projectName << "\" });\n";
        editorFile << "    }\n";
        editorFile << "}\n";
    }
    
    return true;
}

bool UEProjectBuilder::generateUProjectFile() {
    path uprojectPath = path(config_.projectPath) / (config_.projectName + ".uproject");
    
    std::ofstream file(uprojectPath);
    if (!file.is_open()) {
        DiagnosticEngine::get().error("Cannot create .uproject file: " + uprojectPath.string());
        return false;
    }
    
    file << "{\n";
    file << "    \"FileVersion\": 3,\n";
    file << "    \"EngineAssociation\": \"" << config_.engineVersion.toString() << "\",\n";
    file << "    \"Category\": \"\",\n";
    file << "    \"Description\": \"\",\n";
    file << "    \"Modules\": [\n";
    
    bool first = true;
    for (const auto& [moduleName, moduleInfo] : modules_) {
        if (!first) file << ",\n";
        first = false;
        
        file << "        {\n";
        file << "            \"Name\": \"" << moduleInfo.name << "\",\n";
        file << "            \"Type\": \"" << moduleInfo.type << "\",\n";
        file << "            \"LoadingPhase\": \"Default\"\n";
        file << "        }";
    }
    
    file << "\n    ]\n";
    file << "}\n";
    
    return true;
}

bool UEProjectBuilder::generateUPluginFile() {
    // This would generate a .uplugin file for plugin projects
    // Implementation similar to generateUProjectFile
    
    return true;
}

bool UEProjectBuilder::compileGeneratedCode(const std::string& moduleName) {
    // This would compile generated C++ code
    // In a real implementation, this would invoke the compiler
    
    DiagnosticEngine::get().info("Compiling generated code for module: " + moduleName);
    return true;
}

bool UEProjectBuilder::linkModule(const std::string& moduleName,
                                 UEBuildConfiguration configuration,
                                 UEPlatform platform) {
    // This would link the compiled module
    DiagnosticEngine::get().info("Linking module: " + moduleName);
    return true;
}

void UEProjectBuilder::collectBuildDependencies(const std::string& moduleName,
                                               std::vector<std::string>& dependencies) const {
    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        return;
    }
    
    for (const auto& dep : it->second.publicDependencyModuleNames) {
        if (std::find(dependencies.begin(), dependencies.end(), dep) == dependencies.end()) {
            dependencies.push_back(dep);
            collectBuildDependencies(dep, dependencies);
        }
    }
    
    for (const auto& dep : it->second.privateDependencyModuleNames) {
        if (std::find(dependencies.begin(), dependencies.end(), dep) == dependencies.end()) {
            dependencies.push_back(dep);
            collectBuildDependencies(dep, dependencies);
        }
    }
}

void UEProjectBuilder::validateModuleDependencies() const {
    // Check for circular dependencies
    for (const auto& [moduleName, moduleInfo] : modules_) {
        std::unordered_set<std::string> visited;
        std::vector<std::string> path;
        
        // Depth-first search for cycles
        std::function<bool(const std::string&)> hasCycle = 
            [&](const std::string& current) -> bool {
                if (visited.find(current) != visited.end()) {
                    // Found a cycle
                    DiagnosticEngine::get().error("Circular dependency detected involving: " + current);
                    return true;
                }
                
                visited.insert(current);
                path.push_back(current);
                
                auto it = modules_.find(current);
                if (it != modules_.end()) {
                    for (const auto& dep : it->second.publicDependencyModuleNames) {
                        if (hasCycle(dep)) {
                            return true;
                        }
                    }
                    
                    for (const auto& dep : it->second.privateDependencyModuleNames) {
                        if (hasCycle(dep)) {
                            return true;
                        }
                    }
                }
                
                visited.erase(current);
                path.pop_back();
                return false;
            };
        
        hasCycle(moduleName);
    }
}

} // namespace Unreal
} // namespace kiwiLang