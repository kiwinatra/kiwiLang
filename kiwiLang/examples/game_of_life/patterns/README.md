cmake_minimum_required(VERSION 3.15)
project(hello_world LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_DEBUG "Enable debug symbols" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Find KiwiLang compiler
find_program(KIWIC_COMPILER kiwic PATHS ${CMAKE_SOURCE_DIR}/../build/bin)
if(NOT KIWIC_COMPILER)
    message(WARNING "KiwiLang compiler not found, building from source")
    # Build compiler if not found
    add_subdirectory(${CMAKE_SOURCE_DIR}/../compiler)
    set(KIWIC_COMPILER ${CMAKE_BINARY_DIR}/../compiler/bin/kiwic)
endif()

# KiwiLang source files
set(KIWILANG_SOURCES
    main.kiwi
    greeting.kiwi
    utils.kiwi
)

# Generate C++ files from KiwiLang
set(GENERATED_SOURCES)
foreach(kiwi_file ${KIWILANG_SOURCES})
    get_filename_component(base_name ${kiwi_file} NAME_WE)
    set(cpp_file ${CMAKE_CURRENT_BINARY_DIR}/${base_name}.cpp)
    
    add_custom_command(
        OUTPUT ${cpp_file}
        COMMAND ${KIWIC_COMPILER}
        ARGS -c ${CMAKE_CURRENT_SOURCE_DIR}/${kiwi_file}
             -o ${cpp_file}
             --emit-llvm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${kiwi_file}
        COMMENT "Compiling KiwiLang: ${kiwi_file}"
    )
    
    list(APPEND GENERATED_SOURCES ${cpp_file})
endforeach()

# Main executable
add_executable(hello_world
    ${GENERATED_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# Link libraries
target_link_libraries(hello_world
    PRIVATE
        kiwiLang_runtime
        kiwiLang_stdlib
)

if(UNIX)
    target_link_libraries(hello_world
        PRIVATE
            pthread
            m
            dl
    )
endif()

# Install targets
install(TARGETS hello_world
    RUNTIME DESTINATION bin
)

# Copy runtime files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/../runtime/
    DESTINATION share/kiwilang/runtime
    FILES_MATCHING PATTERN "*.kiwi"
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(test_hello
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_greeting.cpp
    )
    
    target_link_libraries(test_hello
        PRIVATE
            gtest_main
            gtest
            hello_world
    )
    
    add_test(NAME test_hello_world COMMAND test_hello)
endif()

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "Hello World Example")
    set(DOXYGEN_PROJECT_NUMBER "1.0")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    
    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generate documentation"
    )
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/hello_worldConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hello_worldConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/hello_worldConfig.cmake
    INSTALL_DESTINATION lib/cmake/hello_world
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hello_worldConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hello_worldConfigVersion.cmake
    DESTINATION lib/cmake/hello_world
)

# Export targets
export(EXPORT hello_worldTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/hello_worldTargets.cmake
)

install(EXPORT hello_worldTargets
    FILE hello_worldTargets.cmake
    DESTINATION lib/cmake/hello_world
)

# CPack configuration
set(CPACK_PACKAGE_NAME "hello_world")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION "Hello World Example for KiwiLang")
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/../LICENSE)

include(CPack)