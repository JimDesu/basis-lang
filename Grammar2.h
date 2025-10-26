#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

namespace basis {

    struct Grammar2 {
        // Literals
        inline static const SPFN DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
        inline static const SPFN HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
        inline static const SPFN NUMBER = match(Production::NUMBER, TokenType::NUMBER);
        inline static const SPFN STRING = match(Production::STRING, TokenType::STRING);

        // Identifiers
        inline static const SPFN IDENTIFIER = match(Production::IDENTIFIER, TokenType::IDENTIFIER);
        inline static const SPFN TYPENAME = match(Production::TYPENAME, TokenType::TYPENAME);

        // Reserved words
        inline static const SPFN ALIAS = match(Production::ALIAS, TokenType::ALIAS);
        inline static const SPFN CLASS = match(Production::CLASS, TokenType::CLASS);
        inline static const SPFN COMMAND = match(Production::COMMAND, TokenType::COMMAND);
        inline static const SPFN DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
        inline static const SPFN ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
        inline static const SPFN INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
        inline static const SPFN OBJECT = match(Production::OBJECT, TokenType::OBJECT);
        inline static const SPFN RECORD = match(Production::RECORD, TokenType::RECORD);

        // Punctuation
        inline static const SPFN AMBANG = match(Production::AMBANG, TokenType::AMBANG);
        inline static const SPFN AMPERSAND = match(Production::AMPERSAND, TokenType::AMPERSAND);
        inline static const SPFN AMPHORA = match(Production::AMPHORA, TokenType::AMPHORA);
        inline static const SPFN ASTERISK = match(Production::ASTERISK, TokenType::ASTERISK);
        inline static const SPFN BANG = match(Production::BANG, TokenType::BANG);
        inline static const SPFN BANGLANGLE = match(Production::BANGLANGLE, TokenType::BANGLANGLE);
        inline static const SPFN CARAT = match(Production::CARAT, TokenType::CARAT);
        inline static const SPFN CARATQ = match(Production::CARATQ, TokenType::CARATQ);
        inline static const SPFN COMMA = match(Production::COMMA, TokenType::COMMA);
        inline static const SPFN COLON = match(Production::COLON, TokenType::COLON);
        inline static const SPFN COLANGLE = match(Production::COLANGLE, TokenType::COLANGLE);
        inline static const SPFN DCOLON = match(Production::DCOLON, TokenType::DCOLON);
        inline static const SPFN EQUALS = match(Production::EQUALS, TokenType::EQUALS);
        inline static const SPFN LANGLE = match(Production::LANGLE, TokenType::LANGLE);
        inline static const SPFN LARROW = match(Production::LARROW, TokenType::LARROW);
        inline static const SPFN LBRACE = match(Production::LBRACE, TokenType::LBRACE);
        inline static const SPFN LBRACKET = match(Production::LBRACKET, TokenType::LBRACKET);
        inline static const SPFN LPAREN = match(Production::LPAREN, TokenType::LPAREN);
        inline static const SPFN MINUS = match(Production::MINUS, TokenType::MINUS);
        inline static const SPFN PERCENT = match(Production::PERCENT, TokenType::PERCENT);
        inline static const SPFN PIPE = match(Production::PIPE, TokenType::PIPE);
        inline static const SPFN PIPECOL = match(Production::PIPECOL, TokenType::PIPECOL);
        inline static const SPFN PLUS = match(Production::PLUS, TokenType::PLUS);
        inline static const SPFN POUND = match(Production::POUND, TokenType::POUND);
        inline static const SPFN QCOLON = match(Production::QCOLON, TokenType::QCOLON);
        inline static const SPFN QLANGLE = match(Production::QLANGLE, TokenType::QLANGLE);
        inline static const SPFN QMARK = match(Production::QMARK, TokenType::QMARK);
        inline static const SPFN QMINUS = match(Production::QMINUS, TokenType::QMINUS);
        inline static const SPFN RANGLE = match(Production::RANGLE, TokenType::RANGLE);
        inline static const SPFN RARROW = match(Production::RARROW, TokenType::RARROW);
        inline static const SPFN RBRACE = match(Production::RBRACE, TokenType::RBRACE);
        inline static const SPFN RBRACKET = match(Production::RBRACKET, TokenType::RBRACKET);
        inline static const SPFN RPAREN = match(Production::RPAREN, TokenType::RPAREN);
        inline static const SPFN SLASH = match(Production::SLASH, TokenType::SLASH);

        // Composite grammar rules
        inline static const SPFN LITERAL = any(DECIMAL, HEXNUMBER, NUMBER, STRING);

        // Enumerations
        inline static const SPFN DEF_ENUM_ITEM_LIST = separated(
            all(match(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER), EQUALS, LITERAL),
           COMMA);
        inline static const SPFN DEF_ENUM_NAME2 = maybe(match(Production::DEF_ENUM_NAME2, TokenType::TYPENAME));
        inline static const SPFN DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
        inline static const SPFN DEF_ENUM = boundedGroup(Production::DEF_ENUM,
            ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2, COLON, DEF_ENUM_ITEM_LIST);
        /*
         TODO: fix up this stuff from the template version
        using DEF_CMD_BODY = int; //TODO
    using DEF_CMD_PARMTYPE_DECO = Maybe<OneOrMore<Any<AMPHORA, All<LBRACKET, Maybe<NUMBER>, RBRACKET>>>>;
    using DEF_CMD_PARMTYPE_NAME =
              All<DEF_CMD_PARMTYPE_DECO, Match<Production::DEF_CMD_PARMTYPE_NAME, TokenType::TYPENAME>>;

    using DEF_CMD_PARMTYPE_EXPR_INTR = All<DEF_CMD_PARMTYPE_NAME, POUND, TYPENAME >;
    using DEF_CMF_PARMTYPE_EXPR = All<LPAREN, Any<DEF_CMD_PARMTYPE_EXPR_INTR>, RPAREN>;

    using DEF_CMD_PARM_TYPE = Any<DEF_CMF_PARMTYPE_EXPR, DEF_CMD_PARMTYPE_NAME>;
    using DEF_CMD_PARM_NAME = Match<Production::DEF_CMD_PARM_NAME, TokenType::IDENTIFIER>;
    using DEF_CMD_PARM = All<DEF_CMD_PARM_TYPE, DEF_CMD_PARM_NAME>;
    using DEF_CMD_PARMS = Maybe<All<COLON, Separated<DEF_CMD_PARM, COMMA>>>;
    using DEF_CMD_IMPARMS = Maybe<All<SLASH, Separated<DEF_CMD_PARM, COMMA>>>;
    using DEF_CMD_RETVAL = Maybe<All<RARROW, DEF_CMD_PARM_NAME>>;
    using DEF_CMD_NAME = Match<Production::DEF_CMD_NAME, TokenType::IDENTIFIER>;
    using DEF_CMD = BoundedGroup<Production::DEF_CMD,
              COMMAND, DEF_CMD_NAME, DEF_CMD_PARMS, DEF_CMD_IMPARMS, DEF_CMD_RETVAL, DEF_CMD_BODY>;
         */

    };

}

#endif // GRAMMAR2_H

