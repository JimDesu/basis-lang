#include "doctest.h"

#include "../Grammar.h"
#include "../Lexer.h"
#include <sstream>

using namespace basis;

namespace {
    std::list<Token> tokenize(const std::string& text) {
        std::istringstream input(text);
        Lexer lexer(input);
        lexer.scan();
        return lexer.output;
    }
}

TEST_CASE("test parse literals") {
    // DECIMAL
    {
        DECIMAL parser(tokenize("3.14"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DECIMAL);
    }

    // HEXNUMBER
    {
        HEXNUMBER parser(tokenize("0x1234"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::HEXNUMBER);
    }

    // NUMBER
    {
        NUMBER parser(tokenize("1234"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::NUMBER);
    }

    // STRING
    {
        STRING parser(tokenize("\"foo\\n\\\"bar's\""));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::STRING);
    }
}

TEST_CASE("test parse identifiers") {
    // IDENTIFIER
    {
        IDENTIFIER parser(tokenize("foobar"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::IDENTIFIER);
    }
}

TEST_CASE("test parse reserved words") {
    // ALIAS
    {
        ALIAS parser(tokenize(".alias"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ALIAS);
    }

    // CLASS
    {
        CLASS parser(tokenize(".class"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CLASS);
    }

    // COMMAND
    {
        COMMAND parser(tokenize(".cmd"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COMMAND);
    }

    // DOMAIN
    {
        DOMAIN parser(tokenize(".domain"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DOMAIN);
    }

    // ENUMERATION
    {
        ENUMERATION parser(tokenize(".enum"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ENUMERATION);
    }

    // INTRINSIC
    {
        INTRINSIC parser(tokenize(".intrinsic"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::INTRINSIC);
    }

    // OBJECT
    {
        OBJECT parser(tokenize(".object"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::OBJECT);
    }

    // RECORD
    {
        RECORD parser(tokenize(".record"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RECORD);
    }
}


TEST_CASE("test parse punctuation part 1") {
    // AMBANG
    {
        AMBANG parser(tokenize("@!"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMBANG);
    }

    // AMPERSAND
    {
        AMPERSAND parser(tokenize("&"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMPERSAND);
    }

    // AMPHORA
    {
        AMPHORA parser(tokenize("@"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMPHORA);
    }

    // ASSIGN
    {
        ASSIGN parser(tokenize("<-"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ASSIGN);
    }

    // ASTERISK
    {
        ASTERISK parser(tokenize("*"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ASTERISK);
    }

    // BANG
    {
        BANG parser(tokenize("!"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::BANG);
    }

    // BANGLANGLE
    {
        BANGLANGLE parser(tokenize("!<"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::BANGLANGLE);
    }

    // CARAT
    {
        CARAT parser(tokenize("^"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CARAT);
    }

    // CARATQ
    {
        CARATQ parser(tokenize("^?"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CARATQ);
    }

    // COMMA
    {
        COMMA parser(tokenize(","));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COMMA);
    }

    // COLON
    {
        COLON parser(tokenize(":"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COLON);
    }

    // COLANGLE
    {
        COLANGLE parser(tokenize(":<"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COLANGLE);
    }

    // DCOLON
    {
        DCOLON parser(tokenize("::"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DCOLON);
    }

    // EQUALS
    {
        EQUALS parser(tokenize("="));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::EQUALS);
    }
}


TEST_CASE("test parse punctuation part 2") {
    // LANGLE
    {
        LANGLE parser(tokenize("<"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LANGLE);
    }

    // LBRACE
    {
        LBRACE parser(tokenize("{"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LBRACE);
    }

    // LBRACKET
    {
        LBRACKET parser(tokenize("["));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LBRACKET);
    }

    // LPAREN
    {
        LPAREN parser(tokenize("("));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LPAREN);
    }

    // MINUS
    {
        MINUS parser(tokenize("-"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::MINUS);
    }

    // PERCENT
    {
        PERCENT parser(tokenize("%"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PERCENT);
    }

    // PIPE
    {
        PIPE parser(tokenize("|"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PIPE);
    }

    // PIPECOL
    {
        PIPECOL parser(tokenize("|:"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PIPECOL);
    }

    // PLUS
    {
        PLUS parser(tokenize("+"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PLUS);
    }

    // QCOLON
    {
        QCOLON parser(tokenize("?:"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QCOLON);
    }

    // QLANGLE
    {
        QLANGLE parser(tokenize("?<"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QLANGLE);
    }

    // QMARK
    {
        QMARK parser(tokenize("?"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QMARK);
    }

    // QMINUS
    {
        QMINUS parser(tokenize("?-"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QMINUS);
    }

    // RANGLE
    {
        RANGLE parser(tokenize(">"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RANGLE);
    }

    // RBRACE
    {
        RBRACE parser(tokenize("}"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RBRACE);
    }

    // RBRACKET
    {
        RBRACKET parser(tokenize("]"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RBRACKET);
    }

    // RPAREN
    {
        RPAREN parser(tokenize(")"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RPAREN);
    }

    // SLASH
    {
        SLASH parser(tokenize("/"));
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::SLASH);
    }
}



