# FindLLVM.cmake - Find LLVM libraries and headers
#
# This module defines:
#   LLVM_FOUND          - True if LLVM was found
#   LLVM_INCLUDE_DIRS   - Include directories for LLVM
#   LLVM_LIBRARY_DIRS   - Library directories for LLVM
#   LLVM_LIBRARIES      - Libraries needed to use LLVM
#   LLVM_VERSION        - LLVM version (major.minor.patch)
#   LLVM_VERSION_MAJOR  - LLVM major version
#   LLVM_VERSION_MINOR  - LLVM minor version
#   LLVM_VERSION_PATCH  - LLVM patch version
#   LLVM_PACKAGE_VERSION - Full LLVM package version
#   LLVM_CXXFLAGS       - C++ flags required by LLVM
#   LLVM_DEFINITIONS    - Compiler definitions for LLVM
#   LLVM_HAS_RTTI       - Whether LLVM was built with RTTI
#   LLVM_BUILD_MODE     - Build mode (Release, Debug, etc.)
#
# This module also provides the following imported targets:
#   LLVM::LLVM          - Interface target with all LLVM dependencies
#
# Use the LLVM_DIR variable to specify a custom LLVM installation

cmake_minimum_required(VERSION 3.21)

include(FindPackageHandleStandardArgs)
include(SelectLibraryConfigurations)

# Try to find llvm-config first
find_program(LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config-15 llvm-config-14 llvm-config-13 llvm-config-12 llvm-config
    HINTS
        ${LLVM_DIR}
        ${LLVM_ROOT}
        $ENV{LLVM_DIR}
        $ENV{LLVM_ROOT}
    PATH_SUFFIXES bin
    DOC "LLVM config program"
)

if(LLVM_CONFIG_EXECUTABLE)
    message(STATUS "Found llvm-config: ${LLVM_CONFIG_EXECUTABLE}")
    
    # Get LLVM version
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_PACKAGE_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Parse version components
    if(LLVM_PACKAGE_VERSION MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(LLVM_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(LLVM_VERSION_MINOR ${CMAKE_MATCH_2})
        set(LLVM_VERSION_PATCH ${CMAKE_MATCH_3})
        set(LLVM_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")
    endif()
    
    # Get include directories
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get library directory
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
        OUTPUT_VARIABLE LLVM_LIBRARY_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get C++ flags
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE LLVM_CXXFLAGS_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get linker flags
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
        OUTPUT_VARIABLE LLVM_LDFLAGS_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get system libraries
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
        OUTPUT_VARIABLE LLVM_SYSTEM_LIBS_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Parse C++ flags into definitions and include directories
    separate_arguments(LLVM_CXXFLAGS_LIST UNIX_COMMAND ${LLVM_CXXFLAGS_RAW})
    set(LLVM_CXXFLAGS "")
    set(LLVM_DEFINITIONS "")
    
    foreach(flag ${LLVM_CXXFLAGS_LIST})
        if(flag MATCHES "^-I")
            # Include directory
            string(REGEX REPLACE "^-I" "" include_dir ${flag})
            list(APPEND LLVM_INCLUDE_DIR ${include_dir})
        elseif(flag MATCHES "^-D")
            # Definition
            string(REGEX REPLACE "^-D" "" definition ${flag})
            list(APPEND LLVM_DEFINITIONS ${definition})
        else()
            # Other flag
            list(APPEND LLVM_CXXFLAGS ${flag})
        endif()
    endforeach()
    
    # Check if LLVM was built with RTTI
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --has-rtti
        OUTPUT_VARIABLE LLVM_HAS_RTTI_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(LLVM_HAS_RTTI ${LLVM_HAS_RTTI_RAW})
    
    # Get build mode
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --build-mode
        OUTPUT_VARIABLE LLVM_BUILD_MODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Find specific LLVM libraries
    set(LLVM_LIBRARIES "")
    
    # Core LLVM libraries needed by KLang
    set(LLVM_LIBRARY_NAMES
        LLVMCore
        LLVMSupport
        LLVMAnalysis
        LLVMIRReader
        LLVMAsmParser
        LLVMBitReader
        LLVMBitWriter
        LLVMTransformUtils
        LLVMInstCombine
        LLVMScalarOpts
        LLVMipo
        LLVMVectorize
        LLVMObjCARCOpts
        LLVMCoroutines
        LLVMLinker
        LLVMAggressiveInstCombine
        LLVMInstrumentation
        LLVMProfileData
        LLVMObject
        LLVMMCParser
        LLVMMC
        LLVMDebugInfoCodeView
        LLVMDebugInfoDWARF
        LLVMDebugInfoMSF
        LLVMDebugInfoPDB
        LLVMSymbolize
        LLVMExecutionEngine
        LLVMInterpreter
        LLVMMCJIT
        LLVMOrcJIT
        LLVMRuntimeDyld
        LLVMTarget
        LLVMX86CodeGen
        LLVMX86AsmParser
        LLVMX86Disassembler
        LLVMX86Desc
        LLVMX86Info
        LLVMX86Utils
        LLVMAsmPrinter
        LLVMSelectionDAG
        LLVMCodeGen
        LLVMScalarOpts
        LLVMAggressiveInstCombine
        LLVMInstCombine
        LLVMTransformUtils
        LLVMBitWriter
        LLVMBitReader
        LLVMipo
        LLVMVectorize
        LLVMObjCARCOpts
        LLVMCoroutines
        LLVMLinker
        LLVMInstrumentation
        LLVMFrontendOpenMP
        LLVMFrontendOpenACC
    )
    
    foreach(lib_name ${LLVM_LIBRARY_NAMES})
        find_library(${lib_name}_LIBRARY
            NAMES ${lib_name}
            HINTS ${LLVM_LIBRARY_DIR}
            NO_DEFAULT_PATH
        )
        
        if(${lib_name}_LIBRARY)
            list(APPEND LLVM_LIBRARIES ${${lib_name}_LIBRARY})
        endif()
    endforeach()
    
    # Find additional required libraries
    find_library(LLVM_TERMINFO_LIBRARY
        NAMES tinfo ncursesw ncurses curses
    )
    
    if(LLVM_TERMINFO_LIBRARY)
        list(APPEND LLVM_LIBRARIES ${LLVM_TERMINFO_LIBRARY})
    endif()
    
    # Parse system libraries
    separate_arguments(LLVM_SYSTEM_LIBS_LIST UNIX_COMMAND ${LLVM_SYSTEM_LIBS_RAW})
    foreach(sys_lib ${LLVM_SYSTEM_LIBS_LIST})
        if(sys_lib MATCHES "^-l")
            string(REGEX REPLACE "^-l" "" lib_name ${sys_lib})
            find_library(${lib_name}_LIBRARY
                NAMES ${lib_name}
            )
            if(${lib_name}_LIBRARY)
                list(APPEND LLVM_LIBRARIES ${${lib_name}_LIBRARY})
            endif()
        endif()
    endforeach()
    
    # Remove duplicates
    list(REMOVE_DUPLICATES LLVM_INCLUDE_DIR)
    list(REMOVE_DUPLICATES LLVM_LIBRARIES)
    
    # Set output variables
    set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIR})
    set(LLVM_LIBRARY_DIRS ${LLVM_LIBRARY_DIR})
    
else()
    # Fallback: try to find LLVM using CMake's find_package
    
    find_package(LLVM 15.0 QUIET CONFIG)
    
    if(LLVM_FOUND)
        message(STATUS "Found LLVM via CMake config: ${LLVM_DIR}")
        
        # Get version from LLVMConfig
        set(LLVM_VERSION_MAJOR ${LLVM_VERSION_MAJOR})
        set(LLVM_VERSION_MINOR ${LLVM_VERSION_MINOR})
        set(LLVM_VERSION_PATCH ${LLVM_VERSION_PATCH})
        set(LLVM_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")
        set(LLVM_PACKAGE_VERSION ${LLVM_VERSION})
        
        # Use LLVM provided variables
        set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
        set(LLVM_LIBRARY_DIRS ${LLVM_LIBRARY_DIRS})
        
        # Get libraries using llvm_map_components_to_libnames
        include_directories(${LLVM_INCLUDE_DIRS})
        
        # Define required components for KLang
        set(LLVM_REQUIRED_COMPONENTS
            core
            support
            analysis
            irreader
            asmparser
            bitreader
            bitwriter
            transformutils
            instcombine
            scalaropts
            ipo
            vectorize
            objcarcopts
            coroutines
            linker
            aggressiveinstcombine
            instrumentation
            profiledata
            object
            mcparser
            mc
            debuginfocodeview
            debuginfodwarf
            debuginfomsf
            debuginfopdb
            symbolize
            executionengine
            interpreter
            mcjit
            orcjit
            runtimedyld
            target
            x86codegen
            x86asmparser
            x86disassembler
            x86desc
            x86info
            x86utils
            asmprinter
            selectiondag
            codegen
        )
        
        llvm_map_components_to_libnames(LLVM_LIBRARIES ${LLVM_REQUIRED_COMPONENTS})
        
        # Get definitions and flags
        set(LLVM_DEFINITIONS "")
        set(LLVM_CXXFLAGS "")
        
        if(LLVM_DEFINITIONS)
            set(LLVM_DEFINITIONS ${LLVM_DEFINITIONS})
        endif()
        
        # Check RTTI
        set(LLVM_HAS_RTTI ON)  # Most LLVM builds have RTTI enabled
        
        # Build mode
        if(CMAKE_BUILD_TYPE)
            set(LLVM_BUILD_MODE ${CMAKE_BUILD_TYPE})
        else()
            set(LLVM_BUILD_MODE "Release")
        endif()
        
    else()
        # Manual search as last resort
        
        find_path(LLVM_INCLUDE_DIR
            NAMES llvm/Config/llvm-config.h
            HINTS
                ${LLVM_DIR}
                ${LLVM_ROOT}
                $ENV{LLVM_DIR}
                $ENV{LLVM_ROOT}
                /usr/local/opt/llvm
                /opt/homebrew/opt/llvm
            PATH_SUFFIXES include
        )
        
        find_library(LLVM_CORE_LIBRARY
            NAMES LLVMCore
            HINTS
                ${LLVM_DIR}
                ${LLVM_ROOT}
                $ENV{LLVM_DIR}
                $ENV{LLVM_ROOT}
                /usr/local/opt/llvm
                /opt/homebrew/opt/llvm
            PATH_SUFFIXES lib
        )
        
        if(LLVM_INCLUDE_DIR AND LLVM_CORE_LIBRARY)
            # Extract version from config header
            if(EXISTS "${LLVM_INCLUDE_DIR}/llvm/Config/llvm-config.h")
                file(STRINGS "${LLVM_INCLUDE_DIR}/llvm/Config/llvm-config.h" LLVM_CONFIG_H)
                
                foreach(line ${LLVM_CONFIG_H})
                    if(line MATCHES "#define LLVM_VERSION_MAJOR ([0-9]+)")
                        set(LLVM_VERSION_MAJOR ${CMAKE_MATCH_1})
                    elseif(line MATCHES "#define LLVM_VERSION_MINOR ([0-9]+)")
                        set(LLVM_VERSION_MINOR ${CMAKE_MATCH_1})
                    elseif(line MATCHES "#define LLVM_VERSION_PATCH ([0-9]+)")
                        set(LLVM_VERSION_PATCH ${CMAKE_MATCH_1})
                    endif()
                endforeach()
                
                set(LLVM_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")
                set(LLVM_PACKAGE_VERSION ${LLVM_VERSION})
            endif()
            
            set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIR})
            set(LLVM_LIBRARIES ${LLVM_CORE_LIBRARY})
            
            # Try to find other required libraries
            find_library(LLVM_SUPPORT_LIBRARY LLVMSupport HINTS ${LLVM_DIR} PATH_SUFFIXES lib)
            find_library(LLVM_ANALYSIS_LIBRARY LLVMAnalysis HINTS ${LLVM_DIR} PATH_SUFFIXES lib)
            
            if(LLVM_SUPPORT_LIBRARY)
                list(APPEND LLVM_LIBRARIES ${LLVM_SUPPORT_LIBRARY})
            endif()
            
            if(LLVM_ANALYSIS_LIBRARY)
                list(APPEND LLVM_LIBRARIES ${LLVM_ANALYSIS_LIBRARY})
            endif()
            
            # Set default values
            set(LLVM_HAS_RTTI ON)
            set(LLVM_BUILD_MODE "Unknown")
            set(LLVM_CXXFLAGS "")
            set(LLVM_DEFINITIONS "")
        endif()
    endif()
endif()

# Handle QUIET and REQUIRED arguments
find_package_handle_standard_args(LLVM
    REQUIRED_VARS
        LLVM_INCLUDE_DIRS
        LLVM_LIBRARIES
        LLVM_VERSION
    VERSION_VAR LLVM_VERSION
    HANDLE_VERSION_RANGE
)

# Create imported target
if(LLVM_FOUND AND NOT TARGET LLVM::LLVM)
    add_library(LLVM::LLVM INTERFACE IMPORTED)
    
    set_target_properties(LLVM::LLVM PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIRS}"
        INTERFACE_COMPILE_DEFINITIONS "${LLVM_DEFINITIONS}"
        INTERFACE_COMPILE_OPTIONS "${LLVM_CXXFLAGS}"
        INTERFACE_LINK_LIBRARIES "${LLVM_LIBRARIES}"
    )
    
    # Add RTTI requirement if LLVM was built with RTTI
    if(LLVM_HAS_RTTI)
        set_target_properties(LLVM::LLVM PROPERTIES
            INTERFACE_COMPILE_OPTIONS "-frtti"
        )
    else()
        set_target_properties(LLVM::LLVM PROPERTIES
            INTERFACE_COMPILE_OPTIONS "-fno-rtti"
        )
    endif()
endif()

# Print summary
if(LLVM_FOUND AND NOT LLVM_FIND_QUIETLY)
    message(STATUS "Found LLVM ${LLVM_VERSION}")
    message(STATUS "  Include directories: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "  Library directories: ${LLVM_LIBRARY_DIRS}")
    message(STATUS "  Libraries: ${LLVM_LIBRARIES}")
    message(STATUS "  Has RTTI: ${LLVM_HAS_RTTI}")
    message(STATUS "  Build mode: ${LLVM_BUILD_MODE}")
    
    if(LLVM_DEFINITIONS)
        message(STATUS "  Definitions: ${LLVM_DEFINITIONS}")
    endif()
    
    if(LLVM_CXXFLAGS)
        message(STATUS "  CXX flags: ${LLVM_CXXFLAGS}")
    endif()
endif()

# Mark advanced variables
mark_as_advanced(
    LLVM_CONFIG_EXECUTABLE
    LLVM_INCLUDE_DIR
    LLVM_CORE_LIBRARY
    LLVM_SUPPORT_LIBRARY
    LLVM_ANALYSIS_LIBRARY
    LLVM_TERMINFO_LIBRARY
)

# Export variables
set(LLVM_FOUND ${LLVM_FOUND} CACHE BOOL "Whether LLVM was found")
set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS} CACHE PATH "LLVM include directories")
set(LLVM_LIBRARY_DIRS ${LLVM_LIBRARY_DIRS} CACHE PATH "LLVM library directories")
set(LLVM_LIBRARIES ${LLVM_LIBRARIES} CACHE FILEPATH "LLVM libraries")
set(LLVM_VERSION ${LLVM_VERSION} CACHE STRING "LLVM version")
set(LLVM_VERSION_MAJOR ${LLVM_VERSION_MAJOR} CACHE STRING "LLVM major version")
set(LLVM_VERSION_MINOR ${LLVM_VERSION_MINOR} CACHE STRING "LLVM minor version")
set(LLVM_VERSION_PATCH ${LLVM_VERSION_PATCH} CACHE STRING "LLVM patch version")
set(LLVM_PACKAGE_VERSION ${LLVM_PACKAGE_VERSION} CACHE STRING "LLVM package version")
set(LLVM_CXXFLAGS ${LLVM_CXXFLAGS} CACHE STRING "LLVM C++ flags")
set(LLVM_DEFINITIONS ${LLVM_DEFINITIONS} CACHE STRING "LLVM compiler definitions")
set(LLVM_HAS_RTTI ${LLVM_HAS_RTTI} CACHE BOOL "Whether LLVM was built with RTTI")
set(LLVM_BUILD_MODE ${LLVM_BUILD_MODE} CACHE STRING "LLVM build mode")