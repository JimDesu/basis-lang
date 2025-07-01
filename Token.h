#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>

namespace basis {
    enum class TokenType {
        // data types
        DECIMAL,
        HEXNUMBER,
        IDENTIFIER,
        NUMBER,
        STRING,
        // reserved words
        COMMAND,

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
