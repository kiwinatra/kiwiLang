#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Unreal/Bindings/UObjectRegistry.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UECodeGenerator;
class UETypeConverter;

struct UEFunctionCallContext {
    void* object;
    UClassBindingBase* classBinding;
    UFunctionBindingBase* functionBinding;
    std::vector<Runtime::Value> arguments;
    Runtime::Value returnValue;
    bool succeeded;
    
    bool validate() const;
};

struct UEFunctionSignature {
    std::string name;
    std::vector<AST::TypePtr> parameterTypes;
    AST::TypePtr returnType;
    std::uint32_t flags;
    std::uint32_t hash;
    
    bool matches(const UEFunctionSignature& other) const;
    std::size_t calculateHash() const;
};

class KIWI_UNREAL_API UEFunctionDispatcher {
public:
    explicit UEFunctionDispatcher(UObjectRegistry* registry);
    ~UEFunctionDispatcher();
    
    Runtime::Value dispatch(const std::string& functionName,
                          void* object,
                          const std::vector<Runtime::Value>& args);
    
    bool registerFunction(const std::string& functionName,
                         std::function<Runtime::Value(void*, const std::vector<Runtime::Value>&)> handler);
    
    bool unregisterFunction(const std::string& functionName);
    
    std::vector<std::string> getRegisteredFunctions() const;
    
private:
    UObjectRegistry* registry_;
    std::unordered_map<std::string, 
        std::function<Runtime::Value(void*, const std::vector<Runtime::Value>&)>> handlers_;
    mutable std::mutex mutex_;
    
    Runtime::Value invokeNative(const std::string& functionName,
                               void* object,
                               const std::vector<Runtime::Value>& args);
    Runtime::Value invokeBlueprint(const std::string& functionName,
                                  void* object,
                                  const std::vector<Runtime::Value>& args);
    Runtime::Value invokeReflection(const std::string& functionName,
                                   void* object,
                                   const std::vector<Runtime::Value>& args);
};

class KIWI_UNREAL_API UEFunctionResolver {
public:
    explicit UEFunctionResolver(UETypeRegistry* typeRegistry,
                              UETypeConverter* typeConverter);
    ~UEFunctionResolver();
    
    UEFunctionSignature resolveSignature(const std::string& functionName) const;
    
    bool validateCall(const std::string& functionName,
                     const std::vector<Runtime::Value>& args,
                     std::vector<std::string>& errors) const;
    
    std::vector<UEFunctionSignature> findOverloads(const std::string& functionName) const;
    
    bool addOverload(const UEFunctionSignature& signature);
    bool removeOverload(const std::string& functionName, std::uint32_t signatureHash);
    
private:
    UETypeRegistry* typeRegistry_;
    UETypeConverter* typeConverter_;
    std::unordered_multimap<std::string, UEFunctionSignature> signatures_;
    mutable std::mutex mutex_;
    
    AST::TypePtr resolveType(const std::string& typeName) const;
    bool typesCompatible(AST::TypePtr expected, AST::TypePtr actual) const;
    void buildDefaultSignatures();
};

class KIWI_UNREAL_API UEFunctionOptimizer {
public:
    explicit UEFunctionOptimizer(UECodeGenerator* codeGenerator);
    ~UEFunctionOptimizer();
    
    bool optimizeCallSite(const std::string& functionName,
                         const UEFunctionCallContext& context,
                         std::vector<uint8_t>& optimizedCode);
    
    bool inlineFunction(const std::string& functionName,
                       const UEFunctionSignature& signature,
                       std::vector<uint8_t>& inlinedCode);
    
    bool specializeTemplate(const std::string& functionName,
                           const std::vector<AST::TypePtr>& templateArgs,
                           std::vector<uint8_t>& specializedCode);
    
    bool generateThunk(const std::string& functionName,
                      const UEFunctionSignature& fromSignature,
                      const UEFunctionSignature& toSignature,
                      std::vector<uint8_t>& thunkCode);
    
private:
    UECodeGenerator* codeGenerator_;
    
    struct OptimizationCache {
        std::unordered_map<std::uint32_t, std::vector<uint8_t>> callSiteCache;
        std::unordered_map<std::uint32_t, std::vector<uint8_t>> inliningCache;
        std::unordered_map<std::uint32_t, std::vector<uint8_t>> specializationCache;
        std::unordered_map<std::uint32_t, std::vector<uint8_t>> thunkCache;
    };
    
    OptimizationCache cache_;
    mutable std::mutex mutex_;
    
    std::uint32_t calculateCallSiteHash(const std::string& functionName,
                                       const UEFunctionCallContext& context);
    std::uint32_t calculateInliningHash(const std::string& functionName,
                                       const UEFunctionSignature& signature);
    std::uint32_t calculateSpecializationHash(const std::string& functionName,
                                            const std::vector<AST::TypePtr>& templateArgs);
    std::uint32_t calculateThunkHash(const UEFunctionSignature& from,
                                    const UEFunctionSignature& to);
    
    bool checkCache(std::uint32_t hash, std::vector<uint8_t>& output) const;
    void updateCache(std::uint32_t hash, const std::vector<uint8_t>& code);
};

class KIWI_UNREAL_API UEFunctionProfiler {
public:
    struct ProfileData {
        std::string functionName;
        std::uint64_t callCount;
        std::uint64_t totalTimeNs;
        std::uint64_t minTimeNs;
        std::uint64_t maxTimeNs;
        std::uint64_t totalMemoryBytes;
        std::vector<std::uint64_t> callTimings;
        
        double averageTimeMs() const;
        double callsPerSecond() const;
        void reset();
    };
    
    explicit UEFunctionProfiler();
    ~UEFunctionProfiler();
    
    void startProfiling(const std::string& functionName);
    void stopProfiling(const std::string& functionName, 
                      std::uint64_t memoryUsed = 0);
    
    ProfileData getProfileData(const std::string& functionName) const;
    std::vector<std::string> getProfiledFunctions() const;
    
    bool saveProfileReport(const std::string& filePath) const;
    bool loadProfileReport(const std::string& filePath);
    
    void resetAll();
    
private:
    struct ProfilingState {
        std::chrono::high_resolution_clock::time_point startTime;
        bool isProfiling;
    };
    
    std::unordered_map<std::string, ProfileData> profiles_;
    std::unordered_map<std::string, ProfilingState> activeProfiles_;
    mutable std::mutex mutex_;
    
    std::uint64_t getCurrentTimeNs() const;
    void updateProfileData(const std::string& functionName,
                          std::uint64_t durationNs,
                          std::uint64_t memoryUsed);
};

class KIWI_UNREAL_API UEFunctionValidator {
public:
    explicit UEFunctionValidator(UETypeRegistry* typeRegistry,
                               UETypeConverter* typeConverter);
    ~UEFunctionValidator();
    
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::uint32_t score; // 0-100
        
        bool hasErrors() const { return !errors.empty(); }
        bool hasWarnings() const { return !warnings.empty(); }
    };
    
    ValidationResult validateFunction(const std::string& functionName,
                                     const UEFunctionSignature& signature);
    
    ValidationResult validateCall(const std::string& functionName,
                                 const std::vector<Runtime::Value>& args,
                                 const UEFunctionSignature& signature);
    
    bool checkCircularDependencies(const std::vector<std::string>& functionChain);
    
private:
    UETypeRegistry* typeRegistry_;
    UETypeConverter* typeConverter_;
    
    std::unordered_set<std::string> validatingFunctions_;
    mutable std::mutex mutex_;
    
    bool validateParameterTypes(const std::vector<AST::TypePtr>& paramTypes,
                               std::vector<std::string>& errors) const;
    bool validateReturnType(AST::TypePtr returnType,
                           std::vector<std::string>& errors) const;
    bool checkRecursiveCall(const std::string& functionName,
                           const std::vector<std::string>& callChain);
    std::uint32_t calculateComplexityScore(const UEFunctionSignature& signature) const;
};

} // namespace Unreal
} // namespace kiwiLang