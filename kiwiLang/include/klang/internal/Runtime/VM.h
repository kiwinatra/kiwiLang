#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <system_error>

namespace klang {

class Object;
class MemoryManager;
class GarbageCollector;
class Function;
class Closure;
class Upvalue;
class Module;
class Bytecode;
class CallFrame;

enum class VMOpcode : uint8_t {
    NOP,
    CONSTANT,
    CONSTANT_LONG,
    NIL,
    TRUE,
    FALSE,
    ZERO,
    ONE,
    NEGATE,
    NOT,
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    AND,
    OR,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    SHL,
    SHR,
    INCREMENT,
    DECREMENT,
    JUMP,
    JUMP_IF_FALSE,
    JUMP_IF_TRUE,
    LOOP,
    CALL,
    INVOKE,
    SUPER_INVOKE,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    RETURN_VALUE,
    PRINT,
    POP,
    DUP,
    SWAP,
    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
    GET_INDEX,
    SET_INDEX,
    DEFINE_GLOBAL,
    DEFINE_FUNCTION,
    DEFINE_METHOD,
    DEFINE_CLASS,
    DEFINE_FIELD,
    INHERIT,
    METHOD,
    CLASS,
    INSTANCE,
    ARRAY,
    HASH,
    RANGE,
    SLICE,
    ITER,
    NEXT,
    IMPORT,
    EXPORT,
    THROW,
    CATCH,
    FINALLY,
    TRY,
    AWAIT,
    YIELD,
    RESUME,
    SUSPEND,
    PANIC,
    ASSERT,
    BREAKPOINT,
    PROFILE,
    TRACE,
    DEBUG,
    HALT
};

enum class VMState {
    Ready,
    Running,
    Paused,
    Stopped,
    Error,
    Panic,
    Dead
};

enum class VMError {
    None,
    StackOverflow,
    StackUnderflow,
    OutOfMemory,
    DivisionByZero,
    InvalidOpcode,
    TypeMismatch,
    UndefinedVariable,
    UndefinedFunction,
    InvalidCall,
    InvalidJump,
    InvalidReturn,
    InvalidCast,
    InvalidIndex,
    InvalidProperty,
    InvalidOperation,
    Panic,
    AssertionFailed,
    Timeout,
    SecurityViolation,
    ExternalError
};

struct VMOptions {
    size_t stack_size = 64 * 1024;
    size_t call_stack_size = 1024;
    size_t max_string_length = 64 * 1024;
    size_t max_array_size = 1024 * 1024;
    size_t max_hash_size = 1024 * 1024;
    size_t max_recursion_depth = 1000;
    size_t max_instructions = 0;
    size_t max_memory = 0;
    size_t timeout_ms = 0;
    
    bool enable_gc = true;
    size_t gc_threshold = 8 * 1024 * 1024;
    bool gc_aggressive = false;
    
    bool enable_jit = true;
    size_t jit_threshold = 1000;
    bool jit_optimize = true;
    
    bool enable_profiling = false;
    bool enable_tracing = false;
    bool enable_debugging = false;
    bool enable_assertions = false;
    
    bool enable_security = true;
    bool sandbox_mode = false;
    bool restrict_syscalls = false;
    
    bool enable_exceptions = true;
    bool enable_finalizers = true;
    bool enable_weak_refs = false;
    
    bool multithreaded = false;
    size_t max_threads = 1;
    bool thread_safe = true;
    
    std::string entry_point = "main";
    std::vector<std::string> search_paths;
    std::unordered_map<std::string, std::string> environment;
};

struct VMStats {
    size_t instructions_executed = 0;
    size_t function_calls = 0;
    size_t exceptions_thrown = 0;
    size_t exceptions_caught = 0;
    size_t allocations = 0;
    size_t deallocations = 0;
    size_t gc_collections = 0;
    size_t gc_time_ns = 0;
    size_t jit_compilations = 0;
    size_t jit_time_ns = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    size_t stack_max_usage = 0;
    size_t call_stack_max_usage = 0;
    size_t memory_peak = 0;
    size_t memory_current = 0;
    
    double instructions_per_second = 0.0;
    double gc_overhead_percent = 0.0;
    double jit_overhead_percent = 0.0;
    double cache_hit_rate = 0.0;
    
    std::unordered_map<std::string, size_t> opcode_counts;
    std::unordered_map<std::string, size_t> function_calls_counts;
    std::unordered_map<std::string, size_t> exception_counts;
};

class VirtualMachine {
public:
    VirtualMachine();
    explicit VirtualMachine(const VMOptions& options);
    ~VirtualMachine();
    
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine& operator=(const VirtualMachine&) = delete;
    
    VirtualMachine(VirtualMachine&& other) noexcept;
    VirtualMachine& operator=(VirtualMachine&& other) noexcept;
    
    // Initialization and cleanup
    bool init(const VMOptions& options = VMOptions());
    bool shutdown();
    bool is_initialized() const;
    
    // Bytecode loading and execution
    bool load(const Bytecode* bytecode);
    bool load(const uint8_t* data, size_t size);
    bool load_file(const std::string& filename);
    
    int run();
    int run_function(const std::string& name, const std::vector<Object*>& args = {});
    int run_module(Module* module);
    
    void pause();
    void resume();
    void stop();
    void reset();
    
    // State management
    VMState state() const;
    bool is_running() const;
    bool is_paused() const;
    bool is_stopped() const;
    
    VMError error() const;
    const std::string& error_message() const;
    std::error_code error_code() const;
    
    // Stack operations
    size_t stack_size() const;
    size_t stack_capacity() const;
    size_t call_stack_size() const;
    size_t call_stack_capacity() const;
    
    Object* stack_top() const;
    Object* stack_at(size_t index) const;
    Object* stack_bottom() const;
    
    void stack_push(Object* obj);
    Object* stack_pop();
    void stack_clear();
    void stack_ensure(size_t size);
    
    // Frame management
    size_t frame_count() const;
    CallFrame* current_frame() const;
    CallFrame* frame_at(size_t index) const;
    
    void push_frame(Function* func, uint8_t* ip, Object* receiver = nullptr);
    void pop_frame();
    void clear_frames();
    
    // Function management
    Function* get_function(const std::string& name) const;
    std::vector<Function*> get_functions() const;
    bool has_function(const std::string& name) const;
    
    Object* call_function(Function* func, const std::vector<Object*>& args);
    Object* call_native(void(*native)(), const std::vector<Object*>& args);
    
    // Module management
    Module* get_module(const std::string& name) const;
    std::vector<Module*> get_modules() const;
    bool has_module(const std::string& name) const;
    
    bool import_module(const std::string& name);
    bool export_symbol(const std::string& name, Object* value);
    
    // Global variables
    Object* get_global(const std::string& name) const;
    bool set_global(const std::string& name, Object* value);
    bool has_global(const std::string& name) const;
    void remove_global(const std::string& name);
    
    std::unordered_map<std::string, Object*> globals() const;
    
    // Upvalue management
    Upvalue* capture_upvalue(Object* local);
    void close_upvalues(Object* last);
    size_t upvalue_count() const;
    
    // Exception handling
    void throw_exception(Object* exception);
    void throw_error(const std::string& message);
    void throw_type_error(const std::string& message);
    void throw_range_error(const std::string& message);
    void throw_reference_error(const std::string& message);
    void throw_syntax_error(const std::string& message);
    
    bool has_exception() const;
    Object* current_exception() const;
    void clear_exception();
    
    void push_try_handler(uint8_t* handler_ip);
    void pop_try_handler();
    
    // Garbage collection
    void collect_garbage();
    void collect_garbage_minor();
    void collect_garbage_major();
    
    bool gc_running() const;
    size_t gc_threshold() const;
    void set_gc_threshold(size_t threshold);
    
    void suspend_gc();
    void resume_gc();
    
    // JIT compilation
    void* compile_function(Function* func);
    void* compile_closure(Closure* closure);
    bool invalidate_code(void* code_ptr);
    void flush_jit_cache();
    
    bool jit_enabled() const;
    void enable_jit(bool enable);
    
    // Debugging and profiling
    void set_breakpoint(const std::string& function, size_t instruction);
    void clear_breakpoint(const std::string& function, size_t instruction);
    void clear_all_breakpoints();
    
    void step();
    void step_over();
    void step_out();
    void continue_execution();
    
    void start_profiling();
    void stop_profiling();
    VMStats get_profile_data();
    
    void enable_tracing(bool enable);
    bool tracing_enabled() const;
    
    // Security
    void enable_sandbox(bool enable);
    bool sandbox_enabled() const;
    
    void restrict_syscall(int syscall, bool allow);
    bool is_syscall_allowed(int syscall) const;
    
    void set_capability(const std::string& cap, bool granted);
    bool has_capability(const std::string& cap) const;
    
    // Memory management
    MemoryManager* memory();
    const MemoryManager* memory() const;
    
    GarbageCollector* gc();
    const GarbageCollector* gc() const;
    
    // Configuration
    const VMOptions& options() const;
    VMOptions& mutable_options();
    bool reconfigure(const VMOptions& new_options);
    
    // Statistics
    const VMStats& stats() const;
    void reset_stats();
    void print_stats() const;
    
    // Serialization
    bool save_state(const std::string& filename) const;
    bool load_state(const std::string& filename);
    
    size_t serialize_state(void* buffer, size_t size) const;
    bool deserialize_state(const void* buffer, size_t size);
    
    // Utility functions
    static std::string opcode_to_string(VMOpcode op);
    static VMOpcode string_to_opcode(const std::string& str);
    
    static std::string state_to_string(VMState state);
    static std::string error_to_string(VMError error);
    
    static size_t opcode_size(VMOpcode op);
    static bool opcode_has_operand(VMOpcode op);
    static bool opcode_is_jump(VMOpcode op);
    static bool opcode_is_call(VMOpcode op);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace klang