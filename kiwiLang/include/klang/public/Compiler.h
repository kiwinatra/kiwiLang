#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <system_error>

namespace kiwiLang {

// Forward declarations
class Module;
class Function;
class Type;
class Diagnostics;
class CompilerInstance;
class CodeGenerator;
class Optimizer;

// Compilation target
enum class Target {
    Native,
    WebAssembly,
    LLVM_IR,
    Bytecode,
    Assembly,
    ObjectFile,
    SharedLibrary,
    StaticLibrary,
    Executable
};

// Optimization level
enum class OptimizationLevel {
    O0,  // No optimization
    O1,  // Basic optimization
    O2,  // Default optimization
    O3,  // Aggressive optimization
    Os,  // Optimize for size
    Oz   // Maximum size optimization
};

// Compilation flags
enum class CompileFlags : uint64_t {
    None = 0,
    EmitLLVM = 1ULL << 0,
    EmitAssembly = 1ULL << 1,
    EmitObject = 1ULL << 2,
    EmitBytecode = 1ULL << 3,
    EmitIR = 1ULL << 4,
    EmitAST = 1ULL << 5,
    EmitTokens = 1ULL << 6,
    EmitSymbols = 1ULL << 7,
    EmitDebugInfo = 1ULL << 8,
    EmitLineTables = 1ULL << 9,
    EmitSourceMap = 1ULL << 10,
    EmitProfile = 1ULL << 11,
    EmitCoverage = 1ULL << 12,
    EmitAnnotations = 1ULL << 13,
    EmitMetadata = 1ULL << 14,
    EmitComments = 1ULL << 15,
    
    // Optimization flags
    OptimizeForSize = 1ULL << 16,
    OptimizeForSpeed = 1ULL << 17,
    OptimizeForDebugging = 1ULL << 18,
    OptimizeForFuzzing = 1ULL << 19,
    OptimizeAggressive = 1ULL << 20,
    OptimizeConservative = 1ULL << 21,
    
    // Language feature flags
    EnableExtensions = 1ULL << 22,
    EnableExperimental = 1ULL << 23,
    EnableDeprecated = 1ULL << 24,
    EnableUnsafe = 1ULL << 25,
    EnablePedantic = 1ULL << 26,
    EnableStrict = 1ULL << 27,
    EnableWarnings = 1ULL << 28,
    EnableErrors = 1ULL << 29,
    EnableFatalErrors = 1ULL << 30,
    EnableAllWarnings = 1ULL << 31,
    
    // Security flags
    EnableSecurity = 1ULL << 32,
    EnableSandbox = 1ULL << 33,
    EnableMemorySafety = 1ULL << 34,
    EnableTypeSafety = 1ULL << 35,
    EnableBoundsChecking = 1ULL << 36,
    EnableNullChecking = 1ULL << 37,
    EnableOverflowChecking = 1ULL << 38,
    EnableFormatChecking = 1ULL << 39,
    
    // Debugging flags
    EnableDebugging = 1ULL << 40,
    EnableProfiling = 1ULL << 41,
    EnableTracing = 1ULL << 42,
    EnableInstrumentation = 1ULL << 43,
    EnableSanitizers = 1ULL << 44,
    EnableCoverage = 1ULL << 45,
    EnableAssertions = 1ULL << 46,
    EnableLogging = 1ULL << 47,
    
    // Code generation flags
    EnableJIT = 1ULL << 48,
    EnableLTO = 1ULL << 49,
    EnablePGO = 1ULL << 50,
    EnableCFI = 1ULL << 51,
    EnablePIC = 1ULL << 52,
    EnablePIE = 1ULL << 53,
    EnableStackProtector = 1ULL << 54,
    EnableShadowCallStack = 1ULL << 55,
    EnableSafeStack = 1ULL << 56,
    
    // Compilation mode flags
    CompileOnly = 1ULL << 57,
    AssembleOnly = 1ULL << 58,
    LinkOnly = 1ULL << 59,
    PreprocessOnly = 1ULL << 60,
    SyntaxOnly = 1ULL << 61,
    SemanticOnly = 1ULL << 62,
    CodegenOnly = 1ULL << 63
};

inline CompileFlags operator|(CompileFlags a, CompileFlags b) {
    return static_cast<CompileFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline CompileFlags operator&(CompileFlags a, CompileFlags b) {
    return static_cast<CompileFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline CompileFlags operator~(CompileFlags a) {
    return static_cast<CompileFlags>(~static_cast<uint64_t>(a));
}

// Compilation result
struct CompileResult {
    bool success = false;
    std::error_code error_code;
    std::string error_message;
    std::string warning_message;
    std::string info_message;
    
    // Output artifacts
    std::vector<uint8_t> object_code;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> assembly;
    std::vector<uint8_t> ir_code;
    std::vector<uint8_t> ast_dump;
    std::vector<uint8_t> symbol_table;
    std::vector<uint8_t> debug_info;
    
    // Statistics
    size_t compile_time_ms = 0;
    size_t lex_time_ms = 0;
    size_t parse_time_ms = 0;
    size_t semantic_time_ms = 0;
    size_t codegen_time_ms = 0;
    size_t optimize_time_ms = 0;
    size_t link_time_ms = 0;
    
    size_t source_lines = 0;
    size_t tokens_generated = 0;
    size_t ast_nodes = 0;
    size_t ir_instructions = 0;
    size_t machine_instructions = 0;
    size_t data_bytes = 0;
    size_t code_bytes = 0;
    size_t total_bytes = 0;
    
    // Optimization metrics
    size_t optimizations_applied = 0;
    size_t functions_inlined = 0;
    size_t dead_code_eliminated = 0;
    size_t constants_folded = 0;
    size_t common_subexprs = 0;
    size_t loops_optimized = 0;
    size_t registers_allocated = 0;
    
    // Diagnostics
    size_t errors = 0;
    size_t warnings = 0;
    size_t notes = 0;
    size_t fixes = 0;
    
    // Performance metrics
    double instructions_per_cycle = 0.0;
    double cache_hit_rate = 0.0;
    double branch_prediction_rate = 0.0;
    double memory_bandwidth_mbps = 0.0;
    
    // Resource usage
    size_t peak_memory_mb = 0;
    size_t total_memory_mb = 0;
    size_t cpu_time_ms = 0;
    
    // Validation
    bool validated = false;
    std::string validation_error;
};

// Compilation job
struct CompileJob {
    std::string source_file;
    std::vector<std::string> include_dirs;
    std::vector<std::string> library_dirs;
    std::vector<std::string> libraries;
    std::vector<std::string> defines;
    std::vector<std::string> undefines;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<std::string> features;
    std::vector<std::string> extensions;
    
    Target target = Target::Native;
    OptimizationLevel opt_level = OptimizationLevel::O2;
    CompileFlags flags = CompileFlags::None;
    
    std::string output_file;
    std::string output_format;
    std::string target_triple;
    std::string cpu;
    std::string features_str;
    std::string abi;
    std::string relocation_model;
    std::string code_model;
    
    size_t thread_count = 0;
    size_t memory_limit_mb = 0;
    size_t time_limit_ms = 0;
    
    bool verbose = false;
    bool quiet = false;
    bool color_output = true;
    bool stats = false;
    bool timings = false;
    
    // Callbacks
    std::function<void(const std::string&)> progress_callback;
    std::function<void(const std::string&, size_t, size_t, const std::string&)> diagnostic_callback;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> output_callback;
};

// Compiler instance - main compiler interface
class Compiler {
public:
    Compiler();
    ~Compiler();
    
    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    
    // Compilation methods
    CompileResult compile(const CompileJob& job);
    CompileResult compile_string(const std::string& source, const CompileJob& job);
    CompileResult compile_files(const std::vector<std::string>& files, const CompileJob& job);
    
    // Module-based compilation
    CompileResult compile_module(Module* module, const CompileJob& job);
    CompileResult compile_modules(const std::vector<Module*>& modules, const CompileJob& job);
    
    // Incremental compilation
    CompileResult compile_incremental(const std::string& changed_file, const CompileJob& job);
    bool supports_incremental() const;
    
    // Parallel compilation
    CompileResult compile_parallel(const std::vector<std::string>& files, const CompileJob& job);
    size_t max_parallel_jobs() const;
    
    // Distributed compilation
    CompileResult compile_distributed(const std::vector<std::string>& files, const CompileJob& job);
    bool supports_distributed() const;
    
    // JIT compilation
    void* compile_jit(const std::string& source, const CompileJob& job);
    void* compile_jit_module(Module* module, const CompileJob& job);
    bool invalidate_jit(void* code_ptr);
    
    // Optimization
    CompileResult optimize(const std::vector<uint8_t>& code, const CompileJob& job);
    CompileResult optimize_module(Module* module, const CompileJob& job);
    
    // Linking
    CompileResult link(const std::vector<std::vector<uint8_t>>& objects, const CompileJob& job);
    CompileResult link_files(const std::vector<std::string>& object_files, const CompileJob& job);
    
    // Code generation
    CompileResult generate_code(Module* module, const CompileJob& job);
    CompileResult generate_ir(Module* module, const CompileJob& job);
    CompileResult generate_assembly(Module* module, const CompileJob& job);
    CompileResult generate_object(Module* module, const CompileJob& job);
    CompileResult generate_bytecode(Module* module, const CompileJob& job);
    
    // Analysis
    CompileResult analyze(const std::string& source, const CompileJob& job);
    CompileResult analyze_module(Module* module, const CompileJob& job);
    CompileResult analyze_dependencies(const std::string& file, const CompileJob& job);
    
    // Transformation
    CompileResult transform(const std::string& source, const CompileJob& job);
    CompileResult transform_module(Module* module, const CompileJob& job);
    
    // Verification
    bool verify(const std::vector<uint8_t>& code, const CompileJob& job);
    bool verify_module(Module* module, const CompileJob& job);
    
    // Profiling
    CompileResult profile(const std::string& source, const CompileJob& job);
    CompileResult profile_module(Module* module, const CompileJob& job);
    
    // Debugging
    CompileResult debug(const std::string& source, const CompileJob& job);
    CompileResult debug_module(Module* module, const CompileJob& job);
    
    // Configuration
    void set_target(Target target);
    Target target() const;
    
    void set_optimization_level(OptimizationLevel level);
    OptimizationLevel optimization_level() const;
    
    void set_flags(CompileFlags flags);
    CompileFlags flags() const;
    void add_flag(CompileFlags flag);
    void remove_flag(CompileFlags flag);
    bool has_flag(CompileFlags flag) const;
    
    void set_option(const std::string& name, const std::string& value);
    std::string get_option(const std::string& name) const;
    bool has_option(const std::string& name) const;
    
    // Diagnostics
    Diagnostics* diagnostics();
    const Diagnostics* diagnostics() const;
    
    void set_diagnostic_handler(std::function<void(const std::string&, size_t, size_t, const std::string&)> handler);
    
    // Resource management
    void set_memory_limit(size_t megabytes);
    size_t memory_limit() const;
    
    void set_time_limit(size_t milliseconds);
    size_t time_limit() const;
    
    void set_thread_count(size_t count);
    size_t thread_count() const;
    
    // State management
    void reset();
    void clear_cache();
    void flush();
    
    bool is_ready() const;
    bool is_busy() const;
    
    // Version information
    static std::string version_string();
    static uint32_t version_major();
    static uint32_t version_minor();
    static uint32_t version_patch();
    static std::string version_build();
    static std::string version_full();
    
    // Feature detection
    static bool supports_target(Target target);
    static bool supports_optimization(OptimizationLevel level);
    static bool supports_feature(const std::string& feature);
    static std::vector<std::string> supported_features();
    static std::vector<Target> supported_targets();
    
    // Factory methods
    static std::unique_ptr<Compiler> create();
    static std::unique_ptr<Compiler> create_with_target(Target target);
    static std::unique_ptr<Compiler> create_with_options(const CompileJob& job);
    
    // Global compiler state
    static void set_default_options(const CompileJob& job);
    static CompileJob default_options();
    static void register_target(const std::string& name, Target target);
    static void register_optimization(const std::string& name, OptimizationLevel level);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Compiler builder for fluent interface
class CompilerBuilder {
public:
    CompilerBuilder();
    
    CompilerBuilder& target(Target target);
    CompilerBuilder& optimization(OptimizationLevel level);
    CompilerBuilder& flags(CompileFlags flags);
    CompilerBuilder& add_flag(CompileFlags flag);
    CompilerBuilder& remove_flag(CompileFlags flag);
    
    CompilerBuilder& option(const std::string& name, const std::string& value);
    CompilerBuilder& define(const std::string& name, const std::string& value = "");
    CompilerBuilder& undefine(const std::string& name);
    CompilerBuilder& include_dir(const std::string& dir);
    CompilerBuilder& library_dir(const std::string& dir);
    CompilerBuilder& library(const std::string& lib);
    
    CompilerBuilder& output(const std::string& file);
    CompilerBuilder& format(const std::string& fmt);
    CompilerBuilder& triple(const std::string& triple);
    CompilerBuilder& cpu(const std::string& cpu);
    CompilerBuilder& features(const std::string& features);
    
    CompilerBuilder& memory_limit(size_t megabytes);
    CompilerBuilder& time_limit(size_t milliseconds);
    CompilerBuilder& thread_count(size_t count);
    
    CompilerBuilder& verbose(bool verbose = true);
    CompilerBuilder& quiet(bool quiet = true);
    CompilerBuilder& color(bool color = true);
    CompilerBuilder& stats(bool stats = true);
    CompilerBuilder& timings(bool timings = true);
    
    CompilerBuilder& on_progress(std::function<void(const std::string&)> callback);
    CompilerBuilder& on_diagnostic(std::function<void(const std::string&, size_t, size_t, const std::string&)> callback);
    CompilerBuilder& on_output(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);
    
    std::unique_ptr<Compiler> build();
    CompileJob build_job();
    
private:
    CompileJob job_;
};

// Compilation context for managing compilation sessions
class CompilationContext {
public:
    CompilationContext();
    explicit CompilationContext(const CompileJob& job);
    ~CompilationContext();
    
    CompilationContext(const CompilationContext&) = delete;
    CompilationContext& operator=(const CompilationContext&) = delete;
    
    // Compilation
    CompileResult compile(const std::string& source);
    CompileResult compile_file(const std::string& file);
    CompileResult compile_files(const std::vector<std::string>& files);
    
    // Module management
    Module* create_module(const std::string& name);
    Module* get_module(const std::string& name);
    bool has_module(const std::string& name) const;
    void remove_module(const std::string& name);
    
    // Function management
    Function* create_function(const std::string& name, Type* return_type, const std::vector<Type*>& param_types);
    Function* get_function(const std::string& name);
    bool has_function(const std::string& name) const;
    
    // Type management
    Type* get_type(const std::string& name);
    Type* create_type(const std::string& name, size_t size, size_t alignment);
    
    // Code generation
    CodeGenerator* codegen();
    Optimizer* optimizer();
    
    // Diagnostics
    Diagnostics* diagnostics();
    
    // Configuration
    void set_job(const CompileJob& job);
    const CompileJob& job() const;
    
    void set_compiler(Compiler* compiler);
    Compiler* compiler() const;
    
    // State management
    void reset();
    void clear();
    bool is_valid() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Compilation pipeline for advanced control
class CompilationPipeline {
public:
    CompilationPipeline();
    ~CompilationPipeline();
    
    // Pipeline stages
    using Stage = std::function<CompileResult(const CompileJob&, const std::vector<uint8_t>&)>;
    
    void add_stage(const std::string& name, Stage stage);
    void remove_stage(const std::string& name);
    void insert_stage(const std::string& name, Stage stage, size_t position);
    void replace_stage(const std::string& name, Stage stage);
    
    // Pipeline execution
    CompileResult execute(const CompileJob& job, const std::vector<uint8_t>& input);
    CompileResult execute_file(const CompileJob& job, const std::string& file);
    
    // Pipeline configuration
    void set_parallel(bool parallel);
    bool parallel() const;
    
    void set_max_parallel(size_t count);
    size_t max_parallel() const;
    
    void set_cache_results(bool cache);
    bool cache_results() const;
    
    // Pipeline inspection
    std::vector<std::string> stage_names() const;
    size_t stage_count() const;
    
    // Predefined pipelines
    static CompilationPipeline default_pipeline();
    static CompilationPipeline optimization_pipeline();
    static CompilationPipeline jit_pipeline();
    static CompilationPipeline debug_pipeline();
    static CompilationPipeline profile_pipeline();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Compilation errors
class CompilationError : public std::runtime_error {
public:
    CompilationError(const std::string& message, const CompileResult& result);
    
    const CompileResult& result() const;
    std::error_code error_code() const;
    
private:
    CompileResult result_;
};

class SyntaxError : public CompilationError {
public:
    SyntaxError(const std::string& message, size_t line, size_t column, const CompileResult& result);
    
    size_t line() const;
    size_t column() const;
    
private:
    size_t line_;
    size_t column_;
};

class SemanticError : public CompilationError {
public:
    SemanticError(const std::string& message, const std::string& symbol, const CompileResult& result);
    
    const std::string& symbol() const;
    
private:
    std::string symbol_;
};

class CodegenError : public CompilationError {
public:
    CodegenError(const std::string& message, const std::string& function, const CompileResult& result);
    
    const std::string& function() const;
    
private:
    std::string function_;
};

class OptimizationError : public CompilationError {
public:
    OptimizationError(const std::string& message, const std::string& pass, const CompileResult& result);
    
    const std::string& pass() const;
    
private:
    std::string pass_;
};

class LinkingError : public CompilationError {
public:
    LinkingError(const std::string& message, const std::string& symbol, const CompileResult& result);
    
    const std::string& symbol() const;
    
private:
    std::string symbol_;
};

} // namespace kiwiLang