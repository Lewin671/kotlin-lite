#include "lexer.hpp"
#include <stdexcept>

namespace kotlin_lite {

const std::map<std::string, TokenType> Lexer::keywords_ = {
    {"fun", TokenType::FUN},
    {"val", TokenType::VAL},
    {"var", TokenType::VAR},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"return", TokenType::RETURN},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"null", TokenType::NULL_LITERAL},
    {"package", TokenType::PACKAGE},
    {"import", TokenType::IMPORT},
    {"class", TokenType::CLASS},
    {"interface", TokenType::INTERFACE},
    {"when", TokenType::WHEN},
    {"for", TokenType::FOR},
    {"as", TokenType::AS},
    {"is", TokenType::IS},
    {"this", TokenType::THIS},
    {"super", TokenType::SUPER},
    {"in", TokenType::IN}
};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        char c = advance();
        if (isAlpha(c)) {
            cursor_--;
            column_--;
            tokens.push_back(identifier());
        } else if (isDigit(c)) {
            cursor_--;
            column_--;
            tokens.push_back(number());
        } else {
            switch (c) {
                case '(': tokens.push_back(makeToken(TokenType::LPAREN)); break;
                case ')': tokens.push_back(makeToken(TokenType::RPAREN)); break;
                case '{': tokens.push_back(makeToken(TokenType::LBRACE)); break;
                case '}': tokens.push_back(makeToken(TokenType::RBRACE)); break;
                case ',': tokens.push_back(makeToken(TokenType::COMMA)); break;
                case '.': tokens.push_back(makeToken(TokenType::DOT)); break;
                case ':': tokens.push_back(makeToken(TokenType::COLON)); break;
                case ';': tokens.push_back(makeToken(TokenType::SEMICOLON)); break;
                case '+': tokens.push_back(makeToken(TokenType::PLUS)); break;
                case '-': 
                    if (match('>')) tokens.push_back(makeToken(TokenType::ARROW));
                    else tokens.push_back(makeToken(TokenType::MINUS));
                    break;
                case '*': tokens.push_back(makeToken(TokenType::STAR)); break;
                case '/': tokens.push_back(makeToken(TokenType::SLASH)); break;
                case '%': tokens.push_back(makeToken(TokenType::PERCENT)); break;
                case '!':
                    if (match('=')) tokens.push_back(makeToken(TokenType::NOT_EQUAL));
                    else tokens.push_back(makeToken(TokenType::NOT));
                    break;
                case '=':
                    if (match('=')) tokens.push_back(makeToken(TokenType::EQUAL));
                    else tokens.push_back(makeToken(TokenType::ASSIGN));
                    break;
                case '<':
                    if (match('=')) tokens.push_back(makeToken(TokenType::LESS_EQUAL));
                    else tokens.push_back(makeToken(TokenType::LESS));
                    break;
                case '>':
                    if (match('=')) tokens.push_back(makeToken(TokenType::GREATER_EQUAL));
                    else tokens.push_back(makeToken(TokenType::GREATER));
                    break;
                case '&':
                    if (match('&')) tokens.push_back(makeToken(TokenType::AND));
                    else tokens.push_back(makeToken(TokenType::INVALID, "&"));
                    break;
                case '|':
                    if (match('|')) tokens.push_back(makeToken(TokenType::OR));
                    else tokens.push_back(makeToken(TokenType::INVALID, "|"));
                    break;
                case '"':
                    cursor_--;
                    column_--;
                    tokens.push_back(string());
                    break;
                default:
                    tokens.push_back(makeToken(TokenType::INVALID, std::string(1, c)));
                    break;
            }
        }
    }
    tokens.push_back(makeToken(TokenType::EOF_TOKEN));
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[cursor_];
}

char Lexer::advance() {
    char c = source_[cursor_++];
    column_++;
    if (c == '\n') {
        line_++;
        column_ = 1;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[cursor_] != expected) return false;
    cursor_++;
    column_++;
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance();
                break;
            case '/':
                if (cursor_ + 1 < source_.length()) {
                    if (source_[cursor_ + 1] == '/') {
                        while (!isAtEnd() && peek() != '\n') advance();
                    } else if (source_[cursor_ + 1] == '*') {
                        advance(); // /
                        advance(); // *
                        int depth = 1;
                        while (!isAtEnd() && depth > 0) {
                            if (peek() == '/' && cursor_ + 1 < source_.length() && source_[cursor_ + 1] == '*') {
                                advance(); advance();
                                depth++;
                            } else if (peek() == '*' && cursor_ + 1 < source_.length() && source_[cursor_ + 1] == '/') {
                                advance(); advance();
                                depth--;
                            } else {
                                advance();
                            }
                        }
                    } else {
                        return;
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::makeToken(TokenType type, std::string value) {
    return Token(type, std::move(value), line_, column_);
}

Token Lexer::identifier() {
    size_t start = cursor_;
    while (isAlphaNumeric(peek())) advance();
    std::string value = source_.substr(start, cursor_ - start);
    
    auto it = keywords_.find(value);
    if (it != keywords_.end()) {
        return makeToken(it->second, value);
    }
    return makeToken(TokenType::IDENTIFIER, value);
}

Token Lexer::number() {
    size_t start = cursor_;
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(cursor_ + 1 < source_.length() ? source_[cursor_ + 1] : '\0')) {
        advance(); // .
        while (isDigit(peek())) advance();
        return makeToken(TokenType::FLOAT, source_.substr(start, cursor_ - start));
    }

    return makeToken(TokenType::INTEGER, source_.substr(start, cursor_ - start));
}

Token Lexer::string() {
    advance(); // "
    size_t start = cursor_;
    while (!isAtEnd() && peek() != '"') {
        advance();
    }
    
    if (isAtEnd()) {
        return makeToken(TokenType::INVALID, "Unterminated string");
    }
    
    std::string value = source_.substr(start, cursor_ - start);
    advance(); // "
    return makeToken(TokenType::STRING, value);
}

bool Lexer::isAtEnd() const {
    return cursor_ >= source_.length();
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

std::string_view to_string(TokenType type) {
    switch (type) {
        case TokenType::FUN: return "FUN";
        case TokenType::VAL: return "VAL";
        case TokenType::VAR: return "VAR";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::RETURN: return "RETURN";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CONTINUE: return "CONTINUE";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::NULL_LITERAL: return "NULL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::ARROW: return "ARROW";
        case TokenType::EOF_TOKEN: return "EOF";
        default: return "INVALID";
    }
}

} // namespace kotlin_lite
