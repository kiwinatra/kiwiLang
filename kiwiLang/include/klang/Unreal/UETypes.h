#pragma once

#include "kiwiLang/AST/Type.h"
#include "kiwiLang/Sema/TypeSystem.h"
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace kiwiLang {
namespace Unreal {

class UETypeConverter;

enum class UEPropertyFlags : std::uint64_t {
    None                = 0,
    Editable            = 1 << 0,
    BlueprintReadOnly   = 1 << 1,
    BlueprintReadWrite  = 1 << 2,
    BlueprintAssignable = 1 << 3,
    BlueprintCallable   = 1 << 4,
    BlueprintAuthorityOnly = 1 << 5,
    Config              = 1 << 6,
    GlobalConfig        = 1 << 7,
    Export              = 1 << 8,
    Native              = 1 << 9,
    Transient           = 1 << 10,
    DuplicateTransient  = 1 << 11,
    TextExportTransient = 1 << 12,
    NonPIEDuplicateTransient = 1 << 13,
    EditFixedSize       = 1 << 14,
    EditConst           = 1 << 15,
    EditConstArray      = 1 << 16,
    Parm               = 1 << 17,
    OutParm            = 1 << 18,
    SkipParm           = 1 << 19,
    ReturnParm         = 1 << 20,
    ReferenceParm      = 1 << 21,
    BlueprintVisible   = 1 << 22,
    BlueprintProtected = 1 << 23,
    Net               = 1 << 24,
    NetSerialize      = 1 << 25,
    RepNotify         = 1 << 26,
    Interp            = 1 << 27,
    NonTransactional  = 1 << 28,
    EditorOnly        = 1 << 29,
    NoDestructor      = 1 << 30,
    AutoWeak          = 1ULL << 31,
    ContainsInstancedReference = 1ULL << 32,
    SimpleDisplay     = 1ULL << 33,
    AdvancedDisplay   = 1ULL << 34,
    SaveGame          = 1ULL << 35,
    Deprecated        = 1ULL << 36
};

constexpr UEPropertyFlags operator|(UEPropertyFlags a, UEPropertyFlags b) noexcept {
    return static_cast<UEPropertyFlags>(static_cast<std::uint64_t>(a) | static_cast<std::uint64_t>(b));
}

constexpr UEPropertyFlags operator&(UEPropertyFlags a, UEPropertyFlags b) noexcept {
    return static_cast<UEPropertyFlags>(static_cast<std::uint64_t>(a) & static_cast<std::uint64_t>(b));
}

constexpr UEPropertyFlags& operator|=(UEPropertyFlags& a, UEPropertyFlags b) noexcept {
    a = a | b;
    return a;
}

enum class UEFunctionFlags : std::uint32_t {
    None                = 0,
    Final               = 1 << 0,
    RequiredAPI         = 1 << 1,
    BlueprintAuthorityOnly = 1 << 2,
    BlueprintCosmetic   = 1 << 3,
    Net                 = 1 << 4,
    NetReliable         = 1 << 5,
    NetRequest          = 1 << 6,
    Exec                = 1 << 7,
    Native              = 1 << 8,
    Event               = 1 << 9,
    NetResponse         = 1 << 10,
    Static              = 1 << 11,
    NetMulticast        = 1 << 12,
    MulticastDelegate   = 1 << 13,
    Public              = 1 << 14,
    Private             = 1 << 15,
    Protected           = 1 << 16,
    Delegate            = 1 << 17,
    NetServer           = 1 << 18,
    HasOutParms         = 1 << 19,
    HasDefaults         = 1 << 20,
    NetClient           = 1 << 21,
    DLLImport           = 1 << 22,
    BlueprintCallable   = 1 << 23,
    BlueprintEvent      = 1 << 24,
    BlueprintPure       = 1 << 25,
    EditorOnly          = 1 << 26,
    Const               = 1 << 27,
    NetValidate         = 1 << 28,
    AllFlags            = 0xFFFFFFFF
};

enum class UEClassFlags : std::uint32_t {
    None                = 0,
    Abstract            = 1 << 0,
    DefaultConfig       = 1 << 1,
    Config              = 1 << 2,
    Transient           = 1 << 3,
    Parsed              = 1 << 4,
    AdvancedDisplay     = 1 << 5,
    Native              = 1 << 6,
    NoExport            = 1 << 7,
    NotPlaceable        = 1 << 8,
    PerObjectConfig     = 1 << 9,
    EditInlineNew       = 1 << 10,
    CollapseCategories  = 1 << 11,
    Interface           = 1 << 12,
    CustomConstructor   = 1 << 13,
    Const               = 1 << 14,
    LayoutChanging      = 1 << 15,
    CompiledFromBlueprint = 1 << 16,
    MinimalAPI          = 1 << 17,
    RequiredAPI         = 1 << 18,
    DefaultToInstanced  = 1 << 19,
    TokenStreamAssembled = 1 << 20,
    HasInstancedReference = 1 << 21,
    Hidden              = 1 << 22,
    Deprecated          = 1 << 23,
    HideDropDown        = 1 << 24,
    GlobalUserConfig    = 1 << 25,
    Intrinsic           = 1 << 26,
    Constructed         = 1 << 27,
    ConfigDoNotCheckDefaults = 1 << 28,
    NewerVersionExists  = 1 << 29,
    HideFunctions       = 1 << 30,
    Exported            = 1u << 31
};

struct UEPropertyInfo {
    std::string name;
    AST::TypePtr kiwiType;
    UEPropertyFlags flags;
    std::string category;
    std::string metaData;
    std::string defaultValue;
    std::uint32_t arraySize;
    std::uint32_t offset;
    
    bool isEditable() const { return (flags & UEPropertyFlags::Editable) != UEPropertyFlags::None; }
    bool isBlueprintVisible() const { return (flags & UEPropertyFlags::BlueprintVisible) != UEPropertyFlags::None; }
    bool isConfig() const { return (flags & UEPropertyFlags::Config) != UEPropertyFlags::None; }
    std::string getCPPTypeName(const UETypeConverter& converter) const;
    std::string getUHTMetaData() const;
};

struct UEFunctionParam {
    std::string name;
    AST::TypePtr type;
    UEPropertyFlags flags;
    std::string defaultValue;
    
    bool isOut() const { return (flags & UEPropertyFlags::OutParm) != UEPropertyFlags::None; }
    bool isReference() const { return (flags & UEPropertyFlags::ReferenceParm) != UEPropertyFlags::None; }
    bool isConst() const { return (flags & UEPropertyFlags::EditConst) != UEPropertyFlags::None; }
};

struct UEFunctionInfo {
    std::string name;
    AST::TypePtr returnType;
    std::vector<UEFunctionParam> parameters;
    UEFunctionFlags flags;
    std::string category;
    std::string metaData;
    std::uint32_t functionId;
    
    bool isBlueprintCallable() const { return (flags & UEFunctionFlags::BlueprintCallable) != UEFunctionFlags::None; }
    bool isBlueprintEvent() const { return (flags & UEFunctionFlags::BlueprintEvent) != UEFunctionFlags::None; }
    bool isPure() const { return (flags & UEFunctionFlags::BlueprintPure) != UEFunctionFlags::None; }
    bool isNet() const { return (flags & UEFunctionFlags::Net) != UEFunctionFlags::None; }
    std::string getSignature(const UETypeConverter& converter) const;
};

struct UEClassInfo {
    std::string name;
    std::string baseClassName;
    UEClassFlags flags;
    std::vector<UEPropertyInfo> properties;
    std::vector<UEFunctionInfo> functions;
    std::string configName;
    std::string metaData;
    std::uint32_t classId;
    bool isBlueprintType;
    bool isBlueprintable;
    
    bool isAbstract() const { return (flags & UEClassFlags::Abstract) != UEClassFlags::None; }
    bool isConfig() const { return (flags & UEClassFlags::Config) != UEClassFlags::None; }
    bool isNative() const { return (flags & UEClassFlags::Native) != UEClassFlags::None; }
    bool hasAnyBlueprintFunction() const;
    std::string getGeneratedClassName() const;
};

class KIWI_UNREAL_API UETypeRegistry {
public:
    explicit UETypeRegistry(Sema::TypeSystem& typeSystem);
    
    bool registerClass(const UEClassInfo& classInfo);
    bool registerStruct(const std::string& name, AST::TypePtr type);
    bool registerEnum(const std::string& name, AST::TypePtr type);
    
    std::shared_ptr<const UEClassInfo> findClass(const std::string& name) const;
    AST::TypePtr findNativeType(const std::string& ueTypeName) const;
    std::string findUETypeName(AST::TypePtr kiwiType) const;
    
    bool generateTypeDeclarations() const;
    bool validateTypeCompatibility() const;
    
private:
    Sema::TypeSystem& typeSystem_;
    std::unordered_map<std::string, std::shared_ptr<UEClassInfo>> registeredClasses_;
    std::unordered_map<std::string, AST::TypePtr> nativeTypes_;
    std::unordered_map<AST::TypePtr, std::string, AST::TypeHash> typeToUEName_;
    
    void registerBuiltinTypes();
    bool validateClassHierarchy(const UEClassInfo& classInfo) const;
    bool validatePropertyType(const UEPropertyInfo& prop) const;
};

} // namespace Unreal
} // namespace kiwiLang