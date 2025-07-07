#include "Lexer.h"

#include <cctype>
#include <iostream>

using namespace basis;

basis::Lexer::~Lexer() {
    output.clear();
}

bool basis::Lexer::read() {
    readChar = 0;
    while(input.good()) {
        input.get(readChar);
        if( readChar == EOF ) break;
        if( readChar == '\n') {
          lineNumber++;
          columnNumber = 0;
          continue;
        }
        columnNumber++;
        return readChar != 0;
    }
    return false;
}

Token* basis::Lexer::nextToken() {
    // record and initialize the token
    output.emplace_back();
    Token* pToken = &output.back();
    pToken->lineNumber = lineNumber;
    pToken->columnNumber = columnNumber;
    // handle indent-based scope bounding
    while (!indents.empty() && indents.top()->columnNumber >= pToken->columnNumber) {
        indents.top()->bound = pToken;
        indents.pop();
    }
    indents.push(pToken);
    // done
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

    resWords[".alias"]     = TokenType::ALIAS;
    resWords[".class"]     = TokenType::CLASS;
    resWords[".cmd"]       = TokenType::COMMAND;
    resWords[".domain"]    = TokenType::DOMAIN;
    resWords[".enum"]      = TokenType::ENUMERATION;
    resWords[".intrinsic"] = TokenType::INTRINSIC;
    resWords[".object"]    = TokenType::OBJECT;
    resWords[".record"]    = TokenType::RECORD;
}

void basis::Lexer::writeError(const std::string& message, const Token* pToken) {
  std::cerr << message
            << " at line " << pToken->lineNumber
            << " column "  << pToken->columnNumber << std::endl;

}
bool basis::Lexer::scan(CompilerContext& context) {
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
        // read hexadecimals before numerics
        if( readChar == '0' && input.good() && input.peek() == 'x' && read()) {
            Token* pToken = nextToken();
            pToken->columnNumber -= 1; // correct for the extra 'x' character
            pToken->type = TokenType::HEXNUMBER;
            size_t hexDigitCount = 0;
            while( input.good() && isxdigit(input.peek()) && read() ) {
                pToken->text += readChar;
                hexDigitCount++;
            }
            // ensure we read an even number of digits so we have whole bytes
            if( hexDigitCount % 2 != 0 ) {
                output.pop_back(); // Remove the invalid token from output
                writeError("invalid hex value", pToken);
                return false;
            }
        }
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
                output.pop_back(); // Remove the invalid token from output
                writeError("invalid number", pToken);
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
          auto pv = resWords.find(pToken->text);
          if( pv == resWords.end() ) {
            writeError("invalid reserved word", pToken);
            output.pop_back(); // Remove the invalid token from output
            return false;
          }
          pToken->type = pv->second;
        }
        // read a string
        if( readChar == '"' ) {
            Token* pToken = nextToken();
            pToken->type = TokenType::STRING;
            bool foundClosingQuote = false;
            bool isValidString = true;
            while( input.good() && read() ) {
                if( readChar == '\n' ) {
                    isValidString = false;
                    break;
                }
                if( readChar == '"' ) {
                    // found closing quote - check if followed by alphanumeric character
                    if( input.good() && isalnum(input.peek()) ) {
                        isValidString = false;
                        break;
                    }
                    foundClosingQuote = true;
                    break;
                }
                if( readChar == '\\' ) {
                    // escape sequence
                    pToken->text += readChar;
                    if( input.good() && read() ) {
                        if( isalpha(readChar) || readChar == '\\' ) {
                            // valid following char
                            pToken->text += readChar;
                        } else {
                            // garbage following char
                            isValidString = false;
                            break;
                        }
                    } else {
                        isValidString = false;
                        break;
                    }
                } else {
                    pToken->text += readChar;
                }
            }
            if( !foundClosingQuote || !isValidString ) {
                writeError("invalid string", pToken);
                output.pop_back(); // Remove the invalid token from output
                return false;
            }
        }
    }
    return true;
}
