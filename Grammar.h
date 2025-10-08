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
    using DECIMAL = Parser<Match<Production::DECIMAL, TokenType::DECIMAL>>;
    using HEXNUMBER = Parser<Match<Production::HEXNUMBER, TokenType::HEXNUMBER>>;
    using NUMBER = Parser<Match<Production::NUMBER, TokenType::NUMBER>>;
    using STRING = Parser<Match<Production::STRING, TokenType::STRING>>;
    // identifiers
    using IDENTIFIER = Parser<Match<Production::IDENTIFIER, TokenType::IDENTIFIER>>;
    // reserved words
    using ALIAS = Parser<Match<Production::ALIAS, TokenType::ALIAS>>;
    using CLASS = Parser<Match<Production::CLASS, TokenType::CLASS>>;
    using COMMAND = Parser<Match<Production::COMMAND, TokenType::COMMAND>>;
    using DOMAIN = Parser<Match<Production::DOMAIN, TokenType::DOMAIN>>;
    using ENUMERATION = Parser<Match<Production::ENUMERATION, TokenType::ENUMERATION>>;
    using INTRINSIC = Parser<Match<Production::INTRINSIC, TokenType::INTRINSIC>>;
    using OBJECT = Parser<Match<Production::OBJECT, TokenType::OBJECT>>;
    using RECORD = Parser<Match<Production::RECORD, TokenType::RECORD>>;
    // punctuation
    using AMBANG = Parser<Match<Production::AMBANG, TokenType::AMBANG>>;
    using AMPERSAND = Parser<Match<Production::AMPERSAND, TokenType::AMPERSAND>>;
    using AMPHORA = Parser<Match<Production::AMPHORA, TokenType::AMPHORA>>;
    using ASTERISK = Parser<Match<Production::ASTERISK, TokenType::ASTERISK>>;
    using BANG = Parser<Match<Production::BANG, TokenType::BANG>>;
    using BANGLANGLE = Parser<Match<Production::BANGLANGLE, TokenType::BANGLANGLE>>;
    using CARAT = Parser<Match<Production::CARAT, TokenType::CARAT>>;
    using CARATQ = Parser<Match<Production::CARATQ, TokenType::CARATQ>>;
    using COMMA = Parser<Match<Production::COMMA, TokenType::COMMA>>;
    using COLON = Parser<Match<Production::COLON, TokenType::COLON>>;
    using COLANGLE = Parser<Match<Production::COLANGLE, TokenType::COLANGLE>>;
    using DCOLON = Parser<Match<Production::DCOLON, TokenType::DCOLON>>;
    using EQUALS = Parser<Match<Production::EQUALS, TokenType::EQUALS>>;
    using LANGLE = Parser<Match<Production::LANGLE, TokenType::LANGLE>>;
    using LARROW = Parser<Match<Production::LARROW, TokenType::LARROW>>;
    using LBRACE = Parser<Match<Production::LBRACE, TokenType::LBRACE>>;
    using LBRACKET = Parser<Match<Production::LBRACKET, TokenType::LBRACKET>>;
    using LPAREN = Parser<Match<Production::LPAREN, TokenType::LPAREN>>;
    using MINUS = Parser<Match<Production::MINUS, TokenType::MINUS>>;
    using PERCENT = Parser<Match<Production::PERCENT, TokenType::PERCENT>>;
    using PIPE = Parser<Match<Production::PIPE, TokenType::PIPE>>;
    using PIPECOL = Parser<Match<Production::PIPECOL, TokenType::PIPECOL>>;
    using PLUS = Parser<Match<Production::PLUS, TokenType::PLUS>>;
    using QCOLON = Parser<Match<Production::QCOLON, TokenType::QCOLON>>;
    using QLANGLE = Parser<Match<Production::QLANGLE, TokenType::QLANGLE>>;
    using QMARK = Parser<Match<Production::QMARK, TokenType::QMARK>>;
    using QMINUS = Parser<Match<Production::QMINUS, TokenType::QMINUS>>;
    using RANGLE = Parser<Match<Production::RANGLE, TokenType::RANGLE>>;
    using RARROW = Parser<Match<Production::RARROW, TokenType::RARROW>>;
    using RBRACE = Parser<Match<Production::RBRACE, TokenType::RBRACE>>;
    using RBRACKET = Parser<Match<Production::RBRACKET, TokenType::RBRACKET>>;
    using RPAREN = Parser<Match<Production::RPAREN, TokenType::RPAREN>>;
    using SLASH = Parser<Match<Production::SLASH, TokenType::SLASH>>;

    using LITERAL = Parser<Any<DECIMAL, HEXNUMBER, NUMBER, STRING>>;

    using DEF_ENUM_ITEM_NAME = Parser<Match<Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER>>;
    using DEF_ENUM_ITEM = Parser<All<DEF_ENUM_ITEM_NAME, EQUALS, LITERAL>>;
    using DEF_ENUM_ITEM_LIST = Parser<Separated<DEF_ENUM_ITEM, COMMA>>;
    using DEF_ENUM_NAME2 = Parser<Maybe<Match<Production::DEF_ENUM_NAME2, TokenType::IDENTIFIER>>>;
    using DEF_ENUM_NAME1 = Parser<Match<Production::DEF_ENUM_NAME1, TokenType::IDENTIFIER>>;
    using DEF_ENUM = Parser<BoundedGroup<Production::DEF_ENUM,
                         ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2 , COLON, DEF_ENUM_ITEM_LIST>>;
}

#endif // GRAMMAR_H

