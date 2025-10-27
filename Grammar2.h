#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

namespace basis {

    struct Grammar2 {
        // Literals
        inline static const SPPF DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
        inline static const SPPF HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
        inline static const SPPF NUMBER = match(Production::NUMBER, TokenType::NUMBER);
        inline static const SPPF STRING = match(Production::STRING, TokenType::STRING);

        // Identifiers
        inline static const SPPF IDENTIFIER = match(Production::IDENTIFIER, TokenType::IDENTIFIER);
        inline static const SPPF TYPENAME = match(Production::TYPENAME, TokenType::TYPENAME);

        // Reserved words
        inline static const SPPF ALIAS = match(Production::ALIAS, TokenType::ALIAS);
        inline static const SPPF CLASS = match(Production::CLASS, TokenType::CLASS);
        inline static const SPPF COMMAND = match(Production::COMMAND, TokenType::COMMAND);
        inline static const SPPF DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
        inline static const SPPF ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
        inline static const SPPF INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
        inline static const SPPF OBJECT = match(Production::OBJECT, TokenType::OBJECT);
        inline static const SPPF RECORD = match(Production::RECORD, TokenType::RECORD);

        // Punctuation
        inline static const SPPF AMBANG = match(Production::AMBANG, TokenType::AMBANG);
        inline static const SPPF AMPERSAND = match(Production::AMPERSAND, TokenType::AMPERSAND);
        inline static const SPPF AMPHORA = match(Production::AMPHORA, TokenType::AMPHORA);
        inline static const SPPF ASTERISK = match(Production::ASTERISK, TokenType::ASTERISK);
        inline static const SPPF BANG = match(Production::BANG, TokenType::BANG);
        inline static const SPPF BANGLANGLE = match(Production::BANGLANGLE, TokenType::BANGLANGLE);
        inline static const SPPF CARAT = match(Production::CARAT, TokenType::CARAT);
        inline static const SPPF CARATQ = match(Production::CARATQ, TokenType::CARATQ);
        inline static const SPPF COMMA = match(Production::COMMA, TokenType::COMMA);
        inline static const SPPF COLON = match(Production::COLON, TokenType::COLON);
        inline static const SPPF COLANGLE = match(Production::COLANGLE, TokenType::COLANGLE);
        inline static const SPPF DCOLON = match(Production::DCOLON, TokenType::DCOLON);
        inline static const SPPF EQUALS = match(Production::EQUALS, TokenType::EQUALS);
        inline static const SPPF LANGLE = match(Production::LANGLE, TokenType::LANGLE);
        inline static const SPPF LARROW = match(Production::LARROW, TokenType::LARROW);
        inline static const SPPF LBRACE = match(Production::LBRACE, TokenType::LBRACE);
        inline static const SPPF LBRACKET = match(Production::LBRACKET, TokenType::LBRACKET);
        inline static const SPPF LPAREN = match(Production::LPAREN, TokenType::LPAREN);
        inline static const SPPF MINUS = match(Production::MINUS, TokenType::MINUS);
        inline static const SPPF PERCENT = match(Production::PERCENT, TokenType::PERCENT);
        inline static const SPPF PIPE = match(Production::PIPE, TokenType::PIPE);
        inline static const SPPF PIPECOL = match(Production::PIPECOL, TokenType::PIPECOL);
        inline static const SPPF PLUS = match(Production::PLUS, TokenType::PLUS);
        inline static const SPPF POUND = match(Production::POUND, TokenType::POUND);
        inline static const SPPF QCOLON = match(Production::QCOLON, TokenType::QCOLON);
        inline static const SPPF QLANGLE = match(Production::QLANGLE, TokenType::QLANGLE);
        inline static const SPPF QMARK = match(Production::QMARK, TokenType::QMARK);
        inline static const SPPF QMINUS = match(Production::QMINUS, TokenType::QMINUS);
        inline static const SPPF RANGLE = match(Production::RANGLE, TokenType::RANGLE);
        inline static const SPPF RARROW = match(Production::RARROW, TokenType::RARROW);
        inline static const SPPF RBRACE = match(Production::RBRACE, TokenType::RBRACE);
        inline static const SPPF RBRACKET = match(Production::RBRACKET, TokenType::RBRACKET);
        inline static const SPPF RPAREN = match(Production::RPAREN, TokenType::RPAREN);
        inline static const SPPF SLASH = match(Production::SLASH, TokenType::SLASH);

        // Composite grammar rules
        inline static const SPPF LITERAL = any(DECIMAL, HEXNUMBER, NUMBER, STRING);

        // Enumerations
        inline static const SPPF DEF_ENUM_ITEM_LIST = separated(
            all(match(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER), EQUALS, LITERAL),
           COMMA);
        inline static const SPPF DEF_ENUM_NAME2 = maybe(match(Production::DEF_ENUM_NAME2, TokenType::TYPENAME));
        inline static const SPPF DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
        inline static const SPPF DEF_ENUM = boundedGroup(Production::DEF_ENUM,
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

