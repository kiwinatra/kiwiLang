include/kiwiLang/internal/Lexer/Token.h
#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <cstdint>
#include "kiwiLang/internal/Utils/SourceLocation.h"

namespace kiwiLang {
namespace internal {

enum class TokenType : uint16_t {
    // Single-character tokens
    LEFT_PAREN = 0, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET, COMMA, DOT, SEMICOLON, COLON,
    QUESTION, AT, HASH, DOLLAR, BACKTICK,
    
    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, RIGHT_SHIFT, UNSIGNED_RIGHT_SHIFT,
    LESS, LESS_EQUAL, LEFT_SHIFT,
    PLUS, PLUS_EQUAL, PLUS_PLUS,
    MINUS, MINUS_EQUAL, MINUS_MINUS, RIGHT_ARROW,
    STAR, STAR_EQUAL, STAR_STAR,
    SLASH, SLASH_EQUAL,
    PERCENT, PERCENT_EQUAL,
    AMPERSAND, AMPERSAND_EQUAL, AMPERSAND_AMPERSAND,
    PIPE, PIPE_EQUAL, PIPE_PIPE,
    CARET, CARET_EQUAL,
    TILDE,
    DOT_DOT, DOT_DOT_DOT,
    COLON_COLON, COLON_EQUAL,
    FAT_ARROW,
    
    // Literals
    IDENTIFIER, STRING, NUMBER, CHAR,
    
    // Keywords
    AND, AS, ASYNC, AWAIT, BREAK, CASE, CATCH, CLASS, CONST,
    CONTINUE, DEFAULT, DEFER, DO, ELSE, ENUM, EXPORT, EXTENDS,
    FALSE, FINALLY, FOR, FROM, FUNC, IF, IMPL, IMPORT, IN,
    INTERFACE, IS, LET, LOOP, MATCH, MODULE, MUT, NIL,
    NOT_IN, OF, OR, PRIVATE, PROTECTED, PUBLIC, RETURN,
    SELF, STATIC, STRUCT, SUPER, SWITCH, THEN, THROW, TRUE,
    TRY, TYPE, TYPEOF, UNSAFE, USE, VAR, VIRTUAL, WHILE,
    WITH, YIELD,
    
    // Type keywords
    BOOL, INT, FLOAT, DOUBLE, STR, BYTE, ANY, VOID,
    
    // Special
    ERROR, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
    std::variant<std::monostate, double, int64_t, std::string, char> literal;
    
    Token(TokenType type, std::string lexeme, SourceLocation location);
    Token(TokenType type, std::string lexeme, SourceLocation location, double value);
    Token(TokenType type, std::string lexeme, SourceLocation location, int64_t value);
    Token(TokenType type, std::string lexeme, SourceLocation location, std::string value);
    Token(TokenType type, std::string lexeme, SourceLocation location, char value);
    
    bool isLiteral() const;
    bool isOperator() const;
    bool isKeyword() const;
    bool isTypeKeyword() const;
    
    std::string toString() const;
    
    template<typename T>
    T getLiteral() const {
        return std::get<T>(literal);
    }
    
    bool hasLiteral() const {
        return !std::holds_alternative<std::monostate>(literal);
    }
};

class TokenStream {
public:
    explicit TokenStream(std::vector<Token> tokens);
    
    const Token& current() const;
    const Token& peek(std::size_t offset = 0) const;
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool isAtEnd() const;
    
    const Token& advance();
    const Token& previous() const;
    
    void consume(TokenType type, std::string_view errorMessage);
    
    std::size_t position() const { return current_; }
    void seek(std::size_t position);
    
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Token;
        using difference_type = std::ptrdiff_t;
        using pointer = const Token*;
        using reference = const Token&;
        
        explicit Iterator(const TokenStream* stream, std::size_t pos);
        
        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int);
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        
    private:
        const TokenStream* stream_;
        std::size_t pos_;
    };
    
    Iterator begin() const;
    Iterator end() const;

private:
    std::vector<Token> tokens_;
    std::size_t current_;
};

const char* tokenTypeToString(TokenType type);
TokenType keywordFromString(std::string_view str);
bool isAssignmentOperator(TokenType type);
int getOperatorPrecedence(TokenType type);
Associativity getOperatorAssociativity(TokenType type);

enum class Associativity {
    LEFT,
    RIGHT,
    NONE
};

struct TokenHash {
    std::size_t operator()(TokenType type) const {
        return static_cast<std::size_t>(type);
    }
};

struct TokenEqual {
    bool operator()(TokenType a, TokenType b) const {
        return a == b;
    }
};

} // namespace internal
} // namespace kiwiLang