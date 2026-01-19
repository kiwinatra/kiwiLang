#pragma once

#include "kiwiLang/AST/Type.h"
#include "kiwiLang/Sema/TypeSystem.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace kiwiLang {
namespace Unreal {

class UETypeRegistry;

enum class UETypeConversionDirection {
    KiwiToUE,
    UEToKiwi,
    Bidirectional
};

enum class UETypeConversionFlags : std::uint32_t {
    None = 0,
    AllowLossy = 1 << 0,
    GenerateStubs = 1 << 1,
    PreserveConst = 1 << 2,
    PreserveReference = 1 << 3,
    GenerateDefaultValue = 1 << 4,
    ValidateBlueprints = 1 << 5,
    Default = PreserveConst | GenerateDefaultValue
};

struct UETypeConversionResult {
    bool success;
    AST::TypePtr convertedType;
    std::string errorMessage;
    std::string generatedStub;
    std::uint32_t conversionFlagsUsed;
    
    bool isValid() const { return success && convertedType != nullptr; }
};

struct UETypeMapping {
    std::string ueTypeName;
    AST::TypePtr kiwiType;
    UETypeConversionDirection direction;
    std::function<Runtime::Value(void*)> toKiwiConverter;
    std::function<void*(const Runtime::Value&)> toUEConverter;
    bool isBuiltin;
    bool isBlueprintSupported;
    
    bool canConvertToKiwi() const {
        return direction == UETypeConversionDirection::KiwiToUE || 
               direction == UETypeConversionDirection::Bidirectional;
    }
    
    bool canConvertToUE() const {
        return direction == UETypeConversionDirection::UEToKiwi || 
               direction == UETypeConversionDirection::Bidirectional;
    }
};

class KIWI_UNREAL_API UETypeConverter {
public:
    explicit UETypeConverter(Sema::TypeSystem& typeSystem,
                            UETypeRegistry* registry = nullptr);
    ~UETypeConverter();
    
    UETypeConversionResult convertType(AST::TypePtr kiwiType,
                                      UETypeConversionDirection direction,
                                      UETypeConversionFlags flags = UETypeConversionFlags::Default);
    
    UETypeConversionResult convertType(const std::string& ueTypeName,
                                      UETypeConversionDirection direction,
                                      UETypeConversionFlags flags = UETypeConversionFlags::Default);
    
    bool registerTypeMapping(const UETypeMapping& mapping);
    bool unregisterTypeMapping(const std::string& ueTypeName);
    
    bool isTypeSupported(const std::string& ueTypeName) const;
    bool isTypeSupported(AST::TypePtr kiwiType) const;
    
    std::string getUETypeName(AST::TypePtr kiwiType) const;
    AST::TypePtr getKiwiType(const std::string& ueTypeName) const;
    
    Runtime::Value convertValueToKiwi(void* ueValue,
                                     const std::string& ueTypeName,
                                     UETypeConversionFlags flags = UETypeConversionFlags::Default);
    
    void* convertValueToUE(const Runtime::Value& kiwiValue,
                          const std::string& ueTypeName,
                          UETypeConversionFlags flags = UETypeConversionFlags::Default);
    
    std::string generateTypeStub(const std::string& ueTypeName,
                                UETypeConversionFlags flags = UETypeConversionFlags::Default) const;
    
    std::string generateConversionStub(AST::TypePtr fromType,
                                      AST::TypePtr toType,
                                      UETypeConversionFlags flags = UETypeConversionFlags::Default) const;
    
    bool validateTypeCompatibility(AST::TypePtr kiwiType,
                                  const std::string& ueTypeName,
                                  UETypeConversionFlags flags = UETypeConversionFlags::Default) const;
    
    const std::unordered_map<std::string, UETypeMapping>& getTypeMappings() const { return typeMappings_; }
    
private:
    Sema::TypeSystem& typeSystem_;
    UETypeRegistry* registry_;
    
    std::unordered_map<std::string, UETypeMapping> typeMappings_;
    std::unordered_map<AST::TypePtr, std::string, AST::TypeHash> reverseMappings_;
    
    void initializeBuiltinMappings();
    
    UETypeConversionResult convertBuiltinType(AST::TypePtr kiwiType,
                                             UETypeConversionDirection direction,
                                             UETypeConversionFlags flags);
    
    UETypeConversionResult convertBuiltinType(const std::string& ueTypeName,
                                             UETypeConversionDirection direction,
                                             UETypeConversionFlags flags);
    
    UETypeConversionResult convertClassType(AST::TypePtr kiwiType,
                                           UETypeConversionDirection direction,
                                           UETypeConversionFlags flags);
    
    UETypeConversionResult convertClassType(const std::string& ueTypeName,
                                           UETypeConversionDirection direction,
                                           UETypeConversionFlags flags);
    
    UETypeConversionResult convertStructType(AST::TypePtr kiwiType,
                                            UETypeConversionDirection direction,
                                            UETypeConversionFlags flags);
    
    UETypeConversionResult convertStructType(const std::string& ueTypeName,
                                            UETypeConversionDirection direction,
                                            UETypeConversionFlags flags);
    
    UETypeConversionResult convertEnumType(AST::TypePtr kiwiType,
                                          UETypeConversionDirection direction,
                                          UETypeConversionFlags flags);
    
    UETypeConversionResult convertEnumType(const std::string& ueTypeName,
                                          UETypeConversionDirection direction,
                                          UETypeConversionFlags flags);
    
    UETypeConversionResult convertArrayType(AST::TypePtr kiwiType,
                                           UETypeConversionDirection direction,
                                           UETypeConversionFlags flags);
    
    UETypeConversionResult convertMapType(AST::TypePtr kiwiType,
                                         UETypeConversionDirection direction,
                                         UETypeConversionFlags flags);
    
    UETypeConversionResult convertDelegateType(AST::TypePtr kiwiType,
                                              UETypeConversionDirection direction,
                                              UETypeConversionFlags flags);
    
    UETypeConversionResult convertTemplateType(AST::TypePtr kiwiType,
                                              UETypeConversionDirection direction,
                                              UETypeConversionFlags flags);
    
    std::string generateClassStub(const std::string& ueTypeName) const;
    std::string generateStructStub(const std::string& ueTypeName) const;
    std::string generateEnumStub(const std::string& ueTypeName) const;
    std::string generateArrayStub(const std::string& ueTypeName) const;
    std::string generateMapStub(const std::string& ueTypeName) const;
    
    bool validateClassCompatibility(const UEClassInfo* classInfo,
                                   AST::TypePtr kiwiType,
                                   UETypeConversionFlags flags) const;
    
    bool validateStructCompatibility(const std::string& ueTypeName,
                                    AST::TypePtr kiwiType,
                                    UETypeConversionFlags flags) const;
    
    bool validateEnumCompatibility(const std::string& ueTypeName,
                                  AST::TypePtr kiwiType,
                                  UETypeConversionFlags flags) const;
    
    Runtime::Value convertUObjectToKiwi(void* ueValue, const UETypeMapping& mapping);
    void* convertKiwiToUObject(const Runtime::Value& kiwiValue, const UETypeMapping& mapping);
    
    Runtime::Value convertTArrayToKiwi(void* ueValue, const UETypeMapping& mapping);
    void* convertKiwiToTArray(const Runtime::Value& kiwiValue, const UETypeMapping& mapping);
    
    Runtime::Value convertTMapToKiwi(void* ueValue, const UETypeMapping& mapping);
    void* convertKiwiToTMap(const Runtime::Value& kiwiValue, const UETypeMapping& mapping);
};

} // namespace Unreal
} // namespace kiwiLang