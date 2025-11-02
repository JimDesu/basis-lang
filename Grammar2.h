#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

namespace basis {
    struct Grammar2 {
        // n.b. this relies on default initialization of member fields proper to execution
        // of the constructor
        Grammar2();
        void initLiterals();
        void initIdentifiers();
        void initPunctuation();
        void initReservedWords();
        void initEnumerations();

        // Literals
        SPPF DECIMAL;
        SPPF HEXNUMBER;
        SPPF NUMBER;
        SPPF STRING;
        SPPF LITERAL;
        // Identifiers
        SPPF IDENTIFIER;
        SPPF TYPENAME;
        // Reserved words
        SPPF ALIAS;
        SPPF CLASS;
        SPPF COMMAND;
        SPPF DOMAIN;
        SPPF ENUMERATION;
        SPPF INTRINSIC;
        SPPF OBJECT;
        SPPF RECORD;
        // Punctuation
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
        // Enumerations
        SPPF DEF_ENUM_ITEM_LIST;
        SPPF DEF_ENUM_NAME2;
        SPPF DEF_ENUM_NAME1;
        SPPF DEF_ENUM;
        // top-level parse function
        SPPF COMPILATION_UNIT;
    };
    Grammar2& getGrammar();

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

