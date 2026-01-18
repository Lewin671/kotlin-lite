#include "ir.hpp"
#include <sstream>

namespace kotlin_lite {
namespace ir {

std::string BinaryInst::dump() const {
    std::string op;
    switch (kind) {
        case OpKind::Add: op = "add"; break;
        case OpKind::Sub: op = "sub"; break;
        case OpKind::Mul: op = "mul"; break;
        case OpKind::SDiv: op = "sdiv"; break;
        case OpKind::SRem: op = "srem"; break;
        case OpKind::ICmpEq: op = "icmp eq"; break;
        case OpKind::ICmpNe: op = "icmp ne"; break;
        case OpKind::ICmpLt: op = "icmp lt"; break;
        case OpKind::ICmpLe: op = "icmp le"; break;
        case OpKind::ICmpGt: op = "icmp gt"; break;
        case OpKind::ICmpGe: op = "icmp ge"; break;
        default: op = "unknown"; break;
    }
    return getName() + " = " + op + " " + to_string(left->getType()) + " " + left->getName() + ", " + right->getName();
}

std::string UnaryInst::dump() const {
    std::string op = (kind == OpKind::Not) ? "not" : "unknown";
    return getName() + " = " + op + " " + to_string(operand->getType()) + " " + operand->getName();
}

std::string PhiInst::dump() const {
    std::string result = getName() + " = phi " + to_string(type) + " ";
    bool first = true;
    for (auto const& [bb, val] : incomings) {
        if (!first) result += ", ";
        result += "[ " + val->getName() + ", %" + bb->label + " ]";
        first = false;
    }
    return result;
}

std::string CallInst::dump() const {
    std::string result = "";
    if (type != Type::Void) {
        result += getName() + " = ";
    }
    result += "call " + to_string(type) + " @" + callee + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) result += ", ";
        result += to_string(args[i]->getType()) + " " + args[i]->getName();
    }
    result += ")";
    return result;
}

std::string BranchInst::dump() const {
    return "br label %" + target->label;
}

std::string CondBranchInst::dump() const {
    return "condbr i1 " + condition->getName() + ", label %" + thenBB->label + ", label %" + elseBB->label;
}

std::string ReturnInst::dump() const {
    if (value) {
        return "ret " + to_string(value->getType()) + " " + value->getName();
    }
    return "ret void";
}

std::string Module::dump() const {
    std::stringstream ss;
    for (const auto& func : functions) {
        ss << "define " << to_string(func->returnType) << " @" << func->name << "(";
        for (size_t i = 0; i < func->args.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << to_string(func->args[i].type) << " %" << func->args[i].name;
        }
        ss << ") {\n";

        for (const auto& bb : func->blocks) {
            ss << bb->label << ":\n";
            for (const auto& inst : bb->instructions) {
                ss << "  " << inst->dump() << "\n";
            }
        }
        ss << "}\n\n";
    }
    return ss.str();
}

} // namespace ir
} // namespace kotlin_lite

