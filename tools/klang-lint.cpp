#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

struct SourceLocation {
    size_t line = 1;
    size_t column = 1;
    size_t offset = 0;
    fs::path file;

    std::string toString() const {
        std::ostringstream oss;
        oss << file.filename().string() << ":" << line << ":" << column;
        return oss.str();
    }
};

struct Diagnostic {
    enum class Severity {
        Error,
        Warning,
        Info,
        Style
    };

    Severity severity;
    std::string message;
    SourceLocation location;
    std::string rule;
    std::string suggestion;

    Diagnostic(Severity s, std::string m, SourceLocation loc, std::string r = "", std::string sug = "")
        : severity(s), message(std::move(m)), location(std::move(loc)), rule(std::move(r)), suggestion(std::move(sug)) {}
};

class SourceFile {
public:
    explicit SourceFile(const fs::path& path) : path_(path) {
        load();
    }

    const std::string& content() const { return content_; }
    const fs::path& path() const { return path_; }
    size_t size() const { return content_.size(); }

    char operator[](size_t pos) const {
        return pos < content_.size() ? content_[pos] : '\0';
    }

    std::string_view substr(size_t pos, size_t len = std::string_view::npos) const {
        if (pos >= content_.size()) return {};
        return std::string_view(content_.data() + pos, 
            std::min(len, content_.size() - pos));
    }

    SourceLocation getLocation(size_t offset) const {
        SourceLocation loc;
        loc.file = path_;
        loc.offset = offset;

        for (size_t i = 0; i < std::min(offset, content_.size()); ++i) {
            if (content_[i] == '\n') {
                ++loc.line;
                loc.column = 1;
            } else {
                ++loc.column;
            }
        }
        return loc;
    }

private:
    void load() {
        std::ifstream file(path_, std::ios::binary);
        if (!file) return;

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        content_.resize(size);
        file.read(content_.data(), size);
    }

    fs::path path_;
    std::string content_;
};

class Token {
public:
    enum class Type {
        Identifier,
        Keyword,
        Number,
        String,
        Operator,
        Punctuator,
        Comment,
        Whitespace,
        Newline,
        EOF
    };

    Type type;
    std::string_view text;
    SourceLocation start;
    SourceLocation end;
};

class Rule {
public:
    virtual ~Rule() = default;
    virtual void check(const SourceFile& file, std::vector<Diagnostic>& diagnostics) = 0;
    virtual std::string name() const = 0;
    virtual Diagnostic::Severity defaultSeverity() const = 0;
};

class NamingConventionRule : public Rule {
public:
    void check(const SourceFile& file, std::vector<Diagnostic>& diagnostics) override {
        static const std::regex typeRegex(R"(\b(class|struct|enum|interface)\s+([a-zA-Z_][a-zA-Z0-9_]*))");
        static const std::regex funcRegex(R"(\b(fn|function)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\()");
        static const std::regex varRegex(R"(\b(let|var|const)\s+([a-zA-Z_][a-zA-Z0-9_]*))");

        std::string_view content = file.content();
        std::cregex_iterator it(content.begin(), content.end(), typeRegex);
        std::cregex_iterator end;

        for (; it != end; ++it) {
            std::string name = (*it)[2];
            if (!std::isupper(name[0])) {
                size_t pos = it->position(2);
                diagnostics.emplace_back(
                    Diagnostic::Severity::Warning,
                    "Type names should start with uppercase letter: " + name,
                    file.getLocation(pos),
                    "naming-convention",
                    "Consider renaming to " + capitalize(name)
                );
            }
        }
    }

    std::string name() const override { return "naming-convention"; }
    Diagnostic::Severity defaultSeverity() const override { return Diagnostic::Severity::Warning; }

private:
    static std::string capitalize(const std::string& str) {
        if (str.empty()) return str;
        std::string result = str;
        result[0] = std::toupper(result[0]);
        return result;
    }
};

class CyclomaticComplexityRule : public Rule {
public:
    void check(const SourceFile& file, std::vector<Diagnostic>& diagnostics) override {
        std::string_view content = file.content();
        size_t pos = 0;
        size_t depth = 0;
        size_t complexity = 1;

        while (pos < content.size()) {
            if (content.substr(pos, 2) == "fn") {
                size_t fnStart = pos;
                pos += 2;
                while (pos < content.size() && content[pos] != '{') ++pos;
                if (pos < content.size() && content[pos] == '{') {
                    size_t bodyStart = pos;
                    depth = 1;
                    complexity = 1;
                    
                    for (++pos; pos < content.size() && depth > 0; ++pos) {
                        if (content[pos] == '{') ++depth;
                        else if (content[pos] == '}') --depth;
                        else if (content.substr(pos, 2) == "if") complexity++;
                        else if (content.substr(pos, 5) == "while") complexity++;
                        else if (content.substr(pos, 3) == "for") complexity++;
                        else if (content.substr(pos, 5) == "match") complexity++;
                    }

                    if (complexity > maxComplexity) {
                        diagnostics.emplace_back(
                            Diagnostic::Severity::Warning,
                            "Function cyclomatic complexity too high: " + std::to_string(complexity),
                            file.getLocation(fnStart),
                            "cyclomatic-complexity",
                            "Consider refactoring into smaller functions"
                        );
                    }
                }
            }
            ++pos;
        }
    }

    std::string name() const override { return "cyclomatic-complexity"; }
    Diagnostic::Severity defaultSeverity() const override { return Diagnostic::Severity::Warning; }

private:
    static constexpr size_t maxComplexity = 10;
};

class UnusedVariableRule : public Rule {
public:
    void check(const SourceFile& file, std::vector<Diagnostic>& diagnostics) override {
        std::unordered_map<std::string, std::vector<SourceLocation>> declarations;
        std::unordered_set<std::string> usages;

        std::string_view content = file.content();
        static const std::regex declRegex(R"(\b(let|var)\s+([a-zA-Z_][a-zA-Z0-9_]*))");
        static const std::regex usageRegex(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\b)");

        std::cregex_iterator declIt(content.begin(), content.end(), declRegex);
        for (; declIt != std::cregex_iterator(); ++declIt) {
            std::string var = (*declIt)[2];
            declarations[var].push_back(file.getLocation(declIt->position(2)));
        }

        std::cregex_iterator usageIt(content.begin(), content.end(), usageRegex);
        for (; usageIt != std::cregex_iterator(); ++usageIt) {
            std::string var = (*usageIt)[1];
            if (declarations.find(var) != declarations.end()) {
                usages.insert(var);
            }
        }

        for (const auto& [var, locs] : declarations) {
            if (usages.find(var) == usages.end()) {
                for (const auto& loc : locs) {
                    diagnostics.emplace_back(
                        Diagnostic::Severity::Warning,
                        "Unused variable: " + var,
                        loc,
                        "unused-variable",
                        "Consider removing or using this variable"
                    );
                }
            }
        }
    }

    std::string name() const override { return "unused-variable"; }
    Diagnostic::Severity defaultSeverity() const override { return Diagnostic::Severity::Warning; }
};

class Linter {
public:
    Linter() {
        registerRule(std::make_unique<NamingConventionRule>());
        registerRule(std::make_unique<CyclomaticComplexityRule>());
        registerRule(std::make_unique<UnusedVariableRule>());
    }

    void addRule(std::unique_ptr<Rule> rule) {
        rules_[rule->name()] = std::move(rule);
    }

    std::vector<Diagnostic> lintFile(const fs::path& filepath) {
        std::vector<Diagnostic> diagnostics;
        
        if (!fs::exists(filepath)) {
            std::cerr << "File not found: " << filepath << std::endl;
            return diagnostics;
        }

        SourceFile file(filepath);
        
        for (const auto& [name, rule] : rules_) {
            rule->check(file, diagnostics);
        }

        return diagnostics;
    }

    std::vector<Diagnostic> lintDirectory(const fs::path& dirpath) {
        std::vector<Diagnostic> diagnostics;
        
        if (!fs::exists(dirpath)) {
            std::cerr << "Directory not found: " << dirpath << std::endl;
            return diagnostics;
        }

        for (const auto& entry : fs::recursive_directory_iterator(dirpath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".kiwi") {
                auto fileDiagnostics = lintFile(entry.path());
                diagnostics.insert(diagnostics.end(), 
                    fileDiagnostics.begin(), fileDiagnostics.end());
            }
        }

        return diagnostics;
    }

private:
    void registerRule(std::unique_ptr<Rule> rule) {
        rules_[rule->name()] = std::move(rule);
    }

    std::unordered_map<std::string, std::unique_ptr<Rule>> rules_;
};

void printDiagnostic(const Diagnostic& diag, bool colors) {
    const char* colorReset = colors ? "\033[0m" : "";
    const char* colorRed = colors ? "\033[1;31m" : "";
    const char* colorYellow = colors ? "\033[1;33m" : "";
    const char* colorBlue = colors ? "\033[1;34m" : "";
    const char* colorCyan = colors ? "\033[1;36m" : "";

    const char* severityStr = "";
    const char* severityColor = "";
    
    switch (diag.severity) {
        case Diagnostic::Severity::Error:
            severityStr = "error";
            severityColor = colorRed;
            break;
        case Diagnostic::Severity::Warning:
            severityStr = "warning";
            severityColor = colorYellow;
            break;
        case Diagnostic::Severity::Info:
            severityStr = "info";
            severityColor = colorBlue;
            break;
        case Diagnostic::Severity::Style:
            severityStr = "style";
            severityColor = colorCyan;
            break;
    }

    std::cout << diag.location.toString() << ": " 
              << severityColor << severityStr << colorReset
              << ": " << diag.message;
    
    if (!diag.rule.empty()) {
        std::cout << " [" << diag.rule << "]";
    }
    
    if (!diag.suggestion.empty()) {
        std::cout << "\n    suggestion: " << diag.suggestion;
    }
    
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    bool useColors = true;
    std::vector<fs::path> paths;
    std::unordered_set<std::string> enabledRules;
    std::unordered_set<std::string> disabledRules;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-color") {
            useColors = false;
        } else if (arg == "--enable" && i + 1 < argc) {
            enabledRules.insert(argv[++i]);
        } else if (arg == "--disable" && i + 1 < argc) {
            disabledRules.insert(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: kiwiLang-lint [options] <files or directories>\n"
                      << "Options:\n"
                      << "  --no-color          Disable colored output\n"
                      << "  --enable RULE       Enable specific rule\n"
                      << "  --disable RULE      Disable specific rule\n"
                      << "  --help              Show this help message\n";
            return 0;
        } else if (arg[0] != '-') {
            paths.emplace_back(arg);
        }
    }

    if (paths.empty()) {
        std::cerr << "Error: No files or directories specified" << std::endl;
        return 1;
    }

    Linter linter;
    std::vector<Diagnostic> allDiagnostics;

    for (const auto& path : paths) {
        std::vector<Diagnostic> diagnostics;
        
        if (fs::is_directory(path)) {
            diagnostics = linter.lintDirectory(path);
        } else {
            diagnostics = linter.lintFile(path);
        }
        
        allDiagnostics.insert(allDiagnostics.end(), 
            diagnostics.begin(), diagnostics.end());
    }

    std::sort(allDiagnostics.begin(), allDiagnostics.end(),
        [](const Diagnostic& a, const Diagnostic& b) {
            if (a.location.file != b.location.file) {
                return a.location.file < b.location.file;
            }
            if (a.location.line != b.location.line) {
                return a.location.line < b.location.line;
            }
            return a.location.column < b.location.column;
        });

    for (const auto& diag : allDiagnostics) {
        printDiagnostic(diag, useColors);
    }

    size_t errorCount = std::count_if(allDiagnostics.begin(), allDiagnostics.end(),
        [](const Diagnostic& d) { return d.severity == Diagnostic::Severity::Error; });
    size_t warningCount = std::count_if(allDiagnostics.begin(), allDiagnostics.end(),
        [](const Diagnostic& d) { return d.severity == Diagnostic::Severity::Warning; });

    if (errorCount > 0 || warningCount > 0) {
        std::cout << "\nFound " << errorCount << " error(s) and " 
                  << warningCount << " warning(s)" << std::endl;
    }

    return errorCount > 0 ? 1 : 0;
}