#include "ParseObject.h"
#include <stdexcept>

using namespace basis;

ParseObject::ParseObject(Token* token, ParseObject* next, ParseObject* down)
    : token(token), next(next), down(down) {
    if (token == nullptr) {
        throw std::invalid_argument("Token cannot be null");
    }
}

ParseObject::ParseObject(const ParseObject& other)
    : token(other.token), next(other.next), down(other.down) {
}

ParseObject& ParseObject::operator=(const ParseObject& rhs) {
    if (this != &rhs) {
        token = rhs.token;
        next = rhs.next;
        down = rhs.down;
    }
    return *this;
}

ParseObject::~ParseObject() {}
