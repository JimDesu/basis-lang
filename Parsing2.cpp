#include "Parsing2.h"

namespace basis {

    // ParseFn static helper
    bool ParseFn::atLimit(const std::list<spToken>& tokens, itToken* pIter, const Token* pLimit) {
        return (*pIter) == tokens.cend() || (pLimit != nullptr && (*pIter)->get() == pLimit);
    }

    // Parser2 implementation
    Parsing2::Parsing2(const std::list<spToken>& tokens, const ParseFn& parseFn)
        : tokens(tokens), parseFn(parseFn) {}

    bool Parsing2::parse() {
        itToken start = tokens.cbegin();
        spParseTree* pTree = &parseTree;
        return parseFn.parse(tokens, &pTree, &start, nullptr);
    }

    // Discard implementation
    Discard::Discard(TokenType type) : type(type) {}

    bool Discard::parse(const std::list<spToken>& tokens, spParseTree** _unused,
                       itToken* pIter, const Token* pLimit) const {
        if (atLimit(tokens, pIter, pLimit)) return false;
        if ((*pIter)->get()->type == type) {
            ++(*pIter);
            return true;
        }
        return false;
    }

    // Match implementation
    Match::Match(Production prod, TokenType type) : prod(prod), type(type) {}

    bool Match::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        if (atLimit(tokens, pIter, pLimit)) return false;
        if ((*pIter)->get()->type == type) {
            **dpspResult = std::make_shared<ParseTree>(prod, (*pIter)->get());
            ++(*pIter);
            *dpspResult = &((**dpspResult)->spNext);
            return true;
        }
        return false;
    }

    // Maybe implementation
    Maybe::Maybe(const ParseFn& parseFn) : parseFn(parseFn) {}

    bool Maybe::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        parseFn.parse(tokens, dpspResult, pIter, pLimit);
        return true;  // Always succeeds
    }

    // Any implementation
    Any::Any(std::vector<const ParseFn*> alternatives) : alternatives(alternatives) {}

    bool Any::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        itToken start = *pIter;
        for (const ParseFn* alt : alternatives) {
            if (alt->parse(tokens, dpspResult, pIter, pLimit)) {
                return true;
            }
            *pIter = start;  // Reset on failure
        }
        return false;
    }

    // All implementation
    All::All(std::vector<const ParseFn*> sequence) : sequence(sequence) {}

    bool All::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        spParseTree* next = *dpspResult;
        for (const ParseFn* fn : sequence) {
            if (!fn->parse(tokens, &next, pIter, pLimit)) {
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    // OneOrMore implementation
    OneOrMore::OneOrMore(const ParseFn& parseFn) : parseFn(parseFn) {}

    bool OneOrMore::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                         itToken* pIter, const Token* pLimit) const {
        if (!parseFn.parse(tokens, dpspResult, pIter, pLimit)) {
            return false;
        }
        spParseTree* next = *dpspResult;
        if (*next) next = &((*next)->spNext);
        while (parseFn.parse(tokens, &next, pIter, pLimit)) {
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    // Separated implementation
    Separated::Separated(const ParseFn& element, const ParseFn& separator)
        : element(element), separator(separator) {}

    bool Separated::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                         itToken* pIter, const Token* pLimit) const {
        if (!element.parse(tokens, dpspResult, pIter, pLimit)) {
            return false;
        }
        spParseTree* next = *dpspResult;
        if (*next) next = &((*next)->spNext);
        while (true) {
            itToken beforeSep = *pIter;
            if (!separator.parse(tokens, &next, pIter, pLimit)) {
                break;
            }
            if (!element.parse(tokens, &next, pIter, pLimit)) {
                // Separator found but no element after it - parse failure
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    // Bound implementation
    Bound::Bound(const ParseFn& parseFn) : parseFn(parseFn) {}

    bool Bound::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        const Token* boundLimit = (*pIter)->get()->bound ? (*pIter)->get()->bound.get() : nullptr;
        return parseFn.parse(tokens, dpspResult, pIter, boundLimit);
    }

    // Group implementation
    Group::Group(Production prod, const ParseFn& parseFn) : prod(prod), parseFn(parseFn) {}

    bool Group::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        spParseTree* target = *dpspResult;
        (*target) = std::make_shared<ParseTree>(prod);
        spParseTree* down = &(*target)->spDown;
        if (parseFn.parse(tokens, &down, pIter, pLimit)) {
            return true;
        }
        target->reset();
        return false;
    }

    // BoundedGroup implementation
    BoundedGroup::BoundedGroup(Production prod, std::vector<const ParseFn*> sequence)
        : all(sequence), bound(all), group(prod, bound) {}

    bool BoundedGroup::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                            itToken* pIter, const Token* pLimit) const {
        return group.parse(tokens, dpspResult, pIter, pLimit);
    }

}

