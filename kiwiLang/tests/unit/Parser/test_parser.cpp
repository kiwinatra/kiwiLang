#include "kiwiLang/Parser/Parser.h"
#include "kiwiLang/Lexer/Lexer.h"
#include "kiwiLang/Diagnostics/Diagnostic.h"
#include "kiwiLang/AST/ASTDump.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        diagnostics_ = std::make_shared<DiagnosticReporter>();
    }
    
    std::shared_ptr<DiagnosticReporter> diagnostics_;
    
    std::unique_ptr<ast::Program> parse(const char* source) {
        Lexer lexer(diagnostics_);
        auto tokens = lexer.lex(source, "test.kwl");
        Parser parser(std::move(tokens), diagnostics_);
        return parser.parseProgram();
    }
};

TEST_F(ParserTest, EmptyProgram) {
    const char* source = "";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
    EXPECT_EQ(ast->declarations.size(), 0);
}

TEST_F(ParserTest, FunctionDeclaration) {
    const char* source = R"(
        func hello() -> void {
            // Empty function
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
    EXPECT_EQ(ast->declarations.size(), 1);
    
    auto* func = dynamic_cast<ast::FunctionDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "hello");
    EXPECT_TRUE(func->returnType->isVoid());
    EXPECT_EQ(func->parameters.size(), 0);
}

TEST_F(ParserTest, FunctionWithParameters) {
    const char* source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
    
    auto* func = dynamic_cast<ast::FunctionDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->parameters.size(), 2);
    EXPECT_EQ(func->parameters[0]->name, "a");
    EXPECT_EQ(func->parameters[1]->name, "b");
}

TEST_F(ParserTest, VariableDeclaration) {
    const char* source = R"(
        func test() -> void {
            let x = 42;
            var y: int = 10;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ReturnStatement) {
    const char* source = R"(
        func answer() -> int {
            return 42;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, IfStatement) {
    const char* source = R"(
        func test(x: int) -> int {
            if x > 0 {
                return 1;
            } else {
                return 0;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, WhileLoop) {
    const char* source = R"(
        func count(n: int) -> void {
            var i = 0;
            while i < n {
                i = i + 1;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ForLoop) {
    const char* source = R"(
        func sum(n: int) -> int {
            var total = 0;
            for var i = 0; i < n; i = i + 1 {
                total = total + i;
            }
            return total;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, BinaryExpressions) {
    const char* source = R"(
        func compute() -> int {
            return 1 + 2 * 3 - 4 / 2;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ParenthesizedExpression) {
    const char* source = R"(
        func test() -> int {
            return (1 + 2) * 3;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, UnaryOperators) {
    const char* source = R"(
        func test() -> int {
            return !true;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ComparisonOperators) {
    const char* source = R"(
        func compare(a: int, b: int) -> bool {
            return a == b && a != b && a < b && a > b && a <= b && a >= b;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ArrayLiteral) {
    const char* source = R"(
        func getArray() -> [int] {
            return [1, 2, 3, 4, 5];
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ArrayIndexing) {
    const char* source = R"(
        func getElement(arr: [int], idx: int) -> int {
            return arr[idx];
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, StructDeclaration) {
    const char* source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func createPoint() -> Point {
            return Point { x: 10, y: 20 };
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, MemberAccess) {
    const char* source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func getX(p: Point) -> int {
            return p.x;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, LambdaExpression) {
    const char* source = R"(
        func test() -> int {
            let add = (a: int, b: int) -> int {
                return a + b;
            };
            return add(5, 3);
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ImportDeclaration) {
    const char* source = R"(
        import std.io;
        import math as m;
        
        func main() -> void {
            // Use imported modules
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, TypeAlias) {
    const char* source = R"(
        type MyInt = int;
        type Callback = (int) -> void;
        
        func useTypes() -> MyInt {
            return 42;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, GenericFunction) {
    const char* source = R"(
        func identity<T>(value: T) -> T {
            return value;
        }
        
        func test() -> int {
            return identity<int>(42);
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, MatchExpression) {
    const char* source = R"(
        func describe(x: int | string) -> string {
            match x {
                case int i => return "integer: " + i as string;
                case string s => return "string: " + s;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, TryCatchBlock) {
    const char* source = R"(
        func risky() -> int throws {
            throw "error";
        }
        
        func safe() -> int {
            try {
                return risky();
            } catch e: string {
                return -1;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, NullableTypes) {
    const char* source = R"(
        func maybeInt() -> int? {
            return nil;
        }
        
        func unwrap(x: int?) -> int {
            return x!;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, OptionalChaining) {
    const char* source = R"(
        struct User {
            name: string,
            address: Address?
        }
        
        struct Address {
            city: string
        }
        
        func getCity(user: User?) -> string? {
            return user?.address?.city;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, SpreadOperator) {
    const char* source = R"(
        func combine(arr1: [int], arr2: [int]) -> [int] {
            return [...arr1, ...arr2];
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, RangeExpression) {
    const char* source = R"(
        func getRange() -> [int] {
            return [1..10];
        }
        
        func getStepRange() -> [int] {
            return [0..100 step 2];
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, DeferStatement) {
    const char* source = R"(
        func withCleanup() -> void {
            defer {
                // cleanup code
            }
            // main logic
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, AsyncAwait) {
    const char* source = R"(
        async func fetchData() -> string {
            let data = await http.get("https://example.com");
            return data;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, YieldExpression) {
    const char* source = R"(
        generator func counter(max: int) -> int {
            for var i = 0; i < max; i = i + 1 {
                yield i;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ComplexExpressionParsing) {
    const char* source = R"(
        func complex() -> int {
            return (a + b) * (c - d) / (e % f) ^ (g & h) | (i << j) >> k;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, NestedStructures) {
    const char* source = R"(
        func process(data: [[int]]) -> [[int]] {
            var result: [[int]] = [];
            for arr in data {
                if arr.length > 0 {
                    result.push(arr.map(x => x * 2));
                }
            }
            return result;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ErrorRecovery) {
    const char* source = R"(
        func missingParen() -> int {
            return (1 + 2;  // Missing closing paren
            
            let x = 42;
            return x;
        }
    )";
    
    auto ast = parse(source);
    
    // Should recover and continue parsing
    EXPECT_NE(ast, nullptr);
    // Might have errors, but shouldn't crash
}

TEST_F(ParserTest, InvalidSyntax) {
    const char* source = R"(
        func invalid() -> int {
            return 1 + ;  // Missing operand
        }
    )";
    
    auto ast = parse(source);
    
    // Should report error
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, OperatorPrecedence) {
    const char* source = R"(
        func precedence() -> int {
            // a + b * c should parse as a + (b * c)
            return 1 + 2 * 3;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
    
    // Verify AST structure
    auto* func = dynamic_cast<ast::FunctionDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    
    auto* returnStmt = dynamic_cast<ast::ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NE(returnStmt, nullptr);
    
    auto* binaryExpr = dynamic_cast<ast::BinaryExpr*>(returnStmt->expression.get());
    ASSERT_NE(binaryExpr, nullptr);
    EXPECT_EQ(binaryExpr->op, TokenType::PLUS);
}

TEST_F(ParserTest, CommentsIgnored) {
    const char* source = R"(
        // Single line comment
        func test() -> int {
            /*
             * Multi-line comment
             */
            return 42; // End of line comment
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, StringLiterals) {
    const char* source = R"(
        func strings() -> string {
            return "Hello, World!\n\t\"Escaped\"";
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, TemplateStrings) {
    const char* source = R"(
        func greeting(name: string) -> string {
            return `Hello, ${name}!`;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, CharacterLiterals) {
    const char* source = R"(
        func chars() -> char {
            return 'A';
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, NumberLiterals) {
    const char* source = R"(
        func numbers() -> void {
            let decimal = 42;
            let hex = 0x2A;
            let binary = 0b101010;
            let octal = 0o52;
            let float = 3.14;
            let scientific = 6.022e23;
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, ModuleDeclaration) {
    const char* source = R"(
        module math {
            pub func add(a: int, b: int) -> int {
                return a + b;
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ParserTest, InterfaceDeclaration) {
    const char* source = R"(
        interface Printable {
            func toString() -> string;
        }
        
        struct Point implements Printable {
            x: int,
            y: int
            
            func toString() -> string {
                return "Point(" + x as string + ", " + y as string + ")";
            }
        }
    )";
    
    auto ast = parse(source);
    
    ASSERT_NE(ast, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}