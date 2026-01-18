#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <map>

namespace kotlin_lite {

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t cursor_ = 0;
    int line_ = 1;
    int column_ = 1;

    static const std::map<std::string, TokenType> keywords_;

    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespaceAndComments();
    
    Token makeToken(TokenType type, std::string value = "");
    Token identifier();
    Token number();
    Token string();
    
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};

} // namespace kotlin_lite
