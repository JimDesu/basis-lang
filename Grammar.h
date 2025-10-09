#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "Parsing.h"

namespace basis {
    // production rule identifiers
    enum class Production {
        // ----- basic tokens -----
        // literals
        DECIMAL,
        HEXNUMBER,
        NUMBER,
        STRING,
        // identifiers
        IDENTIFIER,
        // reserved words
        ALIAS,
        CLASS,
        COMMAND,
        DOMAIN,
        ENUMERATION,
        INTRINSIC,
        OBJECT,
        RECORD,
        // punctuation
        AMBANG,
        AMPERSAND,
        AMPHORA,
        ASTERISK,
        BANG,
        BANGLANGLE,
        CARAT,
        CARATQ,
        COMMA,
        COLON,
        COLANGLE,
        DCOLON,
        EQUALS,
        LANGLE,
        LARROW,
        LBRACE,
        LBRACKET,
        LPAREN,
        MINUS,
        PERCENT,
        PIPE,
        PIPECOL,
        PLUS,
        QCOLON,
        QLANGLE,
        QMARK,
        QMINUS,
        RANGLE,
        RARROW,
        RBRACE,
        RBRACKET,
        RPAREN,
        SLASH,
        // ----- regular productions -----
        LITERAL,

        DEF_ENUM,
        DEF_ENUM_NAME1,
        DEF_ENUM_NAME2,
        DEF_ENUM_ITEM,
        DEF_ENUM_ITEM_NAME,
        DEF_ENUM_ITEM_LIST
    };

    /* Parser types for basic tokens */
    // literals
    using DECIMAL = Match<Production::DECIMAL, TokenType::DECIMAL>;
    using HEXNUMBER = Match<Production::HEXNUMBER, TokenType::HEXNUMBER>;
    using NUMBER = Match<Production::NUMBER, TokenType::NUMBER>;
    using STRING = Match<Production::STRING, TokenType::STRING>;
    // identifiers
    using IDENTIFIER = Match<Production::IDENTIFIER, TokenType::IDENTIFIER>;
    // reserved words
    using ALIAS = Match<Production::ALIAS, TokenType::ALIAS>;
    using CLASS = Match<Production::CLASS, TokenType::CLASS>;
    using COMMAND = Match<Production::COMMAND, TokenType::COMMAND>;
    using DOMAIN = Match<Production::DOMAIN, TokenType::DOMAIN>;
    using ENUMERATION = Match<Production::ENUMERATION, TokenType::ENUMERATION>;
    using INTRINSIC = Match<Production::INTRINSIC, TokenType::INTRINSIC>;
    using OBJECT = Match<Production::OBJECT, TokenType::OBJECT>;
    using RECORD = Match<Production::RECORD, TokenType::RECORD>;
    // punctuation
    using AMBANG = Match<Production::AMBANG, TokenType::AMBANG>;
    using AMPERSAND = Match<Production::AMPERSAND, TokenType::AMPERSAND>;
    using AMPHORA = Match<Production::AMPHORA, TokenType::AMPHORA>;
    using ASTERISK = Match<Production::ASTERISK, TokenType::ASTERISK>;
    using BANG = Match<Production::BANG, TokenType::BANG>;
    using BANGLANGLE = Match<Production::BANGLANGLE, TokenType::BANGLANGLE>;
    using CARAT = Match<Production::CARAT, TokenType::CARAT>;
    using CARATQ = Match<Production::CARATQ, TokenType::CARATQ>;
    using COMMA = Match<Production::COMMA, TokenType::COMMA>;
    using COLON = Match<Production::COLON, TokenType::COLON>;
    using COLANGLE = Match<Production::COLANGLE, TokenType::COLANGLE>;
    using DCOLON = Match<Production::DCOLON, TokenType::DCOLON>;
    using EQUALS = Match<Production::EQUALS, TokenType::EQUALS>;
    using LANGLE = Match<Production::LANGLE, TokenType::LANGLE>;
    using LARROW = Match<Production::LARROW, TokenType::LARROW>;
    using LBRACE = Match<Production::LBRACE, TokenType::LBRACE>;
    using LBRACKET = Match<Production::LBRACKET, TokenType::LBRACKET>;
    using LPAREN = Match<Production::LPAREN, TokenType::LPAREN>;
    using MINUS = Match<Production::MINUS, TokenType::MINUS>;
    using PERCENT = Match<Production::PERCENT, TokenType::PERCENT>;
    using PIPE = Match<Production::PIPE, TokenType::PIPE>;
    using PIPECOL = Match<Production::PIPECOL, TokenType::PIPECOL>;
    using PLUS = Match<Production::PLUS, TokenType::PLUS>;
    using QCOLON = Match<Production::QCOLON, TokenType::QCOLON>;
    using QLANGLE = Match<Production::QLANGLE, TokenType::QLANGLE>;
    using QMARK = Match<Production::QMARK, TokenType::QMARK>;
    using QMINUS = Match<Production::QMINUS, TokenType::QMINUS>;
    using RANGLE = Match<Production::RANGLE, TokenType::RANGLE>;
    using RARROW = Match<Production::RARROW, TokenType::RARROW>;
    using RBRACE = Match<Production::RBRACE, TokenType::RBRACE>;
    using RBRACKET = Match<Production::RBRACKET, TokenType::RBRACKET>;
    using RPAREN = Match<Production::RPAREN, TokenType::RPAREN>;
    using SLASH = Match<Production::SLASH, TokenType::SLASH>;

    using LITERAL = Any<DECIMAL, HEXNUMBER, NUMBER, STRING>;

    using DEF_ENUM_ITEM_NAME = Match<Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER>;
    using DEF_ENUM_ITEM = All<DEF_ENUM_ITEM_NAME, EQUALS, LITERAL>;
    using DEF_ENUM_ITEM_LIST = Separated<DEF_ENUM_ITEM, COMMA>;
    using DEF_ENUM_NAME2 = Maybe<Match<Production::DEF_ENUM_NAME2, TokenType::IDENTIFIER>>;
    using DEF_ENUM_NAME1 = Match<Production::DEF_ENUM_NAME1, TokenType::IDENTIFIER>;
    using DEF_ENUM = BoundedGroup<Production::DEF_ENUM,
                         ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2 , COLON, DEF_ENUM_ITEM_LIST>;
}

#endif // GRAMMAR_H

