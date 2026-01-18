include/klang/internal/Parser/Parser.h
#pragma once
#include "klang/internal/Lexer/Token.h"
#include "klang/internal/AST/Expr.h"
#include "klang/internal/AST/Stmt.h"
#include "klang/internal/Diagnostics/Diagnostic.h"
#include "klang/internal/Utils/SourceLocation.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

namespace klang {
namespace internal {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, DiagnosticReporter& diagnostics);
    
    std::unique_ptr<ast::Program> parseProgram();
    std::unique_ptr<ast::Module> parseModule();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }

private:
    std::vector<Token> tokens_;
    std::size_t current_;
    DiagnosticReporter& diagnostics_;
    std::unordered_map<std::string, ast::FunctionDecl*> functionTable_;
    std::unordered_map<std::string, ast::ClassDecl*> classTable_;
    
    // Error recovery
    bool hadError_;
    bool panicMode_;
    
    // Parsing methods
    const Token& advance();
    const Token& peek() const;
    const Token& previous() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    bool consume(TokenType type, std::string_view message);
    Token consumeIdentifier(std::string_view message);
    
    void synchronize();
    bool isAtEnd() const;
    
    // Declaration parsing
    std::unique_ptr<ast::Decl> declaration();
    std::unique_ptr<ast::FunctionDecl> functionDeclaration();
    std::unique_ptr<ast::ClassDecl> classDeclaration();
    std::unique_ptr<ast::VarDecl> variableDeclaration();
    std::unique_ptr<ast::ImportDecl> importDeclaration();
    std::unique_ptr<ast::ExportDecl> exportDeclaration();
    
    // Statement parsing
    std::unique_ptr<ast::Stmt> statement();
    std::unique_ptr<ast::BlockStmt> blockStatement();
    std::unique_ptr<ast::IfStmt> ifStatement();
    std::unique_ptr<ast::WhileStmt> whileStatement();
    std::unique_ptr<ast::ForStmt> forStatement();
    std::unique_ptr<ast::ReturnStmt> returnStatement();
    std::unique_ptr<ast::BreakStmt> breakStatement();
    std::unique_ptr<ast::ContinueStmt> continueStatement();
    std::unique_ptr<ast::ExpressionStmt> expressionStatement();
    std::unique_ptr<ast::TryCatchStmt> tryCatchStatement();
    std::unique_ptr<ast::ThrowStmt> throwStatement();
    
    // Expression parsing (operator precedence parsing)
    std::unique_ptr<ast::Expr> expression();
    std::unique_ptr<ast::Expr> assignment();
    std::unique_ptr<ast::Expr> logicalOr();
    std::unique_ptr<ast::Expr> logicalAnd();
    std::unique_ptr<ast::Expr> bitwiseOr();
    std::unique_ptr<ast::Expr> bitwiseXor();
    std::unique_ptr<ast::Expr> bitwiseAnd();
    std::unique_ptr<ast::Expr> equality();
    std::unique_ptr<ast::Expr> comparison();
    std::unique_ptr<ast::Expr> shift();
    std::unique_ptr<ast::Expr> term();
    std::unique_ptr<ast::Expr> factor();
    std::unique_ptr<ast::Expr> unary();
    std::unique_ptr<ast::Expr> call();
    std::unique_ptr<ast::Expr> primary();
    
    // Helpers for expressions
    std::unique_ptr<ast::Expr> finishCall(std::unique_ptr<ast::Expr> callee);
    std::unique_ptr<ast::Expr> finishArrayLiteral();
    std::unique_ptr<ast::Expr> finishObjectLiteral();
    std::unique_ptr<ast::Expr> finishLambda();
    
    // Type parsing
    ast::Type typeAnnotation();
    ast::Type functionType();
    ast::Type arrayType();
    ast::Type mapType();
    ast::Type genericType();
    ast::Type primaryType();
    
    // Template/generics
    std::vector<ast::TemplateParameter> templateParameters();
    std::vector<ast::Type> templateArguments();
    
    // Pattern matching
    std::unique_ptr<ast::Pattern> pattern();
    std::unique_ptr<ast::IdentifierPattern> identifierPattern();
    std::unique_ptr<ast::LiteralPattern> literalPattern();
    std::unique_ptr<ast::WildcardPattern> wildcardPattern();
    std::unique_ptr<ast::DestructurePattern> destructurePattern();
    
    // Error reporting
    void error(const Token& token, std::string_view message);
    void errorAtCurrent(std::string_view message);
    void warning(const Token& token, std::string_view message);
    
    // Recovery points
    struct RecoveryPoint {
        std::size_t tokenIndex;
        bool hadError;
        bool panicMode;
    };
    
    RecoveryPoint createRecoveryPoint();
    void restoreRecoveryPoint(const RecoveryPoint& point);
    
    // Lookahead utilities
    std::optional<TokenType> lookahead(std::size_t offset) const;
    bool checkSequence(std::initializer_list<TokenType> types) const;
    
    // Context tracking
    enum class Context {
        NONE,
        FUNCTION,
        LOOP,
        CLASS,
        MODULE
    };
    
    Context context_;
    std::vector<Context> contextStack_;
    
    void pushContext(Context context);
    void popContext();
    bool inContext(Context context) const;
    
    // Module management
    std::string currentModule_;
    std::unordered_map<std::string, std::unique_ptr<ast::Module>> importedModules_;
};

} // namespace internal
} // namespace klang