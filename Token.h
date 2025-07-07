#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>

namespace basis {
    enum class TokenType {
        // literals
        DECIMAL,
        HEXNUMBER,
        NUMBER,
        STRING,
        // identifiers
        IDENTIFIER,
        // reserved words
        ALIAS,
        CLASS,
        COMMAND,
        DOMAIN,
        ENUMERATION,
        INTRINSIC,
        OBJECT,
        RECORD,
        // punctuation
        AMPERSAND,
        AMPHORA,
        ASSIGN,
        ASTERISK,
        BANG,
        CARAT,
        COMMA,
        COLON,
        DCOLON,
        EQUALS,
        LANGLE,
        LBRACE,
        LBRACKET,
        LPAREN,
        MINUS,
        PLUS,
        RANGLE,
        RBRACE,
        RBRACKET,
        RPAREN,
        SEMICOLON,
        SLASH,

    };

    struct Token {
        TokenType type;
        std::string text;
        size_t lineNumber;
        size_t columnNumber;
        Token* bound;
    };
}



#endif //TOKEN_H
