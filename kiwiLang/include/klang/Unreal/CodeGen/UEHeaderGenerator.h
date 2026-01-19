// hnr
#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/AST/Decl.h"
#include "kiwiLang/CodeGen/ModuleBuilder.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UETypeConverter;
class UECodeGenerator;
struct UEClassInfo;
struct UEFunctionInfo;
struct UEPropertyInfo;

enum class UEHeaderType {
    ClassDeclaration,
    StructDeclaration,
    EnumDeclaration,
    FunctionDeclaration,
    PropertyDeclaration,
    DelegateDeclaration,
    InterfaceDeclaration,
    ModuleHeader,
    GeneratedHeader,
    PCHHeader
};

enum class UEHeaderGenFlags : std::uint32_t {
    None = 0,
    GenerateUHTMacros = 1 << 0,
    GenerateExportMacros = 1 << 1,
    GenerateInlineDefinitions = 1 << 2,
    GenerateCommentDocumentation = 1 << 3,
    GenerateBlueprintTypeDeclarations = 1 << 4,
    GenerateEditorOnlyDeclarations = 1 << 5,
    GenerateDeprecatedDeclarations = 1 << 6,
    GenerateForwardDeclarations = 1 << 7,
    GenerateIncludeGuards = 1 << 8,
    Default = GenerateUHTMacros | GenerateExportMacros | GenerateIncludeGuards
};

struct UEHeaderGenConfig {
    UEHeaderType headerType;
    UEHeaderGenFlags flags;
    std::string fileName;
    std::string moduleName;
    std::vector<std::string> includePaths;
    std::vector<std::string> forwardDeclarations;
    std::unordered_map<std::string, std::string> macroDefinitions;
    
    bool hasFlag(UEHeaderGenFlags flag) const {
        return (flags & flag) != UEHeaderGenFlags::None;
    }
    
    std::string getIncludeGuard() const;
};

struct UEHeaderGenResult {
    bool success;
    std::string generatedHeader;
    std::vector<std::string> dependencies;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
};

class KIWI_UNREAL_API UEHeaderGenerator {
public:
    explicit UEHeaderGenerator(CodeGen::ModuleBuilder& moduleBuilder,
                              UETypeRegistry* typeRegistry = nullptr,
                              UETypeConverter* typeConverter = nullptr,
                              UECodeGenerator* codeGenerator = nullptr);
    ~UEHeaderGenerator();
    
    UEHeaderGenResult generateClassHeader(const UEClassInfo& classInfo,
                                         const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateStructHeader(const std::string& structName,
                                          AST::TypePtr kiwiType,
                                          const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateEnumHeader(const std::string& enumName,
                                        AST::TypePtr kiwiType,
                                        const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateFunctionHeader(const UEFunctionInfo& functionInfo,
                                            const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generatePropertyHeader(const UEPropertyInfo& propertyInfo,
                                            const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateDelegateHeader(const std::string& delegateType,
                                            const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateInterfaceHeader(const std::string& interfaceName,
                                             const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateModuleHeader(const std::string& moduleName,
                                          const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generatePCHHeader(const std::vector<std::string>& modules,
                                       const UEHeaderGenConfig& config);
    
    bool registerHeaderTemplate(const std::string& templateName,
                               const std::string& templateContent);
    
    bool unregisterHeaderTemplate(const std::string& templateName);
    
    std::string expandHeaderTemplate(const std::string& templateName,
                                    const std::unordered_map<std::string, std::string>& parameters) const;
    
    const CodeGen::ModuleBuilder& getModuleBuilder() const { return moduleBuilder_; }
    
private:
    CodeGen::ModuleBuilder& moduleBuilder_;
    UETypeRegistry* typeRegistry_;
    UETypeConverter* typeConverter_;
    UECodeGenerator* codeGenerator_;
    
    std::unordered_map<std::string, std::string> headerTemplates_;
    std::unordered_map<UEHeaderType, std::function<UEHeaderGenResult(const UEHeaderGenConfig&)>> headerGenerators_;
    
    void initializeDefaultTemplates();
    void initializeHeaderGenerators();
    
    UEHeaderGenResult generateClassDeclaration(const UEClassInfo& classInfo,
                                              const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateClassUHTMacros(const UEClassInfo& classInfo,
                                            const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateClassBlueprintMacros(const UEClassInfo& classInfo,
                                                  const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateStructDeclaration(const std::string& structName,
                                               AST::TypePtr kiwiType,
                                               const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateStructUHTMacros(const std::string& structName,
                                             AST::TypePtr kiwiType,
                                             const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateEnumDeclaration(const std::string& enumName,
                                             AST::TypePtr kiwiType,
                                             const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateEnumUHTMacros(const std::string& enumName,
                                           AST::TypePtr kiwiType,
                                           const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateFunctionDeclaration(const UEFunctionInfo& functionInfo,
                                                 const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateFunctionUHTMacros(const UEFunctionInfo& functionInfo,
                                               const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generatePropertyDeclaration(const UEPropertyInfo& propertyInfo,
                                                 const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generatePropertyUHTMacros(const UEPropertyInfo& propertyInfo,
                                               const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateDelegateDeclaration(const std::string& delegateType,
                                                 const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateInterfaceDeclaration(const std::string& interfaceName,
                                                  const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generateModuleDeclaration(const std::string& moduleName,
                                               const UEHeaderGenConfig& config);
    
    UEHeaderGenResult generatePCHDeclaration(const std::vector<std::string>& modules,
                                            const UEHeaderGenConfig& config);
    
    std::string processClassHeaderTemplate(const std::string& templateContent,
                                          const UEClassInfo& classInfo,
                                          const UEHeaderGenConfig& config) const;
    
    std::string processStructHeaderTemplate(const std::string& templateContent,
                                           const std::string& structName,
                                           AST::TypePtr kiwiType,
                                           const UEHeaderGenConfig& config) const;
    
    std::string processEnumHeaderTemplate(const std::string& templateContent,
                                         const std::string& enumName,
                                         AST::TypePtr kiwiType,
                                         const UEHeaderGenConfig& config) const;
    
    std::string processFunctionHeaderTemplate(const std::string& templateContent,
                                             const UEFunctionInfo& functionInfo,
                                             const UEHeaderGenConfig& config) const;
    
    std::string processPropertyHeaderTemplate(const std::string& templateContent,
                                             const UEPropertyInfo& propertyInfo,
                                             const UEHeaderGenConfig& config) const;
    
    std::string processDelegateHeaderTemplate(const std::string& templateContent,
                                             const std::string& delegateType,
                                             const UEHeaderGenConfig& config) const;
    
    std::string processInterfaceHeaderTemplate(const std::string& templateContent,
                                              const std::string& interfaceName,
                                              const UEHeaderGenConfig& config) const;
    
    std::string processModuleHeaderTemplate(const std::string& templateContent,
                                           const std::string& moduleName,
                                           const UEHeaderGenConfig& config) const;
    
    std::string processPCHHeaderTemplate(const std::string& templateContent,
                                        const std::vector<std::string>& modules,
                                        const UEHeaderGenConfig& config) const;
    
    bool validateHeaderConfig(const UEHeaderGenConfig& config) const;
    bool validateClassForHeader(const UEClassInfo& classInfo) const;
    bool validateStructForHeader(const std::string& structName, AST::TypePtr kiwiType) const;
    bool validateEnumForHeader(const std::string& enumName, AST::TypePtr kiwiType) const;
    
    std::string generateIncludeGuard(const std::string& fileName) const;
    std::string generateExportMacro(const std::string& symbolName,
                                   const UEHeaderGenConfig& config) const;
    
    std::string generateUClassMacro(const UEClassInfo& classInfo) const;
    std::string generateUStructMacro(const std::string& structName,
                                    AST::TypePtr kiwiType) const;
    std::string generateUEnumMacro(const std::string& enumName,
                                  AST::TypePtr kiwiType) const;
    std::string generateUFUNCTIONMacro(const UEFunctionInfo& functionInfo) const;
    std::string generateUPROPERTYMacro(const UEPropertyInfo& propertyInfo) const;
    std::string generateUDELEGATEMacro(const std::string& delegateType) const;
    
    std::string generateBlueprintTypeMacro(const UEClassInfo& classInfo) const;
    std::string generateBlueprintableMacro(const UEClassInfo& classInfo) const;
    
    std::string generateClassForwardDeclaration(const UEClassInfo& classInfo) const;
    std::string generateStructForwardDeclaration(const std::string& structName) const;
    std::string generateEnumForwardDeclaration(const std::string& enumName) const;
    
    std::string generateClassCommentDocumentation(const UEClassInfo& classInfo) const;
    std::string generateFunctionCommentDocumentation(const UEFunctionInfo& functionInfo) const;
    std::string generatePropertyCommentDocumentation(const UEPropertyInfo& propertyInfo) const;
    
    std::vector<std::string> collectClassHeaderDependencies(const UEClassInfo& classInfo) const;
    std::vector<std::string> collectStructHeaderDependencies(const std::string& structName,
                                                            AST::TypePtr kiwiType) const;
    std::vector<std::string> collectEnumHeaderDependencies(const std::string& enumName,
                                                          AST::TypePtr kiwiType) const;
    std::vector<std::string> collectFunctionHeaderDependencies(const UEFunctionInfo& functionInfo) const;
    std::vector<std::string> collectPropertyHeaderDependencies(const UEPropertyInfo& propertyInfo) const;
    
    std::string generateDefaultConstructorDeclaration(const UEClassInfo& classInfo) const;
    std::string generateCopyConstructorDeclaration(const UEClassInfo& classInfo) const;
    std::string generateDestructorDeclaration(const UEClassInfo& classInfo) const;
    std::string generateAssignmentOperatorDeclaration(const UEClassInfo& classInfo) const;
    
    std::string generateVirtualFunctionDeclarations(const UEClassInfo& classInfo) const;
    std::string generateStaticFunctionDeclarations(const UEClassInfo& classInfo) const;
    std::string generateBlueprintFunctionDeclarations(const UEClassInfo& classInfo) const;
    
    std::string generatePropertyDeclarations(const UEClassInfo& classInfo) const;
    std::string generatePropertyAccessorDeclarations(const UEClassInfo& classInfo) const;
    
    friend class UEHeaderGeneratorImpl;
};

} // namespace Unreal
} // namespace kiwiLang