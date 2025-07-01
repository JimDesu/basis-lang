#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <list>
#include <map>
#include "Token.h"
#include "CompilerContext.h"

namespace basis {
    class Lexer {
        std::istream& input;
        std::list<Token> output;
        std::list<Token*> indents;
        size_t lineNumber;
        size_t columnNumber;
        char readChar;
        std::map<std::string, TokenType> reswords;
        public:
            Lexer(std::istream& inputStream) : input(inputStream), output(), indents() {
                loadReservedWords();
            };
            ~Lexer();
            bool scan(CompilerContext& compilerContext);
        private:
            bool read();
            Token* nextToken();
            void drainLine();
            bool isIdentifierChar(char c);
            void loadReservedWords();
            void writeError(std::string message, int lineNo, int columnNo);
    };
}

#endif //LEXER_H
