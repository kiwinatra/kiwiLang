#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

struct FormatOptions {
    size_t indentWidth = 4;
    bool useSpaces = true;
    size_t maxLineLength = 100;
    bool braceNextLine = false;
    bool compactBraces = false;
    size_t tabWidth = 4;
    bool alignChainedCalls = true;
    bool alignAssignments = false;
    bool trailingCommas = true;
    bool singleQuoteStrings = false;
    bool spacesInParens = false;
    bool spacesInBrackets = false;
    bool spacesInBraces = true;
    bool spacesAroundOperators = true;
    bool spacesAfterComma = true;
    bool spacesAfterSemicolon = true;
    bool spacesBeforeColon = false;
    bool spacesAfterColon = true;
    bool spacesBeforeLambdaArrow = true;
    bool spacesAfterLambdaArrow = true;
    bool wrapAfterParens = true;
    bool keepBlankLines = true;
    size_t maxBlankLines = 2;
    bool sortImports = true;
    bool groupImports = true;
    std::string newlineStyle = "auto";
};

class Token {
public:
    enum class Type {
        Unknown,
        Whitespace,
        Newline,
        Comment,
        Identifier,
        Keyword,
        Number,
        String,
        Char,
        Operator,
        Punctuator,
        Preprocessor,
        EOF
    };

    Type type;
    std::string_view text;
    size_t line;
    size_t column;
    size_t offset;
    size_t length;
    
    bool isKeyword() const { return type == Type::Keyword; }
    bool isIdentifier() const { return type == Type::Identifier; }
    bool isOperator() const { return type == Type::Operator; }
    bool isPunctuator() const { return type == Type::Punctuator; }
    bool isWhitespace() const { return type == Type::Whitespace; }
    bool isNewline() const { return type == Type::Newline; }
    bool isComment() const { return type == Type::Comment; }
    bool isString() const { return type == Type::String; }
    bool isNumber() const { return type == Type::Number; }
    
    char firstChar() const { return !text.empty() ? text[0] : '\0'; }
    char lastChar() const { return !text.empty() ? text[text.size() - 1] : '\0'; }
};

class Tokenizer {
public:
    explicit Tokenizer(std::string_view source) : source_(source), pos_(0), line_(1), column_(1) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (pos_ < source_.size()) {
            size_t start = pos_;
            size_t startLine = line_;
            size_t startColumn = column_;
            
            char c = source_[pos_];
            
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (c == '\n') {
                    advanceNewline();
                    tokens.push_back(createToken(Token::Type::Newline, start));
                } else {
                    advanceWhitespace();
                    tokens.push_back(createToken(Token::Type::Whitespace, start));
                }
            } else if (c == '/' && pos_ + 1 < source_.size()) {
                if (source_[pos_ + 1] == '/') {
                    advanceLineComment();
                    tokens.push_back(createToken(Token::Type::Comment, start));
                } else if (source_[pos_ + 1] == '*') {
                    advanceBlockComment();
                    tokens.push_back(createToken(Token::Type::Comment, start));
                } else {
                    advanceOperator();
                    tokens.push_back(createToken(Token::Type::Operator, start));
                }
            } else if (c == '#') {
                advancePreprocessor();
                tokens.push_back(createToken(Token::Type::Preprocessor, start));
            } else if (c == '"' || c == '\'') {
                advanceString(c);
                tokens.push_back(createToken(c == '"' ? Token::Type::String : Token::Type::Char, start));
            } else if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && pos_ + 1 < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_ + 1])))) {
                advanceNumber();
                tokens.push_back(createToken(Token::Type::Number, start));
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                advanceIdentifier();
                auto token = createToken(Token::Type::Identifier, start);
                if (isKeyword(token.text)) {
                    token.type = Token::Type::Keyword;
                }
                tokens.push_back(token);
            } else if (isOperatorChar(c)) {
                advanceOperator();
                tokens.push_back(createToken(Token::Type::Operator, start));
            } else if (isPunctuatorChar(c)) {
                advancePunctuator();
                tokens.push_back(createToken(Token::Type::Punctuator, start));
            } else {
                advanceChar();
                tokens.push_back(createToken(Token::Type::Unknown, start));
            }
        }
        
        tokens.push_back({Token::Type::EOF, "", line_, column_, pos_, 0});
        return tokens;
    }

private:
    Token createToken(Token::Type type, size_t start) const {
        return {
            type,
            source_.substr(start, pos_ - start),
            line_,
            column_ - (pos_ - start),
            start,
            pos_ - start
        };
    }
    
    void advanceChar() {
        if (pos_ < source_.size()) {
            if (source_[pos_] == '\n') {
                ++line_;
                column_ = 1;
            } else {
                ++column_;
            }
            ++pos_;
        }
    }
    
    void advanceNewline() {
        while (pos_ < source_.size() && source_[pos_] == '\n') {
            ++line_;
            column_ = 1;
            ++pos_;
        }
    }
    
    void advanceWhitespace() {
        while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[pos_])) && source_[pos_] != '\n') {
            ++column_;
            ++pos_;
        }
    }
    
    void advanceLineComment() {
        while (pos_ < source_.size() && source_[pos_] != '\n') {
            ++column_;
            ++pos_;
        }
    }
    
    void advanceBlockComment() {
        pos_ += 2;
        column_ += 2;
        
        while (pos_ < source_.size()) {
            if (source_[pos_] == '\n') {
                ++line_;
                column_ = 1;
            } else {
                ++column_;
            }
            
            if (source_[pos_] == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
                pos_ += 2;
                column_ += 2;
                break;
            }
            ++pos_;
        }
    }
    
    void advancePreprocessor() {
        while (pos_ < source_.size() && source_[pos_] != '\n') {
            ++column_;
            ++pos_;
        }
    }
    
    void advanceString(char quote) {
        ++pos_;
        ++column_;
        bool escaped = false;
        
        while (pos_ < source_.size()) {
            if (source_[pos_] == '\n') {
                ++line_;
                column_ = 1;
            } else {
                ++column_;
            }
            
            if (!escaped && source_[pos_] == quote) {
                ++pos_;
                ++column_;
                break;
            }
            
            escaped = (source_[pos_] == '\\' && !escaped);
            ++pos_;
        }
    }
    
    void advanceNumber() {
        bool hasDot = false;
        bool hasExp = false;
        
        while (pos_ < source_.size()) {
            char c = source_[pos_];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                ++column_;
                ++pos_;
            } else if (c == '.') {
                if (hasDot || hasExp) break;
                hasDot = true;
                ++column_;
                ++pos_;
            } else if (c == 'e' || c == 'E') {
                if (hasExp) break;
                hasExp = true;
                ++column_;
                ++pos_;
                if (pos_ < source_.size() && (source_[pos_] == '+' || source_[pos_] == '-')) {
                    ++column_;
                    ++pos_;
                }
            } else if (c == 'f' || c == 'F' || c == 'l' || c == 'L') {
                ++column_;
                ++pos_;
                break;
            } else {
                break;
            }
        }
    }
    
    void advanceIdentifier() {
        while (pos_ < source_.size() && (std::isalnum(static_cast<unsigned char>(source_[pos_])) || source_[pos_] == '_')) {
            ++column_;
            ++pos_;
        }
    }
    
    void advanceOperator() {
        static const std::unordered_set<std::string_view> multiCharOps = {
            "==", "!=", "<=", ">=", "&&", "||", "++", "--",
            "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "<<", ">>", "->", "::", "...", "..", "=>"
        };
        
        size_t start = pos_;
        while (pos_ < source_.size() && isOperatorChar(source_[pos_])) {
            ++column_;
            ++pos_;
            
            if (pos_ - start > 1) {
                std::string_view op = source_.substr(start, pos_ - start);
                if (multiCharOps.find(op) != multiCharOps.end()) {
                    continue;
                }
                --pos_;
                --column_;
                break;
            }
        }
    }
    
    void advancePunctuator() {
        ++column_;
        ++pos_;
    }
    
    static bool isKeyword(std::string_view str) {
        static const std::unordered_set<std::string_view> keywords = {
            "fn", "let", "var", "const", "if", "else", "while", "for",
            "in", "match", "return", "break", "continue", "struct",
            "enum", "trait", "impl", "use", "mod", "pub", "priv",
            "static", "mut", "ref", "self", "Self", "true", "false",
            "null", "type", "where", "async", "await", "try", "catch",
            "throw", "interface", "abstract", "final", "override"
        };
        return keywords.find(str) != keywords.end();
    }
    
    static bool isOperatorChar(char c) {
        static const std::string operators = "+-*/%=!<>&|^~?.:";
        return operators.find(c) != std::string::npos;
    }
    
    static bool isPunctuatorChar(char c) {
        static const std::string punctuators = "()[]{},;";
        return punctuators.find(c) != std::string::npos;
    }
    
    std::string_view source_;
    size_t pos_;
    size_t line_;
    size_t column_;
};

class Formatter {
public:
    Formatter(const FormatOptions& opts) : opts_(opts), indentLevel_(0), lineLength_(0), inTemplate_(false) {}
    
    std::string format(std::string_view source) {
        tokens_ = Tokenizer(source).tokenize();
        current_ = 0;
        output_.clear();
        indentLevel_ = 0;
        lineLength_ = 0;
        inTemplate_ = false;
        
        while (!isAtEnd()) {
            formatToken();
            advance();
        }
        
        return output_;
    }

private:
    void formatToken() {
        const Token& token = peek();
        
        switch (token.type) {
            case Token::Type::Newline:
                handleNewline();
                break;
            case Token::Type::Whitespace:
                handleWhitespace();
                break;
            case Token::Type::Comment:
                handleComment();
                break;
            case Token::Type::Keyword:
                handleKeyword();
                break;
            case Token::Type::Identifier:
                handleIdentifier();
                break;
            case Token::Type::Punctuator:
                handlePunctuator();
                break;
            case Token::Type::Operator:
                handleOperator();
                break;
            case Token::Type::String:
            case Token::Type::Char:
                handleString(token);
                break;
            case Token::Type::Number:
                handleNumber(token);
                break;
            case Token::Type::Preprocessor:
                handlePreprocessor();
                break;
            default:
                writeToken(token.text);
                break;
        }
    }
    
    void handleNewline() {
        if (!opts_.keepBlankLines) {
            size_t newlineCount = 0;
            size_t i = current_;
            while (i < tokens_.size() && tokens_[i].type == Token::Type::Newline) {
                ++newlineCount;
                ++i;
            }
            
            if (newlineCount > opts_.maxBlankLines) {
                newlineCount = opts_.maxBlankLines;
            }
            
            for (size_t j = 0; j < newlineCount; ++j) {
                writeNewline();
            }
            
            current_ += newlineCount - 1;
        } else {
            writeNewline();
        }
        
        lineLength_ = 0;
        if (shouldIndentAfterNewline()) {
            writeIndent();
        }
    }
    
    void handleWhitespace() {
    }
    
    void handleComment() {
        if (peek().text.substr(0, 2) == "//") {
            writeToken(peek().text);
            writeNewline();
            writeIndent();
        } else {
            writeToken(peek().text);
            if (peekNext().type != Token::Type::Newline) {
                writeSpace();
            }
        }
    }
    
    void handleKeyword() {
        std::string_view keyword = peek().text;
        writeToken(keyword);
        
        if (keyword == "fn" || keyword == "struct" || keyword == "enum" || keyword == "trait" || keyword == "impl") {
            writeSpace();
        } else if (keyword == "if" || keyword == "while" || keyword == "for" || keyword == "match") {
            writeSpace();
        } else if (keyword == "let" || keyword == "var" || keyword == "const") {
            writeSpace();
        } else if (keyword == "return" || keyword == "break" || keyword == "continue") {
            if (peekNext().type != Token::Type::Newline && peekNext().type != Token::Type::Semicolon) {
                writeSpace();
            }
        }
    }
    
    void handleIdentifier() {
        writeToken(peek().text);
    }
    
    void handlePunctuator() {
        char punct = peek().firstChar();
        
        switch (punct) {
            case '(':
            case '[':
                writeToken(peek().text);
                if (opts_.spacesInParens && punct == '(') {
                    writeSpace();
                } else if (opts_.spacesInBrackets && punct == '[') {
                    writeSpace();
                }
                break;
                
            case ')':
            case ']':
                if (opts_.spacesInParens && punct == ')') {
                    writeSpace();
                } else if (opts_.spacesInBrackets && punct == ']') {
                    writeSpace();
                }
                writeToken(peek().text);
                break;
                
            case '{':
                if (opts_.braceNextLine) {
                    writeNewline();
                    writeIndent();
                } else if (!opts_.compactBraces && peek(-1).type != Token::Type::Whitespace) {
                    writeSpace();
                }
                writeToken(peek().text);
                ++indentLevel_;
                writeNewline();
                writeIndent();
                break;
                
            case '}':
                --indentLevel_;
                writeNewline();
                writeIndent();
                writeToken(peek().text);
                break;
                
            case ',':
                writeToken(peek().text);
                if (opts_.spacesAfterComma && peekNext().type != Token::Type::Newline) {
                    writeSpace();
                }
                break;
                
            case ';':
                writeToken(peek().text);
                if (opts_.spacesAfterSemicolon && peekNext().type != Token::Type::Newline) {
                    writeSpace();
                }
                break;
                
            case ':':
                if (opts_.spacesBeforeColon) {
                    writeSpace();
                }
                writeToken(peek().text);
                if (opts_.spacesAfterColon) {
                    writeSpace();
                }
                break;
                
            default:
                writeToken(peek().text);
                break;
        }
    }
    
    void handleOperator() {
        std::string_view op = peek().text;
        
        if (op == "=>") {
            if (opts_.spacesBeforeLambdaArrow) {
                writeSpace();
            }
            writeToken(op);
            if (opts_.spacesAfterLambdaArrow) {
                writeSpace();
            }
        } else if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
            writeToken(op);
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
        } else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
            writeToken(op);
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
        } else if (op == "&&" || op == "||") {
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
            writeToken(op);
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
        } else if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (lineLength_ > 0 && opts_.spacesAroundOperators) {
                writeSpace();
            }
            writeToken(op);
            if (opts_.spacesAroundOperators) {
                writeSpace();
            }
        } else {
            writeToken(op);
        }
    }
    
    void handleString(const Token& token) {
        if (opts_.singleQuoteStrings && token.type == Token::Type::String) {
            writeChar('\'');
            writeToken(token.text.substr(1, token.text.size() - 2));
            writeChar('\'');
        } else {
            writeToken(token.text);
        }
    }
    
    void handleNumber(const Token& token) {
        writeToken(token.text);
    }
    
    void handlePreprocessor() {
        writeToken(peek().text);
        writeNewline();
        writeIndent();
    }
    
    void writeToken(std::string_view text) {
        output_.append(text);
        lineLength_ += text.size();
    }
    
    void writeChar(char c) {
        output_.push_back(c);
        ++lineLength_;
    }
    
    void writeSpace() {
        if (lineLength_ > 0 && output_.back() != ' ') {
            writeChar(' ');
        }
    }
    
    void writeNewline() {
        if (!output_.empty() && output_.back() != '\n') {
            writeChar('\n');
        }
    }
    
    void writeIndent() {
        if (opts_.useSpaces) {
            for (size_t i = 0; i < indentLevel_ * opts_.indentWidth; ++i) {
                writeChar(' ');
            }
        } else {
            for (size_t i = 0; i < indentLevel_; ++i) {
                writeChar('\t');
            }
        }
    }
    
    bool shouldIndentAfterNewline() const {
        if (isAtEnd()) return false;
        
        const Token& next = peek();
        if (next.type == Token::Type::Newline || next.type == Token::Type::EOF) {
            return false;
        }
        
        if (next.type == Token::Type::Punctuator && next.firstChar() == '}') {
            return false;
        }
        
        return true;
    }
    
    const Token& peek() const {
        return tokens_[current_];
    }
    
    const Token& peek(int offset) const {
        size_t index = current_ + offset;
        if (index >= tokens_.size()) index = tokens_.size() - 1;
        return tokens_[index];
    }
    
    const Token& peekNext() const {
        return peek(1);
    }
    
    void advance() {
        if (!isAtEnd()) ++current_;
    }
    
    bool isAtEnd() const {
        return current_ >= tokens_.size() || tokens_[current_].type == Token::Type::EOF;
    }
    
    FormatOptions opts_;
    std::vector<Token> tokens_;
    size_t current_;
    std::string output_;
    size_t indentLevel_;
    size_t lineLength_;
    bool inTemplate_;
};

bool readFile(const fs::path& path, std::string& content) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    content.resize(size);
    file.read(content.data(), size);
    return true;
}

bool writeFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.write(content.data(), content.size());
    return true;
}

void printHelp() {
    std::cout << "Usage: kiwiLang-format [options] <files>\n"
              << "Options:\n"
              << "  -i, --in-place          Edit files in-place\n"
              << "  --check                 Check if files need formatting\n"
              << "  --indent-width N        Set indent width (default: 4)\n"
              << "  --use-tabs              Use tabs instead of spaces\n"
              << "  --max-line-length N     Set maximum line length (default: 100)\n"
              << "  --brace-next-line       Place braces on next line\n"
              << "  --compact-braces        Use compact brace style\n"
              << "  --no-trailing-commas    Don't add trailing commas\n"
              << "  --single-quote-strings  Use single quotes for strings\n"
              << "  --no-space-operators    Don't add spaces around operators\n"
              << "  -h, --help              Show this help message\n";
}

int main(int argc, char* argv[]) {
    FormatOptions options;
    bool inPlace = false;
    bool checkOnly = false;
    std::vector<fs::path> files;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-i" || arg == "--in-place") {
            inPlace = true;
        } else if (arg == "--check") {
            checkOnly = true;
        } else if (arg == "--indent-width" && i + 1 < argc) {
            options.indentWidth = std::stoul(argv[++i]);
        } else if (arg == "--use-tabs") {
            options.useSpaces = false;
        } else if (arg == "--max-line-length" && i + 1 < argc) {
            options.maxLineLength = std::stoul(argv[++i]);
        } else if (arg == "--brace-next-line") {
            options.braceNextLine = true;
        } else if (arg == "--compact-braces") {
            options.compactBraces = true;
        } else if (arg == "--no-trailing-commas") {
            options.trailingCommas = false;
        } else if (arg == "--single-quote-strings") {
            options.singleQuoteStrings = true;
        } else if (arg == "--no-space-operators") {
            options.spacesAroundOperators = false;
        } else if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg[0] != '-') {
            files.emplace_back(arg);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }
    
    if (files.empty()) {
        std::cerr << "Error: No files specified" << std::endl;
        return 1;
    }
    
    int exitCode = 0;
    Formatter formatter(options);
    
    for (const auto& file : files) {
        if (!fs::exists(file)) {
            std::cerr << "File not found: " << file << std::endl;
            exitCode = 1;
            continue;
        }
        
        std::string source;
        if (!readFile(file, source)) {
            std::cerr << "Failed to read file: " << file << std::endl;
            exitCode = 1;
            continue;
        }
        
        std::string formatted = formatter.format(source);
        
        if (checkOnly) {
            if (source != formatted) {
                std::cout << file << " needs formatting\n";
                exitCode = 1;
            }
        } else if (inPlace) {
            if (!writeFile(file, formatted)) {
                std::cerr << "Failed to write file: " << file << std::endl;
                exitCode = 1;
            } else {
                std::cout << "Formatted: " << file << std::endl;
            }
        } else {
            std::cout << formatted;
            if (files.size() > 1) {
                std::cout << "\n--- " << file << " ---\n";
            }
        }
    }
    
    return exitCode;
}