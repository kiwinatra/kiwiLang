# CompilerWarnings.cmake - Configure compiler warnings for KLang

include(CheckCXXCompilerFlag)

function(set_default_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
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
            /w14905
            /w14906
            /w14928
        )
        
        if(KLANG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            
            # Reasonable and useful warnings
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
            -Wimplicit-fallthrough
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
            -Wnull-dereference
            
            # C++ specific warnings
            -Wctor-dtor-privacy
            -Wnoexcept
            -Wstrict-null-sentinel
            -Wextra-semi
            -Wsign-promo
            -Wzero-as-null-pointer-constant
        )
        
        # Clang/AppleClang specific warnings
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
            target_compile_options(${target} PRIVATE
                -Wdocumentation
                -Wdocumentation-unknown-command
                -Wheader-hygiene
                -Weverything
                
                # Disable some overly pedantic warnings
                -Wno-c++98-compat
                -Wno-c++98-compat-pedantic
                -Wno-padded
                -Wno-exit-time-destructors
                -Wno-global-constructors
                -Wno-weak-vtables
                -Wno-covered-switch-default
                -Wno-deprecated
                -Wno-disabled-macro-expansion
                -Wno-reserved-id-macro
                -Wno-date-time
            )
        endif()
        
        # GCC specific warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
                -Wnull-dereference
                -Wnoexcept
                -Wstrict-null-sentinel
                -Wsign-promo
                -Wzero-as-null-pointer-constant
                
                # C++20 warnings
                $<$<VERSION_GREATER:${CMAKE_CXX_COMPILER_VERSION},10>:
                    -Wmismatched-tags
                    -Wredundant-tags
                >
            )
        endif()
        
        if(KLANG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(set_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /Wall
            /wd4820  # bytes padding added
            /wd4619  # pragma warning
            /wd4668  # undefined macro
            /wd4710  # function not inlined
            /wd4711  # function inlined
            /wd5045  # Spectre mitigation
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Weverything  # Clang only
            
            # Additional strict warnings
            -Wswitch-enum
            -Wswitch-default
            -Wcovered-switch-default
            -Wfloat-equal
            -Wundef
            -Wunreachable-code
            -Wunreachable-code-break
            -Wunreachable-code-return
            
            # Disable some noisy warnings for strict mode
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -Wno-c++98-compat
                -Wno-c++98-compat-pedantic
                -Wno-padded
                -Wno-exit-time-destructors
                -Wno-global-constructors
                -Wno-weak-vtables
                -Wno-covered-switch-default
                -Wno-deprecated
            >
        )
    endif()
endfunction()

function(set_sanitizer_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -fsanitize-address-use-after-scope
            
            # Additional sanitizer flags
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -fsanitize=integer
                -fsanitize=nullability
                -fsanitize=return
                -fsanitize=signed-integer-overflow
                -fsanitize=bounds
                -fsanitize=alignment
                -fsanitize=object-size
                -fsanitize=float-divide-by-zero
                -fsanitize=float-cast-overflow
                -fsanitize=nonnull-attribute
                -fsanitize=returns-nonnull-attribute
                -fsanitize=bool
                -fsanitize=enum
                -fsanitize=shift
                -fsanitize=unreachable
                -fsanitize=vla-bound
                -fsanitize=vptr
            >
        )
        
        target_link_options(${target} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
        )
    endif()
endfunction()

function(set_coverage_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -fprofile-arcs
            -ftest-coverage
            --coverage
        )
        
        target_link_options(${target} PRIVATE
            -fprofile-arcs
            -ftest-coverage
            --coverage
        )
    endif()
endfunction()

function(set_performance_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /O2
            /Ob2
            /Oi
            /Ot
            /GT
            /GL
            /fp:fast
            /Qpar
            /arch:AVX2
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -O3
            -march=native
            -mtune=native
            -ffast-math
            -funroll-loops
            -flto
            -fomit-frame-pointer
            
            # Warning flags that help with performance
            -Wdisabled-optimization
            -Waggressive-loop-optimizations
            
            # CPU specific optimizations
            $<$<CXX_COMPILER_ID:GNU>:
                -fipa-pta
                -fdevirtualize-speculatively
                -fdevirtualize-at-ltrans
                -fschedule-insns2
                -fno-semantic-interposition
            >
            
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -fvectorize
                -fslp-vectorize
                -ffp-contract=fast
            >
        )
        
        target_link_options(${target} PRIVATE
            -flto
            -fuse-linker-plugin
        )
    endif()
endfunction()

function(set_security_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /GS
            /guard:cf
            /sdl
            /DYNAMICBASE
            /NXCOMPAT
            /CETCOMPAT
        )
        
        target_link_options(${target} PRIVATE
            /GUARD:CF
            /DYNAMICBASE
            /NXCOMPAT
            /CETCOMPAT
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -D_FORTIFY_SOURCE=2
            -fstack-protector-strong
            -fstack-clash-protection
            -fcf-protection=full
            -fPIE
            
            # Security warning flags
            -Wformat-security
            -Wtrampolines
            
            # Additional security flags for GCC
            $<$<CXX_COMPILER_ID:GNU>:
                -Wa,--noexecstack
                -fexceptions
                -fasynchronous-unwind-tables
                -fstack-check
            >
            
            # Additional security flags for Clang
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -fstack-protector-all
                -ftrivial-auto-var-init=pattern
            >
        )
        
        target_link_options(${target} PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,-z,noexecstack
            -pie
        )
    endif()
endfunction()

function(set_debug_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /Od
            /Ob0
            /RTC1
            /JMC
            /ZI
            /DEBUG:FULL
            /MDd
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -O0
            -g3
            -ggdb
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            
            # Debug information flags
            -fvar-tracking
            -fvar-tracking-assignments
            
            # Additional debug flags for GCC
            $<$<CXX_COMPILER_ID:GNU>:
                -fdiagnostics-color=always
                -frecord-gcc-switches
                -gsplit-dwarf
            >
            
            # Additional debug flags for Clang
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -fdebug-macro
                -fdebug-default-version=4
                -glldb
                -fstandalone-debug
            >
        )
    endif()
endfunction()

function(set_cpp20_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${target} PRIVATE
            $<$<VERSION_GREATER:${CMAKE_CXX_COMPILER_VERSION},10>:
                -Wctad-maybe-unsupported
                -Wdeprecated-enum-enum-conversion
                -Wdeprecated-enum-float-conversion
                -Wredundant-tags
                -Wmismatched-tags
                -Wrange-loop-construct
                -Wredundant-move
                -Wpessimizing-move
            >
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wc++20-compat
            -Wdeprecated-copy
            -Wdeprecated-copy-dtor
            -Wrange-loop-construct
            -Wredundant-move
            -Wpessimizing-move
        )
    endif()
endfunction()

function(enable_compiler_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target ${target} does not exist")
    endif()
    
    message(STATUS "Configuring warnings for target: ${target}")
    
    # Always apply default warnings
    set_default_warnings(${target})
    
    # Apply additional warning sets based on configuration
    if(KLANG_ENABLE_SANITIZERS)
        set_sanitizer_warnings(${target})
    endif()
    
    if(KLANG_ENABLE_COVERAGE)
        set_coverage_warnings(${target})
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_debug_warnings(${target})
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_performance_warnings(${target})
        set_security_warnings(${target})
    endif()
    
    # C++20 specific warnings
    if(KLANG_CXX_STANDARD GREATER_EQUAL 20)
        set_cpp20_warnings(${target})
    endif()
    
    # Strict warnings if requested
    if(KLANG_MAX_WARNINGS)
        set_strict_warnings(${target})
    endif()
endfunction()

function(disable_specific_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target ${target} does not exist")
    endif()
    
    # Disable specific warnings that might be too noisy
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /wd4068  # unknown pragma
            /wd4100  # unreferenced formal parameter
            /wd4127  # conditional expression is constant
            /wd4189  # local variable is initialized but not referenced
            /wd4201  # nonstandard extension used: nameless struct/union
            /wd4244  # conversion from 'type1' to 'type2', possible loss of data
            /wd4251  # class needs to have dll-interface to be used by clients
            /wd4267  # conversion from 'size_t' to 'type', possible loss of data
            /wd4324  # structure was padded due to alignment specifier
            /wd4351  # new behavior: elements of array will be default initialized
            /wd4371  # layout of class may have changed from a previous version
            /wd4514  # unreferenced inline function has been removed
            /wd4571  # catch(...) semantics changed since Visual C++ 7.1
            /wd4623  # default constructor was implicitly defined as deleted
            /wd4625  # copy constructor was implicitly defined as deleted
            /wd4626  # assignment operator was implicitly defined as deleted
            /wd4668  # 'symbol' is not defined as a preprocessor macro
            /wd4710  # function not inlined
            /wd4711  # function selected for inline expansion
            /wd4820  # bytes padding added after data member
            /wd5026  # move constructor was implicitly defined as deleted
            /wd5027  # move assignment operator was implicitly defined as deleted
            /wd5039  # pointer or reference to potentially throwing function
            /wd5045  # compiler will insert Spectre mitigation
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wno-unused-parameter
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -Wno-unused-local-typedefs
            -Wno-unused-function
            -Wno-deprecated-declarations
            -Wno-missing-field-initializers
            -Wno-extra-semi
            -Wno-zero-as-null-pointer-constant
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
        )
        
        # Clang specific warning suppressions
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
            target_compile_options(${target} PRIVATE
                -Wno-padded
                -Wno-exit-time-destructors
                -Wno-global-constructors
                -Wno-weak-vtables
                -Wno-covered-switch-default
                -Wno-disabled-macro-expansion
                -Wno-reserved-id-macro
                -Wno-date-time
                -Wno-documentation-unknown-command
                -Wno-undefined-func-template
            )
        endif()
    endif()
endfunction()

# Helper function to check if a warning flag is supported
function(check_warning_flag flag var)
    set(CMAKE_REQUIRED_FLAGS "${flag}")
    check_cxx_compiler_flag("${flag}" ${var})
    
    if(${var})
        message(STATUS "Compiler supports warning flag: ${flag}")
    else()
        message(STATUS "Compiler does NOT support warning flag: ${flag}")
    endif()
endfunction()

# Test compiler warning support
function(test_compiler_warnings)
    message(STATUS "Testing compiler warning support...")
    
    # Test common warning flags
    check_warning_flag("-Wall" HAS_WALL)
    check_warning_flag("-Wextra" HAS_WEXTRA)
    check_warning_flag("-Wpedantic" HAS_WPEDANTIC)
    check_warning_flag("-Werror" HAS_WERROR)
    
    # Test GCC specific flags
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        check_warning_flag("-Wduplicated-cond" HAS_DUPLICATED_COND)
        check_warning_flag("-Wduplicated-branches" HAS_DUPLICATED_BRANCHES)
        check_warning_flag("-Wlogical-op" HAS_LOGICAL_OP)
    endif()
    
    # Test Clang specific flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        check_warning_flag("-Weverything" HAS_EVERYTHING)
        check_warning_flag("-Wdocumentation" HAS_DOCUMENTATION)
    endif()
    
    # Report results
    message(STATUS "Compiler warning support test complete")
endfunction()

# Export functions
set(COMPILER_WARNINGS_FUNCTIONS
    enable_compiler_warnings
    disable_specific_warnings
    set_default_warnings
    set_strict_warnings
    set_sanitizer_warnings
    set_coverage_warnings
    set_performance_warnings
    set_security_warnings
    set_debug_warnings
    set_cpp20_warnings
    test_compiler_warnings
)

foreach(func ${COMPILER_WARNINGS_FUNCTIONS})
    set(${func} ${func} PARENT_SCOPE)
endforeach()