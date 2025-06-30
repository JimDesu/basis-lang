#include "Lexer.h"

#include <cctype>
#include <iostream>

using namespace basis;

basis::Lexer::~Lexer() {
    output.clear();
    indents.clear();
}

bool basis::Lexer::read() {
    if(input.good()) {
        input >> readChar;
        if( readChar == '\n') {
          lineNumber++;
          columnNumber = 0;
          return read();
        }
        columnNumber++;
        return true;
    }
    return false;
}

Token* basis::Lexer::nextToken() {
    output.emplace_back();
    Token* pToken = &output.back();
    pToken->lineNumber = lineNumber;
    pToken->columnNumber = columnNumber;
    // handle bounding here
    return pToken;
}

void basis::Lexer::drainLine() {
    // get the current line number
    size_t line = lineNumber;
    // blindly read until we're on the next line
    while( read() && line == lineNumber);
}

bool basis::Lexer::scan() {
    output.clear();
    indents.clear();
    if(!input.good()) return false;
    while( read() ) {
        // skip over whitespace chars
        if( isspace(readChar) ) continue;
        if( iscntrl(readChar) ) continue;
        // read digits
        if( isdigit(readChar) ) {
            Token* pToken = nextToken();
            pToken->text += readChar;
            while( input.good() && isdigit(input.peek()) ){
                read();
                pToken->text += readChar;
            }
            if( input.good() && input.peek() == '.' ) {
                // this is a decimal; get the rest of it
                read();
                pToken->text += readChar;
                pToken->type = TokenType::DECIMAL;
                while( input.good() && isdigit(input.peek()) ){
                    read();
                    pToken->text += readChar;
                }
            } else {
              pToken->type = TokenType::NUMBER;
            }
            // validate that we don't have invalid trailing chars
            if( input.good() &&
                (input.peek() == '.' || isalpha(input.peek())) ){
                std::cerr << "Invalid token at line " << lineNumber << " column " << columnNumber << std::endl;
                return false;
            }
        }
    }
    return true;
}
