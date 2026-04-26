#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <list>
#include <map>
#include <stack>

#include "Diagnostic.h"
#include "Token.h"

namespace basis {
    class Lexer {
        public:
            // Errors are reported into the provided Diagnostics. Pass
            // discardDiagnostics() if you don't care to inspect them.
            explicit Lexer(std::istream& inputStream, Diagnostics& diags = discardDiagnostics()) :
                output(), diagnostics(diags), input(inputStream), indents(), parenStack(), braceStack(), bracketStack(),
                lineNumber(1), columnNumber(0),readChar(0),
                checks{ &Lexer::checkComment, &Lexer::checkWhitespace, &Lexer::checkHex, &Lexer::checkBinary, &Lexer::checkNumeric,
                    &Lexer::checkTypename, &Lexer::checkIdentifier, &Lexer::checkResWord, &Lexer::checkString, &Lexer::checkPunct },
                reads { &Lexer::readComment, &Lexer::readWhitespace, &Lexer::readHex, &Lexer::readBinary, &Lexer::readNumeric,
                    &Lexer::readTypename, &Lexer::readIdentifier, &Lexer::readResWord, &Lexer::readString, &Lexer::readPunct } {}
            ~Lexer();
            std::list<spToken> output;
            bool scan();
        private:
            Diagnostics& diagnostics;
            static bool isIdentifierChar(char c);
            void writeError(const std::string& message, const Token* pToken) const;
            const static std::map<std::string, TokenType> resWords;
            constexpr static int fnCount{ 10 };
            std::istream& input;
            std::list<spToken> indents;
            std::stack<spToken> parenStack;
            std::stack<spToken> braceStack;
            std::stack<spToken> bracketStack;
            size_t lineNumber;
            size_t columnNumber;
            char readChar;
            bool read();
            spToken nextToken();
            // check functions... if one of these returns true, the concomitant read function must succeed.
            // These are listed in correct lexing order; e.g. reserved words start with '.', and have to be
            // lexed before general punctuation
            bool (Lexer::*checks[fnCount])() const;
            bool (Lexer::*reads[fnCount])();
            bool checkComment() const;
            bool readComment();
            bool checkWhitespace() const;
            bool readWhitespace();
            bool checkHex() const;
            bool readHex();
            bool checkBinary() const;
            bool readBinary();
            bool checkNumeric() const;
            bool readNumeric();
            bool checkTypename() const;
            bool readTypename();
            bool checkIdentifier() const;
            bool readIdentifier();
            bool checkResWord() const;
            bool readResWord();
            bool checkString() const;
            bool readString();
            bool checkPunct() const;
            bool readPunct();
            void bindDelimiters(std::stack<spToken>& delimiterStack, spToken closingToken);
    };
}

#endif //LEXER_H
