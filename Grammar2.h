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
        void initDomainTypes();
        void initRecordTypes();
        void initObjectTypes();
        void initClassTypes();
        void initInstanceDecls();
        void initTypeExpressions();
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
        SPPF DECLARE;
        SPPF DOMAIN;
        SPPF ENUMERATION;
        SPPF INTRINSIC;
        SPPF OBJECT;
        SPPF RECORD;
        SPPF INSTANCE;

        // Punctuation
        SPPF AMBANG;
        SPPF AMPERSAND;
        SPPF AMPHORA;
        SPPF APOSTROPHE;
        SPPF ASTERISK;
        SPPF BANG;
        SPPF BANGLANGLE;
        SPPF CARAT;
        SPPF COMMA;
        SPPF COLON;
        SPPF COLANGLE;
        SPPF DCOLON;
        SPPF DOLLAR;
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
        SPPF DQMARK;
        SPPF RANGLE;
        SPPF RARROW;
        SPPF RBRACE;
        SPPF RBRACKET;
        SPPF RPAREN;
        SPPF SLASH;
        SPPF UNDERSCORE;

        // Enumerations
        SPPF DEF_ENUM_ITEM_LIST;
        SPPF DEF_ENUM_NAME2;
        SPPF DEF_ENUM_NAME1;
        SPPF DEF_ENUM;

        // Instance Declarations
        SPPF DEF_INSTANCE;
        SPPF DEF_INSTANCE_NAME;
        SPPF DEF_INSTANCE_DELEGATE;
        SPPF DEF_INSTANCE_TYPES;

        // Type Expressions
        SPPF TYPE_EXPR;
        SPPF TYPE_EXPR_PTR;
        SPPF TYPE_EXPR_RANGE;
        SPPF TYPE_EXPR_RANGE_FIXED;
        SPPF TYPE_EXPR_CMD;
        SPPF TYPE_CMDEXPR_ARG;
        SPPF TYPEDEF_NAME_Q;
        SPPF TYPEDEF_PARMS;
        SPPF TYPEDEF_PARM_TYPE;
        SPPF TYPEDEF_PARM_VALUE;
        SPPF TYPE_NAME_Q;
        SPPF TYPE_NAME_ARGS;
        SPPF TYPE_ARG_TYPE;
        SPPF TYPE_ARG_VALUE;
        SPPF TYPE_EXPR_DOMAIN;

        // Alias definition
        SPPF DEF_ALIAS;

        // Domain definition
        SPPF DEF_DOMAIN;

        // Record definition
        SPPF DEF_RECORD;
        SPPF DEF_RECORD_NAME;
        SPPF DEF_RECORD_FIELDS;
        SPPF DEF_RECORD_FIELD;
        SPPF DEF_RECORD_FIELD_NAME;
        SPPF DEF_RECORD_FIELD_DOMAIN;

        // Object definition
        SPPF DEF_OBJECT;
        SPPF DEF_OBJECT_NAME;
        SPPF DEF_OBJECT_FIELDS;
        SPPF DEF_OBJECT_FIELD;
        SPPF DEF_OBJECT_FIELD_NAME;
        SPPF DEF_OBJECT_FIELD_TYPE;

        // Class definition
        SPPF DEF_CLASS;
        SPPF DEF_CLASS_NAME;
        SPPF DEF_CLASS_CMDS;

        // Command definitions
        SPPF DEF_CMD;
        SPPF DEF_CMD_DECL;
        SPPF DEF_CMD_INTRINSIC;
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
        SPPF DEF_CMD_EMPTY;
        SPPF CALL_GROUP;
        SPPF CALL_INVOKE;
        SPPF CALL_CONSTRUCTOR;
        SPPF CALL_COMMAND;
        SPPF CALL_VCOMMAND;
        SPPF CALL_ASSIGNMENT;
        SPPF CALL_EXPRESSION;
        SPPF CALL_EXPR_TERM;
        SPPF CALL_EXPR_SUFFIX;
        SPPF CALL_EXPR_INDEX;
        SPPF CALL_EXPR_ADDR;
        SPPF CALL_EXPR_DEREF;
        SPPF CALL_OPERATOR;
        SPPF CALL_QUOTE;
        SPPF CALL_CMD_TARGET;
        SPPF CALL_IDENTIFIER;
        SPPF CALL_PARAMETER;
        SPPF CALL_PARM_EXPR;
        SPPF CALL_PARM_EMPTY;
        SPPF SUB_CALL;
        SPPF SUBCALL_CONSTRUCTOR;
        SPPF SUBCALL_COMMAND;
        SPPF SUBCALL_VCOMMAND;
        SPPF RECOVER_SPEC;
        SPPF BLOCK_HEADER;
        SPPF BLOCK;

        // Expressions

        // top-level parse function
        SPPF COMPILATION_UNIT;
    };
    Grammar2& getGrammar();

}


#endif // GRAMMAR2_H

