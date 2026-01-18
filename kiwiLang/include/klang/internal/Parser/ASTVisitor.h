include/klang/internal/Parser/ASTVisitor.h
#pragma once
#include "klang/internal/AST/Expr.h"
#include "klang/internal/AST/Stmt.h"
#include "klang/internal/AST/Decl.h"
#include "klang/internal/AST/Type.h"
#include <type_traits>
#include <memory>

namespace klang {
namespace internal {
namespace ast {

class Visitor {
public:
    virtual ~Visitor() = default;
    
    // Expressions
    virtual void visit(AssignExpr& expr) = 0;
    virtual void visit(BinaryExpr& expr) = 0;
    virtual void visit(UnaryExpr& expr) = 0;
    virtual void visit(CallExpr& expr) = 0;
    virtual void visit(GetExpr& expr) = 0;
    virtual void visit(SetExpr& expr) = 0;
    virtual void visit(SuperExpr& expr) = 0;
    virtual void visit(ThisExpr& expr) = 0;
    virtual void visit(LiteralExpr& expr) = 0;
    virtual void visit(IdentifierExpr& expr) = 0;
    virtual void visit(GroupingExpr& expr) = 0;
    virtual void visit(ArrayExpr& expr) = 0;
    virtual void visit(ObjectExpr& expr) = 0;
    virtual void visit(IndexExpr& expr) = 0;
    virtual void visit(RangeExpr& expr) = 0;
    virtual void visit(LambdaExpr& expr) = 0;
    virtual void visit(MatchExpr& expr) = 0;
    virtual void visit(YieldExpr& expr) = 0;
    virtual void visit(AwaitExpr& expr) = 0;
    virtual void visit(SpreadExpr& expr) = 0;
    virtual void visit(ConditionalExpr& expr) = 0;
    virtual void visit(TypeAssertionExpr& expr) = 0;
    
    // Statements
    virtual void visit(BlockStmt& stmt) = 0;
    virtual void visit(ExpressionStmt& stmt) = 0;
    virtual void visit(IfStmt& stmt) = 0;
    virtual void visit(WhileStmt& stmt) = 0;
    virtual void visit(ForStmt& stmt) = 0;
    virtual void visit(ForInStmt& stmt) = 0;
    virtual void visit(ReturnStmt& stmt) = 0;
    virtual void visit(BreakStmt& stmt) = 0;
    virtual void visit(ContinueStmt& stmt) = 0;
    virtual void visit(VarDeclStmt& stmt) = 0;
    virtual void visit(FunctionDeclStmt& stmt) = 0;
    virtual void visit(ClassDeclStmt& stmt) = 0;
    virtual void visit(ImportStmt& stmt) = 0;
    virtual void visit(ExportStmt& stmt) = 0;
    virtual void visit(TryCatchStmt& stmt) = 0;
    virtual void visit(ThrowStmt& stmt) = 0;
    virtual void visit(DeferStmt& stmt) = 0;
    virtual void visit(MatchStmt& stmt) = 0;
    virtual void visit(YieldStmt& stmt) = 0;
    
    // Declarations
    virtual void visit(FunctionDecl& decl) = 0;
    virtual void visit(ClassDecl& decl) = 0;
    virtual void visit(VarDecl& decl) = 0;
    virtual void visit(ImportDecl& decl) = 0;
    virtual void visit(ExportDecl& decl) = 0;
    virtual void visit(TemplateDecl& decl) = 0;
    virtual void visit(TypeAliasDecl& decl) = 0;
    virtual void visit(EnumDecl& decl) = 0;
    
    // Types
    virtual void visit(NamedType& type) = 0;
    virtual void visit(FunctionType& type) = 0;
    virtual void visit(ArrayType& type) = 0;
    virtual void visit(MapType& type) = 0;
    virtual void visit(GenericType& type) = 0;
    virtual void visit(UnionType& type) = 0;
    virtual void visit(IntersectionType& type) = 0;
    virtual void visit(OptionalType& type) = 0;
    virtual void visit(TupleType& type) = 0;
};

template<typename T>
class GenericVisitor : public Visitor {
    static_assert(std::is_base_of_v<Node, T>, "T must inherit from Node");
    
public:
    virtual T visit(AssignExpr& expr) = 0;
    virtual T visit(BinaryExpr& expr) = 0;
    virtual T visit(UnaryExpr& expr) = 0;
    virtual T visit(CallExpr& expr) = 0;
    virtual T visit(GetExpr& expr) = 0;
    virtual T visit(SetExpr& expr) = 0;
    virtual T visit(SuperExpr& expr) = 0;
    virtual T visit(ThisExpr& expr) = 0;
    virtual T visit(LiteralExpr& expr) = 0;
    virtual T visit(IdentifierExpr& expr) = 0;
    virtual T visit(GroupingExpr& expr) = 0;
    virtual T visit(ArrayExpr& expr) = 0;
    virtual T visit(ObjectExpr& expr) = 0;
    virtual T visit(IndexExpr& expr) = 0;
    virtual T visit(RangeExpr& expr) = 0;
    virtual T visit(LambdaExpr& expr) = 0;
    virtual T visit(MatchExpr& expr) = 0;
    virtual T visit(YieldExpr& expr) = 0;
    virtual T visit(AwaitExpr& expr) = 0;
    virtual T visit(SpreadExpr& expr) = 0;
    virtual T visit(ConditionalExpr& expr) = 0;
    virtual T visit(TypeAssertionExpr& expr) = 0;
    
    virtual T visit(BlockStmt& stmt) = 0;
    virtual T visit(ExpressionStmt& stmt) = 0;
    virtual T visit(IfStmt& stmt) = 0;
    virtual T visit(WhileStmt& stmt) = 0;
    virtual T visit(ForStmt& stmt) = 0;
    virtual T visit(ForInStmt& stmt) = 0;
    virtual T visit(ReturnStmt& stmt) = 0;
    virtual T visit(BreakStmt& stmt) = 0;
    virtual T visit(ContinueStmt& stmt) = 0;
    virtual T visit(VarDeclStmt& stmt) = 0;
    virtual T visit(FunctionDeclStmt& stmt) = 0;
    virtual T visit(ClassDeclStmt& stmt) = 0;
    virtual T visit(ImportStmt& stmt) = 0;
    virtual T visit(ExportStmt& stmt) = 0;
    virtual T visit(TryCatchStmt& stmt) = 0;
    virtual T visit(ThrowStmt& stmt) = 0;
    virtual T visit(DeferStmt& stmt) = 0;
    virtual T visit(MatchStmt& stmt) = 0;
    virtual T visit(YieldStmt& stmt) = 0;
    
    virtual T visit(FunctionDecl& decl) = 0;
    virtual T visit(ClassDecl& decl) = 0;
    virtual T visit(VarDecl& decl) = 0;
    virtual T visit(ImportDecl& decl) = 0;
    virtual T visit(ExportDecl& decl) = 0;
    virtual T visit(TemplateDecl& decl) = 0;
    virtual T visit(TypeAliasDecl& decl) = 0;
    virtual T visit(EnumDecl& decl) = 0;
    
    virtual T visit(NamedType& type) = 0;
    virtual T visit(FunctionType& type) = 0;
    virtual T visit(ArrayType& type) = 0;
    virtual T visit(MapType& type) = 0;
    virtual T visit(GenericType& type) = 0;
    virtual T visit(UnionType& type) = 0;
    virtual T visit(IntersectionType& type) = 0;
    virtual T visit(OptionalType& type) = 0;
    virtual T visit(TupleType& type) = 0;
};

class RecursiveVisitor : public Visitor {
public:
    void visit(AssignExpr& expr) override;
    void visit(BinaryExpr& expr) override;
    void visit(UnaryExpr& expr) override;
    void visit(CallExpr& expr) override;
    void visit(GetExpr& expr) override;
    void visit(SetExpr& expr) override;
    void visit(SuperExpr& expr) override;
    void visit(ThisExpr& expr) override;
    void visit(LiteralExpr& expr) override;
    void visit(IdentifierExpr& expr) override;
    void visit(GroupingExpr& expr) override;
    void visit(ArrayExpr& expr) override;
    void visit(ObjectExpr& expr) override;
    void visit(IndexExpr& expr) override;
    void visit(RangeExpr& expr) override;
    void visit(LambdaExpr& expr) override;
    void visit(MatchExpr& expr) override;
    void visit(YieldExpr& expr) override;
    void visit(AwaitExpr& expr) override;
    void visit(SpreadExpr& expr) override;
    void visit(ConditionalExpr& expr) override;
    void visit(TypeAssertionExpr& expr) override;
    
    void visit(BlockStmt& stmt) override;
    void visit(ExpressionStmt& stmt) override;
    void visit(IfStmt& stmt) override;
    void visit(WhileStmt& stmt) override;
    void visit(ForStmt& stmt) override;
    void visit(ForInStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(BreakStmt& stmt) override;
    void visit(ContinueStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(FunctionDeclStmt& stmt) override;
    void visit(ClassDeclStmt& stmt) override;
    void visit(ImportStmt& stmt) override;
    void visit(ExportStmt& stmt) override;
    void visit(TryCatchStmt& stmt) override;
    void visit(ThrowStmt& stmt) override;
    void visit(DeferStmt& stmt) override;
    void visit(MatchStmt& stmt) override;
    void visit(YieldStmt& stmt) override;
    
    void visit(FunctionDecl& decl) override;
    void visit(ClassDecl& decl) override;
    void visit(VarDecl& decl) override;
    void visit(ImportDecl& decl) override;
    void visit(ExportDecl& decl) override;
    void visit(TemplateDecl& decl) override;
    void visit(TypeAliasDecl& decl) override;
    void visit(EnumDecl& decl) override;
    
    void visit(NamedType& type) override;
    void visit(FunctionType& type) override;
    void visit(ArrayType& type) override;
    void visit(MapType& type) override;
    void visit(GenericType& type) override;
    void visit(UnionType& type) override;
    void visit(IntersectionType& type) override;
    void visit(OptionalType& type) override;
    void visit(TupleType& type) override;
};

template<typename Context>
class ContextualVisitor : public Visitor {
public:
    explicit ContextualVisitor(Context& context) : context_(context) {}
    
    virtual void visitWithContext(AssignExpr& expr, Context& context) = 0;
    virtual void visitWithContext(BinaryExpr& expr, Context& context) = 0;
    virtual void visitWithContext(UnaryExpr& expr, Context& context) = 0;
    virtual void visitWithContext(CallExpr& expr, Context& context) = 0;
    virtual void visitWithContext(GetExpr& expr, Context& context) = 0;
    virtual void visitWithContext(SetExpr& expr, Context& context) = 0;
    virtual void visitWithContext(SuperExpr& expr, Context& context) = 0;
    virtual void visitWithContext(ThisExpr& expr, Context& context) = 0;
    virtual void visitWithContext(LiteralExpr& expr, Context& context) = 0;
    virtual void visitWithContext(IdentifierExpr& expr, Context& context) = 0;
    virtual void visitWithContext(GroupingExpr& expr, Context& context) = 0;
    virtual void visitWithContext(ArrayExpr& expr, Context& context) = 0;
    virtual void visitWithContext(ObjectExpr& expr, Context& context) = 0;
    virtual void visitWithContext(IndexExpr& expr, Context& context) = 0;
    virtual void visitWithContext(RangeExpr& expr, Context& context) = 0;
    virtual void visitWithContext(LambdaExpr& expr, Context& context) = 0;
    virtual void visitWithContext(MatchExpr& expr, Context& context) = 0;
    virtual void visitWithContext(YieldExpr& expr, Context& context) = 0;
    virtual void visitWithContext(AwaitExpr& expr, Context& context) = 0;
    virtual void visitWithContext(SpreadExpr& expr, Context& context) = 0;
    virtual void visitWithContext(ConditionalExpr& expr, Context& context) = 0;
    virtual void visitWithContext(TypeAssertionExpr& expr, Context& context) = 0;
    
    virtual void visitWithContext(BlockStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ExpressionStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(IfStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(WhileStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ForStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ForInStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ReturnStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(BreakStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ContinueStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(VarDeclStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(FunctionDeclStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ClassDeclStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ImportStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ExportStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(TryCatchStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(ThrowStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(DeferStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(MatchStmt& stmt, Context& context) = 0;
    virtual void visitWithContext(YieldStmt& stmt, Context& context) = 0;
    
    virtual void visitWithContext(FunctionDecl& decl, Context& context) = 0;
    virtual void visitWithContext(ClassDecl& decl, Context& context) = 0;
    virtual void visitWithContext(VarDecl& decl, Context& context) = 0;
    virtual void visitWithContext(ImportDecl& decl, Context& context) = 0;
    virtual void visitWithContext(ExportDecl& decl, Context& context) = 0;
    virtual void visitWithContext(TemplateDecl& decl, Context& context) = 0;
    virtual void visitWithContext(TypeAliasDecl& decl, Context& context) = 0;
    virtual void visitWithContext(EnumDecl& decl, Context& context) = 0;
    
    virtual void visitWithContext(NamedType& type, Context& context) = 0;
    virtual void visitWithContext(FunctionType& type, Context& context) = 0;
    virtual void visitWithContext(ArrayType& type, Context& context) = 0;
    virtual void visitWithContext(MapType& type, Context& context) = 0;
    virtual void visitWithContext(GenericType& type, Context& context) = 0;
    virtual void visitWithContext(UnionType& type, Context& context) = 0;
    virtual void visitWithContext(IntersectionType& type, Context& context) = 0;
    virtual void visitWithContext(OptionalType& type, Context& context) = 0;
    virtual void visitWithContext(TupleType& type, Context& context) = 0;
    
protected:
    Context& context_;
};

class ConstVisitor {
public:
    virtual ~ConstVisitor() = default;
    
    virtual void visit(const AssignExpr& expr) = 0;
    virtual void visit(const BinaryExpr& expr) = 0;
    virtual void visit(const UnaryExpr& expr) = 0;
    virtual void visit(const CallExpr& expr) = 0;
    virtual void visit(const GetExpr& expr) = 0;
    virtual void visit(const SetExpr& expr) = 0;
    virtual void visit(const SuperExpr& expr) = 0;
    virtual void visit(const ThisExpr& expr) = 0;
    virtual void visit(const LiteralExpr& expr) = 0;
    virtual void visit(const IdentifierExpr& expr) = 0;
    virtual void visit(const GroupingExpr& expr) = 0;
    virtual void visit(const ArrayExpr& expr) = 0;
    virtual void visit(const ObjectExpr& expr) = 0;
    virtual void visit(const IndexExpr& expr) = 0;
    virtual void visit(const RangeExpr& expr) = 0;
    virtual void visit(const LambdaExpr& expr) = 0;
    virtual void visit(const MatchExpr& expr) = 0;
    virtual void visit(const YieldExpr& expr) = 0;
    virtual void visit(const AwaitExpr& expr) = 0;
    virtual void visit(const SpreadExpr& expr) = 0;
    virtual void visit(const ConditionalExpr& expr) = 0;
    virtual void visit(const TypeAssertionExpr& expr) = 0;
    
    virtual void visit(const BlockStmt& stmt) = 0;
    virtual void visit(const ExpressionStmt& stmt) = 0;
    virtual void visit(const IfStmt& stmt) = 0;
    virtual void visit(const WhileStmt& stmt) = 0;
    virtual void visit(const ForStmt& stmt) = 0;
    virtual void visit(const ForInStmt& stmt) = 0;
    virtual void visit(const ReturnStmt& stmt) = 0;
    virtual void visit(const BreakStmt& stmt) = 0;
    virtual void visit(const ContinueStmt& stmt) = 0;
    virtual void visit(const VarDeclStmt& stmt) = 0;
    virtual void visit(const FunctionDeclStmt& stmt) = 0;
    virtual void visit(const ClassDeclStmt& stmt) = 0;
    virtual void visit(const ImportStmt& stmt) = 0;
    virtual void visit(const ExportStmt& stmt) = 0;
    virtual void visit(const TryCatchStmt& stmt) = 0;
    virtual void visit(const ThrowStmt& stmt) = 0;
    virtual void visit(const DeferStmt& stmt) = 0;
    virtual void visit(const MatchStmt& stmt) = 0;
    virtual void visit(const YieldStmt& stmt) = 0;
    
    virtual void visit(const FunctionDecl& decl) = 0;
    virtual void visit(const ClassDecl& decl) = 0;
    virtual void visit(const VarDecl& decl) = 0;
    virtual void visit(const ImportDecl& decl) = 0;
    virtual void visit(const ExportDecl& decl) = 0;
    virtual void visit(const TemplateDecl& decl) = 0;
    virtual void visit(const TypeAliasDecl& decl) = 0;
    virtual void visit(const EnumDecl& decl) = 0;
    
    virtual void visit(const NamedType& type) = 0;
    virtual void visit(const FunctionType& type) = 0;
    virtual void visit(const ArrayType& type) = 0;
    virtual void visit(const MapType& type) = 0;
    virtual void visit(const GenericType& type) = 0;
    virtual void visit(const UnionType& type) = 0;
    virtual void visit(const IntersectionType& type) = 0;
    virtual void visit(const OptionalType& type) = 0;
    virtual void visit(const TupleType& type) = 0;
};

} // namespace ast
} // namespace internal
} // namespace klang