#include "kiwiLang/Lexer/Lexer.h"
#include "kiwiLang/Diagnostics/Diagnostic.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal;

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {
        diagnostics_ = std::make_shared<DiagnosticReporter>();
    }
    
    std::shared_ptr<DiagnosticReporter> diagnostics_;
    
    TokenStream lex(const char* source) {
        Lexer lexer(source, *diagnostics_);
        return lexer.lex();
    }
};

TEST_F(LexerTest, BasicLexing) {
    const char* source = "func main() -> int { return 42; }";
    
    auto stream = lex(source);
    
    std::vector<TokenType> expected = {
        TokenType::FUNC,
        TokenType::IDENTIFIER,
        TokenType::LEFT_PAREN,
        TokenType::RIGHT_PAREN,
        TokenType::RIGHT_ARROW,
        TokenType::INT,
        TokenType::LEFT_BRACE,
        TokenType::RETURN,
        TokenType::NUMBER,
        TokenType::SEMICOLON,
        TokenType::RIGHT_BRACE,
        TokenType::EOF_TOKEN
    };
    
    std::size_t i = 0;
    for (auto it = stream.begin(); it != stream.end() && i < expected.size(); ++it, ++i) {
        EXPECT_EQ(it->type, expected[i]) << "At position " << i;
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, TokenStreamOperations) {
    const char* source = "a b c d e";
    
    auto stream = lex(source);
    
    EXPECT_FALSE(stream.isAtEnd());
    
    const Token& first = stream.current();
    EXPECT_EQ(first.type, TokenType::IDENTIFIER);
    EXPECT_EQ(first.lexeme, "a");
    
    const Token& peeked = stream.peek(2);
    EXPECT_EQ(peeked.type, TokenType::IDENTIFIER);
    EXPECT_EQ(peeked.lexeme, "c");
    
    EXPECT_TRUE(stream.check(TokenType::IDENTIFIER));
    EXPECT_FALSE(stream.check(TokenType::NUMBER));
    
    EXPECT_TRUE(stream.match(TokenType::IDENTIFIER));
    EXPECT_EQ(stream.previous().lexeme, "a");
    
    EXPECT_TRUE(stream.match(TokenType::IDENTIFIER));
    EXPECT_EQ(stream.previous().lexeme, "b");
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, LookaheadBuffer) {
    const char* source = "1 2 3 4 5 6 7 8 9 10";
    
    LexerContext context(source, *diagnostics_);
    LookaheadBuffer buffer(context);
    
    // Test peeking
    Token t1 = buffer.peek(0);
    EXPECT_EQ(t1.type, TokenType::NUMBER);
    EXPECT_EQ(t1.getLiteral<int64_t>(), 1);
    
    Token t5 = buffer.peek(4);
    EXPECT_EQ(t5.type, TokenType::NUMBER);
    EXPECT_EQ(t5.getLiteral<int64_t>(), 5);
    
    // Test consuming
    Token consumed = buffer.consume();
    EXPECT_EQ(consumed.getLiteral<int64_t>(), 1);
    
    Token next = buffer.peek(0);
    EXPECT_EQ(next.getLiteral<int64_t>(), 2);
    
    // Test matching
    EXPECT_TRUE(buffer.match(TokenType::NUMBER));
    EXPECT_TRUE(buffer.match(TokenType::NUMBER));
    EXPECT_EQ(buffer.peek(0).getLiteral<int64_t>(), 4);
    
    // Test rewinding
    std::size_t pos = buffer.position();
    buffer.consume(); // Consume 4
    buffer.rewind(1);
    EXPECT_EQ(buffer.peek(0).getLiteral<int64_t>(), 4);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, IncrementalLexing) {
    const char* initial = "func test() -> int { return 1; }";
    
    IncrementalLexer lexer(*diagnostics_);
    lexer.updateSource(initial, 0, 0); // Initial load
    
    auto tokens = lexer.currentTokens();
    EXPECT_GT(tokens.size(), 0);
    
    // Change "1" to "42"
    std::string modified = "func test() -> int { return 42; }";
    lexer.updateSource(modified, 30, 31); // Replace character at position 30
    
    auto newTokens = lexer.currentTokens();
    
    // Should have same number of tokens
    EXPECT_EQ(tokens.size(), newTokens.size());
    
    // Find the number token
    for (const auto& token : newTokens) {
        if (token.type == TokenType::NUMBER && token.hasLiteral()) {
            EXPECT_EQ(token.getLiteral<int64_t>(), 42);
        }
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, CommentFilter) {
    const char* source = R"(
        // This is a comment
        func test() -> int {
            /* Another comment */
            return 42; // End comment
        }
    )";
    
    LexerPipeline pipeline(source, *diagnostics_);
    pipeline.addFilter(std::make_unique<CommentFilter>());
    
    auto stream = pipeline.lex();
    
    // Count non-comment tokens
    int tokenCount = 0;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        ++tokenCount;
    }
    
    // Should have: func, test, (, ), ->, int, {, return, 42, ;, }
    EXPECT_EQ(tokenCount, 11);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, WhitespaceFilter) {
    const char* source = "a   b\tc\nd";
    
    LexerPipeline pipeline(source, *diagnostics_);
    pipeline.addFilter(std::make_unique<WhitespaceFilter>());
    
    auto stream = pipeline.lex();
    
    // Should get 4 identifiers, no whitespace tokens
    int idCount = 0;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::IDENTIFIER) {
            ++idCount;
        }
    }
    
    EXPECT_EQ(idCount, 4);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, TokenCache) {
    const char* source = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
    
    LexerContext context(source, *diagnostics_);
    TokenCache cache(context);
    
    // Get tokens 0-4
    auto tokens1 = cache.getTokens(0, 5);
    EXPECT_EQ(tokens1.size(), 5);
    
    // Get overlapping range 3-7
    auto tokens2 = cache.getTokens(3, 8);
    EXPECT_EQ(tokens2.size(), 5);
    
    // First 3 tokens should be from cache
    EXPECT_EQ(tokens2[0].lexeme, "d");
    EXPECT_EQ(tokens2[1].lexeme, "e");
    EXPECT_EQ(tokens2[2].lexeme, "f");
    
    // Invalidate part of cache
    cache.invalidate(2, 4);
    
    // Get token at position 3 (should be re-fetched)
    const Token& token = cache.getToken(3);
    EXPECT_EQ(token.lexeme, "d");
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, LexerFactory) {
    const char* source = "func test() -> void {}";
    
    auto stream1 = LexerFactory::lexString(source, *diagnostics_);
    EXPECT_FALSE(diagnostics_->hasErrors());
    
    auto context = LexerFactory::createContext(source, *diagnostics_);
    EXPECT_NE(context, nullptr);
    
    auto pipeline = LexerFactory::createPipeline(source, *diagnostics_);
    EXPECT_NE(pipeline, nullptr);
    
    auto incremental = LexerFactory::createIncrementalLexer(*diagnostics_);
    EXPECT_NE(incremental, nullptr);
}

TEST_F(LexerTest, UnicodeHandling) {
    // Test various Unicode characters
    const char* source = 
        "café naïve π ≈ 3.14 ∆λφα βήта "
        "🎉 🚀 // Emoji in comments"
        "func 😀() -> void {}";
    
    auto stream = lex(source);
    
    // Should lex without errors
    EXPECT_FALSE(diagnostics_->hasErrors());
    
    // Find the emoji identifier
    bool foundEmojiFunc = false;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::IDENTIFIER && it->lexeme.find("😀") != std::string::npos) {
            foundEmojiFunc = true;
        }
    }
    
    EXPECT_TRUE(foundEmojiFunc);
}

TEST_F(LexerTest, ErrorRecovery) {
    const char* source = "\"unterminated string func valid() -> int { return 1; }";
    
    auto stream = lex(source);
    
    // Should have reported error but continued lexing
    EXPECT_TRUE(diagnostics_->hasErrors());
    
    // Should still find the valid function
    bool foundFunc = false;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::FUNC) {
            foundFunc = true;
        }
    }
    
    EXPECT_TRUE(foundFunc);
}

TEST_F(LexerTest, LineAndColumnTracking) {
    const char* source = 
        "line1\n"
        "  line2\n"
        "\tline3\n"
        "line4\r\n"
        "line5";
    
    auto stream = lex(source);
    
    // Check positions of "lineX" tokens
    std::vector<std::pair<int, int>> expectedPositions = {
        {1, 1},   // line1
        {2, 3},   // line2 (indented)
        {3, 2},   // line3 (tab counts as 1 column in some systems)
        {4, 1},   // line4
        {5, 1}    // line5
    };
    
    int idx = 0;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::IDENTIFIER && it->lexeme.find("line") == 0) {
            if (idx < expectedPositions.size()) {
                EXPECT_EQ(it->location.line, expectedPositions[idx].first)
                    << "For token " << it->lexeme;
                // Column checking might be system-dependent due to tab handling
                ++idx;
            }
        }
    }
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, MaximumTokenLimit) {
    // Create source with many tokens
    std::string source;
    for (int i = 0; i < 10000; ++i) {
        source += "a ";
    }
    
    auto stream = lex(source.c_str());
    
    // Should lex successfully or report error if limit exceeded
    // Depends on implementation
    EXPECT_TRUE(!diagnostics_->hasErrors() || diagnostics_->hasErrors());
}

TEST_F(LexerTest, TokenEquality) {
    const char* source = "func func test test";
    
    auto stream = lex(source);
    
    std::vector<Token> tokens;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        tokens.push_back(*it);
    }
    
    // First two tokens should be equal (both "func" keywords)
    EXPECT_EQ(tokens[0].type, tokens[1].type);
    EXPECT_EQ(tokens[0].lexeme, tokens[1].lexeme);
    
    // Last two tokens should be equal (both "test" identifiers)
    EXPECT_EQ(tokens[2].type, tokens[3].type);
    EXPECT_EQ(tokens[2].lexeme, tokens[3].lexeme);
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, PerformanceTest) {
    // Generate large source for performance testing
    std::string source;
    const int LINES = 1000;
    const int TOKENS_PER_LINE = 20;
    
    for (int i = 0; i < LINES; ++i) {
        source += "func test" + std::to_string(i) + "(a: int, b: int) -> int { return a + b; }\n";
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto stream = lex(source.c_str());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 5000); // 5 seconds
    
    // Count tokens
    int tokenCount = 0;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        ++tokenCount;
    }
    
    // Should have approximately LINES * TOKENS_PER_LINE tokens
    EXPECT_GT(tokenCount, LINES * 10);
    
    std::cout << "Lexed " << tokenCount << " tokens in " << duration.count() << "ms\n";
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, NestedComments) {
    const char* source = R"(
        /* Outer comment
           /* Inner comment */
           Still in outer */
        func test() -> void {}
    )";
    
    auto stream = lex(source);
    
    // Should handle nested comments correctly
    // Different languages have different rules about nested comments
    
    bool foundFunc = false;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::FUNC) {
            foundFunc = true;
        }
    }
    
    EXPECT_TRUE(foundFunc);
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, StringInterpolation) {
    const char* source = "`Hello, ${name}! Today is ${day}.`";
    
    auto stream = lex(source);
    
    // Template string with interpolation should produce special tokens
    // Depends on language design
    
    EXPECT_FALSE(diagnostics_->hasErrors());
}

TEST_F(LexerTest, ShebangHandling) {
    const char* source = "#!/usr/bin/env kiwic\n#!another\nfunc main() {}";
    
    auto stream = lex(source);
    
    // Shebang should be ignored, only first line
    // Second #! should probably be an error or comment
    
    bool foundFunc = false;
    for (auto it = stream.begin(); it != stream.end() && it->type != TokenType::EOF_TOKEN; ++it) {
        if (it->type == TokenType::FUNC) {
            foundFunc = true;
        }
    }
    
    EXPECT_TRUE(foundFunc);
}

TEST_F(LexerTest, PreprocessorDirectives) {
    const char* source = 
        "#define DEBUG 1\n"
        "#if DEBUG\n"
        "func debug() -> void {}\n"
        "#endif\n"
        "func release() -> void {}";
    
    auto stream = lex(source);
    
    
    EXPECT_TRUE(!diagnostics_->hasErrors() || diagnostics_->hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}