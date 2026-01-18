#pragma once
#include "ir.hpp"
#include <map>

namespace kotlin_lite {
namespace ir {

class IRBuilder {
public:
    IRBuilder() : next_id_(0) {}

    void setInsertPoint(BasicBlock* bb) {
        current_bb_ = bb;
    }

    BasicBlock* getInsertPoint() const {
        return current_bb_;
    }

    std::string nextId() {
        return std::to_string(next_id_++);
    }

    Value* createAdd(Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(Instruction::OpKind::Add, Type::I32, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createSub(Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(Instruction::OpKind::Sub, Type::I32, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createMul(Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(Instruction::OpKind::Mul, Type::I32, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createSDiv(Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(Instruction::OpKind::SDiv, Type::I32, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createSRem(Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(Instruction::OpKind::SRem, Type::I32, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createICmp(Instruction::OpKind kind, Value* l, Value* r) {
        auto inst = std::make_unique<BinaryInst>(kind, Type::I1, nextId(), l, r);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createNot(Value* op) {
        auto inst = std::make_unique<UnaryInst>(Instruction::OpKind::Not, Type::I1, nextId(), op);
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    PhiInst* createPhi(Type type) {
        auto inst = std::make_unique<PhiInst>(type, nextId());
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    Value* createCall(Type retType, std::string callee, std::vector<Value*> args) {
        std::string id = (retType == Type::Void) ? "" : nextId();
        auto inst = std::make_unique<CallInst>(retType, id, std::move(callee), std::move(args));
        auto ptr = inst.get();
        current_bb_->addInstruction(std::move(inst));
        return ptr;
    }

    void createBr(BasicBlock* target) {
        current_bb_->addInstruction(std::make_unique<BranchInst>(target));
    }

    void createCondBr(Value* cond, BasicBlock* thenBB, BasicBlock* elseBB) {
        current_bb_->addInstruction(std::make_unique<CondBranchInst>(cond, thenBB, elseBB));
    }

    void createRet(Value* val = nullptr) {
        current_bb_->addInstruction(std::make_unique<ReturnInst>(val));
    }

private:
    BasicBlock* current_bb_ = nullptr;
    int next_id_;
};

} // namespace ir
} // namespace kotlin_lite
