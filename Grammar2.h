#ifndef GRAMMAR2_H
#define GRAMMAR2_H

#include "Parsing2.h"

// This struture is awkward to work with between the different files, but it's a compromise to both
// facilitate strenuous unit testing and make circular/forward references simple.
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
        void initSubTypes();
        void initRecordTypes();
        void initObjectTypes();
        void initClassTypes();
        void initCompoundTypes();
        void initTypeAliases();
        void initCommandBody();
        void initCommandDefinitions();

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
        // Compound types

        // Alias definition

        // Command definitions
        SPPF DEF_CMD;
        SPPF DEF_CMD_RECEIVERS;
        SPPF DEF_CMD_RECEIVER;
        SPPF DEF_CMD_NAME_SPEC;
        SPPF DEF_CMD_NAME;
        SPPF DEF_CMD_FAILS;
        SPPF DEF_CMD_MAYFAIL;
        SPPF DEF_CMD_PARMS;
        SPPF DEF_CMD_PARM;
        SPPF DEF_CMD_PARMTYPE_NAME;
        SPPF DEF_CMD_PARMTYPE_VAR;
        SPPF DEF_CMD_PARM_TYPE;
        SPPF DEF_CMD_PARM_NAME;
        SPPF DEF_CMD_IMPARMS;
        SPPF DEF_CMD_RETVAL;
        // Command Body
        SPPF DEF_CMD_BODY;
        SPPF DEF_CMD_BODY_ITEM;
        SPPF DEF_CMD_BODY_ITEM_NAME;
        SPPF DEF_CMD_BODY_ITEM_EXPR;
        SPPF DEF_CMD_BODY_ITEM_EXPR_INTR;
        SPPF DEF_CMD_BODY_ITEM_TYPE;
        SPPF DEF_CMD_BODY_ITEMS;
        SPPF DEF_CMD_BODY_ITEM_LIST;


        // top-level parse function
        SPPF COMPILATION_UNIT;
    };
    Grammar2& getGrammar();

}


#endif // GRAMMAR2_H

