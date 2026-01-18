#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace kiwiLang {

// Forward declarations
class Runtime;
class Function;

// JIT compilation target architecture
enum class TargetArchitecture {
    X86_64,
    AArch64,
    RISCV64,
    WebAssembly,
    Unknown
};

// JIT optimization level
enum class OptimizationLevel {
    O0,  // No optimization
    O1,  // Basic optimization
    O2,  // Default optimization
    O3,  // Aggressive optimization
    Os,  // Optimize for size
    Oz   // Maximum size optimization
};

// JIT compilation flags
enum class CompileFlags : uint32_t {
    None = 0,
    DebugInfo = 1 << 0,
    ProfileGuided = 1 << 1,
    LTO = 1 << 2,
    PGO = 1 << 3,
    ThinLTO = 1 << 4,
    StackProtector = 1 << 5,
    CFI = 1 << 6,      // Control Flow Integrity
    ShadowCallStack = 1 << 7,
    SafeStack = 1 << 8,
    ASLR = 1 << 9,     // Address Space Layout Randomization
    PIC = 1 << 10,     // Position Independent Code
    PIE = 1 << 11,     // Position Independent Executable
    JITLink = 1 << 12,
    InlineAll = 1 << 13,
    NoInline = 1 << 14,
    TailCall = 1 << 15,
    FastMath = 1 << 16,
    StrictAliasing = 1 << 17,
    NoUnwind = 1 << 18,
    NoExcept = 1 << 19,
    Cold = 1 << 20,
    Hot = 1 << 21,
    OptForSize = 1 << 22,
    OptForSpeed = 1 << 23,
    OptForDebugging = 1 << 24,
    OptForFuzzing = 1 << 25
};

inline CompileFlags operator|(CompileFlags a, CompileFlags b) {
    return static_cast<CompileFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CompileFlags operator&(CompileFlags a, CompileFlags b) {
    return static_cast<CompileFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// JIT compilation result
struct CompileResult {
    void* code_ptr = nullptr;
    size_t code_size = 0;
    size_t data_size = 0;
    size_t relocation_size = 0;
    size_t symbol_size = 0;
    size_t debug_info_size = 0;
    uint64_t compile_time_ns = 0;
    uint64_t optimize_time_ns = 0;
    uint64_t link_time_ns = 0;
    uint32_t optimization_passes = 0;
    uint32_t functions_compiled = 0;
    uint32_t basic_blocks = 0;
    uint32_t instructions = 0;
    uint32_t registers_allocated = 0;
    uint32_t stack_slots = 0;
    bool success = false;
    std::string error_message;
    
    // Cache information
    uint32_t cache_hits = 0;
    uint32_t cache_misses = 0;
    uint32_t cache_evictions = 0;
    
    // Performance counters
    double instructions_per_cycle = 0.0;
    double branch_mispredict_rate = 0.0;
    double cache_miss_rate = 0.0;
    double tlb_miss_rate = 0.0;
    
    // Optimization metrics
    uint32_t inlined_functions = 0;
    uint32_t tail_calls = 0;
    uint32_t loop_optimizations = 0;
    uint32_t vectorized_loops = 0;
    uint32_t constant_folded = 0;
    uint32_t dead_code_eliminated = 0;
    uint32_t common_subexprs = 0;
};

// JIT function signature
using JITFunction = void(*)();
using JITFunctionWithArgs = void(*)(void* args, size_t arg_count);
using JITFunctionReturn = void*(*)();
using JITFunctionGeneric = void*(*)(void* args, size_t arg_count);

// JIT symbol information
struct SymbolInfo {
    std::string name;
    void* address = nullptr;
    size_t size = 0;
    bool is_function = false;
    bool is_global = false;
    bool is_weak = false;
    bool is_external = false;
    uint32_t section_index = 0;
    uint64_t file_offset = 0;
    
    // Debug information
    std::string source_file;
    uint32_t line_number = 0;
    uint32_t column_number = 0;
    
    // Type information
    std::string type_name;
    size_t type_size = 0;
    size_t type_alignment = 0;
};

// JIT relocation information
struct RelocationInfo {
    void* location = nullptr;
    SymbolInfo* symbol = nullptr;
    int64_t addend = 0;
    uint32_t type = 0;
    uint32_t size = 0;
    bool is_pc_relative = false;
    bool is_absolute = false;
    bool is_relative = false;
};

// JIT section information
struct SectionInfo {
    std::string name;
    void* address = nullptr;
    size_t size = 0;
    uint32_t alignment = 0;
    bool is_executable = false;
    bool is_writable = false;
    bool is_readable = true;
    bool is_allocated = true;
    bool is_initialized = true;
    bool is_bss = false;
    bool is_rodata = false;
    bool is_text = false;
    bool is_data = false;
    
    // Memory protection
    uint32_t protection_flags = 0;
};

// JIT cache entry
struct CacheEntry {
    std::string key;
    void* code_ptr = nullptr;
    size_t code_size = 0;
    size_t data_size = 0;
    uint64_t compile_time = 0;
    uint64_t last_access = 0;
    uint64_t access_count = 0;
    uint32_t hit_count = 0;
    uint32_t generation = 0;
    bool is_dirty = false;
    bool is_pinned = false;
    bool is_shared = false;
    
    // Statistics
    double average_execution_time = 0.0;
    uint64_t total_execution_time = 0;
    uint32_t execution_count = 0;
};

// JIT profiling data
struct ProfileData {
    struct FunctionProfile {
        std::string name;
        void* address = nullptr;
        uint64_t total_time_ns = 0;
        uint64_t self_time_ns = 0;
        uint64_t call_count = 0;
        uint64_t instruction_count = 0;
        uint64_t cache_misses = 0;
        uint64_t branch_mispredicts = 0;
        double time_percent = 0.0;
        double cache_miss_rate = 0.0;
        double branch_mispredict_rate = 0.0;
        
        // Call graph
        std::unordered_map<std::string, uint64_t> callers;
        std::unordered_map<std::string, uint64_t> callees;
    };
    
    std::vector<FunctionProfile> functions;
    uint64_t total_time_ns = 0;
    uint64_t total_instructions = 0;
    uint64_t total_cache_misses = 0;
    uint64_t total_branch_mispredicts = 0;
    uint32_t sample_count = 0;
    uint64_t sampling_interval_ns = 0;
    bool is_complete = false;
};

// JIT compilation configuration
struct JITConfig {
    OptimizationLevel opt_level = OptimizationLevel::O2;
    TargetArchitecture target_arch = TargetArchitecture::X86_64;
    CompileFlags flags = CompileFlags::None;
    
    // Code generation
    bool enable_avx = true;
    bool enable_avx2 = true;
    bool enable_avx512 = false;
    bool enable_sse4 = true;
    bool enable_fma = true;
    bool enable_bmi = true;
    bool enable_lzcnt = true;
    bool enable_popcnt = true;
    
    // Optimization settings
    uint32_t inline_threshold = 100;
    uint32_t max_inline_size = 50;
    bool enable_tail_call = true;
    bool enable_loop_vectorization = true;
    bool enable_slp_vectorization = true;
    bool enable_interprocedural_opt = true;
    bool enable_global_opt = true;
    bool enable_constant_propagation = true;
    bool enable_dead_code_elimination = true;
    bool enable_common_subexpr_elimination = true;
    bool enable_instruction_scheduling = true;
    bool enable_register_allocation = true;
    bool enable_stack_slot_coloring = true;
    
    // Cache settings
    size_t code_cache_size = 4 * 1024 * 1024;     // 4 MB
    size_t data_cache_size = 1 * 1024 * 1024;     // 1 MB
    size_t max_cache_entries = 1024;
    uint32_t cache_associativity = 8;
    bool enable_cache_prefetch = true;
    bool enable_cache_partitioning = false;
    
    // Profiling
    bool enable_profiling = false;
    uint64_t profiling_interval_ns = 1000000;     // 1 ms
    size_t profiling_buffer_size = 64 * 1024;     // 64 KB
    bool profile_cache_behavior = true;
    bool profile_branch_prediction = true;
    bool profile_instruction_mix = true;
    
    // Security
    bool enable_cfi = true;
    bool enable_shadow_call_stack = false;
    bool enable_safe_stack = true;
    bool enable_pointer_authentication = false;
    bool enable_memory_tagging = false;
    bool enable_control_flow_guard = true;
    
    // Debugging
    bool generate_debug_info = false;
    bool generate_symbol_table = true;
    bool generate_line_table = false;
    bool preserve_names = true;
    bool emit_assembly = false;
    bool emit_llvm_ir = false;
    bool emit_object_file = false;
    
    // Linking
    bool lazy_linking = true;
    bool incremental_linking = false;
    bool dead_strip = true;
    bool strip_debug = false;
    bool strip_symbols = false;
    
    // Memory management
    size_t page_size = 4096;
    size_t allocation_granularity = 65536;        // 64 KB
    bool use_guard_pages = true;
    bool use_memory_protection = true;
    bool track_allocations = false;
    
    // Threading
    bool thread_safe = true;
    uint32_t max_threads = 4;
    bool use_thread_local_cache = false;
    bool use_atomic_operations = true;
};

// JIT compiler interface
class JITCompiler {
public:
    virtual ~JITCompiler() = default;
    
    // Compilation
    virtual CompileResult compile(
        const void* ir_data,
        size_t ir_size,
        const std::string& name,
        const JITConfig& config
    ) = 0;
    
    virtual CompileResult compile_function(
        Function* function,
        const JITConfig& config
    ) = 0;
    
    virtual CompileResult compile_module(
        const std::vector<Function*>& functions,
        const JITConfig& config
    ) = 0;
    
    // Code management
    virtual bool add_code(void* code_ptr, size_t size, const SectionInfo& section) = 0;
    virtual bool remove_code(void* code_ptr) = 0;
    virtual bool relocate_code(void* code_ptr, const std::vector<RelocationInfo>& relocs) = 0;
    virtual bool patch_code(void* location, const void* new_code, size_t size) = 0;
    
    // Symbol management
    virtual SymbolInfo* find_symbol(const std::string& name) = 0;
    virtual SymbolInfo* find_symbol_by_address(void* address) = 0;
    virtual bool add_symbol(const SymbolInfo& symbol) = 0;
    virtual bool remove_symbol(const std::string& name) = 0;
    virtual std::vector<SymbolInfo> list_symbols() const = 0;
    
    // Cache management
    virtual CacheEntry* get_cached_code(const std::string& key) = 0;
    virtual bool cache_code(const std::string& key, const CompileResult& result) = 0;
    virtual bool invalidate_cache(const std::string& key) = 0;
    virtual void clear_cache() = 0;
    virtual size_t cache_size() const = 0;
    virtual size_t cache_usage() const = 0;
    
    // Profiling
    virtual ProfileData collect_profile_data() = 0;
    virtual void start_profiling() = 0;
    virtual void stop_profiling() = 0;
    virtual bool is_profiling() const = 0;
    
    // Configuration
    virtual const JITConfig& config() const = 0;
    virtual void reconfigure(const JITConfig& new_config) = 0;
    
    // Statistics
    struct Stats {
        uint64_t compilations = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        uint64_t bytes_generated = 0;
        uint64_t total_compile_time_ns = 0;
        uint64_t total_execution_time_ns = 0;
        uint32_t functions_compiled = 0;
        uint32_t functions_executed = 0;
        double average_compile_time_ms = 0.0;
        double average_execution_time_us = 0.0;
        double cache_hit_rate = 0.0;
    };
    
    virtual Stats stats() const = 0;
    virtual void reset_stats() = 0;
    
    // Utility functions
    virtual TargetArchitecture detect_architecture() const = 0;
    virtual std::string get_target_triple() const = 0;
    virtual size_t get_page_size() const = 0;
    virtual size_t get_cache_line_size() const = 0;
    
    // Factory function
    static std::unique_ptr<JITCompiler> create(Runtime* runtime = nullptr);
};

// JIT execution engine
class JITEngine {
public:
    explicit JITEngine(Runtime* runtime = nullptr);
    ~JITEngine();
    
    // Disable copying
    JITEngine(const JITEngine&) = delete;
    JITEngine& operator=(const JITEngine&) = delete;
    
    // Move operations
    JITEngine(JITEngine&& other) noexcept;
    JITEngine& operator=(JITEngine&& other) noexcept;
    
    // Initialization
    bool initialize(const JITConfig& config = JITConfig());
    bool is_initialized() const;
    void shutdown();
    
    // Compilation
    CompileResult compile(
        const void* ir_data,
        size_t ir_size,
        const std::string& name = ""
    );
    
    CompileResult compile_function(
        Function* function,
        const std::string& name = ""
    );
    
    // Execution
    void* execute(
        const void* ir_data,
        size_t ir_size,
        const std::string& name = ""
    );
    
    void* execute_function(
        Function* function,
        const std::string& name = ""
    );
    
    template<typename Ret, typename... Args>
    Ret execute_with_args(
        const void* ir_data,
        size_t ir_size,
        Args... args
    );
    
    // Function lookup
    JITFunction find_function(const std::string& name);
    template<typename Ret, typename... Args>
    std::function<Ret(Args...)> get_function(const std::string& name);
    
    // Code management
    bool add_external_symbol(const std::string& name, void* address);
    bool remove_external_symbol(const std::string& name);
    void* get_code_address(const std::string& name) const;
    
    // Cache management
    void enable_caching(bool enable);
    bool is_caching_enabled() const;
    void set_cache_size(size_t code_size, size_t data_size);
    void clear_cache();
    
    // Profiling
    void enable_profiling(bool enable);
    bool is_profiling_enabled() const;
    ProfileData get_profile_data();
    
    // Configuration
    const JITConfig& config() const;
    void set_config(const JITConfig& config);
    
    // Accessors
    JITCompiler* compiler();
    Runtime* runtime();
    
    // Static utilities
    static bool is_available();
    static std::string get_version();
    static std::vector<TargetArchitecture> get_supported_architectures();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Inline implementations
template<typename Ret, typename... Args>
Ret JITEngine::execute_with_args(
    const void* ir_data,
    size_t ir_size,
    Args... args
) {
    using FunctionType = Ret(*)(Args...);
    
    auto result = compile(ir_data, ir_size, "jit_function");
    if (!result.success || !result.code_ptr) {
        throw std::runtime_error("JIT compilation failed: " + result.error_message);
    }
    
    auto func = reinterpret_cast<FunctionType>(result.code_ptr);
    return func(std::forward<Args>(args)...);
}

template<typename Ret, typename... Args>
std::function<Ret(Args...)> JITEngine::get_function(const std::string& name) {
    using FunctionType = Ret(*)(Args...);
    
    void* addr = get_code_address(name);
    if (!addr) {
        return nullptr;
    }
    
    auto func = reinterpret_cast<FunctionType>(addr);
    return [func](Args... args) -> Ret {
        return func(std::forward<Args>(args)...);
    };
}

// JIT compilation helper
class JITHelper {
public:
    static CompileResult compile_to_native(
        const void* ir_data,
        size_t ir_size,
        const JITConfig& config = JITConfig()
    );
    
    static void* compile_and_execute(
        const void* ir_data,
        size_t ir_size,
        void* args = nullptr,
        size_t arg_count = 0
    );
    
    static bool optimize_code(
        void* code_ptr,
        size_t code_size,
        OptimizationLevel level
    );
    
    static bool verify_code(
        void* code_ptr,
        size_t code_size,
        bool check_cfi = true
    );
    
    static size_t get_code_size(void* code_ptr);
    static bool is_code_executable(void* code_ptr);
    static bool make_code_executable(void* code_ptr, size_t size);
    static bool make_code_writable(void* code_ptr, size_t size);
    
    static std::string disassemble(
        void* code_ptr,
        size_t code_size,
        TargetArchitecture arch = TargetArchitecture::X86_64
    );
    
    static ProfileData profile_execution(
        void* code_ptr,
        size_t iterations = 1000
    );
    
    static double benchmark_execution(
        void* code_ptr,
        size_t iterations = 10000,
        size_t warmup = 1000
    );
};

// JIT compilation exceptions
class JITCompilationError : public std::runtime_error {
public:
    explicit JITCompilationError(const std::string& message, const CompileResult& result);
    
    const CompileResult& result() const;
    
private:
    CompileResult result_;
};

class JITExecutionError : public std::runtime_error {
public:
    explicit JITExecutionError(const std::string& message, void* fault_address = nullptr);
    
    void* fault_address() const;
    
private:
    void* fault_address_;
};

// RAII wrapper for JIT code
class JITCode {
public:
    JITCode();
    JITCode(void* code_ptr, size_t size);
    ~JITCode();
    
    JITCode(const JITCode&) = delete;
    JITCode& operator=(const JITCode&) = delete;
    
    JITCode(JITCode&& other) noexcept;
    JITCode& operator=(JITCode&& other) noexcept;
    
    void* get() const;
    size_t size() const;
    bool empty() const;
    
    template<typename T>
    T as() const {
        return reinterpret_cast<T>(code_ptr_);
    }
    
    operator bool() const {
        return code_ptr_ != nullptr;
    }
    
private:
    void* code_ptr_ = nullptr;
    size_t size_ = 0;
};

} // namespace kiwiLang