#ifndef PARSING_H
#define PARSING_H

#include <list>
#include <memory>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<Token>::const_iterator;

    // Forward declaration
    template<typename ParseFnType>
    class Parser;

    // Type trait unwrapper so the parse functions can take either parse functions or a parser instance as
    // an argument (to keep the grammar clean)
    template<typename T>
    struct UnwrapParser {
        using type = T;
    };
    template<typename ParseFnType>
    struct UnwrapParser<Parser<ParseFnType>> {
        using type = ParseFnType;
    };
    template<typename T>
    using unwrap = typename UnwrapParser<T>::type;

    // the actual parser for the given parse function types
    template<typename ParseFnType>
    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens) : tokens(tokens) {}

        bool parse() {
            itToken start = tokens.cbegin();
            spParseTree* pTree = &parseTree;
            return ParseFnType::parse(*this, &pTree, &start, nullptr);
        }

        bool atLimit(itToken* pIter, const Token* pLimit) const {
            return (*pIter) == tokens.cend() || (pLimit != nullptr && &(**pIter) == pLimit);
        }

        spParseTree parseTree;

    private:
        const std::list<Token>& tokens;
    };

    // parse function types
    template<TokenType Type>
    struct Discard {
        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** _unused, itToken* pIter, const Token* pLimit) {
            if (parser.atLimit(pIter, pLimit)) return false;
            if ((*pIter)->type == Type) {
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
            if ((*pIter)->type == Type) {
                **dpspResult = std::make_shared<ParseTree>(Prod, &(**pIter));
                ++(*pIter);
                *dpspResult = &((**dpspResult)->spNext);
                return true;
            }
            return false;
        }
    };

    template<typename ParseFnType>
    struct Maybe {
        using UnwrappedParseFn = unwrap<ParseFnType>;

        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            return UnwrappedParseFn::parse(parser, dpspResult, pIter, pLimit) || true;
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
                return try_parse<unwrap<ParseFnTypes>...>(parser, dpspResult, pIter, pLimit, start);
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
                bool result = parse_all<unwrap<ParseFnTypes>...>(parser, next, pIter, pLimit);
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
        using UnwrappedParseFn = unwrap<ParseFnType>;

        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if (!UnwrappedParseFn::parse(parser, dpspResult, pIter, pLimit)) {
                return false;
            }
            spParseTree* next = *dpspResult;
            if (*next) next = &((*next)->spNext);
            while (UnwrappedParseFn::parse(parser, &next, pIter, pLimit)) {
                if (*next) next = &((*next)->spNext);
            }
            *dpspResult = next;
            return true;
        }
    };

    template<typename ParseFnType, typename SeparatorFnType>
    struct Separated {
        using UnwrappedParseFn = unwrap<ParseFnType>;
        using UnwrappedSeparatorFn = unwrap<SeparatorFnType>;

        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            if (!UnwrappedParseFn::parse(parser, dpspResult, pIter, pLimit)) {
                return false;
            }
            spParseTree* next = *dpspResult;
            if (*next) next = &((*next)->spNext);
            while (true) {
                itToken beforeSep = *pIter;
                if (!UnwrappedSeparatorFn::parse(parser, &next, pIter, pLimit)) {
                    break;
                }
                if (!UnwrappedParseFn::parse(parser, &next, pIter, pLimit)) {
                    *pIter = beforeSep;
                    break;
                }
                if (*next) next = &((*next)->spNext);
            }
            *dpspResult = next;
            return true;
        }
    };

    template<typename ParseFnType>
    struct Bound {
        using UnwrappedParseFn = unwrap<ParseFnType>;

        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            return UnwrappedParseFn::parse(parser, dpspResult, pIter, (*pIter)->bound);
        }
    };

    template<Production Prod, typename ParseFnType>
    struct Group {
        using UnwrappedParseFn = unwrap<ParseFnType>;

        template<typename ParserType>
        static bool parse(ParserType& parser, spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
            spParseTree* target = *dpspResult;
            (*target) = std::make_shared<ParseTree>(Prod);
            spParseTree* down = &(*target)->spDown;
            if (UnwrappedParseFn::parse(parser, &down, pIter, pLimit)) {
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
            return Group<Prod, Bound<All<unwrap<ParseFnTypes>...>>>::parse(parser, dpspResult, pIter, pLimit);
        }
    };


}

#endif // PARSING_H
