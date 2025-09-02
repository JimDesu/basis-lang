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
    parser.setParseFn( discard(parser, TokenType::IDENTIFIER) );
    CHECK( parser.parse() );
}

TEST_CASE("test single match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, true);
    parser.setParseFn( match(Production::VARNAME, parser, TokenType::IDENTIFIER) );
    CHECK( parser.parse() );
    // TODO validate the result
}

TEST_CASE("test simple match fail") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, true );
    parser.setParseFn( discard(parser, TokenType::NUMBER) );
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test maybe match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn( maybe(discard(parser, TokenType::NUMBER) ));
    CHECK( parser.parse() );
    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    parser.setParseFn( maybe(discard(parser, TokenType::IDENTIFIER) ));
    CHECK( parser.parse() );
}

TEST_CASE("test choice match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn(
        firstOf(
            { discard(parser, TokenType::NUMBER), discard(parser, TokenType::IDENTIFIER) } ));
    CHECK( parser.parse() );
}

TEST_CASE("test sequence match") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser parser(tokens,true);
    parser.setParseFn(
        allOf( Production::VARNAME,
        { discard(parser,TokenType::IDENTIFIER),
                 discard(parser, TokenType::COLON),
                 discard(parser, TokenType::ASSIGN) } ));
    CHECK( parser.parse() );
}

TEST_CASE("test sequence fail") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser parser(tokens,true);
    parser.setParseFn(
        allOf( Production::VARNAME,
        { discard(parser,TokenType::ASSIGN), discard(parser, TokenType::COLON) } ));
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test repeating match") {
    std::list<Token> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );
    Parser parser(tokens,true);
    parser.setParseFn(
        allOf( Production::VARNAME,
        { oneOrMore(Production::VARNAME, discard(parser,TokenType::IDENTIFIER)),
                 discard(parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    parser.setParseFn(
        allOf( Production::VARNAME,
        { oneOrMore(Production::VARNAME, match(Production::VARNAME, parser,TokenType::IDENTIFIER)),
                 discard(parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    parser.setParseFn(
        allOf( Production::VARNAME,
        { oneOrMore(Production::VARNAME, discard(parser,TokenType::IDENTIFIER)),
                 match(Production::VARNAME, parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    parser.setParseFn(
        allOf( Production::VARNAME,
        { oneOrMore(Production::VARNAME, match(Production::temp2, parser,TokenType::IDENTIFIER)),
                 match(Production::VARNAME, parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
}
