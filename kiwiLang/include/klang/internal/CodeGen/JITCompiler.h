include/klang/internal/CodeGen/JITCompiler.h
#pragma once
#include "IR.h"
#include "Target.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace klang {
namespace internal {
namespace codegen {

class JITModule;
class JITFunction;

using JITFunctionPtr = void*;
using JITCompileCallback = std::function<void*(FunctionIR&)>;

class JITMemoryManager {
public:
    JITMemoryManager();
    ~JITMemoryManager();
    
    void* allocateCodeMemory(std::size_t size, std::size_t alignment);
    void* allocateDataMemory(std::size_t size, std::size_t alignment);
    
    void protectCodeMemory(void* ptr, std::size_t size);
    void protectDataMemory(void* ptr, std::size_t size);
    
    void freeMemory(void* ptr, std::size_t size);
    
    std::size_t allocatedCodeBytes() const { return allocatedCodeBytes_; }
    std::size_t allocatedDataBytes() const { return allocatedDataBytes_; }

private:
    struct MemoryBlock {
        void* ptr;
        std::size_t size;
        bool isExecutable;
    };
    
    std::vector<MemoryBlock> blocks_;
    std::size_t allocatedCodeBytes_;
    std::size_t allocatedDataBytes_;
    
#ifdef _WIN32
    void* processHeap_;
#else
    int codeMemFd_;
    int dataMemFd_;
#endif
    
    void* allocateMemory(std::size_t size, std::size_t alignment, bool executable);
    void setMemoryProtection(void* ptr, std::size_t size, bool executable);
    
    static constexpr std::size_t PAGE_SIZE = 4096;
};

class JITCodeCache {
public:
    JITCodeCache();
    
    void addFunction(std::string_view name, JITFunctionPtr function, std::size_t size);
    void removeFunction(std::string_view name);
    
    JITFunctionPtr getFunction(std::string_view name) const;
    std::size_t getFunctionSize(std::string_view name) const;
    
    void clear();
    
    std::size_t cachedFunctions() const { return functionCache_.size(); }
    std::size_t cachedBytes() const { return cachedBytes_; }

private:
    struct CacheEntry {
        JITFunctionPtr function;
        std::size_t size;
    };
    
    std::unordered_map<std::string, CacheEntry> functionCache_;
    std::size_t cachedBytes_;
    mutable std::mutex mutex_;
};

class JITCompiler {
public:
    explicit JITCompiler(TargetInfo target);
    ~JITCompiler();
    
    JITFunctionPtr compileFunction(FunctionIR& function);
    void compileModule(ModuleIR& module);
    
    JITFunctionPtr getFunctionAddress(std::string_view name);
    void* getGlobalAddress(std::string_view name);
    
    void setOptimizationLevel(OptimizationLevel level);
    void setCompileCallback(JITCompileCallback callback);
    
    void invalidateFunction(std::string_view name);
    void invalidateAll();
    
    std::size_t compiledFunctions() const { return codeCache_.cachedFunctions(); }
    std::size_t allocatedBytes() const { return memoryManager_.allocatedCodeBytes(); }

private:
    TargetInfo target_;
    OptimizationLevel optLevel_;
    JITMemoryManager memoryManager_;
    JITCodeCache codeCache_;
    JITCompileCallback compileCallback_;
    std::unique_ptr<TargetCodeGen> codeGen_;
    
    struct CompiledFunction {
        std::vector<std::byte> code;
        std::size_t alignment;
    };
    
    CompiledFunction generateMachineCode(FunctionIR& function);
    JITFunctionPtr installFunction(CompiledFunction&& compiled);
    
    void applyRuntimeRelocations(void* code, std::size_t size, 
                                const std::vector<Relocation>& relocs);
    
    static constexpr std::size_t CODE_ALIGNMENT = 16;
};

class JITExecutionEngine {
public:
    explicit JITExecutionEngine(TargetInfo target);
    
    template<typename Ret, typename... Args>
    Ret executeFunction(std::string_view name, Args... args);
    
    void* getFunctionPointer(std::string_view name);
    void addModule(std::unique_ptr<ModuleIR> module);
    void removeModule(std::string_view name);
    
    void setTrampolineSize(std::size_t size) { trampolineSize_ = size; }
    
    const JITCompiler& compiler() const { return compiler_; }

private:
    JITCompiler compiler_;
    std::unordered_map<std::string, std::unique_ptr<ModuleIR>> modules_;
    std::size_t trampolineSize_;
    
    void* createTrampoline(JITFunctionPtr target);
    void* getOrCreateTrampoline(std::string_view name);
    
    static constexpr std::size_t DEFAULT_TRAMPOLINE_SIZE = 32;
};

class LazyJITCompiler {
public:
    explicit LazyJITCompiler(TargetInfo target);
    
    JITFunctionPtr getOrCompileFunction(std::string_view name, 
                                        std::function<FunctionIR*()> generator);
    
    void setCompilationThreshold(std::size_t threshold) { compilationThreshold_ = threshold; }
    
    std::size_t pendingCompilations() const { return pendingFunctions_.size(); }
    
    void compilePendingFunctions();

private:
    JITCompiler compiler_;
    std::unordered_map<std::string, JITFunctionPtr> compiledFunctions_;
    std::unordered_map<std::string, std::function<FunctionIR*()>> pendingFunctions_;
    std::size_t compilationThreshold_;
    std::size_t callCount_;
    
    struct CallSite {
        std::string functionName;
        std::size_t count;
    };
    
    std::vector<CallSite> callSites_;
    
    void recordCall(std::string_view name);
    bool shouldCompile(std::string_view name) const;
    
    static constexpr std::size_t DEFAULT_THRESHOLD = 1000;
};

class TieredJITCompiler {
public:
    explicit TieredJITCompiler(TargetInfo target);
    
    JITFunctionPtr compileBaseline(FunctionIR& function);
    JITFunctionPtr compileOptimized(FunctionIR& function);
    
    void promoteToOptimized(std::string_view name);
    
    void setProfilingEnabled(bool enabled) { profilingEnabled_ = enabled; }
    void setOptimizationThreshold(std::size_t threshold) { optimizationThreshold_ = threshold; }

private:
    TargetInfo target_;
    JITCompiler baselineCompiler_;
    JITCompiler optimizingCompiler_;
    std::unordered_map<std::string, JITFunctionPtr> baselineFunctions_;
    std::unordered_map<std::string, JITFunctionPtr> optimizedFunctions_;
    bool profilingEnabled_;
    std::size_t optimizationThreshold_;
    
    struct FunctionProfile {
        std::size_t executionCount;
        std::size_t totalTime;
        std::vector<std::size_t> hotPaths;
    };
    
    std::unordered_map<std::string, FunctionProfile> profiles_;
    
    void recordExecution(std::string_view name, std::size_t time);
    bool shouldOptimize(std::string_view name) const;
    
    static constexpr std::size_t DEFAULT_OPT_THRESHOLD = 10000;
};

class JITRuntime {
public:
    explicit JITRuntime(TargetInfo target);
    
    void initialize();
    void shutdown();
    
    JITFunctionPtr resolveSymbol(std::string_view name);
    void* allocateRuntimeMemory(std::size_t size, std::size_t alignment);
    
    void registerExternalFunction(std::string_view name, void* address);
    void registerRuntimeFunction(std::string_view name, JITFunctionPtr address);
    
    const JITMemoryManager& memoryManager() const { return memoryManager_; }

private:
    TargetInfo target_;
    JITMemoryManager memoryManager_;
    std::unordered_map<std::string, void*> externalSymbols_;
    std::unordered_map<std::string, JITFunctionPtr> runtimeSymbols_;
    
    void setupRuntimeFunctions();
    void* getSymbolAddress(std::string_view name);
    
    static void* resolveExternalSymbol(std::string_view name);
};

class JITProfiler {
public:
    struct ProfileData {
        std::string functionName;
        std::size_t compileTime;
        std::size_t executionCount;
        std::size_t totalTime;
        std::size_t cacheHits;
        std::size_t cacheMisses;
    };
    
    JITProfiler();
    
    void recordCompilation(std::string_view name, std::size_t time);
    void recordExecution(std::string_view name, std::size_t time);
    void recordCacheHit(std::string_view name);
    void recordCacheMiss(std::string_view name);
    
    std::vector<ProfileData> getProfileData() const;
    void reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ProfileData> profiles_;
    
    ProfileData& getOrCreateProfile(std::string_view name);
};

template<typename Ret, typename... Args>
Ret JITExecutionEngine::executeFunction(std::string_view name, Args... args) {
    using FunctionType = Ret(*)(Args...);
    
    void* funcPtr = getFunctionPointer(name);
    if (!funcPtr) {
        throw std::runtime_error("Function not found: " + std::string(name));
    }
    
    auto function = reinterpret_cast<FunctionType>(funcPtr);
    return function(std::forward<Args>(args)...);
}

class JITModuleBuilder {
public:
    explicit JITModuleBuilder(JITCompiler& compiler);
    
    void addFunction(std::string_view name, std::unique_ptr<FunctionIR> function);
    void addGlobal(std::string_view name, Type type, void* initialValue = nullptr);
    
    std::unique_ptr<JITModule> build();
    
    void setModuleName(std::string_view name) { moduleName_ = name; }

private:
    JITCompiler& compiler_;
    std::string moduleName_;
    std::unordered_map<std::string, std::unique_ptr<FunctionIR>> functions_;
    std::unordered_map<std::string, std::pair<Type, void*>> globals_;
};

} // namespace codegen
} // namespace internal
} // namespace klang