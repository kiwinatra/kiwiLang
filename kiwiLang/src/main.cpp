// main execution lib. it COULD be changed
#include "kiwiLang/Compiler/Driver.h"
#include "kiwiLang/Compiler/Version.h"
#include "kiwiLang/Utils/FileSystem.h"
#include "kiwiLang/Utils/CommandLine.h"
#include "kiwiLang/Utils/ErrorHandler.h"
#include <iostream>
#include <memory>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace kiwiLang;

void printBanner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║               KiwiLang Compiler              ║\n";
    std::cout << "║         Version " << getVersionString() << "              ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printHelp() {
    std::cout << "Usage: kiwic [options] file...\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help               Display this information\n";
    std::cout << "  -v, --version            Display compiler version\n";
    std::cout << "  -V, --verbose            Verbose output\n";
    std::cout << "  -o <file>                Write output to <file>\n";
    std::cout << "  -c                       Compile only, do not link\n";
    std::cout << "  -S                       Compile to assembly\n";
    std::cout << "  -emit-llvm               Emit LLVM IR\n";
    std::cout << "  -emit-ast                Emit AST\n";
    std::cout << "  -emit-tokens             Emit tokens\n";
    std::cout << "  -I <dir>                 Add directory to include search path\n";
    std::cout << "  -L <dir>                 Add directory to library search path\n";
    std::cout << "  -l <library>             Link with library\n";
    std::cout << "  -D <macro>[=<value>]     Define preprocessor macro\n";
    std::cout << "  -U <macro>               Undefine preprocessor macro\n";
    std::cout << "  -O0, -O1, -O2, -O3       Set optimization level\n";
    std::cout << "  -Os                      Optimize for size\n";
    std::cout << "  -Oz                      Optimize aggressively for size\n";
    std::cout << "  -g                       Generate debug information\n";
    std::cout << "  -pg                      Generate profiling information\n";
    std::cout << "  -std=<standard>          Language standard (default: 2023)\n";
    std::cout << "  -pedantic                Warn about language extensions\n";
    std::cout << "  -Wall                    Enable all warnings\n";
    std::cout << "  -Werror                  Treat warnings as errors\n";
    std::cout << "  -Wno-<warning>           Disable specific warning\n";
    std::cout << "  -target <triple>         Target architecture\n";
    std::cout << "  -march=<arch>            Target architecture\n";
    std::cout << "  -mtune=<cpu>             Tune for specific CPU\n";
    std::cout << "  -m<feature>              Enable target feature\n";
    std::cout << "  -fPIC                    Generate position-independent code\n";
    std::cout << "  -fPIE                    Generate position-independent executable\n";
    std::cout << "  -shared                  Generate shared library\n";
    std::cout << "  -static                  Link statically\n";
    std::cout << "  -nostdlib                Do not link standard library\n";
    std::cout << "  -nostartfiles            Do not link startup files\n";
    std::cout << "  -nodefaultlibs           Do not link default libraries\n";
    std::cout << "  -save-temps              Save temporary files\n";
    std::cout << "  -time                    Time compilation phases\n";
    std::cout << "  --dump-config            Dump compiler configuration\n";
    std::cout << "  --print-search-dirs      Print search directories\n";
    std::cout << "  @<file>                  Read options from file\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  kiwic hello.kwl          Compile and link executable\n";
    std::cout << "  kiwic -c hello.kwl       Compile to object file\n";
    std::cout << "  kiwic -S hello.kwl       Compile to assembly\n";
    std::cout << "  kiwic -emit-llvm hello.kwl  Emit LLVM IR\n";
    std::cout << "\n";
}

void printVersion() {
    std::cout << "KiwiLang Compiler " << getVersionString() << "\n";
    std::cout << "Build: " << getBuildDate() << "\n";
    std::cout << "Target: " << getTargetTriple() << "\n";
    std::cout << "Thread model: " << getThreadModel() << "\n";
    std::cout << "InstalledDir: " << getInstalledDir() << "\n";
    std::cout << "\n";
}

void dumpConfig(const CompileOptions& options) {
    std::cout << "Compiler Configuration:\n";
    std::cout << "  Input files: ";
    for (const auto& file : options.inputFiles) {
        std::cout << file << " ";
    }
    std::cout << "\n";
    std::cout << "  Output file: " << options.outputFile << "\n";
    std::cout << "  Output type: " << static_cast<int>(options.outputType) << "\n";
    std::cout << "  Optimization: " << static_cast<int>(options.optimizationLevel) << "\n";
    std::cout << "  Debug info: " << (options.debugInfo ? "yes" : "no") << "\n";
    std::cout << "  Target triple: " << options.targetTriple << "\n";
    std::cout << "  Include paths: ";
    for (const auto& path : options.includePaths) {
        std::cout << path << " ";
    }
    std::cout << "\n";
    std::cout << "  Library paths: ";
    for (const auto& path : options.libraryPaths) {
        std::cout << path << " ";
    }
    std::cout << "\n";
    std::cout << "  Libraries: ";
    for (const auto& lib : options.libraries) {
        std::cout << lib << " ";
    }
    std::cout << "\n";
}

class Timer {
public:
    Timer(std::string name) : name_(std::move(name)), start_(std::chrono::high_resolution_clock::now()) {}
    
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        if (!name_.empty()) {
            std::cerr << "[" << name_ << "] " << duration.count() << "ms\n";
        }
    }

private:
    std::string name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

int compileWithTiming(Driver& driver, const CompileOptions& options) {
    Timer totalTimer("Total compilation");
    
    auto result = driver.compile(options);
    
    if (!result.success) {
        std::cerr << "Compilation failed: " << result.errorMessage << "\n";
        return EXIT_FAILURE;
    }
    
    if (options.verbose) {
        std::cout << "Compilation successful\n";
        if (!result.warnings.empty()) {
            std::cout << "\nWarnings:\n";
            for (const auto& warning : result.warnings) {
                std::cout << warning << "\n";
            }
        }
    }
    
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable UTF-8 console output on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    
    // Initialize compiler
    Compiler::initialize();
    
    // Parse command line
    CommandLineParser parser;
    parser.parse(argc, argv);
    
    if (parser.hasHelp()) {
        printBanner();
        printHelp();
        return EXIT_SUCCESS;
    }
    
    if (parser.hasVersion()) {
        printVersion();
        return EXIT_SUCCESS;
    }
    
    if (parser.inputFiles().empty()) {
        std::cerr << "error: no input files\n";
        printHelp();
        return EXIT_FAILURE;
    }
    
    // Create driver
    Driver driver;
    
    // Setup compile options
    CompileOptions options;
    options.inputFiles = parser.inputFiles();
    options.outputFile = parser.getOutputFile();
    options.outputType = parser.getOutputType();
    options.optimizationLevel = parser.getOptimizationLevel();
    options.debugInfo = parser.hasDebugInfo();
    options.profileInfo = parser.hasProfileInfo();
    options.verbose = parser.isVerbose();
    options.emitLLVM = parser.shouldEmitLLVM();
    options.emitAST = parser.shouldEmitAST();
    options.emitTokens = parser.shouldEmitTokens();
    options.targetTriple = parser.getTargetTriple();
    options.includePaths = parser.getIncludePaths();
    options.libraryPaths = parser.getLibraryPaths();
    options.libraries = parser.getLibraries();
    options.defines = parser.getDefines();
    options.undefines = parser.getUndefines();
    options.warnings = parser.getWarnings();
    options.pedantic = parser.isPedantic();
    options.warningsAsErrors = parser.warningsAsErrors();
    options.positionIndependent = parser.isPositionIndependent();
    options.sharedLibrary = parser.isSharedLibrary();
    options.staticLinking = parser.isStaticLinking();
    options.noStdLib = parser.noStdLib();
    options.noStartFiles = parser.noStartFiles();
    options.noDefaultLibs = parser.noDefaultLibs();
    options.saveTemps = parser.saveTemps();
    options.timing = parser.hasTiming();
    
    if (parser.dumpConfig()) {
        dumpConfig(options);
        return EXIT_SUCCESS;
    }
    
    if (parser.printSearchDirs()) {
        // TODO: Implement search directory printing
        return EXIT_SUCCESS;
    }
    
    // Validate input files
    for (const auto& file : options.inputFiles) {
        if (!FileSystem::exists(file)) {
            std::cerr << "error: file not found: " << file << "\n";
            return EXIT_FAILURE;
        }
        
        if (!FileSystem::isRegularFile(file)) {
            std::cerr << "error: not a regular file: " << file << "\n";
            return EXIT_FAILURE;
        }
        
        std::string ext = FileSystem::extension(file);
        if (ext != ".kwl" && ext != ".kiwi") {
            std::cerr << "warning: unknown file extension: " << ext << "\n";
        }
    }
    
    // Check output directory
    if (!options.outputFile.empty()) {
        std::string outputDir = FileSystem::parentPath(options.outputFile);
        if (!outputDir.empty() && !FileSystem::exists(outputDir)) {
            if (!FileSystem::createDirectories(outputDir)) {
                std::cerr << "error: cannot create output directory: " << outputDir << "\n";
                return EXIT_FAILURE;
            }
        }
    }
    
    try {
        if (options.timing) {
            return compileWithTiming(driver, options);
        } else {
            auto result = driver.compile(options);
            
            if (!result.success) {
                std::cerr << "error: " << result.errorMessage << "\n";
                return EXIT_FAILURE;
            }
            
            return EXIT_SUCCESS;
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "fatal error: unknown exception\n";
        return EXIT_FAILURE;
    }
}

// Entry point for Windows subsystem
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (!argvW) {
        return EXIT_FAILURE;
    }
    
    // Convert wide char arguments to UTF-8
    std::vector<std::string> argvStrings;
    std::vector<char*> argvPtrs;
    
    argvStrings.reserve(argc);
    argvPtrs.reserve(argc + 1);
    
    for (int i = 0; i < argc; ++i) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, &str[0], size, nullptr, nullptr);
        str.resize(size - 1); // Remove null terminator
        argvStrings.push_back(str);
        argvPtrs.push_back(argvStrings.back().data());
    }
    argvPtrs.push_back(nullptr);
    
    LocalFree(argvW);
    
    return main(argc, argvPtrs.data());
}
#endif