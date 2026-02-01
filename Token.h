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
        BINARY,
        NUMBER,
        STRING,
        // identifiers
        IDENTIFIER,
        TYPENAME,
        // reserved words
        ALIAS,
        CLASS,
        COMMAND,
        DECLARE,
        DOMAIN,
        ENUMERATION,
        IMPORT,
        INSTANCE,
        INTRINSIC,
        MODULE,
        OBJECT,
        PROGRAM,
        RECORD,
        TEST,
        // punctuation
        AMBANG,
        AMPERSAND,
        AMPHORA,
        APOSTROPHE,
        ASTERISK,
        BANG,
        BANGBRACE,
        BANGLANGLE,
        CARAT,
        COMMA,
        COLON,
        COLANGLE,
        COLBRACE,
        DCOLON,
        DOLLAR,
        EQUALS,
        LANGLE,
        LARROW,
        LBRACE,
        LBRACKET,
        LPAREN,
        MINUS,
        PERCENT,
        PIPE,
        PLUS,
        POUND,
        QBRACE,
        QCOLON,
        QLANGLE,
        QMARK,
        QMINUS,
        DQMARK,
        RANGLE,
        RARROW,
        RBRACE,
        RBRACKET,
        RPAREN,
        SLASH,
        UNDERSCORE,
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
