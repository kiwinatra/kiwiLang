cmake_minimum_required(VERSION 3.21)
project(kiwiLang
    VERSION 1.0.0
    DESCRIPTION "The Kiwi Programming Language"
    HOMEPAGE_URL "https://github.com/kiwiLanglang/kiwiLang"
    LANGUAGES CXX C
)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option(kiwiLang_ENABLE_TESTS "Enable building tests" ON)
option(kiwiLang_ENABLE_EXAMPLES "Enable building examples" ON)
option(kiwiLang_ENABLE_BENCHMARKS "Enable building benchmarks" OFF)
option(kiwiLang_ENABLE_JIT "Enable JIT compilation support" ON)
option(kiwiLang_USE_LLVM "Use LLVM for code generation" ON)
option(kiwiLang_ENABLE_SANITIZERS "Enable sanitizers (ASAN, UBSAN)" OFF)
option(kiwiLang_ENABLE_ASSERTIONS "Enable runtime assertions" ON)
option(kiwiLang_ENABLE_LOGGING "Enable debug logging" OFF)
option(kiwiLang_ENABLE_COVERAGE "Enable code coverage" OFF)
option(kiwiLang_BUILD_SHARED "Build shared library" OFF)
option(kiwiLang_BUILD_STATIC "Build static library" ON)
option(kiwiLang_INSTALL "Generate install target" ON)

set(kiwiLang_CXX_STANDARD 20 CACHE STRING "C++ standard to use")
set(kiwiLang_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors")
set(kiwiLang_MAX_WARNINGS ON CACHE BOOL "Enable maximum warnings")

include(CheckCXXCompilerFlag)
include(CMakeDependentOption)

if(kiwiLang_ENABLE_TESTS)
    find_package(Catch2 3.0 REQUIRED)
endif()

if(kiwiLang_USE_LLVM)
    find_package(LLVM 15.0 REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVM libs: ${LLVM_LIBRARY_DIRS}")
    message(STATUS "Using LLVM includes: ${LLVM_INCLUDE_DIRS}")
    
    llvm_map_components_to_libnames(LLVM_LIBS
        core
        support
        analysis
        irreader
        passes
        executionengine
        orcjit
        target
        x86codegen
    )
endif()

if(kiwiLang_ENABLE_JIT AND NOT kiwiLang_USE_LLVM)
    message(WARNING "JIT requires LLVM. Disabling JIT support.")
    set(kiwiLang_ENABLE_JIT OFF)
endif()

set(CMAKE_CXX_STANDARD ${kiwiLang_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

function(k_lang_set_target_warnings target)
    if(kiwiLang_MAX_WARNINGS)
        target_compile_options(${target} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
                -Wall
                -Wextra
                -Wpedantic
                -Wshadow
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Wcast-align
                -Wunused
                -Woverloaded-virtual
                -Wconversion
                -Wsign-conversion
                -Wnull-dereference
                -Wdouble-promotion
                -Wformat=2
            >
            $<$<CXX_COMPILER_ID:MSVC>:
                /W4
                /permissive-
                /w14242
                /w14287
                /w14296
                /w14311
                /w14545
                /w14546
                /w14547
                /w14549
                /w14555
                /w14619
                /w14640
                /w14826
            >
        )
        
        if(kiwiLang_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
                    -Werror
                >
                $<$<CXX_COMPILER_ID:MSVC>:
                    /WX
                >
            )
        endif()
    endif()
endfunction()

function(k_lang_set_target_sanitizers target)
    if(kiwiLang_ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -fno-optimize-sibling-calls
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()

function(k_lang_set_target_coverage target)
    if(kiwiLang_ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(${target} PRIVATE
                -fprofile-arcs
                -ftest-coverage
            )
            target_link_options(${target} PRIVATE
                -fprofile-arcs
                -ftest-coverage
            )
        endif()
    endif()
endfunction()

add_library(kiwiLang_utils STATIC
    src/Utils/ArenaAllocator.cpp
    src/Utils/StringPool.cpp
    src/Utils/Diagnostics.cpp
    src/Utils/FileManager.cpp
)

target_include_directories(kiwiLang_utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_utils)
k_lang_set_target_sanitizers(kiwiLang_utils)

add_library(kiwiLang_lexer STATIC
    src/Lexer/Scanner.cpp
    src/Lexer/Lexer.cpp
)

target_link_libraries(kiwiLang_lexer PUBLIC kiwiLang_utils)
target_include_directories(kiwiLang_lexer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_lexer)
k_lang_set_target_sanitizers(kiwiLang_lexer)

add_library(kiwiLang_ast STATIC
    src/AST/AST.cpp
    src/AST/Node.cpp
    src/AST/Type.cpp
)

target_link_libraries(kiwiLang_ast PUBLIC kiwiLang_utils)
target_include_directories(kiwiLang_ast PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_ast)
k_lang_set_target_sanitizers(kiwiLang_ast)

add_library(kiwiLang_parser STATIC
    src/Parser/Parser.cpp
    src/Parser/Grammar.cpp
)

target_link_libraries(kiwiLang_parser PUBLIC kiwiLang_lexer kiwiLang_ast)
target_include_directories(kiwiLang_parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_parser)
k_lang_set_target_sanitizers(kiwiLang_parser)

add_library(kiwiLang_semantic STATIC
    src/Semantic/Analyzer.cpp
    src/Semantic/SymbolTable.cpp
    src/Semantic/TypeChecker.cpp
)

target_link_libraries(kiwiLang_semantic PUBLIC kiwiLang_parser)
target_include_directories(kiwiLang_semantic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_semantic)
k_lang_set_target_sanitizers(kiwiLang_semantic)

add_library(kiwiLang_codegen STATIC
    src/CodeGen/IRGenerator.cpp
    src/CodeGen/Optimizer.cpp
)

if(kiwiLang_USE_LLVM)
    target_sources(kiwiLang_codegen PRIVATE
        src/CodeGen/LLVMBackend.cpp
    )
    
    if(kiwiLang_ENABLE_JIT)
        target_sources(kiwiLang_codegen PRIVATE
            src/CodeGen/JITCompiler.cpp
        )
    endif()
    
    target_include_directories(kiwiLang_codegen PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(kiwiLang_codegen PRIVATE ${LLVM_LIBS})
endif()

target_link_libraries(kiwiLang_codegen PUBLIC kiwiLang_semantic)
target_include_directories(kiwiLang_codegen PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_codegen)
k_lang_set_target_sanitizers(kiwiLang_codegen)

add_library(kiwiLang_runtime STATIC
    src/Runtime/VM.cpp
    src/Runtime/Memory.cpp
    src/Runtime/GarbageCollector.cpp
    src/Runtime/Object.cpp
    src/Runtime/String.cpp
    src/Runtime/Array.cpp
    src/Runtime/HashTable.cpp
)

target_link_libraries(kiwiLang_runtime PUBLIC kiwiLang_utils)
target_include_directories(kiwiLang_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

k_lang_set_target_warnings(kiwiLang_runtime)
k_lang_set_target_sanitizers(kiwiLang_runtime)

add_executable(kiwiLang
    src/main.cpp
    src/Driver.cpp
)

target_link_libraries(kiwiLang
    PRIVATE
        kiwiLang_codegen
        kiwiLang_runtime
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_link_options(kiwiLang PRIVATE
        -rdynamic
    )
endif()

k_lang_set_target_warnings(kiwiLang)
k_lang_set_target_sanitizers(kiwiLang)
k_lang_set_target_coverage(kiwiLang)

if(kiwiLang_BUILD_SHARED)
    add_library(kiwiLang_shared SHARED $<TARGET_OBJECTS:kiwiLang_codegen>)
    target_link_libraries(kiwiLang_shared PUBLIC kiwiLang_runtime)
    set_target_properties(kiwiLang_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "kiwiLang"
    )
    
    if(WIN32)
        target_compile_definitions(kiwiLang_shared PRIVATE kiwiLang_DLL_EXPORT)
    endif()
endif()

if(kiwiLang_BUILD_STATIC)
    add_library(kiwiLang_static STATIC $<TARGET_OBJECTS:kiwiLang_codegen>)
    target_link_libraries(kiwiLang_static PUBLIC kiwiLang_runtime)
    set_target_properties(kiwiLang_static PROPERTIES
        OUTPUT_NAME "kiwiLang"
    )
endif()

add_library(kiwiLang::Compiler ALIAS kiwiLang_codegen)
add_library(kiwiLang::Runtime ALIAS kiwiLang_runtime)

if(kiwiLang_ENABLE_TESTS)
    enable_testing()
    
    add_executable(kiwiLang_tests
        tests/test_main.cpp
        tests/unit/Lexer/test_scanner.cpp
        tests/unit/Lexer/test_lexer.cpp
        tests/unit/Parser/test_parser.cpp
        tests/unit/Semantic/test_typechecker.cpp
        tests/unit/CodeGen/test_irgen.cpp
        tests/unit/Runtime/test_vm.cpp
        tests/unit/Runtime/test_gc.cpp
        tests/integration/test_compiler.cpp
        tests/integration/test_stdlib.cpp
    )
    
    target_link_libraries(kiwiLang_tests
        PRIVATE
            Catch2::Catch2WithMain
            kiwiLang_codegen
            kiwiLang_runtime
    )
    
    k_lang_set_target_warnings(kiwiLang_tests)
    k_lang_set_target_sanitizers(kiwiLang_tests)
    k_lang_set_target_coverage(kiwiLang_tests)
    
    add_test(NAME kiwiLang_unit_tests COMMAND kiwiLang_tests)
    set_tests_properties(kiwiLang_unit_tests PROPERTIES
        TIMEOUT 30
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS kiwiLang_tests
        COMMENT "Running tests..."
    )
endif()

if(kiwiLang_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

if(kiwiLang_ENABLE_BENCHMARKS)
    add_subdirectory(bench)
endif()

add_custom_target(format
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/update_docs.py
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/benchmark.py --check-only
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running code formatters and documentation updates"
)

add_custom_target(benchmark
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/benchmark.py
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --output-dir ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_results
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running benchmarks"
    DEPENDS kiwiLang
)

add_custom_target(docs
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/update_docs.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation"
)

if(kiwiLang_INSTALL)
    include(GNUInstallDirs)
    
    install(TARGETS kiwiLang
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    if(kiwiLang_BUILD_SHARED)
        install(TARGETS kiwiLang_shared
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
    
    if(kiwiLang_BUILD_STATIC)
        install(TARGETS kiwiLang_static
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
    
    install(DIRECTORY include/kiwiLang
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
    
    install(FILES
        LICENSE
        README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
    
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kiwiLangConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/kiwiLangConfig.cmake
        @ONLY
    )
    
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/kiwiLangConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kiwiLang
    )
endif()

if(kiwiLang_ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            COMMAND $<TARGET_FILE:kiwiLang_tests>
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '*/tests/*' '*/third_party/*' --output-file coverage.info
            COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_report
            COMMAND ${CMAKE_COMMAND} -E remove coverage.info
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS kiwiLang_tests
            COMMENT "Generating code coverage report"
        )
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY}
            -checks=*
            -warnings-as-errors=*
        )
    endif()
endif()

message(STATUS "")
message(STATUS "kiwiLang Configuration Summary")
message(STATUS "===========================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: C++${kiwiLang_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  JIT Support: ${kiwiLang_ENABLE_JIT}")
message(STATUS "  LLVM Backend: ${kiwiLang_USE_LLVM}")
message(STATUS "  Tests: ${kiwiLang_ENABLE_TESTS}")
message(STATUS "  Examples: ${kiwiLang_ENABLE_EXAMPLES}")
message(STATUS "  Benchmarks: ${kiwiLang_ENABLE_BENCHMARKS}")
message(STATUS "  Sanitizers: ${kiwiLang_ENABLE_SANITIZERS}")
message(STATUS "  Assertions: ${kiwiLang_ENABLE_ASSERTIONS}")
message(STATUS "  Logging: ${kiwiLang_ENABLE_LOGGING}")
message(STATUS "  Coverage: ${kiwiLang_ENABLE_COVERAGE}")
message(STATUS "")
message(STATUS "Build Targets:")
message(STATUS "  kiwiLang - Main compiler executable")
if(kiwiLang_BUILD_SHARED)
    message(STATUS "  kiwiLang_shared - Shared library")
endif()
if(kiwiLang_BUILD_STATIC)
    message(STATUS "  kiwiLang_static - Static library")
endif()
if(kiwiLang_ENABLE_TESTS)
    message(STATUS "  kiwiLang_tests - Test suite")
    message(STATUS "  check - Run tests")
endif()
if(kiwiLang_ENABLE_COVERAGE)
    message(STATUS "  coverage - Generate coverage report")
endif()
message(STATUS "  format - Format code and update docs")
message(STATUS "  benchmark - Run benchmarks")
message(STATUS "  docs - Generate documentation")
message(STATUS "")