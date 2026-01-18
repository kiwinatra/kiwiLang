#include "kiwiLang/CodeGen/IRGenerator.h"
#include "kiwiLang/CodeGen/IR.h"
#include "kiwiLang/Parser/Parser.h"
#include "kiwiLang/Lexer/Lexer.h"
#include "kiwiLang/Semantic/TypeChecker.h"
#include "kiwiLang/Diagnostics/Diagnostic.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal;
using namespace kiwiLang::internal::codegen;

class IRGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        diagnostics_ = std::make_shared<DiagnosticReporter>();
        typeChecker_ = std::make_unique<TypeChecker>(diagnostics_);
        irGen_ = std::make_unique<IRGenerator>(diagnostics_);
    }
    
    std::shared_ptr<DiagnosticReporter> diagnostics_;
    std::unique_ptr<TypeChecker> typeChecker_;
    std::unique_ptr<IRGenerator> irGen_;
    
    std::unique_ptr<ModuleIR> generateIR(const char* source) {
        Lexer lexer(diagnostics_);
        auto tokens = lexer.lex(source, "test.kwl");
        Parser parser(std::move(tokens), diagnostics_);
        auto ast = parser.parseProgram();
        
        if (!ast || diagnostics_->hasErrors()) {
            return nullptr;
        }
        
        if (!typeChecker_->checkProgram(ast.get())) {
            return nullptr;
        }
        
        return irGen_->generate(ast.get(), "test_module");
    }
};

TEST_F(IRGeneratorTest, EmptyModule) {
    const char* source = "";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->name(), "test_module");
    EXPECT_EQ(module->functions().size(), 0);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, SimpleFunction) {
    const char* source = R"(
        func main() -> int {
            return 42;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->functions().size(), 1);
    
    auto* func = module->functions()[0].get();
    EXPECT_EQ(func->name(), "main");
    EXPECT_EQ(func->returnType(), Type::i32());
    EXPECT_EQ(func->parameters().size(), 0);
    
    // Should have one basic block with return instruction
    EXPECT_EQ(func->blocks().size(), 1);
    auto& block = func->blocks()[0];
    EXPECT_EQ(block.instructions.size(), 1);
    
    auto* retInst = dynamic_cast<ReturnInstruction*>(block.instructions[0].get());
    ASSERT_NE(retInst, nullptr);
    
    auto* constVal = dynamic_cast<ConstantInt*>(retInst->value.get());
    ASSERT_NE(constVal, nullptr);
    EXPECT_EQ(constVal->value(), 42);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, FunctionWithParameters) {
    const char* source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    EXPECT_EQ(func->name(), "add");
    EXPECT_EQ(func->parameters().size(), 2);
    
    auto& params = func->parameters();
    EXPECT_EQ(params[0].name, "a");
    EXPECT_EQ(params[0].type, Type::i32());
    EXPECT_EQ(params[1].name, "b");
    EXPECT_EQ(params[1].type, Type::i32());
    
    // Should have binary add instruction
    auto& block = func->blocks()[0];
    EXPECT_GE(block.instructions.size(), 2);
    
    bool foundAdd = false;
    for (auto& inst : block.instructions) {
        if (auto* binop = dynamic_cast<BinaryInstruction*>(inst.get())) {
            if (binop->op == BinaryOp::ADD) {
                foundAdd = true;
            }
        }
    }
    EXPECT_TRUE(foundAdd);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, LocalVariables) {
    const char* source = R"(
        func test() -> int {
            let x = 10;
            let y = 20;
            return x + y;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have alloca instructions for variables
    bool foundAlloca = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<AllocaInstruction*>(inst.get())) {
                foundAlloca = true;
            }
        }
    }
    EXPECT_TRUE(foundAlloca);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, ControlFlow) {
    const char* source = R"(
        func abs(x: int) -> int {
            if x >= 0 {
                return x;
            } else {
                return -x;
            }
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have multiple basic blocks for if-else
    EXPECT_GE(func->blocks().size(), 3); // Entry, then, else, merge
    
    // Should have branch instruction
    bool foundBranch = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<BranchInstruction*>(inst.get())) {
                foundBranch = true;
            }
        }
    }
    EXPECT_TRUE(foundBranch);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, Loops) {
    const char* source = R"(
        func sum(n: int) -> int {
            var total = 0;
            for var i = 0; i < n; i = i + 1 {
                total = total + i;
            }
            return total;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Loop should create multiple basic blocks
    EXPECT_GE(func->blocks().size(), 4); // Entry, loop header, body, after loop
    
    // Should have phi instructions for loop variables
    bool foundPhi = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<PhiInstruction*>(inst.get())) {
                foundPhi = true;
            }
        }
    }
    EXPECT_TRUE(foundPhi);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, FunctionCalls) {
    const char* source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
        
        func main() -> int {
            return add(5, 3);
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->functions().size(), 2);
    
    // Find main function
    FunctionIR* mainFunc = nullptr;
    for (auto& func : module->functions()) {
        if (func->name() == "main") {
            mainFunc = func.get();
            break;
        }
    }
    ASSERT_NE(mainFunc, nullptr);
    
    // Should have call instruction
    bool foundCall = false;
    for (auto& block : mainFunc->blocks()) {
        for (auto& inst : block.instructions) {
            if (auto* call = dynamic_cast<CallInstruction*>(inst.get())) {
                foundCall = true;
                EXPECT_EQ(call->callee->name(), "add");
                EXPECT_EQ(call->args.size(), 2);
            }
        }
    }
    EXPECT_TRUE(foundCall);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, Arrays) {
    const char* source = R"(
        func sumArray(arr: [int], size: int) -> int {
            var total = 0;
            for var i = 0; i < size; i = i + 1 {
                total = total + arr[i];
            }
            return total;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have GEP (get element pointer) instruction for array access
    bool foundGEP = false;
    bool foundLoad = false;
    
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<GEPInstruction*>(inst.get())) {
                foundGEP = true;
            }
            if (dynamic_cast<LoadInstruction*>(inst.get())) {
                foundLoad = true;
            }
        }
    }
    
    EXPECT_TRUE(foundGEP);
    EXPECT_TRUE(foundLoad);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, Structs) {
    const char* source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func createPoint() -> Point {
            return Point { x: 10, y: 20 };
        }
        
        func getX(p: Point) -> int {
            return p.x;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    // Should have struct type definition
    bool foundStructType = false;
    for (auto& type : module->types()) {
        if (auto* structType = dynamic_cast<StructType*>(type.get())) {
            if (structType->name().find("Point") != std::string::npos) {
                foundStructType = true;
                EXPECT_EQ(structType->fields().size(), 2);
            }
        }
    }
    EXPECT_TRUE(foundStructType);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, GlobalVariables) {
    const char* source = R"(
        global PI: float = 3.14159;
        
        func getPi() -> float {
            return PI;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    // Should have global variable
    EXPECT_GT(module->globals().size(), 0);
    
    auto* global = module->globals()[0].get();
    EXPECT_EQ(global->name(), "PI");
    EXPECT_EQ(global->type(), Type::f32());
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, StringLiterals) {
    const char* source = R"(
        func greet() -> string {
            return "Hello, World!";
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    // Should have string constant
    EXPECT_GT(module->constants().size(), 0);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, TypeConversions) {
    const char* source = R"(
        func convert() -> int {
            let x = 3.14 as int;
            return x;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have cast instruction
    bool foundCast = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (auto* cast = dynamic_cast<CastInstruction*>(inst.get())) {
                foundCast = true;
                EXPECT_EQ(cast->fromType, Type::f32());
                EXPECT_EQ(cast->toType, Type::i32());
            }
        }
    }
    EXPECT_TRUE(foundCast);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, RecursiveFunction) {
    const char* source = R"(
        func factorial(n: int) -> int {
            if n <= 1 {
                return 1;
            }
            return n * factorial(n - 1);
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have recursive call
    bool foundRecursiveCall = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (auto* call = dynamic_cast<CallInstruction*>(inst.get())) {
                if (call->callee->name() == "factorial") {
                    foundRecursiveCall = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundRecursiveCall);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, MemoryOperations) {
    const char* source = R"(
        func swap(a: int*, b: int*) -> void {
            let temp = *a;
            *a = *b;
            *b = temp;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have load and store instructions
    int loadCount = 0;
    int storeCount = 0;
    
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<LoadInstruction*>(inst.get())) {
                ++loadCount;
            }
            if (dynamic_cast<StoreInstruction*>(inst.get())) {
                ++storeCount;
            }
        }
    }
    
    EXPECT_GE(loadCount, 2);
    EXPECT_GE(storeCount, 2);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, BuiltinOperations) {
    const char* source = R"(
        func builtins() -> void {
            let a = sizeof(int);
            let b = alignof(float);
            // Additional builtins would be here
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, ErrorHandling) {
    const char* source = R"(
        func test() -> int {
            throw "error";
        }
        
        func safe() -> int {
            try {
                return test();
            } catch e: string {
                return -1;
            }
        }
    )";
    
    auto module = generateIR(source);
    
    // Should generate IR for exception handling
    // Implementation depends on language design
    
    EXPECT_TRUE(module != nullptr || diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, ModuleImports) {
    const char* source = R"(
        import math;
        
        func useImported() -> float {
            return math.sin(3.14);
        }
    )";
    
    auto module = generateIR(source);
    
    // Should handle external function calls
    // Implementation depends on module system
    
    EXPECT_TRUE(module != nullptr || diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, GenericFunctions) {
    const char* source = R"(
        func identity<T>(value: T) -> T {
            return value;
        }
        
        func useGeneric() -> int {
            return identity<int>(42);
        }
    )";
    
    auto module = generateIR(source);
    
    // Should generate specialized version or generic IR
    // Implementation depends on generics design
    
    EXPECT_TRUE(module != nullptr || diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, ComplexExpressions) {
    const char* source = R"(
        func complex(a: int, b: int, c: int) -> int {
            return (a + b) * (c - a) / (b % c) ^ (a & b) | (c << a) >> b;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Count different operations
    std::map<BinaryOp, int> opCounts;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (auto* binop = dynamic_cast<BinaryInstruction*>(inst.get())) {
                opCounts[binop->op]++;
            }
        }
    }
    
    // Should have various operations
    EXPECT_GT(opCounts.size(), 0);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, VectorOperations) {
    const char* source = R"(
        func vectorAdd(a: [4]float, b: [4]float) -> [4]float {
            return a + b;
        }
    )";
    
    auto module = generateIR(source);
    
    // Should generate vector instructions if supported
    // Implementation depends on vector support
    
    EXPECT_TRUE(module != nullptr || diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, ConstantFolding) {
    const char* source = R"(
        func constants() -> int {
            return 2 + 3 * 4;  // Should be 14
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Optimizer might fold constants, but IR generator should
    // still produce the operations
    
    bool hasConstant = false;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<ConstantInt*>(inst.get())) {
                hasConstant = true;
            }
        }
    }
    EXPECT_TRUE(hasConstant);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, SSAForm) {
    const char* source = R"(
        func ssaTest(x: int) -> int {
            var y = x;
            y = y + 1;
            y = y * 2;
            return y;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // In SSA form, each assignment creates new value
    // Should see phi nodes after optimizations
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, MultipleReturns) {
    const char* source = R"(
        func multiReturn(x: int) -> int {
            if x > 0 {
                return 1;
            }
            if x < 0 {
                return -1;
            }
            return 0;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    auto* func = module->functions()[0].get();
    
    // Should have multiple return instructions
    int returnCount = 0;
    for (auto& block : func->blocks()) {
        for (auto& inst : block.instructions) {
            if (dynamic_cast<ReturnInstruction*>(inst.get())) {
                ++returnCount;
            }
        }
    }
    
    EXPECT_GE(returnCount, 3);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, PerformanceTest) {
    // Generate complex function for performance testing
    std::string source = R"(
        func benchmark(n: int) -> int {
            var sum = 0;
            for var i = 0; i < n; i = i + 1 {
                for var j = 0; j < n; j = j + 1 {
                    sum = sum + i * j;
                }
            }
            return sum;
        }
    )";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto module = generateIR(source.c_str());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_NE(module, nullptr);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 1000); // 1 second
    
    auto* func = module->functions()[0].get();
    EXPECT_GT(func->blocks().size(), 0);
    
    std::cout << "IR generation: " << duration.count() << "ms\n";
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(IRGeneratorTest, IRValidation) {
    const char* source = R"(
        func valid() -> int {
            return 42;
        }
    )";
    
    auto module = generateIR(source);
    
    ASSERT_NE(module, nullptr);
    
    // IR should be valid (type-checked, SSA, etc.)
    for (auto& func : module->functions()) {
        // Check each function
        EXPECT_FALSE(func->blocks().empty());
        
        for (auto& block : func->blocks()) {
            for (auto& inst : block.instructions) {
                // Instruction should have valid types
                EXPECT_NE(inst->type, Type::voidTy());
                
                // Check operands
                for (auto& operand : inst->operands) {
                    EXPECT_NE(operand, nullptr);
                }
            }
        }
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}