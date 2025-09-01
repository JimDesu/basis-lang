#include <iostream>

#include "doctest.h"

#include <vector>
#include "../Parser.h"

using namespace basis;

void addToken(std::list<Token>& tokens, const TokenType type) {
    tokens.emplace_back();
    tokens.back().type = type;
}

void addTokens(std::list<Token>& tokens, std::vector<TokenType> types) {
    for ( auto& type : types ) {
        addToken(tokens, type);
    }
}

TEST_CASE("test least match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn( parser.match(TokenType::IDENTIFIER) );
    CHECK( parser.parse() );
}

TEST_CASE("test single match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, true);
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
    Parser parser(tokens, true );
    parser.setParseFn( parser.match(TokenType::NUMBER) );
    CHECK( !parser.parse() );
}

TEST_CASE("test maybe match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn( Parser::maybe(parser.match(TokenType::NUMBER) ));
    CHECK( parser.parse() );
    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    parser.setParseFn( Parser::maybe(parser.match(TokenType::IDENTIFIER) ));
    CHECK( parser.parse() );
}

/*
TEST_CASE("test choice match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn(
        Parser::choice( { parser.match(TokenType::NUMBER), parser.match(TokenType::IDENTIFIER) } ));
    CHECK( parser.parse() );
}
*/

TEST_CASE("test sequence match") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser parser(tokens,true);
    parser.setParseFn(
        Parser::sequence( Production::VARNAME,
        { parser.match(TokenType::IDENTIFIER), parser.match(TokenType::COLON) } ));
    CHECK( parser.parse() );
}


