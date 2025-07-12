#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <list>
#include <map>
#include <stack>

#include "Token.h"
#include "CompilerContext.h"

namespace basis {
    class Lexer {
        std::istream& input;
        std::stack<Token*> indents;
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
            std::list<Token> output;
            bool scan(const CompilerContext& compilerContext);
        private:
            bool read();
            Token* nextToken();
            void drainLine();
            void loadReservedWords();
            bool readHexNumber();
            bool readNumeric();
            bool readIdentifier();
            bool readResWord();
            bool readString();
            bool readPunct();
            bool checkHex() const;
            bool checkDigit() const;
            bool checkIdentifier() const;
            bool checkResWord() const;
            bool checkString() const;
            bool checkPunct() const;
            static bool isIdentifierChar(char c);
            static void writeError(const std::string& message, const Token* pToken);
    };
}

#endif //LEXER_H
