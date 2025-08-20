#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <memory>

#include "ParseTree.h"
#include "Token.h"

namespace basis {
    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens);
        Parser(const Parser& other);
        Parser& operator=(const Parser& rhs);
        ~Parser();

        bool parse();
    private:
        std::list<Token> tokens;
        std::shared_ptr<ParseTree> parseTree;
    };
}

#endif // PARSER_H
