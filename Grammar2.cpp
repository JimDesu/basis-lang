#include "Grammar2.h"
using namespace basis;


Grammar2::Grammar2() {
    initLiterals();
    initPunctuation();
    initIdentifiers();
    initReservedWords();
    initTypeExpressions();
    initEnumerations();
    initModuleTypes();
    initImports();
    initDomainTypes();
    initRecordTypes();
    initObjectTypes();
    initTypeAliases();
    initCommandDefinitions();
    initClassTypes();
    initInstanceDecls();
    initCommandBody();
    initProgramDefinitions();
    initTestDefinitions();
    initCompilationUnit();
}

void Grammar2::initLiterals() {
   DECIMAL = match(Production::DECIMAL, TokenType::DECIMAL);
   HEXNUMBER = match(Production::HEXNUMBER, TokenType::HEXNUMBER);
   BINARY = match(Production::BINARY, TokenType::BINARY);
   NUMBER = match(Production::NUMBER, TokenType::NUMBER);
   STRING = match(Production::STRING, TokenType::STRING);
   LITERAL = any(DECIMAL, HEXNUMBER, BINARY, NUMBER, STRING);
}

void Grammar2::initIdentifiers() {
   QUALIFIED_TYPENAME = group(Production::QUALIFIED_TYPENAME,separated(
       match(Production::TYPENAME, TokenType::TYPENAME), DCOLON, false ));
   TYPENAME_UNQUALIFIED = match(Production::TYPENAME, TokenType::TYPENAME);
   TYPENAME = any(QUALIFIED_TYPENAME, TYPENAME_UNQUALIFIED);

   IDENTIFIER = group(Production::IDENTIFIER, all(
       maybe(oneOrMore(all(TYPENAME_UNQUALIFIED, DCOLON))),
       match(Production::IDENTIFIER, TokenType::IDENTIFIER)));
}

void Grammar2::initPunctuation() {
   ON_EXIT_FAIL = match(Production::ON_EXIT_FAIL, TokenType::AMBANG);
   AMPERSAND = match(Production::AMPERSAND, TokenType::AMPERSAND);
   ON_EXIT = match(Production::ON_EXIT, TokenType::AMPHORA);
   WRITEABLE = match(Production::APOSTROPHE, TokenType::APOSTROPHE);
   ASTERISK = match(Production::ASTERISK, TokenType::ASTERISK);
   DO_UNLESS = match(Production::BANG, TokenType::BANG);
   BANGBRACE = match(Production::BANGBRACE, TokenType::BANGBRACE);
   BANGLANGLE = match(Production::BANGLANGLE, TokenType::BANGLANGLE);
   CARAT = match(Production::CARAT, TokenType::CARAT);
   COMMA = match(Production::COMMA, TokenType::COMMA);
   COLON = match(Production::COLON, TokenType::COLON);
   COLANGLE = match(Production::COLANGLE, TokenType::COLANGLE);
   COLBRACE = match(Production::COLBRACE, TokenType::COLBRACE);
   DO_WHEN_MULTI = match(Production::DQMARK, TokenType::DQMARK);
   DCOLON = match(Production::DCOLON, TokenType::DCOLON);
   EXEC_CMD = match(Production::DOLLAR, TokenType::DOLLAR);
   EQUALS = match(Production::EQUALS, TokenType::EQUALS);
   EXTRACT = match(Production::EXTRACT, TokenType::DRANGLE);
   GREQUALS = match(Production::GREQUALS, TokenType::GREQUALS);
   INSERT = match(Production::INSERT, TokenType::DLANGLE);
   LANGLE = match(Production::LANGLE, TokenType::LANGLE);
   LEQUALS = match(Production::LEQUALS, TokenType::LEQUALS);
   LARROW = match(Production::LARROW, TokenType::LARROW);
   LBRACE = match(Production::LBRACE, TokenType::LBRACE);
   LBRACKET = match(Production::LBRACKET, TokenType::LBRACKET);
   LPAREN = match(Production::LPAREN, TokenType::LPAREN);
   MINUS = match(Production::MINUS, TokenType::MINUS);
   DO_BLOCK = match(Production::PERCENT, TokenType::PERCENT);
   PIPE = match(Production::PIPE, TokenType::PIPE);
   PLUS = match(Production::PLUS, TokenType::PLUS);
   POUND = match(Production::POUND, TokenType::POUND);
   QBRACE = match(Production::QBRACE, TokenType::QBRACE);
   QCOLON = match(Production::QCOLON, TokenType::QCOLON);
   QLANGLE = match(Production::QLANGLE, TokenType::QLANGLE);
   DO_WHEN = match(Production::QMARK, TokenType::QMARK);
   DO_WHEN_FAIL = match(Production::QMINUS, TokenType::QMINUS);
   RANGLE = match(Production::RANGLE, TokenType::RANGLE);
   RARROW = match(Production::RARROW, TokenType::RARROW);
   RBRACE = match(Production::RBRACE, TokenType::RBRACE);
   RBRACKET = match(Production::RBRACKET, TokenType::RBRACKET);
   RPAREN = match(Production::RPAREN, TokenType::RPAREN);
   SLASH = match(Production::SLASH, TokenType::SLASH);
   UNDERSCORE = match(Production::UNDERSCORE, TokenType::UNDERSCORE);
}

void Grammar2::initReservedWords() {
   ALIAS = match(Production::ALIAS, TokenType::ALIAS);
   CLASS = match(Production::CLASS, TokenType::CLASS);
   COMMAND = match(Production::COMMAND, TokenType::COMMAND);
   DECLARE = match(Production::DECLARE, TokenType::DECLARE);
   DOMAIN = match(Production::DOMAIN, TokenType::DOMAIN);
   ENUMERATION = match(Production::ENUMERATION, TokenType::ENUMERATION);
   IMPORT = match(Production::IMPORT, TokenType::IMPORT);
   INSTANCE = match(Production::INSTANCE, TokenType::INSTANCE);
   INTRINSIC = match(Production::INTRINSIC, TokenType::INTRINSIC);
   MODULE = match(Production::MODULE, TokenType::MODULE);
   OBJECT = match(Production::OBJECT, TokenType::OBJECT);
   PROGRAM = match(Production::PROGRAM, TokenType::PROGRAM);
   RECORD = match(Production::RECORD, TokenType::RECORD);
   TEST = match(Production::TEST, TokenType::TEST);
}

void Grammar2::initTypeExpressions() {
   TYPE_EXPR_PTR = group(Production::TYPE_EXPR_PTR, oneOrMore(CARAT)  );
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
          all(TYPE_EXPR_RANGE, maybe(forward(TYPE_CMDEXPR_ARG))) ), maybe(WRITEABLE) ));

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
   DEF_ENUM = exclusiveGroup(Production::DEF_ENUM,
       ENUMERATION, DEF_ENUM_NAME1, DEF_ENUM_NAME2, COLON, DEF_ENUM_ITEM_LIST);
}

void Grammar2::initModuleTypes() {
    DEF_MODULE_NAME = group(Production::DEF_MODULE_NAME, TYPENAME);
    DEF_MODULE = exclusiveGroup(Production::DEF_MODULE, all(MODULE, DEF_MODULE_NAME));
}

void Grammar2::initProgramDefinitions() {
    DEF_PROGRAM = exclusiveGroup(Production::DEF_PROGRAM,
        all(PROGRAM, EQUALS, forward(CALL_INVOKE)));
}

void Grammar2::initTestDefinitions() {
    DEF_TEST = exclusiveGroup(Production::DEF_TEST,
        all(TEST, STRING, EQUALS, CALL_GROUP));
}

void Grammar2::initImports() {
    DEF_IMPORT_FILE = group(Production::DEF_IMPORT_FILE, STRING);

    DEF_IMPORT_STANDARD = group(Production::DEF_IMPORT_STANDARD,
        all(maybe(all(TYPENAME_UNQUALIFIED, COLON)), TYPENAME));

    DEF_IMPORT = exclusiveGroup(Production::DEF_IMPORT,
        all(IMPORT, any(DEF_IMPORT_FILE, DEF_IMPORT_STANDARD)));
}

void Grammar2::initDomainTypes() {
    DEF_DOMAIN = exclusiveGroup(Production::DEF_DOMAIN,
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
     DEF_RECORD = exclusiveGroup(Production::DEF_RECORD,
        all(RECORD, group(Production::DEF_RECORD_NAME, TYPEDEF_NAME_Q), COLON, DEF_RECORD_FIELDS) );
}

void Grammar2::initObjectTypes() {
    DEF_OBJECT_FIELD = group(Production::DEF_OBJECT_FIELD,
        all(group(Production::DEF_OBJECT_FIELD_TYPE, TYPE_EXPR),
            group(Production::DEF_OBJECT_FIELD_NAME, IDENTIFIER)) );
    DEF_OBJECT_FIELDS = group(Production::DEF_OBJECT_FIELDS,
       separated(DEF_OBJECT_FIELD, COMMA) );
    DEF_OBJECT = exclusiveGroup(Production::DEF_OBJECT,
        all(OBJECT, group(Production::DEF_OBJECT_NAME, TYPEDEF_NAME_Q), COLON, DEF_OBJECT_FIELDS) );
}

void Grammar2::initInstanceDecls() {
    DEF_INSTANCE_DELEGATE = group(Production::DEF_INSTANCE_DELEGATE,
        all(LPAREN, IDENTIFIER, RPAREN) );
    DEF_INSTANCE_TYPES = group(Production::DEF_INSTANCE_TYPES,
        separated(all(TYPENAME, maybe(DEF_INSTANCE_DELEGATE)), COMMA) );
    DEF_INSTANCE_NAME = group(Production::DEF_INSTANCE_NAME, TYPEDEF_NAME_Q);
    DEF_INSTANCE = exclusiveGroup(Production::DEF_INSTANCE,
        all(INSTANCE, DEF_INSTANCE_NAME, COLON, DEF_INSTANCE_TYPES) );
}

void Grammar2::initTypeAliases() {
   DEF_ALIAS = exclusiveGroup(Production::DEF_ALIAS, all(ALIAS, TYPEDEF_NAME_Q, COLON, TYPE_EXPR));
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
       all(LPAREN, separated(DEF_CMD_RECEIVER, COMMA), RPAREN, DCOLON) );
   DEF_CMD_IMPARMS = prefix(SLASH, group(Production::DEF_CMD_IMPARMS,
      separated(DEF_CMD_PARM, COMMA)) );
   DEF_CMD_RETVAL = prefix(RARROW, group(Production::DEF_CMD_RETVAL,
      IDENTIFIER) );
   DEF_CMD_PARM_LIST = separated(DEF_CMD_PARM, COMMA);
   // For regular commands - requires parameters if return value is present
   DEF_CMD_PARMS = prefix(COLON, group(Production::DEF_CMD_PARMS,
      all(DEF_CMD_PARM_LIST, maybe(DEF_CMD_RETVAL)) ));
   // For VCOMMAND - allows return value without parameters (receivers act as implicit parameters)
   DEF_CMD_VPARMS = all(
      maybe(prefix(COLON, group(Production::DEF_CMD_PARMS,
         DEF_CMD_PARM_LIST))),
      maybe(DEF_CMD_RETVAL)
   );

   DEF_CMD_NAME = match(Production::DEF_CMD_NAME, TokenType::IDENTIFIER);
   DEF_CMD_MAYFAIL = match(Production::DEF_CMD_MAYFAIL, TokenType::QMARK);
   DEF_CMD_FAILS = match(Production::DEF_CMD_FAILS, TokenType::BANG);
   DEF_CMD_NAME_SPEC = group(Production::DEF_CMD_NAME_SPEC,
       all(maybe(any(DEF_CMD_MAYFAIL, DEF_CMD_FAILS)), DEF_CMD_NAME) );

   DEF_CMD_DECL = exclusiveGroup(Production::DEF_CMD_DECL,
       all(DECLARE, any(
           // destructor
           all(ON_EXIT,DEF_CMD_RECEIVER),
           // failure handler
           all(ON_EXIT_FAIL, DEF_CMD_RECEIVER),
           // constructor
           all(DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA)),
           // VCOMMAND - with receivers, allows -> result without params
           all(DEF_CMD_RECEIVERS, DEF_CMD_NAME_SPEC, DEF_CMD_VPARMS, DEF_CMD_IMPARMS),
           // Regular command - without receivers, requires params for -> result
           all(DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) )));
   DEF_CMD_INTRINSIC = exclusiveGroup(Production::DEF_CMD_INTRINSIC,
       all(INTRINSIC, DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) );

   DEF_CMD = exclusiveGroup(Production::DEF_CMD,
       all(
           COMMAND,
           any(
               // constructor
               all(DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA) ),
               // destructor -- no parameters allowed
               all(ON_EXIT, DEF_CMD_RECEIVER ),
               // failure handler -- no parms allowed
               all(ON_EXIT_FAIL, DEF_CMD_RECEIVER),
               // VCOMMAND - with receivers, allows -> result without params
               all(DEF_CMD_RECEIVERS, DEF_CMD_NAME_SPEC, DEF_CMD_VPARMS, DEF_CMD_IMPARMS),
               // Regular command - without receivers, requires params for -> result
               all(DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS) ),
            forward(DEF_CMD_BODY) ));

}

void Grammar2::initClassTypes() {
    DEF_CLASS = exclusiveGroup(Production::DEF_CLASS,
        all(CLASS, group(Production::DEF_CLASS_NAME, TYPENAME), COLON,
            group(Production::DEF_CLASS_CMDS, oneOrMore( any(DEF_CMD_DECL,DEF_CMD))) ));
}

void Grammar2::initCommandBody() {
    CALL_IDENTIFIER = any( group(Production::ALLOC_IDENTIFIER, all(POUND, IDENTIFIER)), IDENTIFIER );

    CALL_PARM_EXPR = group(Production::CALL_PARM_EXPR,
        any( forward(SUBCALL_EXPRESSION), CALL_IDENTIFIER ) );
    CALL_PARM_EMPTY = group(Production::CALL_PARM_EMPTY, UNDERSCORE);
    CALL_PARAMETER = group(Production::CALL_PARAMETER,
        any( CALL_PARM_EMPTY, CALL_PARM_EXPR) );

    CALL_OPERATOR = group(Production::CALL_OPERATOR,
        any(PLUS, MINUS, ASTERISK, SLASH, LANGLE, RANGLE, LEQUALS, GREQUALS, EQUALS, INSERT, EXTRACT) );

    CALL_BLOCKQUOTE = any(
        boundedGroup(Production::CALL_BLOCK_NOFAIL,
            all(COLBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)),
        boundedGroup(Production::CALL_BLOCK_FAIL,
            all(BANGBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)),
        boundedGroup(Production::CALL_BLOCK_MAYFAIL,
            all(QBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)) );
    CALL_SUBQUOTE = all( LBRACE, maybe( forward(CALL_INVOKE) ), RBRACE );
    CALL_QUOTE = group(Production::CALL_QUOTE, any( CALL_SUBQUOTE, CALL_BLOCKQUOTE) );

    CALL_CMD_TARGET = group(Production::CALL_CMD_TARGET, any(
            group(Production::CALL_QUOTED, all(EXEC_CMD, maybe(any(ON_EXIT, ON_EXIT_FAIL)), CALL_QUOTE)),
            group(Production::CALL_QUOTED, all(EXEC_CMD, maybe(any(ON_EXIT, ON_EXIT_FAIL)), IDENTIFIER)),
            IDENTIFIER ));
    CALL_EXPR_ADDR = group(Production::CALL_EXPR_ADDR, AMPERSAND);
    CALL_EXPR_DEREF = group(Production::CALL_EXPR_DEREF, CARAT);
    CALL_EXPR_INDEX = group(Production::CALL_EXPR_INDEX, all(
        LBRACKET,
        all(forward(SUBCALL_EXPRESSION), maybe(all(COMMA, forward(SUBCALL_EXPRESSION)))),
        RBRACKET ));
    CALL_EXPR_SUFFIX = all(
        maybe(oneOrMore(any(CALL_EXPR_DEREF, CALL_EXPR_INDEX))),
        maybe(CALL_EXPR_ADDR) );
    CALL_EXPR_TERM = all(
        any( LITERAL,
             group(Production::ENUM_DEREF, all(TYPENAME, LBRACKET, IDENTIFIER, RBRACKET)),
             forward(CALL_INVOKE),
             IDENTIFIER,
             all(LPAREN, forward(SUBCALL_EXPRESSION), RPAREN) ),
        maybe(CALL_EXPR_SUFFIX) );
    CALL_CMD_LITERAL = group(Production::CALL_CMD_LITERAL, all(
        any(COLANGLE, QLANGLE, BANGLANGLE), maybe(DEF_CMD_PARM_LIST), RANGLE, LBRACE, forward(CALL_GROUP), RBRACE ));

    SUBCALL_EXPRESSION = group(Production::CALL_EXPRESSION, any(
        CALL_CMD_LITERAL,
        CALL_QUOTE,
        all(CALL_EXPR_TERM, maybe(oneOrMore(all(CALL_OPERATOR,CALL_EXPR_TERM)))) ));
    CALL_ASSIGNMENT = boundedGroup(Production::CALL_ASSIGNMENT,
        all(CALL_IDENTIFIER, LARROW,
            all( SUBCALL_EXPRESSION, maybe(oneOrMore(all(PIPE, SUBCALL_EXPRESSION))) ) ),
            maybe(oneOrMore(all(CALL_OPERATOR, any(forward(CALL_INVOKE),SUBCALL_EXPRESSION)))) );

    CALL_CONSTRUCTOR = boundedGroup(Production::CALL_CONSTRUCTOR,
        all(TYPE_NAME_Q, COLON, separated(CALL_PARAMETER, COMMA)) );
    CALL_COMMAND = boundedGroup(Production::CALL_COMMAND,
        all(CALL_CMD_TARGET, maybe(all(COLON, separated(CALL_PARAMETER, COMMA)))) );
    CALL_VCOMMAND = boundedGroup(Production::CALL_VCOMMAND,
        all(LPAREN, separated(IDENTIFIER, COMMA), RPAREN, DCOLON, IDENTIFIER,
            maybe(all(COLON, separated(CALL_PARAMETER, COMMA))) ));
    CALL_INVOKE = any(CALL_VCOMMAND, CALL_CONSTRUCTOR, CALL_COMMAND);

    CALL_EXPRESSION = group(Production::CALL_EXPRESSION,
       all(CALL_EXPR_TERM,
           oneOrMore(all(CALL_OPERATOR,CALL_EXPR_TERM)) ));
    CALL_GROUP = group(Production::CALL_GROUP,
        oneOrMore(any(CALL_ASSIGNMENT, CALL_EXPRESSION, CALL_INVOKE, forward(BLOCK))) );
    RECOVER_SPEC = group(Production::RECOVER_SPEC,
        any(all(TYPE_NAME_Q, IDENTIFIER), CALL_EXPR_TERM) );
    BLOCK_HEADER = group(Production::BLOCK_HEADER,
        any(DO_WHEN_MULTI, DO_WHEN, DO_WHEN_FAIL,
            group(Production::DO_ELSE, MINUS),
            DO_UNLESS, DO_BLOCK,
            group(Production::DO_REWIND, CARAT),
            group(Production::DO_RECOVER_SPEC, all(PIPE, RECOVER_SPEC, RARROW)),
            group(Production::DO_RECOVER, PIPE),
            ON_EXIT, ON_EXIT_FAIL ));
    BLOCK = boundedGroup(Production::DO_BLOCK, all(BLOCK_HEADER, CALL_GROUP) );
    DEF_CMD_EMPTY = group(Production::DEF_CMD_EMPTY, UNDERSCORE);
    DEF_CMD_BODY = group(Production::DEF_CMD_BODY,all(
        discard(TokenType::EQUALS), any(DEF_CMD_EMPTY, CALL_GROUP) ));
}

void Grammar2::initCompilationUnit() {
    COMPILATION_UNIT = group(Production::COMPILATION_UNIT, all(
        maybe(DEF_MODULE),
        maybe(oneOrMore(DEF_IMPORT)),
        maybe(oneOrMore(any(
            DEF_ALIAS,
            DEF_CLASS,
            DEF_CMD,
            DEF_CMD_DECL,
            DEF_CMD_INTRINSIC,
            DEF_DOMAIN,
            DEF_ENUM,
            DEF_INSTANCE,
            DEF_OBJECT,
            DEF_PROGRAM,
            DEF_RECORD,
            DEF_TEST )))));
}

Grammar2& basis::getGrammar() {
   static Grammar2 grammar{};
   return grammar;
}

