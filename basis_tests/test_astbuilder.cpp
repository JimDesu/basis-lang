#include "doctest.h"

#include "../AstBuilder.h"
#include "../Grammar2.h"
#include "../Lexer.h"
#include "../Parsing2.h"

#include <sstream>
#include <variant>

using namespace basis;

namespace {

std::shared_ptr<CompilationUnit> parseCompilationUnit(const std::string& text) {
    std::istringstream input(text);
    Lexer lexer(input, false);
    REQUIRE(lexer.scan());

    Parser parser(lexer.output, getGrammar().COMPILATION_UNIT);
    REQUIRE(parser.parse());
    REQUIRE(parser.allTokensConsumed());
    REQUIRE(parser.parseTree != nullptr);
    REQUIRE(parser.parseTree->production == Production::COMPILATION_UNIT);

    auto cu = buildAst(parser.parseTree);
    REQUIRE(cu != nullptr);
    return cu;
}

template <typename T, typename Variant>
const T& requireAlt(const Variant& v) {
    auto* alt = std::get_if<T>(&v);
    REQUIRE(alt != nullptr);
    return *alt;
}

template <typename T>
const T& requireType(const TypeNodePtr& node) {
    REQUIRE(node != nullptr);
    return requireAlt<T>(node->v);
}

template <typename T>
const T& requireExpr(const ExprNodePtr& node) {
    REQUIRE(node != nullptr);
    return requireAlt<T>(node->v);
}

const IdentifierExpr& requireCommandTargetIdentifier(const ExprNodePtr& node) {
    const auto& cmd = requireExpr<CallCommandExpr>(node);
    CHECK(cmd.params.empty());
    return requireExpr<IdentifierExpr>(cmd.target);
}

const ExprStat& requireSingleExprStat(const TestDecl& test) {
    REQUIRE(test.body != nullptr);
    REQUIRE(test.body->statements.size() == 1);
    return requireAlt<ExprStat>(test.body->statements.front().v);
}

} // namespace

TEST_CASE("AstBuilder preserves plain pointer depth") {
    auto cu = parseCompilationUnit(".alias DeepPtr: ^^Int");

    REQUIRE(cu->definitions.size() == 1);
    const auto& alias = requireAlt<AliasDecl>(cu->definitions.front());
    const auto& ptr = requireType<PtrType>(alias.type);
    CHECK_EQ(ptr.depth, 2);
    CHECK_EQ(requireType<NamedType>(ptr.inner).name, "Int");
}

TEST_CASE("AstBuilder preserves command type kind, writeable marker, and nested pointer depth") {
    auto cu = parseCompilationUnit(".alias Handler: ?<^^Int', String>");

    REQUIRE(cu->definitions.size() == 1);
    const auto& alias = requireAlt<AliasDecl>(cu->definitions.front());
    const auto& cmd = requireType<CmdType>(alias.type);
    CHECK(cmd.kind == CmdType::Kind::MayFail);
    REQUIRE(cmd.args.size() == 2);
    CHECK(cmd.args[0].writeable);
    CHECK_FALSE(cmd.args[1].writeable);

    const auto& ptr = requireType<PtrType>(cmd.args[0].type);
    CHECK_EQ(ptr.depth, 2);
    CHECK_EQ(requireType<NamedType>(ptr.inner).name, "Int");
    CHECK_EQ(requireType<NamedType>(cmd.args[1].type).name, "String");
}

TEST_CASE("AstBuilder keeps outer command args non-writeable when only nested command args are writeable") {
    auto cu = parseCompilationUnit(".alias NestedHandler: :<:<Int'> >");

    REQUIRE(cu->definitions.size() == 1);
    const auto& alias = requireAlt<AliasDecl>(cu->definitions.front());
    const auto& outer = requireType<CmdType>(alias.type);
    REQUIRE(outer.args.size() == 1);
    CHECK_FALSE(outer.args[0].writeable);

    const auto& inner = requireType<CmdType>(outer.args[0].type);
    REQUIRE(inner.args.size() == 1);
    CHECK(inner.args[0].writeable);
    CHECK_EQ(requireType<NamedType>(inner.args[0].type).name, "Int");
}

TEST_CASE("AstBuilder preserves exact binary operator text and suffix operators") {
    auto cu = parseCompilationUnit(".test \"ops\" = ptr^[0]& << data& <= limit");

    REQUIRE(cu->definitions.size() == 1);
    const auto& test = requireAlt<TestDecl>(cu->definitions.front());
    const auto& stat = requireSingleExprStat(test);
    const auto& expr = requireExpr<BinaryExpr>(stat.expr);
    REQUIRE(expr.rest.size() == 2);
    CHECK_EQ(expr.rest[0].op, "<<");
    CHECK_EQ(expr.rest[1].op, "<=");

    const auto& lhs = requireExpr<SuffixExpr>(expr.first);
    CHECK_EQ(requireCommandTargetIdentifier(lhs.base).text, "ptr");
    REQUIRE(lhs.suffixes.size() == 3);
    CHECK(lhs.suffixes[0].kind == SuffixOp::Kind::Deref);
    CHECK(lhs.suffixes[1].kind == SuffixOp::Kind::Index);
    CHECK(lhs.suffixes[2].kind == SuffixOp::Kind::Addr);
    CHECK_EQ(requireExpr<LiteralExpr>(lhs.suffixes[1].indexLoc).text, "0");

    const auto& middle = requireExpr<SuffixExpr>(expr.rest[0].term);
    CHECK_EQ(requireCommandTargetIdentifier(middle.base).text, "data");
    REQUIRE(middle.suffixes.size() == 1);
    CHECK(middle.suffixes[0].kind == SuffixOp::Kind::Addr);
    CHECK_EQ(requireCommandTargetIdentifier(expr.rest[1].term).text, "limit");
}

TEST_CASE("AstBuilder preserves command literal kind") {
    auto cu = parseCompilationUnit(".test \"cmd lit\" = (!<^[]Int data>{process: data})");

    REQUIRE(cu->definitions.size() == 1);
    const auto& test = requireAlt<TestDecl>(cu->definitions.front());
    const auto& stat = requireSingleExprStat(test);
    const auto& lit = requireExpr<CmdLiteralExpr>(stat.expr);
    CHECK(lit.kind == CmdLiteralExpr::Kind::MustFail);
    REQUIRE(lit.params.size() == 1);
    CHECK_EQ(lit.params[0].name, "data");

    const auto& ptr = requireType<PtrType>(lit.params[0].type);
    CHECK_EQ(ptr.depth, 1);
    const auto& range = requireType<RangeType>(ptr.inner);
    REQUIRE(range.element != nullptr);
    CHECK_EQ(requireType<NamedType>(range.element).name, "Int");
}
