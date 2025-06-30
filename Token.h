#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>

namespace basis {
    enum class TokenType {
        DECIMAL,
        HEXNUMBER,
        IDENTIFIER,
        NUMBER,
        STRING,
        RESWORD,
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
