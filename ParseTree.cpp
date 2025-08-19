#include "ParseTree.h"
#include <stdexcept>

using namespace basis;

ParseTree::ParseTree(Token* token, ParseTree* next, ParseTree* down)
    : token(token), next(next), down(down) {
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
