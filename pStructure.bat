@echo off
setlocal enabledelayedexpansion

echo Creating kiwiLang (kiwiLang) project structure...
echo.

set ROOT_DIR=kiwiLang
if not exist "%ROOT_DIR%" mkdir "%ROOT_DIR%"
cd "%ROOT_DIR%"

echo Creating root files...
type nul > README.md
type nul > LICENSE
type nul > SECURITY.md
type nul > CONTRIBUTING.md
type nul > CODE_OF_CONDUCT.md
type nul > .gitignore
type nul > .clang-format
type nul > .clang-tidy
type nul > CMakeLists.txt
type nul > CMakePresets.json

mkdir .github
cd .github
type nul > FUNDING.yml
mkdir workflows
cd workflows
type nul > ci.yml
type nul > release.yml
type nul > tests.yml
cd ..\..
mkdir bench
cd bench
type nul > CMakeLists.txt
type nul > microbenchmarks.cpp
mkdir realworld
cd realworld
type nul > primes.kiwi
type nul > mandelbrot.kiwi
cd ..\..
mkdir cmake
cd cmake
type nul > FindLLVM.cmake
type nul > FindReadline.cmake
type nul > CompilerWarnings.cmake
cd ..
mkdir docs
cd docs
mkdir spec
cd spec
type nul > lexical.md
type nul > grammar.md
type nul > typesystem.md
type nul > memorymodel.md
type nul > abi.md
cd ..
mkdir internals
cd internals
type nul > compiler_pipeline.md
type nul > vm_design.md
type nul > garbage_collection.md
cd ..
mkdir api
cd api
type nul > standard_lib.md
type nul > ffi.md
cd ..
mkdir tutorials
cd tutorials
type nul > getting_started.md
type nul > ownership_model.md
type nul > concurrency.md
cd ..
mkdir images
cd images
type nul > architecture.png
cd ..\..
mkdir examples
cd examples
mkdir hello_world
cd hello_world
type nul > hello.kiwi
type nul > CMakeLists.txt
cd ..
mkdir web_server
cd web_server
type nul > server.kiwi
type nul > router.kiwi
type nul > middleware.kiwi
cd ..
mkdir game_of_life
cd game_of_life
type nul > life.kiwi
mkdir patterns
cd patterns
type nul > README.md
cd ..\..
mkdir benchmarks
cd benchmarks
type nul > nbody.kiwi
type nul > ray_tracer.kiwi
cd ..\..
mkdir include
cd include
mkdir kiwiLang
cd kiwiLang
mkdir public
cd public
type nul > Compiler.h
type nul > Runtime.h
type nul > JIT.h
type nul > Config.h
cd ..
mkdir internal
cd internal
mkdir Lexer
cd Lexer
type nul > Token.h
type nul > Scanner.h
type nul > Lexer.h
cd ..
mkdir Parser
cd Parser
type nul > Parser.h
type nul > Grammar.h
type nul > ASTVisitor.h
cd ..
mkdir AST
cd AST
type nul > AST.h
type nul > Node.h
type nul > Type.h
type nul > Statement.h
type nul > Expression.h
cd ..
mkdir Semantic
cd Semantic
type nul > Analyzer.h
type nul > SymbolTable.h
type nul > TypeChecker.h
cd ..
mkdir CodeGen
cd CodeGen
type nul > IRGenerator.h
type nul > Optimizer.h
type nul > LLVMBackend.h
type nul > JITCompiler.h
cd ..
mkdir Runtime
cd Runtime
type nul > VM.h
type nul > Memory.h
type nul > GarbageCollector.h
type nul > Object.h
type nul > String.h
type nul > Array.h
type nul > HashTable.h
cd ..
mkdir Utils
cd Utils
type nul > ArenaAllocator.h
type nul > StringPool.h
type nul > Diagnostics.h
type nul > FileManager.h
type nul > SourceLocation.h
cd ..\..\..\..
mkdir src
cd src
mkdir Lexer
cd Lexer
type nul > Scanner.cpp
type nul > Lexer.cpp
cd ..
mkdir Parser
cd Parser
type nul > Parser.cpp
type nul > Grammar.cpp
cd ..
mkdir AST
cd AST
type nul > AST.cpp
type nul > Node.cpp
type nul > Type.cpp
cd ..
mkdir Semantic
cd Semantic
type nul > Analyzer.cpp
type nul > SymbolTable.cpp
type nul > TypeChecker.cpp
cd ..
mkdir CodeGen
cd CodeGen
type nul > IRGenerator.cpp
type nul > Optimizer.cpp
type nul > LLVMBackend.cpp
type nul > JITCompiler.cpp
cd ..
mkdir Runtime
cd Runtime
type nul > VM.cpp
type nul > Memory.cpp
type nul > GarbageCollector.cpp
type nul > Object.cpp
type nul > String.cpp
type nul > Array.cpp
type nul > HashTable.cpp
cd ..
mkdir Utils
cd Utils
type nul > ArenaAllocator.cpp
type nul > StringPool.cpp
type nul > Diagnostics.cpp
type nul > FileManager.cpp
cd ..
type nul > main.cpp
type nul > Driver.cpp
cd ..
mkdir lib
cd lib
mkdir kiwiLangrt
cd kiwiLangrt
type nul > CMakeLists.txt
mkdir src
cd src
type nul > runtime.cpp
cd ..\..\..
mkdir tests
cd tests
mkdir unit
cd unit
mkdir Lexer
cd Lexer
type nul > test_scanner.cpp
type nul > test_lexer.cpp
cd ..
mkdir Parser
cd Parser
type nul > test_parser.cpp
cd ..
mkdir Semantic
cd Semantic
type nul > test_typechecker.cpp
cd ..
mkdir CodeGen
cd CodeGen
type nul > test_irgen.cpp
cd ..
mkdir Runtime
cd Runtime
type nul > test_vm.cpp
type nul > test_gc.cpp
cd ..\..\..
mkdir integration
cd integration
type nul > test_compiler.cpp
type nul > test_stdlib.cpp
cd ..
type nul > CMakeLists.txt
type nul > test_main.cpp
cd ..
mkdir third_party
cd third_party
type nul > README.md
mkdir catch2
cd catch2
type nul > catch.hpp
cd ..\..
mkdir tools
cd tools
type nul > kiwiLang-format.cpp
type nul > kiwiLang-lint.cpp
mkdir scripts
cd scripts
type nul > generate_parser.py
type nul > update_docs.py
type nul > benchmark.py
cd ..\..
mkdir build
type nul > build\.gitkeep
mkdir .vscode
cd .vscode
type nul > tasks.json
type nul > launch.json
type nul > c_cpp_properties.json
cd ..
mkdir .idea
cd .idea
type nul > .gitignore
cd ..

echo.
echo Structure created successfully in %CD%
echo.
pause