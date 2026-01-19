#include "kiwiLang/Unreal/Bindings/UPropertyBinding.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeConverter.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include "kiwiLang/AST/Type.h"
#include <sstream>

namespace kiwiLang {
namespace Unreal {

UPropertyBindingBase::UPropertyBindingBase(const UEPropertyBindingParams& params)
    : params_(params), isBound_(false), isValid_(false) {
}

UPropertyBindingBase::~UPropertyBindingBase() = default;

bool UEPropertyBindingParams::validate() const {
    if (propertyName.empty()) {
        DiagnosticEngine::get().error("Property name cannot be empty");
        return false;
    }
    
    if (!kiwiType) {
        DiagnosticEngine::get().error("Property must have kiwi type");
        return false;
    }
    
    if (size == 0) {
        DiagnosticEngine::get().warning("Property size is zero: " + propertyName);
    }
    
    return true;
}

std::string UEPropertyBindingParams::generateCPPType(const UETypeConverter& converter) const {
    std::ostringstream oss;
    
    // Handle arrays
    if (arrayDim > 1) {
        oss << "TArray<";
    }
    
    // Convert kiwi type to CPP type
    std::string cppType = converter.getUETypeName(kiwiType);
    if (cppType.empty()) {
        // Try to infer from kiwi type
        if (kiwiType->isIntegerType()) {
            cppType = "int32";
        } else if (kiwiType->isFloatingType()) {
            cppType = "float";
        } else if (kiwiType->isBooleanType()) {
            cppType = "bool";
        } else if (kiwiType->isStringType()) {
            cppType = "FString";
        } else {
            cppType = "void*";
        }
    }
    
    oss << cppType;
    
    if (arrayDim > 1) {
        oss << ">";
    }
    
    // Add reference/pointer qualifiers
    if (isTransient) {
        oss << "*";
    }
    
    return oss.str();
}

std::string UEPropertyBindingParams::generateUHTMetaData() const {
    std::ostringstream oss;
    bool first = true;
    
    auto addMeta = [&](const std::string& meta) {
        if (!first) oss << ", ";
        oss << meta;
        first = false;
    };
    
    // Category
    if (!category.empty()) {
        addMeta("Category=\"" + category + "\"");
    }
    
    // Edit rules
    if (isEditable) {
        addMeta("EditAnywhere");
    } else if (isBlueprintVisible) {
        addMeta("BlueprintReadOnly");
    }
    
    // Blueprint visibility
    if (isBlueprintVisible) {
        if (isEditable) {
            addMeta("BlueprintReadWrite");
        } else {
            addMeta("BlueprintReadOnly");
        }
    }
    
    // Config
    if (isConfig) {
        addMeta("Config");
    }
    
    // Transient
    if (isTransient) {
        addMeta("Transient");
    }
    
    // Custom metadata
    if (!metaData.empty()) {
        addMeta("Meta=(" + metaData + ")");
    }
    
    return oss.str();
}

template<typename PropertyType>
bool UPropertyBinding<PropertyType>::bindToUProperty(UProperty* uProperty) {
    if (!uProperty) {
        DiagnosticEngine::get().error("Cannot bind to null UProperty");
        return false;
    }
    
    // Validate property type compatibility
    std::string ueTypeName = params_.uePropertyInfo.getCPPTypeName(*static_cast<const UETypeConverter*>(nullptr));
    FString uPropertyType = uProperty->GetClass()->GetName();
    
    DiagnosticEngine::get().info("Binding property " + params_.propertyName + 
                                " to UProperty of type " + std::string(TCHAR_TO_UTF8(*uPropertyType)));
    
    // Check if types are compatible
    if (!validatePropertyCompatibility(params_, params_.kiwiType, *static_cast<const UETypeConverter*>(nullptr))) {
        DiagnosticEngine::get().error("Property type mismatch for: " + params_.propertyName);
        return false;
    }
    
    // Set up reflection data
    uint8* propertyData = static_cast<uint8*>(uProperty->ContainerPtrToValuePtr<void>(nullptr));
    if (!propertyData) {
        DiagnosticEngine::get().error("Failed to get property data for: " + params_.propertyName);
        return false;
    }
    
    // Register property offset for direct memory access
    params_.offset = uProperty->GetOffset_ForInternal();
    params_.size = uProperty->GetSize();
    
    DiagnosticEngine::get().info("Successfully bound property: " + params_.propertyName + 
                                " (offset: " + std::to_string(params_.offset) + 
                                ", size: " + std::to_string(params_.size) + ")");
    
    isBound_ = true;
    return true;
}

template<typename PropertyType>
bool UPropertyBinding<PropertyType>::bindToBlueprintVariable(void* blueprintProperty) {
    DiagnosticEngine::get().info("Binding property " + params_.propertyName + " to blueprint variable");
    
    // In UE5, blueprint variables are represented by FProperty in the UClass
    // This would involve:
    // 1. Getting the UBlueprintGeneratedClass
    // 2. Finding the FProperty
    // 3. Setting up value getter/setter callbacks
    // 4. Registering with blueprint reflection
    
    isBound_ = true;
    return true;
}

template<typename PropertyType>
std::string UPropertyBinding<PropertyType>::generateCPPAccessors() const {
    std::ostringstream oss;
    
    oss << "// Generated accessors for property: " << params_.propertyName << "\n";
    oss << "// DO NOT EDIT - Generated by kiwiLang\n\n";
    
    // Getter
    oss << "FORCEINLINE " << params_.generateCPPType(*static_cast<const UETypeConverter*>(nullptr)) 
        << " Get" << params_.propertyName << "() const\n";
    oss << "{\n";
    
    if (rawPtr_) {
        oss << "    if (!NativeObject) return " << defaultValue_ << ";\n";
        oss << "    return *rawPtr_(NativeObject);\n";
    } else if (getter_) {
        oss << "    if (!NativeObject) return " << defaultValue_ << ";\n";
        oss << "    return getter_(NativeObject);\n";
    } else {
        oss << "    return " << defaultValue_ << ";\n";
    }
    
    oss << "}\n\n";
    
    // Setter (if not read-only)
    if (setter_) {
        oss << "FORCEINLINE void Set" << params_.propertyName 
            << "(" << params_.generateCPPType(*static_cast<const UETypeConverter*>(nullptr)) << " Value)\n";
        oss << "{\n";
        oss << "    if (!NativeObject) return;\n";
        oss << "    setter_(NativeObject, Value);\n";
        oss << "    MarkPropertyDirty();\n";
        oss << "}\n\n";
    }
    
    // Direct pointer access (for native code)
    if (rawPtr_) {
        oss << "FORCEINLINE " << params_.generateCPPType(*static_cast<const UETypeConverter*>(nullptr)) 
            << "* Get" << params_.propertyName << "Ptr()\n";
        oss << "{\n";
        oss << "    if (!NativeObject) return nullptr;\n";
        oss << "    return rawPtr_(NativeObject);\n";
        oss << "}\n";
    }
    
    return oss.str();
}

template<typename PropertyType>
std::string UPropertyBinding<PropertyType>::generateSerializationCode() const {
    std::ostringstream oss;
    
    oss << "// Serialization for property: " << params_.propertyName << "\n";
    
    oss << "void Serialize" << params_.propertyName << "(FArchive& Ar)\n";
    oss << "{\n";
    
    // Use UE's serialization system
    if (params_.isTransient) {
        oss << "    // Transient property, skip serialization\n";
        oss << "    return;\n";
    } else {
        oss << "    if (Ar.IsLoading())\n";
        oss << "    {\n";
        oss << "        Ar << PropertyValue;\n";
        oss << "    }\n";
        oss << "    else if (Ar.IsSaving())\n";
        oss << "    {\n";
        oss << "        Ar << PropertyValue;\n";
        oss << "    }\n";
    }
    
    oss << "}\n";
    
    return oss.str();
}

template<typename PropertyType>
std::string UPropertyBinding<PropertyType>::generateReplicationFunctions() const {
    std::ostringstream oss;
    
    if (!params_.uePropertyInfo.isBlueprintVisible()) {
        return ""; // Only replicate blueprint-visible properties
    }
    
    oss << "// Replication for property: " << params_.propertyName << "\n";
    
    oss << "void Replicate" << params_.propertyName << "()\n";
    oss << "{\n";
    oss << "    if (!HasAuthority() || !GetNetMode() != NM_Standalone)\n";
    oss << "    {\n";
    oss << "        return;\n";
    oss << "    }\n\n";
    
    oss << "    if (PropertyValue != LastReplicatedValue)\n";
    oss << "    {\n";
    oss << "        Multicast_Update" << params_.propertyName << "(PropertyValue);\n";
    oss << "        LastReplicatedValue = PropertyValue;\n";
    oss << "    }\n";
    oss << "}\n\n";
    
    // RPC stub
    oss << "UFUNCTION(NetMulticast, Reliable)\n";
    oss << "void Multicast_Update" << params_.propertyName 
        << "(" << params_.generateCPPType(*static_cast<const UETypeConverter*>(nullptr)) << " NewValue);\n";
    
    return oss.str();
}

std::unique_ptr<UPropertyBindingBase> UPropertyBindingFactory::createBinding(
    const UEPropertyBindingParams& params,
    const UEBindingContext& context) {
    
    if (!params.validate()) {
        DiagnosticEngine::get().error("Invalid property binding parameters");
        return nullptr;
    }
    
    DiagnosticEngine::get().info("Creating property binding for: " + params.propertyName);
    
    // Determine the actual C++ type from kiwi type
    AST::TypePtr kiwiType = params.kiwiType;
    if (!kiwiType) {
        DiagnosticEngine::get().error("Property has null kiwi type: " + params.propertyName);
        return nullptr;
    }
    
    // Create appropriate typed binding based on kiwi type
    std::unique_ptr<UPropertyBindingBase> binding;
    
    if (kiwiType->isIntegerType()) {
        if (kiwiType->getBitWidth() == 64) {
            binding = std::make_unique<UPropertyBinding<int64_t>>(params);
        } else {
            binding = std::make_unique<UPropertyBinding<int32_t>>(params);
        }
    } else if (kiwiType->isFloatingType()) {
        if (kiwiType->getBitWidth() == 64) {
            binding = std::make_unique<UPropertyBinding<double>>(params);
        } else {
            binding = std::make_unique<UPropertyBinding<float>>(params);
        }
    } else if (kiwiType->isBooleanType()) {
        binding = std::make_unique<UPropertyBinding<bool>>(params);
    } else if (kiwiType->isStringType()) {
        binding = std::make_unique<UPropertyBinding<FString>>(params);
    } else if (kiwiType->isClassType()) {
        binding = std::make_unique<UPropertyBinding<UObject*>>(params);
    } else {
        // Default to void* for complex types
        binding = std::make_unique<UPropertyBinding<void*>>(params);
    }
    
    if (binding) {
        configureTypedBinding(binding.get(), context);
    }
    
    return binding;
}

template<typename PropertyType>
std::unique_ptr<UPropertyBinding<PropertyType>>
UPropertyBindingFactory::createTypedBinding(const UEPropertyBindingParams& params,
                                           typename UPropertyBinding<PropertyType>::Getter getter,
                                           typename UPropertyBinding<PropertyType>::Setter setter,
                                           const UEBindingContext& context) {
    auto binding = std::make_unique<UPropertyBinding<PropertyType>>(
        params, std::move(getter), std::move(setter));
    configureTypedBinding(binding.get(), context);
    return binding;
}

bool UPropertyBindingFactory::validatePropertyCompatibility(
    const UEPropertyBindingParams& params,
    AST::TypePtr kiwiType,
    const UETypeConverter& converter) {
    
    if (!kiwiType) {
        DiagnosticEngine::get().error("Null kiwi type in property validation");
        return false;
    }
    
    // Get UE type name from kiwi type
    std::string ueTypeName = converter.getUETypeName(kiwiType);
    if (ueTypeName.empty()) {
        DiagnosticEngine::get().warning("No UE type mapping for kiwi type");
        return false;
    }
    
    // Check if the UE type can be represented in blueprints
    if (params.isBlueprintVisible) {
        // Validate blueprint compatibility
        if (!converter.validateTypeCompatibility(
                kiwiType,
                ueTypeName,
                UETypeConversionFlags::ValidateBlueprints)) {
            DiagnosticEngine::get().error("Type not blueprint compatible: " + ueTypeName);
            return false;
        }
    }
    
    // Check array dimensions
    if (params.arrayDim > 1) {
        if (!kiwiType->isValidForArray()) {
            DiagnosticEngine::get().error("Type cannot be used in arrays: " + ueTypeName);
            return false;
        }
    }
    
    // Check size constraints
    std::uint32_t calculatedSize = calculatePropertySize(params, converter);
    if (calculatedSize == 0) {
        DiagnosticEngine::get().warning("Could not calculate property size");
    } else if (calculatedSize > 65535) {
        DiagnosticEngine::get().error("Property size too large for UE reflection: " + 
                                     std::to_string(calculatedSize));
        return false;
    }
    
    return true;
}

std::uint32_t UPropertyBindingFactory::calculatePropertySize(
    const UEPropertyBindingParams& params,
    const UETypeConverter& converter) {
    
    if (!params.kiwiType) {
        return 0;
    }
    
    std::uint32_t elementSize = 0;
    std::string ueTypeName = converter.getUETypeName(params.kiwiType);
    
    // Map common UE types to sizes
    if (ueTypeName == "bool") elementSize = 1;
    else if (ueTypeName == "int8" || ueTypeName == "uint8") elementSize = 1;
    else if (ueTypeName == "int16" || ueTypeName == "uint16") elementSize = 2;
    else if (ueTypeName == "int32" || ueTypeName == "uint32" || ueTypeName == "float") elementSize = 4;
    else if (ueTypeName == "int64" || ueTypeName == "uint64" || ueTypeName == "double") elementSize = 8;
    else if (ueTypeName == "FString") elementSize = sizeof(FString);
    else if (ueTypeName == "FName") elementSize = sizeof(FName);
    else if (ueTypeName == "FText") elementSize = sizeof(FText);
    else if (ueTypeName.find("TArray") == 0) elementSize = sizeof(TArray<void*>);
    else if (ueTypeName.find("TMap") == 0) elementSize = sizeof(TMap<void*, void*>);
    else if (ueTypeName.find("TSet") == 0) elementSize = sizeof(TSet<void*>);
    else elementSize = sizeof(void*); // Pointer size for objects
    
    return elementSize * params.arrayDim;
}

void UPropertyBindingFactory::configureTypedBinding(UPropertyBindingBase* binding,
                                                   const UEBindingContext& context) {
    if (!binding || !context.registry) {
        return;
    }
    
    DiagnosticEngine::get().info("Configuring typed property binding for: " + binding->getParams().propertyName);
    
    // Validate the binding
    if (!binding->isValid()) {
        DiagnosticEngine::get().error("Property binding is invalid");
        return;
    }
    
    // Calculate actual size if not specified
    if (binding->getParams().size == 0) {
        const auto& params = binding->getParams();
        std::uint32_t calculatedSize = calculatePropertySize(params, *context.typeRegistry->getTypeConverter());
        if (calculatedSize > 0) {
            // Can't modify const params, would need to store in mutable cache
        }
    }
    
    // Register with object registry if needed
    if (context.registry) {
        // context.registry->registerPropertyBinding(std::unique_ptr<UPropertyBindingBase>(binding));
        // Note: This would need to handle ownership properly
    }
}

AST::TypePtr UPropertyBindingFactory::determineKiwiType(const UEPropertyBindingParams& params,
                                                       const UETypeConverter& converter) {
    if (params.kiwiType) {
        return params.kiwiType;
    }
    
    // Try to determine from UE type name
    std::string ueTypeName = params.uePropertyInfo.getCPPTypeName(converter);
    return converter.getKiwiType(ueTypeName);
}

UEPropertyInfo UPropertyBindingFactory::createPropertyInfoFromKiwi(
    AST::TypePtr kiwiType,
    const std::string& name,
    const UETypeConverter& converter) {
    
    UEPropertyInfo info;
    info.name = name;
    info.kiwiType = kiwiType;
    
    // Set default flags
    info.flags = UEPropertyFlags::Editable | UEPropertyFlags::BlueprintVisible;
    info.category = "Default";
    info.arraySize = 0;
    info.offset = 0;
    
    // Determine appropriate UE type name
    std::string ueTypeName = converter.getUETypeName(kiwiType);
    if (ueTypeName.empty()) {
        DiagnosticEngine::get().warning("No UE type mapping for kiwi type");
    }
    
    return info;
}

// Explicit template instantiations for common property types
template class UPropertyBinding<bool>;
template class UPropertyBinding<int8_t>;
template class UPropertyBinding<int16_t>;
template class UPropertyBinding<int32_t>;
template class UPropertyBinding<int64_t>;
template class UPropertyBinding<uint8_t>;
template class UPropertyBinding<uint16_t>;
template class UPropertyBinding<uint32_t>;
template class UPropertyBinding<uint64_t>;
template class UPropertyBinding<float>;
template class UPropertyBinding<double>;
template class UPropertyBinding<FString>;
template class UPropertyBinding<FName>;
template class UPropertyBinding<FText>;
template class UPropertyBinding<UObject*>;
template class UPropertyBinding<void*>;
template class UPropertyBinding<TArray<int32_t>>;
template class UPropertyBinding<TArray<FString>>;
template class UPropertyBinding<TMap<FString, int32_t>>;
template class UPropertyBinding<TSet<int32_t>>;

} // namespace Unreal
} // namespace kiwiLang