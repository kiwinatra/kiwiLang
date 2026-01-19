#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include <algorithm>
#include <sstream>

namespace kiwiLang {
namespace Unreal {

std::string UEPropertyInfo::getCPPTypeName(const UETypeConverter& converter) const {
    std::ostringstream oss;
    
    // Handle arrays
    if (arraySize > 0) {
        oss << "TArray<";
    }
    
    // Convert kiwi type to CPP type
    if (kiwiType) {
        std::string cppType = converter.getUETypeName(kiwiType);
        if (cppType.empty()) {
            cppType = "void*"; // Fallback
        }
        oss << cppType;
    } else {
        oss << "void*";
    }
    
    if (arraySize > 0) {
        oss << ">";
    }
    
    return oss.str();
}

std::string UEPropertyInfo::getUHTMetaData() const {
    std::ostringstream oss;
    
    if (!category.empty()) {
        oss << "Category=\"" << category << "\"";
    }
    
    if (!metaData.empty()) {
        if (!oss.str().empty()) oss << ", ";
        oss << "Meta=" << metaData;
    }
    
    if (isEditable()) {
        if (!oss.str().empty()) oss << ", ";
        oss << "EditAnywhere";
    }
    
    if (isBlueprintVisible()) {
        if (!oss.str().empty()) oss << ", ";
        oss << "BlueprintReadWrite";
    } else if ((flags & UEPropertyFlags::BlueprintReadOnly) != UEPropertyFlags::None) {
        if (!oss.str().empty()) oss << ", ";
        oss << "BlueprintReadOnly";
    }
    
    if (isConfig()) {
        if (!oss.str().empty()) oss << ", ";
        oss << "Config";
    }
    
    if ((flags & UEPropertyFlags::Transient) != UEPropertyFlags::None) {
        if (!oss.str().empty()) oss << ", ";
        oss << "Transient";
    }
    
    return oss.str();
}

std::string UEFunctionInfo::getSignature(const UETypeConverter& converter) const {
    std::ostringstream oss;
    
    // Return type
    if (returnType) {
        std::string returnTypeName = converter.getUETypeName(returnType);
        if (!returnTypeName.empty()) {
            oss << returnTypeName << " ";
        } else {
            oss << "void ";
        }
    } else {
        oss << "void ";
    }
    
    // Function name
    oss << name << "(";
    
    // Parameters
    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        
        std::string paramType = converter.getUETypeName(param.type);
        if (paramType.empty()) {
            paramType = "void*";
        }
        
        if (param.isConst()) {
            oss << "const ";
        }
        
        oss << paramType;
        
        if (param.isReference()) {
            oss << "&";
        } else if (param.isOut()) {
            oss << "*";
        }
        
        oss << " " << param.name;
        
        if (!param.defaultValue.empty()) {
            oss << " = " << param.defaultValue;
        }
        
        if (i != parameters.size() - 1) {
            oss << ", ";
        }
    }
    
    oss << ")";
    
    if (isPure()) {
        oss << " const";
    }
    
    return oss.str();
}

bool UEClassInfo::hasAnyBlueprintFunction() const {
    return std::any_of(functions.begin(), functions.end(),
        [](const UEFunctionInfo& func) {
            return func.isBlueprintCallable() || func.isBlueprintEvent();
        });
}

std::string UEClassInfo::getGeneratedClassName() const {
    std::string generatedName = name;
    
    // Remove any namespace prefixes
    size_t lastColon = generatedName.find_last_of("::");
    if (lastColon != std::string::npos) {
        generatedName = generatedName.substr(lastColon + 1);
    }
    
    // Add prefix if this is a generated class
    if (!isBlueprintType && !isNative()) {
        generatedName = "K" + generatedName;
    }
    
    return generatedName;
}

UETypeRegistry::UETypeRegistry(Sema::TypeSystem& typeSystem)
    : typeSystem_(typeSystem) {
    registerBuiltinTypes();
}

bool UETypeRegistry::registerClass(const UEClassInfo& classInfo) {
    if (classInfo.name.empty()) {
        DiagnosticEngine::get().error("Class name cannot be empty");
        return false;
    }
    
    if (registeredClasses_.find(classInfo.name) != registeredClasses_.end()) {
        DiagnosticEngine::get().warning("Class already registered: " + classInfo.name);
        return false;
    }
    
    if (!validateClassHierarchy(classInfo)) {
        DiagnosticEngine::get().error("Invalid class hierarchy for: " + classInfo.name);
        return false;
    }
    
    for (const auto& prop : classInfo.properties) {
        if (!validatePropertyType(prop)) {
            DiagnosticEngine::get().error("Invalid property type in class: " + classInfo.name);
            return false;
        }
    }
    
    auto classPtr = std::make_shared<UEClassInfo>(classInfo);
    registeredClasses_[classInfo.name] = classPtr;
    
    // Register the class type in kiwiLang type system
    AST::TypePtr kiwiType = typeSystem_.createClassType(classInfo.name);
    nativeTypes_[classInfo.name] = kiwiType;
    typeToUEName_[kiwiType] = classInfo.name;
    
    DiagnosticEngine::get().info("Registered class: " + classInfo.name);
    return true;
}

bool UETypeRegistry::registerStruct(const std::string& name, AST::TypePtr type) {
    if (name.empty() || !type) {
        DiagnosticEngine::get().error("Invalid struct registration");
        return false;
    }
    
    if (nativeTypes_.find(name) != nativeTypes_.end()) {
        DiagnosticEngine::get().warning("Struct already registered: " + name);
        return false;
    }
    
    nativeTypes_[name] = type;
    typeToUEName_[type] = name;
    
    DiagnosticEngine::get().info("Registered struct: " + name);
    return true;
}

bool UETypeRegistry::registerEnum(const std::string& name, AST::TypePtr type) {
    if (name.empty() || !type) {
        DiagnosticEngine::get().error("Invalid enum registration");
        return false;
    }
    
    if (nativeTypes_.find(name) != nativeTypes_.end()) {
        DiagnosticEngine::get().warning("Enum already registered: " + name);
        return false;
    }
    
    nativeTypes_[name] = type;
    typeToUEName_[type] = name;
    
    DiagnosticEngine::get().info("Registered enum: " + name);
    return true;
}

std::shared_ptr<const UEClassInfo> UETypeRegistry::findClass(const std::string& name) const {
    auto it = registeredClasses_.find(name);
    if (it != registeredClasses_.end()) {
        return it->second;
    }
    return nullptr;
}

AST::TypePtr UETypeRegistry::findNativeType(const std::string& ueTypeName) const {
    auto it = nativeTypes_.find(ueTypeName);
    if (it != nativeTypes_.end()) {
        return it->second;
    }
    return nullptr;
}

std::string UETypeRegistry::findUETypeName(AST::TypePtr kiwiType) const {
    auto it = typeToUEName_.find(kiwiType);
    if (it != typeToUEName_.end()) {
        return it->second;
    }
    return "";
}

bool UETypeRegistry::generateTypeDeclarations() const {
    bool success = true;
    
    for (const auto& [name, classInfo] : registeredClasses_) {
        // Generate UCLASS macro
        std::ostringstream classMacro;
        classMacro << "UCLASS(";
        
        if (classInfo->isBlueprintType) {
            classMacro << "BlueprintType";
            if (classInfo->isBlueprintable) {
                classMacro << ", Blueprintable";
            }
        }
        
        if (classInfo->isAbstract()) {
            classMacro << ", Abstract";
        }
        
        if (classInfo->isConfig()) {
            classMacro << ", Config=" << classInfo->configName;
        }
        
        classMacro << ")\n";
        
        // Generate class declaration
        std::ostringstream classDecl;
        classDecl << "class " << classInfo->getGeneratedClassName();
        
        if (!classInfo->baseClassName.empty()) {
            classDecl << " : public " << classInfo->baseClassName;
        }
        
        DiagnosticEngine::get().info("Generated declaration for class: " + name);
    }
    
    return success;
}

bool UETypeRegistry::validateTypeCompatibility() const {
    bool compatible = true;
    
    for (const auto& [name, classInfo] : registeredClasses_) {
        for (const auto& prop : classInfo->properties) {
            if (!prop.kiwiType) {
                DiagnosticEngine::get().error("Property has null kiwi type: " + prop.name + " in class " + name);
                compatible = false;
                continue;
            }
            
            // Check if type is registered
            std::string ueTypeName = findUETypeName(prop.kiwiType);
            if (ueTypeName.empty()) {
                DiagnosticEngine::get().warning("Unregistered type used in property: " + prop.name);
            }
        }
        
        for (const auto& func : classInfo->functions) {
            if (func.returnType && findUETypeName(func.returnType).empty()) {
                DiagnosticEngine::get().warning("Unregistered return type in function: " + func.name);
            }
            
            for (const auto& param : func.parameters) {
                if (param.type && findUETypeName(param.type).empty()) {
                    DiagnosticEngine::get().warning("Unregistered parameter type in function: " + func.name);
                }
            }
        }
    }
    
    return compatible;
}

void UETypeRegistry::registerBuiltinTypes() {
    // Register basic types
    nativeTypes_["bool"] = typeSystem_.getBoolType();
    nativeTypes_["int32"] = typeSystem_.getInt32Type();
    nativeTypes_["int64"] = typeSystem_.getInt64Type();
    nativeTypes_["float"] = typeSystem_.getFloatType();
    nativeTypes_["double"] = typeSystem_.getDoubleType();
    nativeTypes_["FString"] = typeSystem_.getStringType();
    nativeTypes_["FName"] = typeSystem_.createType(AST::TypeKind::Custom, "FName");
    nativeTypes_["FText"] = typeSystem_.createType(AST::TypeKind::Custom, "FText");
    
    // Register common UE types
    nativeTypes_["UObject"] = typeSystem_.createClassType("UObject");
    nativeTypes_["AActor"] = typeSystem_.createClassType("AActor");
    nativeTypes_["APawn"] = typeSystem_.createClassType("APawn");
    nativeTypes_["ACharacter"] = typeSystem_.createClassType("ACharacter");
    nativeTypes_["UClass"] = typeSystem_.createClassType("UClass");
    nativeTypes_["UStruct"] = typeSystem_.createStructType("UStruct");
    
    // Register container types
    AST::TypePtr int32Type = typeSystem_.getInt32Type();
    AST::TypePtr floatType = typeSystem_.getFloatType();
    AST::TypePtr stringType = typeSystem_.getStringType();
    
    // TArray<T>
    auto arrayType = typeSystem_.createTemplateType("TArray", {int32Type});
    nativeTypes_["TArray<int32>"] = arrayType;
    typeToUEName_[arrayType] = "TArray<int32>";
    
    // TMap<K,V>
    auto mapType = typeSystem_.createTemplateType("TMap", {stringType, int32Type});
    nativeTypes_["TMap<FString,int32>"] = mapType;
    typeToUEName_[mapType] = "TMap<FString,int32>";
    
    // TSet<T>
    auto setType = typeSystem_.createTemplateType("TSet", {int32Type});
    nativeTypes_["TSet<int32>"] = setType;
    typeToUEName_[setType] = "TSet<int32>";
}

bool UETypeRegistry::validateClassHierarchy(const UEClassInfo& classInfo) const {
    if (classInfo.baseClassName.empty()) {
        return true; // Root class
    }
    
    // Check if base class is registered
    auto baseClass = findClass(classInfo.baseClassName);
    if (!baseClass) {
        // Base class might be a native UE class
        if (nativeTypes_.find(classInfo.baseClassName) == nativeTypes_.end()) {
            DiagnosticEngine::get().warning("Base class not found: " + classInfo.baseClassName);
        }
        return true; // Allow unregistered base classes (might be native)
    }
    
    // Check for circular inheritance
    std::unordered_set<std::string> visited;
    std::string current = classInfo.baseClassName;
    
    while (!current.empty()) {
        if (visited.find(current) != visited.end()) {
            DiagnosticEngine::get().error("Circular inheritance detected involving: " + current);
            return false;
        }
        
        visited.insert(current);
        
        auto nextClass = findClass(current);
        if (!nextClass) {
            break; // Reached native class
        }
        
        current = nextClass->baseClassName;
    }
    
    return true;
}

bool UETypeRegistry::validatePropertyType(const UEPropertyInfo& prop) const {
    if (!prop.kiwiType) {
        DiagnosticEngine::get().error("Property has null type: " + prop.name);
        return false;
    }
    
    // Check if type is supported
    std::string ueTypeName = findUETypeName(prop.kiwiType);
    if (ueTypeName.empty()) {
        // Try to see if it's a builtin type
        if (prop.kiwiType->isBuiltin()) {
            return true;
        }
        
        DiagnosticEngine::get().warning("Unregistered type in property: " + prop.name);
    }
    
    // Validate array dimensions
    if (prop.arraySize > 0 && !prop.kiwiType->isValidForArray()) {
        DiagnosticEngine::get().error("Type cannot be used in array: " + prop.name);
        return false;
    }
    
    return true;
}

} // namespace Unreal
} // namespace kiwiLang