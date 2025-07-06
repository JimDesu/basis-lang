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

basis::Lexer lexInput(const std::string& input, CompilerContext& context) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    CHECK(lexer.scan(context));
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

TEST_CASE("test lex comment produces nothing") {
    std::string lexInput = " ;fish sticks";
    std::istringstream inputStream(lexInput);
    basis::Lexer lexer(inputStream);
    CompilerContext context;
    CHECK(lexer.scan(context));
    CHECK(lexer.output.empty());
}

TEST_CASE("test lex unclosed string produces no token") {
    std::string input = "\"unclosed string";
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    CompilerContext context;
    CHECK(!lexer.scan(context));
    CHECK(lexer.output.empty());
}

