#pragma once
#include "lexer/token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>

namespace kotlin_lite {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<KotlinFile> parse();

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;

    // --- Grammar Rules ---
    std::unique_ptr<FunctionDecl> functionDecl();
    Parameter parameter();
    std::unique_ptr<BlockStmt> block();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> variableDecl();
    std::unique_ptr<Stmt> assignment();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> returnStatement();
    
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> addition();
    std::unique_ptr<Expr> multiplication();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();

    // --- Helpers ---
    bool match(const std::vector<TokenType>& types);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);
    void error(const Token& token, const std::string& message);
};

} // namespace kotlin_lite
