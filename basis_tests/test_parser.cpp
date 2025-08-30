#include "doctest.h"

#include "../Parser.h"

using namespace basis;

void addToken(std::list<Token>& tokens, const TokenType type) {
    tokens.emplace_back();
    tokens.back().type = type;
}

TEST_CASE("test least match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens);
    parser.setParseFn( parser.match(TokenType::IDENTIFIER) );
    CHECK( parser.parse() );
}

TEST_CASE("test single match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens);
    parser.setParseFn( parser.match(Production::VARNAME, TokenType::IDENTIFIER) );
    CHECK( parser.parse() );
    spParseObject& result = parser.parseTree;
    CHECK( isLeaf(result) );
    CHECK( getLeaf(result)->production == Production::VARNAME );
    CHECK( getLeaf(result)->pToken == &(tokens.front()));
}

TEST_CASE("test simple match fail") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens);
    parser.setParseFn( parser.match(TokenType::NUMBER) );
    CHECK( !parser.parse() );
}

TEST_CASE("test maybe match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens);
    parser.setParseFn( Parser::maybe(parser.match(TokenType::NUMBER) ));
    CHECK( parser.parse() );
}