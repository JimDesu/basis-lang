#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <list>
#include <map>
#include <stack>

#include "Token.h"

namespace basis {
    class Lexer {
        public:
            explicit Lexer(std::istream& inputStream)
                : input(inputStream), output(), indents(), lineNumber(1), columnNumber(0),readChar(0),
                  checks{ &checkComment, &checkWhitespace, &checkHex, &checkDigit, &checkIdentifier,
                          &checkResWord, &checkString, &checkPunct },
                  reads { &drainLine, &readWhitespace, &readHexNumber, &readNumeric,
                          &readIdentifier, &readResWord, &readString, &readPunct } {}
            ~Lexer();
            std::list<Token> output;
            bool scan();
        private:
            static bool isIdentifierChar(char c);
            static void writeError(const std::string& message, const Token* pToken);
            const static std::map<std::string, TokenType> resWords;
            std::istream& input;
            std::stack<Token*> indents;
            size_t lineNumber;
            size_t columnNumber;
            char readChar;
            bool read();
            Token* nextToken();
            // check functions; if one of these returns true, the concomitant read function must succeed
            // these are listed in correct lexing order; e.g. reserved words start with '.', and have to be
            // lexed before general punctuation
            bool (Lexer::*checks[8])() const;
            bool checkComment() const;
            bool checkWhitespace() const;
            bool checkHex() const;
            bool checkDigit() const;
            bool checkIdentifier() const;
            bool checkResWord() const;
            bool checkString() const;
            bool checkPunct() const;
            // read functions
            bool (Lexer::*reads[8])();
            bool drainLine();
            bool readWhitespace();
            bool readHexNumber();
            bool readNumeric();
            bool readIdentifier();
            bool readResWord();
            bool readString();
            bool readPunct();
    };
}

#endif //LEXER_H
