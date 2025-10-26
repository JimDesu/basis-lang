#include "doctest.h"

#include "../Grammar2.h"
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

    auto parseText(SPFN parseFn, const std::string& text) {
        Parser parser(tokenize(text), parseFn);
        CHECK(parser.parse());
        return parser.parseTree;
    }

    void parseFail(SPFN parseFn, const std::string& text) {
        Parser parser(tokenize(text), parseFn);
        CHECK_FALSE(parser.parse());
    }
}

TEST_CASE("test parse literals - Parser2") {
    CHECK(parseText(Grammar2::DECIMAL, "3.14")->production == Production::DECIMAL);
    CHECK(parseText(Grammar2::HEXNUMBER, "0x1234")->production == Production::HEXNUMBER);
    CHECK(parseText(Grammar2::NUMBER, "1234")->production == Production::NUMBER);
    CHECK(parseText(Grammar2::STRING, "\"foo\\n\\\"bar's\"")->production == Production::STRING);
    parseFail(Grammar2::DECIMAL, "3.14.56");
    parseFail(Grammar2::HEXNUMBER, "0x12345");
    parseFail(Grammar2::NUMBER, "1234.56");
    parseFail(Grammar2::STRING, "\"foo\nbar\"");
}

TEST_CASE("test parse identifiers - Parser2") {
    CHECK(parseText(Grammar2::IDENTIFIER, "foobar")->production == Production::IDENTIFIER);
    CHECK(parseText(Grammar2::TYPENAME, "Foobar")->production == Production::TYPENAME);
}

TEST_CASE("test parse reserved words - Parser2") {
    CHECK(parseText(Grammar2::ALIAS, ".alias")->production == Production::ALIAS);
    CHECK(parseText(Grammar2::CLASS, ".class")->production == Production::CLASS);
    CHECK(parseText(Grammar2::COMMAND, ".cmd")->production == Production::COMMAND);
    CHECK(parseText(Grammar2::DOMAIN, ".domain")->production == Production::DOMAIN);
    CHECK(parseText(Grammar2::ENUMERATION, ".enum")->production == Production::ENUMERATION);
    CHECK(parseText(Grammar2::INTRINSIC, ".intrinsic")->production == Production::INTRINSIC);
    CHECK(parseText(Grammar2::OBJECT, ".object")->production == Production::OBJECT);
    CHECK(parseText(Grammar2::RECORD, ".record")->production == Production::RECORD);
}

TEST_CASE("test parse punctuation - Parser2") {
    CHECK(parseText(Grammar2::AMBANG, "@!")->production == Production::AMBANG);
    CHECK(parseText(Grammar2::AMPERSAND, "&")->production == Production::AMPERSAND);
    CHECK(parseText(Grammar2::AMPHORA, "@")->production == Production::AMPHORA);
    CHECK(parseText(Grammar2::ASTERISK, "*")->production == Production::ASTERISK);
    CHECK(parseText(Grammar2::BANG, "!")->production == Production::BANG);
    CHECK(parseText(Grammar2::BANGLANGLE, "!<")->production == Production::BANGLANGLE);
    CHECK(parseText(Grammar2::CARAT, "^")->production == Production::CARAT);
    CHECK(parseText(Grammar2::CARATQ, "^?")->production == Production::CARATQ);
    CHECK(parseText(Grammar2::COMMA, ",")->production == Production::COMMA);
    CHECK(parseText(Grammar2::COLON, ":")->production == Production::COLON);
    CHECK(parseText(Grammar2::COLANGLE, ":<")->production == Production::COLANGLE);
    CHECK(parseText(Grammar2::DCOLON, "::")->production == Production::DCOLON);
    CHECK(parseText(Grammar2::EQUALS, "=")->production == Production::EQUALS);
    CHECK(parseText(Grammar2::LANGLE, "<")->production == Production::LANGLE);
    CHECK(parseText(Grammar2::LARROW, "<-")->production == Production::LARROW);
    CHECK(parseText(Grammar2::LBRACE, "{")->production == Production::LBRACE);
    CHECK(parseText(Grammar2::LBRACKET, "[")->production == Production::LBRACKET);
    CHECK(parseText(Grammar2::LPAREN, "(")->production == Production::LPAREN);
    CHECK(parseText(Grammar2::MINUS, "-")->production == Production::MINUS);
    CHECK(parseText(Grammar2::PERCENT, "%")->production == Production::PERCENT);
    CHECK(parseText(Grammar2::PIPE, "|")->production == Production::PIPE);
    CHECK(parseText(Grammar2::PIPECOL, "|:")->production == Production::PIPECOL);
    CHECK(parseText(Grammar2::PLUS, "+")->production == Production::PLUS);
    CHECK(parseText(Grammar2::POUND, "#")->production == Production::POUND);
    CHECK(parseText(Grammar2::QCOLON, "?:")->production == Production::QCOLON);
    CHECK(parseText(Grammar2::QLANGLE, "?<")->production == Production::QLANGLE);
    CHECK(parseText(Grammar2::QMARK, "?")->production == Production::QMARK);
    CHECK(parseText(Grammar2::QMINUS, "?-")->production == Production::QMINUS);
    CHECK(parseText(Grammar2::RANGLE, ">")->production == Production::RANGLE);
    CHECK(parseText(Grammar2::RARROW, "->")->production == Production::RARROW);
    CHECK(parseText(Grammar2::RBRACE, "}")->production == Production::RBRACE);
    CHECK(parseText(Grammar2::RBRACKET, "]")->production == Production::RBRACKET);
    CHECK(parseText(Grammar2::RPAREN, ")")->production == Production::RPAREN);
    CHECK(parseText(Grammar2::SLASH, "/")->production == Production::SLASH);
}

TEST_CASE("test parse enum definition - Parser2") {
    CHECK(parseText(Grammar2::DEF_ENUM, ".enum T Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    CHECK(parseText(Grammar2::DEF_ENUM, ".enum Fish: sockeye = 0, salmon = 1")->production == Production::DEF_ENUM);
    parseFail(Grammar2::DEF_ENUM, ".enum A B fish: sockeye = 0, salmon = 1");
    parseFail(Grammar2::DEF_ENUM, ".enum T Fish: sockeye= 0,\nsalmon = 1");
    parseFail(Grammar2::DEF_ENUM, ".enum T Fish: Sockeye= 0, Salmon = 1");
}

