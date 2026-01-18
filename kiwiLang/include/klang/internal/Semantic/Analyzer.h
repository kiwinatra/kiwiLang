#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace kiwiLang {

class ASTNode;
class Module;
class FunctionDecl;
class StructDecl;
class EnumDecl;
class VarDecl;
class Type;
class Expression;
class Statement;
class SymbolTable;
class TypeChecker;
class Diagnostics;

enum class AnalysisPhase {
    Lexical,
    Syntactic,
    Semantic,
    Type,
    ControlFlow,
    DataFlow,
    Optimization,
    Codegen,
    Complete
};

enum class AnalysisResult {
    Success,
    Warning,
    Error,
    Fatal
};

struct AnalysisContext {
    Module* current_module = nullptr;
    FunctionDecl* current_function = nullptr;
    StructDecl* current_struct = nullptr;
    EnumDecl* current_enum = nullptr;
    Scope* current_scope = nullptr;
    
    AnalysisPhase phase = AnalysisPhase::Lexical;
    uint32_t pass = 0;
    bool in_loop = false;
    bool in_switch = false;
    bool in_try_block = false;
    bool in_catch_block = false;
    bool in_finally_block = false;
    bool in_template = false;
    bool in_generic = false;
    bool in_constexpr = false;
    bool in_comptime = false;
    
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
    void reset();
    void enter_function(FunctionDecl* func);
    void exit_function();
    void enter_struct(StructDecl* str);
    void exit_struct();
    void enter_enum(EnumDecl* enm);
    void exit_enum();
    void enter_loop();
    void exit_loop();
    void enter_switch();
    void exit_switch();
    
    bool has_errors() const;
    bool has_warnings() const;
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(Diagnostics& diagnostics);
    ~SemanticAnalyzer();
    
    SemanticAnalyzer(const SemanticAnalyzer&) = delete;
    SemanticAnalyzer& operator=(const SemanticAnalyzer&) = delete;
    
    // Main analysis entry points
    AnalysisResult analyze(Module* module);
    AnalysisResult analyze(FunctionDecl* func);
    AnalysisResult analyze(StructDecl* str);
    AnalysisResult analyze(EnumDecl* enm);
    AnalysisResult analyze(VarDecl* var);
    AnalysisResult analyze(Expression* expr);
    AnalysisResult analyze(Statement* stmt);
    
    // Phased analysis
    AnalysisResult analyze_phase(Module* module, AnalysisPhase phase);
    AnalysisResult analyze_all_phases(Module* module);
    
    // Incremental analysis
    AnalysisResult reanalyze(Module* module, const std::vector<ASTNode*>& changed_nodes);
    AnalysisResult update_analysis(Module* module, ASTNode* changed_node);
    
    // Symbol table access
    SymbolTable* symbol_table();
    const SymbolTable* symbol_table() const;
    
    void set_symbol_table(SymbolTable* symbols);
    
    // Type checker access
    TypeChecker* type_checker();
    const TypeChecker* type_checker() const;
    
    // Context management
    AnalysisContext& context();
    const AnalysisContext& context() const;
    
    void push_context();
    void pop_context();
    void reset_context();
    
    // Name resolution
    void resolve_names(Module* module);
    void resolve_names(FunctionDecl* func);
    void resolve_names(Expression* expr);
    
    Symbol* resolve_symbol(const std::string& name, Scope* scope = nullptr);
    Type* resolve_type(const std::string& name, Scope* scope = nullptr);
    FunctionDecl* resolve_function(const std::string& name, const std::vector<Type*>& arg_types, Scope* scope = nullptr);
    
    // Type resolution
    void resolve_types(Module* module);
    void resolve_types(FunctionDecl* func);
    void resolve_types(Expression* expr);
    
    Type* resolve_type_expression(Expression* expr);
    Type* deduce_type(Expression* expr);
    
    // Constant evaluation
    bool is_constant_expression(Expression* expr);
    std::optional<int64_t> evaluate_constant_int(Expression* expr);
    std::optional<double> evaluate_constant_float(Expression* expr);
    std::optional<bool> evaluate_constant_bool(Expression* expr);
    std::optional<std::string> evaluate_constant_string(Expression* expr);
    
    // Control flow analysis
    void analyze_control_flow(FunctionDecl* func);
    void analyze_control_flow(Statement* stmt);
    
    bool returns_on_all_paths(FunctionDecl* func);
    bool may_return_early(FunctionDecl* func);
    bool has_unreachable_code(FunctionDecl* func);
    bool has_infinite_loop(FunctionDecl* func);
    
    // Data flow analysis
    void analyze_data_flow(FunctionDecl* func);
    void analyze_data_flow(Statement* stmt);
    
    bool is_definitely_assigned(const std::string& var_name, Statement* stmt);
    bool is_definitely_unassigned(const std::string& var_name, Statement* stmt);
    bool may_be_used_before_assignment(const std::string& var_name, Statement* stmt);
    
    std::vector<std::string> get_live_variables(Statement* stmt);
    std::vector<std::string> get_available_expressions(Statement* stmt);
    
    // Lifetime analysis
    void analyze_lifetimes(FunctionDecl* func);
    void analyze_lifetimes(Expression* expr);
    
    bool check_borrow(const std::string& var_name, Expression* expr);
    bool check_move(const std::string& var_name, Expression* expr);
    bool check_lifetime(const std::string& var_name, Expression* expr);
    
    // Type checking
    void check_types(Module* module);
    void check_types(FunctionDecl* func);
    void check_types(Expression* expr);
    
    bool check_type_compatibility(Type* expected, Type* actual, Expression* expr);
    bool check_assignment_compatibility(Type* target, Type* source, Expression* expr);
    bool check_binary_operation(Type* left, Type* right, const std::string& op, Expression* expr);
    bool check_unary_operation(Type* operand, const std::string& op, Expression* expr);
    
    // Validation
    bool validate(Module* module);
    bool validate(FunctionDecl* func);
    bool validate(StructDecl* str);
    bool validate(EnumDecl* enm);
    bool validate(VarDecl* var);
    
    std::vector<std::string> validation_errors() const;
    std::vector<std::string> validation_warnings() const;
    
    // Error reporting
    void report_error(const std::string& message, SourceLocation loc);
    void report_warning(const std::string& message, SourceLocation loc);
    void report_note(const std::string& message, SourceLocation loc);
    
    // Configuration
    void set_strict_mode(bool strict);
    bool strict_mode() const;
    
    void set_pedantic_mode(bool pedantic);
    bool pedantic_mode() const;
    
    void set_enable_warnings(bool enable);
    bool enable_warnings() const;
    
    void set_warnings_as_errors(bool enable);
    bool warnings_as_errors() const;
    
    void set_max_errors(size_t max);
    size_t max_errors() const;
    
    void set_max_warnings(size_t max);
    size_t max_warnings() const;
    
    // Optimization flags
    void set_enable_constant_folding(bool enable);
    bool enable_constant_folding() const;
    
    void set_enable_dead_code_elimination(bool enable);
    bool enable_dead_code_elimination() const;
    
    void set_enable_inlining(bool enable);
    bool enable_inlining() const;
    
    // Analysis passes
    void add_analysis_pass(const std::string& name, std::function<void(Module*)> pass);
    void remove_analysis_pass(const std::string& name);
    void run_analysis_passes(Module* module);
    
    // Visitor pattern support
    class Visitor {
    public:
        virtual ~Visitor() = default;
        
        virtual void visit_module(Module* module) = 0;
        virtual void visit_function(FunctionDecl* func) = 0;
        virtual void visit_struct(StructDecl* str) = 0;
        virtual void visit_enum(EnumDecl* enm) = 0;
        virtual void visit_variable(VarDecl* var) = 0;
        virtual void visit_expression(Expression* expr) = 0;
        virtual void visit_statement(Statement* stmt) = 0;
    };
    
    void accept(Visitor* visitor, Module* module);
    void accept(Visitor* visitor, FunctionDecl* func);
    void accept(Visitor* visitor, StructDecl* str);
    void accept(Visitor* visitor, EnumDecl* enm);
    
    // Statistics
    struct Stats {
        size_t modules_analyzed = 0;
        size_t functions_analyzed = 0;
        size_t structs_analyzed = 0;
        size_t enums_analyzed = 0;
        size_t variables_analyzed = 0;
        size_t expressions_analyzed = 0;
        size_t statements_analyzed = 0;
        
        size_t name_resolutions = 0;
        size_t type_resolutions = 0;
        size_t constant_evaluations = 0;
        size_t control_flow_analyses = 0;
        size_t data_flow_analyses = 0;
        size_t lifetime_analyses = 0;
        size_t type_checks = 0;
        
        size_t errors_found = 0;
        size_t warnings_found = 0;
        size_t notes_found = 0;
        
        size_t implicit_conversions = 0;
        size_t explicit_conversions = 0;
        size_t narrowing_conversions = 0;
        
        size_t overload_resolutions = 0;
        size_t template_instantiations = 0;
        size_t generic_instantiations = 0;
        
        double average_analysis_time_ms = 0.0;
        size_t peak_memory_usage_mb = 0;
        size_t max_analysis_depth = 0;
        
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t cache_evictions = 0;
    };
    
    Stats stats() const;
    void reset_stats();
    
    // Cache management
    void clear_cache();
    void enable_caching(bool enable);
    bool caching_enabled() const;
    
    // State management
    void reset();
    bool is_ready() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kiwiLang