#include "doctest.h"

#include <vector>
#include "../Parsing.h"

/*
 * n.b. None of the Production or Token types here have any bearing on the actual grammar.  For the tests here
 *      they're just placeholders.  This test the 2-dimensional combinator parsing framework. The actual language
 *      parser will be tested in "test_grammar.cpp"
 */

using namespace basis;

namespace {
    void addToken(std::list<Token>& tokens, const TokenType type) {
        tokens.emplace_back();
        tokens.back().type = type;
    }

    void addTokens(std::list<Token>& tokens, std::vector<TokenType> types) {
        for ( auto& type : types ) {
            addToken(tokens, type);
        }
    }
}

TEST_CASE("test least match 2") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Discard<TokenType::IDENTIFIER>> parser(tokens);
    CHECK( parser.parse() );
}

TEST_CASE("test single match 2") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Match<Production::VARNAME, TokenType::IDENTIFIER>> parser(tokens);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::VARNAME, &tokens.back()} );
}

TEST_CASE("test simple match fail 2") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Discard<TokenType::NUMBER>> parser(tokens);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test maybe match 2") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Maybe<Discard<TokenType::NUMBER>>> parser(tokens);
    CHECK( parser.parse() );
    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    Parser<Maybe<Match<Production::VARNAME, TokenType::IDENTIFIER>>> parser2(tokens);
    CHECK( parser2.parse() );
    CHECK( *(parser2.parseTree->pToken) == tokens.back() );
}

TEST_CASE("test choice match 2") {
    std::list<Token> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Any<Match<Production::VARNAME, TokenType::NUMBER>,
                Match<Production::VARNAME, TokenType::IDENTIFIER>>> parser(tokens);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::VARNAME, &tokens.back()} );
}

TEST_CASE("test sequence match 2") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser<All<Discard<TokenType::IDENTIFIER>,
                Discard<TokenType::COLON>,
                Discard<TokenType::ASSIGN>>> parser(tokens);
    CHECK( parser.parse() );
}

TEST_CASE("test sequence fail 2") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ASSIGN } );
    Parser<All<Discard<TokenType::ASSIGN>,
                Discard<TokenType::COLON>>> parser(tokens);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test repeating match 2") {
    std::list<Token> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );

    Parser<All<OneOrMore<Discard<TokenType::IDENTIFIER>>,
                Discard<TokenType::COMMA>>> parser(tokens);
    CHECK( parser.parse() );

    Parser<All<OneOrMore<Match<Production::VARNAME, TokenType::IDENTIFIER>>,
                Discard<TokenType::COMMA>>> parser2(tokens);
    CHECK( parser2.parse() );
    Token IDENT_EXPECTED = tokens.front();
    CHECK( *parser2.parseTree ==
        ParseTree{Production::VARNAME, &IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::VARNAME, &IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::VARNAME, &IDENT_EXPECTED ) )}
    );

    Parser<All<OneOrMore<Discard<TokenType::IDENTIFIER>>,
                Match<Production::VARNAME, TokenType::COMMA>>> parser3(tokens);
    CHECK( parser3.parse() );

    Parser<All<OneOrMore<Match<Production::temp2, TokenType::IDENTIFIER>>,
                Match<Production::VARNAME, TokenType::COMMA>>> parser4(tokens);
    CHECK( parser4.parse() );
    Token COMMA_EXPECTED = tokens.back();
    CHECK( *parser4.parseTree ==
        ParseTree{Production::temp2, &IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::temp2, &IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::temp2, &IDENT_EXPECTED,
                    std::make_shared<ParseTree>(Production::VARNAME, &COMMA_EXPECTED ) ))}
    );
}

TEST_CASE("test bound match 2") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    tokens.front().bound = &tokens.back();
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser<Bound<OneOrMore<Match<Production::VARNAME, TokenType::IDENTIFIER>>>> parser(tokens);
    CHECK( parser.parse() );
    // can't reuse the expected items because one has a bound and the other doesn't
    Token& EXPECTED1 = tokens.front();
    Token& EXPECTED2 = tokens.back();
    ParseTree c =
        ParseTree { Production::VARNAME, &EXPECTED1,
            std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2) };
    CHECK( *parser.parseTree == c );

    Parser<All<Bound<OneOrMore<Match<Production::VARNAME, TokenType::IDENTIFIER>>>,
                Bound<OneOrMore<Match<Production::VARNAME, TokenType::IDENTIFIER>>>>> parser2(tokens);
    CHECK( parser2.parse() );
    CHECK( *parser2.parseTree == ParseTree{
    Production::VARNAME, &EXPECTED1,
           std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
           std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
               std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
                   std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2,
                       std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED2)  ))))});
}

TEST_CASE("test grouping 2") {
    std::list<Token> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser<Group<Production::VARNAME, OneOrMore<Match<Production::VARNAME, TokenType::IDENTIFIER>>>> parser(tokens);
    CHECK( parser.parse() );
    Token& EXPECTED = tokens.front();
    CHECK(*parser.parseTree == ParseTree{ Production::VARNAME, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED,
            std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED,
                std::make_shared<ParseTree>(Production::VARNAME, &EXPECTED )  ))});
}
