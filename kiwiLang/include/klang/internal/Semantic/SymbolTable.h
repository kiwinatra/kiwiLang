#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace kiwiLang {

class Type;
class ASTNode;
class Scope;
class Symbol;

enum class SymbolKind {
    Unknown,
    Variable,
    Function,
    Parameter,
    Struct,
    Enum,
    EnumVariant,
    TypeAlias,
    Module,
    Namespace,
    Label,
    Constant,
    GenericParam,
    Template,
    Macro,
    Builtin,
    External
};

enum class StorageClass {
    None,
    Auto,
    Static,
    Extern,
    Register,
    ThreadLocal,
    Constexpr,
    Inline,
    Volatile,
    Mutable
};

enum class Linkage {
    None,
    Internal,
    External,
    Weak,
    Private,
    Common
};

enum class Visibility {
    Private,
    Public,
    Protected,
    Internal,
    Hidden
};

struct SymbolAttributes {
    StorageClass storage = StorageClass::None;
    Linkage linkage = Linkage::None;
    Visibility visibility = Visibility::Private;
    bool is_const = false;
    bool is_volatile = false;
    bool is_restrict = false;
    bool is_atomic = false;
    bool is_virtual = false;
    bool is_override = false;
    bool is_final = false;
    bool is_abstract = false;
    bool is_pure = false;
    bool is_extern = false;
    bool is_export = false;
    bool is_import = false;
    bool is_deprecated = false;
    bool is_experimental = false;
    bool is_unstable = false;
    bool is_test = false;
    bool is_benchmark = false;
    bool is_documented = false;
    
    uint32_t alignment = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    
    std::string section;
    std::string visibility_str;
    std::string linkage_str;
    std::string attributes;
    
    bool has_attribute(const std::string& attr) const;
    void add_attribute(const std::string& attr);
    void remove_attribute(const std::string& attr);
};

class Symbol {
public:
    Symbol(SymbolKind kind, std::string name, Type* type = nullptr);
    ~Symbol();
    
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;
    
    Symbol(Symbol&& other) noexcept;
    Symbol& operator=(Symbol&& other) noexcept;
    
    SymbolKind kind() const;
    const std::string& name() const;
    std::string_view name_view() const;
    
    Type* type() const;
    void set_type(Type* type);
    
    ASTNode* node() const;
    void set_node(ASTNode* node);
    
    Scope* scope() const;
    void set_scope(Scope* scope);
    
    const SymbolAttributes& attributes() const;
    SymbolAttributes& mutable_attributes();
    void set_attributes(const SymbolAttributes& attrs);
    
    bool is_variable() const;
    bool is_function() const;
    bool is_parameter() const;
    bool is_struct() const;
    bool is_enum() const;
    bool is_type() const;
    bool is_constant() const;
    bool is_generic() const;
    bool is_builtin() const;
    bool is_external() const;
    
    bool is_const() const;
    bool is_mutable() const;
    bool is_static() const;
    bool is_extern() const;
    bool is_inline() const;
    bool is_virtual() const;
    bool is_override() const;
    bool is_final() const;
    bool is_abstract() const;
    
    size_t hash() const;
    std::string to_string() const;
    
    struct Hasher {
        size_t operator()(const Symbol* sym) const;
    };
    
    struct Equal {
        bool operator()(const Symbol* a, const Symbol* b) const;
    };
    
private:
    SymbolKind kind_;
    std::string name_;
    Type* type_ = nullptr;
    ASTNode* node_ = nullptr;
    Scope* scope_ = nullptr;
    SymbolAttributes attributes_;
};

class Scope {
public:
    Scope();
    explicit Scope(Scope* parent);
    explicit Scope(Scope* parent, const std::string& name);
    ~Scope();
    
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    
    Scope(Scope&& other) noexcept;
    Scope& operator=(Scope&& other) noexcept;
    
    Scope* parent() const;
    void set_parent(Scope* parent);
    
    const std::string& name() const;
    void set_name(const std::string& name);
    
    Scope* root() const;
    Scope* global() const;
    
    bool is_global() const;
    bool is_function() const;
    bool is_block() const;
    bool is_class() const;
    bool is_namespace() const;
    bool is_template() const;
    
    Symbol* insert(Symbol* symbol);
    Symbol* insert(std::unique_ptr<Symbol> symbol);
    
    Symbol* find(const std::string& name) const;
    Symbol* find_local(const std::string& name) const;
    Symbol* find_in_parents(const std::string& name) const;
    
    std::vector<Symbol*> find_all(const std::string& name) const;
    std::vector<Symbol*> find_by_kind(SymbolKind kind) const;
    std::vector<Symbol*> find_by_predicate(std::function<bool(const Symbol*)> pred) const;
    
    bool contains(const std::string& name) const;
    bool contains_local(const std::string& name) const;
    bool contains_symbol(const Symbol* symbol) const;
    
    size_t symbol_count() const;
    size_t local_symbol_count() const;
    
    void remove(const std::string& name);
    void remove(Symbol* symbol);
    void clear();
    
    std::vector<Symbol*> symbols() const;
    std::vector<Symbol*> local_symbols() const;
    std::vector<Symbol*> all_symbols() const;
    
    Scope* create_child(const std::string& name = "");
    void add_child(std::unique_ptr<Scope> child);
    void remove_child(Scope* child);
    
    size_t child_count() const;
    const std::vector<Scope*>& children() const;
    
    void set_depth(uint32_t depth);
    uint32_t depth() const;
    
    void set_index(uint32_t index);
    uint32_t index() const;
    
    size_t hash() const;
    std::string to_string() const;
    
private:
    Scope* parent_ = nullptr;
    std::string name_;
    uint32_t depth_ = 0;
    uint32_t index_ = 0;
    
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols_;
    std::vector<std::unique_ptr<Scope>> children_;
    std::vector<Scope*> child_ptrs_;
    
    void update_child_depths();
};

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();
    
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;
    
    SymbolTable(SymbolTable&& other) noexcept;
    SymbolTable& operator=(SymbolTable&& other) noexcept;
    
    // Scope management
    Scope* enter_scope(const std::string& name = "");
    Scope* enter_scope(Scope* scope);
    void exit_scope();
    
    Scope* current_scope() const;
    Scope* global_scope() const;
    Scope* root_scope() const;
    
    Scope* get_scope(uint32_t depth, uint32_t index) const;
    Scope* find_scope(const std::string& name) const;
    Scope* find_containing_scope(const Symbol* symbol) const;
    
    // Symbol management
    Symbol* insert(Symbol* symbol);
    Symbol* insert(std::unique_ptr<Symbol> symbol);
    
    Symbol* declare(const std::string& name, SymbolKind kind, Type* type = nullptr);
    Symbol* define(const std::string& name, SymbolKind kind, Type* type = nullptr, ASTNode* node = nullptr);
    
    Symbol* find(const std::string& name) const;
    Symbol* find_local(const std::string& name) const;
    Symbol* find_in_scope(const std::string& name, Scope* scope) const;
    Symbol* find_in_all_scopes(const std::string& name) const;
    
    std::vector<Symbol*> find_all(const std::string& name) const;
    std::vector<Symbol*> find_by_kind(SymbolKind kind) const;
    std::vector<Symbol*> find_by_type(Type* type) const;
    std::vector<Symbol*> find_by_predicate(std::function<bool(const Symbol*)> pred) const;
    
    bool contains(const std::string& name) const;
    bool contains_local(const std::string& name) const;
    bool contains_symbol(const Symbol* symbol) const;
    
    void remove(const std::string& name);
    void remove(Symbol* symbol);
    void clear();
    
    // Import/Export
    void import_from(SymbolTable* other, const std::string& prefix = "");
    void export_to(SymbolTable* other, const std::string& prefix = "") const;
    
    void import_symbol(Symbol* symbol, const std::string& alias = "");
    void export_symbol(Symbol* symbol, const std::string& alias = "");
    
    // Namespace management
    Scope* enter_namespace(const std::string& name);
    void exit_namespace();
    
    Scope* current_namespace() const;
    Scope* find_namespace(const std::string& name) const;
    
    // Type management
    Symbol* declare_type(const std::string& name, Type* type);
    Symbol* define_type(const std::string& name, Type* type, ASTNode* node = nullptr);
    
    Type* find_type(const std::string& name) const;
    std::vector<Type*> find_types_by_predicate(std::function<bool(Type*)> pred) const;
    
    // Function management
    Symbol* declare_function(const std::string& name, Type* type);
    Symbol* define_function(const std::string& name, Type* type, ASTNode* node = nullptr);
    
    std::vector<Symbol*> find_functions(const std::string& name) const;
    std::vector<Symbol*> find_functions_by_type(Type* type) const;
    
    // Variable management
    Symbol* declare_variable(const std::string& name, Type* type);
    Symbol* define_variable(const std::string& name, Type* type, ASTNode* node = nullptr);
    
    std::vector<Symbol*> find_variables(const std::string& name) const;
    
    // Overload resolution
    Symbol* resolve_overload(const std::string& name, const std::vector<Type*>& arg_types);
    std::vector<Symbol*> find_overload_candidates(const std::string& name, const std::vector<Type*>& arg_types);
    
    // Shadowing and hiding
    bool is_shadowed(const std::string& name) const;
    bool is_hidden(const std::string& name) const;
    std::vector<Symbol*> get_shadowed_symbols(const std::string& name) const;
    
    void allow_shadowing(bool allow);
    bool allows_shadowing() const;
    
    // Validation
    bool validate() const;
    std::vector<std::string> validation_errors() const;
    
    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
    // Statistics
    struct Stats {
        size_t total_symbols = 0;
        size_t unique_symbols = 0;
        size_t scopes = 0;
        size_t max_scope_depth = 0;
        size_t average_symbols_per_scope = 0;
        
        size_t variables = 0;
        size_t functions = 0;
        size_t parameters = 0;
        size_t structs = 0;
        size_t enums = 0;
        size_t types = 0;
        size_t constants = 0;
        size_t generics = 0;
        
        size_t shadowed_symbols = 0;
        size_t hidden_symbols = 0;
        size_t overload_sets = 0;
        size_t unresolved_symbols = 0;
        
        size_t insertions = 0;
        size_t deletions = 0;
        size_t lookups = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        
        double average_lookup_time_ns = 0.0;
        size_t memory_usage_bytes = 0;
    };
    
    Stats stats() const;
    void reset_stats();
    
    // Debugging
    void dump() const;
    void dump_scope(Scope* scope, int indent = 0) const;
    std::string to_dot() const;
    
    // Configuration
    void set_case_sensitive(bool sensitive);
    bool case_sensitive() const;
    
    void set_allow_redeclaration(bool allow);
    bool allow_redeclaration() const;
    
    void set_allow_redefinition(bool allow);
    bool allow_redefinition() const;
    
    void set_max_scope_depth(uint32_t depth);
    uint32_t max_scope_depth() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

inline SymbolKind Symbol::kind() const {
    return kind_;
}

inline const std::string& Symbol::name() const {
    return name_;
}

inline std::string_view Symbol::name_view() const {
    return name_;
}

inline Type* Symbol::type() const {
    return type_;
}

inline void Symbol::set_type(Type* type) {
    type_ = type;
}

inline ASTNode* Symbol::node() const {
    return node_;
}

inline void Symbol::set_node(ASTNode* node) {
    node_ = node;
}

inline Scope* Symbol::scope() const {
    return scope_;
}

inline void Symbol::set_scope(Scope* scope) {
    scope_ = scope;
}

inline const SymbolAttributes& Symbol::attributes() const {
    return attributes_;
}

inline SymbolAttributes& Symbol::mutable_attributes() {
    return attributes_;
}

inline void Symbol::set_attributes(const SymbolAttributes& attrs) {
    attributes_ = attrs;
}

inline bool Symbol::is_variable() const {
    return kind_ == SymbolKind::Variable;
}

inline bool Symbol::is_function() const {
    return kind_ == SymbolKind::Function;
}

inline bool Symbol::is_parameter() const {
    return kind_ == SymbolKind::Parameter;
}

inline bool Symbol::is_struct() const {
    return kind_ == SymbolKind::Struct;
}

inline bool Symbol::is_enum() const {
    return kind_ == SymbolKind::Enum;
}

inline bool Symbol::is_type() const {
    return kind_ == SymbolKind::Struct || kind_ == SymbolKind::Enum || 
           kind_ == SymbolKind::TypeAlias || kind_ == SymbolKind::GenericParam;
}

inline bool Symbol::is_constant() const {
    return kind_ == SymbolKind::Constant;
}

inline bool Symbol::is_generic() const {
    return kind_ == SymbolKind::GenericParam || kind_ == SymbolKind::Template;
}

inline bool Symbol::is_builtin() const {
    return kind_ == SymbolKind::Builtin;
}

inline bool Symbol::is_external() const {
    return kind_ == SymbolKind::External;
}

inline bool Symbol::is_const() const {
    return attributes_.is_const;
}

inline bool Symbol::is_mutable() const {
    return attributes_.is_const == false && attributes_.storage != StorageClass::Constexpr;
}

inline bool Symbol::is_static() const {
    return attributes_.storage == StorageClass::Static;
}

inline bool Symbol::is_extern() const {
    return attributes_.is_extern;
}

inline bool Symbol::is_inline() const {
    return attributes_.storage == StorageClass::Inline;
}

inline bool Symbol::is_virtual() const {
    return attributes_.is_virtual;
}

inline bool Symbol::is_override() const {
    return attributes_.is_override;
}

inline bool Symbol::is_final() const {
    return attributes_.is_final;
}

inline bool Symbol::is_abstract() const {
    return attributes_.is_abstract;
}

inline size_t Symbol::Hasher::operator()(const Symbol* sym) const {
    return std::hash<std::string>{}(sym->name()) ^ 
           std::hash<int>{}(static_cast<int>(sym->kind()));
}

inline bool Symbol::Equal::operator()(const Symbol* a, const Symbol* b) const {
    return a == b || (a->name() == b->name() && a->kind() == b->kind());
}

inline Scope* Scope::parent() const {
    return parent_;
}

inline void Scope::set_parent(Scope* parent) {
    parent_ = parent;
}

inline const std::string& Scope::name() const {
    return name_;
}

inline void Scope::set_name(const std::string& name) {
    name_ = name;
}

inline bool Scope::is_global() const {
    return parent_ == nullptr;
}

inline size_t Scope::symbol_count() const {
    return symbols_.size();
}

inline size_t Scope::local_symbol_count() const {
    return symbols_.size();
}

inline size_t Scope::child_count() const {
    return children_.size();
}

inline const std::vector<Scope*>& Scope::children() const {
    return child_ptrs_;
}

inline void Scope::set_depth(uint32_t depth) {
    depth_ = depth;
}

inline uint32_t Scope::depth() const {
    return depth_;
}

inline void Scope::set_index(uint32_t index) {
    index_ = index;
}

inline uint32_t Scope::index() const {
    return index_;
}

} // namespace kiwiLang