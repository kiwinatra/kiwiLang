#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/AST/Decl.h"
#include "kiwiLang/CodeGen/IRBuilder.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;
class UETypeConverter;
struct UEClassInfo;
struct UEFunctionInfo;
struct UEPropertyInfo;

enum class UENativeStubType {
    ClassConstructor,
    ClassDestructor,
    VirtualFunction,
    StaticFunction,
    PropertyGetter,
    PropertySetter,
    Serialization,
    Replication,
    RPC,
    EventHandler,
    DelegateBinding,
    BlueprintImplementableEvent,
    BlueprintNativeEvent,
    CustomThunk
};

struct UENativeStubConfig {
    UENativeStubType stubType;
    std::string stubName;
    std::vector<std::string> parameters;
    std::string returnType;
    std::unordered_map<std::string, std::string> attributes;
    bool isConst;
    bool isVirtual;
    bool isPureVirtual;
    bool isInline;
    bool isExport;
    
    bool isValid() const;
    std::string generateSignature() const;
};

struct UENativeStubResult {
    bool success;
    std::string generatedStub;
    std::vector<std::string> dependencies;
    std::vector<std::string> errors;
    
    bool hasErrors() const { return !errors.empty(); }
};

class KIWI_UNREAL_API UENativeStubGenerator {
public:
    explicit UENativeStubGenerator(CodeGen::IRBuilder& irBuilder,
                                  UETypeRegistry* typeRegistry = nullptr,
                                  UETypeConverter* typeConverter = nullptr);
    ~UENativeStubGenerator();
    
    UENativeStubResult generateClassStub(const UEClassInfo& classInfo,
                                        UENativeStubType stubType,
                                        const UENativeStubConfig& config);
    
    UENativeStubResult generateFunctionStub(const UEFunctionInfo& functionInfo,
                                           UENativeStubType stubType,
                                           const UENativeStubConfig& config);
    
    UENativeStubResult generatePropertyStub(const UEPropertyInfo& propertyInfo,
                                           UENativeStubType stubType,
                                           const UENativeStubConfig& config);
    
    UENativeStubResult generateCustomStub(const UENativeStubConfig& config);
    
    UENativeStubResult generateThunkStub(AST::FunctionDecl* kiwiFunction,
                                        const UEFunctionInfo& ueFunctionInfo,
                                        const UENativeStubConfig& config);
    
    UENativeStubResult generateDelegateStub(const std::string& delegateType,
                                           const UENativeStubConfig& config);
    
    bool registerStubTemplate(const std::string& templateName,
                             const std::string& templateContent);
    
    bool unregisterStubTemplate(const std::string& templateName);
    
    std::string expandStubTemplate(const std::string& templateName,
                                  const std::unordered_map<std::string, std::string>& parameters) const;
    
    const CodeGen::IRBuilder& getIRBuilder() const { return irBuilder_; }
    
private:
    CodeGen::IRBuilder& irBuilder_;
    UETypeRegistry* typeRegistry_;
    UETypeConverter* typeConverter_;
    
    std::unordered_map<std::string, std::string> stubTemplates_;
    std::unordered_map<UENativeStubType, std::function<UENativeStubResult(const UENativeStubConfig&)>> stubGenerators_;
    
    void initializeDefaultTemplates();
    void initializeStubGenerators();
    
    UENativeStubResult generateClassConstructorStub(const UEClassInfo& classInfo,
                                                   const UENativeStubConfig& config);
    
    UENativeStubResult generateClassDestructorStub(const UEClassInfo& classInfo,
                                                  const UENativeStubConfig& config);
    
    UENativeStubResult generateVirtualFunctionStub(const UEFunctionInfo& functionInfo,
                                                  const UENativeStubConfig& config);
    
    UENativeStubResult generateStaticFunctionStub(const UEFunctionInfo& functionInfo,
                                                 const UENativeStubConfig& config);
    
    UENativeStubResult generatePropertyGetterStub(const UEPropertyInfo& propertyInfo,
                                                 const UENativeStubConfig& config);
    
    UENativeStubResult generatePropertySetterStub(const UEPropertyInfo& propertyInfo,
                                                 const UENativeStubConfig& config);
    
    UENativeStubResult generateSerializationStub(const UEClassInfo& classInfo,
                                                const UENativeStubConfig& config);
    
    UENativeStubResult generateReplicationStub(const UEClassInfo& classInfo,
                                              const UENativeStubConfig& config);
    
    UENativeStubResult generateRPCStub(const UEFunctionInfo& functionInfo,
                                      const UENativeStubConfig& config);
    
    UENativeStubResult generateEventHandlerStub(const UEClassInfo& classInfo,
                                               const UENativeStubConfig& config);
    
    UENativeStubResult generateDelegateBindingStub(const std::string& delegateType,
                                                  const UENativeStubConfig& config);
    
    UENativeStubResult generateBlueprintEventStub(const UEFunctionInfo& functionInfo,
                                                 UENativeStubType stubType,
                                                 const UENativeStubConfig& config);
    
    UENativeStubResult generateCustomThunkStub(AST::FunctionDecl* kiwiFunction,
                                              const UEFunctionInfo& ueFunctionInfo,
                                              const UENativeStubConfig& config);
    
    std::string processClassStubTemplate(const std::string& templateContent,
                                        const UEClassInfo& classInfo,
                                        const UENativeStubConfig& config) const;
    
    std::string processFunctionStubTemplate(const std::string& templateContent,
                                           const UEFunctionInfo& functionInfo,
                                           const UENativeStubConfig& config) const;
    
    std::string processPropertyStubTemplate(const std::string& templateContent,
                                           const UEPropertyInfo& propertyInfo,
                                           const UENativeStubConfig& config) const;
    
    std::string processCustomStubTemplate(const std::string& templateContent,
                                         const UENativeStubConfig& config) const;
    
    bool validateStubConfig(const UENativeStubConfig& config) const;
    bool validateClassForStub(const UEClassInfo& classInfo, UENativeStubType stubType) const;
    bool validateFunctionForStub(const UEFunctionInfo& functionInfo, UENativeStubType stubType) const;
    bool validatePropertyForStub(const UEPropertyInfo& propertyInfo, UENativeStubType stubType) const;
    
    std::string generateParameterList(const std::vector<std::string>& parameters) const;
    std::string generateAttributeList(const std::unordered_map<std::string, std::string>& attributes) const;
    
    std::string generateConstructorBody(const UEClassInfo& classInfo) const;
    std::string generateDestructorBody(const UEClassInfo& classInfo) const;
    std::string generateVirtualFunctionBody(const UEFunctionInfo& functionInfo) const;
    std::string generatePropertyAccessorBody(const UEPropertyInfo& propertyInfo,
                                            bool isGetter) const;
    
    std::string generateSerializationBody(const UEClassInfo& classInfo) const;
    std::string generateReplicationBody(const UEClassInfo& classInfo) const;
    std::string generateRPCBody(const UEFunctionInfo& functionInfo) const;
    
    std::string generateThunkSignature(AST::FunctionDecl* kiwiFunction,
                                      const UEFunctionInfo& ueFunctionInfo) const;
    
    std::string generateThunkParameterConversion(AST::FunctionDecl* kiwiFunction,
                                                const UEFunctionInfo& ueFunctionInfo) const;
    
    std::string generateThunkReturnConversion(AST::FunctionDecl* kiwiFunction,
                                             const UEFunctionInfo& ueFunctionInfo) const;
    
    std::string generateDelegateSignature(const std::string& delegateType) const;
    std::string generateDelegateBindingBody(const std::string& delegateType) const;
    
    std::string generateBlueprintEventSignature(const UEFunctionInfo& functionInfo,
                                               bool isImplementable) const;
    
    std::string generateBlueprintEventBody(const UEFunctionInfo& functionInfo,
                                          bool isImplementable) const;
    
    std::vector<std::string> collectClassDependencies(const UEClassInfo& classInfo) const;
    std::vector<std::string> collectFunctionDependencies(const UEFunctionInfo& functionInfo) const;
    std::vector<std::string> collectPropertyDependencies(const UEPropertyInfo& propertyInfo) const;
    
    friend class UENativeStubGeneratorImpl;
};

} // namespace Unreal
} // namespace kiwiLang