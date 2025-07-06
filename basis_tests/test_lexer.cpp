#include "doctest.h"

#include <string>
#include <sstream>

#include "../Lexer.h"
#include "../CompilerContext.h"

using namespace basis;

void testLexSingleToken(const std::string& input, TokenType expectedType) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    CompilerContext context;
    CHECK(lexer.scan(context));
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == expectedType);
}

basis::Lexer lexInput(const std::string& input, CompilerContext& context, bool expectSuccess = true) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    bool scanResult = lexer.scan(context);
    CHECK_EQ(scanResult, expectSuccess);
    return lexer;
}

// Helper function to check token type after prefix
basis::Lexer lexTokenAfterPrefix(const std::string& prefix, const std::string& tokenText, TokenType expectedType, CompilerContext& context) {
    std::string input = prefix + tokenText;
    basis::Lexer lexer = lexInput(input, context);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == expectedType);
    return lexer;
}

TEST_CASE("test lex all token types") {
    int tabWidth = 4;
    std::string whitespacePrefix = "\t";
    std::string commentPrefix = " ;fish sticks\n";
    CompilerContext context;
    context.options.tabWidth = tabWidth;

    // Test number token
    std::string numberToken = "1234";
    TokenType numberType = TokenType::NUMBER;
    testLexSingleToken(numberToken, numberType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, numberToken, numberType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, numberToken, numberType, context);

    // Test decimal token
    std::string decimalToken = "1234.5678";
    TokenType decimalType = TokenType::DECIMAL;
    testLexSingleToken(decimalToken, decimalType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, decimalToken, decimalType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, decimalToken, decimalType, context);

    // Test hexadecimal token
    std::string hexToken = "0x1234";
    TokenType hexType = TokenType::HEXNUMBER;
    testLexSingleToken(hexToken, hexType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, hexToken, hexType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, hexToken, hexType, context);

    // Test identifier token
    std::string identifierToken = "foobar";
    TokenType identifierType = TokenType::IDENTIFIER;
    testLexSingleToken(identifierToken, identifierType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, identifierToken, identifierType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, identifierToken, identifierType, context);

    // Test string token
    std::string stringToken = "\"foobar\"";
    TokenType stringType = TokenType::STRING;
    testLexSingleToken(stringToken, stringType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, stringToken, stringType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, stringToken, stringType, context);

    // Test command token
    std::string commandToken = ".cmd foobar";
    TokenType commandType = TokenType::COMMAND;
    testLexSingleToken(commandToken, commandType);
    CHECK(
        lexTokenAfterPrefix(whitespacePrefix, commandToken, commandType, context)
        .output.front().columnNumber == context.options.tabWidth + 1);
    lexTokenAfterPrefix(commentPrefix, commandToken, commandType, context);
}

TEST_CASE("test lex single token negative test cases") {
    CompilerContext context;

    // Test comment line produces no token (success case)
    CHECK(lexInput(" ;fish sticks", context).output.empty());

    // Test unclosed string produces no token (failure case)
    CHECK(lexInput("\"unclosed string", context, false).output.empty());

    // Test invalid reserved word produces no token (failure case)
    CHECK(lexInput(".invalidReservedWord", context, false).output.empty());

    // Test invalid decimal produces no token (failure case)
    CHECK(lexInput("1234.5678.9", context, false).output.empty());

    // Test invalid character after string produces no token (failure case)
    CHECK(lexInput("\"x\"1", context, false).output.empty());
    CHECK(lexInput("\"x\"a", context, false).output.empty());
}

TEST_CASE("test lex multiple tokens") {
    CompilerContext context;
    std::string input = "1234 5678";
    basis::Lexer lexer = lexInput(input, context);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == TokenType::NUMBER);
    CHECK(lexer.output.back().type == TokenType::NUMBER);
}

TEST_CASE("test lex boundary detection") {
    CompilerContext context;
    std::string input = "a b\n c\nd\n e\nf\n";
    basis::Lexer lexer = lexInput(input, context);
    CHECK(!lexer.output.empty());
    CHECK_EQ(lexer.output.size(),6);
    std::vector<Token*> tokens;
    for (auto& token : lexer.output) {
        tokens.push_back(&token);
    }
    CHECK_EQ(tokens[0]->bound, tokens[3]);
    CHECK_EQ(tokens[1]->bound, tokens[2]);
    CHECK_EQ(tokens[2]->bound, tokens[3]);
    CHECK_EQ(tokens[3]->bound, tokens[5]);
    CHECK_EQ(tokens[4]->bound, tokens[5]);
}