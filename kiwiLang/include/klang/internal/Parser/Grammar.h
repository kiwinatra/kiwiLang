include/kiwiLang/internal/Parser/Grammar.h
#pragma once
#include "kiwiLang/internal/Lexer/Token.h"
#include "kiwiLang/internal/AST/Expr.h"
#include "kiwiLang/internal/AST/Stmt.h"
#include "kiwiLang/internal/Diagnostics/Diagnostic.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace kiwiLang {
namespace internal {

class GrammarProduction {
public:
    virtual ~GrammarProduction() = default;
    
    virtual bool matches(std::vector<Token>::const_iterator& it,
                        std::vector<Token>::const_iterator end) const = 0;
    virtual std::unique_ptr<ast::Node> parse(Parser& parser) const = 0;
    
    virtual std::string toString() const = 0;
};

class TerminalProduction : public GrammarProduction {
public:
    explicit TerminalProduction(TokenType type);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;

private:
    TokenType type_;
};

class NonTerminalProduction : public GrammarProduction {
public:
    using ProductionPtr = std::unique_ptr<GrammarProduction>;
    
    NonTerminalProduction(std::string name, std::vector<ProductionPtr> alternatives);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;
    
    void addAlternative(ProductionPtr alternative);

private:
    std::string name_;
    std::vector<ProductionPtr> alternatives_;
};

class SequenceProduction : public GrammarProduction {
public:
    using ProductionPtr = std::unique_ptr<GrammarProduction>;
    
    SequenceProduction(std::vector<ProductionPtr> sequence);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;

private:
    std::vector<ProductionPtr> sequence_;
};

class OptionalProduction : public GrammarProduction {
public:
    explicit OptionalProduction(std::unique_ptr<GrammarProduction> production);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;

private:
    std::unique_ptr<GrammarProduction> production_;
};

class RepeatProduction : public GrammarProduction {
public:
    explicit RepeatProduction(std::unique_ptr<GrammarProduction> production);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;

private:
    std::unique_ptr<GrammarProduction> production_;
};

class LookaheadProduction : public GrammarProduction {
public:
    LookaheadProduction(std::unique_ptr<GrammarProduction> production, bool positive);
    
    bool matches(std::vector<Token>::const_iterator& it,
                std::vector<Token>::const_iterator end) const override;
    std::unique_ptr<ast::Node> parse(Parser& parser) const override;
    
    std::string toString() const override;

private:
    std::unique_ptr<GrammarProduction> production_;
    bool positive_;
};

class GrammarBuilder {
public:
    GrammarBuilder();
    
    NonTerminalProduction* addProduction(std::string name);
    void addAlternative(std::string name, std::unique_ptr<GrammarProduction> alternative);
    
    GrammarProduction* getProduction(std::string_view name) const;
    
    std::unordered_map<std::string, std::unique_ptr<GrammarProduction>> build();

private:
    std::unordered_map<std::string, std::unique_ptr<GrammarProduction>> productions_;
};

class LL1Grammar {
public:
    struct FirstSet {
        std::unordered_set<TokenType> terminals;
        bool containsEpsilon;
    };
    
    struct FollowSet {
        std::unordered_set<TokenType> terminals;
    };
    
    struct ParseTableEntry {
        GrammarProduction* production;
        bool valid;
    };
    
    explicit LL1Grammar(GrammarBuilder& builder);
    
    bool isValid() const;
    const ParseTableEntry& getEntry(std::string_view nonTerminal, TokenType lookahead) const;
    
    void computeFirstSets();
    void computeFollowSets();
    
    const FirstSet& first(std::string_view symbol) const;
    const FollowSet& follow(std::string_view symbol) const;
    
    void generateParseTable();
    
    std::vector<std::string> getDiagnostics() const;

private:
    std::unordered_map<std::string, std::unique_ptr<GrammarProduction>> productions_;
    std::unordered_map<std::string, FirstSet> firstSets_;
    std::unordered_map<std::string, FollowSet> followSets_;
    std::unordered_map<std::string, std::unordered_map<TokenType, ParseTableEntry>> parseTable_;
    std::vector<std::string> diagnostics_;
    
    bool isTerminal(std::string_view symbol) const;
    bool isNonTerminal(std::string_view symbol) const;
    
    FirstSet computeFirst(const GrammarProduction* production);
    void addToFirst(FirstSet& set, const FirstSet& other);
    
    void computeFirstForSymbol(std::string_view symbol);
    void computeFollowForSymbol(std::string_view symbol);
    
    bool checkLL1Condition() const;
};

class ParserGenerator {
public:
    explicit ParserGenerator(LL1Grammar& grammar);
    
    std::string generateParserCode() const;
    std::string generateASTClasses() const;
    std::string generateVisitorClasses() const;
    
private:
    LL1Grammar& grammar_;
    
    std::string generateProductionMethod(const GrammarProduction* production, 
                                        std::string_view methodName) const;
    std::string generateNonTerminalMethod(const NonTerminalProduction* production) const;
    std::string generateSequenceMethod(const SequenceProduction* production) const;
    std::string generateTerminalMethod(const TerminalProduction* production) const;
    
    std::string indent(std::size_t level) const;
    
    static constexpr std::size_t MAX_INDENT = 8;
};

class PredictiveParser {
public:
    PredictiveParser(std::vector<Token> tokens, LL1Grammar& grammar, 
                     DiagnosticReporter& diagnostics);
    
    std::unique_ptr<ast::Program> parse();
    
    const DiagnosticReporter& diagnostics() const { return diagnostics_; }

private:
    std::vector<Token> tokens_;
    std::size_t current_;
    LL1Grammar& grammar_;
    DiagnosticReporter& diagnostics_;
    std::stack<std::string> parseStack_;
    
    const Token& advance();
    const Token& peek() const;
    TokenType lookahead() const;
    
    void push(std::string_view symbol);
    std::string pop();
    
    bool matchTerminal(TokenType expected);
    void parseError(std::string_view expected, const Token& found);
    
    std::unique_ptr<ast::Node> parseNonTerminal(std::string_view nonTerminal);
    std::unique_ptr<ast::Expr> parseExpression();
    std::unique_ptr<ast::Stmt> parseStatement();
    
    void synchronize();
};

class GrammarValidator {
public:
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    static ValidationResult validate(const LL1Grammar& grammar);
    
private:
    static void checkLeftRecursion(const LL1Grammar& grammar, ValidationResult& result);
    static void checkAmbiguity(const LL1Grammar& grammar, ValidationResult& result);
    static void checkCompleteness(const LL1Grammar& grammar, ValidationResult& result);
    static void checkProductivity(const LL1Grammar& grammar, ValidationResult& result);
    static void checkReachability(const LL1Grammar& grammar, ValidationResult& result);
};

} // namespace internal
} // namespace kiwiLang