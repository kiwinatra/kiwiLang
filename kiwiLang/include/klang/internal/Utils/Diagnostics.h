#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include <ostream>
#include <system_error>

namespace klang {

class SourceLocation;
class SourceRange;

enum class DiagnosticLevel {
    Note,
    Remark,
    Warning,
    Error,
    Fatal
};

enum class DiagnosticCategory {
    None,
    Lexical,
    Syntactic,
    Semantic,
    Type,
    Codegen,
    Optimization,
    Linker,
    Runtime,
    Memory,
    Security,
    Style,
    Documentation,
    Performance,
    Portability,
    Compatibility,
    Deprecated,
    Experimental,
    User
};

struct DiagnosticInfo {
    uint32_t id;
    DiagnosticLevel level;
    DiagnosticCategory category;
    std::string message;
    std::string explanation;
    std::string suggestion;
    std::string url;
    
    DiagnosticInfo(uint32_t id, DiagnosticLevel lvl, DiagnosticCategory cat, std::string msg)
        : id(id), level(lvl), category(cat), message(std::move(msg)) {}
};

class Diagnostic {
public:
    Diagnostic();
    Diagnostic(DiagnosticLevel level, DiagnosticCategory category, std::string message);
    Diagnostic(DiagnosticLevel level, DiagnosticCategory category, std::string message, SourceLocation location);
    Diagnostic(DiagnosticLevel level, DiagnosticCategory category, std::string message, SourceRange range);
    Diagnostic(const DiagnosticInfo& info, SourceLocation location);
    Diagnostic(const DiagnosticInfo& info, SourceRange range);
    
    DiagnosticLevel level() const;
    DiagnosticCategory category() const;
    const std::string& message() const;
    const std::string& formatted_message() const;
    
    bool has_location() const;
    SourceLocation location() const;
    
    bool has_range() const;
    SourceRange range() const;
    
    uint32_t id() const;
    const std::string& explanation() const;
    const std::string& suggestion() const;
    const std::string& url() const;
    
    void set_location(SourceLocation loc);
    void set_range(SourceRange range);
    void set_explanation(std::string explanation);
    void set_suggestion(std::string suggestion);
    void set_url(std::string url);
    
    std::string to_string() const;
    std::string to_short_string() const;
    std::string to_color_string() const;
    
    bool is_error() const;
    bool is_warning() const;
    bool is_note() const;
    bool is_fatal() const;
    
    bool operator==(const Diagnostic& other) const;
    bool operator!=(const Diagnostic& other) const;
    
    static Diagnostic note(std::string message, SourceLocation loc = {});
    static Diagnostic remark(std::string message, SourceLocation loc = {});
    static Diagnostic warning(std::string message, SourceLocation loc = {});
    static Diagnostic error(std::string message, SourceLocation loc = {});
    static Diagnostic fatal(std::string message, SourceLocation loc = {});
    
private:
    DiagnosticLevel level_ = DiagnosticLevel::Note;
    DiagnosticCategory category_ = DiagnosticCategory::None;
    uint32_t id_ = 0;
    std::string message_;
    std::string formatted_message_;
    SourceLocation location_;
    SourceRange range_;
    std::string explanation_;
    std::string suggestion_;
    std::string url_;
    
    void format_message();
};

class DiagnosticBuilder {
public:
    explicit DiagnosticBuilder(DiagnosticLevel level, DiagnosticCategory category);
    explicit DiagnosticBuilder(DiagnosticLevel level, DiagnosticCategory category, SourceLocation location);
    explicit DiagnosticBuilder(const DiagnosticInfo& info);
    
    DiagnosticBuilder& location(SourceLocation loc);
    DiagnosticBuilder& range(SourceRange range);
    
    DiagnosticBuilder& message(std::string msg);
    DiagnosticBuilder& message(std::string_view msg);
    DiagnosticBuilder& message(const char* msg);
    
    template<typename... Args>
    DiagnosticBuilder& format(const char* fmt, Args&&... args);
    
    DiagnosticBuilder& explanation(std::string exp);
    DiagnosticBuilder& suggestion(std::string sug);
    DiagnosticBuilder& url(std::string url);
    
    DiagnosticBuilder& note();
    DiagnosticBuilder& remark();
    DiagnosticBuilder& warning();
    DiagnosticBuilder& error();
    DiagnosticBuilder& fatal();
    
    DiagnosticBuilder& category(DiagnosticCategory cat);
    
    Diagnostic build() const;
    operator Diagnostic() const;
    
private:
    Diagnostic diagnostic_;
};

class Diagnostics {
public:
    Diagnostics();
    ~Diagnostics();
    
    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;
    
    Diagnostics(Diagnostics&& other) noexcept;
    Diagnostics& operator=(Diagnostics&& other) noexcept;
    
    // Reporting
    void report(const Diagnostic& diag);
    void report(DiagnosticLevel level, DiagnosticCategory category, std::string message);
    void report(DiagnosticLevel level, DiagnosticCategory category, std::string message, SourceLocation location);
    
    DiagnosticBuilder note();
    DiagnosticBuilder remark();
    DiagnosticBuilder warning();
    DiagnosticBuilder error();
    DiagnosticBuilder fatal();
    
    // Querying
    size_t error_count() const;
    size_t warning_count() const;
    size_t note_count() const;
    size_t total_count() const;
    
    bool has_errors() const;
    bool has_warnings() const;
    bool has_issues() const;
    
    const std::vector<Diagnostic>& diagnostics() const;
    std::vector<Diagnostic> errors() const;
    std::vector<Diagnostic> warnings() const;
    std::vector<Diagnostic> notes() const;
    
    // Filtering
    std::vector<Diagnostic> filter_by_level(DiagnosticLevel level) const;
    std::vector<Diagnostic> filter_by_category(DiagnosticCategory category) const;
    std::vector<Diagnostic> filter_by_file(const std::string& filename) const;
    
    // Clearing
    void clear();
    void clear_errors();
    void clear_warnings();
    void clear_notes();
    
    // Output control
    void set_output_stream(std::ostream& stream);
    void set_error_stream(std::ostream& stream);
    void set_warning_stream(std::ostream& stream);
    
    std::ostream& output_stream() const;
    std::ostream& error_stream() const;
    std::ostream& warning_stream() const;
    
    void set_color_output(bool enable);
    bool color_output() const;
    
    void set_show_column(bool show);
    bool show_column() const;
    
    void set_show_source(bool show);
    bool show_source() const;
    
    void set_show_explanations(bool show);
    bool show_explanations() const;
    
    void set_show_suggestions(bool show);
    bool show_suggestions() const;
    
    void set_max_errors(size_t max);
    size_t max_errors() const;
    
    void set_max_warnings(size_t max);
    size_t max_warnings() const;
    
    void set_warnings_as_errors(bool enable);
    bool warnings_as_errors() const;
    
    void set_pedantic(bool enable);
    bool pedantic() const;
    
    // Formatting
    std::string format_diagnostic(const Diagnostic& diag) const;
    std::string format_diagnostics() const;
    std::string format_summary() const;
    
    void print_diagnostic(const Diagnostic& diag) const;
    void print_diagnostics() const;
    void print_summary() const;
    
    // Callbacks
    using DiagnosticCallback = std::function<void(const Diagnostic&)>;
    
    void set_diagnostic_callback(DiagnosticCallback callback);
    void set_error_callback(DiagnosticCallback callback);
    void set_warning_callback(DiagnosticCallback callback);
    
    // Diagnostic IDs
    void register_diagnostic(const DiagnosticInfo& info);
    bool has_diagnostic(uint32_t id) const;
    DiagnosticInfo get_diagnostic(uint32_t id) const;
    
    void enable_diagnostic(uint32_t id);
    void disable_diagnostic(uint32_t id);
    bool is_diagnostic_enabled(uint32_t id) const;
    
    // Suppression
    void suppress_diagnostic(uint32_t id);
    void suppress_category(DiagnosticCategory category);
    void suppress_file(const std::string& filename);
    void suppress_all();
    
    void unsuppress_diagnostic(uint32_t id);
    void unsuppress_category(DiagnosticCategory category);
    void unsuppress_file(const std::string& filename);
    void unsuppress_all();
    
    bool is_suppressed(const Diagnostic& diag) const;
    
    // Error handling
    std::error_code last_error() const;
    void clear_last_error();
    
    // Statistics
    struct Stats {
        size_t total_diagnostics = 0;
        size_t total_errors = 0;
        size_t total_warnings = 0;
        size_t total_notes = 0;
        size_t suppressed_diagnostics = 0;
        size_t fatal_errors = 0;
        
        size_t lexical_errors = 0;
        size_t syntactic_errors = 0;
        size_t semantic_errors = 0;
        size_t type_errors = 0;
        size_t codegen_errors = 0;
        size_t runtime_errors = 0;
        
        double average_diagnostics_per_file = 0.0;
        size_t files_with_errors = 0;
        size_t files_with_warnings = 0;
        
        uint64_t total_processing_time_ns = 0;
    };
    
    Stats stats() const;
    void reset_stats();
    
    // Diagnostic sources
    void push_source(const std::string& source);
    void pop_source();
    const std::string& current_source() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

inline DiagnosticLevel Diagnostic::level() const {
    return level_;
}

inline DiagnosticCategory Diagnostic::category() const {
    return category_;
}

inline const std::string& Diagnostic::message() const {
    return message_;
}

inline const std::string& Diagnostic::formatted_message() const {
    return formatted_message_;
}

inline bool Diagnostic::has_location() const {
    return location_.is_valid();
}

inline bool Diagnostic::has_range() const {
    return range_.is_valid();
}

inline uint32_t Diagnostic::id() const {
    return id_;
}

inline const std::string& Diagnostic::explanation() const {
    return explanation_;
}

inline const std::string& Diagnostic::suggestion() const {
    return suggestion_;
}

inline const std::string& Diagnostic::url() const {
    return url_;
}

inline void Diagnostic::set_location(SourceLocation loc) {
    location_ = loc;
    format_message();
}

inline void Diagnostic::set_range(SourceRange range) {
    range_ = range;
}

inline void Diagnostic::set_explanation(std::string explanation) {
    explanation_ = std::move(explanation);
}

inline void Diagnostic::set_suggestion(std::string suggestion) {
    suggestion_ = std::move(suggestion);
}

inline void Diagnostic::set_url(std::string url) {
    url_ = std::move(url);
}

inline bool Diagnostic::is_error() const {
    return level_ == DiagnosticLevel::Error || level_ == DiagnosticLevel::Fatal;
}

inline bool Diagnostic::is_warning() const {
    return level_ == DiagnosticLevel::Warning;
}

inline bool Diagnostic::is_note() const {
    return level_ == DiagnosticLevel::Note || level_ == DiagnosticLevel::Remark;
}

inline bool Diagnostic::is_fatal() const {
    return level_ == DiagnosticLevel::Fatal;
}

inline bool Diagnostic::operator==(const Diagnostic& other) const {
    return level_ == other.level_ && category_ == other.category_ && message_ == other.message_ && location_ == other.location_;
}

inline bool Diagnostic::operator!=(const Diagnostic& other) const {
    return !(*this == other);
}

inline Diagnostic Diagnostic::note(std::string message, SourceLocation loc) {
    return Diagnostic(DiagnosticLevel::Note, DiagnosticCategory::None, std::move(message), loc);
}

inline Diagnostic Diagnostic::remark(std::string message, SourceLocation loc) {
    return Diagnostic(DiagnosticLevel::Remark, DiagnosticCategory::None, std::move(message), loc);
}

inline Diagnostic Diagnostic::warning(std::string message, SourceLocation loc) {
    return Diagnostic(DiagnosticLevel::Warning, DiagnosticCategory::None, std::move(message), loc);
}

inline Diagnostic Diagnostic::error(std::string message, SourceLocation loc) {
    return Diagnostic(DiagnosticLevel::Error, DiagnosticCategory::None, std::move(message), loc);
}

inline Diagnostic Diagnostic::fatal(std::string message, SourceLocation loc) {
    return Diagnostic(DiagnosticLevel::Fatal, DiagnosticCategory::None, std::move(message), loc);
}

inline DiagnosticBuilder::DiagnosticBuilder(DiagnosticLevel level, DiagnosticCategory category)
    : diagnostic_(level, category, "") {}

inline DiagnosticBuilder::DiagnosticBuilder(DiagnosticLevel level, DiagnosticCategory category, SourceLocation location)
    : diagnostic_(level, category, "", location) {}

inline DiagnosticBuilder::DiagnosticBuilder(const DiagnosticInfo& info)
    : diagnostic_(info, SourceLocation()) {}

inline DiagnosticBuilder& DiagnosticBuilder::location(SourceLocation loc) {
    diagnostic_.set_location(loc);
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::range(SourceRange range) {
    diagnostic_.set_range(range);
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::message(std::string msg) {
    diagnostic_ = Diagnostic(diagnostic_.level(), diagnostic_.category(), std::move(msg), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::message(std::string_view msg) {
    return message(std::string(msg));
}

inline DiagnosticBuilder& DiagnosticBuilder::message(const char* msg) {
    return message(std::string(msg));
}

inline DiagnosticBuilder& DiagnosticBuilder::explanation(std::string exp) {
    diagnostic_.set_explanation(std::move(exp));
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::suggestion(std::string sug) {
    diagnostic_.set_suggestion(std::move(sug));
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::url(std::string url) {
    diagnostic_.set_url(std::move(url));
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::note() {
    diagnostic_ = Diagnostic(DiagnosticLevel::Note, diagnostic_.category(), diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::remark() {
    diagnostic_ = Diagnostic(DiagnosticLevel::Remark, diagnostic_.category(), diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::warning() {
    diagnostic_ = Diagnostic(DiagnosticLevel::Warning, diagnostic_.category(), diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::error() {
    diagnostic_ = Diagnostic(DiagnosticLevel::Error, diagnostic_.category(), diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::fatal() {
    diagnostic_ = Diagnostic(DiagnosticLevel::Fatal, diagnostic_.category(), diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline DiagnosticBuilder& DiagnosticBuilder::category(DiagnosticCategory cat) {
    diagnostic_ = Diagnostic(diagnostic_.level(), cat, diagnostic_.message(), diagnostic_.location());
    return *this;
}

inline Diagnostic DiagnosticBuilder::build() const {
    return diagnostic_;
}

inline DiagnosticBuilder::operator Diagnostic() const {
    return diagnostic_;
}

inline DiagnosticBuilder Diagnostics::note() {
    return DiagnosticBuilder(DiagnosticLevel::Note, DiagnosticCategory::None);
}

inline DiagnosticBuilder Diagnostics::remark() {
    return DiagnosticBuilder(DiagnosticLevel::Remark, DiagnosticCategory::None);
}

inline DiagnosticBuilder Diagnostics::warning() {
    return DiagnosticBuilder(DiagnosticLevel::Warning, DiagnosticCategory::None);
}

inline DiagnosticBuilder Diagnostics::error() {
    return DiagnosticBuilder(DiagnosticLevel::Error, DiagnosticCategory::None);
}

inline DiagnosticBuilder Diagnostics::fatal() {
    return DiagnosticBuilder(DiagnosticLevel::Fatal, DiagnosticCategory::None);
}

inline size_t Diagnostics::error_count() const {
    return has_errors() ? 0 : 0; // Implementation would track this
}

inline size_t Diagnostics::warning_count() const {
    return has_warnings() ? 0 : 0; // Implementation would track this
}

inline size_t Diagnostics::note_count() const {
    return 0; // Implementation would track this
}

inline size_t Diagnostics::total_count() const {
    return error_count() + warning_count() + note_count();
}

inline bool Diagnostics::has_errors() const {
    return error_count() > 0;
}

inline bool Diagnostics::has_warnings() const {
    return warning_count() > 0;
}

inline bool Diagnostics::has_issues() const {
    return has_errors() || has_warnings();
}

} // namespace klang