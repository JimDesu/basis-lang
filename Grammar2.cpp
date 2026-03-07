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
// TODO evaluate which of these must actually be matches
   AMBANG = discard(TokenType::AMBANG);
   AMPERSAND = discard(TokenType::AMPERSAND);
   AMPHORA = discard(TokenType::AMPHORA);
   WRITEABLE = discard(TokenType::APOSTROPHE);
   ASTERISK = discard(TokenType::ASTERISK);
   BANG = discard(TokenType::BANG);
   BANGBRACE = discard(TokenType::BANGBRACE);
   BANGLANGLE = discard(TokenType::BANGLANGLE);
   CARAT = discard(TokenType::CARAT);
   COMMA = discard(TokenType::COMMA);
   COLON = discard(TokenType::COLON);
   COLANGLE = discard(TokenType::COLANGLE);
   COLBRACE = discard(TokenType::COLBRACE);
   DQMARK = discard(TokenType::DQMARK);
   DCOLON = discard(TokenType::DCOLON);
   EXEC_CMD = discard(TokenType::DOLLAR);
   EQUALS = discard(TokenType::EQUALS);
   EXTRACT = discard(TokenType::DRANGLE);
   GREQUALS = discard(TokenType::GREQUALS);
   INSERT = discard(TokenType::DLANGLE);
   LANGLE = discard(TokenType::LANGLE);
   LEQUALS = discard(TokenType::LEQUALS);
   LARROW = discard(TokenType::LARROW);
   LBRACE = discard(TokenType::LBRACE);
   LBRACKET = discard(TokenType::LBRACKET);
   LPAREN = discard(TokenType::LPAREN);
   MINUS = discard(TokenType::MINUS);
   PERCENT = discard(TokenType::PERCENT);
   PIPE = discard(TokenType::PIPE);
   PLUS = discard(TokenType::PLUS);
   POUND = discard(TokenType::POUND);
   QBRACE = discard(TokenType::QBRACE);
   QCOLON = discard(TokenType::QCOLON);
   QLANGLE = discard(TokenType::QLANGLE);
   QMARK = discard(TokenType::QMARK);
   QMINUS = discard(TokenType::QMINUS);
   RANGLE = discard(TokenType::RANGLE);
   RARROW = discard(TokenType::RARROW);
   RBRACE = discard(TokenType::RBRACE);
   RBRACKET = discard(TokenType::RBRACKET);
   RPAREN = discard(TokenType::RPAREN);
   SLASH = discard(TokenType::SLASH);
   UNDERSCORE = discard(TokenType::UNDERSCORE);
}

void Grammar2::initReservedWords() {
   ALIAS = discard(TokenType::ALIAS);
   CLASS = discard(TokenType::CLASS);
   COMMAND = discard(TokenType::COMMAND);
   DECLARE = discard(TokenType::DECLARE);
   DOMAIN = discard(TokenType::DOMAIN);
   ENUMERATION = discard(TokenType::ENUMERATION);
   IMPORT = discard(TokenType::IMPORT);
   INSTANCE = discard(TokenType::INSTANCE);
   INTRINSIC = discard(TokenType::INTRINSIC);
   MODULE = discard(TokenType::MODULE);
   OBJECT = discard(TokenType::OBJECT);
   PROGRAM = discard(TokenType::PROGRAM);
   RECORD = discard(TokenType::RECORD);
   TEST = discard(TokenType::TEST);
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
          all(TYPE_EXPR_RANGE, maybe(forward(TYPE_CMDEXPR_ARG))) ),
      maybe(as(Production::TYPE_ARG_WRITEABLE, WRITEABLE)) ));

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
   DEF_ENUM_NAME2 = all(
       match(Production::DEF_ENUM_TYPENAME, TokenType::TYPENAME),
       match(Production::DEF_ENUM_NAME, TokenType::TYPENAME) );
   DEF_ENUM_NAME1 = match(Production::DEF_ENUM_NAME, TokenType::TYPENAME);

   DEF_ENUM = exclusiveGroup(Production::DEF_ENUM,
       ENUMERATION, any(DEF_ENUM_NAME2, DEF_ENUM_NAME1), COLON, DEF_ENUM_ITEM_LIST);
}

void Grammar2::initModuleTypes() {
    DEF_MODULE_NAME = group(Production::DEF_MODULE_NAME, TYPENAME);
    DEF_MODULE = exclusiveGroup(Production::DEF_MODULE, all(MODULE, DEF_MODULE_NAME));
}

void Grammar2::initProgramDefinitions() {
    DEF_PROGRAM = exclusiveGroup(Production::DEF_PROGRAM,
        all(PROGRAM, forward(CALL_INVOKE)));
}

void Grammar2::initTestDefinitions() {
    DEF_TEST = exclusiveGroup(Production::DEF_TEST,
        all(TEST, STRING, EQUALS, CALL_GROUP));
}

void Grammar2::initImports() {
    //TODO add prefix here, and restructure to make AST building easier
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

   DEF_CMD_RECEIVERS = group(Production::DEF_CMD_RECEIVERS, all(
           any( all(LPAREN, separated(DEF_CMD_RECEIVER, COMMA), RPAREN), DEF_CMD_RECEIVER ),
           DCOLON ));
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

   DEF_CMD_REGULAR_FORM = all(DEF_CMD_NAME_SPEC, DEF_CMD_PARMS, DEF_CMD_IMPARMS);

   DEF_CMD_SIGNATURE = any(
       // destructor
       as(Production::DEF_CMD_RECEIVER_ATSTACK,all(AMPHORA, DEF_CMD_RECEIVER)),
       // failure handler
       as(Production::DEF_CMD_RECEIVER_ATSTACK_FAIL,all(AMBANG, DEF_CMD_RECEIVER)),
       // constructor
       all(DEF_CMD_RECEIVER, COLON, separated(DEF_CMD_PARM, COMMA)),
       // VCOMMAND - with receivers, allows -> result without params
       all(DEF_CMD_RECEIVERS, DEF_CMD_NAME_SPEC, DEF_CMD_VPARMS, DEF_CMD_IMPARMS),
       // Regular command - without receivers, requires params for -> result
       DEF_CMD_REGULAR_FORM );

   DEF_CMD_DECL = exclusiveGroup(Production::DEF_CMD_DECL,
       all(DECLARE, DEF_CMD_SIGNATURE));
   DEF_CMD_INTRINSIC = exclusiveGroup(Production::DEF_CMD_INTRINSIC,
       all(INTRINSIC, DEF_CMD_REGULAR_FORM) );

   DEF_CMD = exclusiveGroup(Production::DEF_CMD,
       all(COMMAND, DEF_CMD_SIGNATURE, forward(DEF_CMD_BODY)));

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
        any(DCOLON, PIPE, PLUS, MINUS, ASTERISK, SLASH, LANGLE, RANGLE, LEQUALS, GREQUALS, EQUALS, INSERT, EXTRACT) );

    CALL_BLOCKQUOTE = any(
        boundedGroup(Production::CALL_BLOCK_NOFAIL,
            all(COLBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)),
        boundedGroup(Production::CALL_BLOCK_FAIL,
            all(BANGBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)),
        boundedGroup(Production::CALL_BLOCK_MAYFAIL,
            all(QBRACE, any(forward(DEF_CMD_EMPTY),forward(CALL_GROUP)), RBRACE)) );
    CALL_SUBQUOTE = all( LBRACE, maybe( forward(CALL_INVOKE) ), RBRACE );
    CALL_QUOTE = group(Production::CALL_QUOTE, any( CALL_SUBQUOTE, CALL_BLOCKQUOTE) );

    CALL_CMD_TARGET = group(Production::CALL_CMD_TARGET, any( CALL_SUBQUOTE, IDENTIFIER ));
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

    SUBCALL_EXPRESSION = group(Production::SUBCALL_EXPRESSION, any(
        CALL_CMD_LITERAL,
        all(CALL_EXPR_TERM, maybe(oneOrMore(all(CALL_OPERATOR,CALL_EXPR_TERM)))),
        CALL_QUOTE ));

    CALL_CONSTRUCTOR = boundedGroup(Production::CALL_CONSTRUCTOR,
        all(TYPE_NAME_Q, COLON, separated(CALL_PARAMETER, COMMA)) );
    CALL_COMMAND = boundedGroup(Production::CALL_COMMAND,
        all(CALL_CMD_TARGET, maybe(all(COLON, separated(CALL_PARAMETER, COMMA)))) );
    CALL_VCOMMAND = boundedGroup(Production::CALL_VCOMMAND,
        all(LPAREN, separated(IDENTIFIER, COMMA), RPAREN, DCOLON, IDENTIFIER,
            maybe(all(COLON, separated(CALL_PARAMETER, COMMA))) ));
    CALL_INVOKE = any(CALL_VCOMMAND, CALL_CONSTRUCTOR, CALL_COMMAND);

    CALL_EXPRESSION = group(Production::CALL_EXPRESSION,
       all(CALL_EXPR_TERM, oneOrMore(all(CALL_OPERATOR, CALL_EXPR_TERM)) ));

    CALL_ASSIGNMENT = boundedGroup(Production::CALL_ASSIGNMENT,
        all(CALL_IDENTIFIER, LARROW,
            all( SUBCALL_EXPRESSION, maybe(oneOrMore(all(PIPE, SUBCALL_EXPRESSION))) ) ),
        maybe(oneOrMore(all(CALL_OPERATOR, any(forward(CALL_INVOKE),SUBCALL_EXPRESSION)))) );

    RECOVER_SPEC = group(Production::RECOVER_SPEC,
        any(all(TYPE_NAME_Q, IDENTIFIER), CALL_EXPR_TERM) );
    BLOCK = any(
        boundedGroup(Production::DO_WHEN_MULTI, all(DQMARK, forward(CALL_GROUP))),
        boundedGroup(Production::DO_WHEN, all(QMARK, forward(CALL_GROUP))),
        boundedGroup(Production::DO_WHEN_FAIL, all(QMINUS, forward(CALL_GROUP))),
        boundedGroup(Production::DO_WHEN_SELECT, all(QCOLON, forward(CALL_GROUP))),
        boundedGroup(Production::DO_ON_EXIT, all(AMPHORA, forward(CALL_GROUP))),
        boundedGroup(Production::DO_ON_EXIT_FAIL, all(AMBANG, forward(CALL_GROUP))),
        boundedGroup(Production::DO_ELSE, all(MINUS, forward(CALL_GROUP))),
        //boundedGroup(Production::DO_UNLESS, all(DO_UNLESS, forward(CALL_GROUP))),
        boundedGroup(Production::DO_BLOCK, all(PERCENT, forward(CALL_GROUP))),
        boundedGroup(Production::DO_REWIND, all(CARAT, forward(CALL_GROUP))),
        boundedGroup(Production::DO_RECOVER_SPEC, all(PIPE, RECOVER_SPEC, RARROW, forward(CALL_GROUP))),
        boundedGroup(Production::DO_RECOVER, all(PIPE, forward(CALL_GROUP))) );

    CALL_GROUP = group(Production::CALL_GROUP,
        oneOrMore(any(CALL_ASSIGNMENT, CALL_EXPRESSION, CALL_INVOKE, BLOCK)) );
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

