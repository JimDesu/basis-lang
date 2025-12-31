#include "doctest.h"

#include "../Grammar2.h"
#include "../Lexer.h"
#include <sstream>

using namespace basis;

namespace {
    // parser takes the token list by reference, so be sure to put the result onto the stack so
    // we don't end up wth a dangling reference problem (which caused a flaky test that took
    // forever to diagnose)
    std::list<spToken> tokenize(const std::string& text) {
        std::istringstream input(text);
        Lexer lexer(input, false);
        lexer.scan();
        return lexer.output;
    }

    bool testParse(SPPF parseFn, const std::string& text) {
        std::list<spToken> tokens = tokenize(text);
        Parser parser(tokens, parseFn);
        return parser.parse() && parser.allTokensConsumed();
    }

    bool testParse(SPPF parseFn, const std::string& text, Production expected) {
        std::list<spToken> tokens = tokenize(text);
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

    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd ?doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd !doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int 'x, String y", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int 'x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int ctx / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x / Int 'ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x, String y -> result / Int ctx, String s", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: ?doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: !doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w, Button b:: doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w, Button 'b:: doIt: Int x -> 'result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: doIt: Int x -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i, Int j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i, Int j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: (T:Int) i, T j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: ?<Int'> cmd", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: !<String', Int> result", Production::DEF_CMD));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt -> result", Production::DEF_CMD));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: <List[T']> items", Production::DEF_CMD));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: ^Int' ptr"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: List[T'] list"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget' w:: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd ^Widget' ptr:: doIt"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd []Container[T'] items:: process"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget' w:: ^Int' ptr, []String' items"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: (T': []String) x, T' y"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / Int ctx: (T': ^Int) x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: Int ?x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: Int !x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: ?Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: !Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget 'w, Button 'b:: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / :"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / Int i:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt : Int i -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt : -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / x ->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / Int j: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / Int j: Int i -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i / Int j -> i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i / Int j -> i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i -> i"));

    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt"));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic ?doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic !doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic ?doIt: Int x -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic !doIt: Int x -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int 'x, String y -> result / Int 'ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: ^Int ptr -> 'result / String ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: []List[T] items -> result / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: ?<Int'> cmd -> result / String ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: !<String', Int> result -> out / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: (T:Int) i, T j -> result / String ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x, String y -> result / Int ctx, String s", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x, String y -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx"));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x"));
    CHECK(testParse(grammar.DEF_CMD, ".intrinsic doIt :Int x /Int ctx"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result {"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: Int x -> result = x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx: Int x -> result {"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt / Int ctx: Int x -> result = x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: ^Int' ptr -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: []String' items -> result"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".intrinsic doIt: List[T'] list -> result"));
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

