#include "doctest.h"

#include <string>
#include <sstream>

#include "../Lexer.h"
#include "../CompilerContext.h"

using namespace basis;

// Utility function to test lexing a single token
void testLexSingleToken(const std::string& input, TokenType expectedType) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    CompilerContext context;
    CHECK(lexer.scan(context));
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == expectedType);
}

TEST_CASE("test lex number") {
    testLexSingleToken("1234", TokenType::NUMBER);
}

TEST_CASE("test lex decimal") {
    testLexSingleToken("1234.5678", TokenType::DECIMAL);
}

TEST_CASE("test lex hex") {
    testLexSingleToken("0x1234", TokenType::HEXNUMBER);
}

TEST_CASE("test lex identifier") {
    testLexSingleToken("foobar", TokenType::IDENTIFIER);
}

TEST_CASE("test lex string") {
    testLexSingleToken("\"foobar\"", TokenType::STRING);
}

TEST_CASE("test lex command") {
    testLexSingleToken(".cmd foobar", TokenType::COMMAND);
}