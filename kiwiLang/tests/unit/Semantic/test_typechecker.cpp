#include "kiwiLang/Semantic/TypeChecker.h"
#include "kiwiLang/Semantic/SymbolTable.h"
#include "kiwiLang/Parser/Parser.h"
#include "kiwiLang/Lexer/Lexer.h"
#include "kiwiLang/Diagnostics/Diagnostic.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal;

class TypeCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        diagnostics_ = std::make_shared<DiagnosticReporter>();
        typeChecker_ = std::make_unique<TypeChecker>(diagnostics_);
        symbolTable_ = std::make_unique<SymbolTable>();
    }
    
    std::shared_ptr<DiagnosticReporter> diagnostics_;
    std::unique_ptr<TypeChecker> typeChecker_;
    std::unique_ptr<SymbolTable> symbolTable_;
};

TEST_F(TypeCheckerTest, BasicTypeInference) {
    const char* source = R"(
        func test() -> int {
            let x = 42;
            let y = 3.14;
            let z = true;
            let s = "hello";
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, TypeMismatchError) {
    const char* source = R"(
        func test() -> int {
            let x: int = "string";
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, FunctionReturnType) {
    const char* source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
        
        func test() -> int {
            return add(5, 3);
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, InvalidReturnType) {
    const char* source = R"(
        func test() -> int {
            return "not an int";
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, VariableScope) {
    const char* source = R"(
        func outer() -> int {
            let x = 42;
            
            if true {
                let y = x + 1;
                return y;
            }
            
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, UndefinedVariable) {
    const char* source = R"(
        func test() -> int {
            return undefinedVar;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, BinaryOperationTypes) {
    const char* source = R"(
        func test() -> int {
            let a = 5 + 3;
            let b = 10 - 2;
            let c = 3 * 4;
            let d = 8 / 2;
            let e = 7 % 3;
            return a;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, BinaryOperationTypeMismatch) {
    const char* source = R"(
        func test() -> int {
            let x = 5 + "string";
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, ConditionalExpression) {
    const char* source = R"(
        func test() -> int {
            let x = true ? 42 : 24;
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, ConditionalTypeMismatch) {
    const char* source = R"(
        func test() -> int {
            let x = true ? 42 : "string";
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, ArrayTypeChecking) {
    const char* source = R"(
        func test() -> int {
            let arr: [int] = [1, 2, 3, 4, 5];
            return arr[0];
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, ArrayTypeMismatch) {
    const char* source = R"(
        func test() -> int {
            let arr: [int] = [1, "two", 3];
            return arr[0];
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, StructTypeChecking) {
    const char* source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func test() -> int {
            let p = Point { x: 10, y: 20 };
            return p.x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, StructFieldTypeMismatch) {
    const char* source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func test() -> int {
            let p = Point { x: "ten", y: 20 };
            return p.x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, GenericFunction) {
    const char* source = R"(
        func identity<T>(value: T) -> T {
            return value;
        }
        
        func test() -> int {
            return identity<int>(42);
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, GenericTypeInference) {
    const char* source = R"(
        func identity<T>(value: T) -> T {
            return value;
        }
        
        func test() -> int {
            return identity(42); // Should infer T = int
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, RecursiveType) {
    const char* source = R"(
        struct Node {
            value: int,
            next: Node?
        }
        
        func test() -> int {
            let node = Node { value: 1, next: nil };
            return node.value;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, TypeAlias) {
    const char* source = R"(
        type MyInt = int;
        
        func test() -> MyInt {
            let x: MyInt = 42;
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, UnionTypes) {
    const char* source = R"(
        func test() -> int | string {
            if true {
                return 42;
            } else {
                return "string";
            }
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, FunctionOverloading) {
    const char* source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
        
        func add(a: string, b: string) -> string {
            return a + b;
        }
        
        func test() -> int {
            return add(5, 3);
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, AmbiguousOverload) {
    const char* source = R"(
        func process(x: int) -> int {
            return x;
        }
        
        func process(x: float) -> float {
            return x;
        }
        
        func test() -> int {
            return process(42); // Should be ambiguous
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    // This should either resolve to int version or error
    // Depends on language rules
}

TEST_F(TypeCheckerTest, ConstCorrectness) {
    const char* source = R"(
        func test() -> int {
            const x = 42;
            x = 50; // Should error
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, NullableTypes) {
    const char* source = R"(
        func test() -> int? {
            return nil;
        }
        
        func test2() -> int {
            let x: int? = 42;
            return x!; // Force unwrap
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, InvalidUnwrap) {
    const char* source = R"(
        func test() -> int {
            let x: int? = nil;
            return x; // Should require unwrap
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, PatternMatchingTypes) {
    const char* source = R"(
        func test(x: int | string) -> int {
            match x {
                case int i => return i;
                case string s => return s.length();
            }
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, LambdaTypeInference) {
    const char* source = R"(
        func test() -> int {
            let add = (a: int, b: int) -> int {
                return a + b;
            };
            return add(5, 3);
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, CaptureTypeChecking) {
    const char* source = R"(
        func test() -> int {
            let x = 42;
            let getX = () -> int {
                return x;
            };
            return getX();
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, TypeConversion) {
    const char* source = R"(
        func test() -> int {
            let x = 3.14 as int;
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(TypeCheckerTest, InvalidTypeConversion) {
    const char* source = R"(
        func test() -> int {
            let x = "string" as int;
            return x;
        }
    )";
    
    Lexer lexer(diagnostics_);
    auto tokens = lexer.lex(source, "test.kwl");
    Parser parser(std::move(tokens), diagnostics_);
    auto ast = parser.parseProgram();
    
    ASSERT_NE(ast, nullptr);
    
    typeChecker_->checkProgram(ast.get());
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}