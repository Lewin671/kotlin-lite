#pragma once
#include "parser/ast.hpp"
#include "symbol_table.hpp"
#include <vector>
#include <string>

namespace kotlin_lite {

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(KotlinFile& file);
    const std::vector<std::string>& getErrors() const { return errors_; }

private:
    SymbolTable symbol_table_;
    std::vector<std::string> errors_;
    SymbolType current_function_return_type_ = SymbolType::UNKNOWN;

    void error(int line, int column, const std::string& message);
    
    void analyzeFunction(FunctionDecl& node);
    void analyzeStmt(Stmt& node);
    void analyzeBlock(BlockStmt& node);
    
    SymbolType checkExpr(Expr& node);
    SymbolType checkBinaryExpr(BinaryExpr& node);
    SymbolType checkUnaryExpr(UnaryExpr& node);
    SymbolType checkLiteralExpr(LiteralExpr& node);
    SymbolType checkVariableExpr(VariableExpr& node);
    SymbolType checkCallExpr(CallExpr& node);
    SymbolType checkGroupingExpr(GroupingExpr& node);
};

} // namespace kotlin_lite
