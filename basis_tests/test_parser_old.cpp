#include "doctest.h"

#include <vector>
#include "../Parser_old.h"
#include "../Grammar.h"

/*
 * n.b. None of the Production or Token types here have any bearing on the actual grammar.  For the tests here
 *      they're just placeholders.  This test the 2-dimensional combinator parsing framework. The actual language
 *      parser will be tested in "test_grammar.cpp"
 */

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

TEST_CASE("test bound match") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    tokens.front().bound = &tokens.back();
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    Parser parser(tokens,true);
    parser.setParseFn( bound(oneOrMore(match(Production::VARNAME, parser,TokenType::IDENTIFIER))) );
    CHECK( parser.parse() );
    // can't reuse the expected items because one has a bound and the other doesn't
    Token& EXPECTED1 = tokens.front();
    Token& EXPECTED2 = tokens.back();
    ParseTree c =
        ParseTree { Production::VARNAME, &EXPECTED1,
            std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2) };
    CHECK( *parser.parseTree == c );
    parser.setParseFn( all( {
       // should collect the first two only
       bound(oneOrMore(match(Production::VARNAME, parser,TokenType::IDENTIFIER))),
       // should collect the remainder
       bound(oneOrMore(match(Production::VARNAME, parser,TokenType::IDENTIFIER)))  }));
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{
    Production::VARNAME, &EXPECTED1,
           std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
           std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
               std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
                   std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
                       std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2)  ))))});
}

TEST_CASE("test grouping") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    Parser parser(tokens,true);
    parser.setParseFn( group(Production::VARNAME, oneOrMore(match(Production::VARNAME, parser,TokenType::IDENTIFIER))) );
    CHECK( parser.parse() );
    Token& EXPECTED = tokens.front();
    CHECK(*parser.parseTree == ParseTree{ Production::VARNAME, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED,
            std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED,
                std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED )  ))});
}
