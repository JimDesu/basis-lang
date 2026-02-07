#include "doctest.h"

#include "../Grammar2.h"
#include "../Lexer.h"
#include <sstream>
#include <iostream>

using namespace basis;

namespace {
    // parser takes the token list by reference, so be sure to put the result onto the stack so
    // we don't end up wth a dangling reference problem (which caused a flaky test that took
    // forever to diagnose)
    std::list<spToken> tokenize(const std::string& text, bool& success) {
        std::istringstream input(text);
        Lexer lexer(input, false);
        success = lexer.scan();
        return lexer.output;
    }

    bool testParse(SPPF parseFn, const std::string& text) {
        bool lexSuccess = false;
        std::list<spToken> tokens = tokenize(text, lexSuccess);
        if (!lexSuccess) return false;  // Lexing failed, so parsing fails
        Parser parser(tokens, parseFn);
        return parser.parse() && parser.allTokensConsumed();
    }

    bool testParse(SPPF parseFn, const std::string& text, Production expected) {
        bool lexSuccess = false;
        std::list<spToken> tokens = tokenize(text, lexSuccess);
        if (!lexSuccess) return false;  // Lexing failed, so parsing fails
        Parser parser(tokens, parseFn);
        return parser.parse() && parser.allTokensConsumed()
            && parser.parseTree != nullptr && parser.parseTree->production == expected;
    }

    bool debugTestParse(SPPF parseFn, const std::string& text, Production expected) {
        bool lexSuccess = false;
        std::list<spToken> tokens = tokenize(text, lexSuccess);
        if (!lexSuccess) return false;  // Lexing failed, so parsing fails
        Parser parser(tokens, parseFn);
        if (parser.parse() && parser.allTokensConsumed()
            && parser.parseTree != nullptr && parser.parseTree->production == expected) return true;
        std::cerr << parser.getError();
        return false;
    }
}

TEST_CASE("Grammar2::test parse literals") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DECIMAL, "3.14", Production::DECIMAL));
    CHECK(testParse(grammar.HEXNUMBER, "0x1_234", Production::HEXNUMBER));
    CHECK(testParse(grammar.BINARY, "0b000_00100", Production::BINARY));
    CHECK(testParse(grammar.BINARY, "0b0000010000000001", Production::BINARY));
    CHECK(testParse(grammar.NUMBER, "1234", Production::NUMBER));
    CHECK(testParse(grammar.NUMBER, "-1234", Production::NUMBER));
    CHECK(testParse(grammar.STRING, "\"foo\\n\\\"bar's\"", Production::STRING));
    CHECK_FALSE(testParse(grammar.DECIMAL, "3.14.56"));
    CHECK_FALSE(testParse(grammar.HEXNUMBER, "0x12345"));
    CHECK_FALSE(testParse(grammar.BINARY, "0b0010"));
    CHECK_FALSE(testParse(grammar.BINARY, "0b000000100"));
    CHECK_FALSE(testParse(grammar.NUMBER, "1234.56"));
    CHECK_FALSE(testParse(grammar.STRING, "\"foo\nbar\""));
}

TEST_CASE("Grammar2::test parse identifiers") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.IDENTIFIER, "foobar", Production::IDENTIFIER));
    CHECK(testParse(grammar.TYPENAME, "Foobar", Production::TYPENAME));
    CHECK(testParse(grammar.TYPENAME, "Namespace::Type", Production::QUALIFIED_TYPENAME));
    CHECK(testParse(grammar.TYPENAME, "Std::Collections::List", Production::QUALIFIED_TYPENAME));
    CHECK(testParse(grammar.TYPENAME, "A::B::C::D::Type", Production::QUALIFIED_TYPENAME));
    CHECK(testParse(grammar.IDENTIFIER, "Module::identifier", Production::QUALIFIED_IDENTIFIER));
    CHECK(testParse(grammar.IDENTIFIER, "Std::IO::println", Production::QUALIFIED_IDENTIFIER));
    CHECK(testParse(grammar.IDENTIFIER, "A::B::C::method", Production::QUALIFIED_IDENTIFIER));
    CHECK_FALSE(testParse(grammar.TYPENAME, "namespace::Type"));  // lowercase namespace
    CHECK_FALSE(testParse(grammar.IDENTIFIER, "module::identifier"));  // lowercase module
    CHECK_FALSE(testParse(grammar.TYPENAME, "Namespace::type"));  // lowercase final component
    CHECK_FALSE(testParse(grammar.IDENTIFIER, "Module::Identifier"));  // uppercase final component
}

TEST_CASE("Grammar2::test parse reserved words") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.ALIAS, ".alias", Production::ALIAS));
    CHECK(testParse(grammar.CLASS, ".class", Production::CLASS));
    CHECK(testParse(grammar.COMMAND, ".cmd", Production::COMMAND));
    CHECK(testParse(grammar.DOMAIN, ".domain", Production::DOMAIN));
    CHECK(testParse(grammar.ENUMERATION, ".enum", Production::ENUMERATION));
    CHECK(testParse(grammar.IMPORT, ".import", Production::IMPORT));
    CHECK(testParse(grammar.INTRINSIC, ".intrinsic", Production::INTRINSIC));
    CHECK(testParse(grammar.MODULE, ".module", Production::MODULE));
    CHECK(testParse(grammar.OBJECT, ".object", Production::OBJECT));
    CHECK(testParse(grammar.PROGRAM, ".program", Production::PROGRAM));
    CHECK(testParse(grammar.RECORD, ".record", Production::RECORD));
    CHECK(testParse(grammar.TEST, ".test", Production::TEST));
}

TEST_CASE("Grammar2::test parse punctuation") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.AMBANG, "@!", Production::AMBANG));
    CHECK(testParse(grammar.AMPERSAND, "&", Production::AMPERSAND));
    CHECK(testParse(grammar.AMPHORA, "@", Production::AMPHORA));
    CHECK(testParse(grammar.ASTERISK, "*", Production::ASTERISK));
    CHECK(testParse(grammar.BANG, "!", Production::BANG));
    CHECK(testParse(grammar.BANGBRACE, "!{", Production::BANGBRACE));
    CHECK(testParse(grammar.BANGLANGLE, "!<", Production::BANGLANGLE));
    CHECK(testParse(grammar.CARAT, "^", Production::CARAT));
    CHECK(testParse(grammar.COMMA, ",", Production::COMMA));
    CHECK(testParse(grammar.COLON, ":", Production::COLON));
    CHECK(testParse(grammar.COLANGLE, ":<", Production::COLANGLE));
    CHECK(testParse(grammar.COLBRACE, ":{", Production::COLBRACE));
    CHECK(testParse(grammar.DCOLON, "::", Production::DCOLON));
    CHECK(testParse(grammar.DOLLAR, "$", Production::DOLLAR));
    CHECK(testParse(grammar.EQUALS, "=", Production::EQUALS));
    CHECK(testParse(grammar.LANGLE, "<", Production::LANGLE));
    CHECK(testParse(grammar.LARROW, "<-", Production::LARROW));
    CHECK(testParse(grammar.LBRACE, "{", Production::LBRACE));
    CHECK(testParse(grammar.LBRACKET, "[", Production::LBRACKET));
    CHECK(testParse(grammar.LPAREN, "(", Production::LPAREN));
    CHECK(testParse(grammar.MINUS, "-", Production::MINUS));
    CHECK(testParse(grammar.PERCENT, "%", Production::PERCENT));
    CHECK(testParse(grammar.PIPE, "|", Production::PIPE));
    CHECK(testParse(grammar.PLUS, "+", Production::PLUS));
    CHECK(testParse(grammar.POUND, "#", Production::POUND));
    CHECK(testParse(grammar.QBRACE, "?{", Production::QBRACE));
    CHECK(testParse(grammar.QCOLON, "?:", Production::QCOLON));
    CHECK(testParse(grammar.QLANGLE, "?<", Production::QLANGLE));
    CHECK(testParse(grammar.QMARK, "?", Production::QMARK));
    CHECK(testParse(grammar.QMINUS, "?-", Production::QMINUS));
    CHECK(testParse(grammar.DQMARK, "??", Production::DQMARK));
    CHECK(testParse(grammar.RANGLE, ">", Production::RANGLE));
    CHECK(testParse(grammar.RARROW, "->", Production::RARROW));
    CHECK(testParse(grammar.RBRACE, "}", Production::RBRACE));
    CHECK(testParse(grammar.RBRACKET, "]", Production::RBRACKET));
    CHECK(testParse(grammar.RPAREN, ")", Production::RPAREN));
    CHECK(testParse(grammar.SLASH, "/", Production::SLASH));
    CHECK(testParse(grammar.UNDERSCORE, "_", Production::UNDERSCORE));
}

TEST_CASE("Grammar2::test parse enum definition") {
    Grammar2& grammar = getGrammar();
    // untyped enumeration
    CHECK(testParse(grammar.DEF_ENUM, ".enum Fish: sockeye = 0, salmon = 1", Production::DEF_ENUM));
    // enumeration with a type
    CHECK(testParse(grammar.DEF_ENUM, ".enum T Fish: sockeye = 0, salmon = 1", Production::DEF_ENUM));
    CHECK_FALSE(testParse(grammar.DEF_ENUM, ".enum A B fish: sockeye = 0, salmon = 1"));
    CHECK_FALSE(testParse(grammar.DEF_ENUM, ".enum T Fish: sockeye= 0,\nsalmon = 1"));
    CHECK_FALSE(testParse(grammar.DEF_ENUM, ".enum T Fish: Sockeye= 0, Salmon = 1"));
}

TEST_CASE("Grammar2::test module definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_MODULE, ".module MyModule", Production::DEF_MODULE));
    CHECK(testParse(grammar.DEF_MODULE, ".module Std::Collections", Production::DEF_MODULE));
    CHECK(testParse(grammar.DEF_MODULE, ".module A::B::C", Production::DEF_MODULE));
    CHECK_FALSE(testParse(grammar.DEF_MODULE, ".module"));  // missing name
    CHECK_FALSE(testParse(grammar.DEF_MODULE, ".module MyModule:"));  // extra colon
    CHECK_FALSE(testParse(grammar.DEF_MODULE, ".module MyModule: Int"));  // extra content
    CHECK_FALSE(testParse(grammar.DEF_MODULE, "module MyModule"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_MODULE, ".module 123"));  // numeric name
    CHECK_FALSE(testParse(grammar.DEF_MODULE, ".module my_module"));  // identifier instead of typename
}

TEST_CASE("Grammar2::test program definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_PROGRAM, ".program = main", Production::DEF_PROGRAM));
    CHECK(testParse(grammar.DEF_PROGRAM, ".program = start: arg1, arg2", Production::DEF_PROGRAM));
    CHECK(testParse(grammar.DEF_PROGRAM, ".program = run: config", Production::DEF_PROGRAM));
    CHECK_FALSE(testParse(grammar.DEF_PROGRAM, ".program main"));  // missing equals
    CHECK_FALSE(testParse(grammar.DEF_PROGRAM, ".program ="));  // missing command
    CHECK_FALSE(testParse(grammar.DEF_PROGRAM, "program = main"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_PROGRAM, ".program = Main"));  // typename instead of identifier
}

TEST_CASE("Grammar2::test test definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_TEST, ".test \"simple test\" = doSomething", Production::DEF_TEST));
    CHECK(testParse(grammar.DEF_TEST, ".test \"test with params\" = run: arg1, arg2", Production::DEF_TEST));
    CHECK(testParse(grammar.DEF_TEST, ".test \"complex test\" = setup\n    validate: data", Production::DEF_TEST));
    CHECK(testParse(grammar.DEF_TEST, ".test \"constructor test\" = Widget: 10, 20", Production::DEF_TEST));
    CHECK(testParse(grammar.DEF_TEST, ".test \"assignment test\" = result <- compute: x", Production::DEF_TEST));
    CHECK_FALSE(testParse(grammar.DEF_TEST, ".test simple = doSomething"));  // missing quotes on string
    CHECK_FALSE(testParse(grammar.DEF_TEST, ".test \"test\" doSomething"));  // missing equals
    CHECK_FALSE(testParse(grammar.DEF_TEST, ".test \"test\" ="));  // missing call group
    CHECK_FALSE(testParse(grammar.DEF_TEST, "test \"test\" = doSomething"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_TEST, ".test = doSomething"));  // missing string literal
}

TEST_CASE("Grammar2::test import definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_IMPORT, ".import \"file.basis\"", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import Module", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import MyLib:Utils", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import A:B", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import Prefix:A::B::C", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import Std::Collections", Production::DEF_IMPORT));
    CHECK(testParse(grammar.DEF_IMPORT, ".import A::B::C", Production::DEF_IMPORT));
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import"));  // missing argument
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, "import Module"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import module"));  // lowercase identifier
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import 123"));  // numeric
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import A:B:C"));  // single colon can only appear once
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import A::B:C::D"));  // parses as A::B with leftover :C::D
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import Std::Core:Collections"));  // parses as Std::Core with leftover :Collections
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import :Module"));  // colon without prefix
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import Prefix:"));  // colon without typename
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import \"file.basis\" extra"));  // extra content
    CHECK_FALSE(testParse(grammar.DEF_IMPORT, ".import Module extra"));  // extra content
}

TEST_CASE("Grammar2::test domain type definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain MyInt: Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain UserId: Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain Temperature: Float", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain FixedBuffer: [10]", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain DynamicArray: []", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain MyInt:\n Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain MyType: Std::Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain UserId: Database::Types::Id", Production::DEF_DOMAIN));
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain SmallInt: []Int", Production::DEF_DOMAIN));
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain SizedArray: [size]", Production::DEF_DOMAIN));
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain ByteArray: [256]Byte", Production::DEF_DOMAIN));
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain SmallInt:\n []Int", Production::DEF_DOMAIN));
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain myInt: Int"));  // lowercase name
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain MyInt"));  // missing colon and parent
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain MyInt:"));  // missing parent
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain : Int"));  // missing name
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain MyInt: int"));  // lowercase parent
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain MyInt: ^Int"));  // pointer not allowed
    CHECK_FALSE(testParse(grammar.DEF_DOMAIN, ".domain MyInt: List[T]"));  // parameterized type not allowed
}

TEST_CASE("Grammar2::test record type definitions") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_RECORD, ".record Person: String name", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Point3D: Int x, Int y, Int z", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Pair[T]: T first, T second", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Pair[T, U]: T first, U second", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Buffer[Int size]: [size]Byte data", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Matrix[Int rows, Int cols]: [rows][cols]Float values", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Array[T, Int size]: [size]T elements", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Container: List[Int] items", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Mapping: Map[String, Int] data", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Complex: List[Pair[String, Int]] entries", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Buffer: [256]Byte data", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Matrix4x4: [4][4]Float elements", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Entity: String name, [3]Float position, List[Component] components", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Point:\n Int x, Int y", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Person: Std::String name", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Point: Graphics::Int x, Graphics::Int y", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Container: Std::Collections::List[T] items", Production::DEF_RECORD));
    CHECK(testParse(grammar.DEF_RECORD, ".record Entity: UI::Widget widget, Database::Id id", Production::DEF_RECORD));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record point: Int x"));  // lowercase name
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point"));  // missing colon and fields
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point:"));  // missing fields
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record : Int x"));  // missing name
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point Int x"));  // missing colon
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: int x"));  // lowercase type
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int X"));  // uppercase field name
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int"));  // missing field name
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: x"));  // missing field type
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int x Int y"));  // missing comma
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int x,"));  // trailing comma
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int x,\nInt y"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Person: String name,\nInt age"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Node: ^Node next"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Handler: :<Int> callback"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Processor: ?<String'> handler"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Worker: !<Int', String> task"));
    CHECK_FALSE(testParse(grammar.DEF_RECORD, ".record Point: Int' x"));
}

TEST_CASE("Grammar2::test record field parsing") {
    Grammar2& grammar = getGrammar();

    // Basic field types
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "Int x", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "String name", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "Float value", Production::DEF_RECORD_FIELD));

    // Parameterized field types
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "List[Int] items", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "Map[String, Int] data", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "Pair[T, U] pair", Production::DEF_RECORD_FIELD));

    // Fixed-size array fields
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[3]Float vector", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[10]Int buffer", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[4][4]Float matrix", Production::DEF_RECORD_FIELD));

    // Identifier-sized array fields
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[size]Byte data", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[width][height]Int grid", Production::DEF_RECORD_FIELD));

    // Complex nested types
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[10]List[String] items", Production::DEF_RECORD_FIELD));
    CHECK(testParse(grammar.DEF_RECORD_FIELD, "[rows][cols]Matrix[Float] data", Production::DEF_RECORD_FIELD));

    // Negative tests
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "int x"));  // lowercase type
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "Int X"));  // uppercase field name
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "Int"));  // missing field name
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "x"));  // missing type
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "^Int ptr"));  // pointer not allowed
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, ":<Int> cmd"));  // command type not allowed
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELD, "Int' x"));  // apostrophe not allowed
}

TEST_CASE("Grammar2::test record fields list parsing") {
    Grammar2& grammar = getGrammar();

    // Single field
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "Int x", Production::DEF_RECORD_FIELDS));
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "String name", Production::DEF_RECORD_FIELDS));

    // Multiple fields
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "Int x, Int y", Production::DEF_RECORD_FIELDS));
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "String name, Int age, Float salary", Production::DEF_RECORD_FIELDS));
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "T first, U second", Production::DEF_RECORD_FIELDS));

    // Complex field types
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "List[Int] items, Map[String, Int] data", Production::DEF_RECORD_FIELDS));
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "[3]Float position, [4]Float rotation", Production::DEF_RECORD_FIELDS));
    CHECK(testParse(grammar.DEF_RECORD_FIELDS, "[size]Byte buffer, Int size, String name", Production::DEF_RECORD_FIELDS));

    // Negative tests
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELDS, "Int x,"));  // trailing comma
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELDS, "Int x Int y"));  // missing comma
    CHECK_FALSE(testParse(grammar.DEF_RECORD_FIELDS, ""));  // empty
}

TEST_CASE("Grammar2::test type expressions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "T", Production::TYPEDEF_PARM_TYPE));
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "T U", Production::TYPEDEF_PARM_TYPE));
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "String", Production::TYPEDEF_PARM_TYPE));
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "List[T] Element", Production::TYPEDEF_PARM_TYPE));
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "List[4] Element", Production::TYPEDEF_PARM_TYPE));
    CHECK(testParse(grammar.TYPEDEF_PARM_TYPE, "List[T,4] Element", Production::TYPEDEF_PARM_TYPE));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_TYPE, "t"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_TYPE, "T'"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_TYPE, "T value"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_TYPE, "T U V"));

    CHECK(testParse(grammar.TYPEDEF_PARM_VALUE, "T value", Production::TYPEDEF_PARM_VALUE));
    CHECK(testParse(grammar.TYPEDEF_PARM_VALUE, "String name", Production::TYPEDEF_PARM_VALUE));
    CHECK(testParse(grammar.TYPEDEF_PARM_VALUE, "Int count", Production::TYPEDEF_PARM_VALUE));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_VALUE, "T"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_VALUE, "t value"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_VALUE, "T Value"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_VALUE, "value T"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARM_VALUE, "T' value"));

    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T, U]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T U]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T value]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T, U value]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T U, V value]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[T, U, V]", Production::TYPEDEF_PARMS));
    CHECK(testParse(grammar.TYPEDEF_PARMS, "[String name, Int count]", Production::TYPEDEF_PARMS));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "T"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "[T"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "T]"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "[]"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "[t]"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_PARMS, "[T,]"));

    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "Int", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "String", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "T", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "List[T]", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "Map[K, V]", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "Container[T U]", Production::TYPEDEF_NAME_Q));
    CHECK(testParse(grammar.TYPEDEF_NAME_Q, "Dict[String key, Int value]", Production::TYPEDEF_NAME_Q));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "int"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "value"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "Int'"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "T'"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "Int[]"));
    CHECK_FALSE(testParse(grammar.TYPEDEF_NAME_Q, "[T]"));

    CHECK(testParse(grammar.TYPE_ARG_VALUE, "42", Production::TYPE_ARG_VALUE));
    CHECK(testParse(grammar.TYPE_ARG_VALUE, "size", Production::TYPE_ARG_VALUE));
    CHECK(testParse(grammar.TYPE_ARG_VALUE, "count", Production::TYPE_ARG_VALUE));
    CHECK(testParse(grammar.TYPE_ARG_VALUE, "10", Production::TYPE_ARG_VALUE));
    CHECK_FALSE(testParse(grammar.TYPE_ARG_VALUE, "Size"));  // TYPENAME not allowed
    CHECK_FALSE(testParse(grammar.TYPE_ARG_VALUE, "Int"));   // TYPENAME not allowed
    CHECK_FALSE(testParse(grammar.TYPE_ARG_VALUE, "3.14"));  // DECIMAL not allowed

    CHECK(testParse(grammar.TYPE_ARG_TYPE, "Int", Production::TYPE_ARG_TYPE));
    CHECK(testParse(grammar.TYPE_ARG_TYPE, "String", Production::TYPE_ARG_TYPE));
    CHECK(testParse(grammar.TYPE_ARG_TYPE, "T", Production::TYPE_ARG_TYPE));
    CHECK(testParse(grammar.TYPE_ARG_TYPE, "List[Int]", Production::TYPE_ARG_TYPE));
    CHECK(testParse(grammar.TYPE_ARG_TYPE, "Map[String, Int]", Production::TYPE_ARG_TYPE));
    CHECK_FALSE(testParse(grammar.TYPE_ARG_TYPE, "int"));    // identifier not allowed
    CHECK_FALSE(testParse(grammar.TYPE_ARG_TYPE, "value"));  // identifier not allowed
    CHECK_FALSE(testParse(grammar.TYPE_ARG_TYPE, "42"));     // NUMBER not allowed

    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[Int]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[T]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[10]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[size]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[Int, String]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[T, U]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[10, 20]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[width, height]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[Int, 10]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[T, size]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[String, count, Int]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[List[Int]]", Production::TYPE_NAME_ARGS));
    CHECK(testParse(grammar.TYPE_NAME_ARGS, "[Map[K, V], 10]", Production::TYPE_NAME_ARGS));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "Int"));      // missing brackets
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "[Int"));     // missing closing bracket
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "Int]"));     // missing opening bracket
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "[]"));       // empty not allowed
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "[Int,]"));   // trailing comma
    CHECK_FALSE(testParse(grammar.TYPE_NAME_ARGS, "[,Int]"));   // leading comma

    CHECK(testParse(grammar.TYPE_NAME_Q, "Int", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "String", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "T", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "MyType", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "List[Int]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "List[T]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Array[10]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Buffer[size]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Map[String, Int]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Map[K, V]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Matrix[10, 20]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Grid[width, height]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Container[T, 100]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Tuple[Int, String, Float]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Nested[List[Int]]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Complex[Map[K, V], size]", Production::TYPE_NAME_Q));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "int"));         // identifier not allowed
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "myType"));      // identifier not allowed
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "Int'"));        // apostrophe not allowed
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "List[]"));      // empty brackets not allowed
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "List[Int,]"));  // trailing comma
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "[Int]"));       // missing typename

    CHECK(testParse(grammar.TYPE_EXPR_PTR, "^", Production::TYPE_EXPR_PTR));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[]", Production::TYPE_EXPR_RANGE));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[10]", Production::TYPE_EXPR_RANGE));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[size]", Production::TYPE_EXPR_RANGE));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "[Size]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "["));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "]"));

    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[8]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[][8]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[8][]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<^[8][]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[8]^[]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<List[T]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<^[]Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<:<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<[]'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<^[]'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<^[8]'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<Int', String'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<List[T]'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<^[]Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<:<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int', []String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<List[T]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<List[4]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<List[T,4]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<^[]Int', String'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<:<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<?<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, ":<>"));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<>"));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<^>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<r>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<Int,>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "?<Int,>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "!<Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<?Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, ":<!Int>"));

    CHECK(testParse(grammar.TYPE_EXPR, "Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "String", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "T", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "List[T]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "Map[K, V]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "Dict[String key, Int value]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^List[T]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[10]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[size]String", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]List[T]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]^Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^^Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[][]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[10]String", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[5]^Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]^Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<List[T], Map[K, V]'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<Int, String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<List[T]', Map[K, V]>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^:<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]:<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]:<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^ ?<Int'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]!<Int, String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]?<Int'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<:<Int>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<^Int', []String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^:<^Int, []String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<:<Int>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<?<Int'>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<?<Int>, !<String>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^ ?<^Int', []String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]!<^Int, []String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]List[T]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]^Map[K, V]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^^[]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[10][20]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[10][20]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[][20]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[10][]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, ":<>"));
    CHECK(testParse(grammar.TYPE_EXPR, "?<>"));
    CHECK(testParse(grammar.TYPE_EXPR, "!<>"));
    CHECK(testParse(grammar.TYPE_EXPR, "[]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "?<List'[T], Map[K, V']>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "T'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "List'[T']"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "^Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "[]String'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "value"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "^"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int[]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int^"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "[T]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "<?Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "<!Int>"));
}

TEST_CASE("Grammar2::test TYPE_EXPR_DOMAIN") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "Float", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "T", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "List[T]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "Map[K, V]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "Array[10]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "Container[T, U]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "Matrix[Int]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[size]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[100]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]Int", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[size]String", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]Map[K, V]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[size]Container[T]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10][20]Int", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[5][10][15]Int", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10][20]Map[K, V]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[10][20]", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[rows]Int", Production::TYPE_EXPR_DOMAIN));
    CHECK(testParse(grammar.TYPE_EXPR_DOMAIN, "[rows][cols]Int", Production::TYPE_EXPR_DOMAIN));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^String"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^List[T]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^^Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^[]Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]^Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "^[10]Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]^Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, ":<Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "?<Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "!<Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, ":<Int, String>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "?<Int', String>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "!<List[T]>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]:<Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]?<String>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "String'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "List[T]'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]String'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "string"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "myType"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[10]string"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[Size]Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "["));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "Int[]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]Array[10]"));
}

TEST_CASE("Grammar2::test type alias definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt: Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias T: U", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias List[T]: ^[]T", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]: ^[]Pair[K, V]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Dict[String key, Int value]: ^[]Entry", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntPtr: ^Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias StringPtr: ^String", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias PtrPtr: ^^Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyType: Std::Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias List[T]: ^[]Collections::Node[T]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias WidgetPtr: ^UI::Widget", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]: ^[]Std::Collections::Pair[K, V]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias CmdType: :<Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeCmdType: ?<String', Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntArray: []Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FixedArray: [10]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias SizedArray: [size]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Matrix: [][]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd: :<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeInt: ?<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FailInt: !<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MultiCmd: :<Int, String>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias ComplexType: ^[]List[T]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias CmdArray: []:<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias PtrArray: ^[]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias NestedCmd: :<^[]Int, String>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias DeepNest: ^[]?<List[T]>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt:\n Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]:\n ^[]Pair[K, V]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias ComplexType:\n ^[]List[T]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd:\n :<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt:\n Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt: :<>"));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd:\n :<Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeInt:\n ?<Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FailInt:\n !<Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt: []"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias String': String"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias T': U'"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias IntPtr: ^Int'"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias ComplexType': ^[]List'[T']"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias List'[T']:\n ^[]T"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias Map'[K', V']:\n ^[]Pair[K, V]"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias ComplexType':\n ^[]List[T]"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias IntCmd':\n <Int>"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt':\n Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt:"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias : Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, "MyInt: Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias myInt: Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias 123: Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias 'MyInt: Int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt: int"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt: ^"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt:\nInt"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias List[T]:\n^[]T"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]:\nPair[K, V]"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias ComplexType:\n ^[]List[T']"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias DeepNest:\n ^[]?<List'[T']>"));
}

TEST_CASE("Grammar2::test command declarations") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "Int", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "String", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "T", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "List[T]", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "Map[K, V]", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^Int", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^String", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]Int", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[10]Int", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^[]List[T]", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "Std::String", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "Collections::List[T]", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^Namespace::Type", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]Module::Item", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[10]Pkg::Array", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^[]Std::Collections::List[A::B::Type]", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, ":<>"));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, ":<Int>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "?<Int', String>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^:<Int'>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]?<Int, String'>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]"));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[8]"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "!<List[T']>"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "String'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "T'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^Int'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "value"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "int"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^"));

    CHECK(testParse(grammar.DEF_CMD_PARM_NAME, "value", Production::DEF_CMD_PARM_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARM_NAME, "'value", Production::DEF_CMD_PARM_NAME));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_NAME, "Value"));

    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: String)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: ^Int)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: List[U])", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(U: ^[]Int)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: :<Int'>)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(U: ?<String', Int>)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T': String)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: String')"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T': []String')"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "T: Int"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(list: Int)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: int)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(^T: String)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "([]T: Int)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T Int)"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T Int"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "T Int)"));

    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "String", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "^Int", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "[]List[T]", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "(T: Int)", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "(U: []List[T])", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "?<Int', String>", Production::DEF_CMD_PARM_TYPE));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_TYPE, "String'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_TYPE, "(T': ^Int)"));

    CHECK(testParse(grammar.DEF_CMD_PARM, "Int x", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "String name", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "^Int ptr", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "[]List[T] items", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "?<Int', String> cmd", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "Std::String text", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "Collections::List[T] items", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "^Namespace::Type ptr", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "[]Module::Item array", Production::DEF_CMD_PARM));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "String' name"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "value x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "Int X"));

    CHECK(testParse(grammar.DEF_CMD_NAME, "doSomething", Production::DEF_CMD_NAME));
    CHECK_FALSE(testParse(grammar.DEF_CMD_NAME, "DoSomething"));

    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "Widget w", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "Widget 'w", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "^Widget ptr", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "[]Container[T] items", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "?<Widget'> cmd", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "UI::Widget w", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "Std::Collections::Container[T] items", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "^Namespace::Type ptr", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "[]Module::Item array", Production::DEF_CMD_RECEIVER));
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "Widget' w"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "widget w"));

    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl ?doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl !doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int 'x, String y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int 'x -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int ctx / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int x / Int 'ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int x -> result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int x, String y -> result / Int ctx, String s", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: ?doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: !doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w, Button b):: doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w, Button 'b):: doIt: Int x -> 'result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: doIt: Int x -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: doIt: Int x -> result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: method -> w", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Container c):: getSize -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w, Button b):: combine -> w", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Handler h):: ?tryGet -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Processor p):: !mustProcess -> output", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Widget w):: getValue -> result / Int ctx", Production::DEF_CMD_DECL));

    // Test qualified typenames in command declarations
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (UI::Widget w):: render", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (Std::Collections::List[T] items):: process", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Namespace::Type param -> result", Production::DEF_CMD_DECL));
    // syntactically but not semantically valid
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl process: Module::Item x -> Module::result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl (UI::Widget w):: handle: IO::Event e -> UI::response / Std::Context ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: Int i, Int j", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: ?<Int'> cmd", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl doIt: !<String', Int> result", Production::DEF_CMD_DECL));

    // Destructor tests (using @ AMPHORA)
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @ Widget w: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @ Widget w: Int x, Int y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @ Container c: String name", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @ Handler h: ^Int ptr, []String items", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @ Resource r: ?<Int'> cmd", Production::DEF_CMD_DECL));

    // Failure handler tests (using @! AMBANG)
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @! Widget w: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @! Widget w: Int x, Int y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @! Container c: String error", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @! Handler h: ^Int ptr, []String items", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".decl @! Resource r: !<String', Int> result", Production::DEF_CMD_DECL));

    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: ^Int' ptr"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: List[T'] list"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl (Widget' w):: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl (^Widget' ptr):: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl ([]Container[T'] items):: process"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl (Widget' w):: ^Int' ptr, []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / Int ctx: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int ?x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: Int !x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: ?Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt: !Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl (Widget 'w, Button 'b):: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / :"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / Int i:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt : Int i -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt : -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / x ->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / Int j: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl doIt / Int j: Int i -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: Int i / Int j -> i"));

    // Destructor negative tests
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @ doIt: Int x -> result"));  // destructor needs receiver
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @ Widget w"));  // destructor needs parameters
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: @ Int x"));  // @ in wrong position
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @ Widget' w: Int x"));  // receiver can't have apostrophe

    // Failure handler negative tests
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @! (Widget w):: Int i"));  // failure handler can't use ::
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @! doIt: Int x -> result"));  // failure handler needs receiver
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @! Widget w"));  // failure handler needs parameters
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl Widget w: @! Int x"));  // @! in wrong position
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".decl @! Widget' w: Int x"));  // receiver can't have apostrophe

    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt"));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x -> result", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic ?doIt: Int x -> result", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic !doIt: Int x -> result", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x -> result / Int ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic ?doIt: Int x -> result / Int ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic !doIt: Int x -> result / Int ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int 'x, String y -> result / Int 'ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: ^Int ptr -> 'result / String ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: []List[T] items -> result / Int ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: ?<Int'> cmd -> result / String ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: !<String', Int> result -> out / Int ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: (T:Int) i, T j -> result / String ctx", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x, String y -> result / Int ctx, String s", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x -> result", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x, String y -> result", Production::DEF_CMD_INTRINSIC));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt / Int ctx"));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x"));
    CHECK(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt :Int x /Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x -> result {"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: Int x -> result = x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt / Int ctx: Int x -> result {"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt / Int ctx: Int x -> result = x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: ^Int' ptr -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: []String' items -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_INTRINSIC, ".intrinsic doIt: List[T'] list -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / String ctx: (T': ^Int) x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx: ^Int' ptr -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int ?x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int !x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: ?Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: !Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int ctx /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x ->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt :Int x -> / Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt -> / Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result / :"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: -> result / Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx: Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic (Widget w):: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic (Widget w):: doIt: Int x -> result / Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic (Widget w, Button b):: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic (^Widget ptr):: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic ([]Container[T] items):: process: T item -> result"));
}

TEST_CASE("Grammar2::test object field parsing") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "Int x", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "List[Int] items", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "Map[String, Int] data", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[10]Int buffer", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[4][4]Float matrix", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[size]Byte data", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[width][height]Int grid", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[]String strings", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "^String strPtr", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "^List[T] listPtr", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "^^Int ptrPtr", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "^[]Int arrayPtr", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[]^Int ptrArray", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, ":<Int> cmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "?<String> mayFailCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "!<Float> failCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, ":<> voidCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "?<> mayFailVoidCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "!<> failVoidCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, ":<Int, String> multiCmd", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "^:<Int> cmdPtr", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[]:<String> cmdArray", Production::DEF_OBJECT_FIELD));
    CHECK(testParse(grammar.DEF_OBJECT_FIELD, "[5]^Int ptrArray", Production::DEF_OBJECT_FIELD));
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "int x"));  // lowercase type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "Int X"));  // uppercase field name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "Int"));  // missing field name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "x"));  // missing type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "Int' x"));  // apostrophe not allowed in type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELD, "Int x'"));  // apostrophe not allowed in field name
}

TEST_CASE("Grammar2::test object fields list parsing") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "String name", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "String name, Int age, Float salary", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "List[Int] items, Map[String, Int] data", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "[3]Float position, [4]Float rotation", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "[size]Byte buffer, Int size, String name", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "^Node next, ^Node prev, Int data", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "[]^String strPtrs, Int count", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, ":<Int> callback, String name", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "?<String> loader, !<> cleanup", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, ":<> action1, :<> action2, :<> action3", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "^Int ptr, []String names, :<> callback", Production::DEF_OBJECT_FIELDS));
    CHECK(testParse(grammar.DEF_OBJECT_FIELDS, "List[T] items, ^Node next, ?<T> processor", Production::DEF_OBJECT_FIELDS));
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELDS, "Int x,"));  // trailing comma
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELDS, "Int x Int y"));  // missing comma
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELDS, ""));  // empty
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELDS, ",Int x"));  // leading comma
    CHECK_FALSE(testParse(grammar.DEF_OBJECT_FIELDS, "Int x,,Int y"));  // double comma
}

TEST_CASE("Grammar2::test object type definitions") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_OBJECT, ".object Point: Int x, Int y", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Node: T value, ^Node next", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Wrapper: Int value", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object List[T]: T head, ^List[T] tail", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Map[K, V]: K key, V value, ^Map[K, V] next", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object LinkedList: Int data, ^LinkedList next, ^LinkedList prev", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object RefCounter: ^Data ptr, Int count", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Vector3: [3]Float coords", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Matrix: [4][4]Float data", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object DynamicArray: []Int items, Int size, Int capacity", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Buffer: [size]Byte data, Int size", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Handler: :<> callback, String name", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Processor: ?<String> loader, :<Int> processor, !<> cleanup", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object EventListener: :<Event> handler, Int priority", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Complex: ^Int ptr, []String names, :<> callback, List[T] items", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object State: Map[String, Int] vars, []:<> actions, ^State next", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Graph[T]: T value, []^Graph[T] neighbors, ?<T> validator", Production::DEF_OBJECT));
    CHECK(testParse(grammar.DEF_OBJECT, ".object Cache[K, V]: Map[K, V] data, :<V> loader, Int size", Production::DEF_OBJECT));
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object point: Int x"));  // lowercase name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point"));  // missing colon and fields
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point:"));  // missing fields
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object : Int x"));  // missing name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point Int x"));  // missing colon
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, "object Point: Int x"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: int x"));  // lowercase type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int X"));  // uppercase field name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int"));  // missing field name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int x,"));  // trailing comma
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int x Int y"));  // missing comma between fields
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int' x"));  // apostrophe in type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int x'"));  // apostrophe in field name
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: ^Int' ptr"));  // apostrophe in pointer type
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".object Point: Int x,\nInt y"));  // newline in field list
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ".Object Point: Int x"));  // wrong keyword case
    CHECK_FALSE(testParse(grammar.DEF_OBJECT, ". object Point: Int x"));  // wrong keyword case
}

TEST_CASE("Grammar2::test class definition parsing") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass:\n"
        "  .decl doSomething: Int x -> result\n"
        "  .decl doOther: String s -> output",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass: .decl doSomething: Int x -> result\n"
        "                .decl doOther: String s -> output",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass: .decl doSomething: Int x -> result\n"
        "  .decl doOther: String s -> output",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .decl Container c: Int capacity\n"
        "  .decl add: Int item -> result\n"
        "  .decl (Widget w):: process: String s -> output / Int ctx",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Processor:\n"
        "  .decl (Widget w, Button b):: handle: Int x -> result",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .decl ?tryProcess: Int x -> result\n"
        "  .decl !mustFail: String s -> output",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Resource:\n"
        "  .decl Resource r: String name\n"
        "  .decl @ Resource r: Int code",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Transaction:\n"
        "  .decl Transaction t: Int id\n"
        "  .decl @! Transaction t: String error",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Connection:\n"
        "  .decl Connection c: String host, Int port\n"
        "  .decl @ Connection c: Int timeout\n"
        "  .decl @! Connection c: String reason",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd doIt = process",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd doIt: Int x -> result = process: x",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .cmd add: Int item = (items):: append: item",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd process: String data -> result = validate: data\n     result <- (transform: data)",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd doIt = _",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd process: Int x -> result = _",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd ?tryProcess: String data -> result = _",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd !mustFail: Int code = _",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .decl Container c: Int capacity\n"
        "  .cmd add: Int item = (items):: append: item\n"
        "  .decl remove: Int index -> result",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .decl Handler h: String name\n"
        "  .cmd process: String data = validate: data\n"
        "  .cmd cleanup = _\n"
        "  .decl @ Handler h: Int code",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd Widget w: Int x, Int y = init: x, y",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .cmd Container c: Int capacity = allocate: capacity",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Resource:\n"
        "  .cmd Resource r: String name = acquire: name\n"
        "  .cmd @ Resource r: Int code = release: code",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Connection:\n"
        "  .cmd @ Connection c: Int timeout = disconnect\n    cleanup",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Transaction:\n"
        "  .cmd Transaction t: Int id = begin: id\n"
        "  .cmd @! Transaction t: String error = rollback: error",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Connection:\n"
        "  .cmd @! Connection c: String reason = logError: reason\n    notify",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd (Widget w):: render = draw: w",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .cmd (Container c):: process: Int x -> result = validate: x\n    result <- (transform: x)",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd (Widget w, Button b):: handle: Int x = (w):: update: x\n    (b):: click",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .cmd (Container c):: getSize -> result = (c):: computeSize",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd (Widget w):: getValue -> w = (w):: extract",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd (Handler h, Context ctx):: ?tryGet -> result = (h):: attempt: ctx",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process\n"
        "  .cmd ?tryIt: Int x -> result = validate: x\n"
        "  .decl !mustFail: String s -> output\n"
        "  .cmd cleanup = _\n"
        "  .cmd @ Widget w: Int code = release: code\n"
        "  .decl @! Widget w: String error",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd Widget w: Int x = _\n"
        "  .cmd @ Widget w: Int code = _\n"
        "  .cmd @! Widget w: String error = _\n"
        "  .cmd (Widget w):: method = _\n"
        "  .cmd process: Int x -> result = _",
        Production::DEF_CLASS));
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class myClass:\n  .cmd doIt"));  // lowercase class name
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class MyClass"));  // missing colon and commands
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class MyClass:"));  // missing commands
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class : .cmd doIt"));  // missing class name
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class MyClass .cmd doIt"));  // missing colon
    CHECK_FALSE(testParse(grammar.DEF_CLASS, "class MyClass: .cmd doIt"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".Class MyClass: .cmd doIt"));  // wrong keyword case
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class MyClass:\n.cmd doIt"));  // command not indented
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class MyClass: .cmd doIt\n.cmd other"));  // second command not indented
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class List[T]:\n  .cmd add: T item -> result"));  // parameterized class name not allowed
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class Widget:\n  .cmd doIt ="));  // missing body after equals
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class Widget:\n  .cmd doIt _"));  // missing equals before body
    // Regular commands (without receivers) cannot have return value without parameters
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class Widget:\n  .cmd doIt -> result = process"));
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class Handler:\n  .cmd ?tryIt -> output = attempt"));
    CHECK_FALSE(testParse(grammar.DEF_CLASS, ".class Processor:\n  .cmd !mustFail -> result = fail"));
}

TEST_CASE("Grammar2::test instance declaration parsing") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_INSTANCE_NAME, "String", Production::DEF_INSTANCE_NAME));
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_NAME, "string"));  // lowercase

    CHECK(testParse(grammar.DEF_INSTANCE_TYPES, "Interface", Production::DEF_INSTANCE_TYPES));
    CHECK(testParse(grammar.DEF_INSTANCE_TYPES, "Comparable, Serializable, Hashable", Production::DEF_INSTANCE_TYPES));
    CHECK(testParse(grammar.DEF_INSTANCE_TYPES, "Interface(delegate)", Production::DEF_INSTANCE_TYPES));
    CHECK(testParse(grammar.DEF_INSTANCE_TYPES, "Interface1(field1), Interface2(field2), Interface3(field3)", Production::DEF_INSTANCE_TYPES));
    CHECK(testParse(grammar.DEF_INSTANCE_TYPES, "Comparable(cmp), Serializable, Hashable(hash)", Production::DEF_INSTANCE_TYPES));
    CHECK(testParse(grammar.DEF_INSTANCE_DELEGATE, "(field)", Production::DEF_INSTANCE_DELEGATE));
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "(Field)"));  // typename not allowed
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "field"));  // missing parens
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "(field"));  // missing closing paren
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "field)"));  // missing opening paren
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "()"));  // empty parens
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_DELEGATE, "(field1, field2)"));  // multiple identifiers
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "interface"));  // lowercase
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "Interface1, interface2"));  // lowercase in list
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "Interface(Field)"));  // typename in delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "Interface()"));  // empty delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "Interface(field1, field2)"));  // multiple fields in delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE_TYPES, "Interface(123)"));  // number in delegate

    CHECK(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface1, Interface2", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance MyType:\n Interface", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface1,\n Interface2", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface(delegate)", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance Widget: Clickable(handler)", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance Container: Iterable, Serializable(writer)", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance Proxy: Interface1(field1), Interface2(field2), Interface3(field3)", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance Wrapper:\n Comparable(cmp)", Production::DEF_INSTANCE));
    CHECK(testParse(grammar.DEF_INSTANCE, ".instance Adapter: Interface1(field1),\n Interface2(field2)", Production::DEF_INSTANCE));
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance myType: Interface"));  // lowercase type name
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: interface"));  // lowercase interface name
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType"));  // missing colon and interfaces
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType:"));  // missing interfaces
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance : Interface"));  // missing type name
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType Interface"));  // missing colon
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, "instance MyType: Interface"));  // missing dot
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".Instance MyType: Interface"));  // wrong keyword case
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface,"));  // trailing comma
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface1 Interface2"));  // missing comma
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface(Field)"));  // typename in delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface()"));  // empty delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface(field1, field2)"));  // multiple fields in delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface(123)"));  // number in delegate
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface delegate"));  // missing parens
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface(delegate"));  // missing closing paren
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType: Interface delegate)"));  // missing opening paren
    CHECK_FALSE(testParse(grammar.DEF_INSTANCE, ".instance MyType(delegate): Interface"));  // delegate on type name not allowed
}

TEST_CASE("Grammar2::test full command definitions with bodies") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_CMD, ".cmd Container c: Int size = Container: size", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Widget w: String error = logError: error", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Connection c: String msg, Int code = handleFailure: #msg, code", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Resource r: String name, Int id = doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Widget w: String error = doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Transaction t: Int code, String msg = doIt", Production::DEF_CMD));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @ (Widget w):: Int x = cleanup: x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @! (Widget w):: String error = logError: error"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @ doIt: Int x = cleanup: x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @! doIt: String error = logError: error"));
}

TEST_CASE("Grammar2::comprehensive DEF_CMD with all body syntax variations") {
    Grammar2& grammar = getGrammar();

    // Single command definition containing all permissible syntax variations in its body
    CHECK(debugTestParse(grammar.DEF_CMD,
        ".cmd process: Int x, String y -> result / ^Context ctx = "
        " init: x\n"
        " Widget: x, y\n"
        " Container[Int]: x\n"
        " Std::Collections::List[String]: y\n"
        " (w):: method\n"
        " (w, b):: combine: x, y\n"
        " simple\n"
        " Qualified::command: x\n"
        " withParams: x, y\n"
        " ? mayfail: x\n"
        " !mustSucceed: y\n"
        " #temp <- getValue: x\n"
        " result <- compute: x, y\n"
        " #alloc <- allocate\n"
        " temp <- (obj):: process: x\n"
        " value <- Widget: x, y\n"
        " data <- Std::String: y\n"
        " handler <- {process: x}\n"
        " callback <- {Widget: x, y}\n"
        " quoted <- {(obj):: method}\n"
        " cmd <- :<Int 'a> {doIt}\n"
        " process: _\n"
        " handle: x, _, y\n"
        " sum <- x + y\n"
        " product <- x * y\n"
        " expr <- (x + y) * result\n"
        " ptr <- value^\n"
        " addr <- value&\n"
        " item <- array[x]\n"
        " elem <- matrix[x, y]\n"
        " literal <- 42\n"
        " text <- \"hello\"\n"
        " hex <- 0x1234_5678\n"
        " enumVal <- Status[ok]\n"
        " nested <- (((x + y)))\n"
        " $handler: x\n"
        " $ @ cleanup: x\n"
        " $ @ identifier\n"
        " $@!onFail: x\n"
        " $ {process: x}: y\n"
        " process: ($handler)\n"
        " ? validate: x\n"
        "   call: x\n"
        " - fallback: y\n"
        " ?? multiCheck: x\n"
        " ! unless: y\n"
        " a <- :{ % doIt }\n"
        " % block\n"
        "   call: x,y\n"
        "   % nested: a,b\n"
        "     nested: c,d\n"
        " ^ rewind\n"
        " | recover: error\n"
        " | Error e> handleError: e\n"
        " @ cleanup: resource\n"
        " execute: { doIt }\n"
        " @! onFailure: error\n"
        , Production::DEF_CMD));
}

TEST_CASE("Grammar2::test CALL_IDENTIFIER") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_IDENTIFIER, "#temp", Production::ALLOC_IDENTIFIER));
    CHECK_FALSE(testParse(grammar.CALL_IDENTIFIER, "result"));  // uppercase not allowed
    CHECK_FALSE(testParse(grammar.CALL_IDENTIFIER, "Widget"));  // typename not allowed
}

TEST_CASE("Grammar2::test CALL_PARAMETERS") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_PARAMETER, "x", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "value", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "#temp", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "(Widget: x, y)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "(process: item)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "((obj):: method: x)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "($handler: x)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "($callback)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "(${doIt}: data)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "(${Widget: x, y})", Production::CALL_PARAMETER));
}

TEST_CASE("Grammar2::test CALL_CONSTRUCTOR") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: x, y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container: size", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Point: x, y, z", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "List[Int]: item", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Map[String, Int]: key, value", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: #x, #y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: (Point: x, y), z", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "UI::Widget: x, y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Std::Collections::List[T]: item", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Graphics::Point: x, y, z", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "A::B::C::Type: value", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "UI::Widget: (Graphics::Point: x, y), z", Production::CALL_CONSTRUCTOR));
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget: X"));  // uppercase identifier
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "widget: x"));  // lowercase typename
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget x"));  // missing colon
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget:"));  // missing parameters
}

TEST_CASE("Grammar2::test CALL_COMMAND") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.CALL_COMMAND, "doSomething: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: item", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "calculate: a, #b, c", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "doIt", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: #temp, value", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: (Widget: x, y), z", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "$handler: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "$callback", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "$processor: item", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "${doIt}: x", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "${Widget: x, y}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "${(obj):: method}: data", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "Module::function: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "Std::IO::println: message", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "A::B::C::method", Production::CALL_COMMAND));
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "DoSomething: x"));  // uppercase command name
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "Widget: x"));  // typename not allowed
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "doSomething: x y", Production::CALL_COMMAND));
}

TEST_CASE("Grammar2::test CALL_VCOMMAND") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: x, y", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(widget):: process: item", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b):: handle: item", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(x, y, z):: process: data", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: (Widget: x, y)", Production::CALL_VCOMMAND));
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(#obj):: method: x", Production::CALL_VCOMMAND));
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(a, #b):: handle: item"));
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(Obj):: method: x"));  // uppercase receiver
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(obj):: Method: x"));  // uppercase method name
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "obj: method: x"));  // single colon instead of double
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - dereference operator") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "myPtr^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "42^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "3.14^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "\"hello\"^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(x + y)^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((doIt))^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(Widget: x, y)^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^ + y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + ptr^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr1^ * ptr2^", Production::CALL_EXPRESSION));
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - address-of operator") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "x&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "myVar&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "42&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "3.14&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "\"hello\"&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(x + y)&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((doIt))&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(Widget: x, y)&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x& + y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + b&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x& * y&", Production::CALL_EXPRESSION));
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - array indexing") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "data[i]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "items[index]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "myArray[42]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "matrix[i, j]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "grid[0, 0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "table[row, col]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[0][1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data[i][j]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "cube[x][y][z]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "nested[0][1][2][3]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[i, j][k]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data[0][i, j]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "complex[a, b][c][d, e]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[i + 1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data[x * 2]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "items[a + b, c - d]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "42[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "\"text\"[i]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(x + y)[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(doIt)[i]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(Widget: x, y)[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a + b) * c)[index]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[0] + arr[1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data[i] * data[j]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x + arr[i]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "matrix[i, j] - offset", Production::CALL_EXPRESSION));
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - combined operations") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[i][j][k] + data[x][y]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "matrix[0, 0] * matrix[1, 1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[i + j]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data[x * 2, y / 2]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "items[(a + b)]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[0]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^[i][j]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^[0]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^[i, j]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "data^[0][1][2]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^ + offset", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr^[i] * scale", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "ptr1^[0] + ptr2^[1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr^&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr[0]^",Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "arr^^", Production::CALL_EXPRESSION));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr&[0]"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr&&"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr[]"));  // empty index
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr["));  // missing closing bracket
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr]"));  // missing opening bracket
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr[0, 1, 2]"));  // too many dimensions
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "arr[,]"));  // missing expressions
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - ordering rules") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "x^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^[0]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x[0]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^[0]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^[0][1]", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^[0][1]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^[i, j][k]&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x[0]^", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x[0]^&", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x^^", Production::CALL_EXPRESSION));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "x&[0]"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "x&^"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "x&&"));
}

TEST_CASE("Grammar2::test CALL_EXPR_SUFFIX - in assignments") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_ASSIGNMENT, "addr <- (data)[2]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "addr <- data&", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "ref <- (x + y)&", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- arr[0]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "element <- data[i]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "cell <- matrix[i, j]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "item <- nested[0][1][2]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- arr[i] + arr[j]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "sum <- data[0] * data[1]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "ptr <- (x + y)&", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "val <- ptr^", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "item <- ptr^[0]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "element <- ptr^[i, j]", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "addr <- ptr^[0]&", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- ptr1^ + ptr2^", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "sum <- ptr^[0] * ptr^[1]", Production::CALL_ASSIGNMENT));
}

TEST_CASE("Grammar2::test CALL_EXPRESSION") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, "((Widget: x, y))", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(List[Int]: item)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(process: item)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(doIt)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "someVariable", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((obj):: method: x)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a, b):: handle: item)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(process: (Widget: x, y))", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "($handler: x)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "($callback)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(${doIt}: data)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(${Widget: x, y})", Production::CALL_EXPRESSION));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "(Widget: x"));  // missing closing paren
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "Widget: x)"));  // missing opening paren
}

TEST_CASE("Grammar2::test CALL_CMD_LITERAL") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x>{Widget: x, y}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<String name>{Container: name}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<Int value>{List[Int]: value}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<String data>{Map[String, Int]: data, count}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<Int count>{Array[8]: count}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x>{process: x}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<String name>{doIt}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<Int value>{calculate: value, value}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<String data>{validate: data}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x>{(obj):: method: x}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<String name>{(a, b):: handle: name}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<Int value>{(widget):: process}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x, String y>{Widget: x, y}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int a, Int b, Int c>{calculate: a, b, c}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<String name, Int count>{process: name, count}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<Int x, String y, Int z>{(obj):: method: x, y, z}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<^Int ptr>{process: ptr}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<[]Int items>{calculate: items}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<[8]Int buffer>{validate: buffer}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<List[T] list>{Widget: list}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<^[]Int data>{process: data}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<Map[String, Int] map>{doIt}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<(T: Int) value>{process: value}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<(T: String) data>{Widget: data}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<(U: List[T]) items>{calculate: items}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x, ^String ptr, []Int items>{process: x, ptr, items}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<String name, (T: Int) value>{Widget: name, value}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<List[T] list, Int count, ^Int ptr>{(obj):: method: list, count, ptr}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- :<Int x>{Widget: x, y}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "handler <- ?<String name>{process: name}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#temp <- !<Int a, Int b>{calculate: a, b}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "callback <- :<^Int ptr, String name>{(obj):: method: ptr, name}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- ?<List[T] items>{doIt}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_COMMAND, "process: :<Int x>{Widget: x, y}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "execute: ?<String name>{doIt}, context", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "handle: !<Int a, Int b>{calculate: a, b}, :<String s>{validate: s}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: ?<Int x>{process: x}, z", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container: :<Int a, String b>{Widget: a, b}, !<Int c>{doIt}", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: !<String name>{process: name}", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b):: handle: :<Int x, Int y>{Widget: x, y}, ?<String s>{validate: s}", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: (:<Int x>{Widget: x, y})", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: (?<String name>{doIt}), (!<Int value>{process: value})", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- (:<Int a, Int b>{calculate: a, b})", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- :<Int x>{Widget: x, y} + value", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "sum <- ?<String s>{process: s} * count", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- !<Int a, Int b>{calculate: a, b} - offset", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- :<Int x>{Widget: x, y} | ?<String s>{process: s}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- !<Int a>{doIt} | :<String s>{validate: s} | ?<Int x>{finalize}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<>{Widget: x, y}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "?<>{process: data}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "!<>{doIt}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- :<>{(obj):: method: x}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_EXPRESSION, ":<Int x>{%process: x}", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION,
        "?<String name, Int count>{%doIt\n"
                "                           cleanup}", Production::CALL_EXPRESSION));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int x>Widget: x, y"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int x{Widget: x, y}"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int x>{Widget: x, y"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int x>Widget: x, y}"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int>{Widget: x, y}"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<x>{Widget: x, y}"));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, ":<Int X>{Widget: x, y}"));
}


TEST_CASE("Grammar2::test CALL_ASSIGNMENT") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- Widget: x, y", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- (Widget: x, y)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "container <- (List[Int]: item)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (process: item)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- fibx", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "data <- ((obj):: method: x)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- ((a, b):: handle: item)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "widget <- UI::Widget: x, y", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "list <- Std::Collections::List[T]: item", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- Module::function: param", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- A::B::C::process: data", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "handler <- ($callback)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- ($processor: data)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (${doIt}: x, y)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- (${Widget: x, y})", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- (Widget: x, y)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (process: item)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#output <- (process: item)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#data <- ((obj):: method: x)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#result <- (Widget: x, y)", Production::CALL_ASSIGNMENT));
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "Result <- (Widget: x)"));  // uppercase target
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result -> (Widget: x)"));  // wrong arrow
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result Widget: x"));  // missing arrow
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "<- (Widget: x)"));  // missing target
}

TEST_CASE("Grammar2::test CALL_INVOKE") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_INVOKE, "Widget: x, y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_INVOKE, "List[Int]: item", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_INVOKE, "doSomething: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_INVOKE, "process", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_INVOKE, "(obj):: method: x, y", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_INVOKE, "(a, b):: handle", Production::CALL_VCOMMAND));
}

TEST_CASE("Grammar2::test CALL_GROUP") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_GROUP, "doIt", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "Widget: x, y", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "(obj):: method: x", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "doIt\nprocess: x", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "doIt\nprocess: x", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "Widget: x, y \ndoIt", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "init\nprocess: data\ncleanup", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "doIt\nprocess: x", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "Widget: x, y\ndoIt\ncleanup", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "Widget: x\ndoIt\n(obj):: method: y", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "$handler: x\n$callback", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "${doIt}\n${process: data}", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "init\n$handler: x\ncleanup", Production::CALL_GROUP));
}

TEST_CASE("Grammar2::test BLOCK_HEADER") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.BLOCK_HEADER, "?", Production::BLOCK_HEADER));  // DO_WHEN
    CHECK(testParse(grammar.BLOCK_HEADER, "??", Production::BLOCK_HEADER));  // DO_WHEN_MULTI
    CHECK(testParse(grammar.BLOCK_HEADER, "?-", Production::BLOCK_HEADER));  // DO_WHEN_FAIL
    CHECK(testParse(grammar.BLOCK_HEADER, "!", Production::BLOCK_HEADER));  // DO_UNLESS
    CHECK(testParse(grammar.BLOCK_HEADER, "-", Production::BLOCK_HEADER));  // DO_ELSE
    CHECK(testParse(grammar.BLOCK_HEADER, "%", Production::BLOCK_HEADER));  // DO_BLOCK
    CHECK(testParse(grammar.BLOCK_HEADER, "^", Production::BLOCK_HEADER));  // DO_REWIND
    CHECK(testParse(grammar.BLOCK_HEADER, "|", Production::BLOCK_HEADER));  // DO_RECOVER
    CHECK(testParse(grammar.BLOCK_HEADER, "@", Production::BLOCK_HEADER));  // ON_EXIT
    CHECK(testParse(grammar.BLOCK_HEADER, "@!", Production::BLOCK_HEADER));  // ON_EXIT_FAIL
}

TEST_CASE("Grammar2::test BLOCK") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.BLOCK, "? doIt", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "?? process: x", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "?- cleanup", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "! abort", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "- fallback", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "% execute", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "^ retry", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| recover", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| Failtype f> recover", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| Failtype[T] f> recover", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| \"fish\"> recover", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| FailTypes[error] > recover", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "@ cleanup: resource", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "@! handleFailure: error", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "? doIt\n process: x", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "% init\n process: data\n cleanup", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "? Widget: x, y", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "% Container: size\n doIt", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "? (obj):: method: x", Production::DO_BLOCK));
    CHECK(testParse(grammar.BLOCK, "| (a, b):: recover: error", Production::DO_BLOCK));
    CHECK_FALSE(testParse(grammar.BLOCK, "% init\nprocess: data\ncleanup"));
    CHECK_FALSE(testParse(grammar.BLOCK, "% Container: size\ndoIt"));
}

TEST_CASE("Grammar2::test DEF_CMD_BODY") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.DEF_CMD_BODY, "= doIt", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= process: x", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= Widget: x, y", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= List[Int]: item", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= (obj):: method: x", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= (a, b):: handle: item", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n process: x", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= Widget: x, y\ndoIt", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\nprocess: data\ncleanup", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= ? doIt", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n? process: x", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= % execute\n    cleanup", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= doIt\n| recover: error", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= process: x\n@ cleanup: resource", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n? Widget: x, y\n- fallback", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= % doIt\nprocess: x\n| recover", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= _", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_EMPTY, "_", Production::DEF_CMD_EMPTY));
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "Widget: x"));  // missing equals
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "= "));  // missing call
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "doIt"));  // missing equals
}

TEST_CASE("Grammar2::test command body integration - complex scenarios") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: (Point: x, y), (Size: w, h)", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: (transform: data), (validate: input)", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: (Widget: x, y), z", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: #x, #y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: #temp, value", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_GROUP, "Widget: x, y\n(obj):: init\nprocess: data", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "Container: size\ndoIt\n(obj):: method: x\ncleanup", Production::CALL_GROUP));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n? doIt\n- fallback", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= % process: x\n? validate\n- error", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= doIt\n| recover\n@ cleanup", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= ? Widget: x, y\n(obj):: init\n- Container: size\nfallback", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n? process: data\n- error: msg\n@ cleanup: resource", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Map[String, List[Int]]: key, values", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container[Widget[T]]: (Widget[Int]: x)", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_GROUP, "result <- (Widget: x, y)\nprocess: result", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_GROUP, "#temp <- doIt\nprocess: #temp", Production::CALL_GROUP));
    CHECK(testParse(grammar.CALL_COMMAND, "$handler: ($processor: data)", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "${Widget: x, y}: result", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Handler: ($callback)", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Processor: (${doIt}: x)", Production::CALL_CONSTRUCTOR));
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(#obj, #widget):: method: #data", Production::CALL_VCOMMAND));
}

TEST_CASE("Grammar2::test command body edge cases") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_COMMAND, "doIt", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: x", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: x", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: x", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: a, b, c, d, e", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: a, b, c, d, e", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: a, b, c, d, e", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b, c, d):: method: x", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_PARAMETER, "(Widget: (Point: (Value: x)))", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: (Container: (List[Int]: (Value: x)))", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: x, #temp, y, #output, z", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "$handler: a, b, c, d, e", Production::CALL_COMMAND));
    // n.b. these two are syntactically valid but semantically wrong as stack effect commands cannot accept arguments
    CHECK(testParse(grammar.CALL_COMMAND, "$@handler: a, b, c, d, e", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "$@!handler: a, b, c, d, e", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "${process: data}: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_PARAMETER, "($handler)", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_PARAMETER, "(${doIt})", Production::CALL_PARAMETER));
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(a, #b, c):: method: x, #y", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\n? success\n- failure\n| recover\n@ cleanup\n@! onFail", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.CALL_GROUP, "doIt", Production::CALL_GROUP));
    CHECK(testParse(grammar.BLOCK, "? doIt", Production::DO_BLOCK));
}

TEST_CASE("Grammar2::test command body negative cases") {
    Grammar2& grammar = getGrammar();

    CHECK_FALSE(testParse(grammar.CALL_IDENTIFIER, "Widget"));  // typename
    CHECK_FALSE(testParse(grammar.CALL_IDENTIFIER, "123"));  // number
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "widget: x"));  // lowercase typename
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget"));  // missing colon and params
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget:"));  // missing params
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, ": x"));  // missing typename
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "DoIt"));  // uppercase
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "Widget: x"));  // typename not allowed
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(Obj):: method"));  // uppercase receiver
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "(obj):: Method"));  // uppercase method
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "obj: method"));  // single colon
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, ":: method"));  // missing receiver
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "obj::"));  // missing method
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "Result <- doIt"));  // uppercase target
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result -> doIt"));  // wrong arrow
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result doIt"));  // missing arrow
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "<- doIt"));  // missing target
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result <-"));  // missing source
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "(doIt"));  // missing close paren
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "doIt)"));  // missing open paren
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "()"));  // empty expression
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "doIt"));  // missing equals
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "="));  // missing call group
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "= "));  // missing call group (whitespace only)
}

TEST_CASE("Grammar2::test CALL_PARM_EMPTY and CALL_QUOTE") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_PARAMETER, "_", Production::CALL_PARAMETER));
    CHECK(testParse(grammar.CALL_QUOTE, "{Widget: x, y}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{Container: size}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{List[Int]: item}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{Map[String, Int]: key, value}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{doIt}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{process: item}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{calculate: a, b, c}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{(obj):: method: x}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{(a, b):: handle: item}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_QUOTE, "{(widget):: process}", Production::CALL_QUOTE));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: {Point: x, y}, z", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container: {doIt}, size", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Handler: {process: data}", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: {Widget: x, y}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "execute: {doIt}, context", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "handle: {(obj):: method: x}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: {Widget: x, y}", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b):: handle: {process: item}, data", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: _, y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: x, _", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: _, _", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container: _, size, _", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: _", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: _, item", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: item, _", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "calculate: _, _, result", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: _", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: _, x", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b):: handle: x, _, z", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: {Point: x, y}, _", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: _, {doIt}", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: _, {Widget: x, y}, data", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: {process: item}, _", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- {Widget: x, y}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "handler <- {process: data}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "handler <- ((({process: data})))", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "callback <- {(obj):: method: x}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#temp <- {doIt}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= process: {Widget: x, y}", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= handler <- {doIt}", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= execute: {process: data}, context", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\nprocess: {Widget: x, y}\ncleanup", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= process: _", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= Widget: _, y", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= init\nprocess: _, data\ncleanup", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= process: {Widget: _, y}, data", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= handler <- {process: _, item}", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= execute: _, {doIt}, _", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= ? process: {Widget: x, y}\n- fallback: _", Production::DEF_CMD_BODY));
    CHECK_FALSE(testParse(grammar.CALL_QUOTE, "Widget: x, y"));  // missing braces
    CHECK_FALSE(testParse(grammar.CALL_QUOTE, "{Widget: x, y"));  // missing closing brace
    CHECK_FALSE(testParse(grammar.CALL_QUOTE, "Widget: x, y}"));  // missing opening brace
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "handler <- ((({process: data}))"));
}

TEST_CASE("Grammar2::test CALL_OPERATOR") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_OPERATOR, "+", Production::CALL_OPERATOR));
    CHECK(testParse(grammar.CALL_OPERATOR, "-", Production::CALL_OPERATOR));
    CHECK(testParse(grammar.CALL_OPERATOR, "*", Production::CALL_OPERATOR));
    CHECK(testParse(grammar.CALL_OPERATOR, "/", Production::CALL_OPERATOR));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + b", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x - y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "m * n", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "p / q", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + b + c", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x - y - z", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a * b * c * d", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "p / q / r", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + b * c", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x - y / z", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a * b + c * d", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "p / q - r / s", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + b - c * d / e", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(a + b) * c", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "x * (y + z)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(a + b) * (c - d)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a + b) * c) / d", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(x) + (y) * (z)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a + b)) * c", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(((x))) + y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a + (b * c))) / d", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(Widget: x, y) + z", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + (process: item)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(doIt) * (calculate: x)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((obj):: method: x) + y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a + ((b, c):: handle: item)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "Module::value + x", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "a * Std::Math::pi", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "Module::function: x, y", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(UI::Widget: x, y) + offset", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- a + b", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- x - y", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "product <- m * n", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#quotient <- p / q", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "combined <- a | b", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- f:a,b | g:b,c | 0", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "sum <- a + b + c", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "diff <- x - y - z", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- a * b / c + d", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- p + q * r - s / t", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- (Widget: x, y) + z", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- (process: item) * factor", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (doIt) + offset", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "data <- ((obj):: method: x) - base", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- (a + b) * c", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- x + (y * z)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (a + b) / (c - d)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- a + (Widget: x, y)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "value <- x * (process: item) + y", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "output <- (doIt) + (calculate: x) * z", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "handler <- {Widget: x, y} + offset", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "callback <- base + {process: item}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- {doIt} * {calculate: x}", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#temp <- a + b", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#result <- x * y + z", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#value <- (Widget: x, y) + offset", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_EXPRESSION, "((a + b) * (c - d)) / ((e + f) * g)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "(Widget: x, y) + (process: a) * (calculate: b)", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- ((a + b) * c) + ((d - e) / f)", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_EXPRESSION, "width + height", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_EXPRESSION, "total - discount + tax", Production::CALL_EXPRESSION));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "area <- width * height", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "average <- sum / count", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= result <- a + b", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= value <- x * y + z", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= output <- (Widget: x, y) + offset", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= sum <- a + b\nproduct <- x * y", Production::DEF_CMD_BODY));
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "+ a"));  // prefix operator not supported
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "- a"));  // prefix operator not supported
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "a +"));  // missing right operand
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "a + + b"));  // consecutive operators
    CHECK_FALSE(testParse(grammar.CALL_EXPRESSION, "a b + c"));  // missing operator between a and b
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result <- + a"));  // prefix operator
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result <- a +"));  // missing right operand
}

TEST_CASE("Grammar2::test CALL_BLOCKQUOTE") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{doIt}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{process: x}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{Widget: x, y}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{init\n process: data\n cleanup}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{result <- a + b\n }", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{(obj):: method: x, y}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{doIt}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{process: x}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{Widget: x, y}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{init\n process: data\n cleanup}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{result <- a + b\n }", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{(obj):: method: x, y}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{doIt}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{process: x}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{Widget: x, y}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{init\n process: data\n cleanup}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{result <- a + b}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{(obj):: method: x, y}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{? doIt\n - fallback}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{% init\n  process: x}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{| recover: error}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{result <- process: x\n value <- calculate: y}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{Widget: x, y\n process: widget}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{init\n (obj):: method: x\n cleanup}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, ":{_}", Production::CALL_BLOCK_NOFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "?{_}", Production::CALL_BLOCK_MAYFAIL));
    CHECK(testParse(grammar.CALL_BLOCKQUOTE, "!{_}", Production::CALL_BLOCK_FAIL));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: :{doIt}", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Container: !{process: x}, size", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Handler: ?{Widget: x, y}", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_COMMAND, "process: :{init\n"
                                                  "           cleanup}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "execute: !{doIt}, context", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "handle: ?{(obj):: method: x}", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(obj):: method: :{Widget: x, y}", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "(a, b):: handle: !{process: item}, data", Production::CALL_VCOMMAND));
    CHECK_FALSE(testParse(grammar.CALL_BLOCKQUOTE, ":{doIt"));
    CHECK_FALSE(testParse(grammar.CALL_BLOCKQUOTE, "!{process: x"));
    CHECK_FALSE(testParse(grammar.CALL_BLOCKQUOTE, "?{Widget: x, y"));
    CHECK_FALSE(testParse(grammar.CALL_BLOCKQUOTE, "{doIt}"));  // regular braces, not blockquote
    CHECK_FALSE(testParse(grammar.CALL_BLOCKQUOTE, "doIt}"));  // missing opening
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - empty and minimal") {
    Grammar2& grammar = getGrammar();

    // Empty compilation unit (all parts optional)
    CHECK(testParse(grammar.COMPILATION_UNIT, "", Production::COMPILATION_UNIT));

    // Just a module
    CHECK(testParse(grammar.COMPILATION_UNIT, ".module MyModule", Production::COMPILATION_UNIT));
    CHECK(testParse(grammar.COMPILATION_UNIT, ".module Std::Collections", Production::COMPILATION_UNIT));

    // Just imports
    CHECK(testParse(grammar.COMPILATION_UNIT, ".import \"file.basis\"", Production::COMPILATION_UNIT));
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".import \"file1.basis\"\n"
        ".import \"file2.basis\"",
        Production::COMPILATION_UNIT));
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".import Module1\n"
        ".import Module2\n"
        ".import Std:Core",
        Production::COMPILATION_UNIT));

    // Just definitions
    CHECK(testParse(grammar.COMPILATION_UNIT, ".alias MyInt: Int", Production::COMPILATION_UNIT));
    CHECK(testParse(grammar.COMPILATION_UNIT, ".domain UserId: Int", Production::COMPILATION_UNIT));
    CHECK(testParse(grammar.COMPILATION_UNIT, ".enum Fish: sockeye = 0, salmon = 1", Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - module and imports") {
    Grammar2& grammar = getGrammar();

    // Module followed by imports
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"file1.basis\"\n"
        ".import \"file2.basis\"\n"
        ".import Std:Core",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Std::Collections\n"
        ".import Std:Core\n"
        ".import Std:Memory",
        Production::COMPILATION_UNIT));

    // Module with indented imports (boundedGroup rule)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"file1.basis\"\n"
        ".import \"file2.basis\"",
        Production::COMPILATION_UNIT));

    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        "  .import \"file1.basis\"\n"
        "  .import \"file2.basis\""));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - module, imports, and definitions") {
    Grammar2& grammar = getGrammar();

    // Module, imports, and a single definition
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import Std:Core\n"
        ".domain UserId: Int",
        Production::COMPILATION_UNIT));

    // Module, imports, and multiple definitions
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - all definition types") {
    Grammar2& grammar = getGrammar();

    // Test with all types of definitions
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"base.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".record Point: Int x, Int y\n"
        ".object Node: Int value, ^Node next\n"
        ".instance MyType: Interface\n"
        ".test \"simple test\" = doSomething\n"
        ".program = main",
        Production::COMPILATION_UNIT));

    // Test with class containing commands
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import Std:Core\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process",
        Production::COMPILATION_UNIT));

    // Test with standalone command definitions
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - imports and definitions without module") {
    Grammar2& grammar = getGrammar();

    // Imports followed by definitions (no module)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".import Std:Core\n"
        ".import Std:IO\n"
        ".domain UserId: Int\n"
        ".record User: UserId id, String name",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".import \"base.basis\"\n"
        ".enum Color: red = 0, green = 1, blue = 2\n"
        ".alias ColorCode: Int\n"
        ".test \"color test\" = validateColor",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - complex multi-definition files") {
    Grammar2& grammar = getGrammar();

    // Multiple records and objects
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Graphics\n"
        ".import Std:Math\n"
        ".record Point: Int x, Int y\n"
        ".record Vector: Int dx, Int dy\n"
        ".object Shape: Point origin, Int size\n"
        ".object Circle: Point center, Int radius",
        Production::COMPILATION_UNIT));

    // Multiple enums and aliases
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Types\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".enum Priority: low = 0, medium = 1, high = 2\n"
        ".alias StatusCode: Int\n"
        ".alias PriorityLevel: Int",
        Production::COMPILATION_UNIT));

    // Mix of all definition types in order
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyApp\n"
        ".import \"base.basis\"\n"
        ".import Std:Core\n"
        ".alias UserId: Int\n"
        ".domain SessionId: Int\n"
        ".enum Role: admin = 0, user = 1\n"
        ".record User: UserId id, String name\n"
        ".object Session: SessionId id, User user\n"
        ".class UserManager:\n"
        "    .decl create: User u -> result\n"
        ".instance User: Serializable\n"
        ".cmd authenticate: String username -> result = validate: username\n"
        ".decl logout: SessionId sid\n"
        ".intrinsic hashPassword: String pwd\n"
        ".test \"user creation\" = testCreate\n"
        ".program = main",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - definitions only (no module or imports)") {
    Grammar2& grammar = getGrammar();

    // Multiple definitions without module or imports
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".record Point: Int x, Int y\n"
        ".object Node: Int value, ^Node next\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x",
        Production::COMPILATION_UNIT));

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".test \"test\" = run\n"
        ".program = main",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - indentation variations") {
    Grammar2& grammar = getGrammar();

    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        Production::COMPILATION_UNIT));

    // Definitions indented more than module
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        "    .import \"utils.basis\"\n"
        "    .alias MyInt: Int\n"
        "    .domain UserId: Int"));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - with complex class definitions") {
    Grammar2& grammar = getGrammar();

    // Class with multiple commands and declarations
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x, Int y\n"
        "    .cmd doIt = process\n"
        "    .cmd render = draw\n"
        "    .decl update: Int delta\n"
        "    .cmd @ Widget w: Int code = cleanup: code",
        Production::COMPILATION_UNIT));

    // Multiple classes
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module UI\n"
        ".import Graphics:Core\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x, Int y\n"
        ".class Button:\n"
        "    .decl Button b: String label\n"
        "    .cmd click = handleClick\n"
        ".class Container:\n"
        "    .decl Container c: Int capacity",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - with test and program definitions") {
    Grammar2& grammar = getGrammar();

    // Multiple tests
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Tests\n"
        ".test \"test 1\" = run1\n"
        ".test \"test 2\" = run2\n"
        ".test \"test 3\" = run3",
        Production::COMPILATION_UNIT));

    // Tests with other definitions
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyApp\n"
        ".alias MyInt: Int\n"
        ".cmd helper: Int x -> result = process: x\n"
        ".test \"helper test\" = testHelper: 42\n"
        ".program = main",
        Production::COMPILATION_UNIT));

    // Program at the end
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyApp\n"
        ".import Std:Core\n"
        ".cmd initialize = setup\n"
        ".cmd run: []String args = execute: args\n"
        ".program = run: args",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - order variations") {
    Grammar2& grammar = getGrammar();

    // Definitions can appear in any order (except module must be first, imports before definitions)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"base.basis\"\n"
        ".enum Status: active = 0\n"
        ".alias MyInt: Int\n"
        ".record Point: Int x, Int y\n"
        ".domain UserId: Int\n"
        ".object Node: Int value",
        Production::COMPILATION_UNIT));

    // Commands and declarations mixed
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".cmd doIt = process\n"
        ".decl helper: Int x\n"
        ".intrinsic native: String s\n"
        ".cmd other = run\n"
        ".decl another: Int y",
        Production::COMPILATION_UNIT));

    // Classes, instances, tests mixed
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x\n"
        ".instance Widget: Interface\n"
        ".test \"test\" = run\n"
        ".class Button:\n"
        "    .decl Button b: String label\n"
        ".test \"another test\" = run2",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - single definition types") {
    Grammar2& grammar = getGrammar();

    // Just one alias
    CHECK(testParse(grammar.COMPILATION_UNIT, ".alias MyInt: Int", Production::COMPILATION_UNIT));

    // Just one class
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".class Widget:\n"
        "  .decl Widget w: Int x",
        Production::COMPILATION_UNIT));

    // Just one command
    CHECK(testParse(grammar.COMPILATION_UNIT, ".cmd doIt = process", Production::COMPILATION_UNIT));

    // Just one declaration
    CHECK(testParse(grammar.COMPILATION_UNIT, ".decl helper: Int x -> result", Production::COMPILATION_UNIT));

    // Just one intrinsic
    CHECK(testParse(grammar.COMPILATION_UNIT, ".intrinsic native: String s", Production::COMPILATION_UNIT));

    // Just one domain
    CHECK(testParse(grammar.COMPILATION_UNIT, ".domain UserId: Int", Production::COMPILATION_UNIT));

    // Just one enum
    CHECK(testParse(grammar.COMPILATION_UNIT, ".enum Status: active = 0", Production::COMPILATION_UNIT));

    // Just one record
    CHECK(testParse(grammar.COMPILATION_UNIT, ".record Point: Int x, Int y", Production::COMPILATION_UNIT));

    // Just one object
    CHECK(testParse(grammar.COMPILATION_UNIT, ".object Node: Int value", Production::COMPILATION_UNIT));

    // Just one instance
    CHECK(testParse(grammar.COMPILATION_UNIT, ".instance MyType: Interface", Production::COMPILATION_UNIT));

    // Just one test
    CHECK(testParse(grammar.COMPILATION_UNIT, ".test \"test\" = run", Production::COMPILATION_UNIT));

    // Just one program
    CHECK(testParse(grammar.COMPILATION_UNIT, ".program = main", Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - module only variations") {
    Grammar2& grammar = getGrammar();

    // Simple module name
    CHECK(testParse(grammar.COMPILATION_UNIT, ".module MyModule", Production::COMPILATION_UNIT));

    // Qualified module name
    CHECK(testParse(grammar.COMPILATION_UNIT, ".module Std::Collections::List", Production::COMPILATION_UNIT));

    // Module with just imports (no definitions)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".import \"file.basis\"\n"
        ".import Std:Core",
        Production::COMPILATION_UNIT));

    // Module with just definitions (no imports)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - negative tests") {
    Grammar2& grammar = getGrammar();

    // Invalid: imports before module (module must be first if present)
    // Note: This might actually parse successfully since module is optional
    // The grammar allows imports without a module

    // Invalid: definitions before imports (imports must come before definitions)
    // Note: This might also parse since both are optional and can appear in any order
    // The grammar structure allows this flexibility

    // Invalid module syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".module myModule"));  // lowercase module name
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, "module MyModule"));  // missing dot
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".Module MyModule"));  // wrong case

    // Invalid import syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".import file.basis"));  // missing quotes
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, "import \"file.basis\""));  // missing dot
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".Import \"file.basis\""));  // wrong case

    // Invalid definition syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".alias myInt: Int"));  // lowercase type name
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".domain userId: Int"));  // lowercase domain name
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".enum status: active = 0"));  // lowercase enum name

    // Invalid class syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT,
        ".class widget:\n"
        "  .decl Widget w: Int x"));  // lowercase class name

    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT,
        ".class Widget:\n"
        ".decl Widget w: Int x"));  // declaration not indented

    // Invalid record/object syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".record point: Int x"));  // lowercase record name
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".object node: Int value"));  // lowercase object name

    // Invalid instance syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".instance myType: Interface"));  // lowercase type name

    // Invalid command syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".cmd DoIt = process"));  // uppercase command name
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".decl Helper: Int x"));  // uppercase declaration name

    // Invalid test syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".test test = run"));  // missing quotes

    // Invalid program syntax
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, "program = main"));  // missing dot
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT, ".Program = main"));  // wrong case
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - realistic file examples") {
    Grammar2& grammar = getGrammar();

    // Realistic utility module
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Std::Utils\n"
        ".import Std:Core\n"
        ".alias StringList: List[String]\n"
        ".cmd join: StringList items, String separator -> result = _\n"
        ".cmd split: String text, String delimiter -> result = _\n"
        ".test \"join test\" = testJoin\n"
        ".test \"split test\" = testSplit",
        Production::COMPILATION_UNIT));

    // Realistic data model module
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module App::Models\n"
        ".import Std:Core\n"
        ".domain UserId: Int\n"
        ".domain SessionId: Int\n"
        ".enum UserRole: admin = 0, user = 1, guest = 2\n"
        ".record User: UserId id, String name, String email, UserRole role\n"
        ".record Session: SessionId id, UserId userId, Int timestamp\n"
        ".object UserManager: List[User] users\n"
        ".instance User: Serializable, Comparable",
        Production::COMPILATION_UNIT));

    // Realistic service module with classes
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module App::Services\n"
        ".import App:Models\n"
        ".import Std:Core\n"
        ".class AuthService:\n"
        "    .decl AuthService s: String secret\n"
        "    .cmd login: String username, String password -> result = authenticate: username, password\n"
        "    .cmd logout: SessionId sid = invalidate: sid\n"
        "    .decl validateToken: String token -> result\n"
        ".class UserService:\n"
        "    .decl UserService s: UserManager manager\n"
        "    .cmd createUser: String name, String email -> result = (manager):: add: name, email\n"
        "    .cmd getUser: UserId id -> result = (manager):: find: id\n"
        ".test \"auth test\" = testAuth\n"
        ".test \"user test\" = testUser",
        Production::COMPILATION_UNIT));

    // Realistic main program module
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module App::Main\n"
        ".import App:Services\n"
        ".import App:Models\n"
        ".import Std:Core\n"
        ".import Std:IO\n"
        ".cmd initialize = setupDatabase\n    loadConfig\n"
        ".cmd run: []String args = initialize\n    startServer: args\n"
        ".program = run: args",
        Production::COMPILATION_UNIT));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - edge cases with whitespace and newlines") {
    Grammar2& grammar = getGrammar();

    // Multiple newlines between definitions
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        "\n"
        ".import \"file.basis\"\n"
        "\n"
        "\n"
        ".alias MyInt: Int",
        Production::COMPILATION_UNIT));

    // Definitions with varying indentation levels
    CHECK_FALSE(testParse(grammar.COMPILATION_UNIT,
        ".module MyModule\n"
        "  .import \"file.basis\"\n"
        "    .alias MyInt: Int\n"
        "      .domain UserId: Int"));
}

TEST_CASE("Grammar2::test COMPILATION_UNIT - all 13 definition types together") {
    Grammar2& grammar = getGrammar();

    // Comprehensive test with all 13 definition types that begin with reserved words
    // (excluding MODULE and IMPORT which have their own sections)
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".module Comprehensive\n"
        ".import Std:Core\n"
        ".alias MyInt: Int\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".instance Widget: Interface\n"
        ".object Node: Int value\n"
        ".program = main\n"
        ".record Point: Int x, Int y\n"
        ".test \"comprehensive test\" = run",
        Production::COMPILATION_UNIT));

    // Same but without module and imports
    CHECK(testParse(grammar.COMPILATION_UNIT,
        ".alias MyInt: Int\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".instance Widget: Interface\n"
        ".object Node: Int value\n"
        ".program = main\n"
        ".record Point: Int x, Int y\n"
        ".test \"comprehensive test\" = run",
        Production::COMPILATION_UNIT));
}

