#include "parser.hpp"
#include <stdexcept>

namespace kotlin_lite {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::unique_ptr<KotlinFile> Parser::parse() {
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    while (!isAtEnd()) {
        functions.push_back(functionDecl());
    }
    return std::make_unique<KotlinFile>(std::move(functions));
}

std::unique_ptr<FunctionDecl> Parser::functionDecl() {
    consume(TokenType::FUN, "Expect 'fun' for function declaration.");
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    
    consume(TokenType::LPAREN, "Expect '(' after function name.");
    std::vector<Parameter> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            parameters.push_back(parameter());
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters.");

    std::string returnType = "Unit";
    if (match({TokenType::COLON})) {
        returnType = consume(TokenType::IDENTIFIER, "Expect return type.").value;
    }

    std::unique_ptr<BlockStmt> body = block();
    return std::make_unique<FunctionDecl>(std::move(name), std::move(parameters), returnType, std::move(body));
}

Parameter Parser::parameter() {
    Token name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
    consume(TokenType::COLON, "Expect ':' after parameter name.");
    Token type = consume(TokenType::IDENTIFIER, "Expect parameter type.");
    return {std::move(name), type.value};
}

std::unique_ptr<BlockStmt> Parser::block() {
    consume(TokenType::LBRACE, "Expect '{' before block.");
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(statement());
    }
    consume(TokenType::RBRACE, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match({TokenType::VAL, TokenType::VAR})) return variableDecl();
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::RETURN})) return returnStatement();
    if (match({TokenType::BREAK})) return std::make_unique<BreakStmt>(previous());
    if (match({TokenType::CONTINUE})) return std::make_unique<ContinueStmt>(previous());
    if (check(TokenType::LBRACE)) return block();

    // Assignment or Expression Statement
    if (check(TokenType::IDENTIFIER) && tokens_[current_ + 1].type == TokenType::ASSIGN) {
        return assignment();
    }

    return std::make_unique<ExprStmt>(expression());
}

std::unique_ptr<Stmt> Parser::variableDecl() {
    bool is_val = previous().type == TokenType::VAL;
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    
    std::string type = "";
    if (match({TokenType::COLON})) {
        type = consume(TokenType::IDENTIFIER, "Expect type name.").value;
    }

    consume(TokenType::ASSIGN, "Expect '=' for variable initialization.");
    std::unique_ptr<Expr> initializer = expression();
    
    return std::make_unique<VarDeclStmt>(std::move(name), type, std::move(initializer), is_val);
}

std::unique_ptr<Stmt> Parser::assignment() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    consume(TokenType::ASSIGN, "Expect '=' for assignment.");
    std::unique_ptr<Expr> value = expression();
    return std::make_unique<AssignStmt>(std::move(name), std::move(value));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'if'.");
    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expect ')' after condition.");

    std::unique_ptr<Stmt> thenBranch = statement();
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'while'.");
    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expect ')' after condition.");
    std::unique_ptr<Stmt> body = statement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    Token keyword = previous();
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::RBRACE) && !check(TokenType::SEMICOLON) && !check(TokenType::EOF_TOKEN)) {
        value = expression();
    }
    return std::make_unique<ReturnStmt>(std::move(keyword), std::move(value));
}

std::unique_ptr<Expr> Parser::expression() {
    return logicalOr();
}

std::unique_ptr<Expr> Parser::logicalOr() {
    std::unique_ptr<Expr> expr = logicalAnd();
    while (match({TokenType::OR})) {
        Token op = previous();
        std::unique_ptr<Expr> right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    std::unique_ptr<Expr> expr = equality();
    while (match({TokenType::AND})) {
        Token op = previous();
        std::unique_ptr<Expr> right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    std::unique_ptr<Expr> expr = comparison();
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        Token op = previous();
        std::unique_ptr<Expr> right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    std::unique_ptr<Expr> expr = addition();
    while (match({TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL})) {
        Token op = previous();
        std::unique_ptr<Expr> right = addition();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::addition() {
    std::unique_ptr<Expr> expr = multiplication();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        std::unique_ptr<Expr> right = multiplication();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::multiplication() {
    std::unique_ptr<Expr> expr = unary();
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::NOT, TokenType::MINUS})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }
    return primary();
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::FALSE})) return std::make_unique<LiteralExpr>(previous());
    if (match({TokenType::TRUE})) return std::make_unique<LiteralExpr>(previous());
    if (match({TokenType::NULL_LITERAL})) return std::make_unique<LiteralExpr>(previous());
    if (match({TokenType::INTEGER, TokenType::FLOAT, TokenType::STRING})) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match({TokenType::IDENTIFIER})) {
        Token name = previous();
        if (match({TokenType::LPAREN})) {
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments.");
            return std::make_unique<CallExpr>(std::move(name), std::move(arguments));
        }
        return std::make_unique<VariableExpr>(std::move(name));
    }

    if (match({TokenType::LPAREN})) {
        std::unique_ptr<Expr> expr = expression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw std::runtime_error("Expect expression.");
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error(message);
}

} // namespace kotlin_lite
