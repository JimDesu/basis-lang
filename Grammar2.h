#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

namespace basis {
    namespace grammar {
        /*
         * This approach is ugly as hell, and I need to change it.  Originally these were
         * 'inline const', but that led to a cross-file initialization order problem.  I set
         * it up this way in an attempt to force the correct order, but that's still not
         * working right.
         *
         * The "easy" alternative is to stop exposing the subordinate parse function objects,
         * but that would make comprehensive tests grossly impractical.
         *
         * I don't have a better solution yet.  This is fugly AND it doesn't work any
         * better than the simpler way.
         */
        // Literals
        extern SPPF DECIMAL;
        extern SPPF HEXNUMBER;
        extern SPPF NUMBER;
        extern SPPF STRING;

        // Identifiers
        extern SPPF IDENTIFIER;
        extern SPPF TYPENAME;

        // Reserved words
        extern SPPF ALIAS;
        extern SPPF CLASS;
        extern SPPF COMMAND;
        extern SPPF DOMAIN;
        extern SPPF ENUMERATION;
        extern SPPF INTRINSIC;
        extern SPPF OBJECT;
        extern SPPF RECORD;

        // Punctuation
        extern SPPF AMBANG;
        extern SPPF AMPERSAND;
        extern SPPF AMPHORA;
        extern SPPF ASTERISK;
        extern SPPF BANG;
        extern SPPF BANGLANGLE;
        extern SPPF CARAT;
        extern SPPF CARATQ;
        extern SPPF COMMA;
        extern SPPF COLON;
        extern SPPF COLANGLE;
        extern SPPF DCOLON;
        extern SPPF EQUALS;
        extern SPPF LANGLE;
        extern SPPF LARROW;
        extern SPPF LBRACE;
        extern SPPF LBRACKET;
        extern SPPF LPAREN;
        extern SPPF MINUS;
        extern SPPF PERCENT;
        extern SPPF PIPE;
        extern SPPF PIPECOL;
        extern SPPF PLUS;
        extern SPPF POUND;
        extern SPPF QCOLON;
        extern SPPF QLANGLE;
        extern SPPF QMARK;
        extern SPPF QMINUS;
        extern SPPF RANGLE;
        extern SPPF RARROW;
        extern SPPF RBRACE;
        extern SPPF RBRACKET;
        extern SPPF RPAREN;
        extern SPPF SLASH;

        // Composite grammar rules
        extern SPPF LITERAL;

        // Enumerations
        extern SPPF DEF_ENUM_ITEM_LIST;
        extern SPPF DEF_ENUM_NAME2;
        extern SPPF DEF_ENUM_NAME1;
        extern SPPF DEF_ENUM;

        // temporary definition; we'll build up to this
        extern SPPF COMPILATION_UNIT;

        SPPF initGrammar();
    }

    SPPF getGrammar();

    /*
     TODO: fix up this stuff from the template version
    using DEF_CMD_BODY; //TODO
    using DEF_CMD_PARMTYPE_DECO;
    using DEF_CMD_PARMTYPE_NAME =
              All<DEF_CMD_PARMTYPE_DECO, Match<Production::DEF_CMD_PARMTYPE_NAME, TokenType::TYPENAME>>;

    using DEF_CMD_PARMTYPE_EXPR_INTR;
    using DEF_CMF_PARMTYPE_EXPR;

    using DEF_CMD_PARM_TYPE;
    using DEF_CMD_PARM_NAME;
    using DEF_CMD_PARM;
    using DEF_CMD_PARMS;
    using DEF_CMD_IMPARMS;
    using DEF_CMD_RETVAL;
    using DEF_CMD_NAME;
    using DEF_CMD = BoundedGroup<Production::DEF_CMD,
              COMMAND, DEF_CMD_NAME, DEF_CMD_PARMS, DEF_CMD_IMPARMS, DEF_CMD_RETVAL, DEF_CMD_BODY>;
     */

}


#endif // GRAMMAR2_H

