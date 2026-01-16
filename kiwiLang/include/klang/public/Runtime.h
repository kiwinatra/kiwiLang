#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace klang {

// Forward declarations
class VirtualMachine;
class GarbageCollector;
class MemoryManager;
class Object;
class String;
class Array;
class HashTable;

// Runtime configuration
struct RuntimeConfig {
    size_t initial_heap_size = 16 * 1024 * 1024;  // 16 MB
    size_t max_heap_size = 1024 * 1024 * 1024;    // 1 GB
    size_t stack_size = 64 * 1024;                // 64 KB
    bool enable_gc = true;
    size_t gc_threshold = 8 * 1024 * 1024;        // 8 MB
    bool enable_jit = true;
    bool enable_profiling = false;
    bool enable_debugging = false;
    bool enable_assertions = false;
    bool enable_logging = false;
    
    // JIT configuration
    size_t jit_code_cache_size = 4 * 1024 * 1024; // 4 MB
    size_t jit_data_cache_size = 1 * 1024 * 1024; // 1 MB
    bool jit_optimize = true;
    size_t jit_opt_level = 2;
    
    // Garbage collector configuration
    enum class GCType {
        MarkSweep,
        Generational,
        Incremental,
        Concurrent
    };
    
    GCType gc_type = GCType::MarkSweep;
    size_t gc_minor_heap_size = 2 * 1024 * 1024;  // 2 MB
    size_t gc_major_heap_size = 32 * 1024 * 1024; // 32 MB
    size_t gc_idle_timeout = 1000;                // 1 second
    bool gc_compact = true;
    bool gc_verify = false;
    
    // Memory allocator configuration
    enum class AllocatorType {
        System,
        Pool,
        Segregated,
        Buddy
    };
    
    AllocatorType allocator_type = AllocatorType::Segregated;
    size_t pool_size = 64 * 1024;                 // 64 KB
    size_t small_object_size = 256;               // 256 bytes
    size_t medium_object_size = 4096;             // 4 KB
    bool use_guard_pages = true;
    bool poison_memory = true;
    bool track_allocations = false;
    
    // Execution limits
    size_t max_execution_time = 0;                // 0 = unlimited
    size_t max_memory_usage = 0;                  // 0 = unlimited
    size_t max_instructions = 0;                  // 0 = unlimited
    size_t max_recursion_depth = 1000;
    size_t max_call_stack_size = 10000;
    
    // Security settings
    bool enable_sandbox = false;
    bool restrict_syscalls = false;
    bool enable_capabilities = false;
    bool enable_memory_protection = true;
    bool enable_control_flow_integrity = true;
    
    // Debugging and profiling
    bool record_statistics = true;
    bool profile_memory = false;
    bool profile_execution = false;
    bool trace_gc = false;
    bool trace_allocations = false;
    bool trace_deallocations = false;
    
    // Error handling
    bool catch_exceptions = true;
    bool abort_on_error = false;
    bool print_errors = true;
    bool log_errors = false;
    
    // Threading
    size_t max_threads = 1;
    bool enable_thread_support = false;
    bool thread_safe_gc = false;
    bool use_thread_local_allocators = false;
    
    // Serialization
    bool enable_serialization = false;
    size_t serialization_buffer_size = 1 * 1024 * 1024; // 1 MB
};

// Runtime statistics
struct RuntimeStats {
    // Memory statistics
    size_t total_allocated = 0;
    size_t currently_allocated = 0;
    size_t peak_allocated = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
    
    // GC statistics
    size_t gc_collections = 0;
    size_t gc_minor_collections = 0;
    size_t gc_major_collections = 0;
    size_t gc_reclaimed_bytes = 0;
    size_t gc_pause_time_ns = 0;
    size_t gc_total_time_ns = 0;
    
    // Execution statistics
    size_t instructions_executed = 0;
    size_t function_calls = 0;
    size_t exceptions_thrown = 0;
    size_t exceptions_caught = 0;
    
    // JIT statistics
    size_t jit_functions_compiled = 0;
    size_t jit_bytes_generated = 0;
    size_t jit_compilation_time_ns = 0;
    
    // Cache statistics
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t cache_evictions = 0;
    
    // Performance counters
    double execution_time_sec = 0.0;
    double gc_overhead_percent = 0.0;
    double jit_overhead_percent = 0.0;
    double cache_hit_rate = 0.0;
    
    // Object type statistics
    struct ObjectTypeStats {
        size_t count = 0;
        size_t total_size = 0;
        size_t peak_count = 0;
    };
    
    ObjectTypeStats integer_objects;
    ObjectTypeStats float_objects;
    ObjectTypeStats boolean_objects;
    ObjectTypeStats string_objects;
    ObjectTypeStats array_objects;
    ObjectTypeStats hash_table_objects;
    ObjectTypeStats function_objects;
    ObjectTypeStats closure_objects;
    ObjectTypeStats class_objects;
    ObjectTypeStats instance_objects;
};

// Runtime events
enum class RuntimeEvent {
    VMStart,
    VMStop,
    GCStart,
    GCStop,
    GCCollect,
    GCMark,
    GCSweep,
    GCCompact,
    JITCompileStart,
    JITCompileStop,
    JITInvalidate,
    FunctionEnter,
    FunctionExit,
    ExceptionThrow,
    ExceptionCatch,
    Allocation,
    Deallocation,
    StackOverflow,
    StackUnderflow,
    MemoryLimitExceeded,
    TimeLimitExceeded,
    SecurityViolation
};

// Event callback type
using RuntimeEventCallback = void(*)(RuntimeEvent event, void* data, size_t size, void* user_data);

// Runtime context - main interface to KLang runtime
class Runtime {
public:
    Runtime();
    explicit Runtime(const RuntimeConfig& config);
    ~Runtime();
    
    // Disable copying
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    
    // Move operations
    Runtime(Runtime&& other) noexcept;
    Runtime& operator=(Runtime&& other) noexcept;
    
    // Initialization and cleanup
    bool initialize();
    bool shutdown();
    bool is_initialized() const;
    
    // Configuration
    const RuntimeConfig& config() const;
    RuntimeConfig& mutable_config();
    bool reconfigure(const RuntimeConfig& new_config);
    
    // Statistics
    const RuntimeStats& stats() const;
    void reset_stats();
    void print_stats() const;
    
    // Event handling
    void set_event_callback(RuntimeEventCallback callback, void* user_data = nullptr);
    void clear_event_callback();
    
    // Memory management
    void* allocate(size_t size);
    void* allocate_aligned(size_t size, size_t alignment);
    void deallocate(void* ptr);
    size_t allocated_size(void* ptr) const;
    
    // Object management
    Object* create_integer(int64_t value);
    Object* create_float(double value);
    Object* create_boolean(bool value);
    Object* create_string(const char* value, size_t length = 0);
    Object* create_string(const std::string& value);
    Object* create_array(size_t capacity = 0);
    Object* create_hash_table(size_t capacity = 0);
    Object* create_function(void* code_ptr, size_t arity);
    Object* create_closure(Object* function, size_t upvalue_count);
    Object* create_class(const char* name, size_t field_count);
    Object* create_instance(Object* class_obj);
    
    // Garbage collection
    void collect_garbage();
    void collect_garbage_minor();
    void collect_garbage_major();
    void force_gc();
    size_t gc_threshold() const;
    void set_gc_threshold(size_t threshold);
    bool gc_running() const;
    void suspend_gc();
    void resume_gc();
    
    // JIT compilation
    void* compile_function(const void* bytecode, size_t size, const char* name = nullptr);
    bool invalidate_function(void* code_ptr);
    void flush_jit_cache();
    size_t jit_cache_size() const;
    size_t jit_cache_usage() const;
    
    // Execution control
    bool load_module(const void* bytecode, size_t size);
    bool load_module_from_file(const char* filename);
    int execute_module();
    int execute_function(const char* name, const std::vector<Object*>& args);
    void pause_execution();
    void resume_execution();
    void stop_execution();
    bool is_running() const;
    
    // Debugging
    void set_breakpoint(const char* function_name, size_t line);
    void clear_breakpoint(const char* function_name, size_t line);
    void clear_all_breakpoints();
    void step_execution();
    void continue_execution();
    void inspect_object(Object* obj);
    void inspect_stack();
    void inspect_heap();
    
    // Profiling
    void start_profiling();
    void stop_profiling();
    void dump_profile(const char* filename = nullptr);
    bool is_profiling() const;
    
    // Serialization
    bool serialize_state(const char* filename);
    bool deserialize_state(const char* filename);
    size_t serialize_to_buffer(void* buffer, size_t size);
    bool deserialize_from_buffer(const void* buffer, size_t size);
    
    // Security
    void enable_sandbox(bool enable);
    bool sandbox_enabled() const;
    void restrict_syscall(int syscall_number, bool allow);
    void set_capability(const char* capability, bool granted);
    bool check_capability(const char* capability) const;
    
    // Threading
    bool create_thread(void(*entry)(void*), void* arg);
    bool join_threads();
    size_t thread_count() const;
    
    // Error handling
    const char* last_error() const;
    void clear_error();
    void set_error_handler(void(*handler)(const char* error, void* user_data), void* user_data = nullptr);
    
    // Utility functions
    static Runtime* current();
    static void set_current(Runtime* runtime);
    static size_t page_size();
    static size_t system_memory();
    static size_t available_memory();
    
    // Internal access (for advanced use)
    VirtualMachine* vm();
    GarbageCollector* gc();
    MemoryManager* memory();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Helper functions for common operations
namespace runtime {
    
    // Create a runtime with default configuration
    inline std::unique_ptr<Runtime> create() {
        return std::make_unique<Runtime>();
    }
    
    // Create a runtime with custom configuration
    inline std::unique_ptr<Runtime> create(const RuntimeConfig& config) {
        return std::make_unique<Runtime>(config);
    }
    
    // Execute a module from bytecode
    inline int execute(const void* bytecode, size_t size, const RuntimeConfig& config = RuntimeConfig()) {
        auto rt = create(config);
        if (!rt->initialize()) {
            return -1;
        }
        if (!rt->load_module(bytecode, size)) {
            return -2;
        }
        return rt->execute_module();
    }
    
    // Execute a module from file
    inline int execute_file(const char* filename, const RuntimeConfig& config = RuntimeConfig()) {
        auto rt = create(config);
        if (!rt->initialize()) {
            return -1;
        }
        if (!rt->load_module_from_file(filename)) {
            return -2;
        }
        return rt->execute_module();
    }
    
    // Get current runtime (thread-local)
    inline Runtime* current() {
        return Runtime::current();
    }
    
    // Allocate memory using current runtime
    inline void* allocate(size_t size) {
        auto rt = current();
        return rt ? rt->allocate(size) : nullptr;
    }
    
    // Deallocate memory using current runtime
    inline void deallocate(void* ptr) {
        auto rt = current();
        if (rt) {
            rt->deallocate(ptr);
        }
    }
    
    // Create object using current runtime
    template<typename T, typename... Args>
    inline Object* create_object(Args&&... args) {
        auto rt = current();
        return rt ? rt->create_object<T>(std::forward<Args>(args)...) : nullptr;
    }
    
} // namespace runtime

// Exception thrown by runtime errors
class RuntimeError : public std::exception {
public:
    explicit RuntimeError(const char* message);
    explicit RuntimeError(const std::string& message);
    ~RuntimeError() override;
    
    const char* what() const noexcept override;
    
private:
    std::string message_;
};

// Helper for automatic runtime initialization/cleanup
class RuntimeGuard {
public:
    explicit RuntimeGuard(Runtime& rt);
    ~RuntimeGuard();
    
    RuntimeGuard(const RuntimeGuard&) = delete;
    RuntimeGuard& operator=(const RuntimeGuard&) = delete;
    
private:
    Runtime& runtime_;
    bool was_initialized_;
};

// Type aliases for common runtime objects
using RuntimePtr = std::unique_ptr<Runtime>;
using RuntimeRef = Runtime&;

} // namespace klang