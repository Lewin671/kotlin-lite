#include <gtest/gtest.h>
#include "lexer/lexer.hpp"

using namespace kotlin_lite;

TEST(LexerTest, BasicTokens) {
    std::string source = "fun main() { val x: Int = 42 }";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 13);
    EXPECT_EQ(tokens[0].type, TokenType::FUN);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "main");
    EXPECT_EQ(tokens[2].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[3].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[4].type, TokenType::LBRACE);
    EXPECT_EQ(tokens[5].type, TokenType::VAL);
    EXPECT_EQ(tokens[6].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[6].value, "x");
    EXPECT_EQ(tokens[7].type, TokenType::COLON);
    EXPECT_EQ(tokens[8].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[8].value, "Int");
    EXPECT_EQ(tokens[9].type, TokenType::ASSIGN);
    EXPECT_EQ(tokens[10].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[10].value, "42");
    EXPECT_EQ(tokens[11].type, TokenType::RBRACE);
    EXPECT_EQ(tokens[12].type, TokenType::EOF_TOKEN);
}

TEST(LexerTest, FullLexing) {
    std::string source = "fun main() {\n"
                         "    val x = 10\n"
                         "    var y = 20.5\n"
                         "    if (x < y) {\n"
                         "        return x + y\n"
                         "    } else {\n"
                         "        return x - y\n"
                         "    }\n"
                         "}";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    // Check some key tokens
    EXPECT_EQ(tokens[0].type, TokenType::FUN);
    EXPECT_EQ(tokens[tokens.size()-1].type, TokenType::EOF_TOKEN);
}

TEST(LexerTest, Comments) {
    std::string source = "// line comment\n"
                         "/* block \n"
                         "   comment */\n"
                         "val x = 1";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 5); // val, x, =, 1, EOF
    EXPECT_EQ(tokens[0].type, TokenType::VAL);
}
