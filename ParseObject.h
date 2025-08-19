#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include "Token.h"

namespace basis {
    class ParseObject {
    public:
        explicit ParseObject(Token* token, ParseObject* next = nullptr, ParseObject* down = nullptr);
        ParseObject(const ParseObject& other);
        ParseObject& operator=(const ParseObject& rhs);
        ~ParseObject();
        
        Token* token;
        ParseObject* next;
        ParseObject* down;
    };
}

#endif // PARSEOBJECT_H
