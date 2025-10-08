#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>

namespace basis {
    enum class TokenType {
        _NOTHING,
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
        AMBANG,
        AMPERSAND,
        AMPHORA,
        LARROW,
        ASTERISK,
        BANG,
        BANGLANGLE,
        CARAT,
        CARATQ,
        COMMA,
        COLON,
        COLANGLE,
        DCOLON,
        EQUALS,
        LANGLE,
        LBRACE,
        LBRACKET,
        LPAREN,
        MINUS,
        PERCENT,
        PIPE,
        PIPECOL,
        PLUS,
        QCOLON,
        QLANGLE,
        QMARK,
        QMINUS,
        RANGLE,
        RARROW,
        RBRACE,
        RBRACKET,
        RPAREN,
        SLASH,
    };

    class Token {
    public:
        TokenType type;
        std::string text;
        size_t lineNumber;
        size_t columnNumber;
        Token* bound;
        Token() : type(TokenType::_NOTHING), lineNumber(0), columnNumber(0), bound(nullptr) {}
    };

    bool operator==(const Token& lhs, const Token& rhs);
}



#endif //TOKEN_H
