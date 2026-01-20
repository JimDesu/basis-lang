#include "Parsing2.h"

namespace basis {

    // ParseFn static helpers
    bool ParseFn::atLimit(const std::list<spToken>& tokens, itToken* pIter, const Token* pLimit) {
        return (*pIter) == tokens.cend() || (pLimit != nullptr && (*pIter)->get() == pLimit);
    }

    const Token* ParseFn::getBoundLimit(const std::list<spToken>& tokens, itToken* pIter, const Token* pLimit) {
        if (atLimit(tokens, pIter, pLimit)) return nullptr;
        return (*pIter)->get()->bound ? (*pIter)->get()->bound.get() : nullptr;
    }

    spParseTree* ParseFn::createGroupNode(Production prod, spParseTree** dpspResult) {
        spParseTree* target = *dpspResult;
        (*target) = std::make_shared<ParseTree>(prod);
        return &(*target)->spDown;
    }

    // Parsing2 implementation
    Parser::Parser(const std::list<spToken>& tokens, SPPF spParseFn)
        : tokens(tokens), spfn(spParseFn), finalPosition(tokens.cend()) {}

    bool Parser::parse() {
        finalPosition = tokens.cbegin();
        spParseTree* pTree = &parseTree;
        return spfn->parse(tokens, &pTree, &finalPosition, nullptr);
    }

    bool Parser::allTokensConsumed() const {
        return finalPosition == tokens.cend();
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
        RollbackGuard<itToken> guard(pIter);
        spParseTree* next = *dpspResult;
        if (spfn->parse(tokens, &next, pIter, pLimit)) {
            *dpspResult = next;
            guard.commit();
        }
        return true;
    }

    SPPF maybe(SPPF parseFn) { return std::make_shared<Maybe>(parseFn); }

    // Prefix implementation
    Prefix::Prefix(std::vector<SPPF> sequence) : sequence(sequence) {}

    bool Prefix::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                       itToken* pIter, const Token* pLimit) const {
        if (sequence.empty()) return true;

        RollbackGuard<itToken> guard(pIter);
        spParseTree* next = *dpspResult;

        // Try to match the first element (the prefix)
        if (!sequence[0]->parse(tokens, &next, pIter, pLimit)) {
            // Prefix not found - succeed without consuming anything
            *dpspResult = next;
            guard.commit();
            return true;
        }

        // Prefix matched - now all remaining elements must match
        if (*next) next = &((*next)->spNext);
        for (size_t i = 1; i < sequence.size(); ++i) {
            if (!sequence[i]->parse(tokens, &next, pIter, pLimit)) {
                // Failed after prefix matched - restore position and fail
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        guard.commit();
        return true;
    }

    SPPF prefix(SPPF parseFn) { return std::make_shared<Prefix>(std::vector<SPPF>{parseFn}); }

    // Any implementation
    Any::Any(std::vector<SPPF> alternatives) : alternatives(alternatives) {}

    bool Any::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        RollbackGuard<itToken> guard(pIter);
        for (const SPPF& alt : alternatives) {
            if (alt->parse(tokens, dpspResult, pIter, pLimit)) {
                guard.commit();
                return true;
            }
            guard.restore();
        }
        return false;
    }

    // All implementation
    All::All(std::vector<SPPF> sequence) : sequence(sequence) {}

    bool All::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                   itToken* pIter, const Token* pLimit) const {
        RollbackGuard<itToken> guard(pIter);
        spParseTree* next = *dpspResult;
        for (const SPPF& fn : sequence) {
            if (!fn->parse(tokens, &next, pIter, pLimit)) {
                return false;
            }
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        guard.commit();
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
            RollbackGuard<itToken> guard(pIter);
            if (!spSeparator->parse(tokens, &next, pIter, pLimit)) {
                break;
            }
            if (!spElement->parse(tokens, &next, pIter, pLimit)) {
                // Found separator but no following element - restore position and fail
                return false;
            }
            guard.commit();
            if (*next) next = &((*next)->spNext);
        }
        *dpspResult = next;
        return true;
    }

    SPPF separated(SPPF element, SPPF separator) {
        return std::make_shared<Separated>(element, separator);
    }

    // Bound implementation
    Bound::Bound(SPPF spParseFn) : spfn(spParseFn) {}

    bool Bound::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        if (atLimit(tokens, pIter, pLimit)) return false;
        const Token* boundLimit = (*pIter)->get()->bound ? (*pIter)->get()->bound.get() : nullptr;
        return spfn->parse(tokens, dpspResult, pIter, boundLimit);
    }

    SPPF bound(SPPF parseFn) { return std::make_shared<Bound>(parseFn); }

    // Group implementation
    Group::Group(Production prod, SPPF spParseFn) : prod(prod), spfn(spParseFn) {}

    bool Group::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        RollbackGuard<itToken> guard(pIter);
        spParseTree* target = *dpspResult;
        (*target) = std::make_shared<ParseTree>(prod);
        spParseTree* down = &(*target)->spDown;
        if (spfn->parse(tokens, &down, pIter, pLimit)) {
            guard.commit();
            return true;
        }
        target->reset();
        return false;
    }
    SPPF group(Production prod, SPPF parseFn) { return std::make_shared<Group>(prod, parseFn); }

    // BoundedGroup implementation
    BoundedGroup::BoundedGroup(Production prod, std::vector<SPPF> sequence)
        : prod(prod), sequence(sequence) {}

    bool BoundedGroup::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                            itToken* pIter, const Token* pLimit) const {
        if (atLimit(tokens, pIter, pLimit)) return false;
        const Token* boundLimit = getBoundLimit(tokens, pIter, pLimit);

        RollbackGuard<itToken> guard(pIter);
        spParseTree* down = createGroupNode(prod, dpspResult);

        All allParser(sequence);
        if (allParser.parse(tokens, &down, pIter, boundLimit) &&
            atLimit(tokens, pIter, boundLimit)) {
            guard.commit();
            return true;
        }

        (*dpspResult)->reset();
        return false;
    }

    // Forward implementation
    Forward::Forward(const SPPF& ref) : spfnRef(ref) {}

    bool Forward::parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                     itToken* pIter, const Token* pLimit) const {
        return spfnRef->parse(tokens, dpspResult, pIter, pLimit);
    }

    SPPF forward(const SPPF& spfnRef) {
        return std::make_shared<Forward>(spfnRef);
    }

}

