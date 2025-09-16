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
    CHECK( *parser.parseTree == ParseTree{Production::VARNAME, &tokens.back()} );
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
    parser.setParseFn( maybe(match(Production::VARNAME, parser, TokenType::IDENTIFIER) ));
    CHECK( parser.parse() );
    CHECK( *(parser.parseTree->pToken) == tokens.back() );
}

TEST_CASE("test choice match") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens,true);
    parser.setParseFn(
        any(
            { match(Production::VARNAME, parser, TokenType::NUMBER),
                     match(Production::VARNAME, parser, TokenType::IDENTIFIER) } ));
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::VARNAME, &tokens.back()} );
}

TEST_CASE("test sequence match") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser parser(tokens,true);
    parser.setParseFn(
        all( { discard(parser,TokenType::IDENTIFIER),
                 discard(parser, TokenType::COLON),
                 discard(parser, TokenType::ASSIGN) } ));
    CHECK( parser.parse() );
}

TEST_CASE("test sequence fail") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser parser(tokens,true);
    parser.setParseFn(
        all( { discard(parser,TokenType::ASSIGN), discard(parser, TokenType::COLON) } ));
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test repeating match") {
    std::list<Token> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );
    Parser parser(tokens,true);
    parser.setParseFn(
        all( { oneOrMore(discard(parser,TokenType::IDENTIFIER)),
                 discard(parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    parser.setParseFn(
        all( { oneOrMore(match(Production::VARNAME, parser,TokenType::IDENTIFIER)),
                 discard(parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    Token IDENT_EXPECTED = tokens.front();
    CHECK( *parser.parseTree ==
        ParseTree{Production::VARNAME, &IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::VARNAME, &IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::VARNAME, &IDENT_EXPECTED ) )}
    );
    parser.setParseFn(
        all( { oneOrMore(discard(parser,TokenType::IDENTIFIER)),
                 match(Production::VARNAME, parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    parser.setParseFn(
        all( { oneOrMore(match(Production::temp2, parser,TokenType::IDENTIFIER)),
                 match(Production::VARNAME, parser, TokenType::COMMA) } ));
    CHECK( parser.parse() );
    Token COMMA_EXPECTED = tokens.back();
    CHECK( *parser.parseTree ==
        ParseTree{Production::temp2, &IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::temp2, &IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::temp2, &IDENT_EXPECTED,
                    std::make_shared<ParseTree>(Production::VARNAME, &COMMA_EXPECTED ) ))}
    );
}
