#include "doctest.h"

#include <string>
#include <sstream>
#include <vector>

#include "../Lexer.h"

using namespace basis;

void testLexSingleToken(const std::string& input, TokenType expectedType) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    CHECK(lexer.scan());
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == expectedType);
}

basis::Lexer lexInput(const std::string& input, bool expectSuccess = true) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream);
    bool scanResult = lexer.scan();
    CHECK_EQ(scanResult, expectSuccess);
    return lexer;
}

// Helper function to check token type after prefix
basis::Lexer lexTokenAfterPrefix(const std::string& prefix, const std::string& tokenText, TokenType expectedType) {
    std::string input = prefix + tokenText;
    basis::Lexer lexer = lexInput(input);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == expectedType);
    return lexer;
}

// Helper function to test a complete token with prefix variations
void testSingleToken(const std::string& tokenText, TokenType tokenType) {
    static const std::string whitespacePrefix = "\t";
    static const std::string commentPrefix = " ;fish sticks\n";

    // Test as single token
    testLexSingleToken(tokenText, tokenType);

    // Test with whitespace prefix and verify column number
    CHECK(lexTokenAfterPrefix(whitespacePrefix, tokenText, tokenType).output.front().columnNumber == 2);

    // Test with comment prefix
    lexTokenAfterPrefix(commentPrefix, tokenText, tokenType);
}

TEST_CASE("test lex all token types") {
    testSingleToken("-1234", TokenType::NUMBER);
    testSingleToken("-1234.5678", TokenType::DECIMAL);
    testSingleToken("1234", TokenType::NUMBER);
    testSingleToken("1234.5678", TokenType::DECIMAL);
    testSingleToken("0x1234", TokenType::HEXNUMBER);
    testSingleToken("foobar", TokenType::IDENTIFIER);
    testSingleToken("\"foo\\n\\\"bar's\"", TokenType::STRING);
    testSingleToken("&", TokenType::AMPERSAND);
    testSingleToken("@", TokenType::AMPHORA);
    testSingleToken("@!", TokenType::AMBANG);
    testSingleToken("<-", TokenType::ASSIGN);
    testSingleToken("*", TokenType::ASTERISK);
    testSingleToken("!", TokenType::BANG);
    testSingleToken("!<", TokenType::BANGLANGLE);
    testSingleToken("^", TokenType::CARAT);
    testSingleToken("^?", TokenType::CARATQ);
    testSingleToken(",", TokenType::COMMA);
    testSingleToken("::", TokenType::DCOLON);
    testSingleToken(":<", TokenType::COLANGLE);
    testSingleToken(":", TokenType::COLON);
    testSingleToken("=", TokenType::EQUALS);
    testSingleToken("<", TokenType::LANGLE);
    testSingleToken("{", TokenType::LBRACE);
    testSingleToken("[", TokenType::LBRACKET);
    testSingleToken("(", TokenType::LPAREN);
    testSingleToken("-", TokenType::MINUS);
    testSingleToken("%", TokenType::PERCENT);
    testSingleToken("|", TokenType::PIPE);
    testSingleToken("|:", TokenType::PIPECOL);
    testSingleToken("+", TokenType::PLUS);
    testSingleToken("?:", TokenType::QCOLON);
    testSingleToken("?<", TokenType::QLANGLE);
    testSingleToken("?", TokenType::QMARK);
    testSingleToken("?-", TokenType::QMINUS);
    testSingleToken(">", TokenType::RANGLE);
    testSingleToken("}", TokenType::RBRACE);
    testSingleToken("]", TokenType::RBRACKET);
    testSingleToken(")", TokenType::RPAREN);
    testSingleToken("/", TokenType::SLASH);
    testSingleToken(".cmd foobar", TokenType::COMMAND);
    testSingleToken(".class foobar", TokenType::CLASS);
    testSingleToken(".domain foobar", TokenType::DOMAIN);
    testSingleToken(".enum foobar", TokenType::ENUMERATION);
    testSingleToken(".intrinsic foobar", TokenType::INTRINSIC);
    testSingleToken(".object foobar", TokenType::OBJECT);
    testSingleToken(".record foobar", TokenType::RECORD);
}

TEST_CASE("test lex single token negative test cases") {
    // Test comment line produces no token (success case)
    CHECK(lexInput(" ;fish sticks").output.empty());

    // Test unclosed string produces no token (failure case)
    CHECK(lexInput("\"unclosed string", false).output.empty());

    // Test multi-line string fails
    CHECK(lexInput("\"multiline\nstring\"", false).output.empty());

    // Test invalid reserved word produces no token (failure case)
    CHECK(lexInput(".invalidReservedWord", false).output.empty());

    // Test invalid decimal produces no token (failure case)
    CHECK(lexInput("1234.5678.9", false).output.empty());

    // Test invalid character after string produces no token (failure case)
    CHECK(lexInput("\"x\"1", false).output.empty());
    CHECK(lexInput("\"x\"a", false).output.empty());

    // Test invalid reserved words produces no token (failure case)
    CHECK(lexInput(".invalidReservedWord", false).output.empty());
    CHECK(lexInput(".cm", false).output.empty());
    CHECK(lexInput(".cmda", false).output.empty());
    CHECK(lexInput(".cmd2", false).output.empty());
    CHECK(lexInput(".cmd_", false).output.empty());
}

TEST_CASE("test lex multiple tokens") {
    std::string input = "1234 -5678";
    basis::Lexer lexer = lexInput(input);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front().type == TokenType::NUMBER);
    CHECK(lexer.output.back().type == TokenType::NUMBER);
}

TEST_CASE("test lex boundary detection") {
    std::string input = "a b\n c\nd\n e\nf\n";
    basis::Lexer lexer = lexInput(input);
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