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
    CHECK(lexer.output.front()->type == expectedType);
}

basis::Lexer lexInput(const std::string& input, bool expectSuccess = true) {
    std::istringstream inputStream(input);
    basis::Lexer lexer(inputStream, false);
    bool scanResult = lexer.scan();
    CHECK_EQ(scanResult, expectSuccess);
    return lexer;
}

// Helper function to check token type after prefix
basis::Lexer lexTokenAfterPrefix(const std::string& prefix, const std::string& tokenText, TokenType expectedType) {
    std::string input = prefix + tokenText;
    basis::Lexer lexer = lexInput(input);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front()->type == expectedType);
    return lexer;
}

// Helper function to test a complete token with prefix variations
void testSingleToken(const std::string& tokenText, TokenType tokenType) {
    static const std::string whitespacePrefix = "\t";
    static const std::string commentPrefix = " ;fish sticks\n";

    // Test as single token
    testLexSingleToken(tokenText, tokenType);

    // Test with whitespace prefix and verify column number
    CHECK(lexTokenAfterPrefix(whitespacePrefix, tokenText, tokenType).output.front()->columnNumber == 2);

    // Test with comment prefix
    lexTokenAfterPrefix(commentPrefix, tokenText, tokenType);
}

TEST_CASE("Lexer::test lex all token types") {
    testSingleToken("-1234", TokenType::NUMBER);
    testSingleToken("-1234.5678", TokenType::DECIMAL);
    testSingleToken("1234", TokenType::NUMBER);
    testSingleToken("1234.5678", TokenType::DECIMAL);
    testSingleToken("0x1234", TokenType::HEXNUMBER);
    testSingleToken("0b00000100", TokenType::BINARY);
    testSingleToken("fOObAR", TokenType::IDENTIFIER);
    testSingleToken("'foobar", TokenType::IDENTIFIER);
    testSingleToken("Foobar", TokenType::TYPENAME);
    testSingleToken("FooBar", TokenType::TYPENAME);
    testSingleToken("\"foo\\n\\\"bar's\"", TokenType::STRING);
    testSingleToken("&", TokenType::AMPERSAND);
    testSingleToken("@", TokenType::AMPHORA);
    testSingleToken("@!", TokenType::AMBANG);
    testSingleToken("*", TokenType::ASTERISK);
    testSingleToken("!", TokenType::BANG);
    testSingleToken("!{", TokenType::BANGBRACE);
    testSingleToken("!<", TokenType::BANGLANGLE);
    testSingleToken("^", TokenType::CARAT);
    testSingleToken(",", TokenType::COMMA);
    testSingleToken("::", TokenType::DCOLON);
    testSingleToken(":<", TokenType::COLANGLE);
    testSingleToken(":{", TokenType::COLBRACE);
    testSingleToken(":", TokenType::COLON);
    testSingleToken("$", TokenType::DOLLAR);
    testSingleToken("=", TokenType::EQUALS);
    testSingleToken(">=", TokenType::GREQUALS);
    testSingleToken("<", TokenType::LANGLE);
    testSingleToken("<=", TokenType::LEQUALS);
    testSingleToken("<-", TokenType::LARROW);
    testSingleToken("{", TokenType::LBRACE);
    testSingleToken("[", TokenType::LBRACKET);
    testSingleToken("(", TokenType::LPAREN);
    testSingleToken("-", TokenType::MINUS);
    testSingleToken("%", TokenType::PERCENT);
    testSingleToken("|", TokenType::PIPE);
    testSingleToken("+", TokenType::PLUS);
    testSingleToken("#", TokenType::POUND);
    testSingleToken("?{", TokenType::QBRACE);
    testSingleToken("?:", TokenType::QCOLON);
    testSingleToken("?<", TokenType::QLANGLE);
    testSingleToken("?", TokenType::QMARK);
    testSingleToken("?-", TokenType::QMINUS);
    testSingleToken("??", TokenType::DQMARK);
    testSingleToken(">", TokenType::RANGLE);
    testSingleToken("->", TokenType::RARROW);
    testSingleToken("}", TokenType::RBRACE);
    testSingleToken("]", TokenType::RBRACKET);
    testSingleToken(")", TokenType::RPAREN);
    testSingleToken("/", TokenType::SLASH);
    testSingleToken("_", TokenType::UNDERSCORE);
    testSingleToken(".cmd foobar", TokenType::COMMAND);
    testSingleToken(".class foobar", TokenType::CLASS);
    testSingleToken(".domain foobar", TokenType::DOMAIN);
    testSingleToken(".enum foobar", TokenType::ENUMERATION);
    testSingleToken(".intrinsic foobar", TokenType::INTRINSIC);
    testSingleToken(".module foobar", TokenType::MODULE);
    testSingleToken(".object foobar", TokenType::OBJECT);
    testSingleToken(".record foobar", TokenType::RECORD);
    testSingleToken(".test foobar", TokenType::TEST);
}

TEST_CASE("Lexer::test lex single token negative test cases") {
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

    // Test reserved words with wrong case (uppercase first letter after dot)
    CHECK(lexInput(".Object", false).output.empty());
    CHECK(lexInput(".Record", false).output.empty());
    CHECK(lexInput(".Class", false).output.empty());
    CHECK(lexInput(".Cmd", false).output.empty());
}

TEST_CASE("Lexer::test lex multiple tokens") {
    std::string input = "1234 -5678";
    basis::Lexer lexer = lexInput(input);
    CHECK(!lexer.output.empty());
    CHECK(lexer.output.front()->type == TokenType::NUMBER);
    CHECK(lexer.output.back()->type == TokenType::NUMBER);
}

TEST_CASE("Lexer::test lex boundary detection") {
    std::string input = "a b\n c\nd\n e\nf\n";
    basis::Lexer lexer = lexInput(input);
    CHECK(!lexer.output.empty());
    CHECK_EQ(lexer.output.size(),6);
    std::vector<Token*> tokens;
    for (auto& token : lexer.output) {
        tokens.push_back(token.get());
    }
    CHECK_EQ(tokens[0]->bound.get(), tokens[3]);
    CHECK_EQ(tokens[1]->bound.get(), tokens[2]);
    CHECK_EQ(tokens[2]->bound.get(), tokens[3]);
    CHECK_EQ(tokens[3]->bound.get(), tokens[5]);
    CHECK_EQ(tokens[4]->bound.get(), tokens[5]);
}


TEST_CASE("Lexer::test identifier vs typename") {
    // Identifiers: start with lowercase or apostrophe+lowercase
    testSingleToken("foo", TokenType::IDENTIFIER);
    testSingleToken("fooBar", TokenType::IDENTIFIER);
    testSingleToken("foo_bar", TokenType::IDENTIFIER);
    testSingleToken("foo123", TokenType::IDENTIFIER);
    testSingleToken("'fOO", TokenType::IDENTIFIER);
    testSingleToken("'fooBar", TokenType::IDENTIFIER);

    // Typenames: start with uppercase
    testSingleToken("Foo", TokenType::TYPENAME);
    testSingleToken("FooBar", TokenType::TYPENAME);
    testSingleToken("Foo_Bar", TokenType::TYPENAME);
    testSingleToken("FOO", TokenType::TYPENAME);
    testSingleToken("F", TokenType::TYPENAME);
    testSingleToken("F123", TokenType::TYPENAME);

    // Test that apostrophe is a separate token
    std::string input = "Foo'";
    basis::Lexer lexer = lexInput(input);
    CHECK_EQ(lexer.output.size(), 2);  // Should be Foo (typename) + apostrophe (punct)
    CHECK_EQ(lexer.output.front()->type, TokenType::TYPENAME);
    CHECK_EQ(lexer.output.front()->text, "Foo");
    auto it = lexer.output.begin();
    ++it;
    CHECK_EQ((*it)->type, TokenType::APOSTROPHE);
    CHECK_EQ((*it)->text, "'");

    // Test that apostrophe+uppercase is NOT recognized as identifier
    std::string input2 = "'Foo";
    basis::Lexer lexer2 = lexInput(input2);
    CHECK_EQ(lexer2.output.size(), 2);  // Should be apostrophe (punct) + Foo (typename)
    CHECK_EQ(lexer2.output.front()->type, TokenType::APOSTROPHE);
    auto it2 = lexer2.output.begin();
    ++it2;
    CHECK_EQ((*it2)->type, TokenType::TYPENAME);
    CHECK_EQ((*it2)->text, "Foo");
}

TEST_CASE("Lexer::test numbers with underscores") {
    testSingleToken("23_000", TokenType::NUMBER);
    testSingleToken("1_000", TokenType::NUMBER);
    testSingleToken("-1_20", TokenType::NUMBER);
    testSingleToken("1_000_000", TokenType::NUMBER);
    testSingleToken("12_34_56", TokenType::NUMBER);
    testSingleToken("1_000.0_23", TokenType::DECIMAL);
    testSingleToken("12_34.56_78", TokenType::DECIMAL);
    testSingleToken("1.2_3", TokenType::DECIMAL);
    testSingleToken("1_2.3", TokenType::DECIMAL);
    testSingleToken("-1_000.5_00", TokenType::DECIMAL);

    // Invalid cases: underscore not between digits
    CHECK(lexInput("12_.0", false).output.empty());
    CHECK(lexInput("-0._5", false).output.empty());
}

TEST_CASE("Lexer::test binary literals") {
    testSingleToken("0b00000000", TokenType::BINARY);
    testSingleToken("0b11111111", TokenType::BINARY);
    testSingleToken("0b00000100", TokenType::BINARY);
    testSingleToken("0b0000010000000001", TokenType::BINARY);
    testSingleToken("0b1010101011001100", TokenType::BINARY);
    testSingleToken("0b000000000000000000000000", TokenType::BINARY);
    testSingleToken("0b0000_0100", TokenType::BINARY);
    testSingleToken("0b00000100_00000001", TokenType::BINARY);
    testSingleToken("0b1010_1010_1100_1100", TokenType::BINARY);
    testSingleToken("0b0000_0000_0000_0000_0000_0000", TokenType::BINARY);

    CHECK(lexInput("0b0010", false).output.empty());
    CHECK(lexInput("0b010", false).output.empty());
    CHECK(lexInput("0b1", false).output.empty());
    CHECK(lexInput("0b0000001", false).output.empty());
    CHECK(lexInput("0b000000100", false).output.empty());
    CHECK(lexInput("0b00000012", false).output.empty());
    CHECK(lexInput("0b0000001a", false).output.empty());
    CHECK(lexInput("0b_00000000", false).output.empty());
    CHECK(lexInput("0b00000000_", false).output.empty());
    CHECK(lexInput("0b0000__0000", false).output.empty());
}

TEST_CASE("Lexer::test hex literals with underscores") {
    testSingleToken("0x12_34", TokenType::HEXNUMBER);
    testSingleToken("0xAB_CD_EF", TokenType::HEXNUMBER);
    testSingleToken("0x00_11_22_33", TokenType::HEXNUMBER);
    testSingleToken("0xDE_AD_BE_EF", TokenType::HEXNUMBER);

    CHECK(lexInput("0x_1234", false).output.empty());
    CHECK(lexInput("0x1234_", false).output.empty());
    CHECK(lexInput("0x12__34", false).output.empty());
}