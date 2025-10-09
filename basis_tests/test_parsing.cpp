#include "doctest.h"

#include <vector>
#include "../Grammar.h"
#include "../Parsing.h"

/*
 * n.b. None of the Production or Token types here have any bearing on the actual grammar.  For the tests here
 *      they're just placeholders.  This test the 2-dimensional combinator parsing framework. The actual language
 *      parser will be tested in "test_grammar.cpp"
 */

using namespace basis;

namespace {
    void addToken(std::list<spToken>& tokens, const TokenType type) {
        spToken token = std::make_shared<Token>();
        token->type = type;
        tokens.push_back(token);
    }

    void addTokens(std::list<spToken>& tokens, std::vector<TokenType> types) {
        for ( auto& type : types ) {
            addToken(tokens, type);
        }
    }
}

TEST_CASE("test least match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Discard<TokenType::IDENTIFIER>> parser(tokens);
    CHECK( parser.parse() );
}

TEST_CASE("test single match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Match<Production::SLASH, TokenType::IDENTIFIER>> parser(tokens);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("test simple match fail 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Discard<TokenType::NUMBER>> parser(tokens);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test maybe match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Maybe<Discard<TokenType::NUMBER>>> parser(tokens);
    CHECK( parser.parse() );
    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    Parser<Maybe<Match<Production::SLASH, TokenType::IDENTIFIER>>> parser2(tokens);
    CHECK( parser2.parse() );
    CHECK( *(parser2.parseTree->pToken) == *tokens.back() );
}

TEST_CASE("test choice match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser<Any<Match<Production::SLASH, TokenType::NUMBER>,
                Match<Production::SLASH, TokenType::IDENTIFIER>>> parser(tokens);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("test sequence match 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parser<All<Discard<TokenType::IDENTIFIER>,
                Discard<TokenType::COLON>,
                Discard<TokenType::ALIAS>>> parser(tokens);
    CHECK( parser.parse() );
}

TEST_CASE("test sequence fail 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parser<All<Discard<TokenType::ALIAS>,
                Discard<TokenType::COLON>>> parser(tokens);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test repeating match 2") {
    std::list<spToken> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );

    Parser<All<OneOrMore<Discard<TokenType::IDENTIFIER>>,
                Discard<TokenType::COMMA>>> parser(tokens);
    CHECK( parser.parse() );

    Parser<All<OneOrMore<Match<Production::SLASH, TokenType::IDENTIFIER>>,
                Discard<TokenType::COMMA>>> parser2(tokens);
    CHECK( parser2.parse() );
    Token* IDENT_EXPECTED = tokens.front().get();
    CHECK( *parser2.parseTree ==
        ParseTree{Production::SLASH, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::SLASH, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, IDENT_EXPECTED ) )}
    );

    Parser<All<OneOrMore<Discard<TokenType::IDENTIFIER>>,
                Match<Production::SLASH, TokenType::COMMA>>> parser3(tokens);
    CHECK( parser3.parse() );

    Parser<All<OneOrMore<Match<Production::CARAT, TokenType::IDENTIFIER>>,
                Match<Production::SLASH, TokenType::COMMA>>> parser4(tokens);
    CHECK( parser4.parse() );
    Token* COMMA_EXPECTED = tokens.back().get();
    CHECK( *parser4.parseTree ==
        ParseTree{Production::CARAT, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::CARAT, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::CARAT, IDENT_EXPECTED,
                    std::make_shared<ParseTree>(Production::SLASH, COMMA_EXPECTED ) ))}
    );
}

TEST_CASE("test bound match 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser<Bound<OneOrMore<Match<Production::SLASH, TokenType::IDENTIFIER>>>> parser(tokens);
    CHECK( parser.parse() );
    // can't reuse the expected items because one has a bound and the other doesn't
    Token* EXPECTED1 = tokens.front().get();
    Token* EXPECTED2 = tokens.back().get();
    ParseTree c =
        ParseTree { Production::SLASH, EXPECTED1,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED2) };
    CHECK( *parser.parseTree == c );

    Parser<All<Bound<OneOrMore<Match<Production::SLASH, TokenType::IDENTIFIER>>>,
                Bound<OneOrMore<Match<Production::SLASH, TokenType::IDENTIFIER>>>>> parser2(tokens);
    CHECK( parser2.parse() );
    CHECK( *parser2.parseTree == ParseTree{
    Production::SLASH, EXPECTED1,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
               std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                   std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                       std::make_shared<ParseTree>(Production::SLASH, EXPECTED2)  ))))});
}

TEST_CASE("test grouping 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser<Group<Production::SLASH, OneOrMore<Match<Production::SLASH, TokenType::IDENTIFIER>>>> parser(tokens);
    CHECK( parser.parse() );
    Token* EXPECTED = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, EXPECTED )  ))});
}

TEST_CASE("test bounded group") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::NUMBER, TokenType::COMMA } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::ALIAS } );

    Parser<BoundedGroup<Production::CARAT,
        Match<Production::SLASH, TokenType::IDENTIFIER>,
        Match<Production::SLASH, TokenType::COLON>,
        Match<Production::SLASH, TokenType::NUMBER>>> parser(tokens);
    CHECK( parser.parse() );
    Token* IDENT = tokens.front().get();
    auto it = tokens.begin();
    ++it;
    Token* COLON = (*it).get();
    ++it;
    Token* NUM = (*it).get();
    CHECK(*parser.parseTree == ParseTree{ Production::CARAT, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, IDENT,
            std::make_shared<ParseTree>(Production::SLASH, COLON,
                std::make_shared<ParseTree>(Production::SLASH, NUM)))});

    Parser<All<BoundedGroup<Production::CARAT,
                   Match<Production::SLASH, TokenType::IDENTIFIER>,
                   Match<Production::SLASH, TokenType::COLON>,
                   Match<Production::SLASH, TokenType::NUMBER>>,
               Match<Production::SLASH, TokenType::COMMA>>> parser2(tokens);
    CHECK( parser2.parse() );
    ++it;
    Token* COMMA = (*it).get();
    CHECK(*parser2.parseTree == ParseTree{ Production::CARAT, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, COMMA),
        std::make_shared<ParseTree>(Production::SLASH, IDENT,
            std::make_shared<ParseTree>(Production::SLASH, COLON,
                std::make_shared<ParseTree>(Production::SLASH, NUM)))});
}

TEST_CASE("test separated") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER } );

    Parser<Separated<Match<Production::SLASH, TokenType::IDENTIFIER>,
                     Discard<TokenType::COMMA>>> parser(tokens);
    CHECK( parser.parse() );
    Token* IDENT1 = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, IDENT1 });

    std::list<spToken> tokens2;
    addTokens(tokens2, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER } );
    Parser<Separated<Match<Production::SLASH, TokenType::IDENTIFIER>,
                     Discard<TokenType::COMMA>>> parser2(tokens2);
    CHECK( parser2.parse() );
    Token* IDENT2_1 = tokens2.front().get();
    Token* IDENT2_2 = tokens2.back().get();
    CHECK(*parser2.parseTree == ParseTree{ Production::SLASH, IDENT2_1,
        std::make_shared<ParseTree>(Production::SLASH, IDENT2_2) });

    std::list<spToken> tokens3;
    addTokens(tokens3, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER,
                         TokenType::COMMA, TokenType::IDENTIFIER } );
    Parser<Separated<Match<Production::SLASH, TokenType::IDENTIFIER>,
                     Discard<TokenType::COMMA>>> parser3(tokens3);
    CHECK( parser3.parse() );
    auto it3 = tokens3.begin();
    Token* IDENT3_1 = (*it3).get();
    ++it3; ++it3;
    Token* IDENT3_2 = (*it3).get();
    ++it3; ++it3;
    Token* IDENT3_3 = (*it3).get();
    CHECK(*parser3.parseTree == ParseTree{ Production::SLASH, IDENT3_1,
        std::make_shared<ParseTree>(Production::SLASH, IDENT3_2,
            std::make_shared<ParseTree>(Production::SLASH, IDENT3_3)) });

    std::list<spToken> tokens4;
    addTokens(tokens4, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER,
                         TokenType::COLON } );
    Parser<All<Separated<Match<Production::SLASH, TokenType::IDENTIFIER>,
                         Discard<TokenType::COMMA>>,
               Match<Production::CARAT, TokenType::COLON>>> parser4(tokens4);
    CHECK( parser4.parse() );
    auto it4 = tokens4.begin();
    Token* IDENT4_1 = (*it4).get();
    ++it4; ++it4;
    Token* IDENT4_2 = (*it4).get();
    ++it4;
    Token* COLON4 = (*it4).get();
    CHECK(*parser4.parseTree == ParseTree{ Production::SLASH, IDENT4_1,
        std::make_shared<ParseTree>(Production::SLASH, IDENT4_2,
            std::make_shared<ParseTree>(Production::CARAT, COLON4)) });

    std::list<spToken> tokens5;
    addTokens(tokens5, { TokenType::IDENTIFIER, TokenType::COMMA } );
    Parser<Separated<Match<Production::SLASH, TokenType::IDENTIFIER>,
                     Discard<TokenType::COMMA>>> parser5(tokens5);
    CHECK( !parser5.parse() );  // Should fail - trailing separator with no following element
}
