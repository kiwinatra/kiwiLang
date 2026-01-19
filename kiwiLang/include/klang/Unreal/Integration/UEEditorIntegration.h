#pragma once

#include "kiwiLang/Unreal/UnrealEngine.h"
#include "kiwiLang/AST/Module.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UObjectRegistry;
class UEProjectBuilder;
class UEModuleLoader;

enum class UEEditorToolType {
    Viewport,
    DetailsPanel,
    ContentBrowser,
    BlueprintEditor,
    LevelEditor,
    MaterialEditor,
    AnimationEditor,
    Custom
};

enum class UEEditorHookPoint {
    OnEditorInit,
    OnMapOpened,
    OnMapChanged,
    OnAssetOpened,
    OnAssetSaved,
    OnBlueprintCompiled,
    OnPlayInEditorStarted,
    OnPlayInEditorStopped,
    PreTick,
    PostTick,
    OnShutdown
};

struct UEEditorToolInfo {
    std::string name;
    UEEditorToolType type;
    std::string menuPath;
    std::string toolbarPath;
    std::string iconPath;
    std::function<void()> executeCallback;
    std::function<bool()> canExecuteCallback;
    std::function<void()> uiGenerator;
    std::uint32_t priority;
    bool isEnabled;
    bool isVisible;
    
    bool validate() const;
};

struct UEEditorHook {
    UEEditorHookPoint point;
    std::string name;
    std::function<void(void*)> callback;
    std::uint32_t priority;
    bool isPersistent;
    
    bool operator<(const UEEditorHook& other) const {
        return priority < other.priority;
    }
};

struct UEEditorWidgetInfo {
    std::string widgetName;
    std::string tabId;
    std::string displayName;
    std::function<void*(void*)> createWidgetCallback;
    std::function<void(void*)> destroyWidgetCallback;
    std::vector<std::string> allowedAreas;
    bool isStandaloneWindow;
    bool isToolbarWidget;
    std::uint32_t defaultWidth;
    std::uint32_t defaultHeight;
    
    bool validate() const;
};

class KIWI_UNREAL_API UEEditorIntegration {
public:
    explicit UEEditorIntegration(const UEConfig& config,
                                UETypeRegistry* typeRegistry = nullptr,
                                UObjectRegistry* objectRegistry = nullptr,
                                UEProjectBuilder* projectBuilder = nullptr,
                                UEModuleLoader* moduleLoader = nullptr);
    ~UEEditorIntegration();
    
    bool initialize();
    bool shutdown();
    
    bool registerTool(const UEEditorToolInfo& toolInfo);
    bool unregisterTool(const std::string& toolName);
    
    bool registerHook(const UEEditorHook& hook);
    bool unregisterHook(const std::string& hookName);
    
    bool registerWidget(const UEEditorWidgetInfo& widgetInfo);
    bool unregisterWidget(const std::string& widgetName);
    
    bool executeTool(const std::string& toolName);
    bool triggerHook(UEEditorHookPoint point, void* context = nullptr);
    
    void* createWidget(const std::string& widgetName, void* parent = nullptr);
    bool destroyWidget(const std::string& widgetName, void* widget);
    
    bool showNotification(const std::string& title,
                         const std::string& message,
                         float duration = 5.0f);
    
    bool openAsset(const std::string& assetPath);
    bool saveAsset(const std::string& assetPath);
    
    bool compileBlueprint(const std::string& blueprintPath);
    bool reloadBlueprint(const std::string& blueprintPath);
    
    bool startPlayInEditor();
    bool stopPlayInEditor();
    
    bool isEditorRunning() const;
    bool isPlayInEditorActive() const;
    
    std::vector<std::string> getRegisteredTools() const;
    std::vector<std::string> getRegisteredWidgets() const;
    
    bool generateEditorExtensions();
    bool generateToolMenuEntries();
    
    const UEConfig& getConfig() const { return config_; }
    
private:
    struct EditorState {
        bool isInitialized;
        bool isShuttingDown;
        bool playInEditorActive;
        std::chrono::steady_clock::time_point lastTickTime;
        std::uint64_t tickCount;
    };
    
    UEConfig config_;
    UETypeRegistry* typeRegistry_;
    UObjectRegistry* objectRegistry_;
    UEProjectBuilder* projectBuilder_;
    UEModuleLoader* moduleLoader_;
    EditorState state_;
    
    std::unordered_map<std::string, UEEditorToolInfo> tools_;
    std::unordered_map<UEEditorHookPoint, std::vector<UEEditorHook>> hooks_;
    std::unordered_map<std::string, UEEditorWidgetInfo> widgets_;
    std::unordered_map<std::string, std::vector<void*>> widgetInstances_;
    
    mutable std::recursive_mutex mutex_;
    
    bool initializeEditorModule();
    bool shutdownEditorModule();
    
    bool registerEditorDelegates();
    bool unregisterEditorDelegates();
    
    bool setupEditorMenus();
    bool cleanupEditorMenus();
    
    bool setupEditorToolbars();
    bool cleanupEditorToolbars();
    
    bool createEditorTab(const UEEditorWidgetInfo& widgetInfo);
    bool destroyEditorTab(const std::string& tabId);
    
    void executeHooksForPoint(UEEditorHookPoint point, void* context);
    
    bool generateEditorModuleFiles() const;
    bool generateToolSourceFiles() const;
    bool generateWidgetSourceFiles() const;
    
    bool registerWithEditorSubsystem(const std::string& subsystemName);
    bool unregisterFromEditorSubsystem(const std::string& subsystemName);
    
    bool validateEditorCompatibility() const;
    bool checkEditorVersion() const;
    
    void handleEditorTick(float deltaTime);
    void handleMapChanged(uint32_t mapChangeFlags);
    void handleAssetOpened(const std::string& assetPath);
    void handleAssetSaved(const std::string& assetPath);
    void handleBlueprintCompiled(const std::string& blueprintPath, bool succeeded);
    void handlePlayInEditorStarted();
    void handlePlayInEditorStopped();
    
    static void staticEditorTickCallback(float deltaTime, void* userData);
    static void staticMapChangedCallback(uint32_t flags, void* userData);
};

} // namespace Unreal
} // namespace kiwiLang