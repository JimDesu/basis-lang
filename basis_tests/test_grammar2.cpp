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

TEST_CASE("Grammar2::test type expressions") {
    Grammar2& grammar = getGrammar();
    CHECK(testParse(grammar.TYPE_TYPEPARM_TYPE, "T", Production::TYPE_TYPEPARM_TYPE));
    CHECK(testParse(grammar.TYPE_TYPEPARM_TYPE, "T U", Production::TYPE_TYPEPARM_TYPE));
    CHECK(testParse(grammar.TYPE_TYPEPARM_TYPE, "String", Production::TYPE_TYPEPARM_TYPE));
    CHECK(testParse(grammar.TYPE_TYPEPARM_TYPE, "List Element", Production::TYPE_TYPEPARM_TYPE));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_TYPE, "t"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_TYPE, "T'"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_TYPE, "T value"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_TYPE, "T U V"));

    CHECK(testParse(grammar.TYPE_TYPEPARM_VALUE, "T value", Production::TYPE_TYPEPARM_VALUE));
    CHECK(testParse(grammar.TYPE_TYPEPARM_VALUE, "String name", Production::TYPE_TYPEPARM_VALUE));
    CHECK(testParse(grammar.TYPE_TYPEPARM_VALUE, "Int count", Production::TYPE_TYPEPARM_VALUE));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_VALUE, "T"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_VALUE, "t value"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_VALUE, "T Value"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_VALUE, "value T"));
    CHECK_FALSE(testParse(grammar.TYPE_TYPEPARM_VALUE, "T' value"));  // apostrophe not allowed

    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T, U]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T U]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T value]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T, U value]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T U, V value]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[T, U, V]", Production::TYPE_NAME_PARMS));
    CHECK(testParse(grammar.TYPE_NAME_PARMS, "[String name, Int count]", Production::TYPE_NAME_PARMS));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "T"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "[T"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "T]"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "[]"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "[t]"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_PARMS, "[T,]"));

    // TYPE_NAME_Q: no apostrophes allowed
    CHECK(testParse(grammar.TYPE_NAME_Q, "Int", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "String", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "T", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "List[T]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Map[K, V]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Container[T U]", Production::TYPE_NAME_Q));
    CHECK(testParse(grammar.TYPE_NAME_Q, "Dict[String key, Int value]", Production::TYPE_NAME_Q));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "int"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "value"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "T'"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "Int[]"));
    CHECK_FALSE(testParse(grammar.TYPE_NAME_Q, "[T]"));

    // TYPE_ARGNAME_Q: apostrophes optional (allows both with and without)
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "Int", Production::TYPE_ARGNAME_Q));
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "Int'", Production::TYPE_ARGNAME_Q));
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "String", Production::TYPE_ARGNAME_Q));
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "String'", Production::TYPE_ARGNAME_Q));
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "T", Production::TYPE_ARGNAME_Q));
    CHECK(testParse(grammar.TYPE_ARGNAME_Q, "T'", Production::TYPE_ARGNAME_Q));
    CHECK_FALSE(testParse(grammar.TYPE_ARGNAME_Q, "int"));
    CHECK_FALSE(testParse(grammar.TYPE_ARGNAME_Q, "int'"));
    CHECK_FALSE(testParse(grammar.TYPE_ARGNAME_Q, "value"));
    CHECK_FALSE(testParse(grammar.TYPE_ARGNAME_Q, "value'"));

    CHECK(testParse(grammar.TYPE_EXPR_PTR, "^", Production::TYPE_EXPR_PTR));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[]", Production::TYPE_EXPR_RANGE));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[10]", Production::TYPE_EXPR_RANGE));
    CHECK(testParse(grammar.TYPE_EXPR_RANGE, "[size]", Production::TYPE_EXPR_RANGE));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "[Size]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "["));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_RANGE, "]"));

    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<List[T]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<^[]Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "<Int', String'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<List[T]'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<^[]Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "?<Int', []String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<Int, String>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<^Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<[]Int>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<List[T]>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<^[]Int', String'>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<<Int>>", Production::TYPE_EXPR_CMD));
    CHECK(testParse(grammar.TYPE_EXPR_CMD, "!<?<Int>>", Production::TYPE_EXPR_CMD));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "?<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "!<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "<Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "<Int,>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "?<Int,>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "!<Int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "<?Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR_CMD, "<!Int>"));

    // TYPE_EXPR: no apostrophes allowed in standalone typenames
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
    // TYPE_EXPR: apostrophes ARE allowed inside TYPE_EXPR_CMD
    CHECK(testParse(grammar.TYPE_EXPR, "<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "<List[T], Map[K, V]'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<Int, String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<List[T]', Map[K, V]>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]<Int', String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]<Int>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^ ?<Int'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]!<Int, String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]?<Int'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "<<Int>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "<^Int', []String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^<^Int, []String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "?<<Int>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "!<?<Int'>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "<?<Int>, !<String>>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^ ?<^Int', []String>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]!<^Int, []String'>", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^[]List[T]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[]^Map[K, V]", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "^^[]Int", Production::TYPE_EXPR));
    CHECK(testParse(grammar.TYPE_EXPR, "[10][20]Int", Production::TYPE_EXPR));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "?<List'[T], Map[K, V']>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "T'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "List'[T']"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "^Int'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "[]String'"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "int"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "value"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "^"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "[]"));
    //TODO fix this
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "?<>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "!<>"));
    //end-TODO
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int[]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "Int^"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "[T]"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "<?Int>"));
    CHECK_FALSE(testParse(grammar.TYPE_EXPR, "<!Int>"));
}

TEST_CASE("Grammar2::test type alias definitions") {
    Grammar2& grammar = getGrammar();
    // Alias name (TYPE_NAME_Q) cannot have apostrophes
    // Alias type (TYPE_EXPR) can have apostrophes only inside TYPE_EXPR_CMD
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt: Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias T: U", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias List[T]: ^[]T", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]: ^[]Pair[K, V]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Dict[String key, Int value]: ^[]Entry", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntPtr: ^Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias StringPtr: ^String", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias PtrPtr: ^^Int", Production::DEF_ALIAS));
    // Apostrophes allowed in TYPE_EXPR_CMD within the type expression
    CHECK(testParse(grammar.DEF_ALIAS, ".alias CmdType: <Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeCmdType: ?<String', Int>", Production::DEF_ALIAS));
    // Apostrophes NOT allowed in alias name or standalone type
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias String': String"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias T': U'"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias IntPtr: ^Int'"));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntArray: []Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FixedArray: [10]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias SizedArray: [size]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Matrix: [][]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd: <Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeInt: ?<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FailInt: !<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MultiCmd: <Int, String>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias ComplexType: ^[]List[T]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias CmdArray: []<Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias PtrArray: ^[]Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias NestedCmd: <^[]Int, String>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias DeepNest: ^[]?<List[T]>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt:\n Int", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]:\n ^[]Pair[K, V]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias ComplexType:\n ^[]List[T]", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd:\n <Int>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MyInt:\n Int", Production::DEF_ALIAS));
    // Apostrophes allowed in TYPE_EXPR (inside TYPE_EXPR_CMD) but NOT in alias name
    CHECK(testParse(grammar.DEF_ALIAS, ".alias IntCmd:\n <Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias MaybeInt:\n ?<Int'>", Production::DEF_ALIAS));
    CHECK(testParse(grammar.DEF_ALIAS, ".alias FailInt:\n !<Int'>", Production::DEF_ALIAS));
    // Apostrophes NOT allowed in alias name
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
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt: []"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt: <>"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias MyInt:\nInt"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias List[T]:\n^[]T"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias Map[K, V]:\nPair[K, V]"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias ComplexType:\n ^[]List[T']"));
    CHECK_FALSE(testParse(grammar.DEF_ALIAS, ".alias DeepNest:\n ^[]?<List'[T']>"));
}

TEST_CASE("Grammar2::test command declarations") {
    Grammar2& grammar = getGrammar();

    // DEF_CMD_PARMTYPE_NAME - full TYPE_EXPR (no apostrophes in standalone types)
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
    // Apostrophes allowed inside TYPE_EXPR_CMD
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "<Int>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "?<Int', String>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^<Int'>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]?<Int, String'>", Production::DEF_CMD_PARMTYPE_NAME));
    // Apostrophes NOT allowed in standalone types
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "!<List[T']>", Production::DEF_CMD_PARMTYPE_NAME));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "String'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "T'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^Int'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "value"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "int"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "^"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "[]"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARMTYPE_NAME, "<>"));

    // DEF_CMD_PARM_NAME - just an identifier
    CHECK(testParse(grammar.DEF_CMD_PARM_NAME, "value", Production::DEF_CMD_PARM_NAME));
    CHECK(testParse(grammar.DEF_CMD_PARM_NAME, "'value", Production::DEF_CMD_PARM_NAME));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_NAME, "Value"));

    // DEF_CMD_PARMTYPE_VAR - (TYPENAME: TYPE_EXPR) - TYPENAME cannot have apostrophe
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: String)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: ^Int)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: List[U])", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(U: ^[]Int)", Production::DEF_CMD_PARMTYPE_VAR));
    // Apostrophes allowed in TYPE_EXPR_CMD within the type expression
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(T: <Int'>)", Production::DEF_CMD_PARMTYPE_VAR));
    CHECK(testParse(grammar.DEF_CMD_PARMTYPE_VAR, "(U: ?<String', Int>)", Production::DEF_CMD_PARMTYPE_VAR));
    // Apostrophes NOT allowed in TYPENAME or standalone type
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

    // DEF_CMD_PARM_TYPE - type (name or var)
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "String", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "^Int", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "[]List[T]", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "(T: Int)", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "(U: []List[T])", Production::DEF_CMD_PARM_TYPE));
    CHECK(testParse(grammar.DEF_CMD_PARM_TYPE, "?<Int', String>", Production::DEF_CMD_PARM_TYPE));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_TYPE, "String'"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM_TYPE, "(T': ^Int)"));

    // DEF_CMD_PARM - type + name
    CHECK(testParse(grammar.DEF_CMD_PARM, "Int x", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "String name", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "^Int ptr", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "[]List[T] items", Production::DEF_CMD_PARM));
    CHECK(testParse(grammar.DEF_CMD_PARM, "?<Int', String> cmd", Production::DEF_CMD_PARM));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "String' name"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "value x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_PARM, "Int X"));

    // DEF_CMD_NAME - identifier
    CHECK(testParse(grammar.DEF_CMD_NAME, "doSomething", Production::DEF_CMD_NAME));
    CHECK_FALSE(testParse(grammar.DEF_CMD_NAME, "DoSomething"));

    // DEF_CMD_RECEIVER - type + name
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "Widget w", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "Widget 'w", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "^Widget ptr", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "[]Container[T] items", Production::DEF_CMD_RECEIVER));
    CHECK(testParse(grammar.DEF_CMD_RECEIVER, "?<Widget'> cmd", Production::DEF_CMD_RECEIVER));
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "Widget' w"));
    CHECK_FALSE(testParse(grammar.DEF_CMD_RECEIVER, "widget w"));

    // DEF_CMD - full command definition
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd ?doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd !doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x, String y", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: Int x / Int ctx", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: ?doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: !doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w, Button b:: doIt", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w, Button 'b:: doIt: Int x / Int ctx -> result", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i, Int j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i, Int j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd Widget w:: (T:Int) i, T j", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: ?<Int'> cmd", Production::DEF_CMD));
    CHECK(testParse(grammar.DEF_CMD, ".cmd doIt: !<String', Int> result", Production::DEF_CMD));
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
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: Int ?x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: Int !x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: ?Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt: !Int x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget 'w, Button 'b:: Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt:"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt /"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt :/ Int i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt : Int i -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt -> T"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt : -> x"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt / x ->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd doIt :/->"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w: Int i / Int j -> i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i / Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i -> Int j"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i / Int j -> i"));
    CHECK_FALSE(testParse(grammar.DEF_CMD, ".cmd Widget w:: Int i -> i"));
}

