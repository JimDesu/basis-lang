#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

namespace basis {
    // Grammar definitions using function objects instead of template metaprogramming

    // Literals
    inline const Match DECIMAL(Production::DECIMAL, TokenType::DECIMAL);
    inline const Match HEXNUMBER(Production::HEXNUMBER, TokenType::HEXNUMBER);
    inline const Match NUMBER(Production::NUMBER, TokenType::NUMBER);
    inline const Match STRING(Production::STRING, TokenType::STRING);

    // Identifiers
    inline const Match IDENTIFIER(Production::IDENTIFIER, TokenType::IDENTIFIER);
    inline const Match TYPENAME(Production::TYPENAME, TokenType::TYPENAME);

    // Reserved words
    inline const Match ALIAS(Production::ALIAS, TokenType::ALIAS);
    inline const Match CLASS(Production::CLASS, TokenType::CLASS);
    inline const Match COMMAND(Production::COMMAND, TokenType::COMMAND);
    inline const Match DOMAIN(Production::DOMAIN, TokenType::DOMAIN);
    inline const Match ENUMERATION(Production::ENUMERATION, TokenType::ENUMERATION);
    inline const Match INTRINSIC(Production::INTRINSIC, TokenType::INTRINSIC);
    inline const Match OBJECT(Production::OBJECT, TokenType::OBJECT);
    inline const Match RECORD(Production::RECORD, TokenType::RECORD);

    // Punctuation
    inline const Match AMBANG(Production::AMBANG, TokenType::AMBANG);
    inline const Match AMPERSAND(Production::AMPERSAND, TokenType::AMPERSAND);
    inline const Match AMPHORA(Production::AMPHORA, TokenType::AMPHORA);
    inline const Match ASTERISK(Production::ASTERISK, TokenType::ASTERISK);
    inline const Match BANG(Production::BANG, TokenType::BANG);
    inline const Match BANGLANGLE(Production::BANGLANGLE, TokenType::BANGLANGLE);
    inline const Match CARAT(Production::CARAT, TokenType::CARAT);
    inline const Match CARATQ(Production::CARATQ, TokenType::CARATQ);
    inline const Match COMMA(Production::COMMA, TokenType::COMMA);
    inline const Match COLON(Production::COLON, TokenType::COLON);
    inline const Match COLANGLE(Production::COLANGLE, TokenType::COLANGLE);
    inline const Match DCOLON(Production::DCOLON, TokenType::DCOLON);
    inline const Match EQUALS(Production::EQUALS, TokenType::EQUALS);
    inline const Match LANGLE(Production::LANGLE, TokenType::LANGLE);
    inline const Match LARROW(Production::LARROW, TokenType::LARROW);
    inline const Match LBRACE(Production::LBRACE, TokenType::LBRACE);
    inline const Match LBRACKET(Production::LBRACKET, TokenType::LBRACKET);
    inline const Match LPAREN(Production::LPAREN, TokenType::LPAREN);
    inline const Match MINUS(Production::MINUS, TokenType::MINUS);
    inline const Match PERCENT(Production::PERCENT, TokenType::PERCENT);
    inline const Match PIPE(Production::PIPE, TokenType::PIPE);
    inline const Match PIPECOL(Production::PIPECOL, TokenType::PIPECOL);
    inline const Match PLUS(Production::PLUS, TokenType::PLUS);
    inline const Match POUND(Production::POUND, TokenType::POUND);
    inline const Match QCOLON(Production::QCOLON, TokenType::QCOLON);
    inline const Match QLANGLE(Production::QLANGLE, TokenType::QLANGLE);
    inline const Match QMARK(Production::QMARK, TokenType::QMARK);
    inline const Match QMINUS(Production::QMINUS, TokenType::QMINUS);
    inline const Match RANGLE(Production::RANGLE, TokenType::RANGLE);
    inline const Match RARROW(Production::RARROW, TokenType::RARROW);
    inline const Match RBRACE(Production::RBRACE, TokenType::RBRACE);
    inline const Match RBRACKET(Production::RBRACKET, TokenType::RBRACKET);
    inline const Match RPAREN(Production::RPAREN, TokenType::RPAREN);
    inline const Match SLASH(Production::SLASH, TokenType::SLASH);

    // Composite grammar rules
    inline const Any LITERAL({&DECIMAL, &HEXNUMBER, &NUMBER, &STRING});

    // Enumerations
    inline const Match DEF_ENUM_ITEM_NAME(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER);
    inline const All DEF_ENUM_ITEM({&DEF_ENUM_ITEM_NAME, &EQUALS, &LITERAL});
    inline const Separated DEF_ENUM_ITEM_LIST(DEF_ENUM_ITEM, COMMA);
    inline const Match DEF_ENUM_NAME2_MATCH(Production::DEF_ENUM_NAME2, TokenType::TYPENAME);
    inline const Maybe DEF_ENUM_NAME2(DEF_ENUM_NAME2_MATCH);
    inline const Match DEF_ENUM_NAME1(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
    inline const BoundedGroup DEF_ENUM(Production::DEF_ENUM,
        {&ENUMERATION, &DEF_ENUM_NAME1, &DEF_ENUM_NAME2, &COLON, &DEF_ENUM_ITEM_LIST});

}

#endif // GRAMMAR2_H

