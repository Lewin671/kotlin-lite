#include <gtest/gtest.h>
#include "ir/ir.hpp"
#include "ir/ir_builder.hpp"

using namespace kotlin_lite::ir;

TEST(IRTest, BasicStructure) {
    Module mod;
    std::vector<Argument> args = {{"a", Type::I32}, {"b", Type::I32}};
    auto func = std::make_unique<Function>("add", Type::I32, args);
    
    BasicBlock* entry = func->createBlock("entry");
    IRBuilder builder;
    builder.setInsertPoint(entry);
    
    // We don't have SSA values for arguments yet in this manual test,
    // so let's just use constants for demonstration.
    auto v1 = new Constant(Type::I32, 10);
    auto v2 = new Constant(Type::I32, 20);
    auto sum = builder.createAdd(v1, v2);
    builder.createRet(sum);
    
    mod.addFunction(std::move(func));
    
    std::string output = mod.dump();
    EXPECT_NE(output.find("define i32 @add(i32 %a, i32 %b) {"), std::string::npos);
    EXPECT_NE(output.find("entry:"), std::string::npos);
    EXPECT_NE(output.find("%0 = add i32 10, 20"), std::string::npos);
    EXPECT_NE(output.find("ret i32 %0"), std::string::npos);
    
    delete v1;
    delete v2;
}
