#include "ir_generator.hpp"
#include <stdexcept>
#include <set>

namespace kotlin_lite {
namespace ir {

IRGenerator::IRGenerator() {}

std::unique_ptr<Module> IRGenerator::generate(KotlinFile& file) {
    module_ = std::make_unique<Module>();
    for (const auto& func : file.functions) {
        visitFunction(*func);
    }
    return std::move(module_);
}

void IRGenerator::visitFunction(FunctionDecl& node) {
    std::vector<Argument> args;
    for (const auto& p : node.parameters) {
        args.push_back({p.name.value, getIRType(p.type)});
    }
    
    auto func = std::make_unique<Function>(node.name.value, getIRType(node.return_type), args);
    auto func_ptr = func.get();
    module_->addFunction(std::move(func));

    BasicBlock* entry = func_ptr->createBlock("entry");
    builder_.setInsertPoint(entry);
    
    current_env_.clear();
    for (auto& arg : func_ptr->args) {
        auto argVal = new ArgumentValue(arg.name, arg.type);
        arg.ssaValue = argVal;
        current_env_[arg.name] = argVal;
    }

    visitBlock(*node.body);
    
    if (!builder_.getInsertPoint()->getTerminator()) {
        if (func_ptr->returnType == Type::Void) {
            builder_.createRet(nullptr);
        } else {
            builder_.createRet(new Constant(func_ptr->returnType, 0));
        }
    }
}

void IRGenerator::visitStmt(Stmt& node) {
    if (auto* block = dynamic_cast<BlockStmt*>(&node)) {
        visitBlock(*block);
    } else if (auto* varDecl = dynamic_cast<VarDeclStmt*>(&node)) {
        Value* init = visitExpr(*varDecl->initializer);
        current_env_[varDecl->name.value] = init;
    } else if (auto* assign = dynamic_cast<AssignStmt*>(&node)) {
        Value* val = visitExpr(*assign->value);
        current_env_[assign->name.value] = val;
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(&node)) {
        Value* cond = visitExpr(*ifStmt->condition);
        Function* func = builder_.getInsertPoint()->parent;
        BasicBlock* thenBB = func->createBlock("if.then");
        BasicBlock* elseBB = func->createBlock("if.else");
        BasicBlock* mergeBB = func->createBlock("if.merge");
        builder_.createCondBr(cond, thenBB, elseBB);
        
        BasicBlock* startBB = builder_.getInsertPoint();
        Environment env_before = current_env_;
        
        builder_.setInsertPoint(thenBB);
        visitStmt(*ifStmt->then_branch);
        BasicBlock* thenOutBB = builder_.getInsertPoint();
        Environment env_then = current_env_;
        if (!thenOutBB->getTerminator()) builder_.createBr(mergeBB);
        
        builder_.setInsertPoint(elseBB);
        current_env_ = env_before;
        if (ifStmt->else_branch) visitStmt(*ifStmt->else_branch);
        BasicBlock* elseOutBB = builder_.getInsertPoint();
        Environment env_else = current_env_;
        if (!elseOutBB->getTerminator()) builder_.createBr(mergeBB);
        
        builder_.setInsertPoint(mergeBB);
        phiMerge(mergeBB, {{thenOutBB, env_then}, {elseOutBB, env_else}});
        
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(&node)) {
        Function* func = builder_.getInsertPoint()->parent;
        BasicBlock* preheaderBB = builder_.getInsertPoint();
        BasicBlock* headerBB = func->createBlock("while.header");
        BasicBlock* bodyBB = func->createBlock("while.body");
        BasicBlock* exitBB = func->createBlock("while.exit");
        
        builder_.createBr(headerBB);
        builder_.setInsertPoint(headerBB);
        
        Environment env_before_loop = current_env_;
        std::map<std::string, PhiInst*> header_phis;
        for (auto const& [name, val] : current_env_) {
            auto phi = builder_.createPhi(val->getType());
            phi->addIncoming(preheaderBB, val);
            header_phis[name] = phi;
            current_env_[name] = phi;
        }
        
        Value* cond = visitExpr(*whileStmt->condition);
        builder_.createCondBr(cond, bodyBB, exitBB);
        
        builder_.setInsertPoint(bodyBB);
        visitStmt(*whileStmt->body);
        BasicBlock* bodyOutBB = builder_.getInsertPoint();
        if (!bodyOutBB->getTerminator()) builder_.createBr(headerBB);
        
        // Backfill header phis
        Environment env_after_body = current_env_;
        for (auto const& [name, phi] : header_phis) {
            phi->addIncoming(bodyOutBB, env_after_body[name]);
        }
        
        builder_.setInsertPoint(exitBB);
        // Variables at exit are those from header (since body might not run)
        current_env_ = env_before_loop;
        for (auto const& [name, phi] : header_phis) {
            current_env_[name] = phi;
        }
        
    } else if (auto* retStmt = dynamic_cast<ReturnStmt*>(&node)) {
        Value* val = retStmt->value ? visitExpr(*retStmt->value) : nullptr;
        builder_.createRet(val);
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(&node)) {
        visitExpr(*exprStmt->expression);
    }
}

void IRGenerator::visitBlock(BlockStmt& node) {
    for (const auto& stmt : node.statements) {
        visitStmt(*stmt);
    }
}

Value* IRGenerator::visitExpr(Expr& node) {
    if (auto* binary = dynamic_cast<BinaryExpr*>(&node)) return visitBinaryExpr(*binary);
    if (auto* unary = dynamic_cast<UnaryExpr*>(&node)) return visitUnaryExpr(*unary);
    if (auto* literal = dynamic_cast<LiteralExpr*>(&node)) return visitLiteralExpr(*literal);
    if (auto* var = dynamic_cast<VariableExpr*>(&node)) return visitVariableExpr(*var);
    if (auto* call = dynamic_cast<CallExpr*>(&node)) return visitCallExpr(*call);
    if (auto* grouping = dynamic_cast<GroupingExpr*>(&node)) return visitGroupingExpr(*grouping);
    return nullptr;
}

Value* IRGenerator::visitBinaryExpr(BinaryExpr& node) {
    if (node.op.type == TokenType::AND) {
        BasicBlock* startBB = builder_.getInsertPoint();
        Value* l = visitExpr(*node.left);
        Function* func = startBB->parent;
        BasicBlock* evalR = func->createBlock("and.rhs");
        BasicBlock* merge = func->createBlock("and.merge");
        builder_.createCondBr(l, evalR, merge);
        
        builder_.setInsertPoint(evalR);
        Value* r = visitExpr(*node.right);
        BasicBlock* rOutBB = builder_.getInsertPoint();
        builder_.createBr(merge);
        
        builder_.setInsertPoint(merge);
        auto phi = builder_.createPhi(Type::I1);
        phi->addIncoming(startBB, new Constant(Type::I1, 0));
        phi->addIncoming(rOutBB, r);
        return phi;
    }
    
    if (node.op.type == TokenType::OR) {
        BasicBlock* startBB = builder_.getInsertPoint();
        Value* l = visitExpr(*node.left);
        Function* func = startBB->parent;
        BasicBlock* evalR = func->createBlock("or.rhs");
        BasicBlock* merge = func->createBlock("or.merge");
        builder_.createCondBr(l, merge, evalR);
        
        builder_.setInsertPoint(evalR);
        Value* r = visitExpr(*node.right);
        BasicBlock* rOutBB = builder_.getInsertPoint();
        builder_.createBr(merge);
        
        builder_.setInsertPoint(merge);
        auto phi = builder_.createPhi(Type::I1);
        phi->addIncoming(startBB, new Constant(Type::I1, 1));
        phi->addIncoming(rOutBB, r);
        return phi;
    }

    Value* l = visitExpr(*node.left);
    Value* r = visitExpr(*node.right);
    
    switch (node.op.type) {
        case TokenType::PLUS: return builder_.createAdd(l, r);
        case TokenType::MINUS: return builder_.createSub(l, r);
        case TokenType::STAR: return builder_.createMul(l, r);
        case TokenType::SLASH: return builder_.createSDiv(l, r);
        case TokenType::PERCENT: return builder_.createSRem(l, r);
        case TokenType::EQUAL: return builder_.createICmp(Instruction::OpKind::ICmpEq, l, r);
        case TokenType::NOT_EQUAL: return builder_.createICmp(Instruction::OpKind::ICmpNe, l, r);
        case TokenType::LESS: return builder_.createICmp(Instruction::OpKind::ICmpLt, l, r);
        case TokenType::LESS_EQUAL: return builder_.createICmp(Instruction::OpKind::ICmpLe, l, r);
        case TokenType::GREATER: return builder_.createICmp(Instruction::OpKind::ICmpGt, l, r);
        case TokenType::GREATER_EQUAL: return builder_.createICmp(Instruction::OpKind::ICmpGe, l, r);
        default: return nullptr;
    }
}

Value* IRGenerator::visitUnaryExpr(UnaryExpr& node) {
    Value* op = visitExpr(*node.right);
    if (node.op.type == TokenType::NOT) return builder_.createNot(op);
    if (node.op.type == TokenType::MINUS) return builder_.createSub(new Constant(Type::I32, 0), op);
    return nullptr;
}

Value* IRGenerator::visitLiteralExpr(LiteralExpr& node) {
    if (node.token.type == TokenType::INTEGER) return new Constant(Type::I32, std::stoi(node.token.value));
    if (node.token.type == TokenType::TRUE) return new Constant(Type::I1, 1);
    if (node.token.type == TokenType::FALSE) return new Constant(Type::I1, 0);
    return nullptr;
}

Value* IRGenerator::visitVariableExpr(VariableExpr& node) {
    auto it = current_env_.find(node.name.value);
    if (it != current_env_.end()) return it->second;
    throw std::runtime_error("Undefined variable in IR generation: " + node.name.value);
}

Value* IRGenerator::visitCallExpr(CallExpr& node) {
    std::vector<Value*> args;
    for (auto const& argExpr : node.arguments) args.push_back(visitExpr(*argExpr));
    Type retType = (node.callee.value == "print_i32" || node.callee.value == "print_bool") ? Type::Void : Type::I32;
    return builder_.createCall(retType, node.callee.value, args);
}

Value* IRGenerator::visitGroupingExpr(GroupingExpr& node) {
    return visitExpr(*node.expression);
}

Type IRGenerator::getIRType(const std::string& kotlinType) {
    if (kotlinType == "Int") return Type::I32;
    if (kotlinType == "Boolean") return Type::I1;
    return Type::Void;
}

void IRGenerator::phiMerge(BasicBlock* mergeBB, const std::vector<std::pair<BasicBlock*, Environment>>& predecessors) {
    std::set<std::string> all_vars;
    for (auto const& [bb, env] : predecessors) {
        if (bb->getTerminator() && bb->getTerminator()->kind == Instruction::OpKind::Ret) continue;
        for (auto const& [name, val] : env) all_vars.insert(name);
    }
    
    for (const auto& var : all_vars) {
        std::map<BasicBlock*, Value*> incomings;
        Value* first_val = nullptr;
        bool all_same = true;
        for (auto const& [bb, env] : predecessors) {
            if (bb->getTerminator() && bb->getTerminator()->kind == Instruction::OpKind::Ret) continue;
            auto it = env.find(var);
            Value* val = (it != env.end()) ? it->second : nullptr;
            if (val) {
                incomings[bb] = val;
                if (!first_val) first_val = val;
                else if (val != first_val) all_same = false;
            }
        }
        if (all_same && first_val && incomings.size() >= 1) current_env_[var] = first_val;
        else if (!incomings.empty()) {
            auto phi = builder_.createPhi(first_val->getType());
            for (auto const& [bb, val] : incomings) phi->addIncoming(bb, val);
            current_env_[var] = phi;
        }
    }
}

} // namespace ir
} // namespace kotlin_lite