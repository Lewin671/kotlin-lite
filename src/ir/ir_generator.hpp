#pragma once
#include "parser/ast.hpp"
#include "ir.hpp"
#include "ir_builder.hpp"
#include <map>
#include <string>

namespace kotlin_lite {
namespace ir {

class IRGenerator {
public:
    IRGenerator();
    std::unique_ptr<Module> generate(KotlinFile& file);

private:
    IRBuilder builder_;
    std::unique_ptr<Module> module_;
    
    // Environment: tracks the current SSA value for each variable
    using Environment = std::map<std::string, Value*>;
    Environment current_env_;
    
    // Loop targets for break/continue
    struct LoopInfo {
        BasicBlock* header;
        BasicBlock* exit;
        Environment entry_env;
    };
    std::vector<LoopInfo> loop_stack_;

    // --- Generation Methods ---
    void visitFunction(FunctionDecl& node);
    void visitStmt(Stmt& node);
    void visitBlock(BlockStmt& node);
    
    Value* visitExpr(Expr& node);
    Value* visitBinaryExpr(BinaryExpr& node);
    Value* visitUnaryExpr(UnaryExpr& node);
    Value* visitLiteralExpr(LiteralExpr& node);
    Value* visitVariableExpr(VariableExpr& node);
    Value* visitCallExpr(CallExpr& node);
    Value* visitGroupingExpr(GroupingExpr& node);

    // --- SSA Helpers ---
    Type getIRType(const std::string& kotlinType);
    void phiMerge(BasicBlock* mergeBB, const std::vector<std::pair<BasicBlock*, Environment>>& predecessors);
};

} // namespace ir
} // namespace kotlin_lite
