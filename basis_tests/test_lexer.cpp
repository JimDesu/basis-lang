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

// Helper function to test a complete token with prefix variations
void testSingleToken(const std::string& tokenText, TokenType tokenType, CompilerContext& context) {
    static const std::string whitespacePrefix = "\t";
    static const std::string commentPrefix = " ;fish sticks\n";

    // Test as single token
    testLexSingleToken(tokenText, tokenType);

    // Test with whitespace prefix and verify column number
    CHECK(lexTokenAfterPrefix(whitespacePrefix, tokenText, tokenType, context)
          .output.front().columnNumber == context.options.tabWidth + 1);

    // Test with comment prefix
    lexTokenAfterPrefix(commentPrefix, tokenText, tokenType, context);
}

TEST_CASE("test lex all token types") {
    int tabWidth = 4;
    CompilerContext context;
    context.options.tabWidth = tabWidth;

    testSingleToken("1234", TokenType::NUMBER, context);
    testSingleToken("1234.5678", TokenType::DECIMAL, context);
    testSingleToken("0x1234", TokenType::HEXNUMBER, context);
    testSingleToken("foobar", TokenType::IDENTIFIER, context);
    testSingleToken("\"foobar\"", TokenType::STRING, context);
    testSingleToken("&", TokenType::AMPERSAND, context);
    testSingleToken("@", TokenType::AMPHORA, context);
    testSingleToken("<-", TokenType::ASSIGN, context);
    testSingleToken("*", TokenType::ASTERISK, context);
    testSingleToken("!", TokenType::BANG, context);
    testSingleToken("^", TokenType::CARAT, context);
    testSingleToken(",", TokenType::COMMA, context);
    testSingleToken("::", TokenType::DCOLON, context);
    testSingleToken(":<", TokenType::COLANGLE, context);
    testSingleToken(":", TokenType::COLON, context);
    testSingleToken("=", TokenType::EQUALS, context);
    testSingleToken("<", TokenType::LANGLE, context);
    testSingleToken("{", TokenType::LBRACE, context);
    testSingleToken("[", TokenType::LBRACKET, context);
    testSingleToken("(", TokenType::LPAREN, context);
    testSingleToken("-", TokenType::MINUS, context);
    testSingleToken("+", TokenType::PLUS, context);
    testSingleToken("?<", TokenType::QLANGLE, context);
    testSingleToken("?", TokenType::QMARK, context);
    testSingleToken(">", TokenType::RANGLE, context);
    testSingleToken("}", TokenType::RBRACE, context);
    testSingleToken("]", TokenType::RBRACKET, context);
    testSingleToken(")", TokenType::RPAREN, context);
    testSingleToken("/", TokenType::SLASH, context);
    testSingleToken(".cmd foobar", TokenType::COMMAND, context);
    testSingleToken(".class foobar", TokenType::CLASS, context);
    testSingleToken(".domain foobar", TokenType::DOMAIN, context);
    testSingleToken(".enum foobar", TokenType::ENUMERATION, context);
    testSingleToken(".intrinsic foobar", TokenType::INTRINSIC, context);
    testSingleToken(".object foobar", TokenType::OBJECT, context);
    testSingleToken(".record foobar", TokenType::RECORD, context);
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

    // Test invalid reserved words produces no token (failure case)
    CHECK(lexInput(".invalidReservedWord", context, false).output.empty());
    CHECK(lexInput(".cm", context, false).output.empty());
    CHECK(lexInput(".cmda", context, false).output.empty());
    CHECK(lexInput(".cmd2", context, false).output.empty());
    CHECK(lexInput(".cmd_", context, false).output.empty());
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