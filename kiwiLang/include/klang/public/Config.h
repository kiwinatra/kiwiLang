#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace klang {

// Forward declarations
class Runtime;

// Configuration version
constexpr uint32_t CONFIG_VERSION = 1;
constexpr uint32_t CONFIG_VERSION_MAJOR = 1;
constexpr uint32_t CONFIG_VERSION_MINOR = 0;
constexpr uint32_t CONFIG_VERSION_PATCH = 0;

// Build configuration
#ifdef KLANG_DEBUG
    constexpr bool IS_DEBUG_BUILD = true;
#else
    constexpr bool IS_DEBUG_BUILD = false;
#endif

#ifdef KLANG_RELEASE
    constexpr bool IS_RELEASE_BUILD = true;
#else
    constexpr bool IS_RELEASE_BUILD = false;
#endif

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define KLANG_WINDOWS 1
    constexpr const char* PLATFORM_NAME = "Windows";
#elif defined(__APPLE__) && defined(__MACH__)
    #define KLANG_MACOS 1
    constexpr const char* PLATFORM_NAME = "macOS";
#elif defined(__linux__)
    #define KLANG_LINUX 1
    constexpr const char* PLATFORM_NAME = "Linux";
#elif defined(__FreeBSD__)
    #define KLANG_FREEBSD 1
    constexpr const char* PLATFORM_NAME = "FreeBSD";
#elif defined(__OpenBSD__)
    #define KLANG_OPENBSD 1
    constexpr const char* PLATFORM_NAME = "OpenBSD";
#elif defined(__NetBSD__)
    #define KLANG_NETBSD 1
    constexpr const char* PLATFORM_NAME = "NetBSD";
#elif defined(__DragonFly__)
    #define KLANG_DRAGONFLY 1
    constexpr const char* PLATFORM_NAME = "DragonFly";
#elif defined(__sun) && defined(__SVR4)
    #define KLANG_SOLARIS 1
    constexpr const char* PLATFORM_NAME = "Solaris";
#elif defined(__HAIKU__)
    #define KLANG_HAIKU 1
    constexpr const char* PLATFORM_NAME = "Haiku";
#elif defined(__EMSCRIPTEN__)
    #define KLANG_WASM 1
    constexpr const char* PLATFORM_NAME = "WebAssembly";
#else
    #define KLANG_UNKNOWN 1
    constexpr const char* PLATFORM_NAME = "Unknown";
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define KLANG_X86_64 1
    constexpr const char* ARCH_NAME = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    #define KLANG_X86 1
    constexpr const char* ARCH_NAME = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define KLANG_ARM64 1
    constexpr const char* ARCH_NAME = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
    #define KLANG_ARM 1
    constexpr const char* ARCH_NAME = "ARM";
#elif defined(__powerpc64__)
    #define KLANG_PPC64 1
    constexpr const char* ARCH_NAME = "PPC64";
#elif defined(__powerpc__)
    #define KLANG_PPC 1
    constexpr const char* ARCH_NAME = "PPC";
#elif defined(__riscv) && (__riscv_xlen == 64)
    #define KLANG_RISCV64 1
    constexpr const char* ARCH_NAME = "RISC-V64";
#elif defined(__riscv) && (__riscv_xlen == 32)
    #define KLANG_RISCV32 1
    constexpr const char* ARCH_NAME = "RISC-V32";
#elif defined(__mips64)
    #define KLANG_MIPS64 1
    constexpr const char* ARCH_NAME = "MIPS64";
#elif defined(__mips__)
    #define KLANG_MIPS 1
    constexpr const char* ARCH_NAME = "MIPS";
#elif defined(__s390x__)
    #define KLANG_S390X 1
    constexpr const char* ARCH_NAME = "S390X";
#elif defined(__alpha__)
    #define KLANG_ALPHA 1
    constexpr const char* ARCH_NAME = "Alpha";
#elif defined(__sparc__)
    #define KLANG_SPARC 1
    constexpr const char* ARCH_NAME = "SPARC";
#elif defined(__EMSCRIPTEN__)
    #define KLANG_WASM32 1
    constexpr const char* ARCH_NAME = "WASM32";
#else
    #define KLANG_UNKNOWN_ARCH 1
    constexpr const char* ARCH_NAME = "Unknown";
#endif

// Compiler detection
#if defined(__clang__)
    #define KLANG_CLANG 1
    constexpr const char* COMPILER_NAME = "Clang";
    constexpr uint32_t COMPILER_VERSION = (__clang_major__ * 10000) + (__clang_minor__ * 100) + __clang_patchlevel__;
#elif defined(__GNUC__) && !defined(__clang__)
    #define KLANG_GCC 1
    constexpr const char* COMPILER_NAME = "GCC";
    constexpr uint32_t COMPILER_VERSION = (__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    #define KLANG_MSVC 1
    constexpr const char* COMPILER_NAME = "MSVC";
    constexpr uint32_t COMPILER_VERSION = _MSC_VER;
#elif defined(__INTEL_COMPILER)
    #define KLANG_ICC 1
    constexpr const char* COMPILER_NAME = "Intel";
    constexpr uint32_t COMPILER_VERSION = __INTEL_COMPILER;
#else
    #define KLANG_UNKNOWN_COMPILER 1
    constexpr const char* COMPILER_NAME = "Unknown";
    constexpr uint32_t COMPILER_VERSION = 0;
#endif

// Endianness detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define KLANG_LITTLE_ENDIAN 1
    constexpr bool IS_LITTLE_ENDIAN = true;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define KLANG_BIG_ENDIAN 1
    constexpr bool IS_LITTLE_ENDIAN = false;
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__)
    #define KLANG_LITTLE_ENDIAN 1
    constexpr bool IS_LITTLE_ENDIAN = true;
#else
    #error "Cannot determine endianness"
#endif

// Configuration option categories
enum class ConfigCategory {
    General,
    Compiler,
    Runtime,
    JIT,
    Memory,
    GC,
    Optimization,
    Security,
    Debugging,
    Profiling,
    Logging,
    Networking,
    Filesystem,
    Threading,
    Serialization,
    Compatibility,
    Experimental,
    UserDefined
};

// Configuration value types
enum class ConfigType {
    Boolean,
    Integer,
    Float,
    String,
    Path,
    Enum,
    List,
    Map,
    Function,
    Object
};

// Configuration option descriptor
struct ConfigOption {
    std::string name;
    std::string description;
    ConfigCategory category;
    ConfigType type;
    bool is_required;
    bool is_readonly;
    bool is_experimental;
    bool is_deprecated;
    std::string deprecation_message;
    std::string default_value;
    std::string min_value;
    std::string max_value;
    std::vector<std::string> allowed_values;
    std::string validation_regex;
    std::function<bool(const std::string&)> validator;
    
    // Metadata
    std::string group;
    std::string subgroup;
    uint32_t version_added;
    uint32_t version_deprecated;
    uint32_t version_removed;
    std::vector<std::string> dependencies;
    std::vector<std::string> conflicts;
};

// Configuration section
class ConfigSection {
public:
    ConfigSection(const std::string& name, const std::string& description = "");
    
    const std::string& name() const;
    const std::string& description() const;
    
    bool has_option(const std::string& name) const;
    ConfigOption get_option(const std::string& name) const;
    void set_option(const ConfigOption& option);
    void remove_option(const std::string& name);
    
    std::vector<std::string> option_names() const;
    size_t option_count() const;
    
private:
    std::string name_;
    std::string description_;
    std::unordered_map<std::string, ConfigOption> options_;
};

// Configuration manager
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Singleton access
    static ConfigManager& instance();
    
    // Configuration loading/saving
    bool load_from_file(const std::string& filename);
    bool save_to_file(const std::string& filename) const;
    
    bool load_from_string(const std::string& config_str);
    std::string save_to_string() const;
    
    bool load_from_environment();
    bool load_from_command_line(int argc, char** argv);
    
    // Section management
    bool has_section(const std::string& name) const;
    ConfigSection& get_section(const std::string& name);
    const ConfigSection& get_section(const std::string& name) const;
    
    void add_section(const ConfigSection& section);
    void remove_section(const std::string& name);
    
    std::vector<std::string> section_names() const;
    size_t section_count() const;
    
    // Value access
    template<typename T>
    T get_value(const std::string& section, const std::string& key, const T& default_value = T()) const;
    
    template<typename T>
    bool set_value(const std::string& section, const std::string& key, const T& value);
    
    bool has_value(const std::string& section, const std::string& key) const;
    void remove_value(const std::string& section, const std::string& key);
    
    // Type conversion
    template<typename T>
    static T convert_from_string(const std::string& str);
    
    template<typename T>
    static std::string convert_to_string(const T& value);
    
    // Validation
    bool validate() const;
    std::vector<std::string> validation_errors() const;
    
    // Merge configurations
    void merge(const ConfigManager& other, bool overwrite = false);
    
    // Reset to defaults
    void reset_to_defaults();
    void clear();
    
    // Change tracking
    bool is_modified() const;
    void mark_clean();
    std::vector<std::string> modified_keys() const;
    
    // Watch for changes
    using ChangeCallback = std::function<void(const std::string& section, const std::string& key, const std::string& old_value, const std::string& new_value)>;
    
    void watch(const std::string& section, const std::string& key, ChangeCallback callback);
    void unwatch(const std::string& section, const std::string& key);
    
    // Runtime configuration
    void apply_to_runtime(Runtime* runtime) const;
    void update_from_runtime(const Runtime* runtime);
    
    // Versioning
    uint32_t version() const;
    void set_version(uint32_t version);
    
    // Metadata
    void set_metadata(const std::string& key, const std::string& value);
    std::string get_metadata(const std::string& key) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Configuration builder for fluent interface
class ConfigBuilder {
public:
    ConfigBuilder();
    explicit ConfigBuilder(ConfigManager& manager);
    
    ConfigBuilder& section(const std::string& name, const std::string& description = "");
    
    template<typename T>
    ConfigBuilder& option(const std::string& name, const T& value, const std::string& description = "");
    
    ConfigBuilder& option(const ConfigOption& option);
    
    ConfigBuilder& required(bool required = true);
    ConfigBuilder& readonly(bool readonly = true);
    ConfigBuilder& experimental(bool experimental = true);
    ConfigBuilder& deprecated(const std::string& message = "");
    
    ConfigBuilder& min_value(const std::string& min);
    ConfigBuilder& max_value(const std::string& max);
    ConfigBuilder& allowed_values(const std::vector<std::string>& values);
    ConfigBuilder& validator(std::function<bool(const std::string&)> validator);
    
    ConfigBuilder& group(const std::string& group);
    ConfigBuilder& subgroup(const std::string& subgroup);
    
    ConfigManager& build();
    
private:
    ConfigManager* manager_;
    std::string current_section_;
    std::string current_description_;
};

// Predefined configurations
namespace config {
    
    // Default compiler configuration
    ConfigManager default_compiler_config();
    
    // Default runtime configuration
    ConfigManager default_runtime_config();
    
    // Default JIT configuration
    ConfigManager default_jit_config();
    
    // Debug configuration
    ConfigManager debug_config();
    
    // Release configuration
    ConfigManager release_config();
    
    // Profile configuration
    ConfigManager profile_config();
    
    // Test configuration
    ConfigManager test_config();
    
    // Minimal configuration
    ConfigManager minimal_config();
    
    // Secure configuration
    ConfigManager secure_config();
    
    // Performance configuration
    ConfigManager performance_config();
    
    // Compatibility configuration
    ConfigManager compatibility_config();
    
    // Platform-specific configurations
    ConfigManager windows_config();
    ConfigManager linux_config();
    ConfigManager macos_config();
    ConfigManager wasm_config();
    
    // Architecture-specific configurations
    ConfigManager x86_64_config();
    ConfigManager arm64_config();
    ConfigManager riscv64_config();
    
} // namespace config

// Configuration utilities
namespace config_utils {
    
    // Environment variable helpers
    std::string get_env(const std::string& name, const std::string& default_value = "");
    bool set_env(const std::string& name, const std::string& value);
    
    // Path resolution
    std::string expand_path(const std::string& path);
    std::string resolve_path(const std::string& path);
    std::string relative_path(const std::string& path, const std::string& base);
    
    // Platform-specific paths
    std::string get_config_dir();
    std::string get_data_dir();
    std::string get_cache_dir();
    std::string get_log_dir();
    std::string get_temp_dir();
    
    // Configuration file detection
    std::vector<std::string> find_config_files(const std::string& name);
    std::string find_config_file(const std::string& name);
    
    // Configuration merging
    ConfigManager merge_configs(const std::vector<ConfigManager>& configs);
    
    // Configuration validation
    bool validate_config(const ConfigManager& config, std::vector<std::string>& errors);
    
    // Configuration comparison
    ConfigManager diff_configs(const ConfigManager& old_config, const ConfigManager& new_config);
    
    // Configuration template expansion
    ConfigManager expand_templates(const ConfigManager& config, const std::unordered_map<std::string, std::string>& variables);
    
} // namespace config_utils

// Configuration macros for compile-time options
#define KLANG_CONFIG_OPTION(name, default_value, description) \
    namespace klang::config::options { \
        constexpr auto name = default_value; \
    }

#define KLANG_CONFIG_FEATURE(name, enabled, description) \
    namespace klang::config::features { \
        constexpr bool name = enabled; \
    }

#define KLANG_CONFIG_DEFINE(type, name, value) \
    namespace klang::config::defines { \
        constexpr type name = value; \
    }

// Predefined configuration options
KLANG_CONFIG_OPTION(max_memory, size_t(1024 * 1024 * 1024), "Maximum memory usage")
KLANG_CONFIG_OPTION(stack_size, size_t(64 * 1024), "Stack size")
KLANG_CONFIG_OPTION(gc_threshold, size_t(8 * 1024 * 1024), "GC threshold")
KLANG_CONFIG_OPTION(jit_cache_size, size_t(4 * 1024 * 1024), "JIT cache size")
KLANG_CONFIG_OPTION(max_recursion_depth, uint32_t(1000), "Maximum recursion depth")

KLANG_CONFIG_FEATURE(enable_jit, true, "Enable JIT compilation")
KLANG_CONFIG_FEATURE(enable_gc, true, "Enable garbage collection")
KLANG_CONFIG_FEATURE(enable_assertions, IS_DEBUG_BUILD, "Enable runtime assertions")
KLANG_CONFIG_FEATURE(enable_logging, IS_DEBUG_BUILD, "Enable logging")
KLANG_CONFIG_FEATURE(enable_profiling, false, "Enable profiling")
KLANG_CONFIG_FEATURE(enable_debugging, IS_DEBUG_BUILD, "Enable debugging")
KLANG_CONFIG_FEATURE(enable_sanitizers, IS_DEBUG_BUILD, "Enable sanitizers")
KLANG_CONFIG_FEATURE(enable_coverage, false, "Enable code coverage")
KLANG_CONFIG_FEATURE(enable_security, true, "Enable security features")
KLANG_CONFIG_FEATURE(enable_threading, false, "Enable threading support")

KLANG_CONFIG_DEFINE(bool, use_sse, true)
KLANG_CONFIG_DEFINE(bool, use_avx, true)
KLANG_CONFIG_DEFINE(bool, use_avx2, true)
KLANG_CONFIG_DEFINE(bool, use_avx512, false)
KLANG_CONFIG_DEFINE(bool, use_fma, true)
KLANG_CONFIG_DEFINE(bool, use_bmi, true)
KLANG_CONFIG_DEFINE(uint32_t, cache_line_size, 64)
KLANG_CONFIG_DEFINE(uint32_t, page_size, 4096)

}