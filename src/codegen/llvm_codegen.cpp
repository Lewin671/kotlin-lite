#include "llvm_codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

namespace kotlin_lite {

LLVMCodegen::LLVMCodegen() : builder_(context_) {}

std::unique_ptr<llvm::Module> LLVMCodegen::generate(const ir::Module& irModule) {
    llvmModule_ = std::make_unique<llvm::Module>("kotlin_lite", context_);
    valueMap_.clear();
    bbMap_.clear();

    // 1. Declare all functions first
    for (const auto& irFunc : irModule.functions) {
        std::vector<llvm::Type*> paramTypes;
        for (const auto& arg : irFunc->args) {
            paramTypes.push_back(getLLVMType(arg.type));
        }
        llvm::FunctionType* funcType = llvm::FunctionType::get(getLLVMType(irFunc->returnType), paramTypes, false);
        llvm::Function* llvmFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, irFunc->name, llvmModule_.get());
        
        // Map IR function to LLVM function
        valueMap_[irFunc.get()] = llvmFunc;

        // Name the arguments and map them
        unsigned i = 0;
        for (auto& llvmArg : llvmFunc->args()) {
            llvmArg.setName(irFunc->args[i].name);
            if (irFunc->args[i].ssaValue) {
                valueMap_[irFunc->args[i].ssaValue] = &llvmArg;
            }
            i++;
        }
    }

    // 2. Generate bodies
    for (const auto& irFunc : irModule.functions) {
        llvm::Function* llvmFunc = llvmModule_->getFunction(irFunc->name);
        
        // Create all basic blocks first to handle forward references
        for (const auto& irBB : irFunc->blocks) {
            llvm::BasicBlock* llvmBB = llvm::BasicBlock::Create(context_, irBB->label, llvmFunc);
            bbMap_[irBB.get()] = llvmBB;
        }

        // Fill in instructions
        for (const auto& irBB : irFunc->blocks) {
            llvm::BasicBlock* llvmBB = bbMap_[irBB.get()];
            builder_.SetInsertPoint(llvmBB);

            for (const auto& irInst : irBB->instructions) {
                llvm::Value* val = nullptr;
                switch (irInst->kind) {
                    case ir::Instruction::OpKind::Add: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        val = builder_.CreateAdd(resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::Sub: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        val = builder_.CreateSub(resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::Mul: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        val = builder_.CreateMul(resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::SDiv: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        val = builder_.CreateSDiv(resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::SRem: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        val = builder_.CreateSRem(resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::ICmpEq:
                    case ir::Instruction::OpKind::ICmpNe:
                    case ir::Instruction::OpKind::ICmpLt:
                    case ir::Instruction::OpKind::ICmpLe:
                    case ir::Instruction::OpKind::ICmpGt:
                    case ir::Instruction::OpKind::ICmpGe: {
                        auto bin = static_cast<ir::BinaryInst*>(irInst.get());
                        llvm::CmpInst::Predicate pred;
                        if (irInst->kind == ir::Instruction::OpKind::ICmpEq) pred = llvm::CmpInst::ICMP_EQ;
                        else if (irInst->kind == ir::Instruction::OpKind::ICmpNe) pred = llvm::CmpInst::ICMP_NE;
                        else if (irInst->kind == ir::Instruction::OpKind::ICmpLt) pred = llvm::CmpInst::ICMP_SLT;
                        else if (irInst->kind == ir::Instruction::OpKind::ICmpLe) pred = llvm::CmpInst::ICMP_SLE;
                        else if (irInst->kind == ir::Instruction::OpKind::ICmpGt) pred = llvm::CmpInst::ICMP_SGT;
                        else pred = llvm::CmpInst::ICMP_SGE;
                        val = builder_.CreateICmp(pred, resolveValue(bin->left), resolveValue(bin->right));
                        break;
                    }
                    case ir::Instruction::OpKind::Not: {
                        auto un = static_cast<ir::UnaryInst*>(irInst.get());
                        val = builder_.CreateNot(resolveValue(un->operand));
                        break;
                    }
                    case ir::Instruction::OpKind::Phi: {
                        auto phi = static_cast<ir::PhiInst*>(irInst.get());
                        llvm::PHINode* llvmPhi = builder_.CreatePHI(getLLVMType(phi->type), phi->incomings.size());
                        val = llvmPhi;
                        break;
                    }
                    case ir::Instruction::OpKind::Call: {
                        auto call = static_cast<ir::CallInst*>(irInst.get());
                        std::vector<llvm::Value*> args;
                        for (auto irArg : call->args) args.push_back(resolveValue(irArg));
                        
                        llvm::Function* callee = llvmModule_->getFunction(call->callee);
                        if (!callee) {
                            std::vector<llvm::Type*> argTypes;
                            for (auto a : args) argTypes.push_back(a->getType());
                            llvm::FunctionType* ft = llvm::FunctionType::get(getLLVMType(call->type), argTypes, false);
                            callee = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, call->callee, llvmModule_.get());
                        }
                        val = builder_.CreateCall(callee, args);
                        break;
                    }
                    case ir::Instruction::OpKind::Br: {
                        auto br = static_cast<ir::BranchInst*>(irInst.get());
                        builder_.CreateBr(bbMap_[br->target]);
                        break;
                    }
                    case ir::Instruction::OpKind::CondBr: {
                        auto cbr = static_cast<ir::CondBranchInst*>(irInst.get());
                        builder_.CreateCondBr(resolveValue(cbr->condition), bbMap_[cbr->thenBB], bbMap_[cbr->elseBB]);
                        break;
                    }
                    case ir::Instruction::OpKind::Ret: {
                        auto ret = static_cast<ir::ReturnInst*>(irInst.get());
                        if (ret->value) builder_.CreateRet(resolveValue(ret->value));
                        else builder_.CreateRetVoid();
                        break;
                    }
                }
                if (val) valueMap_[irInst.get()] = val;
            }
        }
    }

    // 3. Populate Phi nodes
    for (const auto& irFunc : irModule.functions) {
        for (const auto& irBB : irFunc->blocks) {
            for (const auto& irInst : irBB->instructions) {
                if (irInst->kind == ir::Instruction::OpKind::Phi) {
                    auto irPhi = static_cast<ir::PhiInst*>(irInst.get());
                    auto llvmPhi = llvm::cast<llvm::PHINode>(valueMap_[irInst.get()]);
                    for (auto const& [bb, val] : irPhi->incomings) {
                        llvmPhi->addIncoming(resolveValue(val), bbMap_[bb]);
                    }
                }
            }
        }
    }

    return std::move(llvmModule_);
}

void LLVMCodegen::dump(const llvm::Module& module) {
    module.print(llvm::errs(), nullptr);
}

llvm::Type* LLVMCodegen::getLLVMType(ir::Type type) {
    switch (type) {
        case ir::Type::I32: return llvm::Type::getInt32Ty(context_);
        case ir::Type::I1: return llvm::Type::getInt1Ty(context_);
        case ir::Type::Void: return llvm::Type::getVoidTy(context_);
        default: return nullptr;
    }
}

llvm::Value* LLVMCodegen::resolveValue(ir::Value* irVal) {
    if (auto constant = dynamic_cast<ir::Constant*>(irVal)) {
        if (constant->type == ir::Type::I32) {
            return llvm::ConstantInt::get(context_, llvm::APInt(32, constant->value, true));
        } else if (constant->type == ir::Type::I1) {
            return llvm::ConstantInt::get(context_, llvm::APInt(1, constant->value));
        }
    }
    auto it = valueMap_.find(irVal);
    if (it != valueMap_.end()) return it->second;
    throw std::runtime_error("LLVM Codegen: Unresolved IR value: " + irVal->getName());
}

} // namespace kotlin_lite
