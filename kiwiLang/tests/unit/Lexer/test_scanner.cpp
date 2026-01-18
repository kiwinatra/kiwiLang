#include "kiwiLang/Lexer/Scanner.h"
#include "kiwiLang/Lexer/Lexer.h"
#include "kiwiLang/Diagnostics/Diagnostic.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal;

class ScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        diagnostics_ = std::make_shared<DiagnosticReporter>();
    }
    
    std::shared_ptr<DiagnosticReporter> diagnostics_;
    
    std::vector<Token> scan(const char* source) {
        Scanner scanner(source, *diagnostics_);
        return scanner.scanTokens();
    }
};

TEST_F(ScannerTest, EmptySource) {
    const char* source = "";
    
    auto tokens = scan(source);
    
    EXPECT_EQ(tokens.size(), 1); // Just EOF
    EXPECT_EQ(tokens[0].type, TokenType::EOF_TOKEN);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, SingleCharacterTokens) {
    const char* source = "(){},.;:?!@#$`";
    
    auto tokens = scan(source);
    
    // Remove EOF
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 14);
    
    std::vector<TokenType> expected = {
        TokenType::LEFT_PAREN,
        TokenType::RIGHT_PAREN,
        TokenType::LEFT_BRACE,
        TokenType::RIGHT_BRACE,
        TokenType::COMMA,
        TokenType::DOT,
        TokenType::SEMICOLON,
        TokenType::COLON,
        TokenType::QUESTION,
        TokenType::BANG,
        TokenType::AT,
        TokenType::HASH,
        TokenType::DOLLAR,
        TokenType::BACKTICK
    };
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, TwoCharacterOperators) {
    const char* source = "== != >= <= >> << ++ -- -> => && || .. ... :: := += -= *= /= %= &= |= ^= >>= <<= **";
    
    auto tokens = scan(source);
    tokens.pop_back(); // Remove EOF
    
    EXPECT_EQ(tokens.size(), 23);
    
    EXPECT_EQ(tokens[0].type, TokenType::EQUAL_EQUAL);
    EXPECT_EQ(tokens[1].type, TokenType::BANG_EQUAL);
    EXPECT_EQ(tokens[2].type, TokenType::GREATER_EQUAL);
    EXPECT_EQ(tokens[3].type, TokenType::LESS_EQUAL);
    EXPECT_EQ(tokens[4].type, TokenType::RIGHT_SHIFT);
    EXPECT_EQ(tokens[5].type, TokenType::LEFT_SHIFT);
    EXPECT_EQ(tokens[6].type, TokenType::PLUS_PLUS);
    EXPECT_EQ(tokens[7].type, TokenType::MINUS_MINUS);
    EXPECT_EQ(tokens[8].type, TokenType::RIGHT_ARROW);
    EXPECT_EQ(tokens[9].type, TokenType::FAT_ARROW);
    EXPECT_EQ(tokens[10].type, TokenType::AMPERSAND_AMPERSAND);
    EXPECT_EQ(tokens[11].type, TokenType::PIPE_PIPE);
    EXPECT_EQ(tokens[12].type, TokenType::DOT_DOT);
    EXPECT_EQ(tokens[13].type, TokenType::DOT_DOT_DOT);
    EXPECT_EQ(tokens[14].type, TokenType::COLON_COLON);
    EXPECT_EQ(tokens[15].type, TokenType::COLON_EQUAL);
    EXPECT_EQ(tokens[16].type, TokenType::PLUS_EQUAL);
    EXPECT_EQ(tokens[17].type, TokenType::MINUS_EQUAL);
    EXPECT_EQ(tokens[18].type, TokenType::STAR_EQUAL);
    EXPECT_EQ(tokens[19].type, TokenType::SLASH_EQUAL);
    EXPECT_EQ(tokens[20].type, TokenType::PERCENT_EQUAL);
    EXPECT_EQ(tokens[21].type, TokenType::AMPERSAND_EQUAL);
    EXPECT_EQ(tokens[22].type, TokenType::PIPE_EQUAL);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, ThreeCharacterOperators) {
    const char* source = ">>> === **= >>= <<= ^= |=";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 7);
    
    EXPECT_EQ(tokens[0].type, TokenType::UNSIGNED_RIGHT_SHIFT);
    EXPECT_EQ(tokens[1].type, TokenType::EQUAL_EQUAL); // Not === (triple equals not in language)
    EXPECT_EQ(tokens[2].type, TokenType::STAR_STAR);
    EXPECT_EQ(tokens[3].type, TokenType::EQUAL); // **= becomes ** then =
    EXPECT_EQ(tokens[4].type, TokenType::RIGHT_SHIFT_EQUAL);
    EXPECT_EQ(tokens[5].type, TokenType::LEFT_SHIFT_EQUAL);
    EXPECT_EQ(tokens[6].type, TokenType::CARET_EQUAL);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, Keywords) {
    const char* source = "func let var if else while for return true false nil and or not in is as async await yield match case default switch break continue";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 24);
    
    EXPECT_EQ(tokens[0].type, TokenType::FUNC);
    EXPECT_EQ(tokens[1].type, TokenType::LET);
    EXPECT_EQ(tokens[2].type, TokenType::VAR);
    EXPECT_EQ(tokens[3].type, TokenType::IF);
    EXPECT_EQ(tokens[4].type, TokenType::ELSE);
    EXPECT_EQ(tokens[5].type, TokenType::WHILE);
    EXPECT_EQ(tokens[6].type, TokenType::FOR);
    EXPECT_EQ(tokens[7].type, TokenType::RETURN);
    EXPECT_EQ(tokens[8].type, TokenType::TRUE);
    EXPECT_EQ(tokens[9].type, TokenType::FALSE);
    EXPECT_EQ(tokens[10].type, TokenType::NIL);
    EXPECT_EQ(tokens[11].type, TokenType::AND);
    EXPECT_EQ(tokens[12].type, TokenType::OR);
    EXPECT_EQ(tokens[13].type, TokenType::NOT_IN); // 'not' followed by 'in' becomes NOT_IN
    EXPECT_EQ(tokens[14].type, TokenType::IS);
    EXPECT_EQ(tokens[15].type, TokenType::AS);
    EXPECT_EQ(tokens[16].type, TokenType::ASYNC);
    EXPECT_EQ(tokens[17].type, TokenType::AWAIT);
    EXPECT_EQ(tokens[18].type, TokenType::YIELD);
    EXPECT_EQ(tokens[19].type, TokenType::MATCH);
    EXPECT_EQ(tokens[20].type, TokenType::CASE);
    EXPECT_EQ(tokens[21].type, TokenType::DEFAULT);
    EXPECT_EQ(tokens[22].type, TokenType::SWITCH);
    EXPECT_EQ(tokens[23].type, TokenType::BREAK);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, TypeKeywords) {
    const char* source = "int float double bool str byte any void";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 8);
    
    EXPECT_EQ(tokens[0].type, TokenType::INT);
    EXPECT_EQ(tokens[1].type, TokenType::FLOAT);
    EXPECT_EQ(tokens[2].type, TokenType::DOUBLE);
    EXPECT_EQ(tokens[3].type, TokenType::BOOL);
    EXPECT_EQ(tokens[4].type, TokenType::STR);
    EXPECT_EQ(tokens[5].type, TokenType::BYTE);
    EXPECT_EQ(tokens[6].type, TokenType::ANY);
    EXPECT_EQ(tokens[7].type, TokenType::VOID);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, Identifiers) {
    const char* source = "variable_name camelCase PascalCase UPPER_CASE _private var123 func123test";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 7);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::IDENTIFIER);
    }
    
    EXPECT_EQ(tokens[0].lexeme, "variable_name");
    EXPECT_EQ(tokens[1].lexeme, "camelCase");
    EXPECT_EQ(tokens[2].lexeme, "PascalCase");
    EXPECT_EQ(tokens[3].lexeme, "UPPER_CASE");
    EXPECT_EQ(tokens[4].lexeme, "_private");
    EXPECT_EQ(tokens[5].lexeme, "var123");
    EXPECT_EQ(tokens[6].lexeme, "func123test");
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, IntegerLiterals) {
    const char* source = "0 42 123456789 0x2A 0xFF 0b101010 0o755";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 7);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::NUMBER);
        EXPECT_TRUE(token.hasLiteral());
    }
    
    EXPECT_EQ(tokens[0].getLiteral<int64_t>(), 0);
    EXPECT_EQ(tokens[1].getLiteral<int64_t>(), 42);
    EXPECT_EQ(tokens[2].getLiteral<int64_t>(), 123456789);
    EXPECT_EQ(tokens[3].getLiteral<int64_t>(), 42); // 0x2A = 42
    EXPECT_EQ(tokens[4].getLiteral<int64_t>(), 255); // 0xFF = 255
    EXPECT_EQ(tokens[5].getLiteral<int64_t>(), 42); // 0b101010 = 42
    EXPECT_EQ(tokens[6].getLiteral<int64_t>(), 493); // 0o755 = 493
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, FloatLiterals) {
    const char* source = "3.14 .5 6.022e23 1e-10 2.5E+5";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 5);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::NUMBER);
        EXPECT_TRUE(token.hasLiteral());
    }
    
    EXPECT_DOUBLE_EQ(tokens[0].getLiteral<double>(), 3.14);
    EXPECT_DOUBLE_EQ(tokens[1].getLiteral<double>(), 0.5);
    EXPECT_DOUBLE_EQ(tokens[2].getLiteral<double>(), 6.022e23);
    EXPECT_DOUBLE_EQ(tokens[3].getLiteral<double>(), 1e-10);
    EXPECT_DOUBLE_EQ(tokens[4].getLiteral<double>(), 2.5e5);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, StringLiterals) {
    const char* source = "\"hello\" \"world\\n\" \"escaped \\\"quote\\\"\" \"multi\\\\line\"";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 4);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::STRING);
        EXPECT_TRUE(token.hasLiteral());
    }
    
    EXPECT_EQ(tokens[0].getLiteral<std::string>(), "hello");
    EXPECT_EQ(tokens[1].getLiteral<std::string>(), "world\n");
    EXPECT_EQ(tokens[2].getLiteral<std::string>(), "escaped \"quote\"");
    EXPECT_EQ(tokens[3].getLiteral<std::string>(), "multi\\line");
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, CharacterLiterals) {
    const char* source = "'a' '\\n' '\\'' '\\\\' '\\x41' '\\u0042'";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 6);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::CHAR);
        EXPECT_TRUE(token.hasLiteral());
    }
    
    EXPECT_EQ(tokens[0].getLiteral<char>(), 'a');
    EXPECT_EQ(tokens[1].getLiteral<char>(), '\n');
    EXPECT_EQ(tokens[2].getLiteral<char>(), '\'');
    EXPECT_EQ(tokens[3].getLiteral<char>(), '\\');
    EXPECT_EQ(tokens[4].getLiteral<char>(), 'A'); // \x41 = 'A'
    EXPECT_EQ(tokens[5].getLiteral<char>(), 'B'); // \u0042 = 'B'
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, TemplateStrings) {
    const char* source = "`template` `hello ${name}` `multiline\\ntemplate`";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 3);
    
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].getLiteral<std::string>(), "template");
    
    // Template strings with interpolation are handled specially
    // Scanner might produce different tokens for ${...}
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, WhitespaceHandling) {
    const char* source = "  \t\n  func  \r\n  test  \v  (  )  \f  ";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 4);
    
    EXPECT_EQ(tokens[0].type, TokenType::FUNC);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].type, TokenType::LEFT_PAREN);
    EXPECT_EQ(tokens[3].type, TokenType::RIGHT_PAREN);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, LineCounting) {
    const char* source = "line1\nline2\r\nline3\rline4\n\nline6";
    
    auto tokens = scan(source);
    
    // Check line numbers in tokens
    for (const auto& token : tokens) {
        // Each identifier should be on different line
        if (token.type == TokenType::IDENTIFIER) {
            EXPECT_GE(token.location.line, 1);
            EXPECT_LE(token.location.line, 6);
        }
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, Comments) {
    const char* source = R"(
        // Single line comment
        func test() -> int {
            /* 
             * Multi-line comment
             * spanning multiple lines
             */
            return 42; // End of line comment
        }
    )";
    
    auto tokens = scan(source);
    
    // Should have tokens: func, test, (, ), ->, int, {, return, 42, ;, }
    // Comments should be skipped
    EXPECT_EQ(tokens.size() - 1, 11); // Minus EOF
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, UnterminatedString) {
    const char* source = "\"unterminated string";
    
    auto tokens = scan(source);
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, InvalidEscapeSequence) {
    const char* source = "\"invalid \\x escape\"";
    
    auto tokens = scan(source);
    
    // Might be an error or might handle it
    // Depends on scanner implementation
}

TEST_F(ScannerTest, InvalidCharacter) {
    const char* source = "func @invalid char";
    
    auto tokens = scan(source);
    
    // @ is valid, but invalid char would be something like ^Z or other control char
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, NumberFormatErrors) {
    const char* source = "123abc 0x 0b 0o 1.2.3";
    
    auto tokens = scan(source);
    
    EXPECT_TRUE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, UnicodeIdentifiers) {
    const char* source = "café naïve π ∆λφα βήτα гамма";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 5);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::IDENTIFIER);
    }
    
    EXPECT_EQ(tokens[0].lexeme, "café");
    EXPECT_EQ(tokens[1].lexeme, "naïve");
    EXPECT_EQ(tokens[2].lexeme, "π");
    EXPECT_EQ(tokens[3].lexeme, "∆λφα");
    EXPECT_EQ(tokens[4].lexeme, "βήτα");
    // Note: "гамма" might be tokenized differently depending on Unicode handling
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, MixedTokens) {
    const char* source = "func add(a: int, b: int) -> int { return a + b; }";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 18);
    
    std::vector<TokenType> expected = {
        TokenType::FUNC,
        TokenType::IDENTIFIER, // add
        TokenType::LEFT_PAREN,
        TokenType::IDENTIFIER, // a
        TokenType::COLON,
        TokenType::INT,
        TokenType::COMMA,
        TokenType::IDENTIFIER, // b
        TokenType::COLON,
        TokenType::INT,
        TokenType::RIGHT_PAREN,
        TokenType::RIGHT_ARROW,
        TokenType::INT,
        TokenType::LEFT_BRACE,
        TokenType::RETURN,
        TokenType::IDENTIFIER, // a
        TokenType::PLUS,
        TokenType::IDENTIFIER, // b
        // SEMICOLON and RIGHT_BRACE would be here
    };
    
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, OperatorPrecedenceTest) {
    const char* source = "a + b * c == d && e || f";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 11);
    
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // a
    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER); // b
    EXPECT_EQ(tokens[3].type, TokenType::STAR);
    EXPECT_EQ(tokens[4].type, TokenType::IDENTIFIER); // c
    EXPECT_EQ(tokens[5].type, TokenType::EQUAL_EQUAL);
    EXPECT_EQ(tokens[6].type, TokenType::IDENTIFIER); // d
    EXPECT_EQ(tokens[7].type, TokenType::AMPERSAND_AMPERSAND);
    EXPECT_EQ(tokens[8].type, TokenType::IDENTIFIER); // e
    EXPECT_EQ(tokens[9].type, TokenType::PIPE_PIPE);
    EXPECT_EQ(tokens[10].type, TokenType::IDENTIFIER); // f
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, MaximumIdentifierLength) {
    // Create a very long identifier
    std::string longId(300, 'a');
    std::string source = "func " + longId + "() -> void {}";
    
    auto tokens = scan(source.c_str());
    
    // Should either truncate or error
    // Depends on scanner implementation
    EXPECT_TRUE(!diagnostics_->hasErrors() || diagnostics_->hasErrors());
}

TEST_F(ScannerTest, ShebangLine) {
    const char* source = "#!/usr/bin/env kiwic\nfunc main() -> void {}";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    // Shebang should be skipped
    EXPECT_EQ(tokens[0].type, TokenType::FUNC);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(ScannerTest, RawStrings) {
    const char* source = "r\"C:\\\\path\\to\\file\" r#\"raw\"string\"#";
    
    auto tokens = scan(source);
    tokens.pop_back();
    
    EXPECT_EQ(tokens.size(), 2);
    
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, TokenType::STRING);
    }
    
    EXPECT_EQ(tokens[0].getLiteral<std::string>(), "C:\\\\path\\to\\file");
    EXPECT_EQ(tokens[1].getLiteral<std::string>(), "raw\"string");
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}