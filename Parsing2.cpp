#include "Parsing2.h"

namespace basis {

    // ParseFn static helper
    bool ParseFn::atLimit(const std::list<spToken>& tokens, itToken* pIter, const Token* pLimit) {
        return (*pIter) == tokens.cend() || (pLimit != nullptr && (*pIter)->get() == pLimit);
    }

    // Parsing2 implementation
    Parser::Parser(const std::list<spToken>& tokens, SPPF spParseFn)
        : tokens(tokens), spfn(spParseFn) {}

    bool Parser::parse() {
        itToken start = tokens.cbegin();
        spParseTree* pTree = &parseTree;
        return spfn->parse(tokens, &pTree, &start, nullptr);
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

    SPPF discard(TokenType type) { return std::make_shared<Discard>(type); }

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

    SPPF match(Production prod, TokenType type) { return std::make_shared<Match>(prod, type); }

    // Maybe implementation
    Maybe::Maybe(SPPF spParseFn): spfn(spParseFn) {}

    bool Maybe::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                      itToken* pIter, const Token* pLimit) const {
        spfn->parse(tokens, dpspResult, pIter, pLimit);
        return true;
    }

    SPPF maybe(SPPF parseFn) { return std::make_shared<Maybe>(parseFn); }

    // Any implementation
    Any::Any(std::vector<SPPF> alternatives) : alternatives(alternatives) {}

    bool Any::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        itToken start = *pIter;
        for (const SPPF& alt : alternatives) {
            if (alt->parse(tokens, dpspResult, pIter, pLimit)) {
                return true;
            }
            *pIter = start;
        }
        return false;
    }

    // All implementation
    All::All(std::vector<SPPF> sequence) : sequence(sequence) {}

    bool All::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        spParseTree* next = *dpspResult;
        for (const SPPF& fn : sequence) {
            if (!fn->parse(tokens, &next, pIter, pLimit)) {
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    // OneOrMore implementation
    OneOrMore::OneOrMore(SPPF spParseFn) : spfn(spParseFn) {}

    bool OneOrMore::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                         itToken* pIter, const Token* pLimit) const {
        if (!spfn->parse(tokens, dpspResult, pIter, pLimit)) {
            return false;
        }
        spParseTree* next = *dpspResult;
        if (*next) next = &((*next)->spNext);
        while (spfn->parse(tokens, &next, pIter, pLimit)) {
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    SPPF oneOrMore(SPPF parseFn) { return std::make_shared<OneOrMore>(parseFn); }

    // Separated implementation
    Separated::Separated(SPPF spElement, SPPF spSeparator)
        : spElement(spElement), spSeparator(spSeparator) {}

    bool Separated::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                         itToken* pIter, const Token* pLimit) const {
        if (!spElement->parse(tokens, dpspResult, pIter, pLimit)) {
            return false;
        }
        spParseTree* next = *dpspResult;
        if (*next) next = &((*next)->spNext);
        while (true) {
            itToken beforeSep = *pIter;
            if (!spSeparator->parse(tokens, &next, pIter, pLimit)) {
                break;
            }
            if (!spElement->parse(tokens, &next, pIter, pLimit)) {
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    SPPF separated(SPPF element, SPPF separator) { return std::make_shared<Separated>(element, separator); }

    // Bound implementation
    Bound::Bound(SPPF spParseFn) : spfn(spParseFn) {}

    bool Bound::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        const Token* boundLimit = (*pIter)->get()->bound ? (*pIter)->get()->bound.get() : nullptr;
        return spfn->parse(tokens, dpspResult, pIter, boundLimit);
    }

    SPPF bound(SPPF parseFn) { return std::make_shared<Bound>(parseFn); }

    // Group implementation
    Group::Group(Production prod, SPPF spParseFn) : prod(prod), spfn(spParseFn) {}

    bool Group::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        spParseTree* target = *dpspResult;
        (*target) = std::make_shared<ParseTree>(prod);
        spParseTree* down = &(*target)->spDown;
        if (spfn->parse(tokens, &down, pIter, pLimit)) {
            return true;
        }
        target->reset();
        return false;
    }

    SPPF group(Production prod, SPPF parseFn) { return std::make_shared<Group>(prod, parseFn); }

    // BoundedGroup implementation
    BoundedGroup::BoundedGroup(Production prod, std::vector<SPPF> sequence)
        : spfn(group(prod, bound(std::make_shared<All>(sequence)))) {}

    bool BoundedGroup::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                            itToken* pIter, const Token* pLimit) const {
        return spfn->parse(tokens, dpspResult, pIter, pLimit);
    }

    // Forward implementation
    Fwd::Fwd(Production p): prod(p) {
        // don't do anything -- Ref will call the setter
    }
    bool Fwd::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                    itToken* pIter, const Token* pLimit) const {
        return spfn->parse(tokens, dpspResult, pIter, pLimit);
    }
    void Fwd::setParseFn(SPPF spParseFn) {
        spfn = spParseFn;
    }
    SPPF fwd(Production p) { return std::make_shared<Fwd>(p); }

    // Ref implementation
    Ref::Ref(SPPF spFwd, SPPF spParseFn) : spfn(spFwd) {
        auto fwdPtr = std::dynamic_pointer_cast<Fwd>(spFwd);
        if (fwdPtr) {
            fwdPtr->setParseFn(spParseFn);
        }
    }
    bool Ref::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                    itToken* pIter, const Token* pLimit) const {
        return spfn->parse(tokens, dpspResult, pIter, pLimit);
    }
    SPPF ref(SPPF spFwd, SPPF spParseFn) { return std::make_shared<Ref>(spFwd, spParseFn); }

}

