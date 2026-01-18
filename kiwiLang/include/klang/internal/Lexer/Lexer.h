include/klang/internal/Lexer/Lexer.h
#pragma once
#include "Scanner.h"
#include "Token.h"
#include "klang/internal/Diagnostics/Diagnostic.h"
#include "klang/internal/Utils/Unicode.h"
#include <string_view>
#include <vector>
#include <memory>
#include <stack>

namespace klang {
namespace internal {

class LexerContext {
public:
    explicit LexerContext(std::string_view source, DiagnosticReporter& diagnostics);
    
    void reset(std::string_view source);
    
    TokenStream lex();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }
    std::string_view source() const { return source_; }
    
    void pushState();
    void popState();
    void restoreState();

private:
    std::string_view source_;
    DiagnosticReporter& diagnostics_;
    std::stack<std::size_t> stateStack_;
    std::size_t currentPosition_;
    std::size_t currentLine_;
    std::size_t currentColumn_;
    
    struct SavedState {
        std::size_t position;
        std::size_t line;
        std::size_t column;
        std::vector<Token> tokens;
    };
    
    SavedState saveState() const;
    void restoreState(const SavedState& state);
};

class IncrementalLexer {
public:
    explicit IncrementalLexer(DiagnosticReporter& diagnostics);
    
    TokenStream lexIncremental(std::string_view source, std::size_t from, std::size_t to);
    void updateSource(std::string_view newSource, std::size_t editStart, std::size_t editEnd);
    
    const std::vector<Token>& currentTokens() const { return tokens_; }
    
    struct ChangeRange {
        std::size_t tokenStart;
        std::size_t tokenEnd;
        bool needsFullRescan;
    };
    
    ChangeRange computeAffectedRange(std::size_t editStart, std::size_t editEnd) const;

private:
    DiagnosticReporter& diagnostics_;
    std::string source_;
    std::vector<Token> tokens_;
    std::vector<std::size_t> lineStarts_;
    
    void rebuildLineStarts();
    std::size_t lineFromPosition(std::size_t position) const;
    std::size_t columnFromPosition(std::size_t position) const;
    
    SourceLocation locationFromPosition(std::size_t position) const;
    
    void rescanRange(std::size_t start, std::size_t end);
    void mergeTokens(std::vector<Token>&& newTokens, std::size_t replaceStart, std::size_t replaceEnd);
    
    static bool tokensEqual(const Token& a, const Token& b);
    static bool tokensCompatible(const Token& a, const Token& b);
    
    static constexpr std::size_t MAX_INCREMENTAL_RANGE = 4096;
};

class LookaheadBuffer {
public:
    explicit LookaheadBuffer(LexerContext& context);
    
    Token peek(std::size_t offset = 0);
    bool match(TokenType type);
    bool check(TokenType type) const;
    
    Token consume();
    Token consume(TokenType expected, std::string_view errorMessage);
    
    bool isAtEnd() const;
    
    void rewind(std::size_t count);
    std::size_t position() const;

private:
    LexerContext& context_;
    std::vector<Token> buffer_;
    std::size_t currentIndex_;
    std::size_t bufferStart_;
    
    void ensureBuffer(std::size_t requiredSize);
    Token fetchNextToken();
    
    static constexpr std::size_t BUFFER_SIZE = 64;
};

class TokenFilter {
public:
    virtual ~TokenFilter() = default;
    
    virtual Token filter(const Token& token) = 0;
    virtual void reset() = 0;
};

class CommentFilter : public TokenFilter {
public:
    Token filter(const Token& token) override;
    void reset() override {}
};

class WhitespaceFilter : public TokenFilter {
public:
    Token filter(const Token& token) override;
    void reset() override {}
};

class PreprocessorFilter : public TokenFilter {
public:
    explicit PreprocessorFilter(std::string_view source, DiagnosticReporter& diagnostics);
    
    Token filter(const Token& token) override;
    void reset() override;
    
    const std::vector<std::string>& definedMacros() const { return definedMacros_; }
    
private:
    std::string_view source_;
    DiagnosticReporter& diagnostics_;
    std::vector<std::string> definedMacros_;
    std::stack<bool> conditionStack_;
    bool skipping_;
    
    bool evaluateCondition(std::string_view condition);
    void processDirective(std::string_view directive);
    
    static std::string expandMacros(std::string_view input, 
                                   const std::vector<std::string>& macros);
};

class LexerPipeline {
public:
    explicit LexerPipeline(std::string_view source, DiagnosticReporter& diagnostics);
    
    void addFilter(std::unique_ptr<TokenFilter> filter);
    
    TokenStream lex();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }

private:
    std::string_view source_;
    DiagnosticReporter& diagnostics_;
    std::vector<std::unique_ptr<TokenFilter>> filters_;
    LexerContext context_;
};

class TokenCache {
public:
    explicit TokenCache(LexerContext& context);
    
    const Token& getToken(std::size_t position);
    std::vector<Token> getTokens(std::size_t start, std::size_t end);
    
    void invalidate(std::size_t start, std::size_t end);
    void clear();
    
    std::size_t cachedTokens() const { return cache_.size(); }

private:
    LexerContext& context_;
    std::vector<Token> cache_;
    std::vector<bool> valid_;
    std::size_t cacheStart_;
    
    void ensureCache(std::size_t position);
    void fillCache(std::size_t start, std::size_t end);
    
    static constexpr std::size_t CACHE_BLOCK_SIZE = 1024;
};

class LexerFactory {
public:
    static std::unique_ptr<LexerContext> createContext(std::string_view source, 
                                                      DiagnosticReporter& diagnostics);
    static std::unique_ptr<IncrementalLexer> createIncrementalLexer(DiagnosticReporter& diagnostics);
    static std::unique_ptr<LexerPipeline> createPipeline(std::string_view source,
                                                        DiagnosticReporter& diagnostics);
    
    static TokenStream lexString(std::string_view source, DiagnosticReporter& diagnostics);
    static TokenStream lexFile(std::string_view filename, DiagnosticReporter& diagnostics);
};

} // namespace internal
} // namespace klang