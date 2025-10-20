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

        DEF_ENUM,
        DEF_ENUM_NAME1,
        DEF_ENUM_NAME2,
        DEF_ENUM_ITEM,
        DEF_ENUM_ITEM_NAME,
        DEF_ENUM_ITEM_LIST,

        DEF_CMD,
        DEF_CMD_NAME,
        DEF_CMD_PARMS,
        DEF_CMD_PARM,
        DEF_CMD_PARMTYPE_NAME,
        DEF_CMD_PARMTYPE_EXPR,
        DEF_CMD_PARMTYPE_EXPR_INTR,
        DEF_CMD_PARM_TYPE,
        DEF_CMD_PARM_NAME,
        DEF_CMD_IMPARMS,
        DEF_CMD_IMPARM,
        DEF_CMD_RETVAL,
        DEF_CMD_BODY,
    };
}

#endif // PRODUCTIONS_H

