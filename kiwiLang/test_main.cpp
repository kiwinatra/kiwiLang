#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <kiwiLang/internal/Lexer/Token.h>
#include <kiwiLang/internal/Lexer/Scanner.h>
#include <kiwiLang/internal/Lexer/Lexer.h>
#include <kiwiLang/internal/Parser/Parser.h>
#include <kiwiLang/internal/Parser/Grammar.h>
#include <kiwiLang/internal/AST/AST.h>
#include <kiwiLang/internal/AST/Node.h>
#include <kiwiLang/internal/AST/Type.h>
#include <kiwiLang/internal/AST/Statement.h>
#include <kiwiLang/internal/AST/Expression.h>
#include <kiwiLang/internal/Semantic/Analyzer.h>
#include <kiwiLang/internal/Semantic/SymbolTable.h>
#include <kiwiLang/internal/Semantic/TypeChecker.h>
#include <kiwiLang/internal/CodeGen/IRGenerator.h>
#include <kiwiLang/internal/CodeGen/Optimizer.h>
#include <kiwiLang/internal/CodeGen/LLVMBackend.h>
#include <kiwiLang/internal/CodeGen/JITCompiler.h>
#include <kiwiLang/internal/Runtime/VM.h>
#include <kiwiLang/internal/Runtime/Memory.h>
#include <kiwiLang/internal/Runtime/GarbageCollector.h>
#include <kiwiLang/internal/Runtime/Object.h>
#include <kiwiLang/internal/Runtime/String.h>
#include <kiwiLang/internal/Runtime/Array.h>
#include <kiwiLang/internal/Runtime/HashTable.h>
#include <kiwiLang/internal/Utils/ArenaAllocator.h>
#include <kiwiLang/internal/Utils/StringPool.h>
#include <kiwiLang/internal/Utils/Diagnostics.h>
#include <kiwiLang/internal/Utils/FileManager.h>
#include <kiwiLang/internal/Utils/SourceLocation.h>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

using namespace kiwiLang;

TEST_CASE("Token basics", "[lexer][token]") {
    Token token;
    token.type = TokenType::IDENTIFIER;
    token.lexeme = "test";
    token.line = 1;
    token.column = 1;
    token.offset = 0;
    
    REQUIRE(token.is(TokenType::IDENTIFIER));
    REQUIRE(token.isOneOf(TokenType::IDENTIFIER, TokenType::NUMBER));
    REQUIRE_FALSE(token.is(TokenType::NUMBER));
}

TEST_CASE("Scanner basic input", "[lexer][scanner]") {
    std::string source = "fn main() { return 42; }";
    Scanner scanner(source);
    
    auto tokens = scanner.scan();
    
    REQUIRE(tokens.size() > 0);
    REQUIRE(tokens[0].type == TokenType::FN);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].lexeme == "main");
}

TEST_CASE("Lexer token stream", "[lexer]") {
    std::string source = "let x = 10 + 20;";
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Lexer lexer(std::move(scanner), diag);
    
    auto tokens = lexer.lex();
    
    REQUIRE(tokens.size() >= 7);
    REQUIRE(tokens[0].type == TokenType::LET);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[3].type == TokenType::NUMBER);
    REQUIRE(tokens[5].type == TokenType::PLUS);
}

TEST_CASE("Parser expression parsing", "[parser]") {
    std::string source = "1 + 2 * 3";
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Parser parser(std::move(scanner), diag);
    
    auto expr = parser.parseExpression();
    
    REQUIRE(expr != nullptr);
    REQUIRE(expr->kind() == ASTNodeKind::BinaryExpr);
}

TEST_CASE("AST node creation", "[ast]") {
    auto literal = std::make_unique<LiteralExpr>();
    literal->value = "42";
    
    REQUIRE(literal->kind() == ASTNodeKind::LiteralExpr);
    REQUIRE(literal->value == "42");
}

TEST_CASE("Type representation", "[ast][type]") {
    auto intType = std::make_unique<PrimitiveType>("i32");
    
    REQUIRE(intType->kind() == TypeKind::Primitive);
    REQUIRE(intType->name() == "i32");
    REQUIRE(intType->isNumeric());
}

TEST_CASE("Symbol table scoping", "[semantic]") {
    SymbolTable table;
    
    auto symbol = std::make_unique<Symbol>();
    symbol->name = "x";
    symbol->type = std::make_unique<PrimitiveType>("i32");
    symbol->scope = 0;
    
    table.insert("x", std::move(symbol));
    
    auto found = table.lookup("x");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "x");
}

TEST_CASE("Type checker basic", "[semantic][typechecker]") {
    Diagnostics diag;
    TypeChecker checker(diag);
    
    auto intType = std::make_unique<PrimitiveType>("i32");
    auto boolType = std::make_unique<PrimitiveType>("bool");
    
    REQUIRE(checker.isAssignable(*intType, *intType));
    REQUIRE_FALSE(checker.isAssignable(*intType, *boolType));
}

TEST_CASE("Arena allocator", "[utils][memory]") {
    ArenaAllocator arena(4096);
    
    void* ptr1 = arena.allocate(64);
    void* ptr2 = arena.allocate(128);
    
    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(ptr2 > ptr1);
}

TEST_CASE("String pool interning", "[utils][strings]") {
    StringPool pool;
    
    const char* str1 = pool.intern("hello");
    const char* str2 = pool.intern("hello");
    const char* str3 = pool.intern("world");
    
    REQUIRE(str1 == str2);
    REQUIRE(str1 != str3);
    REQUIRE(std::string(str1) == "hello");
}

TEST_CASE("Diagnostics error reporting", "[utils][diagnostics]") {
    Diagnostics diag;
    std::stringstream output;
    diag.setOutputStream(output);
    
    diag.error(1, 5, "Test error");
    
    REQUIRE(output.str().find("error") != std::string::npos);
    REQUIRE(output.str().find("Test error") != std::string::npos);
}

TEST_CASE("Runtime object representation", "[runtime]") {
    Object obj;
    obj.type = ObjectType::INTEGER;
    obj.as.integer = 42;
    
    REQUIRE(obj.type == ObjectType::INTEGER);
    REQUIRE(obj.as.integer == 42);
}

TEST_CASE("String object operations", "[runtime][strings]") {
    String str;
    str.init("Hello, World!");
    
    REQUIRE(str.length() == 13);
    REQUIRE(str.at(0) == 'H');
    REQUIRE(str.compare("Hello, World!") == 0);
}

TEST_CASE("Array basic operations", "[runtime][arrays]") {
    Array arr;
    arr.init(10);
    
    REQUIRE(arr.capacity() >= 10);
    REQUIRE(arr.size() == 0);
    
    arr.push(Object::makeInteger(42));
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.at(0).as.integer == 42);
}

TEST_CASE("Hash table insertion and lookup", "[runtime][hashtable]") {
    HashTable table;
    table.init(16);
    
    auto key = Object::makeString("key");
    auto value = Object::makeInteger(123);
    
    table.insert(key, value);
    
    auto found = table.lookup(key);
    REQUIRE(found != nullptr);
    REQUIRE(found->as.integer == 123);
}

TEST_CASE("Memory allocator", "[runtime][memory]") {
    MemoryManager memory;
    memory.init(1024 * 1024);
    
    void* block = memory.allocate(256);
    REQUIRE(block != nullptr);
    
    memory.deallocate(block);
}

TEST_CASE("Garbage collector basics", "[runtime][gc]") {
    GarbageCollector gc;
    gc.init(1024 * 1024);
    
    Object* obj = gc.allocateObject(ObjectType::STRING);
    REQUIRE(obj != nullptr);
    REQUIRE(obj->type == ObjectType::STRING);
    
    gc.mark(obj);
    gc.sweep();
}

TEST_CASE("Virtual machine execution", "[runtime][vm]") {
    VirtualMachine vm;
    vm.init(1024 * 1024);
    
    std::vector<uint8_t> bytecode = {
        0x01, 0x00, 0x00, 0x00,
        0x02, 0x2A, 0x00, 0x00
    };
    
    vm.loadBytecode(bytecode.data(), bytecode.size());
    int result = vm.execute();
    
    REQUIRE(result == 0);
}

TEST_CASE("IR generation basic", "[codegen][ir]") {
    IRGenerator generator;
    
    auto module = std::make_unique<AST::Module>();
    
    auto func = std::make_unique<AST::FunctionDecl>();
    func->name = "test";
    func->returnType = std::make_unique<PrimitiveType>("i32");
    
    auto returnStmt = std::make_unique<AST::ReturnStmt>();
    auto literal = std::make_unique<LiteralExpr>();
    literal->value = "42";
    returnStmt->value = std::move(literal);
    
    auto block = std::make_unique<AST::BlockStmt>();
    block->statements.push_back(std::move(returnStmt));
    func->body = std::move(block);
    
    module->functions.push_back(std::move(func));
    
    auto ir = generator.generate(*module);
    REQUIRE(ir != nullptr);
}

TEST_CASE("Optimizer constant folding", "[codegen][optimizer]") {
    Optimizer optimizer;
    
    auto binary = std::make_unique<BinaryExpr>();
    binary->op.type = TokenType::PLUS;
    
    auto left = std::make_unique<LiteralExpr>();
    left->value = "10";
    
    auto right = std::make_unique<LiteralExpr>();
    right->value = "20";
    
    binary->left = std::move(left);
    binary->right = std::move(right);
    
    auto optimized = optimizer.constantFold(std::move(binary));
    REQUIRE(optimized != nullptr);
    
    auto literal = dynamic_cast<LiteralExpr*>(optimized.get());
    REQUIRE(literal != nullptr);
    REQUIRE(literal->value == "30");
}

TEST_CASE("Integration: Lexer -> Parser -> AST", "[integration]") {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            return x + y;
        }
    )";
    
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Parser parser(std::move(scanner), diag);
    
    auto module = parser.parseModule();
    
    REQUIRE(module != nullptr);
    REQUIRE(module->functions.size() == 1);
    REQUIRE(module->functions[0]->name == "add");
    REQUIRE(module->functions[0]->parameters.size() == 2);
}

TEST_CASE("File manager source loading", "[utils][files]") {
    FileManager fm;
    
    auto tempFile = std::tmpfile();
    std::fputs("test content", tempFile);
    std::rewind(tempFile);
    
    char tempName[L_tmpnam];
    std::tmpnam(tempName);
    
    std::FILE* file = std::fopen(tempName, "w");
    std::fputs("test content", file);
    std::fclose(file);
    
    std::string content = fm.readFile(tempName);
    REQUIRE(content == "test content");
    
    std::remove(tempName);
}

TEST_CASE("Source location tracking", "[utils][location]") {
    SourceLocation loc;
    loc.file = "test.kiwi";
    loc.line = 10;
    loc.column = 5;
    loc.offset = 100;
    
    REQUIRE(loc.line == 10);
    REQUIRE(loc.column == 5);
    REQUIRE(loc.toString() == "test.kiwi:10:5");
}

TEST_CASE("Complex expression type checking", "[semantic]") {
    Diagnostics diag;
    TypeChecker checker(diag);
    
    auto intType = std::make_unique<PrimitiveType>("i32");
    auto floatType = std::make_unique<PrimitiveType>("f64");
    
    auto binary = std::make_unique<BinaryExpr>();
    binary->op.type = TokenType::PLUS;
    
    auto left = std::make_unique<IdentifierExpr>();
    left->name = "x";
    left->type = std::make_unique<PrimitiveType>("i32");
    
    auto right = std::make_unique<IdentifierExpr>();
    right->name = "y";
    right->type = std::make_unique<PrimitiveType>("i32");
    
    binary->left = std::move(left);
    binary->right = std::move(right);
    
    auto result = checker.checkExpression(*binary);
    REQUIRE(result != nullptr);
    REQUIRE(result->kind() == TypeKind::Primitive);
}

TEST_CASE("Code generation pipeline", "[codegen][integration]") {
    auto module = std::make_unique<AST::Module>();
    
    auto func = std::make_unique<AST::FunctionDecl>();
    func->name = "main";
    func->returnType = std::make_unique<PrimitiveType>("i32");
    
    auto returnStmt = std::make_unique<AST::ReturnStmt>();
    auto literal = std::make_unique<LiteralExpr>();
    literal->value = "0";
    returnStmt->value = std::move(literal);
    
    auto block = std::make_unique<AST::BlockStmt>();
    block->statements.push_back(std::move(returnStmt));
    func->body = std::move(block);
    
    module->functions.push_back(std::move(func));
    
    IRGenerator irGen;
    auto ir = irGen.generate(*module);
    
    REQUIRE(ir != nullptr);
    
#ifdef kiwiLang_USE_LLVM
    LLVMBackend backend;
    auto llvmModule = backend.generate(*ir);
    REQUIRE(llvmModule != nullptr);
#endif
}

TEST_CASE("Runtime object lifecycle", "[runtime]") {
    MemoryManager memory;
    memory.init(1024 * 1024);
    
    Object* obj = memory.allocateObject(ObjectType::STRING);
    REQUIRE(obj != nullptr);
    
    String* str = reinterpret_cast<String*>(obj);
    str->init("test");
    
    REQUIRE(str->length() == 4);
    REQUIRE(str->compare("test") == 0);
    
    memory.freeObject(obj);
}

TEST_CASE("Benchmark: Lexer performance", "[!benchmark]") {
    std::string source;
    for (int i = 0; i < 10000; ++i) {
        source += "let x" + std::to_string(i) + " = " + std::to_string(i) + ";\n";
    }
    
    BENCHMARK("Lex 10000 lines") {
        Scanner scanner(source);
        return scanner.scan();
    };
}

TEST_CASE("Benchmark: Parser performance", "[!benchmark]") {
    std::string source = "fn test() { return 42; }";
    
    BENCHMARK("Parse simple function") {
        auto scanner = std::make_unique<Scanner>(source);
        Diagnostics diag;
        Parser parser(std::move(scanner), diag);
        return parser.parseModule();
    };
}

TEST_CASE("Exception safety", "[safety]") {
    REQUIRE_NOTHROW([]{
        ArenaAllocator arena(1024);
        arena.allocate(512);
    }());
    
    REQUIRE_THROWS([]{
        ArenaAllocator arena(128);
        arena.allocate(256);
    }());
}

TEST_CASE("Memory safety", "[safety][memory]") {
    MemoryManager memory;
    memory.init(4096);
    
    void* ptr = memory.allocate(1024);
    REQUIRE(ptr != nullptr);
    
    memory.deallocate(ptr);
    
    REQUIRE_NOTHROW([&]{
        memory.validate();
    }());
}

TEST_CASE("Thread safety stubs", "[concurrency]") {
    REQUIRE(std::is_destructible<SymbolTable>::value);
    REQUIRE(std::is_copy_constructible<Diagnostics>::value);
    REQUIRE(std::is_move_constructible<ArenaAllocator>::value);
}

TEST_CASE("Platform compatibility", "[platform]") {
    REQUIRE(sizeof(Object) <= 16);
    REQUIRE(alignof(Object) >= 8);
    
    REQUIRE(sizeof(String) >= sizeof(Object));
    REQUIRE(sizeof(Array) >= sizeof(Object));
    
    REQUIRE(std::is_trivially_destructible<Token>::value);
}

int main(int argc, char* argv[]) {
    std::cout << "kiwiLang Test Suite" << std::endl;
    std::cout << "=================" << std::endl;
    
    int result = Catch::Session().run(argc, argv);
    
    return result;
}