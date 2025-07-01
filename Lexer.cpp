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

bool basis::Lexer::isIdentifierChar(char c) {
    return c == '_' || isalnum(c);
}

void basis::Lexer::loadReservedWords() {
    reswords[".cmd"] = TokenType::COMMAND;
}

void basis::Lexer::writeError(std::string message, int lineNo, int columnNo) {
  std::cerr << message
            << " at line " << lineNo
            << " column " << columnNo << std::endl;

}
bool basis::Lexer::scan(CompilerContext& context) {
    output.clear();
    indents.clear();
    if(!input.good()) return false;
    while( read() ) {
        // skip over whitespace chars
        if( isspace(readChar) ) {
          // generate no tokens from whitespace, but respect the
          // tabwidth option
          if( readChar == '\t' ) {
              columnNumber += context.options.tabWidth - 1;
            }
        }
        if( iscntrl(readChar) ) continue;
        // read comments
        if( readChar == ';' ) {
            // this is a comment; drain the remainder of the line without
            // any token being emitted
            drainLine();
        }
        // TODO read hexadecimals before numerics
        // read numerics
        if( isdigit(readChar) ) {
            Token* pToken = nextToken();
            pToken->text += readChar;
            while( input.good() && isdigit(input.peek()) ){
                read();
                pToken->text += readChar;
            }
            if( input.good() && input.peek() == '.' && read()) {
                // this is a decimal; get the rest of it
                pToken->text += readChar;
                pToken->type = TokenType::DECIMAL;
                while( input.good() && isdigit(input.peek()) && read()){
                    pToken->text += readChar;
                }
            } else {
              pToken->type = TokenType::NUMBER;
            }
            // validate that we don't have invalid trailing chars
            if( input.good() &&
                (input.peek() == '.' || isalpha(input.peek())) ){
                writeError("invalid number", pToken->lineNumber, pToken->columnNumber);
                return false;
            }
        }
        // read an identifier
        if( isalpha(readChar) ||
            (readChar == '\'' && input.good() && isalpha(input.peek())) ) {
            Token* pToken = nextToken();
            pToken->text += readChar;
            pToken->type = TokenType::IDENTIFIER;
            while( input.good() && isIdentifierChar(input.peek()) && read() ) {
                pToken->text += readChar;
            }
        }
        // read a reserved word
        if( readChar == '.' && input.good() && isalpha(input.peek()) ) {
          Token* pToken = nextToken();
          pToken->text += readChar;
          while( input.good() && isIdentifierChar(input.peek()) && read() ) {
                pToken->text += readChar;
          }
          if( reswords.find(pToken->text) == reswords.end() ) {
            writeError("invalid reserved word", pToken->lineNumber, pToken->columnNumber);
            return false;
          }
          pToken->type = reswords[pToken->text];
        }
    }
    return true;
}
