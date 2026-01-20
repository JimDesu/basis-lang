#include "Grammar2.h"
using namespace basis;


Grammar2::Grammar2() {
    initLiterals();
    initIdentifiers();
    initPunctuation();
    initReservedWords();
    initTypeExpressions();
    initEnumerations();
    initDomainTypes();
    initRecordTypes();
    initObjectTypes();
    initTypeAliases();
    initCommandDefinitions();
    initClassTypes();
    initInstanceDecls();
    initCommandBody();
}

void Grammar2::initLiterals() {
   DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
   HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
   NUMBER = match(Production::NUMBER, TokenType::NUMBER);
   STRING = match(Production::STRING, TokenType::STRING);
   LITERAL = any(DECIMAL, HEXNUMBER, NUMBER, STRING);
}

void Grammar2::initIdentifiers() {
   IDENTIFIER = match(Production::IDENTIFIER, TokenType::IDENTIFIER);
   TYPENAME = match(Production::TYPENAME, TokenType::TYPENAME);
}

void Grammar2::initPunctuation() {
   AMBANG = match(Production::AMBANG, TokenType::AMBANG);
   AMPERSAND = match(Production::AMPERSAND, TokenType::AMPERSAND);
   AMPHORA = match(Production::AMPHORA, TokenType::AMPHORA);
   APOSTROPHE = match(Production::APOSTROPHE, TokenType::APOSTROPHE);
   ASTERISK = match(Production::ASTERISK, TokenType::ASTERISK);
   BANG = match(Production::BANG, TokenType::BANG);
   BANGLANGLE = match(Production::BANGLANGLE, TokenType::BANGLANGLE);
   CARAT = match(Production::CARAT, TokenType::CARAT);
   COMMA = match(Production::COMMA, TokenType::COMMA);
   COLON = match(Production::COLON, TokenType::COLON);
   COLANGLE = match(Production::COLANGLE, TokenType::COLANGLE);
   DQMARK = match(Production::DQMARK, TokenType::DQMARK);
   DCOLON = match(Production::DCOLON, TokenType::DCOLON);
   EQUALS = match(Production::EQUALS, TokenType::EQUALS);
   LANGLE = match(Production::LANGLE, TokenType::LANGLE);
   LARROW = match(Production::LARROW, TokenType::LARROW);
   LBRACE = match(Production::LBRACE, TokenType::LBRACE);
   LBRACKET = match(Production::LBRACKET, TokenType::LBRACKET);
   LPAREN = match(Production::LPAREN, TokenType::LPAREN);
   MINUS = match(Production::MINUS, TokenType::MINUS);
   PERCENT = match(Production::PERCENT, TokenType::PERCENT);
   PIPE = match(Production::PIPE, TokenType::PIPE);
   PIPECOL = match(Production::PIPECOL, TokenType::PIPECOL);
   PLUS = match(Production::PLUS, TokenType::PLUS);
   POUND = match(Production::POUND, TokenType::POUND);
   QCOLON = match(Production::QCOLON, TokenType::QCOLON);
   QLANGLE = match(Production::QLANGLE, TokenType::QLANGLE);
   QMARK = match(Production::QMARK, TokenType::QMARK);
   QMINUS = match(Production::QMINUS, TokenType::QMINUS);
   RANGLE = match(Production::RANGLE, TokenType::RANGLE);
   RARROW = match(Production::RARROW, TokenType::RARROW);
   RBRACE = match(Production::RBRACE, TokenType::RBRACE);
   RBRACKET = match(Production::RBRACKET, TokenType::RBRACKET);
   RPAREN = match(Production::RPAREN, TokenType::RPAREN);
   SLASH = match(Production::SLASH, TokenType::SLASH);
}

void Grammar2::initReservedWords() {
   ALIAS = match(Production::ALIAS, TokenType::ALIAS);
   CLASS = match(Production::CLASS, TokenType::CLASS);
   COMMAND = match(Production::COMMAND, TokenType::COMMAND);
   DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
   ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
   INSTANCE = match(Production::INSTANCE, TokenType::INSTANCE);
   INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
   OBJECT = match(Production::OBJECT, TokenType::OBJECT);
   RECORD = match(Production::RECORD, TokenType::RECORD);
}

void Grammar2::initTypeExpressions() {
   TYPE_EXPR_PTR = group(Production::TYPE_EXPR_PTR, CARAT  );
   TYPE_EXPR_RANGE = group(Production::TYPE_EXPR_RANGE,
      all(LBRACKET, maybe(any(IDENTIFIER, NUMBER)), RBRACKET ) );
   TYPE_EXPR_RANGE_FIXED = group(Production::TYPE_EXPR_RANGE,
       all(LBRACKET,any(IDENTIFIER, NUMBER), RBRACKET) );

   TYPE_ARG_TYPE = group(Production::TYPE_ARG_TYPE, forward( TYPE_NAME_Q ));
   TYPE_ARG_VALUE = group(Production::TYPE_ARG_VALUE,
       any(NUMBER, IDENTIFIER) );
   // n.b. ordering is important because of shared prefix
   TYPE_NAME_ARGS = group(Production::TYPE_NAME_ARGS,
       all(LBRACKET, separated(any(TYPE_ARG_VALUE, TYPE_ARG_TYPE), COMMA), RBRACKET) );
   TYPE_NAME_Q = group(Production::TYPE_NAME_Q,
       all(TYPENAME, maybe(TYPE_NAME_ARGS)) );

   TYPE_CMDEXPR_ARG = group(Production::TYPE_CMDEXPR_ARG, all(
      any(
          TYPE_NAME_Q,
          forward(TYPE_EXPR_CMD),
          all(TYPE_EXPR_PTR, forward(TYPE_CMDEXPR_ARG)),
          all(TYPE_EXPR_RANGE, maybe(forward(TYPE_CMDEXPR_ARG))) ), maybe(APOSTROPHE) ));

   TYPE_EXPR_CMD = group(Production::TYPE_EXPR_CMD, all(
         any(COLANGLE, QLANGLE, BANGLANGLE),
         maybe(separated(TYPE_CMDEXPR_ARG, COMMA)),
         RANGLE) );

   TYPEDEF_PARM_TYPE = group(Production::TYPEDEF_PARM_TYPE,
       all(TYPE_NAME_Q, maybe(TYPENAME)) );
   TYPEDEF_PARM_VALUE = group(Production::TYPEDEF_PARM_VALUE,
       all(TYPENAME, IDENTIFIER) );
   // n.b. ordering is important because of shared prefix
   TYPEDEF_PARMS = group(Production::TYPEDEF_PARMS,
       all(LBRACKET, separated(any(TYPEDEF_PARM_VALUE, TYPEDEF_PARM_TYPE), COMMA), RBRACKET) );
   TYPEDEF_NAME_Q = group(Production::TYPEDEF_NAME_Q,
       all(TYPENAME, maybe(TYPEDEF_PARMS)) );

   TYPE_EXPR = group(Production::TYPE_EXPR,
         any(
            TYPEDEF_NAME_Q,
            TYPE_EXPR_CMD,
            all(TYPE_EXPR_PTR, forward(TYPE_EXPR)),
            all(TYPE_EXPR_RANGE, maybe(forward(TYPE_EXPR))) ));

    TYPE_EXPR_DOMAIN = group(Production::TYPE_EXPR_DOMAIN,
        any( TYPE_NAME_Q, all(TYPE_EXPR_RANGE_FIXED, maybe(forward(TYPE_EXPR_DOMAIN))) ));
}

void Grammar2::initEnumerations() {
   DEF_ENUM_ITEM_LIST = separated(
       all(match(Production::DEF_ENUM_ITEM_NAME, TokenType::IDENTIFIER), EQUALS, LITERAL),
       COMMA);
   DEF_ENUM_NAME2 = maybe(match(Production::DEF_ENUM_NAME2, TokenType::TYPENAME));
   DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME1, TokenType::TYPENAME);
   DEF_ENUM = boundedGroup(Production::DEF_ENUM,
       ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2, COLON, DEF_ENUM_ITEM_LIST);
}

void Grammar2::initDomainTypes() {
    DEF_DOMAIN = boundedGroup(Production::DEF_DOMAIN,
        all(DOMAIN, group(Production::DEF_DOMAIN_NAME, TYPENAME), COLON,
            group(Production::DEF_DOMAIN_PARENT,
               any(TYPENAME, all(LBRACKET, maybe(NUMBER), RBRACKET)) )));
}
void Grammar2::initRecordTypes() {
    DEF_RECORD_FIELD = group(Production::DEF_RECORD_FIELD,
        all(group(Production::DEF_RECORD_FIELD_DOMAIN, TYPE_EXPR_DOMAIN),
            group(Production::DEF_RECORD_FIELD_NAME, IDENTIFIER)) );
     DEF_RECORD_FIELDS = group(Production::DEF_RECORD_FIELDS,
        separated(DEF_RECORD_FIELD, COMMA) );
     DEF_RECORD = boundedGroup(Production::DEF_RECORD,
        all(RECORD, group(Production::DEF_RECORD_NAME, TYPEDEF_NAME_Q), COLON, DEF_RECORD_FIELDS) );
}

void Grammar2::initObjectTypes() {
    DEF_OBJECT_FIELD = group(Production::DEF_OBJECT_FIELD,
        all(group(Production::DEF_OBJECT_FIELD_TYPE, TYPE_EXPR),
            group(Production::DEF_OBJECT_FIELD_NAME, IDENTIFIER)) );
    DEF_OBJECT_FIELDS = group(Production::DEF_OBJECT_FIELDS,
       separated(DEF_OBJECT_FIELD, COMMA) );
    DEF_OBJECT = boundedGroup(Production::DEF_OBJECT,
        all(OBJECT, group(Production::DEF_OBJECT_NAME, TYPEDEF_NAME_Q), COLON, DEF_OBJECT_FIELDS) );
}

void Grammar2::initInstanceDecls() {
    DEF_INSTANCE_DELEGATE = group(Production::DEF_INSTANCE_DELEGATE,
        all(LPAREN, IDENTIFIER, RPAREN) );
    DEF_INSTANCE_TYPES = group(Production::DEF_INSTANCE_TYPES,
        separated(all(TYPENAME, maybe(DEF_INSTANCE_DELEGATE)), COMMA) );
    DEF_INSTANCE_NAME = group(Production::DEF_INSTANCE_NAME, TYPENAME);
    DEF_INSTANCE = boundedGroup(Production::DEF_INSTANCE,
        all(INSTANCE, DEF_INSTANCE_NAME, COLON, DEF_INSTANCE_TYPES) );
}

void Grammar2::initTypeAliases() {
   DEF_ALIAS = boundedGroup(Production::DEF_ALIAS, all(ALIAS, TYPEDEF_NAME_Q, COLON, TYPE_EXPR));
}

void Grammar2::initCommandDefinitions() {
   DEF_CMD_PARMTYPE_NAME = group(Production::DEF_CMD_PARMTYPE_NAME, TYPE_EXPR);
   DEF_CMD_PARMTYPE_VAR = group(Production::DEF_CMD_PARMTYPE_VAR,
      all(LPAREN, TYPENAME, COLON, DEF_CMD_PARMTYPE_NAME, RPAREN) );
   DEF_CMD_PARM_NAME = match(Production::DEF_CMD_PARM_NAME, TokenType::IDENTIFIER);

   DEF_CMD_PARM_TYPE = group(Production::DEF_CMD_PARM_TYPE,
      any(DEF_CMD_PARMTYPE_NAME, DEF_CMD_PARMTYPE_VAR) );
   DEF_CMD_PARM = group(Production::DEF_CMD_PARM,
      all(DEF_CMD_PARM_TYPE, DEF_CMD_PARM_NAME) );
   DEF_CMD_RECEIVER = group(Production::DEF_CMD_RECEIVER,
      all(DEF_CMD_PARMTYPE_NAME, DEF_CMD_PARM_NAME) );

   DEF_CMD_RECEIVERS = group(Production::DEF_CMD_RECEIVERS,
       all(separated(DEF_CMD_RECEIVER, COMMA), DCOLON) );
   DEF_CMD_IMPARMS = prefix(SLASH, group(Production::DEF_CMD_IMPARMS,
      separated(DEF_CMD_PARM, COMMA)) );
   DEF_CMD_RETVAL = prefix(RARROW, group(Production::DEF_CMD_RETVAL,
      IDENTIFIER) );
   DEF_CMD_PARMS = prefix(COLON, group(Production::DEF_CMD_PARMS,
      all(separated(DEF_CMD_PARM, COMMA),DEF_CMD_RETVAL) ));

   DEF_CMD_NAME = match(Production::DEF_CMD_NAME, TokenType::IDENTIFIER);
   DEF_CMD_MAYFAIL = match(Production::DEF_CMD_MAYFAIL, TokenType::QMARK);
   DEF_CMD_FAILS = match(Production::DEF_CMD_FAILS, TokenType::BANG);
   DEF_CMD_NAME_SPEC = group(Production::DEF_CMD_NAME_SPEC,
       all(maybe(any(DEF_CMD_MAYFAIL, DEF_CMD_FAILS)), DEF_CMD_NAME) );

   // Top-level command definition
    //TODO: add unit tests for this
   DEF_CMD_DECL = group(Production::DEF_CMD_DECL,
       all(COMMAND, any(
           // destructor
           all(group(Production::ON_EXIT, AMPHORA),DEF_CMD_RECEIVER, COLON,
               separated(DEF_CMD_PARM, COMMA)),
           // failure handler
           all(group(Production::ON_EXIT_FAIL, AMBANG), DEF_CMD_RECEIVER, COLON,
               separated(DEF_CMD_PARM, COMMA)),
           // constructor
           all(DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA)),
           // command / method
           all(maybe(DEF_CMD_RECEIVERS), DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) )));
   DEF_CMD_INTRINSIC = boundedGroup(Production::DEF_CMD_INTRINSIC, any(
       all(INTRINSIC, DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) ));

   DEF_CMD = boundedGroup(Production::DEF_CMD,
       all(
           COMMAND,
           any(
               // constructor
               all(DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA) ),
               // destructor
               all(group(Production::ON_EXIT, AMPHORA), DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA)),
               // failure handler
               all(group(Production::ON_EXIT_FAIL, AMBANG), DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA) ),
               // command / method
               all(maybe(DEF_CMD_RECEIVERS), DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) ),
            forward(DEF_CMD_BODY) ));

}

void Grammar2::initClassTypes() {
    DEF_CLASS = boundedGroup(Production::DEF_CLASS,
        all(CLASS, group(Production::DEF_CLASS_NAME, TYPENAME), COLON,
            group(Production::DEF_CLASS_CMDS, oneOrMore( DEF_CMD_DECL )) ));
}

void Grammar2::initCommandBody() {
    // TODO: lbrace vs lparen exprs, operators, and other expression stuff
    CALL_IDENTIFIER = any(
            group(Production::ALLOC_IDENTIFIER, all(POUND, IDENTIFIER)),
            IDENTIFIER );
    CALL_PARAMETER = group(Production::CALL_PARAMETER, any( CALL_IDENTIFIER, forward(CALL_EXPRESSION)) );

    SUBCALL_CONSTRUCTOR = group(Production::CALL_CONSTRUCTOR,
        all(TYPE_NAME_Q, COLON, separated(CALL_PARAMETER, COMMA)) );
    SUBCALL_COMMAND = group(Production::CALL_COMMAND,
        all(IDENTIFIER, maybe(all(COLON, separated(CALL_PARAMETER, COMMA)))) );
    SUBCALL_VCOMMAND = group(Production::CALL_VCOMMAND,
        all(separated(CALL_IDENTIFIER, COMMA), DCOLON, IDENTIFIER,
            maybe(all(COLON, separated(CALL_PARAMETER, COMMA))) ));
    SUB_CALL = any(SUBCALL_VCOMMAND, SUBCALL_CONSTRUCTOR, SUBCALL_COMMAND);

    CALL_EXPRESSION = group(Production::CALL_EXPRESSION, all(LPAREN, SUB_CALL, RPAREN) );
    CALL_ASSIGNMENT = boundedGroup(Production::CALL_ASSIGNMENT,
        all(CALL_IDENTIFIER, LARROW, any(CALL_EXPRESSION,SUB_CALL)) );

    CALL_CONSTRUCTOR = boundedGroup(Production::CALL_CONSTRUCTOR,
        all(TYPE_NAME_Q, COLON, separated(CALL_PARAMETER, COMMA)) );
    CALL_COMMAND = boundedGroup(Production::CALL_COMMAND,
        all(IDENTIFIER, maybe(all(COLON, separated(CALL_PARAMETER, COMMA)))) );
    CALL_VCOMMAND = boundedGroup(Production::CALL_VCOMMAND,
        all(separated(IDENTIFIER, COMMA), DCOLON, IDENTIFIER,
            maybe(all(COLON, separated(CALL_PARAMETER, COMMA))) ));

    CALL_INVOKE = any(CALL_VCOMMAND, CALL_CONSTRUCTOR, CALL_COMMAND);
    CALL_GROUP = group(Production::CALL_GROUP,
        oneOrMore(any(CALL_INVOKE, forward(BLOCK))) );
    // TODO extend DO_RECOVER_SPEC with fail type
    BLOCK_HEADER = group(Production::BLOCK_HEADER,
        any(group(Production::DO_WHEN_MULTI, DQMARK),
            group(Production::DO_WHEN, QMARK),
            group(Production::DO_WHEN_FAIL, QMINUS),
            group(Production::DO_ELSE, MINUS),
            group(Production::DO_UNLESS, BANG),
            group(Production::DO_BLOCK, PERCENT),
            group(Production::DO_REWIND, CARAT),
            group(Production::DO_RECOVER, PIPE),
            group(Production::DO_RECOVER_SPEC, PIPECOL),
            group(Production::ON_EXIT, AMPHORA),
            group(Production::ON_EXIT_FAIL, AMBANG) ));
    BLOCK = boundedGroup(Production::DO_BLOCK, all(BLOCK_HEADER, CALL_GROUP) );
    DEF_CMD_BODY = group(Production::DEF_CMD_BODY,all(
        discard(TokenType::EQUALS), CALL_GROUP) );
}

Grammar2& basis::getGrammar() {
   static Grammar2 grammar{};
   return grammar;
}

