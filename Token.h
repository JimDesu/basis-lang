#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>
#include <memory>

namespace basis {
    class Token;
    using spToken = std::shared_ptr<Token>;

    enum class TokenType {
        _NOTHING,
        // literals
        DECIMAL,
        HEXNUMBER,
        NUMBER,
        STRING,
        // identifiers
        IDENTIFIER,
        TYPENAME,
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
        APOSTROPHE,
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
        LARROW,
        LBRACE,
        LBRACKET,
        LPAREN,
        MINUS,
        PERCENT,
        PIPE,
        PIPECOL,
        PLUS,
        POUND,
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
        spToken bound;
        Token() : type(TokenType::_NOTHING), lineNumber(0), columnNumber(0), bound(nullptr) {}
    };

    bool operator==(const Token& lhs, const Token& rhs);
}



#endif //TOKEN_H
