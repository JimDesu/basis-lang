#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include "Token.h"

namespace basis {
    class ParseTree {
    public:
        explicit ParseTree(Token* token, ParseTree* next = nullptr, ParseTree* down = nullptr);
        ParseTree(const ParseTree& other);
        ParseTree& operator=(const ParseTree& rhs);
        ~ParseTree();
        
        Token* token;
        ParseTree* next;
        ParseTree* down;
    };
}

#endif // PARSEOBJECT_H
