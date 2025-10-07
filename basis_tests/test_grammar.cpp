#include "doctest.h"

#include "../Grammar.h"

using namespace basis;

namespace {
    void testParser(TokenType tokenType, Production production, const std::string& text) {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = tokenType;
        tokens.back().text = text;

        // We can't instantiate the parser generically, so this is a helper for manual tests
        // Each test case will call the specific parser type
    }
}

TEST_CASE("test literals") {
    // DECIMAL
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::DECIMAL;
        tokens.back().text = "3.14";
        DECIMAL parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DECIMAL);
    }

    // HEXNUMBER
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::HEXNUMBER;
        tokens.back().text = "0x1234";
        HEXNUMBER parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::HEXNUMBER);
    }

    // NUMBER
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::NUMBER;
        tokens.back().text = "1234";
        NUMBER parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::NUMBER);
    }

    // STRING
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::STRING;
        tokens.back().text = "\"foo\\n\\\"bar's\"";
        STRING parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::STRING);
    }
}

TEST_CASE("test identifiers") {
    // IDENTIFIER
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::IDENTIFIER;
        tokens.back().text = "foobar";
        IDENTIFIER parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::IDENTIFIER);
    }
}

TEST_CASE("test reserved words") {
    // ALIAS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::ALIAS;
        tokens.back().text = ".alias";
        ALIAS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ALIAS);
    }

    // CLASS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::CLASS;
        tokens.back().text = ".class";
        CLASS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CLASS);
    }

    // COMMAND
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::COMMAND;
        tokens.back().text = ".cmd";
        COMMAND parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COMMAND);
    }

    // DOMAIN
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::DOMAIN;
        tokens.back().text = ".domain";
        DOMAIN parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DOMAIN);
    }

    // ENUMERATION
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::ENUMERATION;
        tokens.back().text = ".enum";
        ENUMERATION parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ENUMERATION);
    }

    // INTRINSIC
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::INTRINSIC;
        tokens.back().text = ".intrinsic";
        INTRINSIC parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::INTRINSIC);
    }

    // OBJECT
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::OBJECT;
        tokens.back().text = ".object";
        OBJECT parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::OBJECT);
    }

    // RECORD
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::RECORD;
        tokens.back().text = ".record";
        RECORD parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RECORD);
    }
}


TEST_CASE("test punctuation part 1") {
    // AMBANG
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::AMBANG;
        tokens.back().text = "@!";
        AMBANG parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMBANG);
    }

    // AMPERSAND
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::AMPERSAND;
        tokens.back().text = "&";
        AMPERSAND parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMPERSAND);
    }

    // AMPHORA
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::AMPHORA;
        tokens.back().text = "@";
        AMPHORA parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::AMPHORA);
    }

    // ASSIGN
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::ASSIGN;
        tokens.back().text = "<-";
        ASSIGN parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ASSIGN);
    }

    // ASTERISK
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::ASTERISK;
        tokens.back().text = "*";
        ASTERISK parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::ASTERISK);
    }

    // BANG
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::BANG;
        tokens.back().text = "!";
        BANG parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::BANG);
    }

    // BANGLANGLE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::BANGLANGLE;
        tokens.back().text = "!<";
        BANGLANGLE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::BANGLANGLE);
    }

    // CARAT
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::CARAT;
        tokens.back().text = "^";
        CARAT parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CARAT);
    }

    // CARATQ
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::CARATQ;
        tokens.back().text = "^?";
        CARATQ parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::CARATQ);
    }

    // COMMA
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::COMMA;
        tokens.back().text = ",";
        COMMA parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COMMA);
    }

    // COLON
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::COLON;
        tokens.back().text = ":";
        COLON parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COLON);
    }

    // COLANGLE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::COLANGLE;
        tokens.back().text = ":<";
        COLANGLE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::COLANGLE);
    }

    // DCOLON
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::DCOLON;
        tokens.back().text = "::";
        DCOLON parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::DCOLON);
    }

    // EQUALS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::EQUALS;
        tokens.back().text = "=";
        EQUALS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::EQUALS);
    }
}


TEST_CASE("test punctuation part 2") {
    // LANGLE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::LANGLE;
        tokens.back().text = "<";
        LANGLE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LANGLE);
    }

    // LBRACE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::LBRACE;
        tokens.back().text = "{";
        LBRACE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LBRACE);
    }

    // LBRACKET
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::LBRACKET;
        tokens.back().text = "[";
        LBRACKET parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LBRACKET);
    }

    // LPAREN
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::LPAREN;
        tokens.back().text = "(";
        LPAREN parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::LPAREN);
    }

    // MINUS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::MINUS;
        tokens.back().text = "-";
        MINUS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::MINUS);
    }

    // PERCENT
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::PERCENT;
        tokens.back().text = "%";
        PERCENT parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PERCENT);
    }

    // PIPE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::PIPE;
        tokens.back().text = "|";
        PIPE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PIPE);
    }

    // PIPECOL
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::PIPECOL;
        tokens.back().text = "|:";
        PIPECOL parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PIPECOL);
    }

    // PLUS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::PLUS;
        tokens.back().text = "+";
        PLUS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::PLUS);
    }

    // QCOLON
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::QCOLON;
        tokens.back().text = "?:";
        QCOLON parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QCOLON);
    }

    // QLANGLE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::QLANGLE;
        tokens.back().text = "?<";
        QLANGLE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QLANGLE);
    }

    // QMARK
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::QMARK;
        tokens.back().text = "?";
        QMARK parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QMARK);
    }

    // QMINUS
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::QMINUS;
        tokens.back().text = "?-";
        QMINUS parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::QMINUS);
    }

    // RANGLE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::RANGLE;
        tokens.back().text = ">";
        RANGLE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RANGLE);
    }

    // RBRACE
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::RBRACE;
        tokens.back().text = "}";
        RBRACE parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RBRACE);
    }

    // RBRACKET
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::RBRACKET;
        tokens.back().text = "]";
        RBRACKET parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RBRACKET);
    }

    // RPAREN
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::RPAREN;
        tokens.back().text = ")";
        RPAREN parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::RPAREN);
    }

    // SLASH
    {
        std::list<Token> tokens;
        tokens.emplace_back();
        tokens.back().type = TokenType::SLASH;
        tokens.back().text = "/";
        SLASH parser(tokens);
        CHECK(parser.parse());
        CHECK(parser.parseTree->production == Production::SLASH);
    }
}



