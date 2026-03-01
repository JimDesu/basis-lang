#include "doctest.h"

#include "../AstBuilder.h"
#include "../Grammar2.h"
#include "../Lexer.h"
#include <sstream>

using namespace basis;

// =============================================================================
// Test helpers
// =============================================================================
namespace {

std::shared_ptr<CompilationUnit> parseAndBuild(const std::string& text) {
    std::istringstream input(text);
    Lexer lexer(input, false);
    if (!lexer.scan()) return nullptr;
    Parser parser(lexer.output, getGrammar().COMPILATION_UNIT);
    if (!parser.parse() || !parser.allTokensConsumed()) return nullptr;
    return buildAst(parser.parseTree);
}

template<typename T>
std::shared_ptr<T> as(const spAstNode& node) {
    return std::dynamic_pointer_cast<T>(node);
}

template<typename T>
std::shared_ptr<T> defAs(const std::shared_ptr<CompilationUnit>& cu, size_t idx) {
    if (!cu || idx >= cu->definitions.size()) return nullptr;
    return as<T>(cu->definitions[idx]);
}

} // namespace

// =============================================================================
// CompilationUnit — empty and module-only
// =============================================================================

TEST_CASE("AstBuilder: empty compilation unit") {
    auto cu = parseAndBuild("");
    REQUIRE(cu != nullptr);
    CHECK(cu->module == nullptr);
    CHECK(cu->imports.empty());
    CHECK(cu->definitions.empty());
}

TEST_CASE("AstBuilder: module only") {
    auto cu = parseAndBuild(".module MyModule");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->module != nullptr);
    CHECK_EQ(cu->module->name, "MyModule");
    CHECK(cu->imports.empty());
    CHECK(cu->definitions.empty());
}

TEST_CASE("AstBuilder: qualified module name") {
    auto cu = parseAndBuild(".module Std::Collections");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->module != nullptr);
    CHECK_EQ(cu->module->name, "Std::Collections");
}

// =============================================================================
// ImportDecl
// =============================================================================

TEST_CASE("AstBuilder: file import") {
    auto cu = parseAndBuild(".import \"utils.basis\"");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->imports.size() == 1);
    CHECK_EQ(cu->imports[0]->kind, ImportDecl::Kind::File);
    CHECK_EQ(cu->imports[0]->path, "utils.basis");
}

TEST_CASE("AstBuilder: standard import without qualifier") {
    auto cu = parseAndBuild(".import MyLib");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->imports.size() == 1);
    CHECK_EQ(cu->imports[0]->kind, ImportDecl::Kind::Standard);
    CHECK(cu->imports[0]->qualifier.empty());
    CHECK_EQ(cu->imports[0]->name, "MyLib");
}

TEST_CASE("AstBuilder: standard import with qualifier") {
    auto cu = parseAndBuild(".import Std:Core");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->imports.size() == 1);
    CHECK_EQ(cu->imports[0]->kind, ImportDecl::Kind::Standard);
    CHECK_EQ(cu->imports[0]->qualifier, "Std");
    CHECK_EQ(cu->imports[0]->name, "Core");
}

TEST_CASE("AstBuilder: multiple imports") {
    auto cu = parseAndBuild(".import \"a.basis\"\n.import Std:Core");
    REQUIRE(cu != nullptr);
    CHECK_EQ(cu->imports.size(), 2);
    CHECK_EQ(cu->imports[0]->kind, ImportDecl::Kind::File);
    CHECK_EQ(cu->imports[1]->kind, ImportDecl::Kind::Standard);
}

// =============================================================================
// AliasDecl
// =============================================================================

TEST_CASE("AstBuilder: alias declaration") {
    auto cu = parseAndBuild(".alias MyInt: Int");
    REQUIRE(cu != nullptr);
    auto alias = defAs<AliasDecl>(cu, 0);
    REQUIRE(alias != nullptr);
    CHECK_EQ(alias->name, "MyInt");
    REQUIRE(alias->type != nullptr);
    CHECK_EQ(alias->type->kind, TypeExpr::Kind::Named);
    CHECK_EQ(alias->type->typeName, "Int");
}

// =============================================================================
// DomainDecl
// =============================================================================

TEST_CASE("AstBuilder: domain declaration") {
    auto cu = parseAndBuild(".domain UserId: Int");
    REQUIRE(cu != nullptr);
    auto dom = defAs<DomainDecl>(cu, 0);
    REQUIRE(dom != nullptr);
    CHECK_EQ(dom->name, "UserId");
    REQUIRE(dom->parent != nullptr);
    CHECK_EQ(dom->parent->typeName, "Int");
}

// =============================================================================
// EnumDecl
// =============================================================================

TEST_CASE("AstBuilder: enum declaration") {
    auto cu = parseAndBuild(".enum Status: active = 0, inactive = 1");
    REQUIRE(cu != nullptr);
    auto en = defAs<EnumDecl>(cu, 0);
    REQUIRE(en != nullptr);
    CHECK_EQ(en->name1, "Status");
    CHECK(en->name2.empty());
    REQUIRE(en->items.size() == 2);
    CHECK_EQ(en->items[0]->name, "active");
    CHECK_EQ(en->items[0]->value, "0");
    CHECK_EQ(en->items[1]->name, "inactive");
    CHECK_EQ(en->items[1]->value, "1");
}

TEST_CASE("AstBuilder: typed enum declaration") {
    auto cu = parseAndBuild(".enum T Fish: sockeye = 0, salmon = 1");
    REQUIRE(cu != nullptr);
    auto en = defAs<EnumDecl>(cu, 0);
    REQUIRE(en != nullptr);
    CHECK_EQ(en->name1, "T");
    CHECK_EQ(en->name2, "Fish");
}

// =============================================================================
// RecordDecl
// =============================================================================

TEST_CASE("AstBuilder: record declaration") {
    auto cu = parseAndBuild(".record Point: Int x, Int y");
    REQUIRE(cu != nullptr);
    auto rec = defAs<RecordDecl>(cu, 0);
    REQUIRE(rec != nullptr);
    CHECK_EQ(rec->name, "Point");
    REQUIRE(rec->fields.size() == 2);
    CHECK_EQ(rec->fields[0]->name, "x");
    REQUIRE(rec->fields[0]->type != nullptr);
    CHECK_EQ(rec->fields[0]->type->typeName, "Int");
    CHECK_EQ(rec->fields[1]->name, "y");
    CHECK_EQ(rec->fields[1]->type->typeName, "Int");
}

TEST_CASE("AstBuilder: record with parameterized field type") {
    auto cu = parseAndBuild(".record Container: List[Int] items");
    REQUIRE(cu != nullptr);
    auto rec = defAs<RecordDecl>(cu, 0);
    REQUIRE(rec != nullptr);
    REQUIRE(rec->fields.size() == 1);
    CHECK_EQ(rec->fields[0]->name, "items");
    REQUIRE(rec->fields[0]->type != nullptr);
    CHECK_EQ(rec->fields[0]->type->typeName, "List");
    REQUIRE(rec->fields[0]->type->typeArgs.size() == 1);
    CHECK_EQ(rec->fields[0]->type->typeArgs[0]->typeName, "Int");
}

// =============================================================================
// ObjectDecl
// =============================================================================

TEST_CASE("AstBuilder: object declaration") {
    auto cu = parseAndBuild(".object Node: Int value, ^Node next");
    REQUIRE(cu != nullptr);
    auto obj = defAs<ObjectDecl>(cu, 0);
    REQUIRE(obj != nullptr);
    CHECK_EQ(obj->name, "Node");
    REQUIRE(obj->fields.size() == 2);
    CHECK_EQ(obj->fields[0]->name, "value");
    CHECK_EQ(obj->fields[0]->type->typeName, "Int");
    CHECK_EQ(obj->fields[1]->name, "next");
    CHECK_EQ(obj->fields[1]->type->kind, TypeExpr::Kind::Pointer);
    CHECK_EQ(obj->fields[1]->type->ptrDepth, 1);
}

// =============================================================================
// InstanceDecl
// =============================================================================

TEST_CASE("AstBuilder: instance declaration single type") {
    auto cu = parseAndBuild(".instance MyType: Interface");
    REQUIRE(cu != nullptr);
    auto inst = defAs<InstanceDecl>(cu, 0);
    REQUIRE(inst != nullptr);
    CHECK_EQ(inst->name, "MyType");
    REQUIRE(inst->types.size() == 1);
    CHECK_EQ(inst->types[0]->typeName, "Interface");
    CHECK(inst->types[0]->delegate.empty());
}

TEST_CASE("AstBuilder: instance declaration multiple types") {
    auto cu = parseAndBuild(".instance User: Serializable, Comparable");
    REQUIRE(cu != nullptr);
    auto inst = defAs<InstanceDecl>(cu, 0);
    REQUIRE(inst != nullptr);
    CHECK_EQ(inst->types.size(), 2);
    CHECK_EQ(inst->types[0]->typeName, "Serializable");
    CHECK_EQ(inst->types[1]->typeName, "Comparable");
}

// =============================================================================
// CmdDecl (standalone .decl)
// =============================================================================

TEST_CASE("AstBuilder: command declaration regular") {
    auto cu = parseAndBuild(".decl helper: Int x -> result");
    REQUIRE(cu != nullptr);
    auto decl = defAs<CmdDecl>(cu, 0);
    REQUIRE(decl != nullptr);
    CHECK_EQ(decl->signature.kind, CmdSignature::Kind::Regular);
    CHECK_EQ(decl->signature.name, "helper");
    REQUIRE(decl->signature.params.size() == 1);
    CHECK_EQ(decl->signature.params[0].name, "x");
    CHECK_EQ(decl->signature.params[0].type->typeName, "Int");
    CHECK_EQ(decl->signature.returnVal, "result");
}

TEST_CASE("AstBuilder: command declaration mayfail") {
    auto cu = parseAndBuild(".decl ?tryIt: Int x -> result");
    REQUIRE(cu != nullptr);
    auto decl = defAs<CmdDecl>(cu, 0);
    REQUIRE(decl != nullptr);
    CHECK(decl->signature.mayFail);
    CHECK_EQ(decl->signature.name, "tryIt");
}

TEST_CASE("AstBuilder: command declaration fails") {
    auto cu = parseAndBuild(".decl !mustFail: String s -> output");
    REQUIRE(cu != nullptr);
    auto decl = defAs<CmdDecl>(cu, 0);
    REQUIRE(decl != nullptr);
    CHECK(decl->signature.fails);
    CHECK_EQ(decl->signature.name, "mustFail");
}

// =============================================================================
// IntrinsicDecl
// =============================================================================

TEST_CASE("AstBuilder: intrinsic declaration") {
    auto cu = parseAndBuild(".intrinsic native: Int x");
    REQUIRE(cu != nullptr);
    auto intr = defAs<IntrinsicDecl>(cu, 0);
    REQUIRE(intr != nullptr);
    CHECK_EQ(intr->signature.name, "native");
    REQUIRE(intr->signature.params.size() == 1);
    CHECK_EQ(intr->signature.params[0].name, "x");
}

// =============================================================================
// CmdDef (standalone .cmd)
// =============================================================================

TEST_CASE("AstBuilder: command definition empty body") {
    auto cu = parseAndBuild(".cmd doIt = _");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.name, "doIt");
    REQUIRE(def->body != nullptr);
    CHECK(def->body->isEmpty);
}

TEST_CASE("AstBuilder: command definition with body") {
    auto cu = parseAndBuild(".cmd doIt: Int x -> result = process: x");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.name, "doIt");
    REQUIRE(def->signature.params.size() == 1);
    CHECK_EQ(def->signature.returnVal, "result");
    REQUIRE(def->body != nullptr);
    CHECK_FALSE(def->body->isEmpty);
    REQUIRE(def->body->group != nullptr);
    CHECK_EQ(def->body->group->statements.size(), 1);
    auto invoke = as<CallInvoke>(def->body->group->statements[0]);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->target, "process");
}

TEST_CASE("AstBuilder: constructor command definition") {
    auto cu = parseAndBuild(".cmd Widget w: Int x, Int y = init: x, y");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.kind, CmdSignature::Kind::Constructor);
    REQUIRE(def->signature.receivers.size() == 1);
    CHECK_EQ(def->signature.receivers[0].name, "w");
    CHECK_EQ(def->signature.params.size(), 2);
}

TEST_CASE("AstBuilder: destructor command definition") {
    auto cu = parseAndBuild(".cmd @ Widget w = release: code");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.kind, CmdSignature::Kind::Destructor);
    REQUIRE(def->signature.receivers.size() == 1);
    CHECK_EQ(def->signature.receivers[0].name, "w");
}

TEST_CASE("AstBuilder: fail handler command definition") {
    auto cu = parseAndBuild(".cmd @! Widget w = rollback: error");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.kind, CmdSignature::Kind::FailHandler);
}

TEST_CASE("AstBuilder: vcommand definition") {
    auto cu = parseAndBuild(".cmd (Widget w):: render = draw: w");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.kind, CmdSignature::Kind::VCommand);
    CHECK_EQ(def->signature.name, "render");
    REQUIRE(def->signature.receivers.size() == 1);
    CHECK_EQ(def->signature.receivers[0].name, "w");
}


// =============================================================================
// ClassDecl
// =============================================================================

TEST_CASE("AstBuilder: class with decl and cmd members") {
    auto cu = parseAndBuild(
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process");
    REQUIRE(cu != nullptr);
    auto cls = defAs<ClassDecl>(cu, 0);
    REQUIRE(cls != nullptr);
    CHECK_EQ(cls->name, "Widget");
    REQUIRE(cls->members.size() == 2);
    auto decl = as<CmdDecl>(cls->members[0]);
    REQUIRE(decl != nullptr);
    CHECK_EQ(decl->signature.kind, CmdSignature::Kind::Constructor);
    auto def = as<CmdDef>(cls->members[1]);
    REQUIRE(def != nullptr);
    CHECK_EQ(def->signature.name, "doIt");
}

// =============================================================================
// ProgramDecl
// =============================================================================

TEST_CASE("AstBuilder: program declaration") {
    auto cu = parseAndBuild(".program = main");
    REQUIRE(cu != nullptr);
    auto prog = defAs<ProgramDecl>(cu, 0);
    REQUIRE(prog != nullptr);
    REQUIRE(prog->entryPoint != nullptr);
    auto invoke = as<CallInvoke>(prog->entryPoint);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->target, "main");
}

TEST_CASE("AstBuilder: program declaration with params") {
    auto cu = parseAndBuild(".program = run: args");
    REQUIRE(cu != nullptr);
    auto prog = defAs<ProgramDecl>(cu, 0);
    REQUIRE(prog != nullptr);
    auto invoke = as<CallInvoke>(prog->entryPoint);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->target, "run");
    CHECK_EQ(invoke->params.size(), 1);
}

// =============================================================================
// TestDecl
// =============================================================================

TEST_CASE("AstBuilder: test declaration") {
    auto cu = parseAndBuild(".test \"my test\" = doSomething");
    REQUIRE(cu != nullptr);
    auto test = defAs<TestDecl>(cu, 0);
    REQUIRE(test != nullptr);
    CHECK_EQ(test->label, "my test");
    REQUIRE(test->body != nullptr);
    auto group = as<CallGroup>(test->body);
    REQUIRE(group != nullptr);
    REQUIRE(group->statements.size() == 1);
    auto invoke = as<CallInvoke>(group->statements[0]);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->target, "doSomething");
}

// =============================================================================
// CallInvoke kinds
// =============================================================================

TEST_CASE("AstBuilder: call invoke command with parameters") {
    auto cu = parseAndBuild(".cmd run = process: x, y");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    REQUIRE(def->body != nullptr);
    REQUIRE(def->body->group != nullptr);
    auto invoke = as<CallInvoke>(def->body->group->statements[0]);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->kind, CallInvoke::Kind::Command);
    CHECK_EQ(invoke->target, "process");
    CHECK_EQ(invoke->params.size(), 2);
}

TEST_CASE("AstBuilder: call invoke constructor") {
    auto cu = parseAndBuild(".cmd run = Widget: 10, 20");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    auto invoke = as<CallInvoke>(def->body->group->statements[0]);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->kind, CallInvoke::Kind::Constructor);
    CHECK_EQ(invoke->target, "Widget");
    CHECK_EQ(invoke->params.size(), 2);
}

TEST_CASE("AstBuilder: call invoke vcommand") {
    auto cu = parseAndBuild(".cmd run = (items):: append: x");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    auto invoke = as<CallInvoke>(def->body->group->statements[0]);
    REQUIRE(invoke != nullptr);
    CHECK_EQ(invoke->kind, CallInvoke::Kind::VCommand);
    CHECK_EQ(invoke->target, "append");
    REQUIRE(invoke->receivers.size() == 1);
    CHECK_EQ(invoke->receivers[0], "items");
}

// =============================================================================
// CallAssignment
// =============================================================================

TEST_CASE("AstBuilder: call assignment") {
    auto cu = parseAndBuild(".cmd run = result <- compute: x");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    auto assign = as<CallAssignment>(def->body->group->statements[0]);
    REQUIRE(assign != nullptr);
    REQUIRE(assign->target != nullptr);
    CHECK_EQ(assign->target->text, "result");
    CHECK_EQ(assign->exprs.size(), 1);
}

// =============================================================================
// TypeExpr variants
// =============================================================================

TEST_CASE("AstBuilder: pointer type in object field") {
    auto cu = parseAndBuild(".object Node: ^Node next");
    REQUIRE(cu != nullptr);
    auto obj = defAs<ObjectDecl>(cu, 0);
    REQUIRE(obj != nullptr);
    REQUIRE(obj->fields.size() == 1);
    auto& t = obj->fields[0]->type;
    REQUIRE(t != nullptr);
    CHECK_EQ(t->kind, TypeExpr::Kind::Pointer);
    CHECK_EQ(t->ptrDepth, 1);
    REQUIRE(t->inner != nullptr);
    CHECK_EQ(t->inner->typeName, "Node");
}

TEST_CASE("AstBuilder: range type in object field") {
    auto cu = parseAndBuild(".object Buf: [256]Byte data");
    REQUIRE(cu != nullptr);
    auto obj = defAs<ObjectDecl>(cu, 0);
    REQUIRE(obj != nullptr);
    auto& t = obj->fields[0]->type;
    REQUIRE(t != nullptr);
    CHECK_EQ(t->kind, TypeExpr::Kind::Range);
    CHECK_EQ(t->rangeSize, "256");
    REQUIRE(t->inner != nullptr);
    CHECK_EQ(t->inner->typeName, "Byte");
}

TEST_CASE("AstBuilder: unbounded range type in object field") {
    auto cu = parseAndBuild(".object Arr: []Int items");
    REQUIRE(cu != nullptr);
    auto obj = defAs<ObjectDecl>(cu, 0);
    REQUIRE(obj != nullptr);
    auto& t = obj->fields[0]->type;
    REQUIRE(t != nullptr);
    CHECK_EQ(t->kind, TypeExpr::Kind::Range);
    CHECK(t->rangeSize.empty());
}

// =============================================================================
// Multi-statement CallGroup
// =============================================================================

TEST_CASE("AstBuilder: multi-statement command body") {
    auto cu = parseAndBuild(".cmd run = setup\n    process: x\n    cleanup");
    REQUIRE(cu != nullptr);
    auto def = defAs<CmdDef>(cu, 0);
    REQUIRE(def != nullptr);
    REQUIRE(def->body != nullptr);
    REQUIRE(def->body->group != nullptr);
    CHECK_EQ(def->body->group->statements.size(), 3);
}

// =============================================================================
// Integration: full compilation unit
// =============================================================================

TEST_CASE("AstBuilder: full compilation unit") {
    auto cu = parseAndBuild(
        ".module App\n"
        ".import Std:Core\n"
        ".alias UserId: Int\n"
        ".domain SessionId: Int\n"
        ".enum Role: admin = 0, user = 1\n"
        ".record User: UserId id, String name\n"
        ".object Session: SessionId id\n"
        ".instance User: Serializable\n"
        ".class Manager:\n"
        "  .decl Manager m: Int capacity\n"
        ".cmd doWork: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".test \"basic test\" = run\n"
        ".program = main");
    REQUIRE(cu != nullptr);
    REQUIRE(cu->module != nullptr);
    CHECK_EQ(cu->module->name, "App");
    CHECK_EQ(cu->imports.size(), 1);
    CHECK_EQ(cu->imports[0]->qualifier, "Std");
    CHECK_EQ(cu->imports[0]->name, "Core");
    CHECK_EQ(cu->definitions.size(), 12);
    CHECK(as<AliasDecl>(cu->definitions[0]) != nullptr);
    CHECK(as<DomainDecl>(cu->definitions[1]) != nullptr);
    CHECK(as<EnumDecl>(cu->definitions[2]) != nullptr);
    CHECK(as<RecordDecl>(cu->definitions[3]) != nullptr);
    CHECK(as<ObjectDecl>(cu->definitions[4]) != nullptr);
    CHECK(as<InstanceDecl>(cu->definitions[5]) != nullptr);
    CHECK(as<ClassDecl>(cu->definitions[6]) != nullptr);
    CHECK(as<CmdDef>(cu->definitions[7]) != nullptr);
    CHECK(as<CmdDecl>(cu->definitions[8]) != nullptr);
    CHECK(as<IntrinsicDecl>(cu->definitions[9]) != nullptr);
    CHECK(as<TestDecl>(cu->definitions[10]) != nullptr);
    CHECK(as<ProgramDecl>(cu->definitions[11]) != nullptr);
}
