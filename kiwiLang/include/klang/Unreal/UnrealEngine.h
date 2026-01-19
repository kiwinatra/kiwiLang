#pragma once

#include "kiwiLang/Config.h"
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#if KIWI_UNREAL_ENABLED

namespace kiwiLang {
namespace Unreal {

class UObjectRegistry;
class UETypeSystem;
class UERuntime;
class UEProjectBuilder;

struct UEVersion {
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;
    
    constexpr bool operator==(const UEVersion& other) const noexcept {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
    
    constexpr bool operator<(const UEVersion& other) const noexcept {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
    
    std::string toString() const;
};

enum class UEPlatform {
    Win64,
    Linux,
    Mac,
    Android,
    IOS,
    PS5,
    XboxSeriesX
};

struct UEConfig {
    UEVersion engineVersion;
    UEPlatform targetPlatform;
    std::string projectName;
    std::string projectPath;
    std::string intermediatePath;
    bool enableHotReload;
    bool enableBlueprintIntegration;
    bool generateStubs;
    
    static UEConfig loadFromProject(const std::string& uprojectPath);
    bool validate() const;
};

class KIWI_UNREAL_API UnrealEngine {
public:
    explicit UnrealEngine(const UEConfig& config);
    ~UnrealEngine();
    
    static bool isEngineAvailable();
    static UEVersion detectEngineVersion(const std::string& engineRoot);
    static std::vector<UEPlatform> getSupportedPlatforms();
    
    bool initialize();
    bool shutdown();
    
    bool compileProject();
    bool generateBindings();
    bool deployToPlatform(UEPlatform platform);
    
    UObjectRegistry& getObjectRegistry();
    UETypeSystem& getTypeSystem();
    UERuntime& getRuntime();
    UEProjectBuilder& getProjectBuilder();
    
    const UEConfig& getConfig() const { return config_; }
    bool isInitialized() const { return initialized_; }
    
private:
    UEConfig config_;
    bool initialized_;
    
    std::unique_ptr<UObjectRegistry> objectRegistry_;
    std::unique_ptr<UETypeSystem> typeSystem_;
    std::unique_ptr<UERuntime> runtime_;
    std::unique_ptr<UEProjectBuilder> projectBuilder_;
    
    bool loadEngineModules();
    bool setupPaths();
    bool verifyCompatibility();
};

} // namespace Unreal
} // namespace kiwiLang

#else // KIWI_UNREAL_ENABLED

namespace kiwiLang {
namespace Unreal {

class KIWI_UNREAL_API UnrealEngine {
public:
    explicit UnrealEngine(const struct UEConfig&) {
        throw std::runtime_error("Unreal Engine support not compiled in");
    }
};

} // namespace Unreal
} // namespace kiwiLang

#endif // KIWI_UNREAL_ENABLED