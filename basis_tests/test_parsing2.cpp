#include "doctest.h"

#include <vector>
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
    SPPF discardIdent = discard(TokenType::IDENTIFIER);
    SPPF discardNumber = discard(TokenType::NUMBER);
    SPPF discardComma = discard(TokenType::COMMA);
    SPPF discardColon = discard(TokenType::COLON);
    SPPF discardAlias = discard(TokenType::ALIAS);
    SPPF matchSlashIdent = match(Production::SLASH, TokenType::IDENTIFIER);
    SPPF matchSlashNumber = match(Production::SLASH, TokenType::NUMBER);
    SPPF matchSlashComma = match(Production::SLASH, TokenType::COMMA);
    SPPF matchSlashColon = match(Production::SLASH, TokenType::COLON);
    SPPF matchSlashAlias = match(Production::SLASH, TokenType::ALIAS);
    SPPF matchCaratIdent = match(Production::CARAT, TokenType::IDENTIFIER);
    SPPF matchCaratColon = match(Production::CARAT, TokenType::COLON);
    SPPF maybeDiscardNumber = maybe(discardNumber);
    SPPF maybeMatchSlashIdent = maybe(matchSlashIdent);
    SPPF oneOrMoreDiscardIdent = oneOrMore(discardIdent);
    SPPF oneOrMoreSlashIdent = oneOrMore(matchSlashIdent);
    SPPF oneOrMoreCaratIdent = oneOrMore(matchCaratIdent);
    SPPF boundOneOrMoreSlashIdent = bound(oneOrMoreSlashIdent);
    SPPF separatedSlashIdentComma = separated(matchSlashIdent, discardComma);
    SPPF anyNumberOrIdent = any(matchSlashNumber, matchSlashIdent);
    SPPF allIdentColonAlias = all(discardIdent, discardColon, discardAlias);
    SPPF allAliasColon = all(discardAlias, discardColon);
    SPPF allOneOrMoreIdentComma = all(oneOrMoreDiscardIdent, discardComma);
    SPPF allOneOrMoreSlashIdentComma = all(oneOrMoreSlashIdent, discardComma);
    SPPF allOneOrMoreIdentSlashComma = all(oneOrMoreDiscardIdent, matchSlashComma);
    SPPF allOneOrMoreCaratIdentSlashComma = all(oneOrMoreCaratIdent, matchSlashComma);
    SPPF allBoundIdentTwice = all(boundOneOrMoreSlashIdent, boundOneOrMoreSlashIdent);
    SPPF allSeparatedCaratColon = all(separatedSlashIdentComma, matchCaratColon);
    SPPF groupSlashOneOrMoreIdent = group(Production::SLASH, oneOrMoreSlashIdent);
    SPPF boundedGroupCaratIdentColonNumber = boundedGroup(Production::CARAT, matchSlashIdent, matchSlashColon, matchSlashNumber);
    SPPF allBoundedGroupComma = all(boundedGroupCaratIdentColonNumber, matchSlashComma);
}

TEST_CASE("Parsing2::test least match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, discardIdent);
    CHECK( parser.parse() );
}

TEST_CASE("Parsing2::test single match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, matchSlashIdent);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("Parsing2::test simple match fail 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, discardNumber);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("Parsing2::test maybe match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, maybeDiscardNumber);
    CHECK( parser.parse() );

    // the failed maybe test shouldn't advance the iterator, so we should be able to match the identifier
    Parser parser2(tokens, maybeMatchSlashIdent);
    CHECK( parser2.parse() );
    CHECK( *(parser2.parseTree->pToken) == *tokens.back() );
}

TEST_CASE("Parsing2::test choice match 2") {
    std::list<spToken> tokens;
    addToken(tokens, TokenType::IDENTIFIER);
    Parser parser(tokens, anyNumberOrIdent);
    CHECK( parser.parse() );
    CHECK( *parser.parseTree == ParseTree{Production::SLASH, tokens.back().get()} );
}

TEST_CASE("Parsing2::test sequence match 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parser parser(tokens, allIdentColonAlias);
    CHECK( parser.parse() );
}

TEST_CASE("Parsing2::test sequence fail 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::ALIAS } );
    Parser parser(tokens, allAliasColon);
    CHECK_FALSE( parser.parse() );
}

TEST_CASE("Parsing2::test repeating match 2") {
    std::list<spToken> tokens;
    addTokens(tokens,
        { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::COMMA } );

    Parser parser(tokens, allOneOrMoreIdentComma);
    CHECK( parser.parse() );

    Parser parser2(tokens, allOneOrMoreSlashIdentComma);
    CHECK( parser2.parse() );
    Token* IDENT_EXPECTED = tokens.front().get();
    CHECK( *parser2.parseTree ==
        ParseTree{Production::SLASH, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::SLASH, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, IDENT_EXPECTED ) )}
    );

    Parser parser3(tokens, allOneOrMoreIdentSlashComma);
    CHECK( parser3.parse() );

    Parser parser4(tokens, allOneOrMoreCaratIdentSlashComma);
    CHECK( parser4.parse() );
    Token* COMMA_EXPECTED = tokens.back().get();
    CHECK( *parser4.parseTree ==
        ParseTree{Production::CARAT, IDENT_EXPECTED,
            std::make_shared<ParseTree>( Production::CARAT, IDENT_EXPECTED,
                std::make_shared<ParseTree>(Production::CARAT, IDENT_EXPECTED,
                    std::make_shared<ParseTree>(Production::SLASH, COMMA_EXPECTED ) ))}
    );
}

TEST_CASE("Parsing2::test bound match 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser parser(tokens, boundOneOrMoreSlashIdent);
    CHECK( parser.parse() );
    Token* EXPECTED1 = tokens.front().get();
    Token* EXPECTED2 = tokens.back().get();
    ParseTree c =
        ParseTree { Production::SLASH, EXPECTED1,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED2) };
    CHECK( *parser.parseTree == c );

    Parser parser2(tokens, allBoundIdentTwice);
    CHECK( parser2.parse() );
    CHECK( *parser2.parseTree == ParseTree{
    Production::SLASH, EXPECTED1,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
           std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
               std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                   std::make_shared<ParseTree>(Production::SLASH, EXPECTED2,
                       std::make_shared<ParseTree>(Production::SLASH, EXPECTED2)  ))))});
}

TEST_CASE("Parsing2::test grouping 2") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::IDENTIFIER, TokenType::IDENTIFIER } );

    Parser parser(tokens, groupSlashOneOrMoreIdent);
    CHECK( parser.parse() );
    Token* EXPECTED = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, nullptr, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
            std::make_shared<ParseTree>(Production::SLASH, EXPECTED,
                std::make_shared<ParseTree>(Production::SLASH, EXPECTED )  ))});
}

TEST_CASE("Parsing2::test bounded group") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::NUMBER, TokenType::COMMA } );
    tokens.front()->bound = tokens.back();
    addTokens(tokens, { TokenType::ALIAS } );

    Parser parser(tokens, boundedGroupCaratIdentColonNumber);
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

    Parser parser2(tokens, allBoundedGroupComma);
    CHECK( parser2.parse() );
    ++it;
    Token* COMMA = (*it).get();
    CHECK(*parser2.parseTree == ParseTree{ Production::CARAT, nullptr,
        std::make_shared<ParseTree>(Production::SLASH, COMMA),
        std::make_shared<ParseTree>(Production::SLASH, IDENT,
            std::make_shared<ParseTree>(Production::SLASH, COLON,
                std::make_shared<ParseTree>(Production::SLASH, NUM)))});
}

TEST_CASE("Parsing2::test separated") {
    std::list<spToken> tokens;
    addTokens(tokens, { TokenType::IDENTIFIER } );

    Parser parser(tokens, separatedSlashIdentComma);
    CHECK( parser.parse() );
    Token* IDENT1 = tokens.front().get();
    CHECK(*parser.parseTree == ParseTree{ Production::SLASH, IDENT1 });

    std::list<spToken> tokens2;
    addTokens(tokens2, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER } );
    Parser parser2(tokens2, separatedSlashIdentComma);
    CHECK( parser2.parse() );
    Token* IDENT2_1 = tokens2.front().get();
    Token* IDENT2_2 = tokens2.back().get();
    CHECK(*parser2.parseTree == ParseTree{ Production::SLASH, IDENT2_1,
        std::make_shared<ParseTree>(Production::SLASH, IDENT2_2) });

    std::list<spToken> tokens3;
    addTokens(tokens3, { TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER,
                         TokenType::COMMA, TokenType::IDENTIFIER } );
    Parser parser3(tokens3, separatedSlashIdentComma);
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
    Parser parser4(tokens4, allSeparatedCaratColon);
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
    Parser parser5(tokens5, separatedSlashIdentComma);
    CHECK( !parser5.parse() );  // Should fail - trailing separator with no following element
}

TEST_CASE("Parsing2::test prefix") {
    // prefix(COLON, IDENT) - if COLON found, IDENT must follow
    SPPF prefixColonIdent = prefix(discardColon, matchSlashIdent);

    // Case 1: prefix not present - should succeed without consuming
    std::list<spToken> tokens1;
    addTokens(tokens1, { TokenType::IDENTIFIER });
    Parser parser1(tokens1, prefixColonIdent);
    CHECK( parser1.parse() );
    CHECK( parser1.parseTree == nullptr );  // Nothing consumed

    // Case 2: prefix present with valid continuation - should succeed
    std::list<spToken> tokens2;
    addTokens(tokens2, { TokenType::COLON, TokenType::IDENTIFIER });
    Parser parser2(tokens2, prefixColonIdent);
    CHECK( parser2.parse() );
    CHECK( parser2.parseTree != nullptr );
    CHECK( parser2.parseTree->production == Production::SLASH );

    // Case 3: prefix present but continuation fails - should fail
    std::list<spToken> tokens3;
    addTokens(tokens3, { TokenType::COLON, TokenType::NUMBER });
    Parser parser3(tokens3, prefixColonIdent);
    CHECK_FALSE( parser3.parse() );

    // Case 4: prefix present but nothing follows - should fail
    std::list<spToken> tokens4;
    addTokens(tokens4, { TokenType::COLON });
    Parser parser4(tokens4, prefixColonIdent);
    CHECK_FALSE( parser4.parse() );

    // Case 5: prefix with multiple continuations
    SPPF prefixColonIdentNumber = prefix(discardColon, matchSlashIdent, matchSlashNumber);

    std::list<spToken> tokens5;
    addTokens(tokens5, { TokenType::COLON, TokenType::IDENTIFIER, TokenType::NUMBER });
    Parser parser5(tokens5, prefixColonIdentNumber);
    CHECK( parser5.parse() );
    CHECK( parser5.parseTree != nullptr );

    // Case 6: prefix with multiple continuations - partial match should fail
    std::list<spToken> tokens6;
    addTokens(tokens6, { TokenType::COLON, TokenType::IDENTIFIER });
    Parser parser6(tokens6, prefixColonIdentNumber);
    CHECK_FALSE( parser6.parse() );

    // Case 7: use prefix in a sequence with other elements
    SPPF allIdentPrefixColonNumber = all(matchSlashIdent, prefix(discardColon, matchSlashNumber));

    std::list<spToken> tokens7a;
    addTokens(tokens7a, { TokenType::IDENTIFIER });
    Parser parser7a(tokens7a, allIdentPrefixColonNumber);
    CHECK( parser7a.parse() );  // IDENT present, prefix not present - OK

    std::list<spToken> tokens7b;
    addTokens(tokens7b, { TokenType::IDENTIFIER, TokenType::COLON, TokenType::NUMBER });
    Parser parser7b(tokens7b, allIdentPrefixColonNumber);
    CHECK( parser7b.parse() );  // IDENT present, prefix present with NUMBER - OK

    std::list<spToken> tokens7c;
    addTokens(tokens7c, { TokenType::IDENTIFIER, TokenType::COLON });
    Parser parser7c(tokens7c, allIdentPrefixColonNumber);
    CHECK_FALSE( parser7c.parse() );  // IDENT present, prefix present but no NUMBER - FAIL
}

