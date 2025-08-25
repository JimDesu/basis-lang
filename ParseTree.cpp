#include "ParseTree.h"
#include <stdexcept>

using namespace basis;

ParseTree::ParseTree(const Token* token): token(token), next(), down() {
    if (token == nullptr) {
        throw std::invalid_argument("Token cannot be null");
    }
}

ParseTree::ParseTree(const ParseTree& other)
    : token(other.token), next(other.next), down(other.down) {
}

ParseTree& ParseTree::operator=(const ParseTree& rhs) {
    if (this != &rhs) {
        token = rhs.token;
        next = rhs.next;
        down = rhs.down;
    }
    return *this;
}

ParseTree::~ParseTree() {}
