#!/usr/bin/env python3

import os
import re
import sys
import json
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Set, Tuple, Any

@dataclass
class GrammarRule:
    name: str
    production: List[str]
    action: str = ""
    precedence: int = 0
    associativity: str = "left"

@dataclass
class TokenDefinition:
    name: str
    pattern: str
    precedence: int = 0
    associativity: str = ""

class ParserGenerator:
    def __init__(self, grammar_file: Path, output_dir: Path):
        self.grammar_file = grammar_file
        self.output_dir = output_dir
        self.rules: List[GrammarRule] = []
        self.tokens: List[TokenDefinition] = []
        self.start_symbol: str = ""
        
    def load_grammar(self) -> bool:
        if not self.grammar_file.exists():
            print(f"Grammar file not found: {self.grammar_file}")
            return False
            
        content = self.grammar_file.read_text()
        
        sections = re.split(r'^\s*%%\s*$', content, flags=re.MULTILINE)
        
        if len(sections) >= 1:
            self._parse_tokens(sections[0])
        
        if len(sections) >= 2:
            self._parse_rules(sections[1])
        
        return True
    
    def _parse_tokens(self, section: str):
        lines = section.strip().split('\n')
        for line in lines:
            line = line.strip()
            if not line or line.startswith('//'):
                continue
                
            match = re.match(r'(\w+)\s*:\s*(.+?)(?:\s*\[(\d+)\s*(left|right|nonassoc)\])?$', line)
            if match:
                name, pattern, prec, assoc = match.groups()
                token = TokenDefinition(
                    name=name,
                    pattern=pattern.strip('"\''),
                    precedence=int(prec) if prec else 0,
                    associativity=assoc if assoc else ""
                )
                self.tokens.append(token)
    
    def _parse_rules(self, section: str):
        lines = section.strip().split('\n')
        current_rule: Optional[GrammarRule] = None
        
        for line in lines:
            line = line.strip()
            
            if not line or line.startswith('//'):
                continue
            
            if line.startswith('%start'):
                self.start_symbol = line.split()[1]
                continue
            
            if line.endswith(':'):
                if current_rule:
                    self.rules.append(current_rule)
                
                rule_name = line[:-1].strip()
                current_rule = GrammarRule(name=rule_name, production=[])
                
                if not self.start_symbol:
                    self.start_symbol = rule_name
                    
            elif current_rule and '->' in line:
                parts = line.split('->')
                if len(parts) >= 2:
                    production = parts[1].strip()
                    action = parts[2].strip() if len(parts) > 2 else ""
                    
                    symbols = [s.strip() for s in production.split() if s.strip()]
                    current_rule.production = symbols
                    current_rule.action = action
                    self.rules.append(current_rule)
                    current_rule = None
                    
            elif current_rule and line.startswith('|'):
                symbols = [s.strip() for s in line[1:].strip().split() if s.strip()]
                if symbols:
                    rule_copy = GrammarRule(
                        name=current_rule.name,
                        production=symbols,
                        action=current_rule.action
                    )
                    self.rules.append(rule_copy)
    
    def generate_parser(self):
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self._generate_token_header()
        self._generate_parser_header()
        self._generate_parser_source()
        self._generate_grammar_header()
        
    def _generate_token_header(self):
        header = """#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace kiwiLang {

enum class TokenType : uint16_t {
    EOF = 0,
    UNKNOWN = 1,
"""
        
        for i, token in enumerate(self.tokens, 2):
            header += f"    {token.name.upper()} = {i},\n"
        
        header += """};

struct Token {
    TokenType type = TokenType::UNKNOWN;
    std::string_view lexeme;
    size_t line = 1;
    size_t column = 1;
    size_t offset = 0;
    
    bool is(TokenType t) const { return type == t; }
    template<typename... Ts>
    bool isOneOf(TokenType t, Ts... ts) const {
        return is(t) || isOneOf(ts...);
    }
    
    std::string toString() const;
};

const char* tokenTypeToString(TokenType type);

} // namespace kiwiLang
"""
        
        output_file = self.output_dir / "Token.h"
        output_file.write_text(header)
        print(f"Generated: {output_file}")
    
    def _generate_parser_header(self):
        header = """#pragma once

#include "Token.h"
#include "AST.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace kiwiLang {

class Diagnostics;
class Scanner;

class Parser {
public:
    explicit Parser(std::unique_ptr<Scanner> scanner, Diagnostics& diagnostics);
    
    std::unique_ptr<AST::Module> parseModule();
    std::unique_ptr<AST::Statement> parseStatement();
    std::unique_ptr<AST::Expression> parseExpression();
    
private:
    struct ParseRule {
        std::function<std::unique_ptr<AST::Expression>(Parser*)> prefix;
        std::function<std::unique_ptr<AST::Expression>(Parser*, std::unique_ptr<AST::Expression> left)> infix;
        int precedence;
    };
    
    std::unique_ptr<Scanner> scanner_;
    Diagnostics& diagnostics_;
    Token current_;
    Token previous_;
    bool hadError_ = false;
    bool panicMode_ = false;
    
    std::unordered_map<TokenType, ParseRule> rules_;
    
    void advance();
    void consume(TokenType type, const std::string& message);
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool isAtEnd() const;
    
    void error(const Token& token, const std::string& message);
    void synchronize();
    
    void initRules();
    
    std::unique_ptr<AST::Expression> parsePrecedence(int precedence);
    
    std::unique_ptr<AST::Module> parseModuleDeclaration();
    std::unique_ptr<AST::FunctionDecl> parseFunction();
    std::unique_ptr<AST::StructDecl> parseStruct();
    std::unique_ptr<AST::EnumDecl> parseEnum();
    std::unique_ptr<AST::VarDecl> parseVariable();
    std::unique_ptr<AST::Type> parseType();
    
    std::unique_ptr<AST::BlockStmt> parseBlock();
    std::unique_ptr<AST::IfStmt> parseIf();
    std::unique_ptr<AST::WhileStmt> parseWhile();
    std::unique_ptr<AST::ForStmt> parseFor();
    std::unique_ptr<AST::ReturnStmt> parseReturn();
    std::unique_ptr<AST::MatchStmt> parseMatch();
    
    std::unique_ptr<AST::BinaryExpr> parseBinary();
    std::unique_ptr<AST::UnaryExpr> parseUnary();
    std::unique_ptr<AST::CallExpr> parseCall();
    std::unique_ptr<AST::ArrayExpr> parseArray();
    std::unique_ptr<AST::MemberExpr> parseMember();
    std::unique_ptr<AST::IndexExpr> parseIndex();
    std::unique_ptr<AST::LambdaExpr> parseLambda();
    
    std::unique_ptr<AST::LiteralExpr> parseLiteral();
    std::unique_ptr<AST::IdentifierExpr> parseIdentifier();
    std::unique_ptr<AST::GroupExpr> parseGroup();
    
    ParseRule* getRule(TokenType type);
};

} // namespace kiwiLang
"""
        
        output_file = self.output_dir / "Parser.h"
        output_file.write_text(header)
        print(f"Generated: {output_file}")
    
    def _generate_parser_source(self):
        source = """#include "Parser.h"
#include "Scanner.h"
#include "Diagnostics.h"
#include <cassert>

namespace kiwiLang {

Parser::Parser(std::unique_ptr<Scanner> scanner, Diagnostics& diagnostics)
    : scanner_(std::move(scanner))
    , diagnostics_(diagnostics) {
    initRules();
    advance();
}

void Parser::advance() {
    previous_ = current_;
    
    for (;;) {
        current_ = scanner_->nextToken();
        if (current_.type != TokenType::ERROR) break;
        error(current_, "Invalid token");
    }
}

void Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    error(current_, message);
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::isAtEnd() const {
    return current_.type == TokenType::EOF;
}

void Parser::error(const Token& token, const std::string& message) {
    if (panicMode_) return;
    panicMode_ = true;
    
    diagnostics_.error(token.line, token.column, message);
    hadError_ = true;
}

void Parser::synchronize() {
    panicMode_ = false;
    
    while (!isAtEnd()) {
        if (previous_.type == TokenType::SEMICOLON) return;
        
        switch (current_.type) {
            case TokenType::FN:
            case TokenType::STRUCT:
            case TokenType::ENUM:
            case TokenType::LET:
            case TokenType::VAR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::unique_ptr<AST::Module> Parser::parseModule() {
    auto module = std::make_unique<AST::Module>();
    
    while (!isAtEnd()) {
        try {
            if (match(TokenType::FN)) {
                module->functions.push_back(parseFunction());
            } else if (match(TokenType::STRUCT)) {
                module->structs.push_back(parseStruct());
            } else if (match(TokenType::ENUM)) {
                module->enums.push_back(parseEnum());
            } else if (match(TokenType::LET) || match(TokenType::VAR)) {
                module->variables.push_back(parseVariable());
            } else {
                error(current_, "Expected declaration");
                advance();
            }
        } catch (...) {
            synchronize();
        }
    }
    
    return module;
}

std::unique_ptr<AST::FunctionDecl> Parser::parseFunction() {
    auto func = std::make_unique<AST::FunctionDecl>();
    
    consume(TokenType::IDENTIFIER, "Expected function name");
    func->name = previous_.lexeme;
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    if (!check(TokenType::RPAREN)) {
        do {
            auto param = std::make_unique<AST::Parameter>();
            
            if (match(TokenType::MUT)) {
                param->isMutable = true;
            }
            
            consume(TokenType::IDENTIFIER, "Expected parameter name");
            param->name = previous_.lexeme;
            
            consume(TokenType::COLON, "Expected ':' after parameter name");
            param->type = parseType();
            
            func->parameters.push_back(std::move(param));
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    if (match(TokenType::ARROW)) {
        func->returnType = parseType();
    }
    
    func->body = parseBlock();
    
    return func;
}

std::unique_ptr<AST::BlockStmt> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    auto block = std::make_unique<AST::BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

std::unique_ptr<AST::Expression> Parser::parseExpression() {
    return parsePrecedence(0);
}

std::unique_ptr<AST::Expression> Parser::parsePrecedence(int precedence) {
    advance();
    
    auto prefixRule = getRule(previous_.type);
    if (!prefixRule || !prefixRule->prefix) {
        error(previous_, "Expected expression");
        return nullptr;
    }
    
    auto left = prefixRule->prefix(this);
    
    while (precedence <= getRule(current_.type)->precedence) {
        advance();
        auto infixRule = getRule(previous_.type);
        left = infixRule->infix(this, std::move(left));
    }
    
    return left;
}

void Parser::initRules() {
"""
        
        precedence_counter = 1
        precedence_map = {}
        
        for token in self.tokens:
            if token.precedence > 0:
                if token.precedence not in precedence_map:
                    precedence_map[token.precedence] = precedence_counter
                    precedence_counter += 1
        
        source += "    rules_.clear();\n\n"
        
        for token in self.tokens:
            prec = precedence_map.get(token.precedence, 0)
            source += f"    rules_[TokenType::{token.name.upper()}] = {{"
            
            if token.name in ['IDENTIFIER', 'NUMBER', 'STRING']:
                source += f"&Parser::parse{token.name.capitalize()}, "
            else:
                source += "nullptr, "
            
            if token.associativity:
                source += f"&Parser::parse{token.associativity.capitalize()}, "
            else:
                source += "nullptr, "
            
            source += f"{prec}}};\n"
        
        source += """}

Parser::ParseRule* Parser::getRule(TokenType type) {
    auto it = rules_.find(type);
    return it != rules_.end() ? &it->second : nullptr;
}

std::unique_ptr<AST::LiteralExpr> Parser::parseLiteral() {
    auto expr = std::make_unique<AST::LiteralExpr>();
    expr->value = previous_.lexeme;
    expr->token = previous_;
    return expr;
}

std::unique_ptr<AST::IdentifierExpr> Parser::parseIdentifier() {
    auto expr = std::make_unique<AST::IdentifierExpr>();
    expr->name = previous_.lexeme;
    expr->token = previous_;
    return expr;
}

std::unique_ptr<AST::BinaryExpr> Parser::parseBinary() {
    auto expr = std::make_unique<AST::BinaryExpr>();
    expr->op = previous_;
    auto rule = getRule(previous_.type);
    expr->right = parsePrecedence(rule->precedence + 1);
    return expr;
}

std::unique_ptr<AST::UnaryExpr> Parser::parseUnary() {
    auto expr = std::make_unique<AST::UnaryExpr>();
    expr->op = previous_;
    expr->right = parsePrecedence(100);
    return expr;
}

std::unique_ptr<AST::GroupExpr> Parser::parseGroup() {
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after expression");
    return expr;
}

} // namespace kiwiLang
"""
        
        output_file = self.output_dir / "Parser.cpp"
        output_file.write_text(source)
        print(f"Generated: {output_file}")
    
    def _generate_grammar_header(self):
        grammar = f"""%start {self.start_symbol}

%%
"""
        
        for token in self.tokens:
            grammar += f"{token.name}: \"{token.pattern}\""
            if token.precedence:
                grammar += f" [{token.precedence} {token.associativity}]"
            grammar += "\n"
        
        grammar += "\n%%\n\n"
        
        rule_dict: Dict[str, List[GrammarRule]] = {}
        for rule in self.rules:
            if rule.name not in rule_dict:
                rule_dict[rule.name] = []
            rule_dict[rule.name].append(rule)
        
        for rule_name, productions in rule_dict.items():
            grammar += f"{rule_name}:\n"
            for i, prod in enumerate(productions):
                if i > 0:
                    grammar += "    | "
                else:
                    grammar += "    "
                
                grammar += " ".join(prod.production)
                
                if prod.action:
                    grammar += f" -> {prod.action}"
                
                grammar += "\n"
            grammar += "    ;\n\n"
        
        output_file = self.output_dir / "grammar.txt"
        output_file.write_text(grammar)
        print(f"Generated: {output_file}")

def main():
    parser = argparse.ArgumentParser(description="Generate parser from grammar")
    parser.add_argument("grammar", type=Path, help="Grammar definition file")
    parser.add_argument("-o", "--output", type=Path, default=Path("."), help="Output directory")
    parser.add_argument("--force", action="store_true", help="Force overwrite")
    
    args = parser.parse_args()
    
    if not args.grammar.exists():
        print(f"Error: Grammar file '{args.grammar}' not found")
        return 1
    
    generator = ParserGenerator(args.grammar, args.output)
    
    if not generator.load_grammar():
        return 1
    
    generator.generate_parser()
    
    print("Parser generation completed")
    return 0

if __name__ == "__main__":
    sys.exit(main())