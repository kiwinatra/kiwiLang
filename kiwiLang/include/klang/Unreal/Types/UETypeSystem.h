#pragma once

#include "kiwiLang/AST/Type.h"
#include "kiwiLang/Sema/TypeSystem.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kiwiLang {
namespace Unreal {

class UETypeConverter;
class UObjectRegistry;

enum class UETypeCategory {
    Builtin,
    Class,
    Struct,
    Enum,
    Delegate,
    Interface,
    Template,
    Array,
    Map,
    Set,
    Optional,
    Unknown
};

enum class UETypeFlags : std::uint32_t {
    None = 0,
    BlueprintType = 1 << 0,
    Blueprintable = 1 << 1,
    Deprecated = 1 << 2,
    Abstract = 1 << 3,
    Transient = 1 << 4,
    Config = 1 << 5,
    EditInlineNew = 1 << 6,
    DefaultToInstanced = 1 << 7,
    AdvancedDisplay = 1 << 8,
    SaveGame = 1 << 9,
    Replicated = 1 << 10,
    Native = 1 << 11,
    Exported = 1 << 12
};

struct UETypeDescriptor {
    std::string name;
    UETypeCategory category;
    UETypeFlags flags;
    AST::TypePtr kiwiType;
    std::vector<std::string> typeParameters;
    std::unordered_map<std::string, std::string> metaData;
    std::uint32_t size;
    std::uint32_t alignment;
    std::uint64_t typeId;
    
    bool isValid() const { return kiwiType != nullptr; }
    bool isBlueprintType() const { return (flags & UETypeFlags::BlueprintType) != UETypeFlags::None; }
    bool isBlueprintable() const { return (flags & UETypeFlags::Blueprintable) != UETypeFlags::None; }
    bool isNative() const { return (flags & UETypeFlags::Native) != UETypeFlags::None; }
    
    std::string getMetaData(const std::string& key, const std::string& defaultValue = "") const;
    bool hasMetaData(const std::string& key) const;
};

struct UETypeRelationship {
    std::string sourceType;
    std::string targetType;
    enum class RelationshipType {
        Inheritance,
        Composition,
        Aggregation,
        Association,
        Dependency,
        TemplateInstance
    } relationshipType;
    std::uint32_t strength; // 0-100
    bool isBidirectional;
    
    bool isValid() const { return strength > 0; }
};

class KIWI_UNREAL_API UETypeSystem {
public:
    explicit UETypeSystem(Sema::TypeSystem& kiwiTypeSystem,
                         UETypeConverter* converter = nullptr,
                         UObjectRegistry* registry = nullptr);
    ~UETypeSystem();
    
    bool registerType(const UETypeDescriptor& descriptor);
    bool unregisterType(const std::string& typeName);
    
    bool registerRelationship(const UETypeRelationship& relationship);
    bool unregisterRelationship(const std::string& sourceType,
                               const std::string& targetType);
    
    const UETypeDescriptor* findType(const std::string& typeName) const;
    const UETypeDescriptor* findType(AST::TypePtr kiwiType) const;
    
    std::vector<UETypeDescriptor> findTypesByCategory(UETypeCategory category) const;
    std::vector<UETypeDescriptor> findTypesByFlag(UETypeFlags flag) const;
    
    std::vector<UETypeRelationship> getTypeRelationships(const std::string& typeName) const;
    std::vector<std::string> getTypeDependencies(const std::string& typeName) const;
    
    bool validateTypeHierarchy() const;
    bool checkCircularDependencies() const;
    
    bool generateTypeInfoHeader(const std::string& outputPath) const;
    bool generateTypeRegistryCode(const std::string& outputPath) const;
    bool generateReflectionData() const;
    
    bool loadTypeDefinitions(const std::string& configPath);
    bool saveTypeDefinitions(const std::string& configPath) const;
    
    bool registerBuiltinTypes();
    bool registerEngineTypes();
    bool registerPluginTypes(const std::string& pluginName);
    
    AST::TypePtr createKiwiType(const UETypeDescriptor& descriptor) const;
    UETypeDescriptor createUETypeDescriptor(AST::TypePtr kiwiType) const;
    
    bool isTypeCompatible(const std::string& sourceType,
                         const std::string& targetType) const;
    
    bool canCastTo(const std::string& fromType,
                  const std::string& toType) const;
    
    std::uint32_t getTypeSize(const std::string& typeName) const;
    std::uint32_t getTypeAlignment(const std::string& typeName) const;
    
    std::size_t getRegisteredTypeCount() const;
    bool isEmpty() const;
    
    const Sema::TypeSystem& getKiwiTypeSystem() const { return kiwiTypeSystem_; }
    
private:
    Sema::TypeSystem& kiwiTypeSystem_;
    UETypeConverter* converter_;
    UObjectRegistry* registry_;
    
    std::unordered_map<std::string, UETypeDescriptor> types_;
    std::unordered_map<AST::TypePtr, std::string, AST::TypeHash> typeToNameMap_;
    std::unordered_multimap<std::string, UETypeRelationship> relationships_;
    std::unordered_map<std::string, std::vector<UETypeRelationship>> typeRelationships_;
    
    mutable std::recursive_mutex mutex_;
    
    std::uint64_t generateTypeId(const std::string& typeName) const;
    
    bool validateTypeDescriptor(const UETypeDescriptor& descriptor) const;
    bool validateTypeRelationship(const UETypeRelationship& relationship) const;
    
    void buildTypeRelationshipIndex();
    void clearTypeRelationshipIndex();
    
    bool checkInheritanceChain(const std::string& derivedType,
                              const std::string& baseType,
                              std::unordered_set<std::string>& visited) const;
    
    bool detectCircularDependency(const std::string& typeName,
                                 std::unordered_set<std::string>& visited,
                                 std::unordered_set<std::string>& recursionStack) const;
    
    std::string generateTypeInfoStruct(const UETypeDescriptor& descriptor) const;
    std::string generateTypeRegistrationCode(const UETypeDescriptor& descriptor) const;
    std::string generateReflectionMacro(const UETypeDescriptor& descriptor) const;
    
    bool registerBuiltinNumericTypes();
    bool registerBuiltinStringTypes();
    bool registerBuiltinContainerTypes();
    bool registerBuiltinDelegateTypes();
    
    bool registerEngineCoreTypes();
    bool registerEngineMathTypes();
    bool registerEngineContainers();
    
    AST::TypePtr createStructType(const UETypeDescriptor& descriptor) const;
    AST::TypePtr createClassType(const UETypeDescriptor& descriptor) const;
    AST::TypePtr createEnumType(const UETypeDescriptor& descriptor) const;
    AST::TypePtr createTemplateType(const UETypeDescriptor& descriptor) const;
    AST::TypePtr createArrayType(const UETypeDescriptor& descriptor) const;
    AST::TypePtr createMapType(const UETypeDescriptor& descriptor) const;
    
    UETypeDescriptor createDescriptorFromStruct(AST::TypePtr type) const;
    UETypeDescriptor createDescriptorFromClass(AST::TypePtr type) const;
    UETypeDescriptor createDescriptorFromEnum(AST::TypePtr type) const;
    UETypeDescriptor createDescriptorFromTemplate(AST::TypePtr type) const;
};

} // namespace Unreal
} // namespace kiwiLang