#include "kiwiLang/Unreal/Integration/UEEditorIntegration.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeSystem.h"
#include "kiwiLang/Unreal/Integration/UEProjectBuilder.h"
#include "kiwiLang/Unreal/Integration/UEModuleLoader.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <chrono>

namespace kiwiLang {
namespace Unreal {

UEEditorIntegration::UEEditorIntegration(const UEConfig& config,
                                        UETypeRegistry* typeRegistry,
                                        UObjectRegistry* objectRegistry,
                                        UEProjectBuilder* projectBuilder,
                                        UEModuleLoader* moduleLoader)
    : config_(config)
    , typeRegistry_(typeRegistry)
    , objectRegistry_(objectRegistry)
    , projectBuilder_(projectBuilder)
    , moduleLoader_(moduleLoader) {
    
    state_.isInitialized = false;
    state_.isShuttingDown = false;
    state_.playInEditorActive = false;
    state_.tickCount = 0;
}

UEEditorIntegration::~UEEditorIntegration() {
    shutdown();
}

bool UEEditorIntegration::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (state_.isInitialized) {
        return true;
    }
    
    DiagnosticEngine::get().info("Initializing UE Editor Integration");
    
    // Check if we're running in editor mode
    if (config_.targetPlatform != UEPlatform::Win64) {
        DiagnosticEngine::get().warning("Editor integration requires Win64 platform");
        return false;
    }
    
    //// Initialize editor module
    if (!initializeEditorModule()) {
        DiagnosticEngine::get().error("Failed to initialize editor module");
        return false;
    }
    
    // Register editor delegates
    if (!registerEditorDelegates()) {
        DiagnosticEngine::get().error("Failed to register editor delegates");
        return false;
    }
    
    // Setup editor menus
    if (!setupEditorMenus()) {
        DiagnosticEngine::get().warning("Failed to setup editor menus");
    }
    
    // Setup editor toolbars
    if (!setupEditorToolbars()) {
        DiagnosticEngine::get().warning("Failed to setup editor toolbars");
    }
    
    // Register with editor subsystems
    if (!registerWithEditorSubsystem("AssetEditorSubsystem")) {
        DiagnosticEngine::get().warning("Failed to register with asset editor subsystem");
    }
    
    // Validate editor compatibility
    if (!validateEditorCompatibility()) {
        DiagnosticEngine::get().warning("Editor compatibility check failed");
    }
    
    state_.isInitialized = true;
    state_.lastTickTime = std::chrono::steady_clock::now();
    
    DiagnosticEngine::get().info("UE Editor Integration initialized successfully");
    return true;
}

bool UEEditorIntegration::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized || state_.isShuttingDown) {
        return true;
    }
    
    state_.isShuttingDown = true;
    DiagnosticEngine::get().info("Shutting down UE Editor Integration");
    
    // Stop PIE if active
    if (state_.playInEditorActive) {
        stopPlayInEditor();
    }
    
    // Unregister editor delegates
    if (!unregisterEditorDelegates()) {
        DiagnosticEngine::get().warning("Failed to unregister editor delegates");
    }
    
    // Cleanup editor menus
    if (!cleanupEditorMenus()) {
        DiagnosticEngine::get().warning("Failed to cleanup editor menus");
    }
    
    // Cleanup editor toolbars
    if (!cleanupEditorToolbars()) {
        DiagnosticEngine::get().warning("Failed to cleanup editor toolbars");
    }
    
    // Unregister from editor subsystems
    if (!unregisterFromEditorSubsystem("AssetEditorSubsystem")) {
        DiagnosticEngine::get().warning("Failed to unregister from asset editor subsystem");
    }
    
    // Shutdown editor module
    if (!shutdownEditorModule()) {
        DiagnosticEngine::get().warning("Failed to shutdown editor module");
    }
    
    // Clear all registered items
    tools_.clear();
    hooks_.clear();
    widgets_.clear();
    
    for (auto& [widgetName, instances] : widgetInstances_) {
        for (void* widget : instances) {
            // Widgets should be destroyed by UE editor
        }
    }
    widgetInstances_.clear();
    
    state_.isInitialized = false;
    state_.isShuttingDown = false;
    
    DiagnosticEngine::get().info("UE Editor Integration shutdown complete");
    return true;
}

bool UEEditorIntegration::registerTool(const UEEditorToolInfo& toolInfo) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized) {
        DiagnosticEngine::get().error("Editor integration not initialized");
        return false;
    }
    
    if (!toolInfo.validate()) {
        DiagnosticEngine::get().error("Invalid tool info");
        return false;
    }
    
    if (tools_.find(toolInfo.name) != tools_.end()) {
        DiagnosticEngine::get().warning("Tool already registered: " + toolInfo.name);
        return false;
    }
    
    tools_[toolInfo.name] = toolInfo;
    
    // Register with editor menu system
    // This would involve creating menu entries and toolbar buttons
    
    DiagnosticEngine::get().info("Registered editor tool: " + toolInfo.name);
    return true;
}

bool UEEditorIntegration::unregisterTool(const std::string& toolName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = tools_.find(toolName);
    if (it == tools_.end()) {
        DiagnosticEngine::get().warning("Tool not found: " + toolName);
        return false;
    }
    
    // Remove from editor menu system
    // This would involve removing menu entries and toolbar buttons
    
    tools_.erase(it);
    
    DiagnosticEngine::get().info("Unregistered editor tool: " + toolName);
    return true;
}

bool UEEditorIntegration::registerHook(const UEEditorHook& hook) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized) {
        DiagnosticEngine::get().error("Editor integration not initialized");
        return false;
    }
    
    // Add hook to appropriate phase
    auto& phaseHooks = hooks_[hook.point];
    
    // Check for duplicate
    auto duplicate = std::find_if(phaseHooks.begin(), phaseHooks.end(),
        [&](const UEEditorHook& h) { return h.name == hook.name; });
    
    if (duplicate != phaseHooks.end()) {
        DiagnosticEngine::get().warning("Hook already registered: " + hook.name);
        return false;
    }
    
    phaseHooks.push_back(hook);
    
    // Sort by priority
    std::sort(phaseHooks.begin(), phaseHooks.end());
    
    DiagnosticEngine::get().info("Registered editor hook: " + hook.name + 
                                " for point: " + std::to_string(static_cast<int>(hook.point)));
    return true;
}

bool UEEditorIntegration::unregisterHook(const std::string& hookName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    bool found = false;
    
    for (auto& [point, phaseHooks] : hooks_) {
        auto it = std::find_if(phaseHooks.begin(), phaseHooks.end(),
            [&](const UEEditorHook& h) { return h.name == hookName; });
        
        if (it != phaseHooks.end()) {
            phaseHooks.erase(it);
            found = true;
            break;
        }
    }
    
    if (!found) {
        DiagnosticEngine::get().warning("Hook not found: " + hookName);
        return false;
    }
    
    DiagnosticEngine::get().info("Unregistered editor hook: " + hookName);
    return true;
}

bool UEEditorIntegration::registerWidget(const UEEditorWidgetInfo& widgetInfo) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized) {
        DiagnosticEngine::get().error("Editor integration not initialized");
        return false;
    }
    
    if (!widgetInfo.validate()) {
        DiagnosticEngine::get().error("Invalid widget info");
        return false;
    }
    
    if (widgets_.find(widgetInfo.widgetName) != widgets_.end()) {
        DiagnosticEngine::get().warning("Widget already registered: " + widgetInfo.widgetName);
        return false;
    }
    
    widgets_[widgetInfo.widgetName] = widgetInfo;
    
    // Create editor tab
    if (!createEditorTab(widgetInfo)) {
        DiagnosticEngine::get().warning("Failed to create editor tab for widget: " + widgetInfo.widgetName);
        // Continue anyway, widget might be created lazily
    }
    
    DiagnosticEngine::get().info("Registered editor widget: " + widgetInfo.widgetName);
    return true;
}

bool UEEditorIntegration::unregisterWidget(const std::string& widgetName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = widgets_.find(widgetName);
    if (it == widgets_.end()) {
        DiagnosticEngine::get().warning("Widget not found: " + widgetName);
        return false;
    }
    
    // Destroy editor tab
    if (!destroyEditorTab(it->second.tabId)) {
        DiagnosticEngine::get().warning("Failed to destroy editor tab for widget: " + widgetName);
    }
    
    // Destroy any existing widget instances
    auto instancesIt = widgetInstances_.find(widgetName);
    if (instancesIt != widgetInstances_.end()) {
        for (void* widget : instancesIt->second) {
            if (it->second.destroyWidgetCallback) {
                it->second.destroyWidgetCallback(widget);
            }
        }
        widgetInstances_.erase(instancesIt);
    }
    
    widgets_.erase(it);
    
    DiagnosticEngine::get().info("Unregistered editor widget: " + widgetName);
    return true;
}

bool UEEditorIntegration::executeTool(const std::string& toolName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = tools_.find(toolName);
    if (it == tools_.end()) {
        DiagnosticEngine::get().error("Tool not found: " + toolName);
        return false;
    }
    
    const auto& toolInfo = it->second;
    
    // Check if tool can be executed
    if (toolInfo.canExecuteCallback && !toolInfo.canExecuteCallback()) {
        DiagnosticEngine::get().warning("Tool cannot be executed: " + toolName);
        return false;
    }
    
    // Execute tool
    DiagnosticEngine::get().info("Executing editor tool: " + toolName);
    
    try {
        if (toolInfo.executeCallback) {
            toolInfo.executeCallback();
        }
        
        // Generate UI if needed
        if (toolInfo.uiGenerator) {
            toolInfo.uiGenerator();
        }
        
        return true;
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Tool execution failed: " + std::string(e.what()));
        return false;
    }
}

bool UEEditorIntegration::triggerHook(UEEditorHookPoint point, void* context) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized) {
        return false;
    }
    
    auto it = hooks_.find(point);
    if (it == hooks_.end()) {
        return true; // No hooks for this point
    }
    
    DiagnosticEngine::get().info("Triggering editor hook point: " + 
        std::to_string(static_cast<int>(point)));
    
    executeHooksForPoint(point, context);
    return true;
}

void* UEEditorIntegration::createWidget(const std::string& widgetName, void* parent) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = widgets_.find(widgetName);
    if (it == widgets_.end()) {
        DiagnosticEngine::get().error("Widget not found: " + widgetName);
        return nullptr;
    }
    
    const auto& widgetInfo = it->second;
    
    // Create widget instance
    void* widget = nullptr;
    if (widgetInfo.createWidgetCallback) {
        widget = widgetInfo.createWidgetCallback(parent);
    }
    
    if (widget) {
        widgetInstances_[widgetName].push_back(widget);
        DiagnosticEngine::get().info("Created widget instance: " + widgetName);
    } else {
        DiagnosticEngine::get().warning("Failed to create widget: " + widgetName);
    }
    
    return widget;
}

bool UEEditorIntegration::destroyWidget(const std::string& widgetName, void* widget) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto instancesIt = widgetInstances_.find(widgetName);
    if (instancesIt == widgetInstances_.end()) {
        DiagnosticEngine::get().warning("No widget instances found: " + widgetName);
        return false;
    }
    
    auto widgetIt = std::find(instancesIt->second.begin(), instancesIt->second.end(), widget);
    if (widgetIt == instancesIt->second.end()) {
        DiagnosticEngine::get().warning("Widget instance not found: " + widgetName);
        return false;
    }
    
    // Destroy widget
    auto widgetsIt = widgets_.find(widgetName);
    if (widgetsIt != widgets_.end() && widgetsIt->second.destroyWidgetCallback) {
        widgetsIt->second.destroyWidgetCallback(widget);
    }
    
    instancesIt->second.erase(widgetIt);
    
    if (instancesIt->second.empty()) {
        widgetInstances_.erase(instancesIt);
    }
    
    DiagnosticEngine::get().info("Destroyed widget instance: " + widgetName);
    return true;
}

bool UEEditorIntegration::showNotification(const std::string& title,
                                          const std::string& message,
                                          float duration) {
    if (!state_.isInitialized) {
        return false;
    }
    
    // In UE5, this would use FNotificationInfo
    DiagnosticEngine::get().info("Editor notification: " + title + " - " + message);
    
    return true;
}

bool UEEditorIntegration::openAsset(const std::string& assetPath) {
    if (!state_.isInitialized) {
        return false;
    }
    
    DiagnosticEngine::get().info("Opening asset: " + assetPath);
    
    // Trigger asset opened hook
    triggerHook(UEEditorHookPoint::OnAssetOpened, const_cast<char*>(assetPath.c_str()));
    
    return true;
}

bool UEEditorIntegration::saveAsset(const std::string& assetPath) {
    if (!state_.isInitialized) {
        return false;
    }
    
    DiagnosticEngine::get().info("Saving asset: " + assetPath);
    
    // Trigger asset saved hook
    triggerHook(UEEditorHookPoint::OnAssetSaved, const_cast<char*>(assetPath.c_str()));
    
    return true;
}

bool UEEditorIntegration::compileBlueprint(const std::string& blueprintPath) {
    if (!state_.isInitialized) {
        return false;
    }
    
    DiagnosticEngine::get().info("Compiling blueprint: " + blueprintPath);
    
    // Trigger blueprint compiled hook
    triggerHook(UEEditorHookPoint::OnBlueprintCompiled, const_cast<char*>(blueprintPath.c_str()));
    
    return true;
}

bool UEEditorIntegration::reloadBlueprint(const std::string& blueprintPath) {
    if (!state_.isInitialized) {
        return false;
    }
    
    DiagnosticEngine::get().info("Reloading blueprint: " + blueprintPath);
    
    // Unload then load the blueprint
    if (moduleLoader_) {
        std::string moduleName = std::filesystem::path(blueprintPath).stem().string();
        return moduleLoader_->reloadModule(moduleName);
    }
    
    return false;
}

bool UEEditorIntegration::startPlayInEditor() {
    if (!state_.isInitialized) {
        return false;
    }
    
    if (state_.playInEditorActive) {
        DiagnosticEngine::get().warning("Play in Editor already active");
        return false;
    }
    
    DiagnosticEngine::get().info("Starting Play in Editor");
    
    // Trigger PIE started hook
    triggerHook(UEEditorHookPoint::OnPlayInEditorStarted, nullptr);
    
    state_.playInEditorActive = true;
    return true;
}

bool UEEditorIntegration::stopPlayInEditor() {
    if (!state_.isInitialized) {
        return false;
    }
    
    if (!state_.playInEditorActive) {
        DiagnosticEngine::get().warning("Play in Editor not active");
        return false;
    }
    
    DiagnosticEngine::get().info("Stopping Play in Editor");
    
    // Trigger PIE stopped hook
    triggerHook(UEEditorHookPoint::OnPlayInEditorStopped, nullptr);
    
    state_.playInEditorActive = false;
    return true;
}

bool UEEditorIntegration::isEditorRunning() const {
    return state_.isInitialized && !state_.isShuttingDown;
}

bool UEEditorIntegration::isPlayInEditorActive() const {
    return state_.playInEditorActive;
}

std::vector<std::string> UEEditorIntegration::getRegisteredTools() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> toolNames;
    toolNames.reserve(tools_.size());
    
    for (const auto& [name, _] : tools_) {
        toolNames.push_back(name);
    }
    
    return toolNames;
}

std::vector<std::string> UEEditorIntegration::getRegisteredWidgets() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::vector<std::string> widgetNames;
    widgetNames.reserve(widgets_.size());
    
    for (const auto& [name, _] : widgets_) {
        widgetNames.push_back(name);
    }
    
    return widgetNames;
}

bool UEEditorIntegration::generateEditorExtensions() {
    if (!state_.isInitialized) {
        return false;
    }
    
    DiagnosticEngine::get().info("Generating editor extensions");
    
    // Generate editor module files
    if (!generateEditorModuleFiles()) {
        DiagnosticEngine::get().error("Failed to generate editor module files");
        return false;
    }
    
    // Generate tool source files
    if (!generateToolSourceFiles()) {
        DiagnosticEngine::get().error("Failed to generate tool source files");
        return false;
    }
    
    // Generate widget source files
    if (!generateWidgetSourceFiles()) {
        DiagnosticEngine::get().error("Failed to generate widget source files");
        return false;
    }
    
    // Generate menu entries
    if (!generateToolMenuEntries()) {
        DiagnosticEngine::get().warning("Failed to generate tool menu entries");
    }
    
    return true;
}

bool UEEditorIntegration::generateToolMenuEntries() {
    if (!projectBuilder_) {
        return false;
    }
    
    // Generate menu entries for all registered tools
    for (const auto& [toolName, toolInfo] : tools_) {
        // Create menu entry source file
        std::string className = "F" + toolName + "MenuEntry";
        
        std::string content = "#pragma once\n\n";
        content += "#include \"Framework/Commands/Commands.h\"\n";
        content += "#include \"Styling/AppStyle.h\"\n\n";
        content += "class " + className + " : public TCommands<" + className + ">\n";
        content += "{\n";
        content += "public:\n";
        content += "    " + className + "()\n";
        content += "        : TCommands<" + className + ">(FName(TEXT(\"" + toolName + "\")), \n";
        content += "          FText::FromString(TEXT(\"" + toolInfo.menuPath + "\")), \n";
        content += "          NAME_None, \n";
        content += "          FAppStyle::GetAppStyleSetName())\n";
        content += "    {}\n\n";
        content += "    virtual void RegisterCommands() override;\n";
        content += "    TSharedPtr<FUICommandInfo> CommandInfo;\n";
        content += "};\n";
        
        std::filesystem::path filePath = config_.intermediatePath + "/Generated/Editor/" + 
                                         toolName + "MenuEntry.h";
        
        if (!projectBuilder_->addGeneratedFile(filePath, content, "Editor", true)) {
            return false;
        }
    }
    
    return true;
}

bool UEEditorIntegration::initializeEditorModule() {
    // Load editor module
    if (moduleLoader_) {
        auto result = moduleLoader_->loadModule("UnrealEd", UEModuleLoadPhase::PostEngineInit);
        if (!result.success) {
            DiagnosticEngine::get().error("Failed to load UnrealEd module");
            return false;
        }
    }
    
    return true;
}

bool UEEditorIntegration::shutdownEditorModule() {
    if (moduleLoader_) {
        return moduleLoader_->unloadModule("UnrealEd");
    }
    
    return true;
}

bool UEEditorIntegration::registerEditorDelegates() {
    // Register tick delegate
    // In UE5: FEditorDelegates::OnEditorTick().AddRaw(this, &UEEditorIntegration::handleEditorTick);
    
    // Register map change delegate
    // In UE5: FEditorDelegates::OnMapOpened().AddRaw(this, &UEEditorIntegration::handleMapChanged);
    
    return true;
}

bool UEEditorIntegration::unregisterEditorDelegates() {
    // Unregister all delegates
    return true;
}

bool UEEditorIntegration::setupEditorMenus() {
    // Create main menu entries
    // In UE5: FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    // LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
    
    return true;
}

bool UEEditorIntegration::cleanupEditorMenus() {
    // Remove all menu entries
    return true;
}

bool UEEditorIntegration::setupEditorToolbars() {
    // Create toolbar buttons
    // In UE5: LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
    
    return true;
}

bool UEEditorIntegration::cleanupEditorToolbars() {
    // Remove all toolbar buttons
    return true;
}

bool UEEditorIntegration::createEditorTab(const UEEditorWidgetInfo& widgetInfo) {
    // Register tab spawner
    // In UE5: FGlobalTabmanager::Get()->RegisterNomadTabSpawner(...);
    
    return true;
}

bool UEEditorIntegration::destroyEditorTab(const std::string& tabId) {
    // Unregister tab spawner
    // In UE5: FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FName(*tabId));
    
    return true;
}

void UEEditorIntegration::executeHooksForPoint(UEEditorHookPoint point, void* context) {
    auto it = hooks_.find(point);
    if (it == hooks_.end()) {
        return;
    }
    
    for (const auto& hook : it->second) {
        try {
            hook.callback(context);
        } catch (const std::exception& e) {
            DiagnosticEngine::get().error("Hook execution failed: " + hook.name + 
                                         " - " + std::string(e.what()));
        }
    }
}

bool UEEditorIntegration::generateEditorModuleFiles() const {
    if (!projectBuilder_) {
        return false;
    }
    
    // Generate editor module .h file
    std::string headerContent = "#pragma once\n\n";
    headerContent += "#include \"CoreMinimal.h\"\n";
    headerContent += "#include \"Modules/ModuleManager.h\"\n\n";
    headerContent += "class FKiwiLangEditorModule : public IModuleInterface\n";
    headerContent += "{\n";
    headerContent += "public:\n";
    headerContent += "    virtual void StartupModule() override;\n";
    headerContent += "    virtual void ShutdownModule() override;\n";
    headerContent += "private:\n";
    headerContent += "    void RegisterMenus();\n";
    headerContent += "    void UnregisterMenus();\n";
    headerContent += "};\n";
    
    std::filesystem::path headerPath = config_.intermediatePath + "/Generated/Editor/KiwiLangEditorModule.h";
    
    if (!projectBuilder_->addGeneratedFile(headerPath, headerContent, "Editor", true)) {
        return false;
    }
    
    // Generate editor module .cpp file
    std::string sourceContent = "#include \"KiwiLangEditorModule.h\"\n";
    sourceContent += "#include \"LevelEditor.h\"\n";
    sourceContent += "#include \"Framework/MultiBox/MultiBoxBuilder.h\"\n\n";
    sourceContent += "#define LOCTEXT_NAMESPACE \"FKiwiLangEditorModule\"\n\n";
    sourceContent += "void FKiwiLangEditorModule::StartupModule()\n";
    sourceContent += "{\n";
    sourceContent += "    RegisterMenus();\n";
    sourceContent += "}\n\n";
    sourceContent += "void FKiwiLangEditorModule::ShutdownModule()\n";
    sourceContent += "{\n";
    sourceContent += "    UnregisterMenus();\n";
    sourceContent += "}\n\n";
    sourceContent += "void FKiwiLangEditorModule::RegisterMenus()\n";
    sourceContent += "{\n";
    sourceContent += "    // Register menu extensions\n";
    sourceContent += "}\n\n";
    sourceContent += "void FKiwiLangEditorModule::UnregisterMenus()\n";
    sourceContent += "{\n";
    sourceContent += "    // Unregister menu extensions\n";
    sourceContent += "}\n\n";
    sourceContent += "#undef LOCTEXT_NAMESPACE\n\n";
    sourceContent += "IMPLEMENT_MODULE(FKiwiLangEditorModule, KiwiLangEditor);\n";
    
    std::filesystem::path sourcePath = config_.intermediatePath + "/Generated/Editor/KiwiLangEditorModule.cpp";
    
    return projectBuilder_->addGeneratedFile(sourcePath, sourceContent, "Editor", false);
}

bool UEEditorIntegration::generateToolSourceFiles() const {
    if (!projectBuilder_) {
        return false;
    }
    
    // Generate source files for each tool
    for (const auto& [toolName, toolInfo] : tools_) {
        std::string className = "F" + toolName + "Tool";
        
        std::string headerContent = "#pragma once\n\n";
        headerContent += "#include \"CoreMinimal.h\"\n";
        headerContent += "#include \"Framework/Commands/Commands.h\"\n";
        headerContent += "#include \"" + toolName + "MenuEntry.h\"\n\n";
        headerContent += "class " + className + "\n";
        headerContent += "{\n";
        headerContent += "public:\n";
        headerContent += "    " + className + "();\n";
        headerContent += "    void Execute();\n";
        headerContent += "};\n";
        
        std::filesystem::path headerPath = config_.intermediatePath + "/Generated/Editor/Tools/" + 
                                           toolName + "Tool.h";
        
        if (!projectBuilder_->addGeneratedFile(headerPath, headerContent, "Editor", true)) {
            return false;
        }
    }
    
    return true;
}

bool UEEditorIntegration::generateWidgetSourceFiles() const {
    if (!projectBuilder_) {
        return false;
    }
    
    // Generate source files for each widget
    for (const auto& [widgetName, widgetInfo] : widgets_) {
        std::string className = "S" + widgetName + "Widget";
        
        std::string headerContent = "#pragma once\n\n";
        headerContent += "#include \"CoreMinimal.h\"\n";
        headerContent += "#include \"Widgets/SCompoundWidget.h\"\n\n";
        headerContent += "class " + className + " : public SCompoundWidget\n";
        headerContent += "{\n";
        headerContent += "public:\n";
        headerContent += "    SLATE_BEGIN_ARGS(" + className + ") {}\n";
        headerContent += "    SLATE_END_ARGS()\n\n";
        headerContent += "    void Construct(const FArguments& InArgs);\n";
        headerContent += "};\n";
        
        std::filesystem::path headerPath = config_.intermediatePath + "/Generated/Editor/Widgets/" + 
                                           widgetName + "Widget.h";
        
        if (!projectBuilder_->addGeneratedFile(headerPath, headerContent, "Editor", true)) {
            return false;
        }
    }
    
    return true;
}

bool UEEditorIntegration::registerWithEditorSubsystem(const std::string& subsystemName) {
    // Register with editor subsystem
    // In UE5: GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->Register...
    
    return true;
}

bool UEEditorIntegration::unregisterFromEditorSubsystem(const std::string& subsystemName) {
    // Unregister from editor subsystem
    return true;
}

bool UEEditorIntegration::validateEditorCompatibility() const {
    // Check editor version
    if (!checkEditorVersion()) {
        DiagnosticEngine::get().warning("Editor version check failed");
        return false;
    }
    
    return true;
}

bool UEEditorIntegration::checkEditorVersion() const {
    // Check if we're compatible with current editor version
    // In UE5: FEngineVersion::Current().GetMajor() >= 5
    
    return true;
}

void UEEditorIntegration::handleEditorTick(float deltaTime) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!state_.isInitialized || state_.isShuttingDown) {
        return;
    }
    
    state_.tickCount++;
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - state_.lastTickTime);
    
    // Trigger pre-tick hooks
    triggerHook(UEEditorHookPoint::PreTick, &deltaTime);
    
    // Update state
    state_.lastTickTime = currentTime;
    
    // Trigger post-tick hooks
    triggerHook(UEEditorHookPoint::PostTick, &deltaTime);
}

void UEEditorIntegration::handleMapChanged(uint32_t mapChangeFlags) {
    triggerHook(UEEditorHookPoint::OnMapChanged, &mapChangeFlags);
}

void UEEditorIntegration::handleAssetOpened(const std::string& assetPath) {
    triggerHook(UEEditorHookPoint::OnAssetOpened, const_cast<char*>(assetPath.c_str()));
}

void UEEditorIntegration::handleAssetSaved(const std::string& assetPath) {
    triggerHook(UEEditorHookPoint::OnAssetSaved, const_cast<char*>(assetPath.c_str()));
}

void UEEditorIntegration::handleBlueprintCompiled(const std::string& blueprintPath, bool succeeded) {
    struct BlueprintCompileResult {
        const char* path;
        bool success;
    };
    
    BlueprintCompileResult result{blueprintPath.c_str(), succeeded};
    triggerHook(UEEditorHookPoint::OnBlueprintCompiled, &result);
}

void UEEditorIntegration::handlePlayInEditorStarted() {
    state_.playInEditorActive = true;
    triggerHook(UEEditorHookPoint::OnPlayInEditorStarted, nullptr);
}

void UEEditorIntegration::handlePlayInEditorStopped() {
    state_.playInEditorActive = false;
    triggerHook(UEEditorHookPoint::OnPlayInEditorStopped, nullptr);
}

void UEEditorIntegration::staticEditorTickCallback(float deltaTime, void* userData) {
    auto* integration = static_cast<UEEditorIntegration*>(userData);
    if (integration) {
        integration->handleEditorTick(deltaTime);
    }
}

void UEEditorIntegration::staticMapChangedCallback(uint32_t flags, void* userData) {
    auto* integration = static_cast<UEEditorIntegration*>(userData);
    if (integration) {
        integration->handleMapChanged(flags);
    }
}

} // namespace Unreal
} // namespace kiwiLang