#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace klang {

class Type;
class ASTNode;
class FunctionDecl;
class StructDecl;
class EnumDecl;
class VarDecl;
class Expression;
class Statement;
class SymbolTable;
class Diagnostics;

enum class TypeRelation {
    Exact,
    Convertible,
    Assignable,
    Comparable,
    Compatible,
    Incompatible
};

enum class ConversionRank {
    Exact,          // No conversion needed
    Promotion,      // Safe widening conversion
    Conversion,     // Safe conversion
    Narrowing,      // Potentially unsafe narrowing
    UserDefined,    // User-defined conversion
    Invalid         // No conversion possible
};

struct TypeConversion {
    Type* source;
    Type* target;
    ConversionRank rank;
    bool is_implicit;
    bool is_lossy;
    std::function<Expression*(Expression*)> conversion_function;
    
    TypeConversion(Type* src, Type* tgt, ConversionRank r, bool implicit = true)
        : source(src), target(tgt), rank(r), is_implicit(implicit), is_lossy(r == ConversionRank::Narrowing) {}
};

class TypeChecker {
public:
    explicit TypeChecker(Diagnostics& diagnostics);
    ~TypeChecker();
    
    TypeChecker(const TypeChecker&) = delete;
    TypeChecker& operator=(const TypeChecker&) = delete;
    
    // Type checking entry points
    Type* check_expression(Expression* expr);
    void check_statement(Statement* stmt);
    void check_function(FunctionDecl* func);
    void check_struct(StructDecl* str);
    void check_enum(EnumDecl* enm);
    void check_variable(VarDecl* var);
    void check_module(ASTNode* module);
    
    // Type inference
    Type* infer_type(Expression* expr);
    Type* infer_binary_expression(Expression* left, Expression* right, const std::string& op);
    Type* infer_unary_expression(Expression* expr, const std::string& op);
    Type* infer_call_expression(Expression* callee, const std::vector<Expression*>& args);
    
    // Type compatibility
    TypeRelation check_type_relation(Type* t1, Type* t2);
    bool is_assignable(Type* target, Type* source);
    bool is_convertible(Type* source, Type* target);
    bool is_comparable(Type* t1, Type* t2);
    bool is_compatible(Type* t1, Type* t2);
    
    ConversionRank get_conversion_rank(Type* source, Type* target);
    TypeConversion get_conversion(Type* source, Type* target);
    
    // Type operations
    Type* common_type(Type* t1, Type* t2);
    Type* promote_type(Type* type);
    Type* demote_type(Type* type);
    
    // Type constraints
    bool satisfies_constraints(Type* type, const std::vector<Type*>& constraints);
    bool check_type_constraints(Type* type, const std::string& constraint);
    
    // Generic type checking
    Type* instantiate_generic(Type* generic, const std::vector<Type*>& type_args);
    bool check_generic_constraints(Type* generic, const std::vector<Type*>& type_args);
    
    // Overload resolution
    FunctionDecl* resolve_overload(const std::vector<FunctionDecl*>& candidates,
                                   const std::vector<Expression*>& args);
    int score_overload(FunctionDecl* func, const std::vector<Expression*>& args);
    
    // Template argument deduction
    std::vector<Type*> deduce_template_arguments(FunctionDecl* template_func,
                                                 const std::vector<Expression*>& args);
    
    // Type trait checking
    bool has_trait(Type* type, const std::string& trait_name);
    bool check_trait_implementation(Type* type, const std::string& trait_name);
    
    // Constexpr evaluation
    bool is_constant_expression(Expression* expr);
    std::optional<int64_t> evaluate_constant_int(Expression* expr);
    std::optional<double> evaluate_constant_float(Expression* expr);
    std::optional<bool> evaluate_constant_bool(Expression* expr);
    std::optional<std::string> evaluate_constant_string(Expression* expr);
    
    // Control flow analysis
    bool returns_on_all_paths(FunctionDecl* func);
    bool may_return_early(FunctionDecl* func);
    bool has_unreachable_code(FunctionDecl* func);
    
    // Data flow analysis
    bool is_definitely_assigned(const std::string& var_name, Statement* stmt);
    bool is_definitely_unassigned(const std::string& var_name, Statement* stmt);
    bool may_be_used_before_assignment(const std::string& var_name, Statement* stmt);
    
    // Lifetime analysis
    bool check_lifetime(const std::string& var_name, Expression* expr);
    bool check_borrow(const std::string& var_name, Expression* expr);
    bool check_move(const std::string& var_name, Expression* expr);
    
    // Error reporting
    void report_type_error(const std::string& message, SourceLocation loc);
    void report_type_mismatch(Type* expected, Type* actual, SourceLocation loc);
    void report_ambiguous_call(const std::vector<FunctionDecl*>& candidates, SourceLocation loc);
    void report_no_matching_overload(const std::string& name, const std::vector<Type*>& arg_types, SourceLocation loc);
    void report_invalid_conversion(Type* source, Type* target, SourceLocation loc);
    
    // Configuration
    void set_strict_mode(bool strict);
    bool strict_mode() const;
    
    void set_pedantic_mode(bool pedantic);
    bool pedantic_mode() const;
    
    void set_allow_implicit_conversions(bool allow);
    bool allow_implicit_conversions() const;
    
    void set_allow_narrowing_conversions(bool allow);
    bool allow_narrowing_conversions() const;
    
    // Context management
    void enter_scope();
    void exit_scope();
    
    void push_function(FunctionDecl* func);
    void pop_function();
    
    void push_loop();
    void pop_loop();
    
    void push_switch();
    void pop_switch();
    
    // Symbol table access
    void set_symbol_table(SymbolTable* symbols);
    SymbolTable* symbol_table() const;
    
    // Type cache
    void clear_cache();
    void reset();
    
    // Statistics
    struct Stats {
        size_t type_checks = 0;
        size_t conversions = 0;
        size_t overload_resolutions = 0;
        size_t generic_instantiations = 0;
        size_t constant_evaluations = 0;
        
        size_t implicit_conversions = 0;
        size_t explicit_conversions = 0;
        size_t narrowing_conversions = 0;
        size_t user_conversions = 0;
        
        size_t type_errors = 0;
        size_t conversion_errors = 0;
        size_t overload_errors = 0;
        size_t generic_errors = 0;
        
        double average_check_time_ns = 0.0;
        size_t max_check_depth = 0;
        size_t recursive_types = 0;
        size_t polymorphic_types = 0;
    };
    
    Stats stats() const;
    void reset_stats();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace klang