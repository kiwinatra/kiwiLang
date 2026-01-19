#pragma once

#include "UnrealEngine.h"
#include "UETypes.h"
#include <memory>
#include <vector>
#include <functional>

namespace kiwiLang {

class Compiler;
class DiagnosticEngine;
class SourceManager;

namespace Unreal {

class UEProjectBuilder;
class UEModuleLoader;

enum class UEIntegrationPhase {
    PreInit,
    ModuleLoading,
    TypeRegistration,
    CodeGeneration,
    PostInit,
    Shutdown
};

struct UEIntegrationContext {
    UnrealEngine* engine;
    Compiler* compiler;
    DiagnosticEngine* diagnostics;
    SourceManager* sourceManager;
    UETypeRegistry* typeRegistry;
    UEProjectBuilder* projectBuilder;
    UEModuleLoader* moduleLoader;
    
    void* userData;
    
    bool isSuccessful() const;
    void reportError(const std::string& message);
    void reportWarning(const std::string& message);
};

class KIWI_UNREAL_API UEIntegrationPlugin {
public:
    virtual ~UEIntegrationPlugin() = default;
    
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual bool supportsPhase(UEIntegrationPhase phase) const = 0;
    
    virtual bool initialize(UEIntegrationContext& context) = 0;
    virtual bool execute(UEIntegrationContext& context) = 0;
    virtual bool shutdown(UEIntegrationContext& context) = 0;
};

class KIWI_UNREAL_API UEBlueprintPlugin : public UEIntegrationPlugin {
public:
    std::string getName() const override { return "BlueprintIntegration"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    bool supportsPhase(UEIntegrationPhase phase) const override {
        return phase == UEIntegrationPhase::TypeRegistration ||
               phase == UEIntegrationPhase::CodeGeneration;
    }
    
    bool initialize(UEIntegrationContext& context) override;
    bool execute(UEIntegrationContext& context) override;
    bool shutdown(UEIntegrationContext& context) override;
    
private:
    struct BlueprintClassInfo;
    std::vector<BlueprintClassInfo> blueprintClasses_;
    
    bool generateBlueprintGraphs(UEIntegrationContext& context);
    bool compileBlueprintAssets(UEIntegrationContext& context);
};

class KIWI_UNREAL_API UENetworkingPlugin : public UEIntegrationPlugin {
public:
    std::string getName() const override { return "Networking"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    bool supportsPhase(UEIntegrationPhase phase) const override {
        return phase == UEIntegrationPhase::TypeRegistration ||
               phase == UEIntegrationPhase::CodeGeneration;
    }
    
    bool initialize(UEIntegrationContext& context) override;
    bool execute(UEIntegrationContext& context) override;
    bool shutdown(UEIntegrationContext& context) override;
    
private:
    struct ReplicatedProperty;
    struct RPCFunction;
    
    std::vector<ReplicatedProperty> replicatedProperties_;
    std::vector<RPCFunction> rpcFunctions_;
    
    bool generateReplicationCode(UEIntegrationContext& context);
    bool validateNetworkTypes(UEIntegrationContext& context);
};

class KIWI_UNREAL_API UEAssetPlugin : public UEIntegrationPlugin {
public:
    std::string getName() const override { return "AssetSystem"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    bool supportsPhase(UEIntegrationPhase phase) const override {
        return phase == UEIntegrationPhase::PreInit ||
               phase == UEIntegrationPhase::PostInit;
    }
    
    bool initialize(UEIntegrationContext& context) override;
    bool execute(UEIntegrationContext& context) override;
    bool shutdown(UEIntegrationContext& context) override;
    
private:
    struct AssetReference;
    struct AssetGenerator;
    
    std::vector<AssetReference> managedAssets_;
    std::unique_ptr<AssetGenerator> assetGenerator_;
    
    bool scanAssetDirectory(UEIntegrationContext& context);
    bool generateAssetRegistry(UEIntegrationContext& context);
};

class KIWI_UNREAL_API UEIntegrationManager {
public:
    explicit UEIntegrationManager(UEIntegrationContext& context);
    ~UEIntegrationManager();
    
    bool registerPlugin(std::unique_ptr<UEIntegrationPlugin> plugin);
    bool unregisterPlugin(const std::string& pluginName);
    
    bool executePhase(UEIntegrationPhase phase);
    bool executeAllPhases();
    
    UEIntegrationPlugin* findPlugin(const std::string& name) const;
    std::vector<std::string> getActivePlugins() const;
    
    bool loadPluginsFromDirectory(const std::string& directory);
    bool savePluginConfiguration() const;
    
    const UEIntegrationContext& getContext() const { return context_; }
    
private:
    UEIntegrationContext context_;
    std::vector<std::unique_ptr<UEIntegrationPlugin>> plugins_;
    std::unordered_map<UEIntegrationPhase, std::vector<UEIntegrationPlugin*>> phasePlugins_;
    
    void organizePluginsByPhase();
    bool validatePluginDependencies() const;
    bool executePluginPhase(UEIntegrationPlugin* plugin, UEIntegrationPhase phase);
};

} // namespace Unreal
} // namespace kiwiLang