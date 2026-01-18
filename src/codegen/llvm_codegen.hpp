#pragma once
#include "ir/ir.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

namespace kotlin_lite {

class LLVMCodegen {
public:
    LLVMCodegen();
    std::unique_ptr<llvm::Module> generate(const ir::Module& irModule);
    void dump(const llvm::Module& module);

private:
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> llvmModule_;
    llvm::IRBuilder<> builder_;

    std::map<ir::Value*, llvm::Value*> valueMap_;
    std::map<ir::BasicBlock*, llvm::BasicBlock*> bbMap_;

    llvm::Type* getLLVMType(ir::Type type);
    llvm::Value* resolveValue(ir::Value* irVal);
};

} // namespace kotlin_lite
