#include "Lexer.h"

#include <cctype>
#include <iostream>

using namespace basis;

basis::Lexer::~Lexer() {
    output.clear();
}

bool basis::Lexer::read() {
    if( !input.good() ) return false;
    char prevChar = readChar;
    readChar = 0;
    input.get(readChar);
    if (readChar == EOF || readChar == 0) return false;
    if ( prevChar == '\n' ) {
        lineNumber++;
        columnNumber = 1;
    } else {
        columnNumber++;
    }
    return true;
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

bool basis::Lexer::readHexNumber() {
    // read hexadecimals before numerics
    if( read()) {
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
        return true;
    }
    return false;
}

bool basis::Lexer::readNumeric() {
    // read numerics
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
    return true;
}

bool basis::Lexer::readIdentifier() {
    // read an identifier
    Token* pToken = nextToken();
    pToken->text += readChar;
    pToken->type = TokenType::IDENTIFIER;
    while( input.good() && isIdentifierChar(input.peek()) && read() ) {
        pToken->text += readChar;
    }
    return true;
}

bool basis::Lexer::readResWord() {
    // read a reserved word
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
    return true;
}

bool basis::Lexer::readString() {
    // read a string
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
    return true;
}

bool basis::Lexer::readPunct() {
    bool isValid = true;
    Token* pToken = nextToken();
    pToken->text += readChar;
    switch( readChar ) {
    case '&':
        pToken->type = TokenType::AMPERSAND;
        break;
    case '@':
        pToken->type = TokenType::AMPHORA;
        break;
    case '*':
        pToken->type = TokenType::ASTERISK;
        break;
    case '!':
        pToken->type = TokenType::BANG;
        break;
    case '^':
        pToken->type = TokenType::CARAT;
        break;
    case ',':
        pToken->type = TokenType::COMMA;
        break;
    case ':':
        if( input.good() && input.peek() == ':' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::DCOLON;
        } else if( input.good() && input.peek() == '<' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::COLANGLE;
        } else {
            pToken->type = TokenType::COLON;
        }
        break;
    case '=':
        pToken->type = TokenType::EQUALS;
        break;
    case '/':
        pToken->type = TokenType::SLASH;
        break;
    case '<':
        if ( input.good() && input.peek() == '-') {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::ASSIGN;
        } else {
            pToken->type = TokenType::LANGLE;
        }
        break;
    case '>':
        pToken->type = TokenType::RANGLE;
        break;
    case '{':
        pToken->type = TokenType::LBRACE;
        break;
    case '}':
        pToken->type = TokenType::RBRACE;
        break;
    case '[':
        pToken->type = TokenType::LBRACKET;
        break;
    case ']':
        pToken->type = TokenType::RBRACKET;
        break;
    case '(':
        pToken->type = TokenType::LPAREN;
        break;
    case ')':
        pToken->type = TokenType::RPAREN;
        break;
    case '+':
        pToken->type = TokenType::PLUS;
        break;
    case '-':
        pToken->type = TokenType::MINUS;
        break;
    case '?':
        if ( input.good() && input.peek() == '<' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::QLANGLE;
        } else {
            pToken->type = TokenType::QMARK;
        }
        break;
    default:
        isValid = false;
    }

    if ( !isValid ) {
        writeError("invalid punctuation", pToken);
        output.pop_back(); // Remove the invalid token from output
        return false;
    }
    return true;
}

bool basis::Lexer::checkHex() const {
    return readChar == '0' && input.good() && input.peek() == 'x';
}

bool basis::Lexer::checkDigit() const {
    return isdigit(readChar);
}

bool basis::Lexer::checkIdentifier() const {
    return isalpha(readChar) || (readChar == '\'' && input.good() && isalpha(input.peek()));
}

bool basis::Lexer::checkResWord() const {
    return readChar == '.' && input.good() && isalpha(input.peek());
}

bool basis::Lexer::checkString() const {
    return readChar == '"';
}

bool basis::Lexer::checkPunct() const {
    return ispunct(readChar);
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
bool basis::Lexer::scan(const CompilerContext& context) {
    if(!input.good()) return false;
    while( read() ) {
        /* --- special handling section --- */
        // read comments first
        if( readChar == ';' ) {
            // this is a comment; drain the remainder of the line without
            // any token being emitted.  Note that drainLine will read into the first
            // character of the next line, so don't continue back up to the read
            drainLine();
            // do not continue!
        }
        // skip over whitespace chars
        if( isspace(readChar) ) {
          // generate no tokens from whitespace, but respect the
          // tabwidth option
          if( readChar == '\t' ) {
              columnNumber += context.options.tabWidth - 1;
            }
            continue;
        }
        // skip over other control characters
        if( iscntrl(readChar) ) continue;
        /* --- regular handling section --- */
        // read hexadecimals before numerics
        if( checkHex() ) {
            if( !readHexNumber() ) return false;
            continue;
        }
        // read numerics
        if( checkDigit() ) {
            if( !readNumeric() ) return false;
            continue;
        }
        // read an identifier before punctuation
        if( checkIdentifier() ) {
            readIdentifier();
            continue;
        }
        // read a reserved word before punctuation
        if( checkResWord() ) {
          if( !readResWord() ) return false;
            continue;
        }
        // read a string before punctuation
        if( checkString() ) {
            if( !readString() ) return false;
            continue;
        }
        // read punctuation symbols
        if ( checkPunct() ) {
            if( !readPunct() ) return false;
            continue;
        }
    }
    return true;
}
