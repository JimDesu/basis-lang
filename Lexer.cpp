#include "Lexer.h"

#include <cctype>
#include <iostream>

using namespace basis;

Lexer::~Lexer() {
    output.clear();
}

bool Lexer::scan() {
    while( read() ) {
        for ( int i = 0; i < fnCount; i++ ) {
           if ( (this->*checks[i])() ) {
               if ( !(this->*reads[i])() ) return false;
               break;
           }
        }
    }
    return true;
}

const std::map<std::string, TokenType> Lexer::resWords {
    {".alias", TokenType::ALIAS},
    {".class", TokenType::CLASS},
    {".cmd", TokenType::COMMAND},
    {".domain", TokenType::DOMAIN},
    {".enum", TokenType::ENUMERATION},
    {".intrinsic", TokenType::INTRINSIC},
    {".object", TokenType::OBJECT},
    {".record", TokenType::RECORD}
};

bool Lexer::read() {
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

Token* Lexer::nextToken() {
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

bool Lexer::checkComment() const {
    return (readChar == ';');
}

bool Lexer::checkWhitespace() const {
    return isspace(readChar) || iscntrl(readChar);
}

bool Lexer::checkHex() const {
    return readChar == '0' && input.good() && input.peek() == 'x';
}

bool Lexer::checkNumeric() const {
    return isdigit(readChar);
}

bool Lexer::checkIdentifier() const {
    return isalpha(readChar) || (readChar == '\'' && input.good() && isalpha(input.peek()));
}

bool Lexer::checkResWord() const {
    return readChar == '.' && input.good() && isalpha(input.peek());
}

bool Lexer::checkString() const {
    return readChar == '"';
}

bool Lexer::checkPunct() const {
    return ispunct(readChar);
}

bool Lexer::isIdentifierChar(char c) {
    return c == '_' || isalnum(c);
}

void Lexer::writeError(const std::string& message, const Token* pToken) {
  std::cerr << message
            << " at line " << pToken->lineNumber
            << " column "  << pToken->columnNumber << std::endl;

}
bool Lexer::readComment() {
    // get the current line number
    size_t line = lineNumber;
    // blindly read until we're on the next line
    while( read() && readChar != '\n');
    return true;
}

bool Lexer::readWhitespace() {
    // generate no tokens from whitespace
    return true;
}

bool Lexer::readHex() {
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

bool Lexer::readNumeric() {
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

bool Lexer::readIdentifier() {
    // read an identifier
    Token* pToken = nextToken();
    pToken->text += readChar;
    pToken->type = TokenType::IDENTIFIER;
    while( input.good() && isIdentifierChar(input.peek()) && read() ) {
        pToken->text += readChar;
    }
    return true;
}

bool Lexer::readResWord() {
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

bool Lexer::readString() {
    // read a string
    Token* pToken = nextToken();
    pToken->type = TokenType::STRING;
    bool foundClosingQuote = false;
    bool isValidString = true;
    while(isValidString && input.good() && read() ) {
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
            if( input.good() && read() ) {
                switch ( readChar ) {
                case '"':
                    pToken->text += '"';
                    break;
                case '\\':
                    pToken->text += '\\';
                    break;
                case 'n':
                    pToken->text += '\n';
                    break;
                case 'r':
                    pToken->text += '\r';
                    break;
                case 'b':
                    pToken->text += '\b';
                    break;
                case 't':
                    pToken->text += '\t';
                    break;
                case 'v':
                    pToken->text += '\v';
                    break;
                default:
                    isValidString = false;
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

bool Lexer::readPunct() {
    Token* pToken = nextToken();
    pToken->text += readChar;
    switch( readChar ) {
    case '&':
        pToken->type = TokenType::AMPERSAND;
        break;
    case '@':
        pToken->type = TokenType::AMPHORA;
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
    case '*':
        pToken->type = TokenType::ASTERISK;
        break;
    case '!':
        if ( input.good() && input.peek() == '<' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::BANGLANGLE;
        } else {
            pToken->type = TokenType::BANG;
        }
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
    case '{':
        pToken->type = TokenType::LBRACE;
        break;
    case '[':
        pToken->type = TokenType::LBRACKET;
        break;
    case '(':
        pToken->type = TokenType::LPAREN;
        break;
    case '-':
        pToken->type = TokenType::MINUS;
        break;
    case '+':
        pToken->type = TokenType::PLUS;
        break;
    case '?':
        if ( input.good() && input.peek() == '<' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::QLANGLE;
        } else if ( input.good() && input.peek() == ':' ) {
            read();
            pToken->text += readChar;
            pToken->type = TokenType::QCOLON;
        } else {
            pToken->type = TokenType::QMARK;
        }
        break;
    case '>':
        pToken->type = TokenType::RANGLE;
        break;
    case '}':
        pToken->type = TokenType::RBRACE;
        break;
    case ']':
        pToken->type = TokenType::RBRACKET;
        break;
    case ')':
        pToken->type = TokenType::RPAREN;
        break;
    case '/':
        pToken->type = TokenType::SLASH;
        break;
    default:
        writeError("invalid punctuation", pToken);
        output.pop_back(); // Remove the invalid token from output
        return false;
    }
    return true;
}
