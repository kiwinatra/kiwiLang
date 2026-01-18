
#include "kiwiLang/Compiler/Compiler.h"
#include "kiwiLang/Compiler/Driver.h"
#include "kiwiLang/Parser/Parser.h"
#include "kiwiLang/CodeGen/CodeGenerator.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

class CompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<kiwiLang::Compiler>();
        driver_ = std::make_unique<kiwiLang::Driver>();
        
        tempDir_ = fs::temp_directory_path() / "kiwilang_test";
        fs::create_directories(tempDir_);
    }
    
    void TearDown() override {
        fs::remove_all(tempDir_);
    }
    
    std::string compileString(std::string_view source) {
        std::string filename = (tempDir_ / "test.kwl").string();
        std::ofstream file(filename);
        file << source;
        file.close();
        
        kiwiLang::CompileOptions options;
        options.inputFile = filename;
        options.outputFile = (tempDir_ / "test.o").string();
        options.emitLLVMIR = true;
        
        auto result = driver_->compile(options);
        if (!result.success) {
            return "COMPILE_ERROR: " + result.errorMessage;
        }
        
        std::ifstream irFile(options.outputFile + ".ll");
        if (!irFile.is_open()) {
            return "NO_IR_OUTPUT";
        }
        
        std::string irContent((std::istreambuf_iterator<char>(irFile)),
                             std::istreambuf_iterator<char>());
        return irContent;
    }
    
    std::unique_ptr<kiwiLang::Compiler> compiler_;
    std::unique_ptr<kiwiLang::Driver> driver_;
    fs::path tempDir_;
};

TEST_F(CompilerTest, BasicArithmetic) {
    std::string source = R"(
        func main() -> int {
            return 1 + 2 * 3;
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("add"), std::string::npos);
    EXPECT_NE(result.find("mul"), std::string::npos);
}

TEST_F(CompilerTest, FunctionCalls) {
    std::string source = R"(
        func add(a: int, b: int) -> int {
            return a + b;
        }
        
        func main() -> int {
            return add(5, 3);
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("define"), std::string::npos);
    EXPECT_NE(result.find("call"), std::string::npos);
}

TEST_F(CompilerTest, ControlFlow) {
    std::string source = R"(
        func max(a: int, b: int) -> int {
            if a > b {
                return a;
            } else {
                return b;
            }
        }
        
        func main() -> int {
            return max(10, 20);
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("icmp"), std::string::npos);
    EXPECT_NE(result.find("br"), std::string::npos);
}

TEST_F(CompilerTest, Variables) {
    std::string source = R"(
        func main() -> int {
            let x = 42;
            let y = x * 2;
            return y;
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("alloca"), std::string::npos);
    EXPECT_NE(result.find("load"), std::string::npos);
    EXPECT_NE(result.find("store"), std::string::npos);
}

TEST_F(CompilerTest, Loops) {
    std::string source = R"(
        func sum(n: int) -> int {
            var total = 0;
            for var i = 0; i < n; i = i + 1 {
                total = total + i;
            }
            return total;
        }
        
        func main() -> int {
            return sum(10);
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("phi"), std::string::npos);
    EXPECT_NE(result.find("loop"), std::string::npos);
}

TEST_F(CompilerTest, Arrays) {
    std::string source = R"(
        func sumArray(arr: [int], size: int) -> int {
            var total = 0;
            for var i = 0; i < size; i = i + 1 {
                total = total + arr[i];
            }
            return total;
        }
        
        func main() -> int {
            let arr = [1, 2, 3, 4, 5];
            return sumArray(arr, 5);
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("getelementptr"), std::string::npos);
    EXPECT_NE(result.find("array"), std::string::npos);
}

TEST_F(CompilerTest, Strings) {
    std::string source = R"(
        func greet() -> str {
            return "Hello, World!";
        }
        
        func main() -> int {
            let msg = greet();
            return 0;
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("private constant"), std::string::npos);
    EXPECT_NE(result.find("i8*"), std::string::npos);
}

TEST_F(CompilerTest, Structs) {
    std::string source = R"(
        struct Point {
            x: int,
            y: int
        }
        
        func createPoint() -> Point {
            return Point { x: 10, y: 20 };
        }
        
        func main() -> int {
            let p = createPoint();
            return p.x + p.y;
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("%Point"), std::string::npos);
    EXPECT_NE(result.find("extractvalue"), std::string::npos);
}

TEST_F(CompilerTest, RecursiveFunctions) {
    std::string source = R"(
        func factorial(n: int) -> int {
            if n <= 1 {
                return 1;
            }
            return n * factorial(n - 1);
        }
        
        func main() -> int {
            return factorial(5);
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("call"), std::string::npos);
    EXPECT_NE(result.find("factorial"), std::string::npos);
}

TEST_F(CompilerTest, TypeInference) {
    std::string source = R"(
        func inferTypes() -> int {
            let x = 42;          // int
            let y = 3.14;        // float
            let z = true;        // bool
            let s = "string";    // str
            
            return x;
        }
        
        func main() -> int {
            return inferTypes();
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_NE(result.find("i32"), std::string::npos);
    EXPECT_NE(result.find("double"), std::string::npos);
    EXPECT_NE(result.find("i1"), std::string::npos);
}

TEST_F(CompilerTest, Modules) {
    std::string module1 = R"(
        // module1.kwl
        pub func add(a: int, b: int) -> int {
            return a + b;
        }
        
        pub const PI = 3.14159;
    )";
    
    std::string module2 = R"(
        // module2.kwl
        import module1;
        
        func main() -> int {
            return module1.add(5, module1.PI as int);
        }
    )";
    
    fs::path module1Path = tempDir_ / "module1.kwl";
    fs::path module2Path = tempDir_ / "module2.kwl";
    
    std::ofstream file1(module1Path);
    file1 << module1;
    file1.close();
    
    std::ofstream file2(module2Path);
    file2 << module2;
    file2.close();
    
    kiwiLang::CompileOptions options;
    options.inputFile = module2Path.string();
    options.outputFile = (tempDir_ / "module_test.o").string();
    options.emitLLVMIR = true;
    options.includePaths.push_back(tempDir_.string());
    
    auto result = driver_->compile(options);
    EXPECT_TRUE(result.success);
}

TEST_F(CompilerTest, ErrorHandling) {
    std::string source = R"(
        func test() -> int {
            let x: int = "string";  // Type mismatch
            return undefined;        // Undefined variable
        }
    )";
    
    std::string result = compileString(source);
    EXPECT_TRUE(result.find("COMPILE_ERROR") != std::string::npos ||
                result.find("error") != std::string::npos);
}

TEST_F(CompilerTest, Optimization) {
    std::string source = R"(
        func constantFold() -> int {
            return 2 + 3 * 4;  // Should fold to 14
        }
        
        func deadCode() -> int {
            let x = 42;
            let y = x * 2;     // Dead if not used
            return x;          // y is dead
        }
        
        func main() -> int {
            return constantFold() + deadCode();
        }
    )";
    
    kiwiLang::CompileOptions options;
    options.inputFile = (tempDir_ / "opt_test.kwl").string();
    options.outputFile = (tempDir_ / "opt_test.o").string();
    options.emitLLVMIR = true;
    options.optimizationLevel = kiwiLang::OptimizationLevel::O2;
    
    std::ofstream file(options.inputFile);
    file << source;
    file.close();
    
    auto result = driver_->compile(options);
    EXPECT_TRUE(result.success);
    
    std::ifstream irFile(options.outputFile + ".ll");
    std::string irContent((std::istreambuf_iterator<char>(irFile)),
                         std::istreambuf_iterator<char>());
    
    // Check for constant folding
    EXPECT_NE(irContent.find("14"), std::string::npos);
}

TEST_F(CompilerTest, CodeGeneration) {
    std::string source = R"(
        func fibonacci(n: int) -> int {
            if n <= 1 {
                return n;
            }
            return fibonacci(n - 1) + fibonacci(n - 2);
        }
        
        func main() -> int {
            return fibonacci(10);
        }
    )";
    
    kiwiLang::CompileOptions options;
    options.inputFile = (tempDir_ / "fib.kwl").string();
    options.outputFile = (tempDir_ / "fib").string();
    options.outputType = kiwiLang::OutputType::EXECUTABLE;
    
    std::ofstream file(options.inputFile);
    file << source;
    file.close();
    
    auto result = driver_->compile(options);
    EXPECT_TRUE(result.success);
    
    // Check if executable was created
    EXPECT_TRUE(fs::exists(options.outputFile));
    
    // On Unix-like systems, check if it's executable
    #ifndef _WIN32
    EXPECT_TRUE((fs::status(options.outputFile).permissions() & 
                 fs::perms::owner_exec) != fs::perms::none);
    #endif
}

TEST_F(CompilerTest, MemoryManagement) {
    std::string source = R"(
        func createArray(size: int) -> [int] {
            return new [int](size);
        }
        
        func useArray(arr: [int], size: int) -> int {
            var sum = 0;
            for var i = 0; i < size; i = i + 1 {
                sum = sum + arr[i];
            }
            return sum;
        }
        
        func main() -> int {
            let arr = createArray(100);
            for var i = 0; i < 100; i = i + 1 {
                arr[i] = i;
            }
            return useArray(arr, 100);
        }
    )";
    
    std::string result = compileString(source);
    
    // Check for memory allocation
    EXPECT_NE(result.find("malloc"), std::string::npos);
    EXPECT_NE(result.find("free"), std::string::npos);
}

TEST_F(CompilerTest, Concurrency) {
    std::string source = R"(
        import std.thread;
        
        func worker(id: int) -> int {
            return id * 2;
        }
        
        func main() -> int {
            let t1 = thread.spawn(|| worker(1));
            let t2 = thread.spawn(|| worker(2));
            
            let r1 = t1.join();
            let r2 = t2.join();
            
            return r1 + r2;
        }
    )";
    
    std::string result = compileString(source);
    
    // Check for thread-related constructs
    EXPECT_NE(result.find("pthread"), std::string::npos) << 
        "Thread support not detected in IR";
}

TEST_F(CompilerTest, StandardLibrary) {
    std::string source = R"(
        import std.io;
        
        func main() -> int {
            io.println("Testing standard library");
            return 0;
        }
    )";
    
    kiwiLang::CompileOptions options;
    options.inputFile = (tempDir_ / "stdlib_test.kwl").string();
    options.outputFile = (tempDir_ / "stdlib_test").string();
    options.outputType = kiwiLang::OutputType::EXECUTABLE;
    options.linkStdlib = true;
    
    std::ofstream file(options.inputFile);
    file << source;
    file.close();
    
    auto result = driver_->compile(options);
    EXPECT_TRUE(result.success);
    
    EXPECT_TRUE(fs::exists(options.outputFile));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize compiler
    kiwiLang::Compiler::initialize();
    
    return RUN_ALL_TESTS();
}