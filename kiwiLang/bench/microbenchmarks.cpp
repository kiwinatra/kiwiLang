#include <benchmark/benchmark.h>
#include <kiwiLang/internal/Lexer/Scanner.h>
#include <kiwiLang/internal/Lexer/Lexer.h>
#include <kiwiLang/internal/Parser/Parser.h>
#include <kiwiLang/internal/AST/AST.h>
#include <kiwiLang/internal/Semantic/Analyzer.h>
#include <kiwiLang/internal/CodeGen/IRGenerator.h>
#include <kiwiLang/internal/Runtime/VM.h>
#include <kiwiLang/internal/Runtime/Memory.h>
#include <kiwiLang/internal/Utils/ArenaAllocator.h>
#include <kiwiLang/internal/Utils/StringPool.h>

#include <random>
#include <string>
#include <vector>
#include <memory>

using namespace kiwiLang;

static std::string generateSourceCode(int size) {
    std::string code;
    code.reserve(size * 100);
    
    code += "fn main() -> i32 {\n";
    code += "    let mut sum = 0;\n";
    
    for (int i = 0; i < size; ++i) {
        code += "    sum = sum + " + std::to_string(i) + ";\n";
    }
    
    code += "    return sum;\n";
    code += "}\n";
    
    return code;
}

static std::string generateLargeSourceCode() {
    std::string code;
    code.reserve(10000);
    
    code += "module benchmark;\n\n";
    
    for (int i = 0; i < 100; ++i) {
        code += "fn func" + std::to_string(i) + "(x: i32, y: i32) -> i32 {\n";
        code += "    let mut result = 0;\n";
        for (int j = 0; j < 10; ++j) {
            code += "    result = result + x * y + " + std::to_string(j) + ";\n";
        }
        code += "    return result;\n";
        code += "}\n\n";
    }
    
    code += "fn main() -> i32 {\n";
    code += "    let mut total = 0;\n";
    for (int i = 0; i < 50; ++i) {
        code += "    total = total + func" + std::to_string(i % 100) + "(" + 
                std::to_string(i) + ", " + std::to_string(i * 2) + ");\n";
    }
    code += "    return total;\n";
    code += "}\n";
    
    return code;
}

static void BM_Scanner(benchmark::State& state) {
    std::string source = generateSourceCode(state.range(0));
    
    for (auto _ : state) {
        Scanner scanner(source);
        auto tokens = scanner.scan();
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetComplexityN(state.range(0));
}

static void BM_Lexer(benchmark::State& state) {
    std::string source = generateSourceCode(state.range(0));
    
    for (auto _ : state) {
        auto scanner = std::make_unique<Scanner>(source);
        Diagnostics diag;
        Lexer lexer(std::move(scanner), diag);
        auto tokens = lexer.lex();
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetComplexityN(state.range(0));
}

static void BM_Parser(benchmark::State& state) {
    std::string source = generateSourceCode(state.range(0));
    
    for (auto _ : state) {
        auto scanner = std::make_unique<Scanner>(source);
        Diagnostics diag;
        Parser parser(std::move(scanner), diag);
        auto module = parser.parseModule();
        benchmark::DoNotOptimize(module);
    }
    
    state.SetComplexityN(state.range(0));
}

static void BM_SemanticAnalysis(benchmark::State& state) {
    std::string source = generateSourceCode(state.range(0));
    
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Parser parser(std::move(scanner), diag);
    auto module = parser.parseModule();
    
    for (auto _ : state) {
        SemanticAnalyzer analyzer(diag);
        analyzer.analyze(*module);
    }
    
    state.SetComplexityN(state.range(0));
}

static void BM_IRGeneration(benchmark::State& state) {
    std::string source = generateSourceCode(state.range(0));
    
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Parser parser(std::move(scanner), diag);
    auto module = parser.parseModule();
    
    SemanticAnalyzer analyzer(diag);
    analyzer.analyze(*module);
    
    for (auto _ : state) {
        IRGenerator generator;
        auto ir = generator.generate(*module);
        benchmark::DoNotOptimize(ir);
    }
    
    state.SetComplexityN(state.range(0));
}

static void BM_ArenaAllocator(benchmark::State& state) {
    const size_t allocationSize = state.range(0);
    const int numAllocations = 1000;
    
    for (auto _ : state) {
        ArenaAllocator arena(allocationSize * numAllocations);
        
        for (int i = 0; i < numAllocations; ++i) {
            void* ptr = arena.allocate(allocationSize);
            benchmark::DoNotOptimize(ptr);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * numAllocations * allocationSize);
}

static void BM_StringPool(benchmark::State& state) {
    const int numStrings = state.range(0);
    std::vector<std::string> strings;
    strings.reserve(numStrings);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lengthDist(5, 50);
    std::uniform_int_distribution<> charDist('a', 'z');
    
    for (int i = 0; i < numStrings; ++i) {
        int len = lengthDist(gen);
        std::string str;
        str.reserve(len);
        for (int j = 0; j < len; ++j) {
            str.push_back(static_cast<char>(charDist(gen)));
        }
        strings.push_back(str);
    }
    
    for (auto _ : state) {
        StringPool pool;
        for (const auto& str : strings) {
            const char* interned = pool.intern(str.c_str());
            benchmark::DoNotOptimize(interned);
        }
    }
    
    state.SetComplexityN(numStrings);
}

static void BM_MemoryAllocation(benchmark::State& state) {
    const size_t blockSize = state.range(0);
    const int numBlocks = 100;
    
    for (auto _ : state) {
        MemoryManager memory;
        memory.init(blockSize * numBlocks * 2);
        
        std::vector<void*> blocks;
        blocks.reserve(numBlocks);
        
        for (int i = 0; i < numBlocks; ++i) {
            void* block = memory.allocate(blockSize);
            blocks.push_back(block);
            benchmark::DoNotOptimize(block);
        }
        
        for (void* block : blocks) {
            memory.deallocate(block);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * numBlocks * blockSize);
}

static void BM_GarbageCollector(benchmark::State& state) {
    const int numObjects = state.range(0);
    
    for (auto _ : state) {
        GarbageCollector gc;
        gc.init(1024 * 1024);
        
        std::vector<Object*> objects;
        objects.reserve(numObjects);
        
        for (int i = 0; i < numObjects; ++i) {
            Object* obj = gc.allocateObject(ObjectType::INTEGER);
            obj->as.integer = i;
            objects.push_back(obj);
            benchmark::DoNotOptimize(obj);
        }
        
        for (Object* obj : objects) {
            gc.mark(obj);
        }
        
        gc.sweep();
    }
    
    state.SetComplexityN(numObjects);
}

static void BM_VMExecution(benchmark::State& state) {
    VirtualMachine vm;
    vm.init(1024 * 1024);
    
    std::vector<uint8_t> bytecode = {
        0x01, 0x00, 0x00, 0x00,
        0x02, 0x2A, 0x00, 0x00,
        0x03, 0x01, 0x00, 0x00,
        0x04
    };
    
    vm.loadBytecode(bytecode.data(), bytecode.size());
    
    for (auto _ : state) {
        int result = vm.execute();
        benchmark::DoNotOptimize(result);
    }
}

static void BM_HashTableInsert(benchmark::State& state) {
    const int numEntries = state.range(0);
    
    for (auto _ : state) {
        HashTable table;
        table.init(numEntries * 2);
        
        for (int i = 0; i < numEntries; ++i) {
            auto key = Object::makeString(("key_" + std::to_string(i)).c_str());
            auto value = Object::makeInteger(i);
            table.insert(key, value);
        }
        
        benchmark::DoNotOptimize(table);
    }
    
    state.SetComplexityN(numEntries);
}

static void BM_HashTableLookup(benchmark::State& state) {
    const int numEntries = state.range(0);
    
    HashTable table;
    table.init(numEntries * 2);
    
    std::vector<Object> keys;
    keys.reserve(numEntries);
    
    for (int i = 0; i < numEntries; ++i) {
        auto key = Object::makeString(("key_" + std::to_string(i)).c_str());
        auto value = Object::makeInteger(i);
        table.insert(key, value);
        keys.push_back(key);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, numEntries - 1);
    
    for (auto _ : state) {
        int index = dist(gen);
        auto* found = table.lookup(keys[index]);
        benchmark::DoNotOptimize(found);
    }
    
    state.SetComplexityN(numEntries);
}

static void BM_ArrayOperations(benchmark::State& state) {
    const int numElements = state.range(0);
    
    for (auto _ : state) {
        Array arr;
        arr.init(numElements);
        
        for (int i = 0; i < numElements; ++i) {
            arr.push(Object::makeInteger(i));
        }
        
        int sum = 0;
        for (int i = 0; i < numElements; ++i) {
            sum += arr.at(i).as.integer;
        }
        
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(arr);
    }
    
    state.SetComplexityN(numElements);
}

static void BM_StringOperations(benchmark::State& state) {
    const int length = state.range(0);
    
    std::string baseString;
    baseString.reserve(length);
    for (int i = 0; i < length; ++i) {
        baseString.push_back('a' + (i % 26));
    }
    
    for (auto _ : state) {
        String str;
        str.init(baseString.c_str());
        
        int hash = str.hash();
        size_t len = str.length();
        char first = str.at(0);
        char last = str.at(len - 1);
        
        benchmark::DoNotOptimize(hash);
        benchmark::DoNotOptimize(len);
        benchmark::DoNotOptimize(first);
        benchmark::DoNotOptimize(last);
    }
    
    state.SetComplexityN(length);
}

static void BM_ComplexCompilationPipeline(benchmark::State& state) {
    std::string source = generateLargeSourceCode();
    
    for (auto _ : state) {
        auto scanner = std::make_unique<Scanner>(source);
        Diagnostics diag;
        
        Lexer lexer(std::move(scanner), diag);
        auto tokens = lexer.lex();
        benchmark::DoNotOptimize(tokens);
        
        Parser parser(lexer.moveScanner(), diag);
        auto module = parser.parseModule();
        benchmark::DoNotOptimize(module);
        
        SemanticAnalyzer analyzer(diag);
        analyzer.analyze(*module);
        
        IRGenerator generator;
        auto ir = generator.generate(*module);
        benchmark::DoNotOptimize(ir);
    }
}

static void BM_ObjectCreation(benchmark::State& state) {
    const int numObjects = state.range(0);
    
    MemoryManager memory;
    memory.init(1024 * 1024);
    
    for (auto _ : state) {
        std::vector<Object*> objects;
        objects.reserve(numObjects);
        
        for (int i = 0; i < numObjects; ++i) {
            ObjectType type = static_cast<ObjectType>(i % 5);
            Object* obj = memory.allocateObject(type);
            
            switch (type) {
                case ObjectType::INTEGER:
                    obj->as.integer = i;
                    break;
                case ObjectType::FLOAT:
                    obj->as.floating = static_cast<double>(i);
                    break;
                case ObjectType::BOOLEAN:
                    obj->as.boolean = (i % 2) == 0;
                    break;
                default:
                    break;
            }
            
            objects.push_back(obj);
            benchmark::DoNotOptimize(obj);
        }
        
        for (Object* obj : objects) {
            memory.freeObject(obj);
        }
    }
    
    state.SetComplexityN(numObjects);
}

static void BM_TypeChecking(benchmark::State& state) {
    std::string source = R"(
        fn complexTypeCheck(x: i32, y: f64, z: bool) -> f64 {
            let a = if z { x } else { 0 };
            let b = a * 2;
            let c = b + y;
            let d = c / 3.14;
            return d;
        }
    )";
    
    auto scanner = std::make_unique<Scanner>(source);
    Diagnostics diag;
    Parser parser(std::move(scanner), diag);
    auto module = parser.parseModule();
    
    for (auto _ : state) {
        SemanticAnalyzer analyzer(diag);
        analyzer.analyze(*module);
    }
}

BENCHMARK(BM_Scanner)->Range(10, 1000)->Complexity();
BENCHMARK(BM_Lexer)->Range(10, 1000)->Complexity();
BENCHMARK(BM_Parser)->Range(10, 1000)->Complexity();
BENCHMARK(BM_SemanticAnalysis)->Range(10, 1000)->Complexity();
BENCHMARK(BM_IRGeneration)->Range(10, 1000)->Complexity();
BENCHMARK(BM_ArenaAllocator)->Range(8, 8<<10)->Complexity();
BENCHMARK(BM_StringPool)->Range(100, 10000)->Complexity();
BENCHMARK(BM_MemoryAllocation)->Range(16, 8<<10)->Complexity();
BENCHMARK(BM_GarbageCollector)->Range(100, 10000)->Complexity();
BENCHMARK(BM_VMExecution);
BENCHMARK(BM_HashTableInsert)->Range(100, 10000)->Complexity();
BENCHMARK(BM_HashTableLookup)->Range(100, 10000)->Complexity();
BENCHMARK(BM_ArrayOperations)->Range(100, 10000)->Complexity();
BENCHMARK(BM_StringOperations)->Range(10, 1000)->Complexity();
BENCHMARK(BM_ComplexCompilationPipeline);
BENCHMARK(BM_ObjectCreation)->Range(100, 10000)->Complexity();
BENCHMARK(BM_TypeChecking);

BENCHMARK_MAIN();