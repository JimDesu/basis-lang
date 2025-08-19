#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens) : tokens(tokens) {
}

Parser::Parser(const Parser& other) : tokens(other.tokens) {
}

Parser& Parser::operator=(const Parser& rhs) {
    if (this != &rhs) {
        tokens = rhs.tokens;
    }
    return *this;
}

Parser::~Parser() {
}
