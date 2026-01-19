#include "kiwiLang/Unreal/UEIntegration.h"
#include "kiwiLang/Compiler/Compiler.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <algorithm>

namespace kiwiLang {
namespace Unreal {

bool UEIntegrationContext::isSuccessful() const {
    if (!diagnostics) return true;
    return !diagnostics->hasErrors();
}

void UEIntegrationContext::reportError(const std::string& message) {
    if (diagnostics) {
        diagnostics->error(message);
    }
}

void UEIntegrationContext::reportWarning(const std::string& message) {
    if (diagnostics) {
        diagnostics->warning(message);
    }
}

bool UEBlueprintPlugin::initialize(UEIntegrationContext& context) {
    if (!context.engine || !context.typeRegistry) {
        context.reportError("Blueprint plugin requires engine and type registry");
        return false;
    }
    
    DiagnosticEngine::get().info("Initializing Blueprint Integration plugin");
    return true;
}

bool UEBlueprintPlugin::execute(UEIntegrationContext& context) {
    if (!context.isSuccessful()) {
        return false;
    }
    
    DiagnosticEngine::get().info("Executing Blueprint Integration plugin");
    
    // Generate blueprint graphs
    if (!generateBlueprintGraphs(context)) {
        context.reportError("Failed to generate blueprint graphs");
        return false;
    }
    
    // Compile blueprint assets
    if (!compileBlueprintAssets(context)) {
        context.reportError("Failed to compile blueprint assets");
        return false;
    }
    
    return true;
}

bool UEBlueprintPlugin::shutdown(UEIntegrationContext& context) {
    blueprintClasses_.clear();
    DiagnosticEngine::get().info("Shutting down Blueprint Integration plugin");
    return true;
}

bool UEBlueprintPlugin::generateBlueprintGraphs(UEIntegrationContext& context) {
    // This would generate blueprint graph assets from kiwiLang classes
    // For now, just log what we would do
    
    DiagnosticEngine::get().info("Generating blueprint graphs...");
    
    // In a real implementation, this would:
    // 1. Iterate through all registered classes in typeRegistry
    // 2. Generate blueprint graph assets for blueprintable classes
    // 3. Create event graphs, function graphs, etc.
    // 4. Save .uasset files
    
    return true;
}

bool UEBlueprintPlugin::compileBlueprintAssets(UEIntegrationContext& context) {
    DiagnosticEngine::get().info("Compiling blueprint assets...");
    
    // This would invoke the Unreal Engine blueprint compiler
    // For now, just simulate success
    
    return true;
}

bool UENetworkingPlugin::initialize(UEIntegrationContext& context) {
    if (!context.engine || !context.typeRegistry) {
        context.reportError("Networking plugin requires engine and type registry");
        return false;
    }
    
    DiagnosticEngine::get().info("Initializing Networking plugin");
    return true;
}

bool UENetworkingPlugin::execute(UEIntegrationContext& context) {
    if (!context.isSuccessful()) {
        return false;
    }
    
    DiagnosticEngine::get().info("Executing Networking plugin");
    
    // Generate replication code
    if (!generateReplicationCode(context)) {
        context.reportError("Failed to generate replication code");
        return false;
    }
    
    // Validate network types
    if (!validateNetworkTypes(context)) {
        context.reportError("Network type validation failed");
        return false;
    }
    
    return true;
}

bool UENetworkingPlugin::shutdown(UEIntegrationContext& context) {
    replicatedProperties_.clear();
    rpcFunctions_.clear();
    DiagnosticEngine::get().info("Shutting down Networking plugin");
    return true;
}

bool UENetworkingPlugin::generateReplicationCode(UEIntegrationContext& context) {
    DiagnosticEngine::get().info("Generating replication code...");
    
    // This would generate:
    // 1. Replication macros for properties
    // 2. RPC function stubs
    // 3. Network serialization code
    
    return true;
}

bool UENetworkingPlugin::validateNetworkTypes(UEIntegrationContext& context) {
    DiagnosticEngine::get().info("Validating network types...");
    
    // Validate that types used in networking are replicatable
    // Check for unsupported types, size limitations, etc.
    
    return true;
}

bool UEAssetPlugin::initialize(UEIntegrationContext& context) {
    if (!context.engine || !context.projectBuilder) {
        context.reportError("Asset plugin requires engine and project builder");
        return false;
    }
    
    DiagnosticEngine::get().info("Initializing Asset System plugin");
    return true;
}

bool UEAssetPlugin::execute(UEIntegrationContext& context) {
    if (!context.isSuccessful()) {
        return false;
    }
    
    DiagnosticEngine::get().info("Executing Asset System plugin");
    
    // Scan asset directory
    if (!scanAssetDirectory(context)) {
        context.reportError("Failed to scan asset directory");
        return false;
    }
    
    // Generate asset registry
    if (!generateAssetRegistry(context)) {
        context.reportError("Failed to generate asset registry");
        return false;
    }
    
    return true;
}

bool UEAssetPlugin::shutdown(UEIntegrationContext& context) {
    managedAssets_.clear();
    assetGenerator_.reset();
    DiagnosticEngine::get().info("Shutting down Asset System plugin");
    return true;
}

bool UEAssetPlugin::scanAssetDirectory(UEIntegrationContext& context) {
    DiagnosticEngine::get().info("Scanning asset directory...");
    
    // Scan for .uasset files, collect metadata
    // Build list of assets that need kiwiLang integration
    
    return true;
}

bool UEAssetPlugin::generateAssetRegistry(UEIntegrationContext& context) {
    DiagnosticEngine::get().info("Generating asset registry...");
    
    // Create registry of assets with kiwiLang bindings
    // Generate asset reference code
    
    return true;
}

UEIntegrationManager::UEIntegrationManager(UEIntegrationContext& context)
    : context_(context) {
    organizePluginsByPhase();
}

UEIntegrationManager::~UEIntegrationManager() {
    shutdown();
}

bool UEIntegrationManager::registerPlugin(std::unique_ptr<UEIntegrationPlugin> plugin) {
    if (!plugin) {
        DiagnosticEngine::get().error("Cannot register null plugin");
        return false;
    }
    
    std::string pluginName = plugin->getName();
    if (findPlugin(pluginName)) {
        DiagnosticEngine::get().warning("Plugin already registered: " + pluginName);
        return false;
    }
    
    plugins_.push_back(std::move(plugin));
    organizePluginsByPhase();
    
    DiagnosticEngine::get().info("Registered plugin: " + pluginName);
    return true;
}

bool UEIntegrationManager::unregisterPlugin(const std::string& pluginName) {
    auto it = std::find_if(plugins_.begin(), plugins_.end(),
        [&](const std::unique_ptr<UEIntegrationPlugin>& plugin) {
            return plugin->getName() == pluginName;
        });
    
    if (it != plugins_.end()) {
        plugins_.erase(it);
        organizePluginsByPhase();
        DiagnosticEngine::get().info("Unregistered plugin: " + pluginName);
        return true;
    }
    
    DiagnosticEngine::get().warning("Plugin not found: " + pluginName);
    return false;
}

bool UEIntegrationManager::executePhase(UEIntegrationPhase phase) {
    DiagnosticEngine::get().info("Executing integration phase: " + 
        std::to_string(static_cast<int>(phase)));
    
    auto phaseIt = phasePlugins_.find(phase);
    if (phaseIt == phasePlugins_.end()) {
        DiagnosticEngine::get().warning("No plugins registered for phase");
        return true;
    }
    
    bool overallSuccess = true;
    
    for (auto* plugin : phaseIt->second) {
        if (!plugin->supportsPhase(phase)) {
            continue;
        }
        
        DiagnosticEngine::get().info("Executing plugin: " + plugin->getName() + 
            " for phase: " + std::to_string(static_cast<int>(phase)));
        
        bool pluginSuccess = executePluginPhase(plugin, phase);
        if (!pluginSuccess) {
            DiagnosticEngine::get().error("Plugin failed: " + plugin->getName());
            overallSuccess = false;
        }
    }
    
    return overallSuccess;
}

bool UEIntegrationManager::executeAllPhases() {
    DiagnosticEngine::get().info("Executing all integration phases");
    
    bool success = true;
    
    // Execute phases in order
    for (int phase = static_cast<int>(UEIntegrationPhase::PreInit);
         phase <= static_cast<int>(UEIntegrationPhase::Shutdown);
         ++phase) {
        
        UEIntegrationPhase currentPhase = static_cast<UEIntegrationPhase>(phase);
        
        // Initialize plugins for this phase
        for (auto* plugin : phasePlugins_[currentPhase]) {
            if (plugin->supportsPhase(currentPhase)) {
                if (!plugin->initialize(context_)) {
                    DiagnosticEngine::get().error("Plugin initialization failed: " + plugin->getName());
                    success = false;
                    continue;
                }
            }
        }
        
        // Execute phase
        if (!executePhase(currentPhase)) {
            success = false;
        }
        
        // Shutdown plugins for this phase
        for (auto* plugin : phasePlugins_[currentPhase]) {
            if (plugin->supportsPhase(currentPhase)) {
                if (!plugin->shutdown(context_)) {
                    DiagnosticEngine::get().error("Plugin shutdown failed: " + plugin->getName());
                    success = false;
                }
            }
        }
    }
    
    return success;
}

UEIntegrationPlugin* UEIntegrationManager::findPlugin(const std::string& name) const {
    auto it = std::find_if(plugins_.begin(), plugins_.end(),
        [&](const std::unique_ptr<UEIntegrationPlugin>& plugin) {
            return plugin->getName() == name;
        });
    
    if (it != plugins_.end()) {
        return it->get();
    }
    
    return nullptr;
}

std::vector<std::string> UEIntegrationManager::getActivePlugins() const {
    std::vector<std::string> activePlugins;
    
    for (const auto& plugin : plugins_) {
        activePlugins.push_back(plugin->getName());
    }
    
    return activePlugins;
}

bool UEIntegrationManager::loadPluginsFromDirectory(const std::string& directory) {
    DiagnosticEngine::get().info("Loading plugins from directory: " + directory);
    
    // In a real implementation, this would:
    // 1. Scan directory for plugin DLLs/shared libraries
    // 2. Load each library
    // 3. Find and instantiate plugin factories
    // 4. Register each plugin
    
    // For now, just register built-in plugins
    registerPlugin(std::make_unique<UEBlueprintPlugin>());
    registerPlugin(std::make_unique<UENetworkingPlugin>());
    registerPlugin(std::make_unique<UEAssetPlugin>());
    
    return true;
}

bool UEIntegrationManager::savePluginConfiguration() const {
    DiagnosticEngine::get().info("Saving plugin configuration");
    
    // This would save plugin configuration to a file
    // Including enabled/disabled state, settings, etc.
    
    return true;
}

void UEIntegrationManager::organizePluginsByPhase() {
    phasePlugins_.clear();
    
    for (const auto& plugin : plugins_) {
        for (int phase = static_cast<int>(UEIntegrationPhase::PreInit);
             phase <= static_cast<int>(UEIntegrationPhase::Shutdown);
             ++phase) {
            
            UEIntegrationPhase currentPhase = static_cast<UEIntegrationPhase>(phase);
            if (plugin->supportsPhase(currentPhase)) {
                phasePlugins_[currentPhase].push_back(plugin.get());
            }
        }
    }
    
    // Sort plugins by priority within each phase
    for (auto& [phase, plugins] : phasePlugins_) {
        // Sort by some priority criteria (could be based on plugin metadata)
        // For now, just maintain insertion order
    }
}

bool UEIntegrationManager::validatePluginDependencies() const {
    // Check for circular dependencies between plugins
    // Verify required plugins are present
    
    return true;
}

bool UEIntegrationManager::executePluginPhase(UEIntegrationPlugin* plugin, UEIntegrationPhase phase) {
    if (!plugin) {
        return false;
    }
    
    try {
        return plugin->execute(context_);
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Plugin execution failed: " + std::string(e.what()));
        return false;
    }
}

} // namespace Unreal
} // namespace kiwiLang