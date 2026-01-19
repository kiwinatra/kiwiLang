#include "kiwiLang/Unreal/UEFunctions.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include "kiwiLang/Unreal/Types/UETypeConverter.h"
#include "kiwiLang/Unreal/CodeGen/UECodeGenerator.h"
#include "kiwiLang/Diagnostics/DiagnosticEngine.h"
#include "kiwiLang/AST/Type.h"
#include "kiwiLang/Sema/TypeSystem.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace kiwiLang {
namespace Unreal {

bool UEFunctionCallContext::validate() const {
    if (!object) {
        DiagnosticEngine::get().error("Null object in function call context");
        return false;
    }
    
    if (!classBinding) {
        DiagnosticEngine::get().error("Null class binding in function call context");
        return false;
    }
    
    if (!functionBinding) {
        DiagnosticEngine::get().error("Null function binding in function call context");
        return false;
    }
    
    return true;
}

bool UEFunctionSignature::matches(const UEFunctionSignature& other) const {
    if (name != other.name) return false;
    if (parameterTypes.size() != other.parameterTypes.size()) return false;
    
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        if (!parameterTypes[i]->isSameType(*other.parameterTypes[i])) {
            return false;
        }
    }
    
    if ((returnType == nullptr) != (other.returnType == nullptr)) return false;
    if (returnType && other.returnType && 
        !returnType->isSameType(*other.returnType)) {
        return false;
    }
    
    return true;
}

std::size_t UEFunctionSignature::calculateHash() const {
    std::size_t seed = 0;
    std::hash<std::string> stringHasher;
    std::hash<void*> pointerHasher;
    
    seed ^= stringHasher(name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    
    for (const auto& type : parameterTypes) {
        seed ^= pointerHasher(type.get()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    
    if (returnType) {
        seed ^= pointerHasher(returnType.get()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    
    seed ^= flags + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash = static_cast<std::uint32_t>(seed);
    return seed;
}

UEFunctionDispatcher::UEFunctionDispatcher(UObjectRegistry* registry)
    : registry_(registry) {
}

UEFunctionDispatcher::~UEFunctionDispatcher() = default;

Runtime::Value UEFunctionDispatcher::dispatch(const std::string& functionName,
                                            void* object,
                                            const std::vector<Runtime::Value>& args) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DiagnosticEngine::get().info("Dispatching function: " + functionName);
    
    auto handlerIt = handlers_.find(functionName);
    if (handlerIt != handlers_.end()) {
        try {
            return handlerIt->second(object, args);
        } catch (const std::exception& e) {
            DiagnosticEngine::get().error("Handler exception for " + functionName + 
                                         ": " + std::string(e.what()));
            return Runtime::Value::null();
        }
    }
    
    if (!registry_) {
        DiagnosticEngine::get().error("Registry not available for function: " + functionName);
        return Runtime::Value::null();
    }
    
    UFunctionBindingBase* binding = registry_->findFunctionBinding(functionName);
    if (!binding) {
        DiagnosticEngine::get().error("Function binding not found: " + functionName);
        return Runtime::Value::null();
    }
    
    std::vector<void*> rawArgs;
    rawArgs.reserve(args.size());
    
    for (const auto& arg : args) {
        if (arg.isInteger()) {
            int64_t* val = new int64_t(arg.asInteger());
            rawArgs.push_back(val);
        } else if (arg.isFloating()) {
            double* dbl = new double(arg.asFloating());
            rawArgs.push_back(dbl);
        } else if (arg.isString()) {
            std::string* str = new std::string(arg.asString());
            rawArgs.push_back(str);
        } else if (arg.isPointer()) {
            rawArgs.push_back(const_cast<void*>(arg.asPointer()));
        } else if (arg.isBoolean()) {
            bool* b = new bool(arg.asBoolean());
            rawArgs.push_back(b);
        } else {
            rawArgs.push_back(nullptr);
        }
    }
    
    void* rawResult = binding->invoke(object, rawArgs.data());
    
    for (size_t i = 0; i < rawArgs.size(); ++i) {
        if (args[i].isInteger()) {
            delete static_cast<int64_t*>(rawArgs[i]);
        } else if (args[i].isFloating()) {
            delete static_cast<double*>(rawArgs[i]);
        } else if (args[i].isString()) {
            delete static_cast<std::string*>(rawArgs[i]);
        } else if (args[i].isBoolean()) {
            delete static_cast<bool*>(rawArgs[i]);
        }
    }
    
    if (!rawResult) {
        return Runtime::Value::null();
    }
    
    const UEFunctionBindingParams& params = binding->getParams();
    if (!params.returnType) {
        return Runtime::Value::null();
    }
    
    if (params.returnType->isIntegerType()) {
        return Runtime::Value::integer(*static_cast<int64_t*>(rawResult));
    } else if (params.returnType->isFloatingType()) {
        return Runtime::Value::floating(*static_cast<double*>(rawResult));
    } else if (params.returnType->isBooleanType()) {
        return Runtime::Value::boolean(*static_cast<bool*>(rawResult));
    } else if (params.returnType->isStringType()) {
        return Runtime::Value::string(*static_cast<std::string*>(rawResult));
    } else {
        return Runtime::Value::pointer(rawResult);
    }
}

bool UEFunctionDispatcher::registerFunction(const std::string& functionName,
                                          std::function<Runtime::Value(void*, 
                                          const std::vector<Runtime::Value>&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (handlers_.find(functionName) != handlers_.end()) {
        DiagnosticEngine::get().warning("Function already registered: " + functionName);
        return false;
    }
    
    handlers_[functionName] = std::move(handler);
    DiagnosticEngine::get().info("Registered function handler: " + functionName);
    return true;
}

bool UEFunctionDispatcher::unregisterFunction(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = handlers_.find(functionName);
    if (it == handlers_.end()) {
        DiagnosticEngine::get().warning("Function not registered: " + functionName);
        return false;
    }
    
    handlers_.erase(it);
    DiagnosticEngine::get().info("Unregistered function handler: " + functionName);
    return true;
}

std::vector<std::string> UEFunctionDispatcher::getRegisteredFunctions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> functions;
    functions.reserve(handlers_.size());
    
    for (const auto& [name, _] : handlers_) {
        functions.push_back(name);
    }
    
    return functions;
}

UEFunctionResolver::UEFunctionResolver(UETypeRegistry* typeRegistry,
                                     UETypeConverter* typeConverter)
    : typeRegistry_(typeRegistry)
    , typeConverter_(typeConverter) {
    
    buildDefaultSignatures();
}

UEFunctionResolver::~UEFunctionResolver() = default;

UEFunctionSignature UEFunctionResolver::resolveSignature(const std::string& functionName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto range = signatures_.equal_range(functionName);
    if (range.first == range.second) {
        DiagnosticEngine::get().warning("No signature found for function: " + functionName);
        return UEFunctionSignature{};
    }
    
    return range.first->second;
}

bool UEFunctionResolver::validateCall(const std::string& functionName,
                                     const std::vector<Runtime::Value>& args,
                                     std::vector<std::string>& errors) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto range = signatures_.equal_range(functionName);
    if (range.first == range.second) {
        errors.push_back("Function not found: " + functionName);
        return false;
    }
    
    for (auto it = range.first; it != range.second; ++it) {
        const UEFunctionSignature& sig = it->second;
        
        if (sig.parameterTypes.size() != args.size()) {
            continue;
        }
        
        bool compatible = true;
        for (size_t i = 0; i < args.size(); ++i) {
            AST::TypePtr argType = nullptr;
            
            if (args[i].isInteger()) {
                argType = typeRegistry_->findNativeType("int64");
            } else if (args[i].isFloating()) {
                argType = typeRegistry_->findNativeType("double");
            } else if (args[i].isBoolean()) {
                argType = typeRegistry_->findNativeType("bool");
            } else if (args[i].isString()) {
                argType = typeRegistry_->findNativeType("FString");
            } else if (args[i].isPointer()) {
                argType = typeRegistry_->findNativeType("UObject*");
            }
            
            if (!argType || !typesCompatible(sig.parameterTypes[i], argType)) {
                compatible = false;
                break;
            }
        }
        
        if (compatible) {
            return true;
        }
    }
    
    errors.push_back("No compatible signature found for function: " + functionName);
    return false;
}

std::vector<UEFunctionSignature> UEFunctionResolver::findOverloads(const std::string& functionName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<UEFunctionSignature> overloads;
    auto range = signatures_.equal_range(functionName);
    
    for (auto it = range.first; it != range.second; ++it) {
        overloads.push_back(it->second);
    }
    
    return overloads;
}

bool UEFunctionResolver::addOverload(const UEFunctionSignature& signature) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    signature.calculateHash();
    
    auto range = signatures_.equal_range(signature.name);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.matches(signature)) {
            DiagnosticEngine::get().warning("Duplicate signature for function: " + signature.name);
            return false;
        }
    }
    
    signatures_.emplace(signature.name, signature);
    DiagnosticEngine::get().info("Added overload for function: " + signature.name);
    return true;
}

bool UEFunctionResolver::removeOverload(const std::string& functionName, std::uint32_t signatureHash) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto range = signatures_.equal_range(functionName);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.hash == signatureHash) {
            signatures_.erase(it);
            DiagnosticEngine::get().info("Removed overload for function: " + functionName);
            return true;
        }
    }
    
    DiagnosticEngine::get().warning("Signature not found: " + functionName);
    return false;
}

AST::TypePtr UEFunctionResolver::resolveType(const std::string& typeName) const {
    if (!typeRegistry_) {
        return nullptr;
    }
    
    AST::TypePtr type = typeRegistry_->findNativeType(typeName);
    if (!type && typeConverter_) {
        auto result = typeConverter_->convertType(typeName, 
            UETypeConversionDirection::UEToKiwi);
        if (result.success) {
            type = result.convertedType;
        }
    }
    
    return type;
}

bool UEFunctionResolver::typesCompatible(AST::TypePtr expected, AST::TypePtr actual) const {
    if (!expected || !actual) {
        return false;
    }
    
    if (expected->isSameType(*actual)) {
        return true;
    }
    
    if (expected->isIntegerType() && actual->isIntegerType()) {
        return expected->getBitWidth() >= actual->getBitWidth();
    }
    
    if (expected->isFloatingType() && actual->isFloatingType()) {
        return expected->getBitWidth() >= actual->getBitWidth();
    }
    
    if (expected->isFloatingType() && actual->isIntegerType()) {
        return true;
    }
    
    return false;
}

void UEFunctionResolver::buildDefaultSignatures() {
    struct DefaultFunction {
        std::string name;
        std::vector<std::string> paramTypes;
        std::string returnType;
        std::uint32_t flags;
    };
    
    std::vector<DefaultFunction> defaults = {
        {"BeginPlay", {}, "void", 0},
        {"Tick", {"float"}, "void", 0},
        {"Destroy", {}, "void", 0},
        {"GetActorLocation", {}, "FVector", 0},
        {"SetActorLocation", {"FVector"}, "void", 0},
        {"GetActorRotation", {}, "FRotator", 0},
        {"SetActorRotation", {"FRotator"}, "void", 0},
        {"GetActorScale", {}, "FVector", 0},
        {"SetActorScale", {"FVector"}, "void", 0},
        {"GetComponentByClass", {"UClass*"}, "UActorComponent*", 0},
        {"AddComponentByClass", {"UClass*", "FString", "bool", "FTransform", "bool"}, "UActorComponent*", 0},
        {"RemoveComponent", {"UActorComponent*"}, "void", 0},
        {"GetOverlappingActors", {"TArray<AActor*>&", "UClass*"}, "void", 0},
        {"GetDistanceTo", {"AActor*"}, "float", 0},
        {"GetHorizontalDistanceTo", {"AActor*"}, "float", 0},
        {"GetVerticalDistanceTo", {"AActor*"}, "float", 0},
        {"LineTraceSingleByChannel", {"FHitResult&", "FVector", "FVector", "ECollisionChannel", "FCollisionQueryParams"}, "bool", 0},
        {"SphereTraceSingleByChannel", {"FHitResult&", "FVector", "FVector", "float", "ECollisionChannel", "FCollisionQueryParams"}, "bool", 0},
        {"GetWorld", {}, "UWorld*", 0},
        {"GetGameInstance", {}, "UGameInstance*", 0},
        {"GetPlayerController", {"int32"}, "APlayerController*", 0},
        {"GetPawn", {}, "APawn*", 0},
        {"GetCharacter", {}, "ACharacter*", 0},
        {"K2_DestroyActor", {}, "void", 0},
        {"K2_SetActorLocation", {"FVector", "bool", "FHitResult&", "bool"}, "bool", 0},
        {"K2_SetActorRotation", {"FRotator", "bool"}, "bool", 0},
        {"K2_AddActorWorldOffset", {"FVector", "bool", "FHitResult&", "bool"}, "void", 0},
        {"K2_AddActorWorldRotation", {"FRotator", "bool", "FHitResult&", "bool"}, "void", 0},
        {"K2_AddActorLocalOffset", {"FVector", "bool", "FHitResult&", "bool"}, "void", 0},
        {"K2_AddActorLocalRotation", {"FRotator", "bool", "FHitResult&", "bool"}, "void", 0},
        {"K2_GetActorLocation", {}, "FVector", 0},
        {"K2_GetActorRotation", {}, "FRotator", 0},
        {"K2_GetActorScale", {}, "FVector", 0},
        {"K2_SetActorScale", {"FVector"}, "void", 0},
        {"ReceiveBeginPlay", {}, "void", 0},
        {"ReceiveTick", {"float"}, "void", 0},
        {"ReceiveDestroyed", {}, "void", 0},
        {"ReceiveActorBeginOverlap", {"AActor*"}, "void", 0},
        {"ReceiveActorEndOverlap", {"AActor*"}, "void", 0},
        {"ReceiveHit", {"UPrimitiveComponent*", "AActor*", "UPrimitiveComponent*", "bool", "FVector", "FVector", "FVector", "const FHitResult&"}, "void", 0},
        {"ReceiveRadialDamage", {"float", "UDamageType*", "FVector", "FHitResult", "AController*", "AActor*"}, "void", 0},
        {"ReceivePointDamage", {"float", "UDamageType*", "FVector", "FHitResult", "AController*", "AActor*"}, "void", 0},
        {"ReceiveAnyDamage", {"float", "UDamageType*", "AController*", "AActor*"}, "void", 0}
    };
    
    for (const auto& df : defaults) {
        UEFunctionSignature sig;
        sig.name = df.name;
        sig.flags = df.flags;
        
        for (const auto& paramType : df.paramTypes) {
            AST::TypePtr type = resolveType(paramType);
            if (type) {
                sig.parameterTypes.push_back(type);
            }
        }
        
        sig.returnType = resolveType(df.returnType);
        sig.calculateHash();
        
        signatures_.emplace(sig.name, sig);
    }
    
    DiagnosticEngine::get().info("Built " + std::to_string(defaults.size()) + " default function signatures");
}

UEFunctionOptimizer::UEFunctionOptimizer(UECodeGenerator* codeGenerator)
    : codeGenerator_(codeGenerator) {
}

UEFunctionOptimizer::~UEFunctionOptimizer() = default;

bool UEFunctionOptimizer::optimizeCallSite(const std::string& functionName,
                                         const UEFunctionCallContext& context,
                                         std::vector<uint8_t>& optimizedCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::uint32_t hash = calculateCallSiteHash(functionName, context);
    
    if (checkCache(hash, optimizedCode)) {
        DiagnosticEngine::get().info("Cache hit for call site optimization: " + functionName);
        return true;
    }
    
    if (!context.validate()) {
        DiagnosticEngine::get().error("Invalid context for optimization");
        return false;
    }
    
    std::stringstream codeStream;
    
    codeStream << "// Optimized call site for: " << functionName << "\n";
    codeStream << "// Generated by kiwiLang optimizer\n\n";
    
    codeStream << "inline auto " << functionName << "_Optimized(void* obj";
    
    for (size_t i = 0; i < context.arguments.size(); ++i) {
        codeStream << ", auto arg" << i;
    }
    
    codeStream << ") {\n";
    codeStream << "    if (!obj) return decltype(auto){};\n\n";
    
    codeStream << "    // Direct vtable call optimization\n";
    codeStream << "    auto result = reinterpret_cast<" << functionName << "_FuncPtr>(obj->vtable["
              << hash % 256 << "])(obj";
    
    for (size_t i = 0; i < context.arguments.size(); ++i) {
        codeStream << ", arg" << i;
    }
    
    codeStream << ");\n\n";
    codeStream << "    // Branch prediction hint\n";
    codeStream << "    __builtin_expect(result != nullptr, 1);\n";
    codeStream << "    return result;\n";
    codeStream << "}\n";
    
    std::string code = codeStream.str();
    optimizedCode.assign(code.begin(), code.end());
    
    updateCache(hash, optimizedCode);
    DiagnosticEngine::get().info("Optimized call site for: " + functionName);
    return true;
}

bool UEFunctionOptimizer::inlineFunction(const std::string& functionName,
                                       const UEFunctionSignature& signature,
                                       std::vector<uint8_t>& inlinedCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::uint32_t hash = calculateInliningHash(functionName, signature);
    
    if (checkCache(hash, inlinedCode)) {
        DiagnosticEngine::get().info("Cache hit for inlining: " + functionName);
        return true;
    }
    
    std::stringstream codeStream;
    
    codeStream << "// Inlined function: " << functionName << "\n";
    codeStream << "// Signature hash: " << hash << "\n\n";
    
    codeStream << "#define " << functionName << "_INLINE \\\n";
    
    if (signature.returnType) {
        codeStream << "    [](";
    } else {
        codeStream << "    [](";
    }
    
    for (size_t i = 0; i < signature.parameterTypes.size(); ++i) {
        codeStream << "auto p" << i;
        if (i != signature.parameterTypes.size() - 1) {
            codeStream << ", ";
        }
    }
    
    codeStream << ") -> auto { \\\n";
    
    codeStream << "        // Inlined body \\\n";
    codeStream << "        if constexpr (sizeof...(decltype(p0))) { \\\n";
    codeStream << "            return (p0 + ...); \\\n";
    codeStream << "        } else { \\\n";
    codeStream << "            return; \\\n";
    codeStream << "        } \\\n";
    codeStream << "    }\n";
    
    std::string code = codeStream.str();
    inlinedCode.assign(code.begin(), code.end());
    
    updateCache(hash, inlinedCode);
    DiagnosticEngine::get().info("Inlined function: " + functionName);
    return true;
}

bool UEFunctionOptimizer::specializeTemplate(const std::string& functionName,
                                           const std::vector<AST::TypePtr>& templateArgs,
                                           std::vector<uint8_t>& specializedCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::uint32_t hash = calculateSpecializationHash(functionName, templateArgs);
    
    if (checkCache(hash, specializedCode)) {
        DiagnosticEngine::get().info("Cache hit for specialization: " + functionName);
        return true;
    }
    
    std::stringstream codeStream;
    
    codeStream << "// Template specialization for: " << functionName << "\n";
    codeStream << "template<";
    
    for (size_t i = 0; i < templateArgs.size(); ++i) {
        codeStream << "typename T" << i;
        if (i != templateArgs.size() - 1) {
            codeStream << ", ";
        }
    }
    
    codeStream << ">\n";
    codeStream << "auto " << functionName << "_Specialized(";
    
    for (size_t i = 0; i < templateArgs.size(); ++i) {
        codeStream << "T" << i << " arg" << i;
        if (i != templateArgs.size() - 1) {
            codeStream << ", ";
        }
    }
    
    codeStream << ") {\n";
    codeStream << "    // Specialized implementation\n";
    
    if (!templateArgs.empty() && templateArgs[0]->isIntegerType()) {
        codeStream << "    if constexpr (std::is_integral_v<T0>) {\n";
        codeStream << "        return arg0 * 2; // Integer specialization\n";
        codeStream << "    }\n";
    }
    
    if (!templateArgs.empty() && templateArgs[0]->isFloatingType()) {
        codeStream << "    if constexpr (std::is_floating_point_v<T0>) {\n";
        codeStream << "        return arg0 * 3.14; // Float specialization\n";
        codeStream << "    }\n";
    }
    
    codeStream << "    return arg0; // Default\n";
    codeStream << "}\n";
    
    std::string code = codeStream.str();
    specializedCode.assign(code.begin(), code.end());
    
    updateCache(hash, specializedCode);
    DiagnosticEngine::get().info("Specialized template: " + functionName);
    return true;
}

bool UEFunctionOptimizer::generateThunk(const std::string& functionName,
                                      const UEFunctionSignature& fromSignature,
                                      const UEFunctionSignature& toSignature,
                                      std::vector<uint8_t>& thunkCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::uint32_t hash = calculateThunkHash(fromSignature, toSignature);
    
    if (checkCache(hash, thunkCode)) {
        DiagnosticEngine::get().info("Cache hit for thunk: " + functionName);
        return true;
    }
    
    std::stringstream codeStream;
    
    codeStream << "// Thunk from " << fromSignature.name << " to " << toSignature.name << "\n";
    codeStream << "extern \"C\" __declspec(naked) void " << functionName << "_Thunk() {\n";
    codeStream << "    __asm {\n";
    codeStream << "        // Prolog\n";
    codeStream << "        push ebp\n";
    codeStream << "        mov ebp, esp\n";
    codeStream << "        sub esp, 0x20\n\n";
    
    codeStream << "        // Parameter conversion\n";
    
    for (size_t i = 0; i < fromSignature.parameterTypes.size(); ++i) {
        codeStream << "        mov eax, [ebp + " << (8 + i * 4) << "]\n";
        if (i < toSignature.parameterTypes.size()) {
            codeStream << "        push eax\n";
        }
    }
    
    codeStream << "\n        // Call target\n";
    codeStream << "        call " << toSignature.name << "\n\n";
    
    codeStream << "        // Epilog\n";
    codeStream << "        mov esp, ebp\n";
    codeStream << "        pop ebp\n";
    codeStream << "        ret\n";
    codeStream << "    }\n";
    codeStream << "}\n";
    
    std::string code = codeStream.str();
    thunkCode.assign(code.begin(), code.end());
    
    updateCache(hash, thunkCode);
    DiagnosticEngine::get().info("Generated thunk for: " + functionName);
    return true;
}

std::uint32_t UEFunctionOptimizer::calculateCallSiteHash(const std::string& functionName,
                                                        const UEFunctionCallContext& context) {
    std::hash<std::string> stringHasher;
    std::hash<void*> pointerHasher;
    
    std::uint32_t hash = stringHasher(functionName);
    hash ^= pointerHasher(context.object) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= context.arguments.size() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    for (const auto& arg : context.arguments) {
        hash ^= arg.getType() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    
    return hash;
}

std::uint32_t UEFunctionOptimizer::calculateInliningHash(const std::string& functionName,
                                                        const UEFunctionSignature& signature) {
    std::hash<std::string> stringHasher;
    
    std::uint32_t hash = stringHasher(functionName);
    hash ^= signature.hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= signature.parameterTypes.size() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    return hash;
}

std::uint32_t UEFunctionOptimizer::calculateSpecializationHash(const std::string& functionName,
                                                              const std::vector<AST::TypePtr>& templateArgs) {
    std::hash<std::string> stringHasher;
    std::hash<void*> pointerHasher;
    
    std::uint32_t hash = stringHasher(functionName);
    
    for (const auto& type : templateArgs) {
        hash ^= pointerHasher(type.get()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    
    return hash;
}

std::uint32_t UEFunctionOptimizer::calculateThunkHash(const UEFunctionSignature& from,
                                                     const UEFunctionSignature& to) {
    return from.hash ^ to.hash;
}

bool UEFunctionOptimizer::checkCache(std::uint32_t hash, std::vector<uint8_t>& output) const {
    auto it = cache_.callSiteCache.find(hash);
    if (it != cache_.callSiteCache.end()) {
        output = it->second;
        return true;
    }
    
    it = cache_.inliningCache.find(hash);
    if (it != cache_.inliningCache.end()) {
        output = it->second;
        return true;
    }
    
    it = cache_.specializationCache.find(hash);
    if (it != cache_.specializationCache.end()) {
        output = it->second;
        return true;
    }
    
    it = cache_.thunkCache.find(hash);
    if (it != cache_.thunkCache.end()) {
        output = it->second;
        return true;
    }
    
    return false;
}

void UEFunctionOptimizer::updateCache(std::uint32_t hash, const std::vector<uint8_t>& code) {
    cache_.callSiteCache[hash] = code;
}

UEFunctionProfiler::UEFunctionProfiler() = default;

UEFunctionProfiler::~UEFunctionProfiler() = default;

double UEFunctionProfiler::ProfileData::averageTimeMs() const {
    if (callCount == 0) return 0.0;
    return (totalTimeNs / 1'000'000.0) / callCount;
}

double UEFunctionProfiler::ProfileData::callsPerSecond() const {
    if (totalTimeNs == 0) return 0.0;
    return (callCount * 1'000'000'000.0) / totalTimeNs;
}

void UEFunctionProfiler::ProfileData::reset() {
    callCount = 0;
    totalTimeNs = 0;
    minTimeNs = std::numeric_limits<std::uint64_t>::max();
    maxTimeNs = 0;
    totalMemoryBytes = 0;
    callTimings.clear();
}

void UEFunctionProfiler::startProfiling(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ProfilingState state;
    state.startTime = std::chrono::high_resolution_clock::now();
    state.isProfiling = true;
    
    activeProfiles_[functionName] = state;
}

void UEFunctionProfiler::stopProfiling(const std::string& functionName, 
                                      std::uint64_t memoryUsed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto activeIt = activeProfiles_.find(functionName);
    if (activeIt == activeProfiles_.end()) {
        DiagnosticEngine::get().warning("Profiling not started for function: " + functionName);
        return;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::uint64_t durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTime - activeIt->second.startTime).count();
    
    updateProfileData(functionName, durationNs, memoryUsed);
    activeProfiles_.erase(activeIt);
}

UEFunctionProfiler::ProfileData UEFunctionProfiler::getProfileData(const std::string& functionName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = profiles_.find(functionName);
    if (it != profiles_.end()) {
        return it->second;
    }
    
    return ProfileData{};
}

std::vector<std::string> UEFunctionProfiler::getProfiledFunctions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> functionNames;
    functionNames.reserve(profiles_.size());
    
    for (const auto& [name, _] : profiles_) {
        functionNames.push_back(name);
    }
    
    return functionNames;
}

bool UEFunctionProfiler::saveProfileReport(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            DiagnosticEngine::get().error("Cannot open file for profile report: " + filePath);
            return false;
        }
        
        file << "KiwiLang Function Profile Report\n";
        file << "================================\n\n";
        
        file << "Total functions profiled: " << profiles_.size() << "\n\n";
        
        for (const auto& [functionName, data] : profiles_) {
            file << "Function: " << functionName << "\n";
            file << "  Call count: " << data.callCount << "\n";
            file << "  Total time: " << (data.totalTimeNs / 1'000'000.0) << " ms\n";
            file << "  Average time: " << data.averageTimeMs() << " ms\n";
            file << "  Min time: " << (data.minTimeNs / 1'000'000.0) << " ms\n";
            file << "  Max time: " << (data.maxTimeNs / 1'000'000.0) << " ms\n";
            file << "  Calls per second: " << data.callsPerSecond() << "\n";
            file << "  Memory used: " << (data.totalMemoryBytes / 1024.0) << " KB\n\n";
        }
        
        DiagnosticEngine::get().info("Saved profile report to: " + filePath);
        return true;
        
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to save profile report: " + std::string(e.what()));
        return false;
    }
}

bool UEFunctionProfiler::loadProfileReport(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            DiagnosticEngine::get().error("Cannot open profile report: " + filePath);
            return false;
        }
        
        std::string line;
        ProfileData currentData;
        std::string currentFunction;
        
        while (std::getline(file, line)) {
            if (line.find("Function: ") == 0) {
                if (!currentFunction.empty()) {
                    profiles_[currentFunction] = currentData;
                    currentData.reset();
                }
                currentFunction = line.substr(9);
            } else if (line.find("Call count: ") != std::string::npos) {
                currentData.callCount = std::stoull(line.substr(12));
            } else if (line.find("Total time: ") != std::string::npos) {
                std::string timeStr = line.substr(12);
                size_t msPos = timeStr.find(" ms");
                if (msPos != std::string::npos) {
                    double ms = std::stod(timeStr.substr(0, msPos));
                    currentData.totalTimeNs = static_cast<std::uint64_t>(ms * 1'000'000);
                }
            }
        }
        
        if (!currentFunction.empty()) {
            profiles_[currentFunction] = currentData;
        }
        
        DiagnosticEngine::get().info("Loaded profile report from: " + filePath);
        return true;
        
    } catch (const std::exception& e) {
        DiagnosticEngine::get().error("Failed to load profile report: " + std::string(e.what()));
        return false;
    }
}

void UEFunctionProfiler::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    profiles_.clear();
    activeProfiles_.clear();
    DiagnosticEngine::get().info("Reset all profiling data");
}

std::uint64_t UEFunctionProfiler::getCurrentTimeNs() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void UEFunctionProfiler::updateProfileData(const std::string& functionName,
                                          std::uint64_t durationNs,
                                          std::uint64_t memoryUsed) {
    ProfileData& data = profiles_[functionName];
    
    data.functionName = functionName;
    data.callCount++;
    data.totalTimeNs += durationNs;
    data.minTimeNs = std::min(data.minTimeNs, durationNs);
    data.maxTimeNs = std::max(data.maxTimeNs, durationNs);
    data.totalMemoryBytes += memoryUsed;
    data.callTimings.push_back(durationNs);
    
    if (data.callTimings.size() > 1000) {
        data.callTimings.erase(data.callTimings.begin());
    }
}

UEFunctionValidator::UEFunctionValidator(UETypeRegistry* typeRegistry,
                                       UETypeConverter* typeConverter)
    : typeRegistry_(typeRegistry)
    , typeConverter_(typeConverter) {
}

UEFunctionValidator::~UEFunctionValidator() = default;

UEFunctionValidator::ValidationResult UEFunctionValidator::validateFunction(
    const std::string& functionName,
    const UEFunctionSignature& signature) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    ValidationResult result;
    result.isValid = true;
    
    if (functionName.empty()) {
        result.errors.push_back("Function name cannot be empty");
        result.isValid = false;
    }
    
    if (functionName.find(" ") != std::string::npos) {
        result.errors.push_back("Function name cannot contain spaces: " + functionName);
        result.isValid = false;
    }
    
    if (!validateParameterTypes(signature.parameterTypes, result.errors)) {
        result.isValid = false;
    }
    
    if (!validateReturnType(signature.returnType, result.errors)) {
        result.isValid = false;
    }
    
    if (checkRecursiveCall(functionName, {functionName})) {
        result.errors.push_back("Recursive function detected: " + functionName);
        result.isValid = false;
    }
    
    result.score = calculateComplexityScore(signature);
    
    if (result.score > 80) {
        result.warnings.push_back("Function complexity too high: " + std::to_string(result.score));
    }
    
    return result;
}

UEFunctionValidator::ValidationResult UEFunctionValidator::validateCall(
    const std::string& functionName,
    const std::vector<Runtime::Value>& args,
    const UEFunctionSignature& signature) {
    
    ValidationResult result;
    result.isValid = true;
    
    if (args.size() != signature.parameterTypes.size()) {
        result.errors.push_back("Argument count mismatch: expected " + 
                               std::to_string(signature.parameterTypes.size()) + 
                               ", got " + std::to_string(args.size()));
        result.isValid = false;
    }
    
    for (size_t i = 0; i < std::min(args.size(), signature.parameterTypes.size()); ++i) {
        AST::TypePtr expectedType = signature.parameterTypes[i];
        Runtime::Value::Type actualType = args[i].getType();
        
        bool compatible = false;
        
        if (expectedType->isIntegerType()) {
            compatible = actualType == Runtime::Value::Type::Integer ||
                        actualType == Runtime::Value::Type::UInteger;
        } else if (expectedType->isFloatingType()) {
            compatible = actualType == Runtime::Value::Type::Floating;
        } else if (expectedType->isBooleanType()) {
            compatible = actualType == Runtime::Value::Type::Boolean;
        } else if (expectedType->isStringType()) {
            compatible = actualType == Runtime::Value::Type::String;
        } else if (expectedType->isClassType() || expectedType->isStructType()) {
            compatible = actualType == Runtime::Value::Type::Pointer;
        }
        
        if (!compatible) {
            result.errors.push_back("Argument " + std::to_string(i) + 
                                   " type mismatch for function: " + functionName);
            result.isValid = false;
        }
    }
    
    result.score = calculateComplexityScore(signature);
    return result;
}

bool UEFunctionValidator::checkCircularDependencies(const std::vector<std::string>& functionChain) {
    if (functionChain.empty()) {
        return false;
    }
    
    std::string lastFunction = functionChain.back();
    
    for (size_t i = 0; i < functionChain.size() - 1; ++i) {
        if (functionChain[i] == lastFunction) {
            return true;
        }
    }
    
    return false;
}

bool UEFunctionValidator::validateParameterTypes(const std::vector<AST::TypePtr>& paramTypes,
                                               std::vector<std::string>& errors) const {
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        if (!paramTypes[i]) {
            errors.push_back("Parameter " + std::to_string(i) + " has null type");
            return false;
        }
        
        if (paramTypes[i]->isVoidType()) {
            errors.push_back("Parameter " + std::to_string(i) + " cannot be void");
            return false;
        }
    }
    
    return true;
}

bool UEFunctionValidator::validateReturnType(AST::TypePtr returnType,
                                           std::vector<std::string>& errors) const {
    if (!returnType) {
        return true;
    }
    
    if (returnType->isVoidType()) {
        return true;
    }
    
    if (!typeRegistry_) {
        errors.push_back("Type registry not available for return type validation");
        return false;
    }
    
    return true;
}

bool UEFunctionValidator::checkRecursiveCall(const std::string& functionName,
                                           const std::vector<std::string>& callChain) {
    if (validatingFunctions_.find(functionName) != validatingFunctions_.end()) {
        return true;
    }
    
    validatingFunctions_.insert(functionName);
    
    bool recursive = false;
    
    for (const auto& calledFunc : callChain) {
        if (calledFunc == functionName) {
            recursive = true;
            break;
        }
    }
    
    validatingFunctions_.erase(functionName);
    return recursive;
}

std::uint32_t UEFunctionValidator::calculateComplexityScore(const UEFunctionSignature& signature) const {
    std::uint32_t score = 0;
    
    score += signature.parameterTypes.size() * 5;
    
    if (signature.returnType && !signature.returnType->isVoidType()) {
        score += 10;
    }
    
    for (const auto& type : signature.parameterTypes) {
        if (type->isClassType() || type->isStructType()) {
            score += 15;
        } else if (type->isArrayType() || type->isMapType()) {
            score += 20;
        } else if (type->isTemplateType()) {
            score += 25;
        }
    }
    
    return std::min(score, 100u);
}

} // namespace Unreal
} // namespace kiwiLang