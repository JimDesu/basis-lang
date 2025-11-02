#include "Grammar2.h"
using namespace basis;

namespace basis {
    namespace grammar {
        // Define everything ahead of time here so that we don't have to worry about
        // forward/circular references when we initialize everything.
        SPPF DECIMAL;
        SPPF HEXNUMBER;
        SPPF NUMBER;
        SPPF STRING;
        SPPF IDENTIFIER;
        SPPF TYPENAME;
        SPPF ALIAS;
        SPPF CLASS;
        SPPF COMMAND;
        SPPF DOMAIN;
        SPPF ENUMERATION;
        SPPF INTRINSIC;
        SPPF OBJECT;
        SPPF RECORD;
        SPPF AMBANG;
        SPPF AMPERSAND;
        SPPF AMPHORA;
        SPPF ASTERISK;
        SPPF BANG;
        SPPF BANGLANGLE;
        SPPF CARAT;
        SPPF CARATQ;
        SPPF COMMA;
        SPPF COLON;
        SPPF COLANGLE;
        SPPF DCOLON;
        SPPF EQUALS;
        SPPF LANGLE;
        SPPF LARROW;
        SPPF LBRACE;
        SPPF LBRACKET;
        SPPF LPAREN;
        SPPF MINUS;
        SPPF PERCENT;
        SPPF PIPE;
        SPPF PIPECOL;
        SPPF PLUS;
        SPPF POUND;
        SPPF QCOLON;
        SPPF QLANGLE;
        SPPF QMARK;
        SPPF QMINUS;
        SPPF RANGLE;
        SPPF RARROW;
        SPPF RBRACE;
        SPPF RBRACKET;
        SPPF RPAREN;
        SPPF SLASH;
        SPPF LITERAL;
        SPPF DEF_ENUM_ITEM_LIST;
        SPPF DEF_ENUM_NAME2;
        SPPF DEF_ENUM_NAME1;
        SPPF DEF_ENUM;
        SPPF COMPILATION_UNIT;
    }
}

SPPF grammar::initGrammar() {
     DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
     HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
     NUMBER = match(Production::NUMBER, TokenType::NUMBER);
     STRING = match(Production::STRING, TokenType::STRING);

     IDENTIFIER = match(Production::IDENTIFIER, TokenType::IDENTIFIER);
     TYPENAME = match(Production::TYPENAME, TokenType::TYPENAME);

        // Reserved words
     ALIAS = match(Production::ALIAS, TokenType::ALIAS);
     CLASS = match(Production::CLASS, TokenType::CLASS);
     COMMAND = match(Production::COMMAND, TokenType::COMMAND);
     DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
     ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
     INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
     OBJECT = match(Production::OBJECT, TokenType::OBJECT);
     RECORD = match(Production::RECORD, TokenType::RECORD);

        // Punctuation
     AMBANG = match(Production::AMBANG, TokenType::AMBANG);
     AMPERSAND = match(Production::AMPERSAND, TokenType::AMPERSAND);
     AMPHORA = match(Production::AMPHORA, TokenType::AMPHORA);
     ASTERISK = match(Production::ASTERISK, TokenType::ASTERISK);
     BANG = match(Production::BANG, TokenType::BANG);
     BANGLANGLE = match(Production::BANGLANGLE, TokenType::BANGLANGLE);
     CARAT = match(Production::CARAT, TokenType::CARAT);
     CARATQ = match(Production::CARATQ, TokenType::CARATQ);
     COMMA = match(Production::COMMA, TokenType::COMMA);
     COLON = match(Production::COLON, TokenType::COLON);
     COLANGLE = match(Production::COLANGLE, TokenType::COLANGLE);
     DCOLON = match(Production::DCOLON, TokenType::DCOLON);
     EQUALS = match(Production::EQUALS, TokenType::EQUALS);
     LANGLE = match(Production::LANGLE, TokenType::LANGLE);
     LARROW = match(Production::LARROW, TokenType::LARROW);
     LBRACE = match(Production::LBRACE, TokenType::LBRACE);
     LBRACKET = match(Production::LBRACKET, TokenType::LBRACKET);
     LPAREN = match(Production::LPAREN, TokenType::LPAREN);
     MINUS = match(Production::MINUS, TokenType::MINUS);
     PERCENT = match(Production::PERCENT, TokenType::PERCENT);
     PIPE = match(Production::PIPE, TokenType::PIPE);
     PIPECOL = match(Production::PIPECOL, TokenType::PIPECOL);
     PLUS = match(Production::PLUS, TokenType::PLUS);
     POUND = match(Production::POUND, TokenType::POUND);
     QCOLON = match(Production::QCOLON, TokenType::QCOLON);
     QLANGLE = match(Production::QLANGLE, TokenType::QLANGLE);
     QMARK = match(Production::QMARK, TokenType::QMARK);
     QMINUS = match(Production::QMINUS, TokenType::QMINUS);
     RANGLE = match(Production::RANGLE, TokenType::RANGLE);
     RARROW = match(Production::RARROW, TokenType::RARROW);
     RBRACE = match(Production::RBRACE, TokenType::RBRACE);
     RBRACKET = match(Production::RBRACKET, TokenType::RBRACKET);
     RPAREN = match(Production::RPAREN, TokenType::RPAREN);
     SLASH = match(Production::SLASH, TokenType::SLASH);

        // Composite grammar rules
     LITERAL = any(DECIMAL, HEXNUMBER, NUMBER, STRING);

        // Enumerations
     DEF_ENUM_ITEM_LIST = separated(
            all(match(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER), EQUALS, LITERAL),
           COMMA);
     DEF_ENUM_NAME2 = maybe(match(Production::DEF_ENUM_NAME2, TokenType::TYPENAME));
     DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
     DEF_ENUM = boundedGroup(Production::DEF_ENUM,
            ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2, COLON, DEF_ENUM_ITEM_LIST);

        // temporary definition; we'll build up to this
     COMPILATION_UNIT = match(Production::DECIMAL, TokenType::DECIMAL);
   return COMPILATION_UNIT;
}

SPPF basis::getGrammar() {
   static SPPF parseFn = grammar::initGrammar();
   return parseFn;
}

