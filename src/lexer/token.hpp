#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace kotlin_lite {

enum class TokenType {
    // Keywords
    FUN, VAL, VAR, IF, ELSE, WHILE, RETURN, BREAK, CONTINUE, TRUE, FALSE, NULL_LITERAL,
    
    // Reserved Keywords
    PACKAGE, IMPORT, CLASS, INTERFACE, WHEN, FOR, AS, IS, THIS, SUPER, IN,

    // Literals
    IDENTIFIER, INTEGER, FLOAT, STRING,

    // Operators and Delimiters
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQUAL, NOT_EQUAL,
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE,
    COMMA, DOT, COLON, SEMICOLON, ARROW,

    // Special
    EOF_TOKEN,
    INVALID
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), column(c) {}
};

std::string_view to_string(TokenType type);

} // namespace kotlin_lite
