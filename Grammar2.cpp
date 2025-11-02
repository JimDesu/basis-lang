#include "Grammar2.h"
using namespace basis;


Grammar2::Grammar2() {
    initLiterals();
    initIdentifiers();
    initPunctuation();
    initReservedWords();
    initEnumerations();
    //TODO compilation unit
}

void Grammar2::initLiterals() {
   DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
   HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
   NUMBER = match(Production::NUMBER, TokenType::NUMBER);
   STRING = match(Production::STRING, TokenType::STRING);
   LITERAL = any(DECIMAL, HEXNUMBER, NUMBER, STRING);
}

void Grammar2::initIdentifiers() {
   IDENTIFIER = match(Production::IDENTIFIER, TokenType::IDENTIFIER);
   TYPENAME = match(Production::TYPENAME, TokenType::TYPENAME);
}

void Grammar2::initPunctuation() {
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
}

void Grammar2::initReservedWords() {
   ALIAS = match(Production::ALIAS, TokenType::ALIAS);
   CLASS = match(Production::CLASS, TokenType::CLASS);
   COMMAND = match(Production::COMMAND, TokenType::COMMAND);
   DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
   ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
   INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
   OBJECT = match(Production::OBJECT, TokenType::OBJECT);
   RECORD = match(Production::RECORD, TokenType::RECORD);
}

void Grammar2::initEnumerations() {
   DEF_ENUM_ITEM_LIST = separated(
       all(match(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER), EQUALS, LITERAL),
       COMMA);
   DEF_ENUM_NAME2 = maybe(match(Production::DEF_ENUM_NAME2, TokenType::TYPENAME));
   DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
   DEF_ENUM = boundedGroup(Production::DEF_ENUM,
       ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2, COLON, DEF_ENUM_ITEM_LIST);
}

Grammar2& basis::getGrammar() {
   static Grammar2 grammar{};
   return grammar;
}

