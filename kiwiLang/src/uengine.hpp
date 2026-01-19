#pragma once

#include "kiwiLang/Config.h"
#include "kiwiLang/Unreal/UnrealEngine.h"
#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Unreal/UEFunctions.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeSystem.h"
#include "kiwiLang/Unreal/Integration/UEProjectBuilder.h"
#include "kiwiLang/Unreal/Integration/UEModuleLoader.h"
#include "kiwiLang/Unreal/Integration/UEEditorIntegration.h"
#include <memory>
#include <string>

namespace kiwiLang {

class KIWI_UNREAL_API UEngine {
public:
    static UEngine& get();
    
    bool initialize(const std::string& projectPath);
    bool shutdown();
    
    bool createProject(const std::string& projectName,
                      const std::string& projectDir,
                      Unreal::UEPlatform platform = Unreal::UEPlatform::Win64);
    
    bool buildProject(Unreal::UEBuildConfiguration config = Unreal::UEBuildConfiguration::Development);
    
    void* createObject(const std::string& className);
    bool destroyObject(void* object, const std::string& className);
    
    Runtime::Value callFunction(const std::string& functionName,
                               void* object,
                               const std::vector<Runtime::Value>& args);
    
    Runtime::Value getProperty(const std::string& propertyName,
                              void* object);
    bool setProperty(const std::string& propertyName,
                    void* object,
                    const Runtime::Value& value);
    
    bool compileBlueprint(const std::string& blueprintPath);
    bool reloadBlueprint(const std::string& blueprintPath);
    
    bool startPlayInEditor();
    bool stopPlayInEditor();
    
    bool isEditorRunning() const;
    bool isPlayInEditorActive() const;
    
    Unreal::UnrealEngine* getEngine() { return engine_.get(); }
    Unreal::UObjectRegistry* getObjectRegistry() { return objectRegistry_.get(); }
    Unreal::UETypeSystem* getTypeSystem() { return typeSystem_.get(); }
    
private:
    UEngine();
    ~UEngine();
    
    static std::unique_ptr<UEngine> instance_;
    static std::mutex instanceMutex_;
    
    bool initialized_;
    std::string projectPath_;
    
    std::unique_ptr<Unreal::UnrealEngine> engine_;
    std::unique_ptr<Unreal::UETypeRegistry> typeRegistry_;
    std::unique_ptr<Unreal::UETypeSystem> typeSystem_;
    std::unique_ptr<Unreal::UObjectRegistry> objectRegistry_;
    std::unique_ptr<Unreal::UEProjectBuilder> projectBuilder_;
    std::unique_ptr<Unreal::UEModuleLoader> moduleLoader_;
    std::unique_ptr<Unreal::UEEditorIntegration> editorIntegration_;
    std::unique_ptr<Unreal::UEFunctionDispatcher> functionDispatcher_;
    std::unique_ptr<Unreal::UEFunctionResolver> functionResolver_;
    std::unique_ptr<Unreal::UEFunctionOptimizer> functionOptimizer_;
    std::unique_ptr<Unreal::UEFunctionProfiler> functionProfiler_;
    std::unique_ptr<Unreal::UEFunctionValidator> functionValidator_;
    
    bool initializeComponents();
    bool shutdownComponents();
    bool loadTypeSystem();
};

} // namespace kiwiLang