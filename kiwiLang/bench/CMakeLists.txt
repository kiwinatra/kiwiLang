cmake_minimum_required(VERSION 3.21)

project(kiwiLang_benchmarks
    DESCRIPTION "kiwiLang Benchmark Suite"
    LANGUAGES CXX
)

if(NOT TARGET kiwiLang_codegen)
    message(FATAL_ERROR "kiwiLang compiler library not found")
endif()

if(NOT TARGET kiwiLang_runtime)
    message(FATAL_ERROR "kiwiLang runtime library not found")
endif()

find_package(benchmark 1.7.0 REQUIRED)

add_executable(kiwiLang_microbenchmarks
    microbenchmarks.cpp
)

target_link_libraries(kiwiLang_microbenchmarks
    PRIVATE
        benchmark::benchmark
        kiwiLang_codegen
        kiwiLang_runtime
)

target_include_directories(kiwiLang_microbenchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(kiwiLang_microbenchmarks
    PRIVATE
        kiwiLang_BENCHMARK_MODE
)

set_target_properties(kiwiLang_microbenchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(kiwiLang_microbenchmarks PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -march=native
        -mtune=native
        -DNDEBUG
    )
    
    target_link_options(kiwiLang_microbenchmarks PRIVATE
        -rdynamic
    )
elseif(MSVC)
    target_compile_options(kiwiLang_microbenchmarks PRIVATE
        /W4
        /O2
        /Ob2
        /DNDEBUG
        /EHsc
    )
endif()

add_custom_target(run_microbenchmarks
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/kiwiLang_microbenchmarks
        --benchmark_min_time=0.5
        --benchmark_out=${CMAKE_CURRENT_BINARY_DIR}/microbenchmark_results.json
        --benchmark_out_format=json
    DEPENDS kiwiLang_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running microbenchmarks"
)

add_custom_target(run_microbenchmarks_brief
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/kiwiLang_microbenchmarks
        --benchmark_min_time=0.1
    DEPENDS kiwiLang_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running brief microbenchmarks"
)

add_custom_target(run_microbenchmarks_detailed
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/kiwiLang_microbenchmarks
        --benchmark_min_time=1.0
        --benchmark_repetitions=10
        --benchmark_report_aggregates_only=true
        --benchmark_out=${CMAKE_CURRENT_BINARY_DIR}/detailed_benchmark_results.json
        --benchmark_out_format=json
    DEPENDS kiwiLang_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running detailed microbenchmarks"
)

add_custom_target(process_benchmark_results
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/scripts/benchmark.py
        --process
        --input ${CMAKE_CURRENT_BINARY_DIR}/microbenchmark_results.json
        --output ${CMAKE_CURRENT_BINARY_DIR}/benchmark_report.html
    DEPENDS run_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Processing benchmark results"
)

add_custom_target(benchmark_report
    COMMAND ${CMAKE_COMMAND} -E echo "Benchmark report available at:"
    COMMAND ${CMAKE_COMMAND} -E echo "  file://${CMAKE_CURRENT_BINARY_DIR}/benchmark_report.html"
    DEPENDS process_benchmark_results
)

file(GLOB REALWORLD_BENCHMARKS "${CMAKE_CURRENT_SOURCE_DIR}/realworld/*.kiwi")

foreach(benchmark_file ${REALWORLD_BENCHMARKS})
    get_filename_component(benchmark_name ${benchmark_file} NAME_WE)
    
    add_custom_target(benchmark_${benchmark_name}
        COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${benchmark_name}..."
        COMMAND ${CMAKE_BINARY_DIR}/bin/kiwiLang compile ${benchmark_file} -O3 -o ${CMAKE_CURRENT_BINARY_DIR}/bin/${benchmark_name}
        COMMAND ${CMAKE_COMMAND} -E echo "Running ${benchmark_name}..."
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/${benchmark_name}
        DEPENDS kiwiLang
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/realworld
        COMMENT "Running real-world benchmark: ${benchmark_name}"
    )
    
    list(APPEND REALWORLD_BENCHMARK_TARGETS benchmark_${benchmark_name})
endforeach()

if(REALWORLD_BENCHMARK_TARGETS)
    add_custom_target(run_realworld_benchmarks
        DEPENDS ${REALWORLD_BENCHMARK_TARGETS}
        COMMENT "Running all real-world benchmarks"
    )
endif()

add_custom_target(benchmark_compilation_time
    COMMAND ${CMAKE_COMMAND} -E time
        ${CMAKE_BINARY_DIR}/bin/kiwiLang compile
        ${CMAKE_CURRENT_SOURCE_DIR}/realworld/primes.kiwi
        -O3
        -o ${CMAKE_CURRENT_BINARY_DIR}/bin/primes_bench
    DEPENDS kiwiLang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Benchmarking compilation time"
)

add_custom_target(benchmark_memory_usage
    COMMAND valgrind --tool=massif
        --massif-out-file=${CMAKE_CURRENT_BINARY_DIR}/massif.out
        ${CMAKE_BINARY_DIR}/bin/kiwiLang compile
        ${CMAKE_CURRENT_SOURCE_DIR}/realworld/mandelbrot.kiwi
        -O1
    COMMAND ms_print ${CMAKE_CURRENT_BINARY_DIR}/massif.out > ${CMAKE_CURRENT_BINARY_DIR}/memory_profile.txt
    DEPENDS kiwiLang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Profiling memory usage"
)

add_custom_target(benchmark_cache
    COMMAND perf stat -e cache-references,cache-misses,instructions,cycles
        ${CMAKE_BINARY_DIR}/bin/kiwiLang_microbenchmarks
        --benchmark_filter="BM_HashTable"
        --benchmark_min_time=0.1
    DEPENDS kiwiLang_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Benchmarking cache performance"
)

add_custom_target(benchmark_all
    DEPENDS
        run_microbenchmarks
        run_realworld_benchmarks
        benchmark_compilation_time
        benchmark_memory_usage
    COMMENT "Running all benchmarks"
)

add_custom_target(benchmark_ci
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/kiwiLang_microbenchmarks
        --benchmark_min_time=0.1
        --benchmark_out=${CMAKE_CURRENT_BINARY_DIR}/ci_benchmark_results.json
        --benchmark_out_format=json
    DEPENDS kiwiLang_microbenchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running CI benchmarks"
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_config.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/benchmark_config.json
    @ONLY
)

install(TARGETS kiwiLang_microbenchmarks
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT benchmarks
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/realworld/primes.kiwi
    ${CMAKE_CURRENT_SOURCE_DIR}/realworld/mandelbrot.kiwi
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kiwiLang/benchmarks
    COMPONENT benchmarks
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/realworld/patterns
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kiwiLang/benchmarks
    COMPONENT benchmarks
)

message(STATUS "")
message(STATUS "kiwiLang Benchmarks Configuration")
message(STATUS "==============================")
message(STATUS "Microbenchmarks: kiwiLang_microbenchmarks")
message(STATUS "Real-world benchmarks: ${REALWORLD_BENCHMARKS}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  run_microbenchmarks - Run all microbenchmarks")
message(STATUS "  run_realworld_benchmarks - Run real-world benchmarks")
message(STATUS "  benchmark_compilation_time - Measure compilation time")
message(STATUS "  benchmark_memory_usage - Profile memory usage")
message(STATUS "  benchmark_cache - Analyze cache performance")
message(STATUS "  benchmark_all - Run all benchmarks")
message(STATUS "  benchmark_ci - Run CI-optimized benchmarks")
message(STATUS "  benchmark_report - Generate HTML benchmark report")
message(STATUS "")