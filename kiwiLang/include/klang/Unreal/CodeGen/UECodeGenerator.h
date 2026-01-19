#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/CodeGen/IRBuilder.h"
#include "kiwiLang/CodeGen/ModuleBuilder.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UObjectRegistry;
class UETypeConverter;
struct UEBlueprintClassInfo;

enum class UECodeGenTarget {
    CppHeader,
    CppSource,
    Blueprint,
    ReflectionData,
    Serialization,
    Replication,
    RPCStubs,
    EditorExtensions
};

enum class UECodeGenFlags : std::uint32_t {
    None = 0,
    GenerateInline = 1 << 0,
    GenerateExportMacros = 1 << 1,
    GenerateVirtualFunctions = 1 << 2,
    GeneratePureVirtualStubs = 1 << 3,
    GenerateBlueprintCallableWrappers = 1 << 4,
    GenerateEditorOnlyCode = 1 << 5,
    GenerateDebugSymbols = 1 << 6,
    GenerateProfileInstrumentation = 1 << 7,
    GenerateHotReloadSupport = 1 << 8,
    Default = GenerateExportMacros | GenerateVirtualFunctions | GenerateBlueprintCallableWrappers
};

struct UECodeGenConfig {
    UECodeGenTarget target;
    UECodeGenFlags flags;
    std::string outputDirectory;
    std::string moduleName;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::unordered_map<std::string, std::string> templateParameters;
    
    bool shouldGenerateForTarget(UECodeGenTarget checkTarget) const {
        return target == checkTarget;
    }
    
    bool hasFlag(UECodeGenFlags flag) const {
        return (flags & flag) != UECodeGenFlags::None;
    }
};

struct UECodeGenResult {
    bool success;
    std::vector<std::string> generatedFiles;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::chrono::milliseconds generationTime;
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
};

class KIWI_UNREAL_API UECodeGenerator {
public:
    explicit UECodeGenerator(CodeGen::IRBuilder& irBuilder,
                            CodeGen::ModuleBuilder& moduleBuilder,
                            UETypeRegistry* typeRegistry = nullptr,
                            UObjectRegistry* objectRegistry = nullptr,
                            UETypeConverter* typeConverter = nullptr);
    ~UECodeGenerator();
    
    UECodeGenResult generateClassCode(const UEClassInfo& classInfo,
                                     const UECodeGenConfig& config);
    
    UECodeGenResult generateFunctionCode(const UEFunctionInfo& functionInfo,
                                        const UECodeGenConfig& config);
    
    UECodeGenResult generatePropertyCode(const UEPropertyInfo& propertyInfo,
                                        const UECodeGenConfig& config);
    
    UECodeGenResult generateBlueprintCode(const UEBlueprintClassInfo& blueprintInfo,
                                         const UECodeGenConfig& config);
    
    UECodeGenResult generateModuleCode(const std::string& moduleName,
                                      const UECodeGenConfig& config);
    
    UECodeGenResult generateAllCode(const UECodeGenConfig& config);
    
    bool registerCodeTemplate(const std::string& templateName,
                             const std::string& templateContent);
    
    bool unregisterCodeTemplate(const std::string& templateName);
    
    std::string expandTemplate(const std::string& templateName,
                              const std::unordered_map<std::string, std::string>& parameters) const;
    
    const CodeGen::IRBuilder& getIRBuilder() const { return irBuilder_; }
    const CodeGen::ModuleBuilder& getModuleBuilder() const { return moduleBuilder_; }
    
private:
    CodeGen::IRBuilder& irBuilder_;
    CodeGen::ModuleBuilder& moduleBuilder_;
    UETypeRegistry* typeRegistry_;
    UObjectRegistry* objectRegistry_;
    UETypeConverter* typeConverter_;
    
    std::unordered_map<std::string, std::string> codeTemplates_;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> templateProcessors_;
    
    void initializeDefaultTemplates();
    void initializeTemplateProcessors();
    
    UECodeGenResult generateClassHeader(const UEClassInfo& classInfo,
                                       const UECodeGenConfig& config);
    
    UECodeGenResult generateClassSource(const UEClassInfo& classInfo,
                                       const UECodeGenConfig& config);
    
    UECodeGenResult generateClassBlueprintWrapper(const UEClassInfo& classInfo,
                                                 const UECodeGenConfig& config);
    
    UECodeGenResult generateFunctionHeader(const UEFunctionInfo& functionInfo,
                                          const UECodeGenConfig& config);
    
    UECodeGenResult generateFunctionSource(const UEFunctionInfo& functionInfo,
                                          const UECodeGenConfig& config);
    
    UECodeGenResult generateFunctionThunk(const UEFunctionInfo& functionInfo,
                                         const UECodeGenConfig& config);
    
    UECodeGenResult generatePropertyHeader(const UEPropertyInfo& propertyInfo,
                                          const UECodeGenConfig& config);
    
    UECodeGenResult generatePropertySource(const UEPropertyInfo& propertyInfo,
                                          const UECodeGenConfig& config);
    
    UECodeGenResult generatePropertyAccessors(const UEPropertyInfo& propertyInfo,
                                             const UECodeGenConfig& config);
    
    UECodeGenResult generateBlueprintEventGraph(const UEBlueprintClassInfo& blueprintInfo,
                                               const UECodeGenConfig& config);
    
    UECodeGenResult generateBlueprintVariables(const UEBlueprintClassInfo& blueprintInfo,
                                              const UECodeGenConfig& config);
    
    UECodeGenResult generateBlueprintFunctions(const UEBlueprintClassInfo& blueprintInfo,
                                              const UECodeGenConfig& config);
    
    UECodeGenResult generateModuleHeader(const std::string& moduleName,
                                        const UECodeGenConfig& config);
    
    UECodeGenResult generateModuleSource(const std::string& moduleName,
                                        const UECodeGenConfig& config);
    
    UECodeGenResult generateModuleBuildFiles(const std::string& moduleName,
                                            const UECodeGenConfig& config);
    
    std::string processClassTemplate(const std::string& templateContent,
                                    const UEClassInfo& classInfo,
                                    const UECodeGenConfig& config) const;
    
    std::string processFunctionTemplate(const std::string& templateContent,
                                       const UEFunctionInfo& functionInfo,
                                       const UECodeGenConfig& config) const;
    
    std::string processPropertyTemplate(const std::string& templateContent,
                                       const UEPropertyInfo& propertyInfo,
                                       const UECodeGenConfig& config) const;
    
    std::string processBlueprintTemplate(const std::string& templateContent,
                                        const UEBlueprintClassInfo& blueprintInfo,
                                        const UECodeGenConfig& config) const;
    
    std::string processModuleTemplate(const std::string& templateContent,
                                     const std::string& moduleName,
                                     const UECodeGenConfig& config) const;
    
    bool validateClassForCodeGeneration(const UEClassInfo& classInfo) const;
    bool validateFunctionForCodeGeneration(const UEFunctionInfo& functionInfo) const;
    bool validatePropertyForCodeGeneration(const UEPropertyInfo& propertyInfo) const;
    bool validateBlueprintForCodeGeneration(const UEBlueprintClassInfo& blueprintInfo) const;
    
    std::string generateClassUHTMacros(const UEClassInfo& classInfo) const;
    std::string generateFunctionUHTMacros(const UEFunctionInfo& functionInfo) const;
    std::string generatePropertyUHTMacros(const UEPropertyInfo& propertyInfo) const;
    
    std::string generateClassReflectionData(const UEClassInfo& classInfo) const;
    std::string generateFunctionReflectionData(const UEFunctionInfo& functionInfo) const;
    std::string generatePropertyReflectionData(const UEPropertyInfo& propertyInfo) const;
    
    std::string generateClassSerializationCode(const UEClassInfo& classInfo) const;
    std::string generateFunctionReplicationCode(const UEFunctionInfo& functionInfo) const;
    std::string generatePropertyReplicationCode(const UEPropertyInfo& propertyInfo) const;
    
    std::string generateIncludeGuards(const std::string& fileName) const;
    std::string generateExportMacros(const std::string& symbolName,
                                    const UECodeGenConfig& config) const;
    
    friend class UECodeGeneratorImpl;
};

} // namespace Unreal
} // namespace kiwiLang