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
}

TEST_CASE("Grammar2::test parse literals") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DECIMAL, "3.14", Production::DECIMAL));
    CHECK(testParse(grammar.HEXNUMBER, "0x1234", Production::HEXNUMBER));
    CHECK(testParse(grammar.NUMBER, "1234", Production::NUMBER));
    CHECK(testParse(grammar.STRING, "\"foo\\n\\\"bar's\"", Production::STRING));
    CHECK_FALSE(testParse(grammar.DECIMAL, "3.14.56"));
    CHECK_FALSE(testParse(grammar.HEXNUMBER, "0x12345"));
    CHECK_FALSE(testParse(grammar.NUMBER, "1234.56"));
    CHECK_FALSE(testParse(grammar.STRING, "\"foo\nbar\""));
}

TEST_CASE("Grammar2::test parse identifiers") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.IDENTIFIER, "foobar", Production::IDENTIFIER));
    CHECK(testParse(grammar.TYPENAME, "Foobar", Production::TYPENAME));
}

TEST_CASE("Grammar2::test parse reserved words") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.ALIAS, ".alias", Production::ALIAS));
    CHECK(testParse(grammar.CLASS, ".class", Production::CLASS));
    CHECK(testParse(grammar.COMMAND, ".cmd", Production::COMMAND));
    CHECK(testParse(grammar.DOMAIN, ".domain", Production::DOMAIN));
    CHECK(testParse(grammar.ENUMERATION, ".enum", Production::ENUMERATION));
    CHECK(testParse(grammar.INTRINSIC, ".intrinsic", Production::INTRINSIC));
    CHECK(testParse(grammar.OBJECT, ".object", Production::OBJECT));
    CHECK(testParse(grammar.RECORD, ".record", Production::RECORD));
}

TEST_CASE("Grammar2::test parse punctuation") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.AMBANG, "@!", Production::AMBANG));
    CHECK(testParse(grammar.AMPERSAND, "&", Production::AMPERSAND));
    CHECK(testParse(grammar.AMPHORA, "@", Production::AMPHORA));
    CHECK(testParse(grammar.ASTERISK, "*", Production::ASTERISK));
    CHECK(testParse(grammar.BANG, "!", Production::BANG));
    CHECK(testParse(grammar.BANGLANGLE, "!<", Production::BANGLANGLE));
    CHECK(testParse(grammar.CARAT, "^", Production::CARAT));
    CHECK(testParse(grammar.CARATQ, "^?", Production::CARATQ));
    CHECK(testParse(grammar.COMMA, ",", Production::COMMA));
    CHECK(testParse(grammar.COLON, ":", Production::COLON));
    CHECK(testParse(grammar.COLANGLE, ":<", Production::COLANGLE));
    CHECK(testParse(grammar.DCOLON, "::", Production::DCOLON));
    CHECK(testParse(grammar.EQUALS, "=", Production::EQUALS));
    CHECK(testParse(grammar.LANGLE, "<", Production::LANGLE));
    CHECK(testParse(grammar.LARROW, "<-", Production::LARROW));
    CHECK(testParse(grammar.LBRACE, "{", Production::LBRACE));
    CHECK(testParse(grammar.LBRACKET, "[", Production::LBRACKET));
    CHECK(testParse(grammar.LPAREN, "(", Production::LPAREN));
    CHECK(testParse(grammar.MINUS, "-", Production::MINUS));
    CHECK(testParse(grammar.PERCENT, "%", Production::PERCENT));
    CHECK(testParse(grammar.PIPE, "|", Production::PIPE));
    CHECK(testParse(grammar.PIPECOL, "|:", Production::PIPECOL));
    CHECK(testParse(grammar.PLUS, "+", Production::PLUS));
    CHECK(testParse(grammar.POUND, "#", Production::POUND));
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

TEST_CASE("Grammar2::test domain type definitions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain MyInt: Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain UserId: Int", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain Temperature: Float", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain FixedBuffer: [10]", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain DynamicArray: []", Production::DEF_DOMAIN));
    CHECK(testParse(grammar.DEF_DOMAIN, ".domain MyInt:\n Int", Production::DEF_DOMAIN));
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
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_DOMAIN, "[]Array[10]", Production::TYPE_EXPR_DOMAIN));
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
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias ComplexType': ^[]List'[T']", Production::DEF_ALIAS));
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
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, ":<>"));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, ":<Int>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "?<Int', String>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^:<Int'>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]?<Int, String'>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]"));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[8]"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "!<List[T']>", Production::DEF_CMD_PARMTYPE_NAME));
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
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "Widget' w"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "widget w"));

    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd ?doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd !doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int 'x, String y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int 'x -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int ctx / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int x / Int 'ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int x -> result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int x, String y -> result / Int ctx, String s", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w:: doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w:: ?doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w:: !doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w, Button b:: doIt", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w, Button 'b:: doIt: Int x -> 'result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w:: doIt: Int x -> result", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w:: doIt: Int x -> result / Int ctx", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: Int i, Int j", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: ?<Int'> cmd", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: !<String', Int> result", Production::DEF_CMD_DECL));

    // Destructor tests (using @ AMPHORA)
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @ Widget w: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @ Widget w: Int x, Int y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @ Container c: String name", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @ Handler h: ^Int ptr, []String items", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @ Resource r: ?<Int'> cmd", Production::DEF_CMD_DECL));

    // Failure handler tests (using @! AMBANG)
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @! Widget w: Int x", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @! Widget w: Int x, Int y", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @! Container c: String error", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @! Handler h: ^Int ptr, []String items", Production::DEF_CMD_DECL));
    CHECK(testParse(grammar.DEF_CMD_DECL, ".cmd @! Resource r: !<String', Int> result", Production::DEF_CMD_DECL));

    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt -> result", Production::DEF_CMD_DECL));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: ^Int' ptr"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: List[T'] list"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget' w:: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd ^Widget' ptr:: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd []Container[T'] items:: process"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget' w:: ^Int' ptr, []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / Int ctx: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int ?x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: Int !x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: ?Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt: !Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget 'w, Button 'b:: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / :"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / Int i:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt : Int i -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt : -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / x ->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / Int j: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd doIt / Int j: Int i -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: Int i / Int j -> i"));

    // Destructor negative tests
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @ doIt: Int x -> result"));  // destructor needs receiver
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @ Widget w"));  // destructor needs parameters
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: @ Int x"));  // @ in wrong position
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @ Widget' w: Int x"));  // receiver can't have apostrophe

    // Failure handler negative tests
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @! Widget w:: Int i"));  // failure handler can't use ::
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @! doIt: Int x -> result"));  // failure handler needs receiver
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @! Widget w"));  // failure handler needs parameters
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd Widget w: @! Int x"));  // @! in wrong position
    CHECK_FALSE(testParse(grammar.DEF_CMD_DECL, ".cmd @! Widget' w: Int x"));  // receiver can't have apostrophe

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
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic Widget w:: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic Widget w:: doIt: Int x -> result / Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic Widget w, Button b:: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic ^Widget ptr:: doIt: Int x -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic []Container[T] items:: process: T item -> result"));
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

    // indentation 1: First DEF_CMD_DECL on next line, all at same indentation
    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass:\n"
        "  .cmd doSomething: Int x -> result\n"
        "  .cmd doOther: String s -> output",
        Production::DEF_CLASS));

    // indentation 2: First DEF_CMD_DECL on same line, subsequent at same indentation
    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass: .cmd doSomething: Int x -> result\n"
        "                .cmd doOther: String s -> output",
        Production::DEF_CLASS));

    // indentation 3: First DEF_CMD_DECL on same line, subsequent indented less (but still more than class start)
    CHECK(testParse(grammar.DEF_CLASS,
        ".class MyClass: .cmd doSomething: Int x -> result\n"
        "  .cmd doOther: String s -> output",
        Production::DEF_CLASS));

    CHECK(testParse(grammar.DEF_CLASS,
        ".class Widget:\n"
        "  .cmd Widget w: Int x, Int y",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Container:\n"
        "  .cmd Container c: Int capacity\n"
        "  .cmd add: Int item -> result\n"
        "  .cmd Widget w:: process: String s -> output / Int ctx",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Processor:\n"
        "  .cmd Widget w, Button b:: handle: Int x -> result",
        Production::DEF_CLASS));
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Handler:\n"
        "  .cmd ?tryProcess: Int x -> result\n"
        "  .cmd !mustFail: String s -> output",
        Production::DEF_CLASS));

    // Class with destructor
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Resource:\n"
        "  .cmd Resource r: String name\n"
        "  .cmd @ Resource r: Int code",
        Production::DEF_CLASS));

    // Class with failure handler
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Transaction:\n"
        "  .cmd Transaction t: Int id\n"
        "  .cmd @! Transaction t: String error",
        Production::DEF_CLASS));

    // Class with constructor, destructor, and failure handler
    CHECK(testParse(grammar.DEF_CLASS,
        ".class Connection:\n"
        "  .cmd Connection c: String host, Int port\n"
        "  .cmd @ Connection c: Int timeout\n"
        "  .cmd @! Connection c: String reason",
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

    // Constructor with body
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w: Int x, Int y = Widget: x, y", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Container c: Int size = Container: size", Production::DEF_CMD));

    // Destructor with body
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Widget w: Int code = cleanup: code", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Resource r: String name = release: name", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Handler h: Int x, Int y = finalize: x, y", Production::DEF_CMD));

    // Failure handler with body
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Widget w: String error = logError: error", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Transaction t: Int code = rollback: code", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Connection c: String msg, Int code = handleFailure: msg, code", Production::DEF_CMD));

    // Destructor without body (optional body)
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Widget w: Int x = doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @ Resource r: String name, Int id = doIt", Production::DEF_CMD));

    // Failure handler without body (optional body)
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Widget w: String error = doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd @! Transaction t: Int code, String msg = doIt", Production::DEF_CMD));

    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @ Widget w:: Int x = cleanup: x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @! Widget w:: String error = logError: error"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @ doIt: Int x = cleanup: x"));

    // Negative tests - failure handlers need receiver
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd @! doIt: String error = logError: error"));
}

TEST_CASE("Grammar2::test command body parsing") {
    Grammar2& grammar = getGrammar();
    // CALL_CONSTRUCTOR positive tests
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "Widget: x, y", Production::CALL_CONSTRUCTOR));
    CHECK(testParse(grammar.CALL_CONSTRUCTOR, "List[Int]: item", Production::CALL_CONSTRUCTOR));
    // CALL_CONSTRUCTOR negative tests
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "Widget: X"));  // uppercase identifier
    CHECK_FALSE(testParse(grammar.CALL_CONSTRUCTOR, "widget: x"));  // lowercase typename

    // CALL_COMMAND positive tests
    CHECK(testParse(grammar.CALL_COMMAND, "doSomething: x, y", Production::CALL_COMMAND));
    CHECK(testParse(grammar.CALL_COMMAND, "process: item", Production::CALL_COMMAND));

    // CALL_COMMAND negative tests
    CHECK_FALSE(testParse(grammar.CALL_COMMAND, "DoSomething: x"));  // uppercase command name

    // CALL_VCOMMAND positive tests
    CHECK(testParse(grammar.CALL_VCOMMAND, "obj:: method: x, y", Production::CALL_VCOMMAND));
    CHECK(testParse(grammar.CALL_VCOMMAND, "a, b:: handle: item", Production::CALL_VCOMMAND));

    // CALL_VCOMMAND negative tests
    CHECK_FALSE(testParse(grammar.CALL_VCOMMAND, "Obj:: method: x"));  // uppercase receiver

    // CALL_ASSIGNMENT positive tests
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "result <- Widget: x, y", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "# output <- process: item", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "data <- obj:: method: x", Production::CALL_ASSIGNMENT));
    CHECK(testParse(grammar.CALL_ASSIGNMENT, "#data <- obj:: method: x", Production::CALL_ASSIGNMENT));

    // CALL_ASSIGNMENT negative tests
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "Result <- Widget: x"));  // uppercase target
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result -> Widget: x"));  // wrong arrow
    CHECK_FALSE(testParse(grammar.CALL_ASSIGNMENT, "result Widget: x"));  // missing arrow

    // DEF_CMD_BODY positive tests
    CHECK(testParse(grammar.DEF_CMD_BODY, "= doIt ", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= Widget: x, y", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= doIt: x", Production::DEF_CMD_BODY));
    CHECK(testParse(grammar.DEF_CMD_BODY, "= obj:: method: x", Production::DEF_CMD_BODY));

    // DEF_CMD_BODY negative tests
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "Widget: x"));  // missing equals
    CHECK_FALSE(testParse(grammar.DEF_CMD_BODY, "= "));  // missing call
}
