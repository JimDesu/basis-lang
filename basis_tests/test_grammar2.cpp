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
        Lexer lexer(input);
        lexer.scan();
        return lexer.output;
    }

    auto parseText(SPPF parseFn, const std::string& text) {
        std::list<spToken> tokens = tokenize(text);
        Parser parser(tokens, parseFn);
        CHECK(parser.parse());
        return parser.parseTree;
    }

    void parseFail(SPPF parseFn, const std::string& text) {
        std::list<spToken> tokens = tokenize(text);
        Parser parser(tokens, parseFn);
        CHECK_FALSE(parser.parse());
    }
}

TEST_CASE("test parse literals - Parser2") {
    Grammar2& grammar = getGrammar();
    CHECK(parseText(grammar.DECIMAL, "3.14")->production == Production::DECIMAL);
    CHECK(parseText(grammar.HEXNUMBER, "0x1234")->production == Production::HEXNUMBER);
    CHECK(parseText(grammar.NUMBER, "1234")->production == Production::NUMBER);
    CHECK(parseText(grammar.STRING, "\"foo\\n\\\"bar's\"")->production == Production::STRING);
    parseFail(grammar.DECIMAL, "3.14.56");
    parseFail(grammar.HEXNUMBER, "0x12345");
    parseFail(grammar.NUMBER, "1234.56");
    parseFail(grammar.STRING, "\"foo\nbar\"");
}

TEST_CASE("test parse identifiers - Parser2") {
    Grammar2& grammar = getGrammar();
    CHECK(parseText(grammar.IDENTIFIER, "foobar")->production == Production::IDENTIFIER);
    CHECK(parseText(grammar.TYPENAME, "Foobar")->production == Production::TYPENAME);
}

TEST_CASE("test parse reserved words - Parser2") {
    Grammar2& grammar = getGrammar();
    CHECK(parseText(grammar.ALIAS, ".alias")->production == Production::ALIAS);
    CHECK(parseText(grammar.CLASS, ".class")->production == Production::CLASS);
    CHECK(parseText(grammar.COMMAND, ".cmd")->production == Production::COMMAND);
    CHECK(parseText(grammar.DOMAIN, ".domain")->production == Production::DOMAIN);
    CHECK(parseText(grammar.ENUMERATION, ".enum")->production == Production::ENUMERATION);
    CHECK(parseText(grammar.INTRINSIC, ".intrinsic")->production == Production::INTRINSIC);
    CHECK(parseText(grammar.OBJECT, ".object")->production == Production::OBJECT);
    CHECK(parseText(grammar.RECORD, ".record")->production == Production::RECORD);
}

TEST_CASE("test parse punctuation - Parser2") {
    Grammar2& grammar = getGrammar();
    CHECK(parseText(grammar.AMBANG, "@!")->production == Production::AMBANG);
    CHECK(parseText(grammar.AMPERSAND, "&")->production == Production::AMPERSAND);
    CHECK(parseText(grammar.AMPHORA, "@")->production == Production::AMPHORA);
    CHECK(parseText(grammar.ASTERISK, "*")->production == Production::ASTERISK);
    CHECK(parseText(grammar.BANG, "!")->production == Production::BANG);
    CHECK(parseText(grammar.BANGLANGLE, "!<")->production == Production::BANGLANGLE);
    CHECK(parseText(grammar.CARAT, "^")->production == Production::CARAT);
    CHECK(parseText(grammar.CARATQ, "^?")->production == Production::CARATQ);
    CHECK(parseText(grammar.COMMA, ",")->production == Production::COMMA);
    CHECK(parseText(grammar.COLON, ":")->production == Production::COLON);
    CHECK(parseText(grammar.COLANGLE, ":<")->production == Production::COLANGLE);
    CHECK(parseText(grammar.DCOLON, "::")->production == Production::DCOLON);
    CHECK(parseText(grammar.EQUALS, "=")->production == Production::EQUALS);
    CHECK(parseText(grammar.LANGLE, "<")->production == Production::LANGLE);
    CHECK(parseText(grammar.LARROW, "<-")->production == Production::LARROW);
    CHECK(parseText(grammar.LBRACE, "{")->production == Production::LBRACE);
    CHECK(parseText(grammar.LBRACKET, "[")->production == Production::LBRACKET);
    CHECK(parseText(grammar.LPAREN, "(")->production == Production::LPAREN);
    CHECK(parseText(grammar.MINUS, "-")->production == Production::MINUS);
    CHECK(parseText(grammar.PERCENT, "%")->production == Production::PERCENT);
    CHECK(parseText(grammar.PIPE, "|")->production == Production::PIPE);
    CHECK(parseText(grammar.PIPECOL, "|:")->production == Production::PIPECOL);
    CHECK(parseText(grammar.PLUS, "+")->production == Production::PLUS);
    CHECK(parseText(grammar.POUND, "#")->production == Production::POUND);
    CHECK(parseText(grammar.QCOLON, "?:")->production == Production::QCOLON);
    CHECK(parseText(grammar.QLANGLE, "?<")->production == Production::QLANGLE);
    CHECK(parseText(grammar.QMARK, "?")->production == Production::QMARK);
    CHECK(parseText(grammar.QMINUS, "?-")->production == Production::QMINUS);
    CHECK(parseText(grammar.RANGLE, ">")->production == Production::RANGLE);
    CHECK(parseText(grammar.RARROW, "->")->production == Production::RARROW);
    CHECK(parseText(grammar.RBRACE, "}")->production == Production::RBRACE);
    CHECK(parseText(grammar.RBRACKET, "]")->production == Production::RBRACKET);
    CHECK(parseText(grammar.RPAREN, ")")->production == Production::RPAREN);
    CHECK(parseText(grammar.SLASH, "/")->production == Production::SLASH);
}

TEST_CASE("test parse enum definition - Parser2") {
    Grammar2& grammar = getGrammar();
    CHECK(parseText(grammar.DEF_ENUM, ".enum T Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    CHECK(parseText(grammar.DEF_ENUM, ".enum Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    parseFail(grammar.DEF_ENUM, ".enum A B fish: sockeye = 0, salmon = 1");
    parseFail(grammar.DEF_ENUM, ".enum T Fish: sockeye= 0,\nsalmon = 1");
    parseFail(grammar.DEF_ENUM, ".enum T Fish: Sockeye= 0, Salmon = 1");
}


