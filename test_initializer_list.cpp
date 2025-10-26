#include "Parsing2.h"
#include "Grammar2.h"
#include <iostream>

using namespace basis;

int main() {
    // Test that the new syntax works without braces
    SPFN test1 = any(
        match(Production::NUMBER, TokenType::NUMBER),
        match(Production::STRING, TokenType::STRING)
    );
    
    SPFN test2 = all(
        match(Production::IDENTIFIER, TokenType::IDENTIFIER),
        match(Production::COLON, TokenType::COLON),
        match(Production::NUMBER, TokenType::NUMBER)
    );
    
    SPFN test3 = boundedGroup(Production::CARAT,
        match(Production::IDENTIFIER, TokenType::IDENTIFIER),
        match(Production::COLON, TokenType::COLON),
        match(Production::NUMBER, TokenType::NUMBER)
    );
    
    std::cout << "Initializer list syntax works!" << std::endl;
    return 0;
}

