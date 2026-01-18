#include <gtest/gtest.h>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

using namespace kotlin_lite;

TEST(ParserTest, BasicFunction) {
    std::string source = "fun main() { val x = 42 }";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto file = parser.parse();

    ASSERT_EQ(file->functions.size(), 1);
    EXPECT_EQ(file->functions[0]->name.value, "main");
    ASSERT_EQ(file->functions[0]->body->statements.size(), 1);
}

TEST(ParserTest, IfElse) {
    std::string source = "fun test() { if (true) { return 1 } else { return 0 } }";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto file = parser.parse();

    ASSERT_EQ(file->functions.size(), 1);
    auto& func = file->functions[0];
    ASSERT_EQ(func->body->statements.size(), 1);
    
    auto* ifStmt = dynamic_cast<IfStmt*>(func->body->statements[0].get());
    ASSERT_NE(ifStmt, nullptr);
    ASSERT_NE(ifStmt->else_branch, nullptr);
}

TEST(ParserTest, ExpressionPrecedence) {
    std::string source = "fun test() { val x = 1 + 2 * 3 }";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto file = parser.parse();

    auto& func = file->functions[0];
    auto* varDecl = dynamic_cast<VarDeclStmt*>(func->body->statements[0].get());
    ASSERT_NE(varDecl, nullptr);
    
    auto* binary = dynamic_cast<BinaryExpr*>(varDecl->initializer.get());
    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op.type, TokenType::PLUS);
    
    auto* rightBinary = dynamic_cast<BinaryExpr*>(binary->right.get());
    ASSERT_NE(rightBinary, nullptr);
    EXPECT_EQ(rightBinary->op.type, TokenType::STAR);
}
