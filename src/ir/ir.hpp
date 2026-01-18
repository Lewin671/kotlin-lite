#pragma once
#include <string>
#include <vector>
#include <memory>
#include <list>
#include <map>

namespace kotlin_lite {
namespace ir {

enum class Type {
    I32,
    I1,
    Void
};

inline std::string to_string(Type type) {
    switch (type) {
        case Type::I32: return "i32";
        case Type::I1: return "i1";
        case Type::Void: return "void";
        default: return "unknown";
    }
}

class BasicBlock;
class Function;

// --- Base class for all SSA values ---
class Value {
public:
    virtual ~Value() = default;
    virtual std::string getName() const = 0;
    virtual Type getType() const = 0;
};

// --- Constants ---
class Constant : public Value {
public:
    Type type;
    int32_t value;

    Constant(Type t, int32_t v) : type(t), value(v) {}
    std::string getName() const override { return std::to_string(value); }
    Type getType() const override { return type; }
};

class ArgumentValue : public Value {
public:
    std::string name;
    Type type;

    ArgumentValue(std::string n, Type t) : name(std::move(n)), type(t) {}
    std::string getName() const override { return "%" + name; }
    Type getType() const override { return type; }
};

// --- Instructions ---
class Instruction : public Value {
public:
    enum class OpKind {
        // Value producing
        Add, Sub, Mul, SDiv, SRem,
        ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,
        Not,
        Phi,
        Call,
        // Terminators
        Br,
        CondBr,
        Ret
    };

    OpKind kind;
    Type type;
    std::string id;
    BasicBlock* parent;

    Instruction(OpKind k, Type t, std::string i, BasicBlock* p = nullptr)
        : kind(k), type(t), id(std::move(i)), parent(p) {}

    std::string getName() const override { return "%" + id; }
    Type getType() const override { return type; }
    virtual std::string dump() const = 0;
};

// --- Specific Instructions ---

class BinaryInst : public Instruction {
public:
    Value* left;
    Value* right;

    BinaryInst(OpKind k, Type t, std::string id, Value* l, Value* r)
        : Instruction(k, t, std::move(id)), left(l), right(r) {}

    std::string dump() const override;
};

class UnaryInst : public Instruction {
public:
    Value* operand;

    UnaryInst(OpKind k, Type t, std::string id, Value* op)
        : Instruction(k, t, std::move(id)), operand(op) {}

    std::string dump() const override;
};

class PhiInst : public Instruction {
public:
    // BasicBlock -> Value
    std::map<BasicBlock*, Value*> incomings;

    PhiInst(Type t, std::string id)
        : Instruction(OpKind::Phi, t, std::move(id)) {}

    void addIncoming(BasicBlock* bb, Value* val) { incomings[bb] = val; }
    std::string dump() const override;
};

class CallInst : public Instruction {
public:
    std::string callee;
    std::vector<Value*> args;

    CallInst(Type t, std::string id, std::string name, std::vector<Value*> a)
        : Instruction(OpKind::Call, t, std::move(id)), callee(std::move(name)), args(std::move(a)) {}

    std::string dump() const override;
};

class BranchInst : public Instruction {
public:
    BasicBlock* target;

    BranchInst(BasicBlock* t)
        : Instruction(OpKind::Br, Type::Void, ""), target(t) {}

    std::string dump() const override;
};

class CondBranchInst : public Instruction {
public:
    Value* condition;
    BasicBlock* thenBB;
    BasicBlock* elseBB;

    CondBranchInst(Value* cond, BasicBlock* t, BasicBlock* e)
        : Instruction(OpKind::CondBr, Type::Void, ""), condition(cond), thenBB(t), elseBB(e) {}

    std::string dump() const override;
};

class ReturnInst : public Instruction {
public:
    Value* value; // Can be nullptr for void

    ReturnInst(Value* val)
        : Instruction(OpKind::Ret, Type::Void, ""), value(val) {}

    std::string dump() const override;
};

// --- Containers ---

class BasicBlock {
public:
    std::string label;
    Function* parent;
    std::list<std::unique_ptr<Instruction>> instructions;

    explicit BasicBlock(std::string l, Function* p = nullptr)
        : label(std::move(l)), parent(p) {}

    void addInstruction(std::unique_ptr<Instruction> inst) {
        inst->parent = this;
        instructions.push_back(std::move(inst));
    }

    Instruction* getTerminator() const {
        if (instructions.empty()) return nullptr;
        Instruction* last = instructions.back().get();
        if (last->kind == Instruction::OpKind::Br ||
            last->kind == Instruction::OpKind::CondBr ||
            last->kind == Instruction::OpKind::Ret) {
            return last;
        }
        return nullptr;
    }
};

struct Argument {
    std::string name;
    Type type;
    Value* ssaValue = nullptr;
};

class Function : public Value {
public:
    std::string name;
    Type returnType;
    std::vector<Argument> args;
    std::list<std::unique_ptr<BasicBlock>> blocks;

    Function(std::string n, Type ret, std::vector<Argument> a)
        : name(std::move(n)), returnType(ret), args(std::move(a)) {}

    std::string getName() const override { return "@" + name; }
    Type getType() const override { return returnType; }

    BasicBlock* createBlock(std::string label) {
        blocks.push_back(std::make_unique<BasicBlock>(std::move(label), this));
        return blocks.back().get();
    }
};

class Module {
public:
    std::vector<std::unique_ptr<Function>> functions;

    void addFunction(std::unique_ptr<Function> func) {
        functions.push_back(std::move(func));
    }

    std::string dump() const;
};

} // namespace ir
} // namespace kotlin_lite
