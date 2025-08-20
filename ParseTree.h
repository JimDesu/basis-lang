#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <memory>

#include "Token.h"

namespace basis {
    class ParseTree {
    public:
        explicit ParseTree(Token* token);
        ParseTree(const ParseTree& other);
        ParseTree& operator=(const ParseTree& rhs);
        ~ParseTree();
        
        Token* token;
        std::shared_ptr<ParseTree> next;
        std::shared_ptr<ParseTree> down;
    };
}

#endif // PARSEOBJECT_H
