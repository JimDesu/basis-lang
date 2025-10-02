#include "Parsing.h"
#include <iostream>

int main() {
    std::list<basis::Token> tokens;
    tokens.emplace_back();
    tokens.back().type = basis::TokenType::IDENTIFIER;

    basis::Parser<basis::Discard<basis::TokenType::IDENTIFIER>> parser(tokens);
    bool result = parser.parse();
    std::cout << "Parser compiled successfully! Parse result: " << result << std::endl;
    return 0;
}
