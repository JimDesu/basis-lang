#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <list>
#include "Token.h"

namespace basis {
    class Lexer {
        std::istream& input;
        std::list<Token> output;
        std::list<Token*> indents;
        size_t lineNumber;
        size_t columnNumber;
        char readChar;
        public:
            Lexer(std::istream& inputStream) : input(inputStream), output(), indents() {};
            ~Lexer();
            bool scan();
        private:
            bool read();
            Token* nextToken();
            void drainLine();
    };
}

#endif //LEXER_H
