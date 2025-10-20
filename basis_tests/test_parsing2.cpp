#include "doctest.h"

#include <vector>
#include "../Grammar2.h"
#include "../Parsing2.h"

/*
 * Tests for Parser2 - the inheritance/function object based parser
 * These tests mirror test_parsing.cpp but use the new Parser2 implementation
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

    // shared combinators
    Discard discardIdent(TokenType::IDENTIFIER);
    Discard discardNumber(TokenType::NUMBER);
    Discard discardComma(TokenType::COMMA);
    Discard discardColon(TokenType::COLON);
    Discard discardAlias(TokenType::ALIAS);
    Match matchSlashIdent(Production::SLASH, TokenType::IDENTIFIER);
    Match matchSlashNumber(Production::SLASH, TokenType::NUMBER);
    Match matchSlashComma(Production::SLASH, TokenType::COMMA);
    Match matchSlashColon(Production::SLASH, TokenType::COLON);
    Match matchSlashAlias(Production::SLASH, TokenType::ALIAS);
    Match matchCaratIdent(Production::CARAT, TokenType::IDENTIFIER);
    Match matchCaratColon(Production::CARAT, TokenType::COLON);
    Maybe maybeDiscardNumber(discardNumber);
    Maybe maybeMatchSlashIdent(matchSlashIdent);
    OneOrMore oneOrMoreDiscardIdent(discardIdent);
    OneOrMore oneOrMoreSlashIdent(matchSlashIdent);
    OneOrMore oneOrMoreCaratIdent(matchCaratIdent);
    Bound boundOneOrMoreSlashIdent(oneOrMoreSlashIdent);
    Separated separatedSlashIdentComma(matchSlashIdent, discardComma);

    // Commonly used Any combinators
    Any anyNumberOrIdent({&matchSlashNumber, &matchSlashIdent});

    // Commonly used All combinators
    All allIdentColonAlias({&discardIdent, &discardColon, &discardAlias});
    All allAliasColon({&discardAlias, &discardColon});
    All allOneOrMoreIdentComma({&oneOrMoreDiscardIdent, &discardComma});
    All allOneOrMoreSlashIdentComma({&oneOrMoreSlashIdent, &discardComma});
    All allOneOrMoreIdentSlashComma({&oneOrMoreDiscardIdent, &matchSlashComma});
    All allOneOrMoreCaratIdentSlashComma({&oneOrMoreCaratIdent, &matchSlashComma});
    All allBoundIdentTwice({&boundOneOrMoreSlashIdent, &boundOneOrMoreSlashIdent});
    All allSeparatedCaratColon({&separatedSlashIdentComma, &matchCaratColon});

    // Commonly used Group combinators
    Group groupSlashOneOrMoreIdent(Production::SLASH, oneOrMoreSlashIdent);

    // Commonly used BoundedGroup combinators
    BoundedGroup boundedGroupCaratIdentColonNumber(Production::CARAT, {&matchSlashIdent, &matchSlashColon, &matchSlashNumber});
    All allBoundedGroupComma({&boundedGroupCaratIdentColonNumber, &matchSlashComma});
}

TEST_CASE("test least match 2 - Parser2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parsing2 parser(tokens, discardIdent);
    CHECK( parser.parse() );
}

TEST_CASE("test single match 2 - Parser2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parsing2 parser(tokens, matchSlashIdent);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("test simple match fail 2 - Parser2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parsing2 parser(tokens, discardNumber);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test maybe match 2 - Parser2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parsing2 parser(tokens, maybeDiscardNumber);
    CHECK( parser.parse() );

    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    Parsing2 parser2(tokens, maybeMatchSlashIdent);
    CHECK( parser2.parse() );
    CHECK( *(parser2.parseTree->pToken) == *tokens.back() );
}

TEST_CASE("test choice match 2 - Parser2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parsing2 parser(tokens, anyNumberOrIdent);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("test sequence match 2 - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parsing2 parser(tokens, allIdentColonAlias);
    CHECK( parser.parse() );
}

TEST_CASE("test sequence fail 2 - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parsing2 parser(tokens, allAliasColon);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("test repeating match 2 - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );

    Parsing2 parser(tokens, allOneOrMoreIdentComma);
    CHECK( parser.parse() );

    Parsing2 parser2(tokens, allOneOrMoreSlashIdentComma);
    CHECK( parser2.parse() );
    Token* IDENT_EXPECTED = tokens.front().get();
    CHECK( *parser2.parseTree ==
        ParseTree{Production::SLASH, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::SLASH, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, IDENT_EXPECTED ) )}
    );

    Parsing2 parser3(tokens, allOneOrMoreIdentSlashComma);
    CHECK( parser3.parse() );

    Parsing2 parser4(tokens, allOneOrMoreCaratIdentSlashComma);
    CHECK( parser4.parse() );
    Token* COMMA_EXPECTED = tokens.back().get();
    CHECK( *parser4.parseTree ==
        ParseTree{Production::CARAT, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::CARAT, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::CARAT, IDENT_EXPECTED,
                    std::make_shared<ParseTree>(Production::SLASH, COMMA_EXPECTED ) ))}
    );
}

TEST_CASE("test bound match 2 - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parsing2 parser(tokens, boundOneOrMoreSlashIdent);
    CHECK( parser.parse() );
    Token* EXPECTED1 = tokens.front().get();
    Token* EXPECTED2 = tokens.back().get();
    ParseTree c =
        ParseTree { Production::SLASH, EXPECTED1,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED2) };
    CHECK( *parser.parseTree == c );

    Parsing2 parser2(tokens, allBoundIdentTwice);
    CHECK( parser2.parse() );
    CHECK( *parser2.parseTree == ParseTree{
    Production::SLASH, EXPECTED1,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
               std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                   std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                       std::make_shared<ParseTree>(Production::SLASH, EXPECTED2)  ))))});
}

TEST_CASE("test grouping 2 - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parsing2 parser(tokens, groupSlashOneOrMoreIdent);
    CHECK( parser.parse() );
    Token* EXPECTED = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, EXPECTED )  ))});
}

TEST_CASE("test bounded group - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::NUMBER, TokenType::COMMA } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::ALIAS } );

    Parsing2 parser(tokens, boundedGroupCaratIdentColonNumber);
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

    Parsing2 parser2(tokens, allBoundedGroupComma);
    CHECK( parser2.parse() );
    ++it;
    Token* COMMA = (*it).get();
    CHECK(*parser2.parseTree == ParseTree{ Production::CARAT, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, COMMA),
        std::make_shared<ParseTree>(Production::SLASH, IDENT,
            std::make_shared<ParseTree>(Production::SLASH, COLON,
                std::make_shared<ParseTree>(Production::SLASH, NUM)))});
}

TEST_CASE("test separated - Parser2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER } );

    Parsing2 parser(tokens, separatedSlashIdentComma);
    CHECK( parser.parse() );
    Token* IDENT1 = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, IDENT1 });

    std::list<spToken> tokens2;
    addTokens(tokens2, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER } );
    Parsing2 parser2(tokens2, separatedSlashIdentComma);
    CHECK( parser2.parse() );
    Token* IDENT2_1 = tokens2.front().get();
    Token* IDENT2_2 = tokens2.back().get();
    CHECK(*parser2.parseTree == ParseTree{ Production::SLASH, IDENT2_1,
        std::make_shared<ParseTree>(Production::SLASH, IDENT2_2) });

    std::list<spToken> tokens3;
    addTokens(tokens3, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER,
                         TokenType::COMMA, TokenType::IDENTIFIER } );
    Parsing2 parser3(tokens3, separatedSlashIdentComma);
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
    Parsing2 parser4(tokens4, allSeparatedCaratColon);
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
    Parsing2 parser5(tokens5, separatedSlashIdentComma);
    CHECK( !parser5.parse() );  // Should fail - trailing separator with no following element
}

