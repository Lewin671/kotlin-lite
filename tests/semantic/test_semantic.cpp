#include <gtest/gtest.h>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"

using namespace kotlin_lite;

TEST(SemanticTest, ValidProgram) {
    std::string source = "fun main() {\n" \
                         "    val x: Int = 42\n" \
                         "    var y = x + 10\n" \
                         "    if (y > 50) {\n" \
                         "        print_i32(y)\n" \
                         "    }\n" \
                         "}";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    EXPECT_TRUE(analyzer.getErrors().empty());
}

TEST(SemanticTest, TypeMismatch) {
    std::string source = "fun main() { val x: Int = true }";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    ASSERT_FALSE(analyzer.getErrors().empty());
    EXPECT_NE(analyzer.getErrors()[0].find("Type mismatch"), std::string::npos);
}

TEST(SemanticTest, UndefinedVariable) {
    std::string source = "fun main() { x = 10 }";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    ASSERT_FALSE(analyzer.getErrors().empty());
    EXPECT_NE(analyzer.getErrors()[0].find("not defined"), std::string::npos);
}

TEST(SemanticTest, ReassignVal) {
    std::string source = "fun main() { val x = 10\n x = 20 }";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    ASSERT_FALSE(analyzer.getErrors().empty());
    EXPECT_NE(analyzer.getErrors()[0].find("Cannot reassign 'val'"), std::string::npos);
}

TEST(SemanticTest, FunctionCallMismatch) {
    std::string source = "fun main() { print_i32(true) }";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    ASSERT_FALSE(analyzer.getErrors().empty());
    EXPECT_NE(analyzer.getErrors()[0].find("expects Int, but got Boolean"), std::string::npos);
}

TEST(SemanticTest, ReturnTypeMismatch) {
    std::string source = "fun foo(): Int { return true }";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto file = parser.parse();
    
    SemanticAnalyzer analyzer;
    analyzer.analyze(*file);
    
    ASSERT_FALSE(analyzer.getErrors().empty());
    EXPECT_NE(analyzer.getErrors()[0].find("Return type mismatch"), std::string::npos);
}
