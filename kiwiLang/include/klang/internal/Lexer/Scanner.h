include/klang/internal/Lexer/Scanner.h
#pragma once
#include "Token.h"
#include "klang/internal/Diagnostics/Diagnostic.h"
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace klang {
namespace internal {

class Scanner {
public:
    explicit Scanner(std::string_view source, DiagnosticReporter& diagnostics);
    
    std::vector<Token> scanTokens();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }

private:
    std::string_view source_;
    std::vector<Token> tokens_;
    DiagnosticReporter& diagnostics_;
    
    std::size_t start_;
    std::size_t current_;
    std::size_t line_;
    std::size_t column_;
    std::size_t offset_;
    
    std::unordered_map<std::string, TokenType> keywords_;
    
    void initKeywords();
    
    void scanToken();
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isHexDigit(char c) const;
    bool isBinaryDigit(char c) const;
    bool isOctalDigit(char c) const;
    
    void addToken(TokenType type);
    void addToken(TokenType type, double value);
    void addToken(TokenType type, int64_t value);
    void addToken(TokenType type, std::string value);
    void addToken(TokenType type, char value);
    
    void identifier();
    void number();
    void string();
    void character();
    void templateString();
    
    void blockComment();
    void lineComment();
    
    void scanNumber();
    void scanHexNumber();
    void scanBinaryNumber();
    void scanOctalNumber();
    void scanFloat();
    
    void scanOperator();
    void scanCompoundOperator(char first);
    
    SourceLocation currentLocation() const;
    SourceLocation startLocation() const;
    
    void error(std::string_view message);
    void errorAt(std::size_t position, std::string_view message);
    
    void skipWhitespace();
    void skipNewline();
    
    char getEscapedChar();
    uint32_t getUnicodeCodePoint();
    std::string parseEscapeSequence();
    
    static bool isSeparator(char c);
    static bool isOperatorStart(char c);
    static TokenType compoundOperatorType(char first, char second, char third);
    
    struct ScanState {
        std::size_t start;
        std::size_t current;
        std::size_t line;
        std::size_t column;
        std::size_t offset;
    };
    
    ScanState saveState() const;
    void restoreState(const ScanState& state);
    
    void pushTemplatePart();
    void scanTemplateExpression();
    
    std::vector<ScanState> stateStack_;
    bool inTemplateString_;
    int braceDepth_;
    
    static constexpr std::size_t MAX_IDENTIFIER_LENGTH = 256;
    static constexpr std::size_t MAX_STRING_LENGTH = 65536;
    static constexpr std::size_t MAX_NUMBER_DIGITS = 128;
};

class Lexer {
public:
    explicit Lexer(std::string_view source, DiagnosticReporter& diagnostics);
    
    TokenStream lex();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }

private:
    Scanner scanner_;
    DiagnosticReporter& diagnostics_;
    
    std::vector<Token> tokens_;
    
    void processTokens();
    void validateTokens();
    void reportLexicalErrors();
    
    bool validateIdentifier(const Token& token);
    bool validateNumber(const Token& token);
    bool validateString(const Token& token);
    
    static bool isReservedWord(std::string_view word);
    static bool isFutureReservedWord(std::string_view word);
    // ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ 67
    static constexpr std::size_t MAX_TOKENS = 67676767;
};

} // namespace internal
} // namespace klang