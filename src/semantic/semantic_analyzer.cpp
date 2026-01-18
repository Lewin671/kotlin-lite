#include "semantic_analyzer.hpp"
#include <iostream>

namespace kotlin_lite {

SemanticAnalyzer::SemanticAnalyzer() {
    // Add built-in functions
    symbol_table_.declareFunction("print_i32", {SymbolType::INT}, SymbolType::UNIT, 0, 0);
    symbol_table_.declareFunction("print_bool", {SymbolType::BOOLEAN}, SymbolType::UNIT, 0, 0);
}

void SemanticAnalyzer::analyze(KotlinFile& file) {
    // Pass 1: Declare all functions
    for (const auto& func : file.functions) {
        std::vector<SymbolType> params;
        for (const auto& p : func->parameters) {
            params.push_back(string_to_type(p.type));
        }
        if (!symbol_table_.declareFunction(func->name.value, params, string_to_type(func->return_type), func->name.line, func->name.column)) {
            error(func->name.line, func->name.column, "Function '" + func->name.value + "' is already defined.");
        }
    }

    // Pass 2: Analyze function bodies
    for (const auto& func : file.functions) {
        analyzeFunction(*func);
    }
}

void SemanticAnalyzer::error(int line, int column, const std::string& message) {
    errors_.push_back("Error at line " + std::to_string(line) + ", col " + std::to_string(column) + ": " + message);
}

void SemanticAnalyzer::analyzeFunction(FunctionDecl& node) {
    symbol_table_.enterScope();
    current_function_return_type_ = string_to_type(node.return_type);

    for (const auto& p : node.parameters) {
        SymbolType type = string_to_type(p.type);
        if (type == SymbolType::UNKNOWN) {
            error(p.name.line, p.name.column, "Unknown type '" + p.type + "' for parameter '" + p.name.value + "'.");
        }
        if (!symbol_table_.declareVariable(p.name.value, type, true, p.name.line, p.name.column)) {
            error(p.name.line, p.name.column, "Parameter '" + p.name.value + "' is already defined.");
        }
    }

    analyzeBlock(*node.body);

    symbol_table_.exitScope();
}

void SemanticAnalyzer::analyzeStmt(Stmt& node) {
    if (auto* block = dynamic_cast<BlockStmt*>(&node)) {
        symbol_table_.enterScope();
        analyzeBlock(*block);
        symbol_table_.exitScope();
    } else if (auto* varDecl = dynamic_cast<VarDeclStmt*>(&node)) {
        SymbolType initType = checkExpr(*varDecl->initializer);
        SymbolType declaredType = varDecl->type.empty() ? initType : string_to_type(varDecl->type);
        
        if (declaredType == SymbolType::UNKNOWN) {
            error(varDecl->name.line, varDecl->name.column, "Unknown type '" + varDecl->type + "'.");
        } else if (initType != declaredType) {
            error(varDecl->name.line, varDecl->name.column, "Type mismatch: declared " + to_string(declaredType) + " but initialized with " + to_string(initType) + ".");
        }

        if (!symbol_table_.declareVariable(varDecl->name.value, declaredType, varDecl->is_val, varDecl->name.line, varDecl->name.column)) {
            error(varDecl->name.line, varDecl->name.column, "Variable '" + varDecl->name.value + "' is already defined in this scope.");
        }
    } else if (auto* assign = dynamic_cast<AssignStmt*>(&node)) {
        auto var = symbol_table_.lookupVariable(assign->name.value);
        if (!var) {
            error(assign->name.line, assign->name.column, "Variable '" + assign->name.value + "' is not defined.");
        } else {
            if (var->is_val) {
                error(assign->name.line, assign->name.column, "Cannot reassign 'val' variable '" + assign->name.value + "'.");
            }
            SymbolType valType = checkExpr(*assign->value);
            if (valType != var->type) {
                error(assign->name.line, assign->name.column, "Type mismatch in assignment to '" + assign->name.value + "'. Expected " + to_string(var->type) + ", got " + to_string(valType) + ".");
            }
        }
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(&node)) {
        if (checkExpr(*ifStmt->condition) != SymbolType::BOOLEAN) {
            error(0, 0, "Condition of 'if' must be Boolean."); // Token info missing in AST for condition?
        }
        analyzeStmt(*ifStmt->then_branch);
        if (ifStmt->else_branch) analyzeStmt(*ifStmt->else_branch);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(&node)) {
        if (checkExpr(*whileStmt->condition) != SymbolType::BOOLEAN) {
            error(0, 0, "Condition of 'while' must be Boolean.");
        }
        analyzeStmt(*whileStmt->body);
    } else if (auto* retStmt = dynamic_cast<ReturnStmt*>(&node)) {
        SymbolType retType = retStmt->value ? checkExpr(*retStmt->value) : SymbolType::UNIT;
        if (retType != current_function_return_type_) {
            error(retStmt->keyword.line, retStmt->keyword.column, "Return type mismatch. Expected " + to_string(current_function_return_type_) + ", got " + to_string(retType) + ".");
        }
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(&node)) {
        checkExpr(*exprStmt->expression);
    }
}

void SemanticAnalyzer::analyzeBlock(BlockStmt& node) {
    for (const auto& stmt : node.statements) {
        analyzeStmt(*stmt);
    }
}

SymbolType SemanticAnalyzer::checkExpr(Expr& node) {
    if (auto* binary = dynamic_cast<BinaryExpr*>(&node)) return checkBinaryExpr(*binary);
    if (auto* unary = dynamic_cast<UnaryExpr*>(&node)) return checkUnaryExpr(*unary);
    if (auto* literal = dynamic_cast<LiteralExpr*>(&node)) return checkLiteralExpr(*literal);
    if (auto* var = dynamic_cast<VariableExpr*>(&node)) return checkVariableExpr(*var);
    if (auto* call = dynamic_cast<CallExpr*>(&node)) return checkCallExpr(*call);
    if (auto* grouping = dynamic_cast<GroupingExpr*>(&node)) return checkGroupingExpr(*grouping);
    return SymbolType::UNKNOWN;
}

SymbolType SemanticAnalyzer::checkBinaryExpr(BinaryExpr& node) {
    SymbolType left = checkExpr(*node.left);
    SymbolType right = checkExpr(*node.right);

    switch (node.op.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            if (left == SymbolType::INT && right == SymbolType::INT) return SymbolType::INT;
            error(node.op.line, node.op.column, "Arithmetic operators require Int operands.");
            return SymbolType::INT;
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
            if (left == right) return SymbolType::BOOLEAN;
            error(node.op.line, node.op.column, "Equality operators require operands of the same type.");
            return SymbolType::BOOLEAN;
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            if (left == SymbolType::INT && right == SymbolType::INT) return SymbolType::BOOLEAN;
            error(node.op.line, node.op.column, "Comparison operators require Int operands.");
            return SymbolType::BOOLEAN;
        case TokenType::AND:
        case TokenType::OR:
            if (left == SymbolType::BOOLEAN && right == SymbolType::BOOLEAN) return SymbolType::BOOLEAN;
            error(node.op.line, node.op.column, "Logical operators require Boolean operands.");
            return SymbolType::BOOLEAN;
        default:
            return SymbolType::UNKNOWN;
    }
}

SymbolType SemanticAnalyzer::checkUnaryExpr(UnaryExpr& node) {
    SymbolType right = checkExpr(*node.right);
    if (node.op.type == TokenType::MINUS) {
        if (right == SymbolType::INT) return SymbolType::INT;
        error(node.op.line, node.op.column, "Unary minus requires Int operand.");
        return SymbolType::INT;
    }
    if (node.op.type == TokenType::NOT) {
        if (right == SymbolType::BOOLEAN) return SymbolType::BOOLEAN;
        error(node.op.line, node.op.column, "Unary NOT requires Boolean operand.");
        return SymbolType::BOOLEAN;
    }
    return SymbolType::UNKNOWN;
}

SymbolType SemanticAnalyzer::checkLiteralExpr(LiteralExpr& node) {
    switch (node.token.type) {
        case TokenType::INTEGER: return SymbolType::INT;
        case TokenType::FLOAT: return SymbolType::FLOAT;
        case TokenType::STRING: return SymbolType::STRING;
        case TokenType::TRUE:
        case TokenType::FALSE: return SymbolType::BOOLEAN;
        case TokenType::NULL_LITERAL: return SymbolType::UNIT; // Simplified
        default: return SymbolType::UNKNOWN;
    }
}

SymbolType SemanticAnalyzer::checkVariableExpr(VariableExpr& node) {
    auto var = symbol_table_.lookupVariable(node.name.value);
    if (!var) {
        error(node.name.line, node.name.column, "Variable '" + node.name.value + "' is not defined.");
        return SymbolType::UNKNOWN;
    }
    return var->type;
}

SymbolType SemanticAnalyzer::checkCallExpr(CallExpr& node) {
    auto func = symbol_table_.lookupFunction(node.callee.value);
    if (!func) {
        error(node.callee.line, node.callee.column, "Function '" + node.callee.value + "' is not defined.");
        return SymbolType::UNKNOWN;
    }

    if (node.arguments.size() != func->parameter_types.size()) {
        error(node.callee.line, node.callee.column, "Function '" + node.callee.value + "' expects " + std::to_string(func->parameter_types.size()) + " arguments, but got " + std::to_string(node.arguments.size()) + ".");
    } else {
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            SymbolType argType = checkExpr(*node.arguments[i]);
            if (argType != func->parameter_types[i]) {
                error(node.callee.line, node.callee.column, "Argument " + std::to_string(i + 1) + " of '" + node.callee.value + "' expects " + to_string(func->parameter_types[i]) + ", but got " + to_string(argType) + ".");
            }
        }
    }
    return func->return_type;
}

SymbolType SemanticAnalyzer::checkGroupingExpr(GroupingExpr& node) {
    return checkExpr(*node.expression);
}

} // namespace kotlin_lite
