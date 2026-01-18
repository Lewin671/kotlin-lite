#pragma once
#include <string>
#include <vector>
#include <memory>
#include "lexer/token.hpp"

namespace kotlin_lite {

class Expr;
class Stmt;

// --- Base AST Node ---
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// --- Expressions ---
class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> l, Token o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
};

class UnaryExpr : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token o, std::unique_ptr<Expr> r)
        : op(std::move(o)), right(std::move(r)) {}
};

class LiteralExpr : public Expr {
public:
    Token token;

    explicit LiteralExpr(Token t) : token(std::move(t)) {}
};

class VariableExpr : public Expr {
public:
    Token name;

    explicit VariableExpr(Token n) : name(std::move(n)) {}
};

class CallExpr : public Expr {
public:
    Token callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(Token c, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(c)), arguments(std::move(args)) {}
};

class GroupingExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;

    explicit GroupingExpr(std::unique_ptr<Expr> e) : expression(std::move(e)) {}
};

// --- Statements ---
class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}
};

class VarDeclStmt : public Stmt {
public:
    Token name;
    std::string type;
    std::unique_ptr<Expr> initializer;
    bool is_val;

    VarDeclStmt(Token n, std::string t, std::unique_ptr<Expr> init, bool val)
        : name(std::move(n)), type(std::move(t)), initializer(std::move(init)), is_val(val) {}
};

class AssignStmt : public Stmt {
public:
    Token name;
    std::unique_ptr<Expr> value;

    AssignStmt(Token n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_b, std::unique_ptr<Stmt> else_b)
        : condition(std::move(cond)), then_branch(std::move(then_b)), else_branch(std::move(else_b)) {}
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;

    ReturnStmt(Token k, std::unique_ptr<Expr> v)
        : keyword(std::move(k)), value(std::move(v)) {}
};

class BreakStmt : public Stmt {
public:
    Token keyword;
    explicit BreakStmt(Token k) : keyword(std::move(k)) {}
};

class ContinueStmt : public Stmt {
public:
    Token keyword;
    explicit ContinueStmt(Token k) : keyword(std::move(k)) {}
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> e) : expression(std::move(e)) {}
};

// --- Top Level ---
struct Parameter {
    Token name;
    std::string type;
};

class FunctionDecl : public ASTNode {
public:
    Token name;
    std::vector<Parameter> parameters;
    std::string return_type;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(Token n, std::vector<Parameter> params, std::string ret_type, std::unique_ptr<BlockStmt> b)
        : name(std::move(n)), parameters(std::move(params)), return_type(std::move(ret_type)), body(std::move(b)) {}
};

class KotlinFile : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    
    explicit KotlinFile(std::vector<std::unique_ptr<FunctionDecl>> funs)
        : functions(std::move(funs)) {}
};

} // namespace kotlin_lite
