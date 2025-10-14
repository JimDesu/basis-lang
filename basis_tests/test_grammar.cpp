#include "doctest.h"

#include "../Grammar.h"
#include "../Lexer.h"
#include <sstream>

using namespace basis;

namespace {
    std::list<spToken> tokenize(const std::string& text) {
        std::istringstream input(text);
        Lexer lexer(input);
        lexer.scan();
        return lexer.output;
    }

    template<typename ParseFnType>
    auto parseText(const std::string& text) {
        Parser<ParseFnType> parser(tokenize(text));
        CHECK(parser.parse());
        return parser.parseTree;
    }

    template<typename ParseFnType>
    void parseFail(const std::string& text) {
        Parser<ParseFnType> parser(tokenize(text));
        CHECK_FALSE(parser.parse());
    }
}

TEST_CASE("test parse literals") {
    CHECK(parseText<DECIMAL>("3.14")->production == Production::DECIMAL);
    CHECK(parseText<HEXNUMBER>("0x1234")->production == Production::HEXNUMBER);
    CHECK(parseText<NUMBER>("1234")->production == Production::NUMBER);
    CHECK(parseText<STRING>("\"foo\\n\\\"bar's\"")->production == Production::STRING);
    parseFail<DECIMAL>("3.14.56");
    parseFail<HEXNUMBER>("0x12345");
    parseFail<NUMBER>("1234.56");
    parseFail<STRING>("\"foo\nbar\"");
}

TEST_CASE("test parse identifiers") {
    CHECK(parseText<IDENTIFIER>("foobar")->production == Production::IDENTIFIER);
    CHECK(parseText<TYPENAME>("Foobar")->production == Production::TYPENAME);
}

TEST_CASE("test parse reserved words") {
    CHECK(parseText<ALIAS>(".alias")->production == Production::ALIAS);
    CHECK(parseText<CLASS>(".class")->production == Production::CLASS);
    CHECK(parseText<COMMAND>(".cmd")->production == Production::COMMAND);
    CHECK(parseText<DOMAIN>(".domain")->production == Production::DOMAIN);
    CHECK(parseText<ENUMERATION>(".enum")->production == Production::ENUMERATION);
    CHECK(parseText<INTRINSIC>(".intrinsic")->production == Production::INTRINSIC);
    CHECK(parseText<OBJECT>(".object")->production == Production::OBJECT);
    CHECK(parseText<RECORD>(".record")->production == Production::RECORD);
}

TEST_CASE("test parse punctuation") {
    CHECK(parseText<AMBANG>("@!")->production == Production::AMBANG);
    CHECK(parseText<AMPERSAND>("&")->production == Production::AMPERSAND);
    CHECK(parseText<AMPHORA>("@")->production == Production::AMPHORA);
    CHECK(parseText<ASTERISK>("*")->production == Production::ASTERISK);
    CHECK(parseText<BANG>("!")->production == Production::BANG);
    CHECK(parseText<BANGLANGLE>("!<")->production == Production::BANGLANGLE);
    CHECK(parseText<CARAT>("^")->production == Production::CARAT);
    CHECK(parseText<CARATQ>("^?")->production == Production::CARATQ);
    CHECK(parseText<COMMA>(",")->production == Production::COMMA);
    CHECK(parseText<COLON>(":")->production == Production::COLON);
    CHECK(parseText<COLANGLE>(":<")->production == Production::COLANGLE);
    CHECK(parseText<DCOLON>("::")->production == Production::DCOLON);
    CHECK(parseText<EQUALS>("=")->production == Production::EQUALS);
    CHECK(parseText<LANGLE>("<")->production == Production::LANGLE);
    CHECK(parseText<LARROW>("<-")->production == Production::LARROW);
    CHECK(parseText<LBRACE>("{")->production == Production::LBRACE);
    CHECK(parseText<LBRACKET>("[")->production == Production::LBRACKET);
    CHECK(parseText<LPAREN>("(")->production == Production::LPAREN);
    CHECK(parseText<MINUS>("-")->production == Production::MINUS);
    CHECK(parseText<PERCENT>("%")->production == Production::PERCENT);
    CHECK(parseText<PIPE>("|")->production == Production::PIPE);
    CHECK(parseText<PIPECOL>("|:")->production == Production::PIPECOL);
    CHECK(parseText<PLUS>("+")->production == Production::PLUS);
    CHECK(parseText<QCOLON>("?:")->production == Production::QCOLON);
    CHECK(parseText<QLANGLE>("?<")->production == Production::QLANGLE);
    CHECK(parseText<QMARK>("?")->production == Production::QMARK);
    CHECK(parseText<QMINUS>("?-")->production == Production::QMINUS);
    CHECK(parseText<RANGLE>(">")->production == Production::RANGLE);
    CHECK(parseText<RARROW>("->")->production == Production::RARROW);
    CHECK(parseText<RBRACE>("}")->production == Production::RBRACE);
    CHECK(parseText<RBRACKET>("]")->production == Production::RBRACKET);
    CHECK(parseText<RPAREN>(")")->production == Production::RPAREN);
    CHECK(parseText<SLASH>("/")->production == Production::SLASH);
}

TEST_CASE("test parse enum definition") {
    CHECK(parseText<DEF_ENUM>(".enum T Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    CHECK(parseText<DEF_ENUM>(".enum Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    parseFail<DEF_ENUM>(".enum A B fish: sockeye = 0, salmon = 1");
    parseFail<DEF_ENUM>(".enum T Fish: sockeye= 0,\nsalmon = 1");
    parseFail<DEF_ENUM>(".enum T Fish: Sockeye= 0, Salmon = 1");
}
