#ifndef PARSER_H
#define PARSER_H

#include <list>
#include "Token.h"

namespace basis {
    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens);
        Parser(const Parser& other);
        Parser& operator=(const Parser& rhs);
        ~Parser();
        
    private:
        std::list<Token> tokens;
    };
}

#endif // PARSER_H
