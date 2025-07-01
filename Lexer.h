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
        std::map<std::string, TokenType> resWords;
        public:
            explicit Lexer(std::istream& inputStream)
                : input(inputStream), output(), indents(), lineNumber(1), columnNumber(0),readChar(0) {
                loadReservedWords();
            }
            ~Lexer();
            bool scan(CompilerContext& compilerContext);
        private:
            bool read();
            Token* nextToken();
            void drainLine();
            void loadReservedWords();
            static bool isIdentifierChar(char c);
            static void writeError(const std::string& message, const Token* pToken);
    };
}

#endif //LEXER_H
