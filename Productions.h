#ifndef PRODUCTIONS_H
#define PRODUCTIONS_H

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
        TYPENAME,
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
        POUND,
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
        // -- enumerations
        DEF_ENUM,
        DEF_ENUM_NAME1,
        DEF_ENUM_NAME2,
        DEF_ENUM_ITEM,
        DEF_ENUM_ITEM_NAME,
        DEF_ENUM_ITEM_LIST,
        // -- type expressions
        TYPE_NAME_Q,
        TYPE_NAME_PARMS,
        TYPE_TYPEPARM_TYPE,
        TYPE_TYPEPARM_VALUE,
        TYPE_EXPR_PTR,
        TYPE_EXPR_RANGE,
        TYPE_EXPR,
        TYPE_EXPR_CMD,

        // -- type aliases
        DEF_ALIAS,
        // -- command definitions/declarations
        DEF_CMD,
        DEF_CMD_RECEIVERS,
        DEF_CMD_RECEIVER,
        DEF_CMD_NAME_SPEC,
        DEF_CMD_NAME,
        DEF_CMD_FAILS,
        DEF_CMD_MAYFAIL,
        DEF_CMD_PARMS,
        DEF_CMD_PARM,
        DEF_CMD_PARMTYPE_NAME,
        DEF_CMD_PARMTYPE_VAR,
        DEF_CMD_PARM_TYPE,
        DEF_CMD_PARM_NAME,
        DEF_CMD_IMPARMS,
        DEF_CMD_RETVAL,
        // -- command body
        DEF_CMD_BODY,
    };
}

#endif // PRODUCTIONS_H

