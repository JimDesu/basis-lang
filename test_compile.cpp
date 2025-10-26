#include "Parsing2.h"
#include <iostream>

int main() {
    std::list<basis::spToken> tokens;
    basis::spToken token = std::make_shared<basis::Token>();
    token->type = basis::TokenType::IDENTIFIER;
    tokens.push_back(token);

    // TODO: repair refactoring damage
    //basis::Parser<basis::Discard<basis::TokenType::IDENTIFIER>> parser(tokens);
    //bool result = parser.parse();
    //std::cout << "Parser compiled successfully! Parse result: " << result << std::endl;
    return 0;
}
