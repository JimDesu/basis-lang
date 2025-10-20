#ifndef PARSING_H
#define PARSING_H

#include <list>
#include <memory>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<spToken>::const_iterator;

    // Template metaprogramming parser using compile-time dispatch
    template<typename ParseFnType>
    class Parser {
    public:
        explicit Parser(const std::list<spToken>& tokens) : tokens(tokens) {}

        bool parse() {
            itToken start = tokens.cbegin();
            spParseTree* pTree = &parseTree;
            return ParseFnType::parse(*this, &pTree, &start, nullptr);
        }

        bool atLimit(itToken* pIter, const Token* pLimit) const {
            return (*pIter) == tokens.cend() || (pLimit != nullptr && (*pIter)->get() == pLimit);
        }

        spParseTree parseTree;

    private:
        const std::list<spToken>& tokens;
    };

    // parse function types
    template<TokenType Type>
    struct Discard {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** _unused, itToken* pIter, const Token* pLimit) {
            if (parser.atLimit(pIter, pLimit)) return false;
            if ((*pIter)->get()->type == Type) {
                ++(*pIter);
                return true;
            }
            return false;
        }
    };

    template<Production Prod, TokenType Type>
    struct Match {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if (parser.atLimit(pIter, pLimit)) return false;
            if ((*pIter)->get()->type == Type) {
                **dpspResult = std::make_shared<ParseTree>(Prod, (*pIter)->get());
                ++(*pIter);
                *dpspResult = &((**dpspResult)->spNext);
                return true;
            }
            return false;
        }
    };

    template<typename ParseFnType>
    struct Maybe {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            return ParseFnType::parse(parser, dpspResult, pIter, pLimit) || true;
        }
    };

    template<typename ParseFnType>
    struct Fwd {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            return ParseFnType::parse(parser, dpspResult, pIter, pLimit);
        }
    };

    template<typename... ParseFnTypes>
    struct Any {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if constexpr (sizeof...(ParseFnTypes) == 0) {
                return false;
            } else {
                itToken start = *pIter;
                return try_parse<ParseFnTypes...>(parser, dpspResult, pIter, pLimit, start);
            }
        }

    private:
        template<typename FirstFn, typename... RestFns, typename ParserType>
        static bool try_parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit,
                             const itToken& start) {
            if (FirstFn::parse(parser, dpspResult, pIter, pLimit)) {
                return true;
            }
            *pIter = start;
            if constexpr (sizeof...(RestFns) > 0) {
                return try_parse<RestFns...>(parser, dpspResult, pIter, pLimit, start);
            }
            return false;
        }
    };

    template<typename... ParseFnTypes>
    struct All {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            spParseTree* next = *dpspResult;
            if constexpr (sizeof...(ParseFnTypes) == 0) {
                return true;
            } else {
                bool result = parse_all<ParseFnTypes...>(parser, next, pIter, pLimit);
                *dpspResult = next;
                return result;
            }
        }

    private:
        template<typename FirstFn, typename... RestFns, typename ParserType>
        static bool parse_all(ParserType& parser, spParseTree*& next, itToken* pIter, const Token* pLimit) {
            if (FirstFn::parse(parser, &next, pIter, pLimit)) {
                // be sure to check for nothing in case the parseFn is a discard
                if (*next) next = &((*next)->spNext);
                if constexpr (sizeof...(RestFns) > 0) {
                    return parse_all<RestFns...>(parser, next, pIter, pLimit);
                }
                return true;
            }
            return false;
        }
    };

    template<typename ParseFnType>
    struct OneOrMore {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if (!ParseFnType::parse(parser, dpspResult, pIter, pLimit)) {
                return false;
            }
            spParseTree* next = *dpspResult;
            if (*next) next = &((*next)->spNext);
            while (ParseFnType::parse(parser, &next, pIter, pLimit)) {
                if (*next) next = &((*next)->spNext);
            }
            *dpspResult = next;
            return true;
        }
    };

    template<typename ParseFnType, typename SeparatorFnType>
    struct Separated {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if (!ParseFnType::parse(parser, dpspResult, pIter, pLimit)) {
                return false;
            }
            spParseTree* next = *dpspResult;
            if (*next) next = &((*next)->spNext);
            while (true) {
                itToken beforeSep = *pIter;
                if (!SeparatorFnType::parse(parser, &next, pIter, pLimit)) {
                    break;
                }
                if (!ParseFnType::parse(parser, &next, pIter, pLimit)) {
                    // Separator found but no element after it - this is a parse failure
                    return false;
                }
                if (*next) next = &((*next)->spNext);
            }
            *dpspResult = next;
            return true;
        }
    };

    template<typename ParseFnType>
    struct Bound {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            const Token* boundLimit = (*pIter)->get()->bound ? (*pIter)->get()->bound.get() : nullptr;
            return ParseFnType::parse(parser, dpspResult, pIter, boundLimit);
        }
    };

    template<Production Prod, typename ParseFnType>
    struct Group {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            spParseTree* target = *dpspResult;
            (*target) = std::make_shared<ParseTree>(Prod);
            spParseTree* down = &(*target)->spDown;
            if (ParseFnType::parse(parser, &down, pIter, pLimit)) {
                return true;
            }
            target->reset();
            return false;
        }
    };

    template<Production Prod, typename... ParseFnTypes>
    struct BoundedGroup {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            return Group<Prod, Bound<All<ParseFnTypes...>>>::parse(parser, dpspResult, pIter, pLimit);
        }
    };


}

#endif // PARSING_H
