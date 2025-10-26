#include "Parsing2.h"
#include "Productions.h"
#include "Token.h"

using namespace basis;

int main() {
    // Test variadic template syntax
    SPFN test1 = any(
        match(Production::NUMBER, TokenType::NUMBER),
        match(Production::STRING, TokenType::STRING)
    );
    
    SPFN test2 = all(
        match(Production::IDENTIFIER, TokenType::IDENTIFIER),
        match(Production::COLON, TokenType::COLON)
    );
    
    return 0;
}

