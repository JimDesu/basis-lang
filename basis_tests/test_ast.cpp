#include "doctest.h"
#include "test_ast_helpers.h"

using namespace basis;

// =============================================================================
// Module and imports
// =============================================================================

TEST_CASE("Ast::ModuleDecl") {
    CHECK(testAst(".module App",
        "CompilationUnit("
            "ModuleDecl('App')"
        ")"));
    CHECK(testAst(".module Std::Core",
        "CompilationUnit("
            "ModuleDecl('Std::Core')"
        ")"));
}

TEST_CASE("Ast::ImportDecl") {
    CHECK(testAst(".import \"base.basis\"",
        "CompilationUnit("
            "ImportDecl(File,'base.basis')"
        ")"));
    CHECK(testAst(".import Std::Core",
        "CompilationUnit("
            "ImportDecl(Standard,'Std::Core')"
        ")"));
    CHECK(testAst(".import Std: Std::Core",
        "CompilationUnit("
            "ImportDecl(Standard,'Std::Core',alias='Std')"
        ")"));
    CHECK(testAst(".import Base: \"base.basis\"",
        "CompilationUnit("
            "ImportDecl(File,'base.basis',alias='Base')"
        ")"));
}

// =============================================================================
// Alias and Domain
// =============================================================================

TEST_CASE("Ast::AliasDecl") {
    CHECK(testAst(".alias UserId: Int",
        "CompilationUnit("
            "AliasDecl('UserId',NamedType('Int'))"
        ")"));
    CHECK(testAst(".alias Ptr: ^Int",
        "CompilationUnit("
            "AliasDecl('Ptr',PtrType(1,NamedType('Int')))"
        ")"));
    CHECK(testAst(".alias Buf: []Int",
        "CompilationUnit("
            "AliasDecl('Buf',RangeType('',NamedType('Int')))"
        ")"));
}

TEST_CASE("Ast::DomainDecl") {
    CHECK(testAst(".domain UserId: Int",
        "CompilationUnit("
            "DomainDecl('UserId',NamedType('Int'))"
        ")"));
    CHECK(testAst(".domain Buf: [10]Int",
        "CompilationUnit("
            "DomainDecl('Buf',RangeType('10',NamedType('Int')))"
        ")"));
}

// =============================================================================
// Enums
// =============================================================================

TEST_CASE("Ast::EnumDecl") {
    CHECK(testAst(".enum Color: red = 0, green = 1, blue = 2",
        "CompilationUnit("
            "EnumDecl('Color',"
                "EnumItem('red','0'),"
                "EnumItem('green','1'),"
                "EnumItem('blue','2')"
            ")"
        ")"));
    CHECK(testAst(".enum Int Status: active = 0, inactive = 1",
        "CompilationUnit("
            "EnumDecl('Status',constraint='Int',"
                "EnumItem('active','0'),"
                "EnumItem('inactive','1')"
            ")"
        ")"));
    // Items without literal values: serialization omits the value position.
    CHECK(testAst(".enum Color: red, green, blue",
        "CompilationUnit("
            "EnumDecl('Color',EnumItem('red'),EnumItem('green'),EnumItem('blue'))"
        ")"));
    CHECK(testAst(".enum Mode: defaults",
        "CompilationUnit(EnumDecl('Mode',EnumItem('defaults')))"));
    // Mixed: each item's value is preserved alongside its name independently.
    CHECK(testAst(".enum Status: active = 0, pending, done = 2",
        "CompilationUnit("
            "EnumDecl('Status',"
                "EnumItem('active','0'),"
                "EnumItem('pending'),"
                "EnumItem('done','2')"
            ")"
        ")"));
    CHECK(testAst(".enum Status: active, pending = 1, done",
        "CompilationUnit("
            "EnumDecl('Status',"
                "EnumItem('active'),"
                "EnumItem('pending','1'),"
                "EnumItem('done')"
            ")"
        ")"));
    // Mixed with type constraint
    CHECK(testAst(".enum T Color: red, green = 1",
        "CompilationUnit("
            "EnumDecl('Color',constraint='T',"
                "EnumItem('red'),"
                "EnumItem('green','1')"
            ")"
        ")"));
    // String-literal values for some items, missing values for others.
    // (txt() on a STRING token returns the unquoted content.)
    CHECK(testAst(".enum Suit: clubs = \"c\", diamonds, hearts = \"h\", spades",
        "CompilationUnit("
            "EnumDecl('Suit',"
                "EnumItem('clubs','c'),"
                "EnumItem('diamonds'),"
                "EnumItem('hearts','h'),"
                "EnumItem('spades')"
            ")"
        ")"));
}

// =============================================================================
// Records and Objects
// =============================================================================

TEST_CASE("Ast::RecordDecl") {
    CHECK(testAst(".record Pt: Int x, Int y",
        "CompilationUnit("
            "RecordDecl('Pt',"
                "FieldDecl(NamedType('Int'),'x'),"
                "FieldDecl(NamedType('Int'),'y')"
            ")"
        ")"));
    CHECK(testAst(".record User: UserId id, String name",
        "CompilationUnit("
            "RecordDecl('User',"
                "FieldDecl(NamedType('UserId'),'id'),"
                "FieldDecl(NamedType('String'),'name')"
            ")"
        ")"));
}

TEST_CASE("Ast::ObjectDecl") {
    CHECK(testAst(".object Node: Int value, ^Node next",
        "CompilationUnit("
            "ObjectDecl('Node',"
                "FieldDecl(NamedType('Int'),'value'),"
                "FieldDecl(PtrType(1,NamedType('Node')),'next')"
            ")"
        ")"));
    CHECK(testAst(".object Buf: []Int data",
        "CompilationUnit("
            "ObjectDecl('Buf',"
                "FieldDecl(RangeType('',NamedType('Int')),'data')"
            ")"
        ")"));
}

// =============================================================================
// Unions and Variants
// =============================================================================

TEST_CASE("Ast::UnionDecl") {
    CHECK(testAst(".union Scalar: Int whole, Float fractional",
        "CompilationUnit("
            "UnionDecl('Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            ")"
        ")"));
}

TEST_CASE("Ast::VariantDecl") {
    CHECK(testAst(".variant Shape: Int circle, ^Node inner",
        "CompilationUnit("
            "VariantDecl('Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            ")"
        ")"));
}

// =============================================================================
// Instance declarations
// =============================================================================

TEST_CASE("Ast::InstanceDecl") {
    CHECK(testAst(".instance User: Serializable",
        "CompilationUnit("
            "InstanceDecl('User',"
                "InstanceType('Serializable')"
            ")"
        ")"));
    CHECK(testAst(".instance User: Serializable, Hashable",
        "CompilationUnit("
            "InstanceDecl('User',"
                "InstanceType('Serializable'),"
                "InstanceType('Hashable')"
            ")"
        ")"));
    // Delegate (lowercase identifier in parentheses)
    CHECK(testAst(".instance Widget: Interface (myImpl)",
        "CompilationUnit("
            "InstanceDecl('Widget',"
                "InstanceType('Interface',delegate='myImpl')"
            ")"
        ")"));
    CHECK(testAst(".instance User: Serializable (mySerial), Comparable (myComp)",
        "CompilationUnit("
            "InstanceDecl('User',"
                "InstanceType('Serializable',delegate='mySerial'),"
                "InstanceType('Comparable',delegate='myComp')"
            ")"
        ")"));
    // Qualified delegate
    CHECK(testAst(".instance Widget: Interface (Std::myImpl)",
        "CompilationUnit("
            "InstanceDecl('Widget',"
                "InstanceType('Interface',delegate='Std::myImpl')"
            ")"
        ")"));
    // Mixed: one delegated, one not
    CHECK(testAst(".instance Map: Container (hashImpl), Iterable",
        "CompilationUnit("
            "InstanceDecl('Map',"
                "InstanceType('Container',delegate='hashImpl'),"
                "InstanceType('Iterable')"
            ")"
        ")"));
}

// =============================================================================
// Programs and Tests
// =============================================================================

TEST_CASE("Ast::ProgramDecl") {
    CHECK(testAst(".program main",
        "CompilationUnit("
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
    CHECK(testAst(".program start: arg1, arg2",
        "CompilationUnit("
            "ProgramDecl("
                "CallCommandExpr("
                    "IdentifierExpr('start'),"
                    "CallParam(CallCommandExpr(IdentifierExpr('arg1'))),"
                    "CallParam(CallCommandExpr(IdentifierExpr('arg2')))"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::TestDecl") {
    CHECK(testAst(".test \"label\" = run",
        "CompilationUnit("
            "TestDecl('label',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('run')))"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Command signatures (regular, vcommand, constructor, destructor, fail handler)
// =============================================================================

TEST_CASE("Ast::CmdDecl - RegularSig") {
    CHECK(testAst(".decl process: Int x",
        "CompilationUnit("
            "CmdDecl(RegularSig('process',param=CmdParam(NamedType('Int'),'x')))"
        ")"));
    CHECK(testAst(".decl process: Int x -> result",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig('process',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(".decl ?fallible: Int x",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig('fallible',"
                    "fail=MayFail,"
                    "param=CmdParam(NamedType('Int'),'x')"
                ")"
            ")"
        ")"));
    CHECK(testAst(".decl !fails: Int x",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig('fails',"
                    "fail=Fails,"
                    "param=CmdParam(NamedType('Int'),'x')"
                ")"
            ")"
        ")"));
    CHECK(testAst(".decl process: Int x / Context ctx",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig('process',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Context'),'ctx')"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CmdDecl - VCommandSig") {
    CHECK(testAst(".decl Widget w:: render: Int x",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig('render',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "param=CmdParam(NamedType('Int'),'x')"
                ")"
            ")"
        ")"));
    CHECK(testAst(".decl (Widget w, Buffer b):: render -> result",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig('render',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "receiver=CmdReceiver(NamedType('Buffer'),'b'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CmdDecl - ConstructorSig") {
    CHECK(testAst(".decl Widget w: Int x, Int y",
        "CompilationUnit("
            "CmdDecl("
                "ConstructorSig("
                    "CmdReceiver(NamedType('Widget'),'w'),"
                    "CmdParam(NamedType('Int'),'x'),"
                    "CmdParam(NamedType('Int'),'y')"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CmdDecl - DestructorSig") {
    CHECK(testAst(".decl @ Widget w",
        "CompilationUnit("
            "CmdDecl(DestructorSig(CmdReceiver(NamedType('Widget'),'w')))"
        ")"));
}

TEST_CASE("Ast::CmdDecl - FailHandlerSig") {
    CHECK(testAst(".decl @! Widget w",
        "CompilationUnit("
            "CmdDecl(FailHandlerSig(CmdReceiver(NamedType('Widget'),'w')))"
        ")"));
}

// =============================================================================
// Intrinsic declarations
// =============================================================================

TEST_CASE("Ast::IntrinsicDecl") {
    CHECK(testAst(".intrinsic hashPwd: String pwd",
        "CompilationUnit("
            "IntrinsicDecl(RegularSig('hashPwd',param=CmdParam(NamedType('String'),'pwd')))"
        ")"));
}

// =============================================================================
// Command definitions (signature + body)
// =============================================================================

TEST_CASE("Ast::CmdDef") {
    CHECK(testAst(".cmd authenticate: String u -> result = validate: u",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('authenticate',"
                    "param=CmdParam(NamedType('String'),'u'),"
                    "ret='result'"
                "),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('validate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('u')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(".cmd noop: Int x = _",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('noop',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody(empty)"
            ")"
        ")"));
}

TEST_CASE("Ast::CmdDef - with .sub subcommands") {
    // Single sub before the call group
    CHECK(testAst(
        ".cmd parent: Int x =\n"
        " .sub child: Int y = work\n"
        " run",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('parent',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "sub=CmdDef("
                        "RegularSig('child',param=CmdParam(NamedType('Int'),'y')),"
                        "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('work')))))"
                    "),"
                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))"
                ")"
            ")"
        ")"));
    // Multiple subs at the same level
    CHECK(testAst(
        ".cmd parent: Int x =\n"
        " .sub a: Int y = work1\n"
        " .sub b: Int z = work2\n"
        " run",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('parent',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "sub=CmdDef("
                        "RegularSig('a',param=CmdParam(NamedType('Int'),'y')),"
                        "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('work1')))))"
                    "),"
                    "sub=CmdDef("
                        "RegularSig('b',param=CmdParam(NamedType('Int'),'z')),"
                        "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('work2')))))"
                    "),"
                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))"
                ")"
            ")"
        ")"));
    // Nested subs (sub inside a sub via the inner CmdBody.subs)
    CHECK(testAst(
        ".cmd parent: Int x =\n"
        " .sub child: Int y =\n"
        "  .sub grandchild: Int z = inner\n"
        "  work\n"
        " run",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('parent',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "sub=CmdDef("
                        "RegularSig('child',param=CmdParam(NamedType('Int'),'y')),"
                        "CmdBody("
                            "sub=CmdDef("
                                "RegularSig('grandchild',param=CmdParam(NamedType('Int'),'z')),"
                                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('inner')))))"
                            "),"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('work'))))"
                        ")"
                    "),"
                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))"
                ")"
            ")"
        ")"));
    // sub paired with empty body marker
    CHECK(testAst(
        ".cmd parent: Int x =\n"
        " .sub child: Int y = work\n"
        " _",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('parent',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "sub=CmdDef("
                        "RegularSig('child',param=CmdParam(NamedType('Int'),'y')),"
                        "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('work')))))"
                    "),"
                    "empty"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Class definitions
// =============================================================================

TEST_CASE("Ast::ClassDecl") {
    CHECK(testAst(".class UserManager:\n"
                  "    .decl create: User u -> result",
        "CompilationUnit("
            "ClassDecl('UserManager',"
                "CmdDecl("
                    "RegularSig('create',"
                        "param=CmdParam(NamedType('User'),'u'),"
                        "ret='result'"
                    ")"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Type expression alternatives (NamedType / PtrType / RangeType / CmdType /
// inline definitions). Most exercised via .alias.
// =============================================================================

TEST_CASE("Ast::Type - NamedType plain") {
    CHECK(testAst(".alias T: Int",
        "CompilationUnit(AliasDecl('T',NamedType('Int')))"));
}

TEST_CASE("Ast::Type - PtrType depth") {
    CHECK(testAst(".alias T: ^Int",
        "CompilationUnit(AliasDecl('T',PtrType(1,NamedType('Int'))))"));
    CHECK(testAst(".alias T: ^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(2,NamedType('Int'))))"));
    CHECK(testAst(".alias T: ^^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(3,NamedType('Int'))))"));
}

TEST_CASE("Ast::Type - RangeType") {
    CHECK(testAst(".alias T: []Int",
        "CompilationUnit(AliasDecl('T',RangeType('',NamedType('Int'))))"));
    CHECK(testAst(".alias T: [10]Int",
        "CompilationUnit(AliasDecl('T',RangeType('10',NamedType('Int'))))"));
    CHECK(testAst(".alias T: []",
        "CompilationUnit(AliasDecl('T',RangeType('')))"));
}

TEST_CASE("Ast::Type - CmdType kinds") {
    CHECK(testAst(".alias T: :<Int>",
        "CompilationUnit("
            "AliasDecl('T',"
                "CmdType(NoFail,CmdTypeArg(false,NamedType('Int')))"
            ")"
        ")"));
    CHECK(testAst(".alias T: ?<Int>",
        "CompilationUnit("
            "AliasDecl('T',"
                "CmdType(MayFail,CmdTypeArg(false,NamedType('Int')))"
            ")"
        ")"));
    CHECK(testAst(".alias T: !<Int>",
        "CompilationUnit("
            "AliasDecl('T',"
                "CmdType(Fails,CmdTypeArg(false,NamedType('Int')))"
            ")"
        ")"));
    CHECK(testAst(".alias T: :<Int', String>",
        "CompilationUnit("
            "AliasDecl('T',"
                "CmdType(NoFail,"
                    "CmdTypeArg(true,NamedType('Int')),"
                    "CmdTypeArg(false,NamedType('String'))"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Type - InlineRecordType") {
    CHECK(testAst(".alias T: .record Int x, Int y",
        "CompilationUnit("
            "AliasDecl('T',"
                "InlineRecordType("
                    "FieldDecl(NamedType('Int'),'x'),"
                    "FieldDecl(NamedType('Int'),'y')"
                ")"
            ")"
        ")"));
    CHECK(testAst(".alias T: .record myScope: Int x",
        "CompilationUnit("
            "AliasDecl('T',"
                "InlineRecordType(scope='myScope',"
                    "FieldDecl(NamedType('Int'),'x')"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Type - InlineUnionType") {
    CHECK(testAst(".alias T: .union Int i, Float f",
        "CompilationUnit("
            "AliasDecl('T',"
                "InlineUnionType("
                    "UnionCandidate(NamedType('Int'),'i'),"
                    "UnionCandidate(NamedType('Float'),'f')"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Expression alternatives — exercised through .program / .test bodies
// =============================================================================

TEST_CASE("Ast::Expr - LiteralExpr") {
    CHECK(testAst(".test \"x\" = ans <- 42",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('ans'),"
                        "LiteralExpr('42')"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - IdentifierExpr alloc") {
    CHECK(testAst(".test \"x\" = #buf <- alloc",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('buf',alloc),"
                        "CallCommandExpr(IdentifierExpr('alloc'))"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - BinaryExpr") {
    CHECK(testAst(".test \"x\" = a + b",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(".test \"x\" = a + b - c",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - SuffixExpr") {
    CHECK(testAst(".test \"x\" = ptr^",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(".test \"x\" = arr[0]&",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(LiteralExpr('0')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - CallConstructorExpr") {
    CHECK(testAst(".test \"x\" = Widget: 10, 20",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(LiteralExpr('10')),"
                            "CallParam(LiteralExpr('20'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - CallParam empty") {
    CHECK(testAst(".test \"x\" = process: _",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - CallFailExpr") {
    CHECK(testAst(".test \"x\" = .fail err",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "CallFailExpr(CallCommandExpr(IdentifierExpr('err')))"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Expr - QuoteExpr Subquote") {
    // A subquote in expression position routes through CALL_CMD_TARGET (which
    // accepts either a subquote or an identifier), so the AST wraps the
    // QuoteExpr in a CallCommandExpr.
    CHECK(testAst(".test \"x\" = a <- {process: data}",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr(Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Statement alternatives — Block kinds via .test bodies
// =============================================================================

TEST_CASE("Ast::Stmt - Block DoWhen / DoElse inside a .cmd body") {
    CHECK(testAst(
        ".cmd run: Int x = check\n"
        " ? then\n"
        " - otherwise",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('check'))),"
                        "Block(DoWhen,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('then')))"
                            ")"
                        "),"
                        "Block(DoElse,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('otherwise')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Stmt - Block DoBlock inside a .cmd body") {
    CHECK(testAst(
        ".cmd run: Int x = check\n"
        " % body",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('check'))),"
                        "Block(DoBlock,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('body')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::Stmt - Block DoWhenSelect inside a .cmd body") {
    CHECK(testAst(
        ".cmd run: Int x = check\n"
        " ?: select",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('check'))),"
                        "Block(DoWhenSelect,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('select')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Mixed / realistic compilation unit
// =============================================================================

TEST_CASE("Ast::CompilationUnit - realistic mix") {
    CHECK(testAst(
        ".module App\n"
        ".import \"base.basis\"\n"
        ".import Std::Core\n"
        ".alias UserId: Int\n"
        ".enum Role: admin = 0, user = 1\n"
        ".record User: UserId id, String name\n"
        ".program main",
        "CompilationUnit("
            "ModuleDecl('App'),"
            "ImportDecl(File,'base.basis'),"
            "ImportDecl(Standard,'Std::Core'),"
            "AliasDecl('UserId',NamedType('Int')),"
            "EnumDecl('Role',"
                "EnumItem('admin','0'),"
                "EnumItem('user','1')"
            "),"
            "RecordDecl('User',"
                "FieldDecl(NamedType('UserId'),'id'),"
                "FieldDecl(NamedType('String'),'name')"
            "),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
}

// =============================================================================
// Coverage gap mirrors — AST shapes for grammar features added in the
// coverage audit. Each grammar TEST_CASE that introduced a new shape has a
// corresponding AST test here.
// =============================================================================

TEST_CASE("Ast::Type - RangeType identifier-size") {
    CHECK(testAst(".alias T: [size]Int",
        "CompilationUnit(AliasDecl('T',RangeType('size',NamedType('Int'))))"));
    CHECK(testAst(".alias T: [10]",
        "CompilationUnit(AliasDecl('T',RangeType('10')))"));
    CHECK(testAst(".alias T: [size]",
        "CompilationUnit(AliasDecl('T',RangeType('size')))"));
}

TEST_CASE("Ast::Expr - SuffixExpr dual-index") {
    CHECK(testAst(".test \"x\" = arr[i, j]",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index("
                                "IdentifierExpr('i'),"
                                "CallCommandExpr(IdentifierExpr('j'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    // Numeric form
    CHECK(testAst(".test \"x\" = arr[0, 1]",
        "CompilationUnit("
            "TestDecl('x',"
                "CallGroup("
                    "ExprStat("
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(LiteralExpr('0'),LiteralExpr('1'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

// (DoRecoverSpec AST tests omitted: in a `.cmd` body context the leading `|`
// of a recover spec collides with the `|` choice operator that would extend
// the preceding expression, so a clean test input requires resolving that
// parser-level ambiguity first. The grammar-level coverage remains via
// Grammar2::BLOCK tests, which exercise grammar.BLOCK directly.)

TEST_CASE("Ast::Type - PtrType deep chains") {
    CHECK(testAst(".alias T: ^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(2,NamedType('Int'))))"));
    CHECK(testAst(".alias T: ^^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(3,NamedType('Int'))))"));
    CHECK(testAst(".alias T: ^^^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(4,NamedType('Int'))))"));
    // Pointer to range
    CHECK(testAst(".alias T: ^[]Int",
        "CompilationUnit("
            "AliasDecl('T',PtrType(1,RangeType('',NamedType('Int'))))"
        ")"));
    // Pointer to command type
    CHECK(testAst(".alias T: ^:<Int>",
        "CompilationUnit("
            "AliasDecl('T',"
                "PtrType(1,"
                    "CmdType(NoFail,CmdTypeArg(false,NamedType('Int')))"
                ")"
            ")"
        ")"));
}

// =============================================================================
// Bulk tests paralleling each TEST_CASE in test_grammar2.cpp.
//
// Coverage approach: every positive (CHECK, not CHECK_FALSE) grammar test
// whose grammar entry point can be expressed inside a COMPILATION_UNIT is
// mirrored here. For sub-grammar tests, the source is wrapped in an
// enclosing definition (e.g., a TYPE_EXPR fragment becomes the type of an
// .alias), so the asserted AST includes that wrapping context.
//
// Skipped: CHECK_FALSE negative tests, leaf-token grammars (raw IDENTIFIER /
// TYPENAME / NUMBER), and a handful of multi-line CALL_BLOCKQUOTE edge cases
// where the wrapper's column tracking is incompatible with the fragment's
// internal indentation.
// =============================================================================

TEST_CASE("Ast::COMPILATION_UNIT - empty and minimal") {
    CHECK(testAst(
        "",
        "CompilationUnit()"));
    CHECK(testAst(
        ".module MyModule",
        "CompilationUnit(ModuleDecl('MyModule'))"));
    CHECK(testAst(
        ".module Std::Collections",
        "CompilationUnit(ModuleDecl('Std::Collections'))"));
    CHECK(testAst(
        ".import \"file.basis\"",
        "CompilationUnit(ImportDecl(File,'file.basis'))"));
    CHECK(testAst(
        ".import B: \"file.basis\"",
        "CompilationUnit(ImportDecl(File,'file.basis',alias='B'))"));
    CHECK(testAst(
        ".import \"file1.basis\"\n"
        ".import F:\"file2.basis\"",
        "CompilationUnit(ImportDecl(File,'file1.basis'),ImportDecl(File,'file2.basis',alias='F'))"));
    CHECK(testAst(
        ".import Module1\n"
        ".import Module2\n"
        ".import Std:Core",
        "CompilationUnit("
            "ImportDecl(Standard,'Module1'),"
            "ImportDecl(Standard,'Module2'),"
            "ImportDecl(Standard,'Core',alias='Std')"
        ")"));
    CHECK(testAst(
        ".alias MyInt: Int",
        "CompilationUnit(AliasDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".domain UserId: Int",
        "CompilationUnit(DomainDecl('UserId',NamedType('Int')))"));
    CHECK(testAst(
        ".enum Fish: sockeye = 0, salmon = 1",
        "CompilationUnit(EnumDecl('Fish',EnumItem('sockeye','0'),EnumItem('salmon','1')))"));
}

TEST_CASE("Ast::COMPILATION_UNIT - module and imports") {
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"",
        "CompilationUnit(ModuleDecl('MyModule'),ImportDecl(File,'utils.basis'))"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"file1.basis\"\n"
        ".import \"file2.basis\"\n"
        ".import Std:Core",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'file1.basis'),"
            "ImportDecl(File,'file2.basis'),"
            "ImportDecl(Standard,'Core',alias='Std')"
        ")"));
    CHECK(testAst(
        ".module Std::Collections\n"
        ".import Std:Core\n"
        ".import Std:Memory",
        "CompilationUnit("
            "ModuleDecl('Std::Collections'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "ImportDecl(Standard,'Memory',alias='Std')"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"",
        "CompilationUnit(ModuleDecl('MyModule'),ImportDecl(File,'utils.basis'))"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"file1.basis\"\n"
        ".import \"file2.basis\"",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'file1.basis'),"
            "ImportDecl(File,'file2.basis')"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - module, imports, and definitions") {
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'utils.basis'),"
            "AliasDecl('MyInt',NamedType('Int'))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import Std:Core\n"
        ".domain UserId: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "DomainDecl('UserId',NamedType('Int'))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'utils.basis'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "DomainDecl('UserId',NamedType('Int')),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1'))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'utils.basis'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "DomainDecl('UserId',NamedType('Int'))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - all definition types") {
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"base.basis\"\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".record Point: Int x, Int y\n"
        ".object Node: Int value, ^Node next\n"
        ".union Scalar: Int whole, Float fractional\n"
        ".variant Shape: Int circle, ^Node inner\n"
        ".instance MyType: Interface\n"
        ".test \"simple test\" = doSomething\n"
        ".program main",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'base.basis'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "DomainDecl('UserId',NamedType('Int')),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1')),"
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "ObjectDecl("
                "'Node',"
                "FieldDecl(NamedType('Int'),'value'),"
                "FieldDecl(PtrType(1,NamedType('Node')),'next')"
            "),"
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            "),"
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            "),"
            "InstanceDecl('MyType',InstanceType('Interface')),"
            "TestDecl('simple test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doSomething'))))),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import Std:Core\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('doIt'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "CmdDef("
                "RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "CmdDecl(RegularSig('helper',param=CmdParam(NamedType('String'),'s'),ret='output')),"
            "IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('Int'),'x')))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - imports and definitions without module") {
    CHECK(testAst(
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        "CompilationUnit(ImportDecl(File,'utils.basis'),AliasDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".import Std:Core\n"
        ".import Std:IO\n"
        ".domain UserId: Int\n"
        ".record User: UserId id, String name",
        "CompilationUnit("
            "ImportDecl(Standard,'Core',alias='Std'),"
            "ImportDecl(Standard,'IO',alias='Std'),"
            "DomainDecl('UserId',NamedType('Int')),"
            "RecordDecl('User',FieldDecl(NamedType('UserId'),'id'),FieldDecl(NamedType('String'),'name'))"
        ")"));
    CHECK(testAst(
        ".import \"base.basis\"\n"
        ".enum Color: red = 0, green = 1, blue = 2\n"
        ".alias ColorCode: Int\n"
        ".test \"color test\" = validateColor",
        "CompilationUnit("
            "ImportDecl(File,'base.basis'),"
            "EnumDecl('Color',EnumItem('red','0'),EnumItem('green','1'),EnumItem('blue','2')),"
            "AliasDecl('ColorCode',NamedType('Int')),"
            "TestDecl('color test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('validateColor')))))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - complex multi-definition files") {
    CHECK(testAst(
        ".module Graphics\n"
        ".import Std:Math\n"
        ".record Point: Int x, Int y\n"
        ".record Vector: Int dx, Int dy\n"
        ".object Shape: Point origin, Int size\n"
        ".object Circle: Point center, Int radius",
        "CompilationUnit("
            "ModuleDecl('Graphics'),"
            "ImportDecl(Standard,'Math',alias='Std'),"
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "RecordDecl('Vector',FieldDecl(NamedType('Int'),'dx'),FieldDecl(NamedType('Int'),'dy')),"
            "ObjectDecl('Shape',FieldDecl(NamedType('Point'),'origin'),FieldDecl(NamedType('Int'),'size')),"
            "ObjectDecl('Circle',FieldDecl(NamedType('Point'),'center'),FieldDecl(NamedType('Int'),'radius'))"
        ")"));
    CHECK(testAst(
        ".module Types\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".enum Priority: low = 0, medium = 1, high = 2\n"
        ".alias StatusCode: Int\n"
        ".alias PriorityLevel: Int",
        "CompilationUnit("
            "ModuleDecl('Types'),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1')),"
            "EnumDecl('Priority',EnumItem('low','0'),EnumItem('medium','1'),EnumItem('high','2')),"
            "AliasDecl('StatusCode',NamedType('Int')),"
            "AliasDecl('PriorityLevel',NamedType('Int'))"
        ")"));
    CHECK(testAst(
        ".module MyApp\n"
        ".import \"base.basis\"\n"
        ".import Std:Core\n"
        ".alias UserId: Int\n"
        ".domain SessionId: Int\n"
        ".enum Role: admin = 0, user = 1\n"
        ".record User: UserId id, String name\n"
        ".object Session: SessionId id, User user\n"
        ".union Scalar: Int whole, Float fractional\n"
        ".variant Shape: Int circle, ^Node inner\n"
        ".class UserManager:\n"
        "    .decl create: User u -> result\n"
        ".instance User: Serializable\n"
        ".cmd authenticate: String username -> result = validate: username\n"
        ".decl logout: SessionId sid\n"
        ".intrinsic hashPassword: String pwd\n"
        ".test \"user creation\" = testCreate\n"
        ".program main",
        "CompilationUnit("
            "ModuleDecl('MyApp'),"
            "ImportDecl(File,'base.basis'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "AliasDecl('UserId',NamedType('Int')),"
            "DomainDecl('SessionId',NamedType('Int')),"
            "EnumDecl('Role',EnumItem('admin','0'),EnumItem('user','1')),"
            "RecordDecl('User',FieldDecl(NamedType('UserId'),'id'),FieldDecl(NamedType('String'),'name')),"
            "ObjectDecl('Session',FieldDecl(NamedType('SessionId'),'id'),FieldDecl(NamedType('User'),'user')),"
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            "),"
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            "),"
            "ClassDecl("
                "'UserManager',"
                "CmdDecl(RegularSig('create',param=CmdParam(NamedType('User'),'u'),ret='result'))"
            "),"
            "InstanceDecl('User',InstanceType('Serializable')),"
            "CmdDef("
                "RegularSig('authenticate',param=CmdParam(NamedType('String'),'username'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('validate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('username')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "CmdDecl(RegularSig('logout',param=CmdParam(NamedType('SessionId'),'sid'))),"
            "IntrinsicDecl(RegularSig('hashPassword',param=CmdParam(NamedType('String'),'pwd'))),"
            "TestDecl('user creation',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('testCreate'))))),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - definitions only (no module or imports)") {
    CHECK(testAst(
        ".alias MyInt: Int\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1",
        "CompilationUnit("
            "AliasDecl('MyInt',NamedType('Int')),"
            "DomainDecl('UserId',NamedType('Int')),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1'))"
        ")"));
    CHECK(testAst(
        ".record Point: Int x, Int y\n"
        ".object Node: Int value, ^Node next\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x",
        "CompilationUnit("
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "ObjectDecl("
                "'Node',"
                "FieldDecl(NamedType('Int'),'value'),"
                "FieldDecl(PtrType(1,NamedType('Node')),'next')"
            "),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl(ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".test \"test\" = run\n"
        ".program main",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "CmdDecl(RegularSig('helper',param=CmdParam(NamedType('String'),'s'),ret='output')),"
            "IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('Int'),'x'))),"
            "TestDecl('test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - indentation variations") {
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"utils.basis\"\n"
        ".alias MyInt: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'utils.basis'),"
            "AliasDecl('MyInt',NamedType('Int'))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - with complex class definitions") {
    CHECK(testAst(
        ".module MyModule\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x, Int y\n"
        "    .cmd doIt = process\n"
        "    .cmd render = draw\n"
        "    .decl update: Int delta\n"
        "    .cmd @ Widget w = cleanup: code",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('doIt'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
                "),"
                "CmdDef("
                    "RegularSig('render'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('draw')))))"
                "),"
                "CmdDecl(RegularSig('update',param=CmdParam(NamedType('Int'),'delta'))),"
                "CmdDef("
                    "DestructorSig(CmdReceiver(NamedType('Widget'),'w')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('cleanup'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('code')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".module UI\n"
        ".import Graphics:Core\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x, Int y\n"
        ".class Button:\n"
        "    .decl Button b: String label\n"
        "    .cmd click = handleClick\n"
        ".class Container:\n"
        "    .decl Container c: Int capacity",
        "CompilationUnit("
            "ModuleDecl('UI'),"
            "ImportDecl(Standard,'Core',alias='Graphics'),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                ")"
            "),"
            "ClassDecl("
                "'Button',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Button'),'b'),"
                        "CmdParam(NamedType('String'),'label')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('click'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('handleClick')))))"
                ")"
            "),"
            "ClassDecl("
                "'Container',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Container'),'c'),"
                        "CmdParam(NamedType('Int'),'capacity')"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - with test and program definitions") {
    CHECK(testAst(
        ".module Tests\n"
        ".test \"test 1\" = run1\n"
        ".test \"test 2\" = run2\n"
        ".test \"test 3\" = run3 | fallback",
        "CompilationUnit("
            "ModuleDecl('Tests'),"
            "TestDecl('test 1',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run1'))))),"
            "TestDecl('test 2',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run2'))))),"
            "TestDecl("
                "'test 3',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('run3')),"
                            "'|',"
                            "CallCommandExpr(IdentifierExpr('fallback'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".module MyApp\n"
        ".alias MyInt: Int\n"
        ".cmd helper: Int x -> result = process: x\n"
        ".test \"helper test\" = testHelper: 42\n"
        ".program main",
        "CompilationUnit("
            "ModuleDecl('MyApp'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "CmdDef("
                "RegularSig('helper',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "TestDecl("
                "'helper test',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('testHelper'),CallParam(LiteralExpr('42'))))"
                ")"
            "),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main')))"
        ")"));
    CHECK(testAst(
        ".module MyApp\n"
        ".import Std:Core\n"
        ".cmd initialize = setup\n"
        ".cmd run: []String args = execute: args\n"
        ".program run: args",
        "CompilationUnit("
            "ModuleDecl('MyApp'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "CmdDef("
                "RegularSig('initialize'),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('setup')))))"
            "),"
            "CmdDef("
                "RegularSig('run',param=CmdParam(RangeType('',NamedType('String')),'args')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('execute'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('args')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "ProgramDecl("
                "CallCommandExpr(IdentifierExpr('run'),CallParam(CallCommandExpr(IdentifierExpr('args'))))"
            ")"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - order variations") {
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"base.basis\"\n"
        ".enum Status: active = 0\n"
        ".alias MyInt: Int\n"
        ".record Point: Int x, Int y\n"
        ".domain UserId: Int\n"
        ".object Node: Int value",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'base.basis'),"
            "EnumDecl('Status',EnumItem('active','0')),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "DomainDecl('UserId',NamedType('Int')),"
            "ObjectDecl('Node',FieldDecl(NamedType('Int'),'value'))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".cmd doIt = process\n"
        ".decl helper: Int x\n"
        ".intrinsic native: String s\n"
        ".cmd other = run\n"
        ".decl another: Int y",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "CmdDef("
                "RegularSig('doIt'),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
            "),"
            "CmdDecl(RegularSig('helper',param=CmdParam(NamedType('Int'),'x'))),"
            "IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('String'),'s'))),"
            "CmdDef(RegularSig('other'),CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run')))))),"
            "CmdDecl(RegularSig('another',param=CmdParam(NamedType('Int'),'y')))"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x\n"
        ".instance Widget: Interface\n"
        ".test \"test\" = run\n"
        ".class Button:\n"
        "    .decl Button b: String label\n"
        ".test \"another test\" = run2",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl(ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')))"
            "),"
            "InstanceDecl('Widget',InstanceType('Interface')),"
            "TestDecl('test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))),"
            "ClassDecl("
                "'Button',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Button'),'b'),"
                        "CmdParam(NamedType('String'),'label')"
                    ")"
                ")"
            "),"
            "TestDecl('another test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run2')))))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - single definition types") {
    CHECK(testAst(
        ".alias MyInt: Int",
        "CompilationUnit(AliasDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .decl Widget w: Int x",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDecl(ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd doIt = process",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('doIt'),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".decl helper: Int x -> result",
        "CompilationUnit(CmdDecl(RegularSig('helper',param=CmdParam(NamedType('Int'),'x'),ret='result')))"));
    CHECK(testAst(
        ".intrinsic native: String s",
        "CompilationUnit(IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('String'),'s'))))"));
    CHECK(testAst(
        ".domain UserId: Int",
        "CompilationUnit(DomainDecl('UserId',NamedType('Int')))"));
    CHECK(testAst(
        ".enum Status: active = 0",
        "CompilationUnit(EnumDecl('Status',EnumItem('active','0')))"));
    CHECK(testAst(
        ".record Point: Int x, Int y",
        "CompilationUnit(RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"));
    CHECK(testAst(
        ".object Node: Int value",
        "CompilationUnit(ObjectDecl('Node',FieldDecl(NamedType('Int'),'value')))"));
    CHECK(testAst(
        ".union Scalar: Int whole, Float fractional",
        "CompilationUnit("
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Shape: Int circle, ^Node inner",
        "CompilationUnit("
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            ")"
        ")"));
    CHECK(testAst(
        ".instance MyType: Interface",
        "CompilationUnit(InstanceDecl('MyType',InstanceType('Interface')))"));
    CHECK(testAst(
        ".test \"test\" = run",
        "CompilationUnit(TestDecl('test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))))"));
    CHECK(testAst(
        ".program main",
        "CompilationUnit(ProgramDecl(CallCommandExpr(IdentifierExpr('main'))))"));
}

TEST_CASE("Ast::COMPILATION_UNIT - module only variations") {
    CHECK(testAst(
        ".module MyModule",
        "CompilationUnit(ModuleDecl('MyModule'))"));
    CHECK(testAst(
        ".module Std::Collections::List",
        "CompilationUnit(ModuleDecl('Std::Collections::List'))"));
    CHECK(testAst(
        ".module MyModule\n"
        ".import \"file.basis\"\n"
        ".import Std:Core",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'file.basis'),"
            "ImportDecl(Standard,'Core',alias='Std')"
        ")"));
    CHECK(testAst(
        ".module MyModule\n"
        ".alias MyInt: Int\n"
        ".domain UserId: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "DomainDecl('UserId',NamedType('Int'))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - realistic file examples") {
    CHECK(testAst(
        ".module Std::Utils\n"
        ".import Std:Core\n"
        ".alias StringList: List[String]\n"
        ".cmd join: StringList items, String separator -> result = _\n"
        ".cmd split: String text, String delimiter -> result = _\n"
        ".test \"join test\" = testJoin\n"
        ".test \"split test\" = testSplit",
        "CompilationUnit("
            "ModuleDecl('Std::Utils'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "AliasDecl('StringList',NamedType('List')),"
            "CmdDef("
                "RegularSig("
                    "'join',"
                    "param=CmdParam(NamedType('StringList'),'items'),"
                    "param=CmdParam(NamedType('String'),'separator'),"
                    "ret='result'"
                "),"
                "CmdBody(empty)"
            "),"
            "CmdDef("
                "RegularSig("
                    "'split',"
                    "param=CmdParam(NamedType('String'),'text'),"
                    "param=CmdParam(NamedType('String'),'delimiter'),"
                    "ret='result'"
                "),"
                "CmdBody(empty)"
            "),"
            "TestDecl('join test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('testJoin'))))),"
            "TestDecl('split test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('testSplit')))))"
        ")"));
    CHECK(testAst(
        ".module App::Models\n"
        ".import Std:Core\n"
        ".domain UserId: Int\n"
        ".domain SessionId: Int\n"
        ".enum UserRole: admin = 0, user = 1, guest = 2\n"
        ".record User: UserId id, String name, String email, UserRole role\n"
        ".record Session: SessionId id, UserId userId, Int timestamp\n"
        ".object UserManager: List[User] users\n"
        ".instance User: Serializable, Comparable",
        "CompilationUnit("
            "ModuleDecl('App::Models'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "DomainDecl('UserId',NamedType('Int')),"
            "DomainDecl('SessionId',NamedType('Int')),"
            "EnumDecl('UserRole',EnumItem('admin','0'),EnumItem('user','1'),EnumItem('guest','2')),"
            "RecordDecl("
                "'User',"
                "FieldDecl(NamedType('UserId'),'id'),"
                "FieldDecl(NamedType('String'),'name'),"
                "FieldDecl(NamedType('String'),'email'),"
                "FieldDecl(NamedType('UserRole'),'role')"
            "),"
            "RecordDecl("
                "'Session',"
                "FieldDecl(NamedType('SessionId'),'id'),"
                "FieldDecl(NamedType('UserId'),'userId'),"
                "FieldDecl(NamedType('Int'),'timestamp')"
            "),"
            "ObjectDecl('UserManager',FieldDecl(NamedType('List'),'users')),"
            "InstanceDecl('User',InstanceType('Serializable'),InstanceType('Comparable'))"
        ")"));
    CHECK(testAst(
        ".module App::Services\n"
        ".import App:Models\n"
        ".import Std:Core\n"
        ".class AuthService:\n"
        "    .decl AuthService s: String secret\n"
        "    .cmd login: String username, String password -> result = authenticate: username, password\n"
        "    .cmd logout: SessionId sid = invalidate: sid\n"
        "    .decl validateToken: String token -> result\n"
        ".class UserService:\n"
        "    .decl UserService s: UserManager manager\n"
        "    .cmd createUser: String name, String email -> result = (manager):: add: name, email\n"
        "    .cmd getUser: UserId id -> result = (manager):: find: id\n"
        ".test \"auth test\" = testAuth\n"
        ".test \"user test\" = testUser",
        "CompilationUnit("
            "ModuleDecl('App::Services'),"
            "ImportDecl(Standard,'Models',alias='App'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "ClassDecl("
                "'AuthService',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('AuthService'),'s'),"
                        "CmdParam(NamedType('String'),'secret')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig("
                        "'login',"
                        "param=CmdParam(NamedType('String'),'username'),"
                        "param=CmdParam(NamedType('String'),'password'),"
                        "ret='result'"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('authenticate'),"
                                    "CallParam(IdentifierExpr('username')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('password')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('logout',param=CmdParam(NamedType('SessionId'),'sid')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('invalidate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('sid')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDecl("
                    "RegularSig('validateToken',param=CmdParam(NamedType('String'),'token'),ret='result')"
                ")"
            "),"
            "ClassDecl("
                "'UserService',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('UserService'),'s'),"
                        "CmdParam(NamedType('UserManager'),'manager')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig("
                        "'createUser',"
                        "param=CmdParam(NamedType('String'),'name'),"
                        "param=CmdParam(NamedType('String'),'email'),"
                        "ret='result'"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'add',"
                                    "receiver='manager',"
                                    "CallParam(IdentifierExpr('name')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('email')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('getUser',param=CmdParam(NamedType('UserId'),'id'),ret='result'),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'find',"
                                    "receiver='manager',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('id')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "TestDecl('auth test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('testAuth'))))),"
            "TestDecl('user test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('testUser')))))"
        ")"));
    CHECK(testAst(
        ".module App::Main\n"
        ".import App:Services\n"
        ".import App:Models\n"
        ".import Std:Core\n"
        ".import Std:IO\n"
        ".cmd initialize = setupDatabase\n"
        "    loadConfig\n"
        ".cmd run: []String args = initialize\n"
        "    startServer: args\n"
        ".program run: args",
        "CompilationUnit("
            "ModuleDecl('App::Main'),"
            "ImportDecl(Standard,'Services',alias='App'),"
            "ImportDecl(Standard,'Models',alias='App'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "ImportDecl(Standard,'IO',alias='Std'),"
            "CmdDef("
                "RegularSig('initialize'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('setupDatabase'))),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('loadConfig')))"
                    ")"
                ")"
            "),"
            "CmdDef("
                "RegularSig('run',param=CmdParam(RangeType('',NamedType('String')),'args')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('initialize'))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('startServer'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('args')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "ProgramDecl("
                "CallCommandExpr(IdentifierExpr('run'),CallParam(CallCommandExpr(IdentifierExpr('args'))))"
            ")"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - edge cases with whitespace and newlines") {
    CHECK(testAst(
        ".module MyModule\n"
        "\n"
        ".import \"file.basis\"\n"
        "\n"
        "\n"
        ".alias MyInt: Int",
        "CompilationUnit("
            "ModuleDecl('MyModule'),"
            "ImportDecl(File,'file.basis'),"
            "AliasDecl('MyInt',NamedType('Int'))"
        ")"));
}

TEST_CASE("Ast::COMPILATION_UNIT - all 15 definition types together") {
    CHECK(testAst(
        ".module Comprehensive\n"
        ".import Std:Core\n"
        ".alias MyInt: Int\n"
        ".class Widget:\n"
        "    .decl Widget w: Int x\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".instance Widget: Interface\n"
        ".object Node: Int value\n"
        ".program main\n"
        ".record Point: Int x, Int y\n"
        ".test \"comprehensive test\" = run\n"
        ".union Scalar: Int whole, Float fractional\n"
        ".variant Shape: Int circle, ^Node inner",
        "CompilationUnit("
            "ModuleDecl('Comprehensive'),"
            "ImportDecl(Standard,'Core',alias='Std'),"
            "AliasDecl('MyInt',NamedType('Int')),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl(ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')))"
            "),"
            "CmdDef("
                "RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "CmdDecl(RegularSig('helper',param=CmdParam(NamedType('String'),'s'),ret='output')),"
            "IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('Int'),'x'))),"
            "DomainDecl('UserId',NamedType('Int')),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1')),"
            "InstanceDecl('Widget',InstanceType('Interface')),"
            "ObjectDecl('Node',FieldDecl(NamedType('Int'),'value')),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main'))),"
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "TestDecl('comprehensive test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))),"
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            "),"
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            ")"
        ")"));
    CHECK(testAst(
        ".alias MyInt: Int\n"
        ".class Widget:\n"
        "  .decl Widget w: Int x\n"
        ".cmd doSomething: Int x -> result = process: x\n"
        ".decl helper: String s -> output\n"
        ".intrinsic native: Int x\n"
        ".domain UserId: Int\n"
        ".enum Status: active = 0, inactive = 1\n"
        ".instance Widget: Interface\n"
        ".object Node: Int value\n"
        ".program main\n"
        ".record Point: Int x, Int y\n"
        ".test \"comprehensive test\" = run\n"
        ".union Scalar: Int whole, Float fractional\n"
        ".variant Shape: Int circle, ^Node inner",
        "CompilationUnit("
            "AliasDecl('MyInt',NamedType('Int')),"
            "ClassDecl("
                "'Widget',"
                "CmdDecl(ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')))"
            "),"
            "CmdDef("
                "RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            "),"
            "CmdDecl(RegularSig('helper',param=CmdParam(NamedType('String'),'s'),ret='output')),"
            "IntrinsicDecl(RegularSig('native',param=CmdParam(NamedType('Int'),'x'))),"
            "DomainDecl('UserId',NamedType('Int')),"
            "EnumDecl('Status',EnumItem('active','0'),EnumItem('inactive','1')),"
            "InstanceDecl('Widget',InstanceType('Interface')),"
            "ObjectDecl('Node',FieldDecl(NamedType('Int'),'value')),"
            "ProgramDecl(CallCommandExpr(IdentifierExpr('main'))),"
            "RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
            "TestDecl('comprehensive test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('run'))))),"
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            "),"
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_MODULE") {
    CHECK(testAst(
        ".module MyModule",
        "CompilationUnit(ModuleDecl('MyModule'))"));
    CHECK(testAst(
        ".module Std::Collections",
        "CompilationUnit(ModuleDecl('Std::Collections'))"));
    CHECK(testAst(
        ".module A::B::C",
        "CompilationUnit(ModuleDecl('A::B::C'))"));
}

TEST_CASE("Ast::DEF_IMPORT") {
    CHECK(testAst(
        ".import \"file.basis\"",
        "CompilationUnit(ImportDecl(File,'file.basis'))"));
    CHECK(testAst(
        ".import Module",
        "CompilationUnit(ImportDecl(Standard,'Module'))"));
    CHECK(testAst(
        ".import MyLib:Utils",
        "CompilationUnit(ImportDecl(Standard,'Utils',alias='MyLib'))"));
    CHECK(testAst(
        ".import A:B",
        "CompilationUnit(ImportDecl(Standard,'B',alias='A'))"));
    CHECK(testAst(
        ".import Prefix:A::B::C",
        "CompilationUnit(ImportDecl(Standard,'A::B::C',alias='Prefix'))"));
    CHECK(testAst(
        ".import Std::Collections",
        "CompilationUnit(ImportDecl(Standard,'Std::Collections'))"));
    CHECK(testAst(
        ".import A::B::C",
        "CompilationUnit(ImportDecl(Standard,'A::B::C'))"));
}

TEST_CASE("Ast::DEF_PROGRAM") {
    CHECK(testAst(
        ".program main",
        "CompilationUnit(ProgramDecl(CallCommandExpr(IdentifierExpr('main'))))"));
    CHECK(testAst(
        ".program start: arg1, arg2",
        "CompilationUnit("
            "ProgramDecl("
                "CallCommandExpr("
                    "IdentifierExpr('start'),"
                    "CallParam(CallCommandExpr(IdentifierExpr('arg1'))),"
                    "CallParam(CallCommandExpr(IdentifierExpr('arg2')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program run: config",
        "CompilationUnit("
            "ProgramDecl("
                "CallCommandExpr(IdentifierExpr('run'),CallParam(CallCommandExpr(IdentifierExpr('config'))))"
            ")"
        ")"));
    CHECK(testAst(
        ".program .fail x",
        "CompilationUnit(ProgramDecl(CallFailExpr(CallCommandExpr(IdentifierExpr('x')))))"));
}

TEST_CASE("Ast::DEF_TEST") {
    CHECK(testAst(
        ".test \"simple test\" = doSomething",
        "CompilationUnit("
            "TestDecl('simple test',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doSomething')))))"
        ")"));
    CHECK(testAst(
        ".test \"test with params\" = run: arg1, arg2",
        "CompilationUnit("
            "TestDecl("
                "'test with params',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('run'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('arg1'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('arg2')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"complex test\" = setup\n"
        "    validate: data",
        "CompilationUnit("
            "TestDecl("
                "'complex test',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('setup'))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('validate'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"constructor test\" = Widget: 10, 20",
        "CompilationUnit("
            "TestDecl("
                "'constructor test',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(LiteralExpr('10')),"
                            "CallParam(LiteralExpr('20'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"assignment test\" = result <- compute: x",
        "CompilationUnit("
            "TestDecl("
                "'assignment test',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallCommandExpr("
                            "IdentifierExpr('compute'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"fail test\" = .fail x",
        "CompilationUnit("
            "TestDecl('fail test',CallGroup(ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('x'))))))"
        ")"));
    CHECK(testAst(
        ".test \"fail expression test\" = .fail x + y",
        "CompilationUnit("
            "TestDecl("
                "'fail expression test',"
                "CallGroup("
                    "ExprStat("
                        "CallFailExpr("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_TEST - with CALL_EXPRESSION") {
    CHECK(testAst(
        ".test \"arithmetic test\" = a + b",
        "CompilationUnit("
            "TestDecl("
                "'arithmetic test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"subtraction test\" = x - y",
        "CompilationUnit("
            "TestDecl("
                "'subtraction test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"multiplication test\" = width * height",
        "CompilationUnit("
            "TestDecl("
                "'multiplication test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('width')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('height'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"division test\" = sum / count",
        "CompilationUnit("
            "TestDecl("
                "'division test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('sum')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('count'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"complex expression\" = (a + b) * (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'complex expression',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"comparison test\" = x < y",
        "CompilationUnit("
            "TestDecl("
                "'comparison test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'<',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"array test\" = arr[0] + arr[1]",
        "CompilationUnit("
            "TestDecl("
                "'array test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0'))),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"pointer test\" = ptr^ + offset",
        "CompilationUnit("
            "TestDecl("
                "'pointer test',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"mixed test\" = setup\n"
        "      a + b\n"
        "      validate: result",
        "CompilationUnit("
            "TestDecl("
                "'mixed test',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('setup'))),"
                    "ExprStat(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('validate'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('result')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"expression with invoke\" = (Widget: x, y) + offset",
        "CompilationUnit("
            "TestDecl("
                "'expression with invoke',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CLASS") {
    CHECK(testAst(
        ".class MyClass:\n"
        "  .decl doSomething: Int x -> result\n"
        "  .decl doOther: String s -> output",
        "CompilationUnit("
            "ClassDecl("
                "'MyClass',"
                "CmdDecl(RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result')),"
                "CmdDecl(RegularSig('doOther',param=CmdParam(NamedType('String'),'s'),ret='output'))"
            ")"
        ")"));
    CHECK(testAst(
        ".class MyClass: .decl doSomething: Int x -> result\n"
        "                .decl doOther: String s -> output",
        "CompilationUnit("
            "ClassDecl("
                "'MyClass',"
                "CmdDecl(RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result')),"
                "CmdDecl(RegularSig('doOther',param=CmdParam(NamedType('String'),'s'),ret='output'))"
            ")"
        ")"));
    CHECK(testAst(
        ".class MyClass: .decl doSomething: Int x -> result\n"
        "  .decl doOther: String s -> output",
        "CompilationUnit("
            "ClassDecl("
                "'MyClass',"
                "CmdDecl(RegularSig('doSomething',param=CmdParam(NamedType('Int'),'x'),ret='result')),"
                "CmdDecl(RegularSig('doOther',param=CmdParam(NamedType('String'),'s'),ret='output'))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .decl Container c: Int capacity\n"
        "  .decl add: Int item -> result\n"
        "  .decl (Widget w):: process: String s -> output / Int ctx",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Container'),'c'),"
                        "CmdParam(NamedType('Int'),'capacity')"
                    ")"
                "),"
                "CmdDecl(RegularSig('add',param=CmdParam(NamedType('Int'),'item'),ret='result')),"
                "CmdDecl("
                    "VCommandSig("
                        "'process',"
                        "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                        "param=CmdParam(NamedType('String'),'s'),"
                        "implicit=CmdParam(NamedType('Int'),'ctx'),"
                        "ret='output'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Processor:\n"
        "  .decl (Widget w, Button b):: handle: Int x -> result",
        "CompilationUnit("
            "ClassDecl("
                "'Processor',"
                "CmdDecl("
                    "VCommandSig("
                        "'handle',"
                        "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                        "receiver=CmdReceiver(NamedType('Button'),'b'),"
                        "param=CmdParam(NamedType('Int'),'x'),"
                        "ret='result'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .decl ?tryProcess: Int x -> result\n"
        "  .decl !mustFail: String s -> output",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDecl("
                    "RegularSig('tryProcess',fail=MayFail,param=CmdParam(NamedType('Int'),'x'),ret='result')"
                "),"
                "CmdDecl("
                    "RegularSig('mustFail',fail=Fails,param=CmdParam(NamedType('String'),'s'),ret='output')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Resource:\n"
        "  .decl Resource r: String name\n"
        "  .decl @ Resource r",
        "CompilationUnit("
            "ClassDecl("
                "'Resource',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Resource'),'r'),"
                        "CmdParam(NamedType('String'),'name')"
                    ")"
                "),"
                "CmdDecl(DestructorSig(CmdReceiver(NamedType('Resource'),'r')))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Transaction:\n"
        "  .decl Transaction t: Int id\n"
        "  .decl @! Transaction t",
        "CompilationUnit("
            "ClassDecl("
                "'Transaction',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Transaction'),'t'),"
                        "CmdParam(NamedType('Int'),'id')"
                    ")"
                "),"
                "CmdDecl(FailHandlerSig(CmdReceiver(NamedType('Transaction'),'t')))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Connection:\n"
        "  .decl Connection c: String host, Int port\n"
        "  .decl @ Connection c\n"
        "  .decl @! Connection c",
        "CompilationUnit("
            "ClassDecl("
                "'Connection',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Connection'),'c'),"
                        "CmdParam(NamedType('String'),'host'),"
                        "CmdParam(NamedType('Int'),'port')"
                    ")"
                "),"
                "CmdDecl(DestructorSig(CmdReceiver(NamedType('Connection'),'c'))),"
                "CmdDecl(FailHandlerSig(CmdReceiver(NamedType('Connection'),'c')))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd doIt = process",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "RegularSig('doIt'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd doIt: Int x -> result = process: x",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "RegularSig('doIt',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .cmd add: Int item = (items):: append: item",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDef("
                    "RegularSig('add',param=CmdParam(NamedType('Int'),'item')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'append',"
                                    "receiver='items',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .cmd process: String data -> result = validate: data\n"
        "     result <- (transform: data)",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDef("
                    "RegularSig('process',param=CmdParam(NamedType('String'),'data'),ret='result'),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            "),"
                            "AssignStat("
                                "IdentifierExpr('result'),"
                                "CallCommandExpr("
                                    "IdentifierExpr('transform'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd doIt = _",
        "CompilationUnit(ClassDecl('Widget',CmdDef(RegularSig('doIt'),CmdBody(empty))))"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd process: Int x -> result = _",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "RegularSig('process',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                    "CmdBody(empty)"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .cmd ?tryProcess: String data -> result = _",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDef("
                    "RegularSig("
                        "'tryProcess',"
                        "fail=MayFail,"
                        "param=CmdParam(NamedType('String'),'data'),"
                        "ret='result'"
                    "),"
                    "CmdBody(empty)"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .cmd !mustFail: Int code = _",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDef("
                    "RegularSig('mustFail',fail=Fails,param=CmdParam(NamedType('Int'),'code')),"
                    "CmdBody(empty)"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .decl (Widget w, Int m) :: print: Int x, Int y\n"
        "  .cmd doIt = process",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                "),"
                "CmdDecl("
                    "VCommandSig("
                        "'print',"
                        "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                        "receiver=CmdReceiver(NamedType('Int'),'m'),"
                        "param=CmdParam(NamedType('Int'),'x'),"
                        "param=CmdParam(NamedType('Int'),'y')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('doIt'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .decl Container c: Int capacity\n"
        "  .cmd add: Int item = (items):: append: item\n"
        "  .decl remove: Int index -> result",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Container'),'c'),"
                        "CmdParam(NamedType('Int'),'capacity')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('add',param=CmdParam(NamedType('Int'),'item')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'append',"
                                    "receiver='items',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDecl(RegularSig('remove',param=CmdParam(NamedType('Int'),'index'),ret='result'))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .decl Handler h: String name\n"
        "  .cmd process: String data = validate: data\n"
        "  .cmd Handler h::process: String data = validate: data\n"
        "  .cmd cleanup = _\n"
        "  .decl @ Handler h",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Handler'),'h'),"
                        "CmdParam(NamedType('String'),'name')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('process',param=CmdParam(NamedType('String'),'data')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef("
                    "VCommandSig("
                        "'process',"
                        "receiver=CmdReceiver(NamedType('Handler'),'h'),"
                        "param=CmdParam(NamedType('String'),'data')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef(RegularSig('cleanup'),CmdBody(empty)),"
                "CmdDecl(DestructorSig(CmdReceiver(NamedType('Handler'),'h')))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd Widget w: Int x, Int y = init: x, y",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('init'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .cmd Container c: Int capacity = allocate: capacity",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDef("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Container'),'c'),"
                        "CmdParam(NamedType('Int'),'capacity')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('allocate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('capacity')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Resource:\n"
        "  .cmd Resource r: String name = acquire: name\n"
        "  .cmd @ Resource r = release: code",
        "CompilationUnit("
            "ClassDecl("
                "'Resource',"
                "CmdDef("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Resource'),'r'),"
                        "CmdParam(NamedType('String'),'name')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('acquire'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef("
                    "DestructorSig(CmdReceiver(NamedType('Resource'),'r')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('release'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('code')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Connection:\n"
        "  .cmd @ Connection c= disconnect\n"
        "    cleanup",
        "CompilationUnit("
            "ClassDecl("
                "'Connection',"
                "CmdDef("
                    "DestructorSig(CmdReceiver(NamedType('Connection'),'c')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat(CallCommandExpr(IdentifierExpr('disconnect'))),"
                            "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Transaction:\n"
        "  .cmd Transaction t: Int id = begin: id\n"
        "  .cmd @! Transaction t= rollback: error",
        "CompilationUnit("
            "ClassDecl("
                "'Transaction',"
                "CmdDef("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Transaction'),'t'),"
                        "CmdParam(NamedType('Int'),'id')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('begin'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('id')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDef("
                    "FailHandlerSig(CmdReceiver(NamedType('Transaction'),'t')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('rollback'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Connection:\n"
        "  .cmd @! Connection c= logError: reason\n"
        "    notify",
        "CompilationUnit("
            "ClassDecl("
                "'Connection',"
                "CmdDef("
                    "FailHandlerSig(CmdReceiver(NamedType('Connection'),'c')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('logError'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('reason')))"
                                ")"
                            "),"
                            "ExprStat(CallCommandExpr(IdentifierExpr('notify')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd (Widget w):: render = draw: w",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "VCommandSig('render',receiver=CmdReceiver(NamedType('Widget'),'w')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('draw'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('w')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .cmd (Container c):: process: Int x -> result = validate: x\n"
        "    result <- (transform: x)",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDef("
                    "VCommandSig("
                        "'process',"
                        "receiver=CmdReceiver(NamedType('Container'),'c'),"
                        "param=CmdParam(NamedType('Int'),'x'),"
                        "ret='result'"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            "),"
                            "AssignStat("
                                "IdentifierExpr('result'),"
                                "CallCommandExpr("
                                    "IdentifierExpr('transform'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .cmd (Widget w, Button b):: handle: Int x = (w):: update: x\n"
        "    (b):: click",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDef("
                    "VCommandSig("
                        "'handle',"
                        "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                        "receiver=CmdReceiver(NamedType('Button'),'b'),"
                        "param=CmdParam(NamedType('Int'),'x')"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'update',"
                                    "receiver='w',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            "),"
                            "ExprStat(CallVCommandExpr('click',receiver='b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Container:\n"
        "  .cmd (Container c):: getSize -> result = (c):: computeSize",
        "CompilationUnit("
            "ClassDecl("
                "'Container',"
                "CmdDef("
                    "VCommandSig('getSize',receiver=CmdReceiver(NamedType('Container'),'c'),ret='result'),"
                    "CmdBody(CallGroup(ExprStat(CallVCommandExpr('computeSize',receiver='c'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd (Widget w):: getValue -> w = (w):: extract",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "VCommandSig('getValue',receiver=CmdReceiver(NamedType('Widget'),'w'),ret='w'),"
                    "CmdBody(CallGroup(ExprStat(CallVCommandExpr('extract',receiver='w'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Handler:\n"
        "  .cmd (Handler h, Context ctx):: ?tryGet -> result = (h):: attempt: ctx",
        "CompilationUnit("
            "ClassDecl("
                "'Handler',"
                "CmdDef("
                    "VCommandSig("
                        "'tryGet',"
                        "fail=MayFail,"
                        "receiver=CmdReceiver(NamedType('Handler'),'h'),"
                        "receiver=CmdReceiver(NamedType('Context'),'ctx'),"
                        "ret='result'"
                    "),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallVCommandExpr("
                                    "'attempt',"
                                    "receiver='h',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('ctx')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .decl Widget w: Int x, Int y\n"
        "  .cmd doIt = process\n"
        "  .cmd ?tryIt: Int x -> result = validate: x\n"
        "  .decl !mustFail: String s -> output\n"
        "  .cmd cleanup = _\n"
        "  .cmd @ Widget w = release: code\n"
        "  .decl @! Widget w",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDecl("
                    "ConstructorSig("
                        "CmdReceiver(NamedType('Widget'),'w'),"
                        "CmdParam(NamedType('Int'),'x'),"
                        "CmdParam(NamedType('Int'),'y')"
                    ")"
                "),"
                "CmdDef("
                    "RegularSig('doIt'),"
                    "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process')))))"
                "),"
                "CmdDef("
                    "RegularSig('tryIt',fail=MayFail,param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDecl("
                    "RegularSig('mustFail',fail=Fails,param=CmdParam(NamedType('String'),'s'),ret='output')"
                "),"
                "CmdDef(RegularSig('cleanup'),CmdBody(empty)),"
                "CmdDef("
                    "DestructorSig(CmdReceiver(NamedType('Widget'),'w')),"
                    "CmdBody("
                        "CallGroup("
                            "ExprStat("
                                "CallCommandExpr("
                                    "IdentifierExpr('release'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('code')))"
                                ")"
                            ")"
                        ")"
                    ")"
                "),"
                "CmdDecl(FailHandlerSig(CmdReceiver(NamedType('Widget'),'w')))"
            ")"
        ")"));
    CHECK(testAst(
        ".class Widget:\n"
        "  .cmd Widget w: Int x = _\n"
        "  .cmd @ Widget w = _\n"
        "  .cmd @! Widget w = _\n"
        "  .cmd (Widget w):: method = _\n"
        "  .cmd process: Int x -> result = _",
        "CompilationUnit("
            "ClassDecl("
                "'Widget',"
                "CmdDef("
                    "ConstructorSig(CmdReceiver(NamedType('Widget'),'w'),CmdParam(NamedType('Int'),'x')),"
                    "CmdBody(empty)"
                "),"
                "CmdDef(DestructorSig(CmdReceiver(NamedType('Widget'),'w')),CmdBody(empty)),"
                "CmdDef(FailHandlerSig(CmdReceiver(NamedType('Widget'),'w')),CmdBody(empty)),"
                "CmdDef(VCommandSig('method',receiver=CmdReceiver(NamedType('Widget'),'w')),CmdBody(empty)),"
                "CmdDef("
                    "RegularSig('process',param=CmdParam(NamedType('Int'),'x'),ret='result'),"
                    "CmdBody(empty)"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_ENUM") {
    CHECK(testAst(
        ".enum Fish: sockeye = 0, salmon = 1",
        "CompilationUnit(EnumDecl('Fish',EnumItem('sockeye','0'),EnumItem('salmon','1')))"));
    CHECK(testAst(
        ".enum T Fish: sockeye = 0, salmon = 1",
        "CompilationUnit(EnumDecl('Fish',constraint='T',EnumItem('sockeye','0'),EnumItem('salmon','1')))"));
}

TEST_CASE("Ast::DEF_DOMAIN") {
    CHECK(testAst(
        ".domain MyInt: Int",
        "CompilationUnit(DomainDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".domain UserId: Int",
        "CompilationUnit(DomainDecl('UserId',NamedType('Int')))"));
    CHECK(testAst(
        ".domain Temperature: Float",
        "CompilationUnit(DomainDecl('Temperature',NamedType('Float')))"));
    CHECK(testAst(
        ".domain FixedBuffer: [10]",
        "CompilationUnit(DomainDecl('FixedBuffer',RangeType('10')))"));
    CHECK(testAst(
        ".domain MyInt:\n"
        " Int",
        "CompilationUnit(DomainDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".domain MyType: Std::Int",
        "CompilationUnit(DomainDecl('MyType',NamedType('Std::Int')))"));
    CHECK(testAst(
        ".domain UserId: Database::Types::Id",
        "CompilationUnit(DomainDecl('UserId',NamedType('Database::Types::Id')))"));
    CHECK(testAst(
        ".domain ByteArray: [256]Byte",
        "CompilationUnit(DomainDecl('ByteArray',RangeType('256',NamedType('Byte'))))"));
}

TEST_CASE("Ast::DEF_RECORD") {
    CHECK(testAst(
        ".record Person: String name",
        "CompilationUnit(RecordDecl('Person',FieldDecl(NamedType('String'),'name')))"));
    CHECK(testAst(
        ".record Point3D: Int x, Int y, Int z",
        "CompilationUnit("
            "RecordDecl("
                "'Point3D',"
                "FieldDecl(NamedType('Int'),'x'),"
                "FieldDecl(NamedType('Int'),'y'),"
                "FieldDecl(NamedType('Int'),'z')"
            ")"
        ")"));
    CHECK(testAst(
        ".record Pair[T]: T first, T second",
        "CompilationUnit("
            "RecordDecl('Pair',FieldDecl(NamedType('T'),'first'),FieldDecl(NamedType('T'),'second'))"
        ")"));
    CHECK(testAst(
        ".record Pair[T, U]: T first, U second",
        "CompilationUnit("
            "RecordDecl('Pair',FieldDecl(NamedType('T'),'first'),FieldDecl(NamedType('U'),'second'))"
        ")"));
    CHECK(testAst(
        ".record Buffer[Int size]: [size]Byte data",
        "CompilationUnit(RecordDecl('Buffer',FieldDecl(RangeType('size',NamedType('Byte')),'data')))"));
    CHECK(testAst(
        ".record Matrix[Int rows, Int cols]: [rows][cols]Float values",
        "CompilationUnit("
            "RecordDecl('Matrix',FieldDecl(RangeType('rows',RangeType('cols',NamedType('Float'))),'values'))"
        ")"));
    CHECK(testAst(
        ".record Array[T, Int size]: [size]T elements",
        "CompilationUnit(RecordDecl('Array',FieldDecl(RangeType('size',NamedType('T')),'elements')))"));
    CHECK(testAst(
        ".record Container: List[Int] items",
        "CompilationUnit(RecordDecl('Container',FieldDecl(NamedType('List',NamedType('Int')),'items')))"));
    CHECK(testAst(
        ".record Mapping: Map[String, Int] data",
        "CompilationUnit("
            "RecordDecl('Mapping',FieldDecl(NamedType('Map',NamedType('String'),NamedType('Int')),'data'))"
        ")"));
    CHECK(testAst(
        ".record Complex: List[Pair[String, Int]] entries",
        "CompilationUnit("
            "RecordDecl("
                "'Complex',"
                "FieldDecl("
                    "NamedType('List',NamedType('Pair',NamedType('String'),NamedType('Int'))),"
                    "'entries'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".record Buffer: [256]Byte data",
        "CompilationUnit(RecordDecl('Buffer',FieldDecl(RangeType('256',NamedType('Byte')),'data')))"));
    CHECK(testAst(
        ".record Matrix4x4: [4][4]Float elements",
        "CompilationUnit("
            "RecordDecl('Matrix4x4',FieldDecl(RangeType('4',RangeType('4',NamedType('Float'))),'elements'))"
        ")"));
    CHECK(testAst(
        ".record Entity: String name, [3]Float position, List[Component] components",
        "CompilationUnit("
            "RecordDecl("
                "'Entity',"
                "FieldDecl(NamedType('String'),'name'),"
                "FieldDecl(RangeType('3',NamedType('Float')),'position'),"
                "FieldDecl(NamedType('List',NamedType('Component')),'components')"
            ")"
        ")"));
    CHECK(testAst(
        ".record Point:\n"
        " Int x, Int y",
        "CompilationUnit(RecordDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"));
    CHECK(testAst(
        ".record Person: Std::String name",
        "CompilationUnit(RecordDecl('Person',FieldDecl(NamedType('Std::String'),'name')))"));
    CHECK(testAst(
        ".record Point: Graphics::Int x, Graphics::Int y",
        "CompilationUnit("
            "RecordDecl("
                "'Point',"
                "FieldDecl(NamedType('Graphics::Int'),'x'),"
                "FieldDecl(NamedType('Graphics::Int'),'y')"
            ")"
        ")"));
    CHECK(testAst(
        ".record Container: Std::Collections::List[T] items",
        "CompilationUnit("
            "RecordDecl('Container',FieldDecl(NamedType('Std::Collections::List',NamedType('T')),'items'))"
        ")"));
    CHECK(testAst(
        ".record Entity: UI::Widget widget, Database::Id id",
        "CompilationUnit("
            "RecordDecl("
                "'Entity',"
                "FieldDecl(NamedType('UI::Widget'),'widget'),"
                "FieldDecl(NamedType('Database::Id'),'id')"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_RECORD_FIELD") {
    CHECK(testAst(
        ".record R: Int x",
        "CompilationUnit(RecordDecl('R',FieldDecl(NamedType('Int'),'x')))"));
    CHECK(testAst(
        ".record R: String name",
        "CompilationUnit(RecordDecl('R',FieldDecl(NamedType('String'),'name')))"));
    CHECK(testAst(
        ".record R: Float value",
        "CompilationUnit(RecordDecl('R',FieldDecl(NamedType('Float'),'value')))"));
    CHECK(testAst(
        ".record R: List[Int] items",
        "CompilationUnit(RecordDecl('R',FieldDecl(NamedType('List',NamedType('Int')),'items')))"));
    CHECK(testAst(
        ".record R: Map[String, Int] data",
        "CompilationUnit("
            "RecordDecl('R',FieldDecl(NamedType('Map',NamedType('String'),NamedType('Int')),'data'))"
        ")"));
    CHECK(testAst(
        ".record R: Pair[T, U] pair",
        "CompilationUnit(RecordDecl('R',FieldDecl(NamedType('Pair',NamedType('T'),NamedType('U')),'pair')))"));
    CHECK(testAst(
        ".record R: [3]Float vector",
        "CompilationUnit(RecordDecl('R',FieldDecl(RangeType('3',NamedType('Float')),'vector')))"));
    CHECK(testAst(
        ".record R: [10]Int buffer",
        "CompilationUnit(RecordDecl('R',FieldDecl(RangeType('10',NamedType('Int')),'buffer')))"));
    CHECK(testAst(
        ".record R: [4][4]Float matrix",
        "CompilationUnit("
            "RecordDecl('R',FieldDecl(RangeType('4',RangeType('4',NamedType('Float'))),'matrix'))"
        ")"));
    CHECK(testAst(
        ".record R: [size]Byte data",
        "CompilationUnit(RecordDecl('R',FieldDecl(RangeType('size',NamedType('Byte')),'data')))"));
    CHECK(testAst(
        ".record R: [width][height]Int grid",
        "CompilationUnit(RecordDecl('R',FieldDecl(RangeType('width',RangeType('height',NamedType('Int'))),'grid')))"));
    CHECK(testAst(
        ".record R: [10]List[String] items",
        "CompilationUnit("
            "RecordDecl('R',FieldDecl(RangeType('10',NamedType('List',NamedType('String'))),'items'))"
        ")"));
    CHECK(testAst(
        ".record R: [rows][cols]Matrix[Float] data",
        "CompilationUnit("
            "RecordDecl("
                "'R',"
                "FieldDecl(RangeType('rows',RangeType('cols',NamedType('Matrix',NamedType('Float')))),'data')"
            ")"
        ")"));
    CHECK(testAst(
        ".record R: .record Int x, Int y pt",
        "CompilationUnit("
            "RecordDecl("
                "'R',"
                "FieldDecl("
                    "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                    "'pt'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".record R: .union Int i, Float f value",
        "CompilationUnit("
            "RecordDecl("
                "'R',"
                "FieldDecl("
                    "InlineUnionType("
                        "UnionCandidate(NamedType('Int'),'i'),"
                        "UnionCandidate(NamedType('Float'),'f')"
                    "),"
                    "'value'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".record R: .record scope: String a, Int b data",
        "CompilationUnit("
            "RecordDecl("
                "'R',"
                "FieldDecl("
                    "InlineRecordType("
                        "scope='scope',"
                        "FieldDecl(NamedType('String'),'a'),"
                        "FieldDecl(NamedType('Int'),'b')"
                    "),"
                    "'data'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_OBJECT_FIELD") {
    CHECK(testAst(
        ".object O: Int x",
        "CompilationUnit(ObjectDecl('O',FieldDecl(NamedType('Int'),'x')))"));
    CHECK(testAst(
        ".object O: List[Int] items",
        "CompilationUnit(ObjectDecl('O',FieldDecl(NamedType('List'),'items')))"));
    CHECK(testAst(
        ".object O: Map[String, Int] data",
        "CompilationUnit(ObjectDecl('O',FieldDecl(NamedType('Map'),'data')))"));
    CHECK(testAst(
        ".object O: [10]Int buffer",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('10',NamedType('Int')),'buffer')))"));
    CHECK(testAst(
        ".object O: [4][4]Float matrix",
        "CompilationUnit("
            "ObjectDecl('O',FieldDecl(RangeType('4',RangeType('4',NamedType('Float'))),'matrix'))"
        ")"));
    CHECK(testAst(
        ".object O: [size]Byte data",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('size',NamedType('Byte')),'data')))"));
    CHECK(testAst(
        ".object O: [width][height]Int grid",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('width',RangeType('height',NamedType('Int'))),'grid')))"));
    CHECK(testAst(
        ".object O: []String strings",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('',NamedType('String')),'strings')))"));
    CHECK(testAst(
        ".object O: ^String strPtr",
        "CompilationUnit(ObjectDecl('O',FieldDecl(PtrType(1,NamedType('String')),'strPtr')))"));
    CHECK(testAst(
        ".object O: ^List[T] listPtr",
        "CompilationUnit(ObjectDecl('O',FieldDecl(PtrType(1,NamedType('List')),'listPtr')))"));
    CHECK(testAst(
        ".object O: ^^Int ptrPtr",
        "CompilationUnit(ObjectDecl('O',FieldDecl(PtrType(2,NamedType('Int')),'ptrPtr')))"));
    CHECK(testAst(
        ".object O: ^[]Int arrayPtr",
        "CompilationUnit(ObjectDecl('O',FieldDecl(PtrType(1,RangeType('',NamedType('Int'))),'arrayPtr')))"));
    CHECK(testAst(
        ".object O: []^Int ptrArray",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('',PtrType(1,NamedType('Int'))),'ptrArray')))"));
    CHECK(testAst(
        ".object O: :<Int> cmd",
        "CompilationUnit(ObjectDecl('O',FieldDecl(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'cmd')))"));
    CHECK(testAst(
        ".object O: ?<String> mayFailCmd",
        "CompilationUnit("
            "ObjectDecl('O',FieldDecl(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'mayFailCmd'))"
        ")"));
    CHECK(testAst(
        ".object O: !<Float> failCmd",
        "CompilationUnit("
            "ObjectDecl('O',FieldDecl(CmdType(Fails,CmdTypeArg(false,NamedType('Float'))),'failCmd'))"
        ")"));
    CHECK(testAst(
        ".object O: :<> voidCmd",
        "CompilationUnit(ObjectDecl('O',FieldDecl(CmdType(NoFail),'voidCmd')))"));
    CHECK(testAst(
        ".object O: ?<> mayFailVoidCmd",
        "CompilationUnit(ObjectDecl('O',FieldDecl(CmdType(MayFail),'mayFailVoidCmd')))"));
    CHECK(testAst(
        ".object O: !<> failVoidCmd",
        "CompilationUnit(ObjectDecl('O',FieldDecl(CmdType(Fails),'failVoidCmd')))"));
    CHECK(testAst(
        ".object O: :<Int, String> multiCmd",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl("
                    "CmdType(NoFail,CmdTypeArg(false,NamedType('Int')),CmdTypeArg(false,NamedType('String'))),"
                    "'multiCmd'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: ^:<Int> cmdPtr",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl(PtrType(1,CmdType(NoFail,CmdTypeArg(false,NamedType('Int')))),'cmdPtr')"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: []:<String> cmdArray",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl(RangeType('',CmdType(NoFail,CmdTypeArg(false,NamedType('String')))),'cmdArray')"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: [5]^Int ptrArray",
        "CompilationUnit(ObjectDecl('O',FieldDecl(RangeType('5',PtrType(1,NamedType('Int'))),'ptrArray')))"));
    CHECK(testAst(
        ".object O: .record Int x, Int y pt",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl("
                    "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                    "'pt'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: .object ^Node next, Int val inner",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl("
                    "InlineObjectType("
                        "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                        "FieldDecl(NamedType('Int'),'val')"
                    "),"
                    "'inner'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: .union Int i, Float f value",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl("
                    "InlineUnionType("
                        "UnionCandidate(NamedType('Int'),'i'),"
                        "UnionCandidate(NamedType('Float'),'f')"
                    "),"
                    "'value'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".object O: .variant Int circle, ^Node n shape",
        "CompilationUnit("
            "ObjectDecl("
                "'O',"
                "FieldDecl("
                    "InlineVariantType("
                        "VariantCandidate(NamedType('Int'),'circle'),"
                        "VariantCandidate(PtrType(1,NamedType('Node')),'n')"
                    "),"
                    "'shape'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_OBJECT") {
    CHECK(testAst(
        ".object Point: Int x, Int y",
        "CompilationUnit(ObjectDecl('Point',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"));
    CHECK(testAst(
        ".object Node: T value, ^Node next",
        "CompilationUnit("
            "ObjectDecl("
                "'Node',"
                "FieldDecl(NamedType('T'),'value'),"
                "FieldDecl(PtrType(1,NamedType('Node')),'next')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Wrapper: Int value",
        "CompilationUnit(ObjectDecl('Wrapper',FieldDecl(NamedType('Int'),'value')))"));
    CHECK(testAst(
        ".object List[T]: T head, ^List[T] tail",
        "CompilationUnit("
            "ObjectDecl("
                "'List',"
                "FieldDecl(NamedType('T'),'head'),"
                "FieldDecl(PtrType(1,NamedType('List')),'tail')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Map[K, V]: K key, V value, ^Map[K, V] next",
        "CompilationUnit("
            "ObjectDecl("
                "'Map',"
                "FieldDecl(NamedType('K'),'key'),"
                "FieldDecl(NamedType('V'),'value'),"
                "FieldDecl(PtrType(1,NamedType('Map')),'next')"
            ")"
        ")"));
    CHECK(testAst(
        ".object LinkedList: Int data, ^LinkedList next, ^LinkedList prev",
        "CompilationUnit("
            "ObjectDecl("
                "'LinkedList',"
                "FieldDecl(NamedType('Int'),'data'),"
                "FieldDecl(PtrType(1,NamedType('LinkedList')),'next'),"
                "FieldDecl(PtrType(1,NamedType('LinkedList')),'prev')"
            ")"
        ")"));
    CHECK(testAst(
        ".object RefCounter: ^Data ptr, Int count",
        "CompilationUnit("
            "ObjectDecl("
                "'RefCounter',"
                "FieldDecl(PtrType(1,NamedType('Data')),'ptr'),"
                "FieldDecl(NamedType('Int'),'count')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Vector3: [3]Float coords",
        "CompilationUnit(ObjectDecl('Vector3',FieldDecl(RangeType('3',NamedType('Float')),'coords')))"));
    CHECK(testAst(
        ".object Matrix: [4][4]Float data",
        "CompilationUnit("
            "ObjectDecl('Matrix',FieldDecl(RangeType('4',RangeType('4',NamedType('Float'))),'data'))"
        ")"));
    CHECK(testAst(
        ".object DynamicArray: []Int items, Int size, Int capacity",
        "CompilationUnit("
            "ObjectDecl("
                "'DynamicArray',"
                "FieldDecl(RangeType('',NamedType('Int')),'items'),"
                "FieldDecl(NamedType('Int'),'size'),"
                "FieldDecl(NamedType('Int'),'capacity')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Buffer: [size]Byte data, Int size",
        "CompilationUnit("
            "ObjectDecl("
                "'Buffer',"
                "FieldDecl(RangeType('size',NamedType('Byte')),'data'),"
                "FieldDecl(NamedType('Int'),'size')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Handler: :<> callback, String name",
        "CompilationUnit("
            "ObjectDecl("
                "'Handler',"
                "FieldDecl(CmdType(NoFail),'callback'),"
                "FieldDecl(NamedType('String'),'name')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Processor: ?<String> loader, :<Int> processor, !<> cleanup",
        "CompilationUnit("
            "ObjectDecl("
                "'Processor',"
                "FieldDecl(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'loader'),"
                "FieldDecl(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'processor'),"
                "FieldDecl(CmdType(Fails),'cleanup')"
            ")"
        ")"));
    CHECK(testAst(
        ".object EventListener: :<Event> handler, Int priority",
        "CompilationUnit("
            "ObjectDecl("
                "'EventListener',"
                "FieldDecl(CmdType(NoFail,CmdTypeArg(false,NamedType('Event'))),'handler'),"
                "FieldDecl(NamedType('Int'),'priority')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Complex: ^Int ptr, []String names, :<> callback, List[T] items",
        "CompilationUnit("
            "ObjectDecl("
                "'Complex',"
                "FieldDecl(PtrType(1,NamedType('Int')),'ptr'),"
                "FieldDecl(RangeType('',NamedType('String')),'names'),"
                "FieldDecl(CmdType(NoFail),'callback'),"
                "FieldDecl(NamedType('List'),'items')"
            ")"
        ")"));
    CHECK(testAst(
        ".object State: Map[String, Int] vars, []:<> actions, ^State next",
        "CompilationUnit("
            "ObjectDecl("
                "'State',"
                "FieldDecl(NamedType('Map'),'vars'),"
                "FieldDecl(RangeType('',CmdType(NoFail)),'actions'),"
                "FieldDecl(PtrType(1,NamedType('State')),'next')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Graph[T]: T value, []^Graph[T] neighbors, ?<T> validator",
        "CompilationUnit("
            "ObjectDecl("
                "'Graph',"
                "FieldDecl(NamedType('T'),'value'),"
                "FieldDecl(RangeType('',PtrType(1,NamedType('Graph'))),'neighbors'),"
                "FieldDecl(CmdType(MayFail,CmdTypeArg(false,NamedType('T'))),'validator')"
            ")"
        ")"));
    CHECK(testAst(
        ".object Cache[K, V]: Map[K, V] data, :<V> loader, Int size",
        "CompilationUnit("
            "ObjectDecl("
                "'Cache',"
                "FieldDecl(NamedType('Map'),'data'),"
                "FieldDecl(CmdType(NoFail,CmdTypeArg(false,NamedType('V'))),'loader'),"
                "FieldDecl(NamedType('Int'),'size')"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_UNION_CANDIDATE") {
    CHECK(testAst(
        ".union U: Int whole",
        "CompilationUnit(UnionDecl('U',UnionCandidate(NamedType('Int'),'whole')))"));
    CHECK(testAst(
        ".union U: Float fractional",
        "CompilationUnit(UnionDecl('U',UnionCandidate(NamedType('Float'),'fractional')))"));
    CHECK(testAst(
        ".union U: String text",
        "CompilationUnit(UnionDecl('U',UnionCandidate(NamedType('String'),'text')))"));
    CHECK(testAst(
        ".union U: UserId uid",
        "CompilationUnit(UnionDecl('U',UnionCandidate(NamedType('UserId'),'uid')))"));
    CHECK(testAst(
        ".union U: List[Int] items",
        "CompilationUnit(UnionDecl('U',UnionCandidate(NamedType('List',NamedType('Int')),'items')))"));
    CHECK(testAst(
        ".union U: Map[String, Int] data",
        "CompilationUnit("
            "UnionDecl('U',UnionCandidate(NamedType('Map',NamedType('String'),NamedType('Int')),'data'))"
        ")"));
    CHECK(testAst(
        ".union U: [3]Float vec",
        "CompilationUnit(UnionDecl('U',UnionCandidate(RangeType('3',NamedType('Float')),'vec')))"));
    CHECK(testAst(
        ".union U: [size]Byte buf",
        "CompilationUnit(UnionDecl('U',UnionCandidate(RangeType('size',NamedType('Byte')),'buf')))"));
    CHECK(testAst(
        ".union U: .record Int x, Int y pt",
        "CompilationUnit("
            "UnionDecl("
                "'U',"
                "UnionCandidate("
                    "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                    "'pt'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".union U: .union Int i, Float f value",
        "CompilationUnit("
            "UnionDecl("
                "'U',"
                "UnionCandidate("
                    "InlineUnionType("
                        "UnionCandidate(NamedType('Int'),'i'),"
                        "UnionCandidate(NamedType('Float'),'f')"
                    "),"
                    "'value'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_UNION") {
    CHECK(testAst(
        ".union Scalar: Int whole, Float fractional",
        "CompilationUnit("
            "UnionDecl("
                "'Scalar',"
                "UnionCandidate(NamedType('Int'),'whole'),"
                "UnionCandidate(NamedType('Float'),'fractional')"
            ")"
        ")"));
    CHECK(testAst(
        ".union NumericId: Int i, String s",
        "CompilationUnit("
            "UnionDecl("
                "'NumericId',"
                "UnionCandidate(NamedType('Int'),'i'),"
                "UnionCandidate(NamedType('String'),'s')"
            ")"
        ")"));
    CHECK(testAst(
        ".union Single: Int value",
        "CompilationUnit(UnionDecl('Single',UnionCandidate(NamedType('Int'),'value')))"));
    CHECK(testAst(
        ".union Either[T]: T left, T right",
        "CompilationUnit("
            "UnionDecl('Either',UnionCandidate(NamedType('T'),'left'),UnionCandidate(NamedType('T'),'right'))"
        ")"));
    CHECK(testAst(
        ".union Result[T, E]: T ok, E err",
        "CompilationUnit("
            "UnionDecl('Result',UnionCandidate(NamedType('T'),'ok'),UnionCandidate(NamedType('E'),'err'))"
        ")"));
    CHECK(testAst(
        ".union Token: Int integer, Float floating, String text",
        "CompilationUnit("
            "UnionDecl("
                "'Token',"
                "UnionCandidate(NamedType('Int'),'integer'),"
                "UnionCandidate(NamedType('Float'),'floating'),"
                "UnionCandidate(NamedType('String'),'text')"
            ")"
        ")"));
    CHECK(testAst(
        ".union Number: Int i, Float f, [3]Float vec",
        "CompilationUnit("
            "UnionDecl("
                "'Number',"
                "UnionCandidate(NamedType('Int'),'i'),"
                "UnionCandidate(NamedType('Float'),'f'),"
                "UnionCandidate(RangeType('3',NamedType('Float')),'vec')"
            ")"
        ")"));
    CHECK(testAst(
        ".union Id: Std::Int i, Std::String s",
        "CompilationUnit("
            "UnionDecl("
                "'Id',"
                "UnionCandidate(NamedType('Std::Int'),'i'),"
                "UnionCandidate(NamedType('Std::String'),'s')"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_VARIANT_CANDIDATE") {
    CHECK(testAst(
        ".variant V: Int circle",
        "CompilationUnit(VariantDecl('V',VariantCandidate(NamedType('Int'),'circle')))"));
    CHECK(testAst(
        ".variant V: String text",
        "CompilationUnit(VariantDecl('V',VariantCandidate(NamedType('String'),'text')))"));
    CHECK(testAst(
        ".variant V: Float value",
        "CompilationUnit(VariantDecl('V',VariantCandidate(NamedType('Float'),'value')))"));
    CHECK(testAst(
        ".variant V: ^Node inner",
        "CompilationUnit(VariantDecl('V',VariantCandidate(PtrType(1,NamedType('Node')),'inner')))"));
    CHECK(testAst(
        ".variant V: ^Int ptr",
        "CompilationUnit(VariantDecl('V',VariantCandidate(PtrType(1,NamedType('Int')),'ptr')))"));
    CHECK(testAst(
        ".variant V: []Int items",
        "CompilationUnit(VariantDecl('V',VariantCandidate(RangeType('',NamedType('Int')),'items')))"));
    CHECK(testAst(
        ".variant V: :<Int> callback",
        "CompilationUnit("
            "VariantDecl('V',VariantCandidate(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'callback'))"
        ")"));
    CHECK(testAst(
        ".variant V: ?<String> loader",
        "CompilationUnit("
            "VariantDecl("
                "'V',"
                "VariantCandidate(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'loader')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant V: !<> cleanup",
        "CompilationUnit(VariantDecl('V',VariantCandidate(CmdType(Fails),'cleanup')))"));
    CHECK(testAst(
        ".variant V: List[Int] items",
        "CompilationUnit(VariantDecl('V',VariantCandidate(NamedType('List'),'items')))"));
    CHECK(testAst(
        ".variant V: Map[String, Int] data",
        "CompilationUnit(VariantDecl('V',VariantCandidate(NamedType('Map'),'data')))"));
    CHECK(testAst(
        ".variant V: .record Int x, Int y pt",
        "CompilationUnit("
            "VariantDecl("
                "'V',"
                "VariantCandidate("
                    "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                    "'pt'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".variant V: .object ^Node next, Int val inner",
        "CompilationUnit("
            "VariantDecl("
                "'V',"
                "VariantCandidate("
                    "InlineObjectType("
                        "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                        "FieldDecl(NamedType('Int'),'val')"
                    "),"
                    "'inner'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".variant V: .union Int i, Float f value",
        "CompilationUnit("
            "VariantDecl("
                "'V',"
                "VariantCandidate("
                    "InlineUnionType("
                        "UnionCandidate(NamedType('Int'),'i'),"
                        "UnionCandidate(NamedType('Float'),'f')"
                    "),"
                    "'value'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".variant V: .variant Int circle, ^Node n shape",
        "CompilationUnit("
            "VariantDecl("
                "'V',"
                "VariantCandidate("
                    "InlineVariantType("
                        "VariantCandidate(NamedType('Int'),'circle'),"
                        "VariantCandidate(PtrType(1,NamedType('Node')),'n')"
                    "),"
                    "'shape'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_VARIANT") {
    CHECK(testAst(
        ".variant Scalar: Int whole, Float fractional",
        "CompilationUnit("
            "VariantDecl("
                "'Scalar',"
                "VariantCandidate(NamedType('Int'),'whole'),"
                "VariantCandidate(NamedType('Float'),'fractional')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Single: Int value",
        "CompilationUnit(VariantDecl('Single',VariantCandidate(NamedType('Int'),'value')))"));
    CHECK(testAst(
        ".variant Shape: Int circle, ^Node inner",
        "CompilationUnit("
            "VariantDecl("
                "'Shape',"
                "VariantCandidate(NamedType('Int'),'circle'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Tree[T]: T leaf, ^Tree[T] branch",
        "CompilationUnit("
            "VariantDecl("
                "'Tree',"
                "VariantCandidate(NamedType('T'),'leaf'),"
                "VariantCandidate(PtrType(1,NamedType('Tree')),'branch')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Expr: Int literal, ^Expr left, ^Expr right",
        "CompilationUnit("
            "VariantDecl("
                "'Expr',"
                "VariantCandidate(NamedType('Int'),'literal'),"
                "VariantCandidate(PtrType(1,NamedType('Expr')),'left'),"
                "VariantCandidate(PtrType(1,NamedType('Expr')),'right')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Action: :<> doIt, ?<String> mayFail",
        "CompilationUnit("
            "VariantDecl("
                "'Action',"
                "VariantCandidate(CmdType(NoFail),'doIt'),"
                "VariantCandidate(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'mayFail')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Handler: :<Int> callback, Int value",
        "CompilationUnit("
            "VariantDecl("
                "'Handler',"
                "VariantCandidate(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'callback'),"
                "VariantCandidate(NamedType('Int'),'value')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Either[T]: T left, T right",
        "CompilationUnit("
            "VariantDecl("
                "'Either',"
                "VariantCandidate(NamedType('T'),'left'),"
                "VariantCandidate(NamedType('T'),'right')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Result[T, E]: T ok, E err",
        "CompilationUnit("
            "VariantDecl("
                "'Result',"
                "VariantCandidate(NamedType('T'),'ok'),"
                "VariantCandidate(NamedType('E'),'err')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Option[T]: T some, Int none",
        "CompilationUnit("
            "VariantDecl("
                "'Option',"
                "VariantCandidate(NamedType('T'),'some'),"
                "VariantCandidate(NamedType('Int'),'none')"
            ")"
        ")"));
    CHECK(testAst(
        ".variant Node: Int leaf, ^Node inner, []Int data, :<> action",
        "CompilationUnit("
            "VariantDecl("
                "'Node',"
                "VariantCandidate(NamedType('Int'),'leaf'),"
                "VariantCandidate(PtrType(1,NamedType('Node')),'inner'),"
                "VariantCandidate(RangeType('',NamedType('Int')),'data'),"
                "VariantCandidate(CmdType(NoFail),'action')"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_ALIAS") {
    CHECK(testAst(
        ".alias MyInt: Int",
        "CompilationUnit(AliasDecl('MyInt',NamedType('Int')))"));
    CHECK(testAst(
        ".alias T: U",
        "CompilationUnit(AliasDecl('T',NamedType('U')))"));
    CHECK(testAst(
        ".alias List[T]: ^[]T",
        "CompilationUnit(AliasDecl('List',PtrType(1,RangeType('',NamedType('T')))))"));
    CHECK(testAst(
        ".alias Map[K, V]: ^[]Pair[K, V]",
        "CompilationUnit(AliasDecl('Map',PtrType(1,RangeType('',NamedType('Pair')))))"));
    CHECK(testAst(
        ".alias Dict[String key, Int value]: ^[]Entry",
        "CompilationUnit(AliasDecl('Dict',PtrType(1,RangeType('',NamedType('Entry')))))"));
    CHECK(testAst(
        ".alias IntPtr: ^Int",
        "CompilationUnit(AliasDecl('IntPtr',PtrType(1,NamedType('Int'))))"));
    CHECK(testAst(
        ".alias IntArray: []Int",
        "CompilationUnit(AliasDecl('IntArray',RangeType('',NamedType('Int'))))"));
    CHECK(testAst(
        ".alias Matrix: [4][4]Float",
        "CompilationUnit(AliasDecl('Matrix',RangeType('4',RangeType('4',NamedType('Float')))))"));
    CHECK(testAst(
        ".alias Callback: :<Int>",
        "CompilationUnit(AliasDecl('Callback',CmdType(NoFail,CmdTypeArg(false,NamedType('Int')))))"));
    CHECK(testAst(
        ".alias MayFail: ?<String>",
        "CompilationUnit(AliasDecl('MayFail',CmdType(MayFail,CmdTypeArg(false,NamedType('String')))))"));
    CHECK(testAst(
        ".alias Fail: !<Float>",
        "CompilationUnit(AliasDecl('Fail',CmdType(Fails,CmdTypeArg(false,NamedType('Float')))))"));
    CHECK(testAst(
        ".alias VoidCmd: :<>",
        "CompilationUnit(AliasDecl('VoidCmd',CmdType(NoFail)))"));
    CHECK(testAst(
        ".alias MultiCmd: :<Int, String>",
        "CompilationUnit("
            "AliasDecl("
                "'MultiCmd',"
                "CmdType(NoFail,CmdTypeArg(false,NamedType('Int')),CmdTypeArg(false,NamedType('String')))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias CmdPtr: ^:<Int>",
        "CompilationUnit(AliasDecl('CmdPtr',PtrType(1,CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))))))"));
    CHECK(testAst(
        ".alias CmdArray: []:<String>",
        "CompilationUnit("
            "AliasDecl('CmdArray',RangeType('',CmdType(NoFail,CmdTypeArg(false,NamedType('String')))))"
        ")"));
    CHECK(testAst(
        ".alias Complex: ^[]:<Int>",
        "CompilationUnit("
            "AliasDecl('Complex',PtrType(1,RangeType('',CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))))))"
        ")"));
    CHECK(testAst(
        ".alias Point: .record Int x, Int y",
        "CompilationUnit("
            "AliasDecl("
                "'Point',"
                "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias Node: .object ^Node next, Int val",
        "CompilationUnit("
            "AliasDecl("
                "'Node',"
                "InlineObjectType("
                    "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                    "FieldDecl(NamedType('Int'),'val')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias Num: .union Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'Num',"
                "InlineUnionType(UnionCandidate(NamedType('Int'),'i'),UnionCandidate(NamedType('Float'),'f'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias Shape: .variant Int circle, ^Node n ",
        "CompilationUnit("
            "AliasDecl("
                "'Shape',"
                "InlineVariantType("
                    "VariantCandidate(NamedType('Int'),'circle'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'n')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias ScopedPt: .record myScope: Int x, Int y",
        "CompilationUnit("
            "AliasDecl("
                "'ScopedPt',"
                "InlineRecordType("
                    "scope='myScope',"
                    "FieldDecl(NamedType('Int'),'x'),"
                    "FieldDecl(NamedType('Int'),'y')"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_INSTANCE") {
    CHECK(testAst(
        ".instance Widget: Interface",
        "CompilationUnit(InstanceDecl('Widget',InstanceType('Interface')))"));
    CHECK(testAst(
        ".instance MyType: Serializable",
        "CompilationUnit(InstanceDecl('MyType',InstanceType('Serializable')))"));
    CHECK(testAst(
        ".instance List[T]: Iterable",
        "CompilationUnit(InstanceDecl('List',InstanceType('Iterable')))"));
    CHECK(testAst(
        ".instance Map[K, V]: Container",
        "CompilationUnit(InstanceDecl('Map',InstanceType('Container')))"));
    CHECK(testAst(
        ".instance User: Serializable, Comparable",
        "CompilationUnit(InstanceDecl('User',InstanceType('Serializable'),InstanceType('Comparable')))"));
    CHECK(testAst(
        ".instance Node: Printable, Hashable, Cloneable",
        "CompilationUnit("
            "InstanceDecl("
                "'Node',"
                "InstanceType('Printable'),"
                "InstanceType('Hashable'),"
                "InstanceType('Cloneable')"
            ")"
        ")"));
    CHECK(testAst(
        ".instance List[T]: Iterable, Serializable",
        "CompilationUnit(InstanceDecl('List',InstanceType('Iterable'),InstanceType('Serializable')))"));
    CHECK(testAst(
        ".instance Widget: Interface (myImpl)",
        "CompilationUnit(InstanceDecl('Widget',InstanceType('Interface',delegate='myImpl')))"));
    CHECK(testAst(
        ".instance User: Serializable (mySerial), Comparable (myComp)",
        "CompilationUnit("
            "InstanceDecl("
                "'User',"
                "InstanceType('Serializable',delegate='mySerial'),"
                "InstanceType('Comparable',delegate='myComp')"
            ")"
        ")"));
    CHECK(testAst(
        ".instance Widget: Interface (Std::myImpl)",
        "CompilationUnit(InstanceDecl('Widget',InstanceType('Interface',delegate='Std::myImpl')))"));
    CHECK(testAst(
        ".instance Map[K, V]: Container (hashImpl), Iterable",
        "CompilationUnit("
            "InstanceDecl('Map',InstanceType('Container',delegate='hashImpl'),InstanceType('Iterable'))"
        ")"));
}

TEST_CASE("Ast::DEF_CMD_PARMTYPE_NAME") {
    CHECK(testAst(
        ".decl doIt",
        "CompilationUnit(CmdDecl(RegularSig('doIt')))"));
    CHECK(testAst(
        ".decl ?doIt",
        "CompilationUnit(CmdDecl(RegularSig('doIt',fail=MayFail)))"));
    CHECK(testAst(
        ".decl !doIt",
        "CompilationUnit(CmdDecl(RegularSig('doIt',fail=Fails)))"));
    CHECK(testAst(
        ".decl doIt: Int x",
        "CompilationUnit(CmdDecl(RegularSig('doIt',param=CmdParam(NamedType('Int'),'x'))))"));
    CHECK(testAst(
        ".decl doIt: Int 'x, String y",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'\\'x'),"
                    "param=CmdParam(NamedType('String'),'y')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: Int 'x -> result",
        "CompilationUnit(CmdDecl(RegularSig('doIt',param=CmdParam(NamedType('Int'),'\\'x'),ret='result')))"));
    CHECK(testAst(
        ".decl doIt: Int ctx / Int ctx",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'ctx'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: Int x / Int 'ctx",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'\\'ctx')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: Int x -> result / Int ctx",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: Int x, String y -> result / Int ctx, String s",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "param=CmdParam(NamedType('String'),'y'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "implicit=CmdParam(NamedType('String'),'s'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: doIt",
        "CompilationUnit(CmdDecl(VCommandSig('doIt',receiver=CmdReceiver(NamedType('Widget'),'w'))))"));
    CHECK(testAst(
        ".decl (Widget w):: ?doIt",
        "CompilationUnit("
            "CmdDecl(VCommandSig('doIt',fail=MayFail,receiver=CmdReceiver(NamedType('Widget'),'w')))"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: !doIt",
        "CompilationUnit("
            "CmdDecl(VCommandSig('doIt',fail=Fails,receiver=CmdReceiver(NamedType('Widget'),'w')))"
        ")"));
    CHECK(testAst(
        ".decl (Widget w, Button b):: doIt",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'doIt',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "receiver=CmdReceiver(NamedType('Button'),'b')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w, Button 'b):: doIt: Int x -> 'result / Int ctx",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'doIt',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "receiver=CmdReceiver(NamedType('Button'),'\\'b'),"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='\\'result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: doIt: Int x -> result",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'doIt',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: doIt: Int x -> result / Int ctx",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'doIt',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: method -> w",
        "CompilationUnit("
            "CmdDecl(VCommandSig('method',receiver=CmdReceiver(NamedType('Widget'),'w'),ret='w'))"
        ")"));
    CHECK(testAst(
        ".decl (Container c):: getSize -> result",
        "CompilationUnit("
            "CmdDecl(VCommandSig('getSize',receiver=CmdReceiver(NamedType('Container'),'c'),ret='result'))"
        ")"));
    CHECK(testAst(
        ".decl (Widget w, Button b):: combine -> w",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'combine',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "receiver=CmdReceiver(NamedType('Button'),'b'),"
                    "ret='w'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Handler h):: ?tryGet -> result",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'tryGet',"
                    "fail=MayFail,"
                    "receiver=CmdReceiver(NamedType('Handler'),'h'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Processor p):: !mustProcess -> output",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'mustProcess',"
                    "fail=Fails,"
                    "receiver=CmdReceiver(NamedType('Processor'),'p'),"
                    "ret='output'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (Widget w):: getValue -> result / Int ctx",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'getValue',"
                    "receiver=CmdReceiver(NamedType('Widget'),'w'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl (UI::Widget w):: render",
        "CompilationUnit(CmdDecl(VCommandSig('render',receiver=CmdReceiver(NamedType('UI::Widget'),'w'))))"));
    CHECK(testAst(
        ".decl (Std::Collections::List[T] items):: process",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig('process',receiver=CmdReceiver(NamedType('Std::Collections::List'),'items'))"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: Namespace::Type param -> result",
        "CompilationUnit("
            "CmdDecl(RegularSig('doIt',param=CmdParam(NamedType('Namespace::Type'),'param'),ret='result'))"
        ")"));
    CHECK(testAst(
        ".decl process: Module::Item x -> result",
        "CompilationUnit("
            "CmdDecl(RegularSig('process',param=CmdParam(NamedType('Module::Item'),'x'),ret='result'))"
        ")"));
    CHECK(testAst(
        ".decl (UI::Widget w):: handle: IO::Event e -> response / Std::Context ctx",
        "CompilationUnit("
            "CmdDecl("
                "VCommandSig("
                    "'handle',"
                    "receiver=CmdReceiver(NamedType('UI::Widget'),'w'),"
                    "param=CmdParam(NamedType('IO::Event'),'e'),"
                    "implicit=CmdParam(NamedType('Std::Context'),'ctx'),"
                    "ret='response'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl Widget w: Int i, Int j",
        "CompilationUnit("
            "CmdDecl("
                "ConstructorSig("
                    "CmdReceiver(NamedType('Widget'),'w'),"
                    "CmdParam(NamedType('Int'),'i'),"
                    "CmdParam(NamedType('Int'),'j')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: ?<Int'> cmd",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig('doIt',param=CmdParam(CmdType(MayFail,CmdTypeArg(true,NamedType('Int'))),'cmd'))"
            ")"
        ")"));
    CHECK(testAst(
        ".decl doIt: !<String', Int> result",
        "CompilationUnit("
            "CmdDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam("
                        "CmdType("
                            "Fails,"
                            "CmdTypeArg(true,NamedType('String')),"
                            "CmdTypeArg(false,NamedType('Int'))"
                        "),"
                        "'result'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".decl @ Widget w",
        "CompilationUnit(CmdDecl(DestructorSig(CmdReceiver(NamedType('Widget'),'w'))))"));
    CHECK(testAst(
        ".decl @! Widget w",
        "CompilationUnit(CmdDecl(FailHandlerSig(CmdReceiver(NamedType('Widget'),'w'))))"));
    CHECK(testAst(
        ".intrinsic doIt: Int x -> result",
        "CompilationUnit(IntrinsicDecl(RegularSig('doIt',param=CmdParam(NamedType('Int'),'x'),ret='result')))"));
    CHECK(testAst(
        ".intrinsic ?doIt: Int x -> result",
        "CompilationUnit("
            "IntrinsicDecl(RegularSig('doIt',fail=MayFail,param=CmdParam(NamedType('Int'),'x'),ret='result'))"
        ")"));
    CHECK(testAst(
        ".intrinsic !doIt: Int x -> result",
        "CompilationUnit("
            "IntrinsicDecl(RegularSig('doIt',fail=Fails,param=CmdParam(NamedType('Int'),'x'),ret='result'))"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: Int x -> result / Int ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic ?doIt: Int x -> result / Int ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "fail=MayFail,"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic !doIt: Int x -> result / Int ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "fail=Fails,"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: Int 'x, String y -> result / Int 'ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'\\'x'),"
                    "param=CmdParam(NamedType('String'),'y'),"
                    "implicit=CmdParam(NamedType('Int'),'\\'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: ^Int ptr -> 'result / String ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(PtrType(1,NamedType('Int')),'ptr'),"
                    "implicit=CmdParam(NamedType('String'),'ctx'),"
                    "ret='\\'result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: []List[T] items -> result / Int ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(RangeType('',NamedType('List')),'items'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: ?<Int'> cmd -> result / String ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(CmdType(MayFail,CmdTypeArg(true,NamedType('Int'))),'cmd'),"
                    "implicit=CmdParam(NamedType('String'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: !<String', Int> result -> out / Int ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam("
                        "CmdType("
                            "Fails,"
                            "CmdTypeArg(true,NamedType('String')),"
                            "CmdTypeArg(false,NamedType('Int'))"
                        "),"
                        "'result'"
                    "),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "ret='out'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: (T:Int) i, T j -> result / String ctx",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'i',typeVar='T'),"
                    "param=CmdParam(NamedType('T'),'j'),"
                    "implicit=CmdParam(NamedType('String'),'ctx'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: Int x, String y -> result / Int ctx, String s",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "param=CmdParam(NamedType('String'),'y'),"
                    "implicit=CmdParam(NamedType('Int'),'ctx'),"
                    "implicit=CmdParam(NamedType('String'),'s'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".intrinsic doIt: Int x -> result",
        "CompilationUnit(IntrinsicDecl(RegularSig('doIt',param=CmdParam(NamedType('Int'),'x'),ret='result')))"));
    CHECK(testAst(
        ".intrinsic doIt: Int x, String y -> result",
        "CompilationUnit("
            "IntrinsicDecl("
                "RegularSig("
                    "'doIt',"
                    "param=CmdParam(NamedType('Int'),'x'),"
                    "param=CmdParam(NamedType('String'),'y'),"
                    "ret='result'"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CMD - full commands with bodies") {
    CHECK(testAst(
        ".cmd Container c: Int size = Container: size",
        "CompilationUnit("
            "CmdDef("
                "ConstructorSig(CmdReceiver(NamedType('Container'),'c'),CmdParam(NamedType('Int'),'size')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallConstructorExpr("
                                "NamedType('Container'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd @! Widget w = logError: error",
        "CompilationUnit("
            "CmdDef("
                "FailHandlerSig(CmdReceiver(NamedType('Widget'),'w')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('logError'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd @! Connection c = handleFailure: #msg, code",
        "CompilationUnit("
            "CmdDef("
                "FailHandlerSig(CmdReceiver(NamedType('Connection'),'c')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('handleFailure'),"
                                "CallParam(IdentifierExpr('msg',alloc)),"
                                "CallParam(CallCommandExpr(IdentifierExpr('code')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd @ Resource r = doIt",
        "CompilationUnit("
            "CmdDef("
                "DestructorSig(CmdReceiver(NamedType('Resource'),'r')),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd @! Widget w = doIt",
        "CompilationUnit("
            "CmdDef("
                "FailHandlerSig(CmdReceiver(NamedType('Widget'),'w')),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd @! Transaction t = doIt",
        "CompilationUnit("
            "CmdDef("
                "FailHandlerSig(CmdReceiver(NamedType('Transaction'),'t')),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_CONSTRUCTOR") {
    CHECK(testAst(
        ".test \"x\" = Widget: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: size",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Point: x, y, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Point'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = List[Int]: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('List',NamedType('Int')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Map[String, Int]: key, value",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Map',NamedType('String'),NamedType('Int')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('key'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: #x, #y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x',alloc)),"
                            "CallParam(IdentifierExpr('y',alloc))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: (Point: x, y), z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Point'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = UI::Widget: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('UI::Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Std::Collections::List[T]: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Std::Collections::List',NamedType('T')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Graphics::Point: x, y, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Graphics::Point'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = A::B::C::Type: value",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('A::B::C::Type'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = UI::Widget: (Graphics::Point: x, y), z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('UI::Widget'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Graphics::Point'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_COMMAND") {
    CHECK(testAst(
        ".test \"x\" = doSomething: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('doSomething'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = calculate: a, #b, c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('calculate'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('a'))),"
                            "CallParam(IdentifierExpr('b',alloc)),"
                            "CallParam(CallCommandExpr(IdentifierExpr('c')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = doIt",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))))"));
    CHECK(testAst(
        ".test \"x\" = process: #temp, value",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(IdentifierExpr('temp',alloc)),"
                            "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: (Widget: x, y), z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {doIt}: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {(obj):: method}: data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr(Subquote,CallVCommandExpr('method',receiver='obj')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Module::function: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('Module::function'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Std::IO::println: message",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('Std::IO::println'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('message')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = A::B::C::method",
        "CompilationUnit("
            "TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('A::B::C::method')))))"
        ")"));
}

TEST_CASE("Ast::CALL_VCOMMAND") {
    CHECK(testAst(
        ".test \"x\" = (obj):: method: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (widget):: process: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'process',"
                            "receiver='widget',"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallVCommandExpr('method',receiver='obj')))))"));
    CHECK(testAst(
        ".test \"x\" = (a, b):: handle: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (x, y, z):: process: data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'process',"
                            "receiver='x',"
                            "receiver='y',"
                            "receiver='z',"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: (Widget: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - dereference operator") {
    CHECK(testAst(
        ".test \"x\" = a <- ptr^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- myPtr^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('myPtr')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- 42^",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('42'),Deref))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- 3.14^",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('3.14'),Deref))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- \"hello\"^",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('hello'),Deref))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (x + y)^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y'))),"
                            "Deref"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((doIt))^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('doIt')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (Widget: x, y)^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "Deref"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^ + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + ptr^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr1^ * ptr2^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr1')),Deref),"
                            "'*',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr2')),Deref)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - address-of operator") {
    CHECK(testAst(
        ".test \"x\" = a <- x&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Addr))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- myVar&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('myVar')),Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Addr))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- 42&",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('42'),Addr))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- 3.14&",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('3.14'),Addr))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- \"hello\"&",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('hello'),Addr))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (x + y)&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((doIt))&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('doIt')),Addr))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (Widget: x, y)&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x& + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Addr),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + b&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('b')),Addr)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x& * y&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Addr),"
                            "'*',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('y')),Addr)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - array indexing") {
    CHECK(testAst(
        ".test \"x\" = a <- data[i]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index(CallCommandExpr(IdentifierExpr('i'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- items[index]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('items')),"
                            "Index(CallCommandExpr(IdentifierExpr('index')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- myArray[42]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('myArray')),Index(LiteralExpr('42')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- matrix[i, j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('matrix')),"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- grid[0, 0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('grid')),"
                            "Index(LiteralExpr('0'),LiteralExpr('0'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- table[row, col]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('table')),"
                            "Index(IdentifierExpr('row'),CallCommandExpr(IdentifierExpr('col')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[0][1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data[i][j]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index(CallCommandExpr(IdentifierExpr('i'))),"
                            "Index(CallCommandExpr(IdentifierExpr('j'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- cube[x][y][z]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('cube')),"
                            "Index(CallCommandExpr(IdentifierExpr('x'))),"
                            "Index(CallCommandExpr(IdentifierExpr('y'))),"
                            "Index(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- nested[0][1][2][3]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('nested')),"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1')),"
                            "Index(LiteralExpr('2')),"
                            "Index(LiteralExpr('3')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[i, j][k]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j'))),"
                            "Index(CallCommandExpr(IdentifierExpr('k')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data[0][i, j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index(LiteralExpr('0')),"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- complex[a, b][c][d, e]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('complex')),"
                            "Index(IdentifierExpr('a'),CallCommandExpr(IdentifierExpr('b'))),"
                            "Index(CallCommandExpr(IdentifierExpr('c'))),"
                            "Index(IdentifierExpr('d'),CallCommandExpr(IdentifierExpr('e')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[i + 1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(BinaryExpr(IdentifierExpr('i'),'+',LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data[x * 2]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index(BinaryExpr(IdentifierExpr('x'),'*',LiteralExpr('2')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- items[a + b, c - d]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('items')),"
                            "Index("
                                "BinaryExpr(IdentifierExpr('a'),'+',IdentifierExpr('b')),"
                                "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- 42[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(LiteralExpr('42'),Index(LiteralExpr('0'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- \"text\"[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(LiteralExpr('text'),Index(CallCommandExpr(IdentifierExpr('i'))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (x + y)[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y'))),"
                            "Index(LiteralExpr('0'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (doIt)[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "Index(CallCommandExpr(IdentifierExpr('i')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (Widget: x, y)[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "Index(LiteralExpr('0'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a + b) * c)[index]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('c'))"
                            "),"
                            "Index(CallCommandExpr(IdentifierExpr('index')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[0] + arr[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0'))),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data[i] * data[j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('j')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x + arr[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- matrix[i, j] - offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('matrix')),"
                                "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                            "),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - combined operations") {
    CHECK(testAst(
        ".test \"x\" = a <- arr[i][j][k] + data[x][y]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i'))),"
                                "Index(CallCommandExpr(IdentifierExpr('j'))),"
                                "Index(CallCommandExpr(IdentifierExpr('k')))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('x'))),"
                                "Index(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- matrix[0, 0] * matrix[1, 1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('matrix')),"
                                "Index(LiteralExpr('0'),LiteralExpr('0'))"
                            "),"
                            "'*',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('matrix')),"
                                "Index(LiteralExpr('1'),LiteralExpr('1'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[i + j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('arr')),"
                            "Index(BinaryExpr(IdentifierExpr('i'),'+',CallCommandExpr(IdentifierExpr('j'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data[x * 2, y / 2]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index("
                                "BinaryExpr(IdentifierExpr('x'),'*',LiteralExpr('2')),"
                                "BinaryExpr(IdentifierExpr('y'),'/',LiteralExpr('2'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- items[(a + b)]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('items')),"
                            "Index(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[0]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0')),Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref,Index(LiteralExpr('0')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^[i][j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('ptr')),"
                            "Deref,"
                            "Index(CallCommandExpr(IdentifierExpr('i'))),"
                            "Index(CallCommandExpr(IdentifierExpr('j')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^[0]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('ptr')),"
                            "Deref,"
                            "Index(LiteralExpr('0')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^[i, j]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('ptr')),"
                            "Deref,"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- data^[0][1][2]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Deref,"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1')),"
                            "Index(LiteralExpr('2')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^ + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr^[i] * scale",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr')),"
                                "Deref,"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('scale'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ptr1^[0] + ptr2^[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr1')),"
                                "Deref,"
                                "Index(LiteralExpr('0'))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr2')),"
                                "Deref,"
                                "Index(LiteralExpr('1'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr^&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Deref,Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr[0]^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- arr^^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Deref,Deref)"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - ordering rules") {
    CHECK(testAst(
        ".test \"x\" = a <- x^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Deref))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Index(LiteralExpr('0')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('a'),SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Addr))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Deref,Index(LiteralExpr('0')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x[0]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Index(LiteralExpr('0')),Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^[0]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Deref,Index(LiteralExpr('0')),Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^[0][1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "Deref,"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^[0][1]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "Deref,"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^[i, j][k]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "Deref,"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j'))),"
                            "Index(CallCommandExpr(IdentifierExpr('k'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x[0]^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Index(LiteralExpr('0')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x[0]^&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Index(LiteralExpr('0')),Deref,Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x^^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('x')),Deref,Deref)"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPR_SUFFIX - in assignments") {
    CHECK(testAst(
        ".test \"x\" = addr <- (data)[2]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('addr'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Index(LiteralExpr('2')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = addr <- data&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('addr'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Addr)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ref <- (x + y)&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('ref'),"
                        "SuffixExpr("
                            "BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- arr[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = element <- data[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('element'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('data')),"
                            "Index(CallCommandExpr(IdentifierExpr('i')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = cell <- matrix[i, j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('cell'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('matrix')),"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = item <- nested[0][1][2]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('item'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('nested')),"
                            "Index(LiteralExpr('0')),"
                            "Index(LiteralExpr('1')),"
                            "Index(LiteralExpr('2'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- arr[i] + arr[j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('j')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = sum <- data[0] * data[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('sum'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Index(LiteralExpr('0'))),"
                            "'*',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr <- (x + y)&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('ptr'),"
                        "SuffixExpr("
                            "BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y'))),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = val <- ptr^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('val'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = item <- ptr^[0]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('item'),"
                        "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref,Index(LiteralExpr('0')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = element <- ptr^[i, j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('element'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('ptr')),"
                            "Deref,"
                            "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = addr <- ptr^[0]&",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('addr'),"
                        "SuffixExpr("
                            "CallCommandExpr(IdentifierExpr('ptr')),"
                            "Deref,"
                            "Index(LiteralExpr('0')),"
                            "Addr"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- ptr1^ + ptr2^",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr1')),Deref),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr2')),Deref)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = sum <- ptr^[0] * ptr^[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('sum'),"
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref,Index(LiteralExpr('0'))),"
                            "'*',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref,Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION") {
    CHECK(testAst(
        ".test \"x\" = a <- ((Widget: x, y))",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (List[Int]: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallConstructorExpr("
                            "NamedType('List',NamedType('Int')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (doIt)",
        "CompilationUnit("
            "TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),CallCommandExpr(IdentifierExpr('doIt')))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- someVariable",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup(AssignStat(IdentifierExpr('a'),CallCommandExpr(IdentifierExpr('someVariable'))))"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((obj):: method: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a, b):: handle: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (process: (Widget: x, y))",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ({doIt}: data)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ({Widget: x, y})",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_CMD_LITERAL") {
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x>{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<String name>{Container: name}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Container'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<Int value>{List[Int]: value}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('Int'),'value'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('List',NamedType('Int')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<String data>{Map[String, Int]: data, count}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'data'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Map',NamedType('String'),NamedType('Int')),"
                                        "CallParam(IdentifierExpr('data')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('count')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<Int count>{Array[8]: count}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('Int'),'count'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Array',NamedType('8')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('count')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x>{process: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<String name>{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<Int value>{calculate: value, value}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('Int'),'value'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(IdentifierExpr('value')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<String data>{validate: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('String'),'data'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('validate'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x>{(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<String name>{(a, b):: handle: name}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'handle',"
                                        "receiver='a',"
                                        "receiver='b',"
                                        "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<Int value>{(widget):: process}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('Int'),'value'),"
                            "CallGroup(ExprStat(CallVCommandExpr('process',receiver='widget')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x, String y>{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "param=CmdParam(NamedType('String'),'y'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int a, Int b, Int c>{calculate: a, b, c}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'a'),"
                            "param=CmdParam(NamedType('Int'),'b'),"
                            "param=CmdParam(NamedType('Int'),'c'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(IdentifierExpr('a')),"
                                        "CallParam(IdentifierExpr('b')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('c')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<String name, Int count>{process: name, count}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "param=CmdParam(NamedType('Int'),'count'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(IdentifierExpr('name')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('count')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<Int x, String y, Int z>{(obj):: method: x, y, z}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "param=CmdParam(NamedType('String'),'y'),"
                            "param=CmdParam(NamedType('Int'),'z'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(IdentifierExpr('y')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<^Int ptr>{process: ptr}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(PtrType(1,NamedType('Int')),'ptr'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('ptr')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<[]Int items>{calculate: items}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(RangeType('',NamedType('Int')),'items'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('items')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<[8]Int buffer>{validate: buffer}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(RangeType('8',NamedType('Int')),'buffer'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('validate'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('buffer')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<List[T] list>{Widget: list}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('List'),'list'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('list')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<^[]Int data>{process: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(PtrType(1,RangeType('',NamedType('Int'))),'data'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<Map[String, Int] map>{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('Map'),'map'),"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<(T: Int) value>{process: value}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'value',typeVar='T'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<(T: String) data>{Widget: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'data',typeVar='T'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<(U: List[T]) items>{calculate: items}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('List'),'items',typeVar='U'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('items')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x, ^String ptr, []Int items>{process: x, ptr, items}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "param=CmdParam(PtrType(1,NamedType('String')),'ptr'),"
                            "param=CmdParam(RangeType('',NamedType('Int')),'items'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(IdentifierExpr('ptr')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('items')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<String name, (T: Int) value>{Widget: name, value}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "param=CmdParam(NamedType('Int'),'value',typeVar='T'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('name')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<List[T] list, Int count, ^Int ptr>{(obj):: method: list, count, ptr}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('List'),'list'),"
                            "param=CmdParam(NamedType('Int'),'count'),"
                            "param=CmdParam(PtrType(1,NamedType('Int')),'ptr'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('list')),"
                                        "CallParam(IdentifierExpr('count')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('ptr')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- :<Int x>{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handler <- ?<String name>{process: name}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('handler'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #temp <- !<Int a, Int b>{calculate: a, b}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('temp',alloc),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "param=CmdParam(NamedType('Int'),'a'),"
                            "param=CmdParam(NamedType('Int'),'b'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(IdentifierExpr('a')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = callback <- :<^Int ptr, String name>{(obj):: method: ptr, name}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('callback'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(PtrType(1,NamedType('Int')),'ptr'),"
                            "param=CmdParam(NamedType('String'),'name'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('ptr')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- ?<List[T] items>{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "param=CmdParam(NamedType('List'),'items'),"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: :<Int x>{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "NoFail,"
                                    "param=CmdParam(NamedType('Int'),'x'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = execute: ?<String name>{doIt}, context",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('execute'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MayFail,"
                                    "param=CmdParam(NamedType('String'),'name'),"
                                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('context')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handle: !<Int a, Int b>{calculate: a, b}, :<String s>{validate: s}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('handle'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MustFail,"
                                    "param=CmdParam(NamedType('Int'),'a'),"
                                    "param=CmdParam(NamedType('Int'),'b'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('calculate'),"
                                                "CallParam(IdentifierExpr('a')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "NoFail,"
                                    "param=CmdParam(NamedType('String'),'s'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('validate'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('s')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: ?<Int x>{process: x}, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MayFail,"
                                    "param=CmdParam(NamedType('Int'),'x'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: :<Int a, String b>{Widget: a, b}, !<Int c>{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "NoFail,"
                                    "param=CmdParam(NamedType('Int'),'a'),"
                                    "param=CmdParam(NamedType('String'),'b'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('a')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MustFail,"
                                    "param=CmdParam(NamedType('Int'),'c'),"
                                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: !<String name>{process: name}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MustFail,"
                                    "param=CmdParam(NamedType('String'),'name'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('name')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a, b):: handle: :<Int x, Int y>{Widget: x, y}, ?<String s>{validate: s}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "NoFail,"
                                    "param=CmdParam(NamedType('Int'),'x'),"
                                    "param=CmdParam(NamedType('Int'),'y'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MayFail,"
                                    "param=CmdParam(NamedType('String'),'s'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('validate'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('s')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: (:<Int x>{Widget: x, y})",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "NoFail,"
                                    "param=CmdParam(NamedType('Int'),'x'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: (?<String name>{doIt}), (!<Int value>{process: value})",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MayFail,"
                                    "param=CmdParam(NamedType('String'),'name'),"
                                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                                ")"
                            "),"
                            "CallParam("
                                "CmdLiteralExpr("
                                    "MustFail,"
                                    "param=CmdParam(NamedType('Int'),'value'),"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (:<Int a, Int b>{calculate: a, b})",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'a'),"
                            "param=CmdParam(NamedType('Int'),'b'),"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(IdentifierExpr('a')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<>{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?<>{process: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MayFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !<>{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "MustFail,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- :<>{(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :<Int x>{%process: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CmdLiteralExpr("
                            "NoFail,"
                            "param=CmdParam(NamedType('Int'),'x'),"
                            "CallGroup("
                                "Block("
                                    "DoBlock,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_ASSIGNMENT") {
    CHECK(testAst(
        ".test \"x\" = result <- Widget: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (Widget: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = container <- (List[Int]: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('container'),"
                        "CallConstructorExpr("
                            "NamedType('List',NamedType('Int')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- fibx",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup(AssignStat(IdentifierExpr('value'),CallCommandExpr(IdentifierExpr('fibx'))))"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data <- ((obj):: method: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('data'),"
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- ((a, b):: handle: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = widget <- UI::Widget: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('widget'),"
                        "CallConstructorExpr("
                            "NamedType('UI::Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = list <- Std::Collections::List[T]: item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('list'),"
                        "CallConstructorExpr("
                            "NamedType('Std::Collections::List',NamedType('T')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- Module::function: param",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallCommandExpr("
                            "IdentifierExpr('Module::function'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('param')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- A::B::C::process: data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "CallCommandExpr("
                            "IdentifierExpr('A::B::C::process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (processor: data)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallCommandExpr("
                            "IdentifierExpr('processor'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- ({doIt}: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "CallCommandExpr("
                            "QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- ({Widget: x, y})",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (Widget: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #output <- (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output',alloc),"
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #data <- ((obj):: method: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('data',alloc),"
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #result <- (Widget: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result',alloc),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_INVOKE") {
    CHECK(testAst(
        ".program Widget: x, y",
        "CompilationUnit("
            "ProgramDecl("
                "CallConstructorExpr("
                    "NamedType('Widget'),"
                    "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program List[Int]: item",
        "CompilationUnit("
            "ProgramDecl("
                "CallConstructorExpr("
                    "NamedType('List',NamedType('Int')),"
                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program doSomething: x, y",
        "CompilationUnit("
            "ProgramDecl("
                "CallCommandExpr("
                    "IdentifierExpr('doSomething'),"
                    "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program process",
        "CompilationUnit(ProgramDecl(CallCommandExpr(IdentifierExpr('process'))))"));
    CHECK(testAst(
        ".program (obj):: method: x, y",
        "CompilationUnit("
            "ProgramDecl("
                "CallVCommandExpr("
                    "'method',"
                    "receiver='obj',"
                    "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program (a, b):: handle",
        "CompilationUnit(ProgramDecl(CallVCommandExpr('handle',receiver='a',receiver='b')))"));
    CHECK(testAst(
        ".program .fail x",
        "CompilationUnit(ProgramDecl(CallFailExpr(CallCommandExpr(IdentifierExpr('x')))))"));
    CHECK(testAst(
        ".program .fail x + y",
        "CompilationUnit("
            "ProgramDecl("
                "CallFailExpr("
                    "BinaryExpr("
                        "CallCommandExpr(IdentifierExpr('x')),"
                        "'+',"
                        "CallCommandExpr(IdentifierExpr('y'))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".program .fail doIt",
        "CompilationUnit(ProgramDecl(CallFailExpr(CallCommandExpr(IdentifierExpr('doIt')))))"));
    CHECK(testAst(
        ".program .fail Widget: x, y",
        "CompilationUnit("
            "ProgramDecl("
                "CallFailExpr("
                    "CallConstructorExpr("
                        "NamedType('Widget'),"
                        "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_GROUP") {
    CHECK(testAst(
        ".test \"x\" = doIt",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))))"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = doIt\n"
        " process: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = doIt\n"
        " process: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y \n"
        " doIt",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = init\n"
        " process: data\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = doIt\n"
        " process: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y\n"
        " doIt\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x\n"
        " doIt\n"
        " (obj):: method: y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {doIt}\n"
        " {process: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = .fail x",
        "CompilationUnit("
            "TestDecl('x',CallGroup(ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('x'))))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = .fail x + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallFailExpr("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = init\n"
        " .fail x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                    "ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('x'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = .fail doIt\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('doIt')))),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = setup\n"
        " .fail errorCode\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('setup'))),"
                    "ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('errorCode')))),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - basic arithmetic") {
    CHECK(testAst(
        ".test \"x\" = a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x - y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = m * n",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('m')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('n'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = p / q",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value << 8",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('value')),'<<',LiteralExpr('8')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = bits >> 4",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('bits')),'>>',LiteralExpr('4')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x - y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = m * n",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('m')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('n'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = p / q",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value << 8",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('value')),'<<',LiteralExpr('8')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = bits >> 4",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('bits')),'>>',LiteralExpr('4')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = width + height",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('width')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('height'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = total - discount",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('total')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('discount'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = area * scale",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('area')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('scale'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = sum / count",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('sum')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('count'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - chained operations") {
    CHECK(testAst(
        ".test \"x\" = a + b + c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x - y - z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a * b * c * d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = p / q / r",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('r'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a << b << c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x >> y >> z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + b + c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x - y - z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a * b * c * d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = p / q / r",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('r'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a << b << c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x >> y >> z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + b - c * d / e",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('e'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = total - discount + tax",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('total')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('discount')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('tax'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value << 1 + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('value')),"
                            "'<<',"
                            "LiteralExpr('1'),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = mask >> 4 * scale",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('mask')),"
                            "'>>',"
                            "LiteralExpr('4'),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('scale'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with parentheses") {
    CHECK(testAst(
        ".test \"x\" = (a + b) * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x * (y + z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('y'),'+',CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a + b) * (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (value << 8) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('value'),'<<',LiteralExpr('8')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = mask >> (shift + 2)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('mask')),"
                            "'>>',"
                            "BinaryExpr(IdentifierExpr('shift'),'+',LiteralExpr('2'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a + b) * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x * (y + z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('y'),'+',CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a + b) * (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ((a + b) * c) / d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('c'))"
                            "),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (x) + (y) * (z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ((a + (b * c))) / d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr("
                                "IdentifierExpr('a'),"
                                "'+',"
                                "BinaryExpr(IdentifierExpr('b'),'*',CallCommandExpr(IdentifierExpr('c')))"
                            "),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (value << 8) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('value'),'<<',LiteralExpr('8')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = mask >> (shift + 2)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('mask')),"
                            "'>>',"
                            "BinaryExpr(IdentifierExpr('shift'),'+',LiteralExpr('2'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - comparison operators") {
    CHECK(testAst(
        ".test \"x\" = a < b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x > y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <= b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x >= y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a = b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a < b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x > y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <= b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x >= y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a = b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a + b) <= (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'<=',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x * y >= z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with invocations") {
    CHECK(testAst(
        ".test \"x\" = (Widget: x, y) + z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (doIt) * (calculate: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (Widget: x, y) + z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (doIt) * (calculate: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ((obj):: method: x) + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallVCommandExpr("
                                "'method',"
                                "receiver='obj',"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + ((b, c):: handle: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallVCommandExpr("
                                "'handle',"
                                "receiver='b',"
                                "receiver='c',"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (Widget: x, y) + (process: a) * (calculate: b)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('a')))"
                            "),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with qualified identifiers") {
    CHECK(testAst(
        ".test \"x\" = Module::value + x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('Module::value')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('x'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a * Std::Math::pi",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('Std::Math::pi'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Module::value + x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('Module::value')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('x'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a * Std::Math::pi",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('Std::Math::pi'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Std::IO::width + Std::IO::height",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('Std::IO::width')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('Std::IO::height'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with array indexing") {
    CHECK(testAst(
        ".test \"x\" = arr[0] + arr[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0'))),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data[i] * data[j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('j')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x + arr[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = matrix[i, j] - offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('matrix')),"
                                "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                            "),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = arr[0] + arr[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('0'))),"
                            "'+',"
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('arr')),Index(LiteralExpr('1')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data[i] * data[j]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('j')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x + arr[i]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = matrix[i, j] - offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('matrix')),"
                                "Index(IdentifierExpr('i'),CallCommandExpr(IdentifierExpr('j')))"
                            "),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = arr[i][j][k] + data[x][y]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('arr')),"
                                "Index(CallCommandExpr(IdentifierExpr('i'))),"
                                "Index(CallCommandExpr(IdentifierExpr('j'))),"
                                "Index(CallCommandExpr(IdentifierExpr('k')))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('data')),"
                                "Index(CallCommandExpr(IdentifierExpr('x'))),"
                                "Index(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with pointer operations") {
    CHECK(testAst(
        ".test \"x\" = ptr^ + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr^[i] * scale",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr')),"
                                "Deref,"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('scale'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr1^[0] + ptr2^[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr1')),"
                                "Deref,"
                                "Index(LiteralExpr('0'))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr2')),"
                                "Deref,"
                                "Index(LiteralExpr('1'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data& + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Addr),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr^ + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr^[i] * scale",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr')),"
                                "Deref,"
                                "Index(CallCommandExpr(IdentifierExpr('i')))"
                            "),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('scale'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = ptr1^[0] + ptr2^[1]",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr1')),"
                                "Deref,"
                                "Index(LiteralExpr('0'))"
                            "),"
                            "'+',"
                            "SuffixExpr("
                                "CallCommandExpr(IdentifierExpr('ptr2')),"
                                "Deref,"
                                "Index(LiteralExpr('1'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data& + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "BinaryExpr("
                            "SuffixExpr(CallCommandExpr(IdentifierExpr('data')),Addr),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - mixed with other statements") {
    CHECK(testAst(
        ".test \"x\" = doIt\n"
        " a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x + y\n"
        " process: result",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y')))),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('result')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y\n"
        " a * b\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat(BinaryExpr(IdentifierExpr('a'),'*',CallCommandExpr(IdentifierExpr('b')))),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = init\n"
        " x + y\n"
        " z - w\n"
        " finalize",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                    "ExprStat(BinaryExpr(IdentifierExpr('x'),'+',CallCommandExpr(IdentifierExpr('y')))),"
                    "ExprStat(BinaryExpr(IdentifierExpr('z'),'-',CallCommandExpr(IdentifierExpr('w')))),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('finalize')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- a + b\n"
        " x * y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))"
                    "),"
                    "ExprStat("
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a + b\n"
        " result <- x * y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))),"
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y\n"
        " a + b\n"
        " result <- c * d\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))),"
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr(IdentifierExpr('c'),'*',CallCommandExpr(IdentifierExpr('d')))"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_EXPRESSION in CALL_GROUP - with literals") {
    CHECK(testAst(
        ".test \"x\" = 42 + x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup(ExprStat(BinaryExpr(LiteralExpr('42'),'+',CallCommandExpr(IdentifierExpr('x')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x + 3.14",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('x')),'+',LiteralExpr('3.14')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = \"hello\" + suffix",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(LiteralExpr('hello'),'+',CallCommandExpr(IdentifierExpr('suffix'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = 42 + x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup(ExprStat(BinaryExpr(LiteralExpr('42'),'+',CallCommandExpr(IdentifierExpr('x')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = x + 3.14",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('x')),'+',LiteralExpr('3.14')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = 100 - discount",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(LiteralExpr('100'),'-',CallCommandExpr(IdentifierExpr('discount'))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = count * 2",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(BinaryExpr(CallCommandExpr(IdentifierExpr('count')),'*',LiteralExpr('2')))"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::BLOCK") {
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ?? process: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhenMulti,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ?- cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhenFail,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ?: select",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhenSelect,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('select')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " - fallback",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoElse,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('fallback'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % execute",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoBlock,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('execute')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ^ retry",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRewind,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('retry'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    // DoRewind with no body — bare `^` is allowed.
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ^",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRewind)"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " | recover",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRecover,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " | Failtype f-> recover",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoRecoverSpec,"
                            "type=NamedType('Failtype'),"
                            "ident='f',"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " | Failtype[T] f-> recover",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoRecoverSpec,"
                            "type=NamedType('Failtype',NamedType('T')),"
                            "ident='f',"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " @ cleanup: resource",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoOnExit,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('cleanup'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('resource')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " @! handleFailure: error",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoOnExitFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('handleFailure'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? doIt\n"
        "  process: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % init\n"
        "  process: data\n"
        "  cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                "),"
                                "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? Widget: x, y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % Container: size\n"
        "  doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Container'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                                    ")"
                                "),"
                                "ExprStat(CallCommandExpr(IdentifierExpr('doIt')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? obj :: method: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('obj')),"
                                        "'::',"
                                        "CallCommandExpr("
                                            "IdentifierExpr('method'),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? (obj):: method: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " | (a, b):: recover: error",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRecover,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'recover',"
                                        "receiver='a',"
                                        "receiver='b',"
                                        "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CMD_BODY") {
    CHECK(testAst(
        ".cmd run: Int x = doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: x + 7",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam("
                                    "BinaryExpr(CallCommandExpr(IdentifierExpr('x')),'+',LiteralExpr('7'))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = Widget: x, y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = List[Int]: item",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallConstructorExpr("
                                "NamedType('List',NamedType('Int')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = (obj):: method: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallVCommandExpr("
                                "'method',"
                                "receiver='obj',"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = (a, b):: handle: item",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallVCommandExpr("
                                "'handle',"
                                "receiver='a',"
                                "receiver='b',"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        "  process: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = Widget: x, y\n"
        " doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        "),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('doIt')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " process: data\n"
        " cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                            ")"
                        "),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = ? doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup(Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? process: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = % execute\n"
        "     cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "Block(DoBlock,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('execute'))))),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = doIt\n"
        " | recover: error",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                        "Block(DoRecover,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('recover'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: x\n"
        " @ cleanup: resource",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        "),"
                        "Block("
                            "DoOnExit,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('cleanup'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('resource')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? Widget: x, y\n"
        " - fallback",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "Block(DoElse,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('fallback')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = % doIt\n"
        " process: x\n"
        " | recover",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "Block(DoBlock,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        "),"
                        "Block(DoRecover,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = _",
        "CompilationUnit(CmdDef(RegularSig('run',param=CmdParam(NamedType('Int'),'x')),CmdBody(empty)))"));
    CHECK(testAst(
        ".cmd run: Int x = .fail x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody(CallGroup(ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('x'))))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = .fail x + y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallFailExpr("
                                "BinaryExpr("
                                    "CallCommandExpr(IdentifierExpr('x')),"
                                    "'+',"
                                    "CallCommandExpr(IdentifierExpr('y'))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = .fail doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody(CallGroup(ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('doIt'))))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = .fail Widget: x, y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallFailExpr("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " .fail x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('x'))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = .fail errorCode\n"
        " ? recover: x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallFailExpr(CallCommandExpr(IdentifierExpr('errorCode')))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('recover'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CMD_BODY - complex scenarios") {
    CHECK(testAst(
        ".test \"x\" = Widget: (Point: x, y), (Size: w, h)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Point'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Size'),"
                                    "CallParam(IdentifierExpr('w')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('h')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: (transform: data), (validate: input)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "IdentifierExpr('transform'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            "),"
                            "CallParam("
                                "CallCommandExpr("
                                    "IdentifierExpr('validate'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('input')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: (Widget: x, y), z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: #x, #y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x',alloc)),"
                            "CallParam(IdentifierExpr('y',alloc))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: #temp, value",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(IdentifierExpr('temp',alloc)),"
                            "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, y\n"
        " (obj):: init\n"
        " process: data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat(CallVCommandExpr('init',receiver='obj')),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: size\n"
        " doIt\n"
        " (obj):: method: x\n"
        " cleanup",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    "),"
                    "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? doIt\n"
        " - fallback",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))),"
                        "Block(DoElse,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('fallback')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = % process: x\n"
        " ? validate\n"
        " - error",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('validate'))))),"
                        "Block(DoElse,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('error')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = doIt\n"
        " | recover\n"
        " @ cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                        "Block(DoRecover,"
                            "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))"
                        "),"
                        "Block(DoOnExit,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = ? Widget: x, y\n"
        " (obj):: init\n"
        " - Container: size\n"
        " fallback",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "ExprStat(CallVCommandExpr('init',receiver='obj')),"
                        "Block("
                            "DoElse,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Container'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('fallback')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? process: data\n"
        " - error: msg\n"
        " @ cleanup: resource",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "Block("
                            "DoElse,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('error'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('msg')))"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "Block("
                            "DoOnExit,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('cleanup'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('resource')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Map[String, List[Int]]: key, values",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Map',NamedType('String'),NamedType('List',NamedType('Int'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('key'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('values')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container[Widget[T]]: (Widget[Int]: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container',NamedType('Widget',NamedType('T'))),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Widget',NamedType('Int')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (Widget: x, y)\n"
        " process: result",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(IdentifierExpr('x')),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    "),"
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('result')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #temp <- doIt\n"
        " process: #temp",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat(IdentifierExpr('temp',alloc),CallCommandExpr(IdentifierExpr('doIt'))),"
                    "ExprStat("
                        "CallCommandExpr(IdentifierExpr('process'),CallParam(IdentifierExpr('temp',alloc)))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handler: (processor: data)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('handler'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "IdentifierExpr('processor'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {Widget: x, y}: result",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('result')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Handler: (callback)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Handler'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('callback')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Processor: ({doIt}: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Processor'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CMD_BODY - with CALL_EXPRESSION") {
    CHECK(testAst(
        ".cmd run: Int x = a + b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('a')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('b'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = x - y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'-',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = width * height",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('width')),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('height'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = sum / count",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('sum')),"
                                "'/',"
                                "CallCommandExpr(IdentifierExpr('count'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = a + b + c",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('a')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('b')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('c'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = (a + b) * (c - d)",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = arr[i] + arr[j]",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "SuffixExpr("
                                    "CallCommandExpr(IdentifierExpr('arr')),"
                                    "Index(CallCommandExpr(IdentifierExpr('i')))"
                                "),"
                                "'+',"
                                "SuffixExpr("
                                    "CallCommandExpr(IdentifierExpr('arr')),"
                                    "Index(CallCommandExpr(IdentifierExpr('j')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = ptr^ + offset",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('offset'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = a < b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('a')),"
                                "'<',"
                                "CallCommandExpr(IdentifierExpr('b'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = x >= y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'>=',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = (Widget: x, y) + offset",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                "),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('offset'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = Module::value + x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('Module::value')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('x'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " a + b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('a')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('b'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = x * y\n"
        " cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(BinaryExpr(IdentifierExpr('x'),'*',CallCommandExpr(IdentifierExpr('y')))),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = doIt\n"
        " a + b\n"
        " finalize",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                        "ExprStat(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('finalize')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = result <- a + b\n"
        " x * y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('result'),"
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))"
                        "),"
                        "ExprStat("
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::BLOCK - with CALL_EXPRESSION") {
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? a + b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('a')),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('b'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ?? x - y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhenMulti,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('x')),"
                                        "'-',"
                                        "CallCommandExpr(IdentifierExpr('y'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ?- width * height",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhenFail,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('width')),"
                                        "'*',"
                                        "CallCommandExpr(IdentifierExpr('height'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " - a + b + c",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoElse,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('a')),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('b')),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('c'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % (a + b) * (c - d)",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "BinaryExpr("
                                            "IdentifierExpr('a'),"
                                            "'+',"
                                            "CallCommandExpr(IdentifierExpr('b'))"
                                        "),"
                                        "'*',"
                                        "BinaryExpr("
                                            "IdentifierExpr('c'),"
                                            "'-',"
                                            "CallCommandExpr(IdentifierExpr('d'))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ^ arr[i] + arr[j]",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRewind,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "SuffixExpr("
                                            "CallCommandExpr(IdentifierExpr('arr')),"
                                            "Index(CallCommandExpr(IdentifierExpr('i')))"
                                        "),"
                                        "'+',"
                                        "SuffixExpr("
                                            "CallCommandExpr(IdentifierExpr('arr')),"
                                            "Index(CallCommandExpr(IdentifierExpr('j')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " | ptr^ + offset",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoRecover,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "SuffixExpr(CallCommandExpr(IdentifierExpr('ptr')),Deref),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('offset'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " @ a < b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoOnExit,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('a')),"
                                        "'<',"
                                        "CallCommandExpr(IdentifierExpr('b'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " @! x >= y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoOnExitFail,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('x')),"
                                        "'>=',"
                                        "CallCommandExpr(IdentifierExpr('y'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? (Widget: x, y) + offset",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallConstructorExpr("
                                            "NamedType('Widget'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        "),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('offset'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % Module::value + x",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('Module::value')),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('x'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? init\n"
        "   a + b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('a')),"
                                        "'+',"
                                        "CallCommandExpr(IdentifierExpr('b'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " % x * y\n"
        "   cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoBlock,"
                            "CallGroup("
                                "ExprStat("
                                    "BinaryExpr(IdentifierExpr('x'),'*',CallCommandExpr(IdentifierExpr('y')))"
                                "),"
                                "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " - doIt\n"
        "   a + b\n"
        "   finalize",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoElse,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('doIt'))),"
                                "ExprStat(BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))),"
                                "ExprStat(CallCommandExpr(IdentifierExpr('finalize')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? result <- a + b\n"
        "   x * y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "AssignStat("
                                    "IdentifierExpr('result'),"
                                    "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))"
                                "),"
                                "ExprStat("
                                    "BinaryExpr("
                                        "CallCommandExpr(IdentifierExpr('x')),"
                                        "'*',"
                                        "CallCommandExpr(IdentifierExpr('y'))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_CMD_BODY - edge cases") {
    CHECK(testAst(
        ".test \"x\" = doIt",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))))"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallVCommandExpr('method',receiver='obj')))))"));
    CHECK(testAst(
        ".test \"x\" = Widget: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: a, b, c, d, e",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('a'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('b'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('c'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('d'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('e')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: a, b, c, d, e",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('a'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('b'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('c'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('d'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('e')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: a, b, c, d, e",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(CallCommandExpr(IdentifierExpr('a'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('b'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('c'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('d'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('e')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a, b, c, d):: method: x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='a',"
                            "receiver='b',"
                            "receiver='c',"
                            "receiver='d',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: (Container: (List[Int]: (Value: x)))",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CallConstructorExpr("
                                    "NamedType('Container'),"
                                    "CallParam("
                                        "CallConstructorExpr("
                                            "NamedType('List',NamedType('Int')),"
                                            "CallParam("
                                                "CallConstructorExpr("
                                                    "NamedType('Value'),"
                                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                                ")"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: x, #temp, y, #output, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(IdentifierExpr('temp',alloc)),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y'))),"
                            "CallParam(IdentifierExpr('output',alloc)),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = {process: data}: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? success\n"
        " - failure\n"
        " | recover\n"
        " @ cleanup\n"
        " @! onFail",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('success'))))),"
                        "Block(DoElse,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('failure'))))),"
                        "Block(DoRecover,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('recover'))))),"
                        "Block(DoOnExit,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('cleanup'))))),"
                        "Block(DoOnExitFail,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('onFail')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = doIt",
        "CompilationUnit(TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))))"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " ? doIt",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "Block(DoWhen,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_PARM_EMPTY and CALL_QUOTE") {
    CHECK(testAst(
        ".test \"x\" = a <- {Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {Container: size}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Container'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {List[Int]: item}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('List',NamedType('Int')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {Map[String, Int]: key, value}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Map',NamedType('String'),NamedType('Int')),"
                                    "CallParam(IdentifierExpr('key')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('value')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {process: item}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {calculate: a, b, c}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('calculate'),"
                                    "CallParam(IdentifierExpr('a')),"
                                    "CallParam(IdentifierExpr('b')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('c')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallVCommandExpr("
                                    "'method',"
                                    "receiver='obj',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {(a, b):: handle: item}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallVCommandExpr("
                                    "'handle',"
                                    "receiver='a',"
                                    "receiver='b',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- {(widget):: process}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr(QuoteExpr(Subquote,CallVCommandExpr('process',receiver='widget')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: {Point: x, y}, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallConstructorExpr("
                                            "NamedType('Point'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: {doIt}, size",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam("
                                "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Handler: {process: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Handler'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallCommandExpr("
                                            "IdentifierExpr('process'),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: {Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallConstructorExpr("
                                            "NamedType('Widget'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = execute: {doIt}, context",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('execute'),"
                            "CallParam("
                                "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('context')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handle: {(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('handle'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallVCommandExpr("
                                            "'method',"
                                            "receiver='obj',"
                                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: {Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallConstructorExpr("
                                            "NamedType('Widget'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a, b):: handle: {process: item}, data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallCommandExpr("
                                            "IdentifierExpr('process'),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: _, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: x, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: _, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat(CallConstructorExpr(NamedType('Widget'),CallParam(empty),CallParam(empty)))"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: _, size, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('size'))),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: _",
        "CompilationUnit("
            "TestDecl('x',CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process'),CallParam(empty)))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: _, item",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: item, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('item'))),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = calculate: _, _, result",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('calculate'),"
                            "CallParam(empty),"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('result')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: _",
        "CompilationUnit("
            "TestDecl('x',CallGroup(ExprStat(CallVCommandExpr('method',receiver='obj',CallParam(empty)))))"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: _, x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a, b):: handle: x, _, z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(empty),"
                            "CallParam(CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: {Point: x, y}, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallConstructorExpr("
                                            "NamedType('Point'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Widget: _, {doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam(empty),"
                            "CallParam("
                                "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = process: _, {Widget: x, y}, data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('process'),"
                            "CallParam(empty),"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallConstructorExpr("
                                            "NamedType('Widget'),"
                                            "CallParam(IdentifierExpr('x')),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: {process: item}, _",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "CallCommandExpr("
                                    "QuoteExpr("
                                        "Subquote,"
                                        "CallCommandExpr("
                                            "IdentifierExpr('process'),"
                                            "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(empty)"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- {Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handler <- {process: data}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('handler'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handler <- ((({process: data})))",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('handler'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallCommandExpr("
                                    "IdentifierExpr('process'),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = callback <- {(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('callback'),"
                        "CallCommandExpr("
                            "QuoteExpr("
                                "Subquote,"
                                "CallVCommandExpr("
                                    "'method',"
                                    "receiver='obj',"
                                    "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #temp <- {doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('temp',alloc),"
                        "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: {Widget: x, y}",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam("
                                    "CallCommandExpr("
                                        "QuoteExpr("
                                            "Subquote,"
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = handler <- {doIt}",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('handler'),"
                            "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt'))))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = execute: {process: data}, context",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('execute'),"
                                "CallParam("
                                    "CallCommandExpr("
                                        "QuoteExpr("
                                            "Subquote,"
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                                            ")"
                                        ")"
                                    ")"
                                "),"
                                "CallParam(CallCommandExpr(IdentifierExpr('context')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " process: {Widget: x, y}\n"
        " cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam("
                                    "CallCommandExpr("
                                        "QuoteExpr("
                                            "Subquote,"
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: _",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody(CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('process'),CallParam(empty)))))"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = Widget: _, y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(empty),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = init\n"
        " process: _, data\n"
        " cleanup",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat(CallCommandExpr(IdentifierExpr('init'))),"
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(empty),"
                                "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                            ")"
                        "),"
                        "ExprStat(CallCommandExpr(IdentifierExpr('cleanup')))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = process: {Widget: _, y}, data",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam("
                                    "CallCommandExpr("
                                        "QuoteExpr("
                                            "Subquote,"
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(empty),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                "),"
                                "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = handler <- {process: _, item}",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('handler'),"
                            "CallCommandExpr("
                                "QuoteExpr("
                                    "Subquote,"
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(empty),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = execute: _, {doIt}, _",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "ExprStat("
                            "CallCommandExpr("
                                "IdentifierExpr('execute'),"
                                "CallParam(empty),"
                                "CallParam("
                                    "CallCommandExpr("
                                        "QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt')))"
                                    ")"
                                "),"
                                "CallParam(empty)"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = ? process: {Widget: x, y}\n"
        " - fallback: _",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "Block("
                            "DoWhen,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam("
                                            "CallCommandExpr("
                                                "QuoteExpr("
                                                    "Subquote,"
                                                    "CallConstructorExpr("
                                                        "NamedType('Widget'),"
                                                        "CallParam(IdentifierExpr('x')),"
                                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                                    ")"
                                                ")"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        "),"
                        "Block("
                            "DoElse,"
                            "CallGroup("
                                "ExprStat(CallCommandExpr(IdentifierExpr('fallback'),CallParam(empty)))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_OPERATOR") {
    CHECK(testAst(
        ".test \"x\" = a <- a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x - y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- m * n",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('m')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('n'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- p / q",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a << b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x >> y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + b + c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x - y - z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a * b * c * d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- p / q / r",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('r'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + b * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x - y / z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a * b + c * d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- p / q - r / s",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('r')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('s'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + b - c * d / e",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('d')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('e'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (a + b) * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x * (y + z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('y'),'+',CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (a + b) * (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a + b) * c) / d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('c'))"
                            "),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (x) + (y) * (z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a + b)) * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (((x))) + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a + (b * c))) / d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr("
                                "IdentifierExpr('a'),"
                                "'+',"
                                "BinaryExpr(IdentifierExpr('b'),'*',CallCommandExpr(IdentifierExpr('c')))"
                            "),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- value << 8",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr(CallCommandExpr(IdentifierExpr('value')),'<<',LiteralExpr('8'))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- bits >> 4",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr(CallCommandExpr(IdentifierExpr('bits')),'>>',LiteralExpr('4'))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a << b << c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'<<',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x >> y >> z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'>>',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a << 1 + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<<',"
                            "LiteralExpr('1'),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x >> 2 * y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>>',"
                            "LiteralExpr('2'),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (a << b) + c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'<<',CallCommandExpr(IdentifierExpr('b'))),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + (b >> c)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "BinaryExpr(IdentifierExpr('b'),'>>',CallCommandExpr(IdentifierExpr('c')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (Widget: x, y) + z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + (process: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (doIt) * (calculate: x)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((obj):: method: x) + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallVCommandExpr("
                                "'method',"
                                "receiver='obj',"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a + ((b, c):: handle: item)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallVCommandExpr("
                                "'handle',"
                                "receiver='b',"
                                "receiver='c',"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- Module::value + x",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('Module::value')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('x'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a * Std::Math::pi",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('Std::Math::pi'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- Module::function: x, y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "CallCommandExpr("
                            "IdentifierExpr('Module::function'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('x'))),"
                            "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (UI::Widget: x, y) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('UI::Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- x - y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = product <- m * n",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('product'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('m')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('n'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #quotient <- p / q",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('quotient',alloc),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('q'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = shifted <- value << 8",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('shifted'),"
                        "BinaryExpr(CallCommandExpr(IdentifierExpr('value')),'<<',LiteralExpr('8'))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = extracted <- bits >> 4",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('extracted'),"
                        "BinaryExpr(CallCommandExpr(IdentifierExpr('bits')),'>>',LiteralExpr('4'))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = combined <- a | b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('combined'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'|',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- f:a,b | g:b,c | 0",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "CallCommandExpr("
                            "IdentifierExpr('f'),"
                            "CallParam(CallCommandExpr(IdentifierExpr('a'))),"
                            "CallParam("
                                "BinaryExpr("
                                    "CallCommandExpr(IdentifierExpr('b')),"
                                    "'|',"
                                    "CallCommandExpr("
                                        "IdentifierExpr('g'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('b'))),"
                                        "CallParam("
                                            "BinaryExpr("
                                                "CallCommandExpr(IdentifierExpr('c')),"
                                                "'|',"
                                                "LiteralExpr('0')"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = sum <- a + b + c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('sum'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = diff <- x - y - z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('diff'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- a * b / c + d",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('b')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('c')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('d'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- p + q * r - s / t",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('p')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('q')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('r')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('s')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('t'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = mask <- (bits << 4) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('mask'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('bits'),'<<',LiteralExpr('4')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (Widget: x, y) + z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- (process: item) * factor",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "BinaryExpr("
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            "),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('factor'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- (doIt) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = data <- ((obj):: method: x) - base",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('data'),"
                        "BinaryExpr("
                            "CallVCommandExpr("
                                "'method',"
                                "receiver='obj',"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            "),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('base'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (a + b) * c",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('c'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- x + (y * z)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'+',"
                            "BinaryExpr(IdentifierExpr('y'),'*',CallCommandExpr(IdentifierExpr('z')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- (a + b) / (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'/',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- a + (Widget: x, y)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = value <- x * (process: item) + y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = output <- (doIt) + (calculate: x) * z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('output'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('doIt')),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                            "),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handler <- {Widget: x, y} + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('handler'),"
                        "BinaryExpr("
                            "CallCommandExpr("
                                "QuoteExpr("
                                    "Subquote,"
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = callback <- base + {process: item}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('callback'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('base')),"
                            "'+',"
                            "CallCommandExpr("
                                "QuoteExpr("
                                    "Subquote,"
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- {doIt} * {calculate: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "CallCommandExpr(QuoteExpr(Subquote,CallCommandExpr(IdentifierExpr('doIt')))),"
                            "'*',"
                            "CallCommandExpr("
                                "QuoteExpr("
                                    "Subquote,"
                                    "CallCommandExpr("
                                        "IdentifierExpr('calculate'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #temp <- a + b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('temp',alloc),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #result <- x * y + z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result',alloc),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = #value <- (Widget: x, y) + offset",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('value',alloc),"
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('offset'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ((a + b) * (c - d)) / ((e + f) * g)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                            "),"
                            "'/',"
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('e'),'+',CallCommandExpr(IdentifierExpr('f'))),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('g'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- (Widget: x, y) + (process: a) * (calculate: b)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallConstructorExpr("
                                "NamedType('Widget'),"
                                "CallParam(IdentifierExpr('x')),"
                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                            "),"
                            "'+',"
                            "CallCommandExpr("
                                "IdentifierExpr('process'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('a')))"
                            "),"
                            "'*',"
                            "CallCommandExpr("
                                "IdentifierExpr('calculate'),"
                                "CallParam(CallCommandExpr(IdentifierExpr('b')))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- ((a + b) * c) + ((d - e) / f)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('c'))"
                            "),"
                            "'+',"
                            "BinaryExpr("
                                "BinaryExpr(IdentifierExpr('d'),'-',CallCommandExpr(IdentifierExpr('e'))),"
                                "'/',"
                                "CallCommandExpr(IdentifierExpr('f'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- width + height",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('width')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('height'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- total - discount + tax",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('total')),"
                            "'-',"
                            "CallCommandExpr(IdentifierExpr('discount')),"
                            "'+',"
                            "CallCommandExpr(IdentifierExpr('tax'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = area <- width * height",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('area'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('width')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('height'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = average <- sum / count",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('average'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('sum')),"
                            "'/',"
                            "CallCommandExpr(IdentifierExpr('count'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = isLess <- a < b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('isLess'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = isGreater <- x > y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('isGreater'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = isLessOrEqual <- a <= b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('isLessOrEqual'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = isGreaterOrEqual <- x >= y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('isGreaterOrEqual'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = isEqual <- a = b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('isEqual'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a < b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x > y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a <= b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'<=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- x >= y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- a = b",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('a')),"
                            "'=',"
                            "CallCommandExpr(IdentifierExpr('b'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = result <- (a + b) <= (c - d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('result'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b'))),"
                            "'<=',"
                            "BinaryExpr(IdentifierExpr('c'),'-',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = check <- x * y >= z",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('check'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'*',"
                            "CallCommandExpr(IdentifierExpr('y')),"
                            "'>=',"
                            "CallCommandExpr(IdentifierExpr('z'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = match <- (a - b) = (c + d)",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('match'),"
                        "BinaryExpr("
                            "BinaryExpr(IdentifierExpr('a'),'-',CallCommandExpr(IdentifierExpr('b'))),"
                            "'=',"
                            "BinaryExpr(IdentifierExpr('c'),'+',CallCommandExpr(IdentifierExpr('d')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = equals <- x = y",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('equals'),"
                        "BinaryExpr("
                            "CallCommandExpr(IdentifierExpr('x')),"
                            "'=',"
                            "CallCommandExpr(IdentifierExpr('y'))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = result <- a + b",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('result'),"
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('a')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('b'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = value <- x * y + z",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('value'),"
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('y')),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('z'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = output <- (Widget: x, y) + offset",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('output'),"
                            "BinaryExpr("
                                "CallConstructorExpr("
                                    "NamedType('Widget'),"
                                    "CallParam(IdentifierExpr('x')),"
                                    "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                "),"
                                "'+',"
                                "CallCommandExpr(IdentifierExpr('offset'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".cmd run: Int x = sum <- a + b\n"
        " product <- x * y",
        "CompilationUnit("
            "CmdDef("
                "RegularSig('run',param=CmdParam(NamedType('Int'),'x')),"
                "CmdBody("
                    "CallGroup("
                        "AssignStat("
                            "IdentifierExpr('sum'),"
                            "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))"
                        "),"
                        "AssignStat("
                            "IdentifierExpr('product'),"
                            "BinaryExpr("
                                "CallCommandExpr(IdentifierExpr('x')),"
                                "'*',"
                                "CallCommandExpr(IdentifierExpr('y'))"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::CALL_BLOCKQUOTE") {
    CHECK(testAst(
        ".test \"x\" = a <- :{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr(BlockNoFail,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :{process: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockNoFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockNoFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :{(obj):: method: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockNoFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr(BlockFail,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !{process: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- !{(obj):: method: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr(BlockMayFail,CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt')))))"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{process: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockMayFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallCommandExpr("
                                        "IdentifierExpr('process'),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockMayFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallConstructorExpr("
                                        "NamedType('Widget'),"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{result <- a + b}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockMayFail,"
                            "CallGroup("
                                "AssignStat("
                                    "IdentifierExpr('result'),"
                                    "BinaryExpr(IdentifierExpr('a'),'+',CallCommandExpr(IdentifierExpr('b')))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{(obj):: method: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockMayFail,"
                            "CallGroup("
                                "ExprStat("
                                    "CallVCommandExpr("
                                        "'method',"
                                        "receiver='obj',"
                                        "CallParam(IdentifierExpr('x')),"
                                        "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{| recover: error}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "AssignStat("
                        "IdentifierExpr('a'),"
                        "QuoteExpr("
                            "BlockMayFail,"
                            "CallGroup("
                                "Block("
                                    "DoRecover,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('recover'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('error')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = a <- :{_}",
        "CompilationUnit(TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),QuoteExpr(BlockNoFail)))))"));
    CHECK(testAst(
        ".test \"x\" = a <- ?{_}",
        "CompilationUnit(TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),QuoteExpr(BlockMayFail)))))"));
    CHECK(testAst(
        ".test \"x\" = a <- !{_}",
        "CompilationUnit(TestDecl('x',CallGroup(AssignStat(IdentifierExpr('a'),QuoteExpr(BlockFail)))))"));
    CHECK(testAst(
        ".test \"x\" = Widget: :{doIt}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Widget'),"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockNoFail,"
                                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Container: !{process: x}, size",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Container'),"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockFail,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('size')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = Handler: ?{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallConstructorExpr("
                            "NamedType('Handler'),"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockMayFail,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = execute: !{doIt}, context",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('execute'),"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockFail,"
                                    "CallGroup(ExprStat(CallCommandExpr(IdentifierExpr('doIt'))))"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('context')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = handle: ?{(obj):: method: x}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallCommandExpr("
                            "IdentifierExpr('handle'),"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockMayFail,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallVCommandExpr("
                                                "'method',"
                                                "receiver='obj',"
                                                "CallParam(CallCommandExpr(IdentifierExpr('x')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (obj):: method: :{Widget: x, y}",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'method',"
                            "receiver='obj',"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockNoFail,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallConstructorExpr("
                                                "NamedType('Widget'),"
                                                "CallParam(IdentifierExpr('x')),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('y')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".test \"x\" = (a, b):: handle: !{process: item}, data",
        "CompilationUnit("
            "TestDecl("
                "'x',"
                "CallGroup("
                    "ExprStat("
                        "CallVCommandExpr("
                            "'handle',"
                            "receiver='a',"
                            "receiver='b',"
                            "CallParam("
                                "QuoteExpr("
                                    "BlockFail,"
                                    "CallGroup("
                                        "ExprStat("
                                            "CallCommandExpr("
                                                "IdentifierExpr('process'),"
                                                "CallParam(CallCommandExpr(IdentifierExpr('item')))"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                            "),"
                            "CallParam(CallCommandExpr(IdentifierExpr('data')))"
                        ")"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::TYPE_EXPR") {
    CHECK(testAst(
        ".alias T: Int",
        "CompilationUnit(AliasDecl('T',NamedType('Int')))"));
    CHECK(testAst(
        ".alias T: String",
        "CompilationUnit(AliasDecl('T',NamedType('String')))"));
    CHECK(testAst(
        ".alias T: T",
        "CompilationUnit(AliasDecl('T',NamedType('T')))"));
    CHECK(testAst(
        ".alias T: List[Int]",
        "CompilationUnit(AliasDecl('T',NamedType('List')))"));
    CHECK(testAst(
        ".alias T: Map[String, Int]",
        "CompilationUnit(AliasDecl('T',NamedType('Map')))"));
    CHECK(testAst(
        ".alias T: ^Int",
        "CompilationUnit(AliasDecl('T',PtrType(1,NamedType('Int'))))"));
    CHECK(testAst(
        ".alias T: ^^Int",
        "CompilationUnit(AliasDecl('T',PtrType(2,NamedType('Int'))))"));
    CHECK(testAst(
        ".alias T: ^List[T]",
        "CompilationUnit(AliasDecl('T',PtrType(1,NamedType('List'))))"));
    CHECK(testAst(
        ".alias T: []Int",
        "CompilationUnit(AliasDecl('T',RangeType('',NamedType('Int'))))"));
    CHECK(testAst(
        ".alias T: [10]Int",
        "CompilationUnit(AliasDecl('T',RangeType('10',NamedType('Int'))))"));
    CHECK(testAst(
        ".alias T: [size]Int",
        "CompilationUnit(AliasDecl('T',RangeType('size',NamedType('Int'))))"));
    CHECK(testAst(
        ".alias T: [][]Int",
        "CompilationUnit(AliasDecl('T',RangeType('',RangeType('',NamedType('Int')))))"));
    CHECK(testAst(
        ".alias T: [4][4]Float",
        "CompilationUnit(AliasDecl('T',RangeType('4',RangeType('4',NamedType('Float')))))"));
    CHECK(testAst(
        ".alias T: :<>",
        "CompilationUnit(AliasDecl('T',CmdType(NoFail)))"));
    CHECK(testAst(
        ".alias T: :<Int>",
        "CompilationUnit(AliasDecl('T',CmdType(NoFail,CmdTypeArg(false,NamedType('Int')))))"));
    CHECK(testAst(
        ".alias T: ?<String>",
        "CompilationUnit(AliasDecl('T',CmdType(MayFail,CmdTypeArg(false,NamedType('String')))))"));
    CHECK(testAst(
        ".alias T: !<Float>",
        "CompilationUnit(AliasDecl('T',CmdType(Fails,CmdTypeArg(false,NamedType('Float')))))"));
    CHECK(testAst(
        ".alias T: :<Int, String>",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "CmdType(NoFail,CmdTypeArg(false,NamedType('Int')),CmdTypeArg(false,NamedType('String')))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: :<:<Int'> >",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "CmdType(NoFail,CmdTypeArg(false,CmdType(NoFail,CmdTypeArg(true,NamedType('Int')))))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: ^[]Int",
        "CompilationUnit(AliasDecl('T',PtrType(1,RangeType('',NamedType('Int')))))"));
    CHECK(testAst(
        ".alias T: []^Int",
        "CompilationUnit(AliasDecl('T',RangeType('',PtrType(1,NamedType('Int')))))"));
    CHECK(testAst(
        ".alias T: ^:<Int>",
        "CompilationUnit(AliasDecl('T',PtrType(1,CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))))))"));
    CHECK(testAst(
        ".alias T: []:<String>",
        "CompilationUnit(AliasDecl('T',RangeType('',CmdType(NoFail,CmdTypeArg(false,NamedType('String'))))))"));
    CHECK(testAst(
        ".alias T: Std::String",
        "CompilationUnit(AliasDecl('T',NamedType('Std::String')))"));
    CHECK(testAst(
        ".alias T: Collections::List[T]",
        "CompilationUnit(AliasDecl('T',NamedType('Collections::List')))"));
    CHECK(testAst(
        ".alias T: ^Namespace::Type",
        "CompilationUnit(AliasDecl('T',PtrType(1,NamedType('Namespace::Type'))))"));
    CHECK(testAst(
        ".alias T: .record Int x, Int y",
        "CompilationUnit("
            "AliasDecl('T',InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"
        ")"));
    CHECK(testAst(
        ".alias T: .record scope: Int x, Int y",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType("
                    "scope='scope',"
                    "FieldDecl(NamedType('Int'),'x'),"
                    "FieldDecl(NamedType('Int'),'y')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object ^Node next, Int val",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                    "FieldDecl(NamedType('Int'),'val')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object scope: ^Node next, Int val",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "scope='scope',"
                    "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                    "FieldDecl(NamedType('Int'),'val')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType(UnionCandidate(NamedType('Int'),'i'),UnionCandidate(NamedType('Float'),'f'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union scope: Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType("
                    "scope='scope',"
                    "UnionCandidate(NamedType('Int'),'i'),"
                    "UnionCandidate(NamedType('Float'),'f')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant Int circle, ^Node inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(NamedType('Int'),'circle'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant scope: Int circle, ^Node inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "scope='scope',"
                    "VariantCandidate(NamedType('Int'),'circle'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: ^.record Int x, Int y",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "PtrType(1,InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: [].union Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "RangeType("
                    "'',"
                    "InlineUnionType("
                        "UnionCandidate(NamedType('Int'),'i'),"
                        "UnionCandidate(NamedType('Float'),'f')"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_INLINE_RECORD") {
    CHECK(testAst(
        ".alias T: .record Int x",
        "CompilationUnit(AliasDecl('T',InlineRecordType(FieldDecl(NamedType('Int'),'x'))))"));
    CHECK(testAst(
        ".alias T: .record String name",
        "CompilationUnit(AliasDecl('T',InlineRecordType(FieldDecl(NamedType('String'),'name'))))"));
    CHECK(testAst(
        ".alias T: .record Int x, Int y",
        "CompilationUnit("
            "AliasDecl('T',InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"
        ")"));
    CHECK(testAst(
        ".alias T: .record String name, Int age, Float salary",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType("
                    "FieldDecl(NamedType('String'),'name'),"
                    "FieldDecl(NamedType('Int'),'age'),"
                    "FieldDecl(NamedType('Float'),'salary')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record myScope: Int x",
        "CompilationUnit(AliasDecl('T',InlineRecordType(scope='myScope',FieldDecl(NamedType('Int'),'x'))))"));
    CHECK(testAst(
        ".alias T: .record scope: String name, Int age",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType("
                    "scope='scope',"
                    "FieldDecl(NamedType('String'),'name'),"
                    "FieldDecl(NamedType('Int'),'age')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record pt: Int x, Int y",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType(scope='pt',FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record [3]Float position",
        "CompilationUnit("
            "AliasDecl('T',InlineRecordType(FieldDecl(RangeType('3',NamedType('Float')),'position')))"
        ")"));
    CHECK(testAst(
        ".alias T: .record [size]Byte data",
        "CompilationUnit(AliasDecl('T',InlineRecordType(FieldDecl(RangeType('size',NamedType('Byte')),'data'))))"));
    CHECK(testAst(
        ".alias T: .record [4][4]Float matrix",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType(FieldDecl(RangeType('4',RangeType('4',NamedType('Float'))),'matrix'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record List[Int] items",
        "CompilationUnit("
            "AliasDecl('T',InlineRecordType(FieldDecl(NamedType('List',NamedType('Int')),'items')))"
        ")"));
    CHECK(testAst(
        ".alias T: .record Map[String, Int] data",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType(FieldDecl(NamedType('Map',NamedType('String'),NamedType('Int')),'data'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record Std::String name",
        "CompilationUnit(AliasDecl('T',InlineRecordType(FieldDecl(NamedType('Std::String'),'name'))))"));
    CHECK(testAst(
        ".alias T: .record .record Int x, Int y pt, Int z",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType("
                    "FieldDecl("
                        "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                        "'pt'"
                    "),"
                    "FieldDecl(NamedType('Int'),'z')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .record .union Int i, Float f value",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineRecordType("
                    "FieldDecl("
                        "InlineUnionType("
                            "UnionCandidate(NamedType('Int'),'i'),"
                            "UnionCandidate(NamedType('Float'),'f')"
                        "),"
                        "'value'"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_INLINE_OBJECT") {
    CHECK(testAst(
        ".alias T: .object Int x",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(NamedType('Int'),'x'))))"));
    CHECK(testAst(
        ".alias T: .object String name",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(NamedType('String'),'name'))))"));
    CHECK(testAst(
        ".alias T: .object Int x, Int y",
        "CompilationUnit("
            "AliasDecl('T',InlineObjectType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')))"
        ")"));
    CHECK(testAst(
        ".alias T: .object String name, Int age",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType(FieldDecl(NamedType('String'),'name'),FieldDecl(NamedType('Int'),'age'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object myScope: Int x",
        "CompilationUnit(AliasDecl('T',InlineObjectType(scope='myScope',FieldDecl(NamedType('Int'),'x'))))"));
    CHECK(testAst(
        ".alias T: .object scope: String name, Int age",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "scope='scope',"
                    "FieldDecl(NamedType('String'),'name'),"
                    "FieldDecl(NamedType('Int'),'age')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object ^Node next",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(PtrType(1,NamedType('Node')),'next'))))"));
    CHECK(testAst(
        ".alias T: .object ^Node next, Int val",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                    "FieldDecl(NamedType('Int'),'val')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object ^^Int ptrPtr",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(PtrType(2,NamedType('Int')),'ptrPtr'))))"));
    CHECK(testAst(
        ".alias T: .object []Int items",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(RangeType('',NamedType('Int')),'items'))))"));
    CHECK(testAst(
        ".alias T: .object ^[]Int ptrArray",
        "CompilationUnit("
            "AliasDecl('T',InlineObjectType(FieldDecl(PtrType(1,RangeType('',NamedType('Int'))),'ptrArray')))"
        ")"));
    CHECK(testAst(
        ".alias T: .object :<Int> callback",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType(FieldDecl(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'callback'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object ?<String> loader",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType(FieldDecl(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'loader'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object :<> voidCmd",
        "CompilationUnit(AliasDecl('T',InlineObjectType(FieldDecl(CmdType(NoFail),'voidCmd'))))"));
    CHECK(testAst(
        ".alias T: .object .record Int x, Int y pt",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "FieldDecl("
                        "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                        "'pt'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object .union Int i, Float f value",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "FieldDecl("
                        "InlineUnionType("
                            "UnionCandidate(NamedType('Int'),'i'),"
                            "UnionCandidate(NamedType('Float'),'f')"
                        "),"
                        "'value'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object .object Int x inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType(FieldDecl(InlineObjectType(FieldDecl(NamedType('Int'),'x')),'inner'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .object .variant Int circle, ^Node n shape",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineObjectType("
                    "FieldDecl("
                        "InlineVariantType("
                            "VariantCandidate(NamedType('Int'),'circle'),"
                            "VariantCandidate(PtrType(1,NamedType('Node')),'n')"
                        "),"
                        "'shape'"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_INLINE_UNION") {
    CHECK(testAst(
        ".alias T: .union Int whole",
        "CompilationUnit(AliasDecl('T',InlineUnionType(UnionCandidate(NamedType('Int'),'whole'))))"));
    CHECK(testAst(
        ".alias T: .union Float fractional",
        "CompilationUnit(AliasDecl('T',InlineUnionType(UnionCandidate(NamedType('Float'),'fractional'))))"));
    CHECK(testAst(
        ".alias T: .union Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType(UnionCandidate(NamedType('Int'),'i'),UnionCandidate(NamedType('Float'),'f'))"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union Int i, Float f, String s",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType("
                    "UnionCandidate(NamedType('Int'),'i'),"
                    "UnionCandidate(NamedType('Float'),'f'),"
                    "UnionCandidate(NamedType('String'),'s')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union myScope: Int whole",
        "CompilationUnit("
            "AliasDecl('T',InlineUnionType(scope='myScope',UnionCandidate(NamedType('Int'),'whole')))"
        ")"));
    CHECK(testAst(
        ".alias T: .union scope: Int i, Float f",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType("
                    "scope='scope',"
                    "UnionCandidate(NamedType('Int'),'i'),"
                    "UnionCandidate(NamedType('Float'),'f')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union [3]Float vec",
        "CompilationUnit("
            "AliasDecl('T',InlineUnionType(UnionCandidate(RangeType('3',NamedType('Float')),'vec')))"
        ")"));
    CHECK(testAst(
        ".alias T: .union [size]Byte buf",
        "CompilationUnit("
            "AliasDecl('T',InlineUnionType(UnionCandidate(RangeType('size',NamedType('Byte')),'buf')))"
        ")"));
    CHECK(testAst(
        ".alias T: .union List[Int] items",
        "CompilationUnit("
            "AliasDecl('T',InlineUnionType(UnionCandidate(NamedType('List',NamedType('Int')),'items')))"
        ")"));
    CHECK(testAst(
        ".alias T: .union Std::String name",
        "CompilationUnit(AliasDecl('T',InlineUnionType(UnionCandidate(NamedType('Std::String'),'name'))))"));
    CHECK(testAst(
        ".alias T: .union .record Int x, Int y pt",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType("
                    "UnionCandidate("
                        "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                        "'pt'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .union .union Int i, Float f value",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineUnionType("
                    "UnionCandidate("
                        "InlineUnionType("
                            "UnionCandidate(NamedType('Int'),'i'),"
                            "UnionCandidate(NamedType('Float'),'f')"
                        "),"
                        "'value'"
                    ")"
                ")"
            ")"
        ")"));
}

TEST_CASE("Ast::DEF_INLINE_VARIANT") {
    CHECK(testAst(
        ".alias T: .variant Int circle",
        "CompilationUnit(AliasDecl('T',InlineVariantType(VariantCandidate(NamedType('Int'),'circle'))))"));
    CHECK(testAst(
        ".alias T: .variant String text",
        "CompilationUnit(AliasDecl('T',InlineVariantType(VariantCandidate(NamedType('String'),'text'))))"));
    CHECK(testAst(
        ".alias T: .variant Int circle, Float radius",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(NamedType('Int'),'circle'),"
                    "VariantCandidate(NamedType('Float'),'radius')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant Int leaf, ^Node inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(NamedType('Int'),'leaf'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant Int leaf, ^Node inner, []Int data",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(NamedType('Int'),'leaf'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'inner'),"
                    "VariantCandidate(RangeType('',NamedType('Int')),'data')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant myScope: Int circle",
        "CompilationUnit("
            "AliasDecl('T',InlineVariantType(scope='myScope',VariantCandidate(NamedType('Int'),'circle')))"
        ")"));
    CHECK(testAst(
        ".alias T: .variant scope: Int leaf, ^Node inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "scope='scope',"
                    "VariantCandidate(NamedType('Int'),'leaf'),"
                    "VariantCandidate(PtrType(1,NamedType('Node')),'inner')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant ^Node inner",
        "CompilationUnit("
            "AliasDecl('T',InlineVariantType(VariantCandidate(PtrType(1,NamedType('Node')),'inner')))"
        ")"));
    CHECK(testAst(
        ".alias T: .variant ^^Int ptr",
        "CompilationUnit("
            "AliasDecl('T',InlineVariantType(VariantCandidate(PtrType(2,NamedType('Int')),'ptr')))"
        ")"));
    CHECK(testAst(
        ".alias T: .variant []Int items",
        "CompilationUnit("
            "AliasDecl('T',InlineVariantType(VariantCandidate(RangeType('',NamedType('Int')),'items')))"
        ")"));
    CHECK(testAst(
        ".alias T: .variant :<Int> action",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(CmdType(NoFail,CmdTypeArg(false,NamedType('Int'))),'action')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant ?<String> loader",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate(CmdType(MayFail,CmdTypeArg(false,NamedType('String'))),'loader')"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant :<> voidAction",
        "CompilationUnit(AliasDecl('T',InlineVariantType(VariantCandidate(CmdType(NoFail),'voidAction'))))"));
    CHECK(testAst(
        ".alias T: .variant .record Int x, Int y pt",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate("
                        "InlineRecordType(FieldDecl(NamedType('Int'),'x'),FieldDecl(NamedType('Int'),'y')),"
                        "'pt'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant .union Int i, Float f value",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate("
                        "InlineUnionType("
                            "UnionCandidate(NamedType('Int'),'i'),"
                            "UnionCandidate(NamedType('Float'),'f')"
                        "),"
                        "'value'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant .object ^Node next, Int val inner",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate("
                        "InlineObjectType("
                            "FieldDecl(PtrType(1,NamedType('Node')),'next'),"
                            "FieldDecl(NamedType('Int'),'val')"
                        "),"
                        "'inner'"
                    ")"
                ")"
            ")"
        ")"));
    CHECK(testAst(
        ".alias T: .variant .variant Int circle, ^Node n shape",
        "CompilationUnit("
            "AliasDecl("
                "'T',"
                "InlineVariantType("
                    "VariantCandidate("
                        "InlineVariantType("
                            "VariantCandidate(NamedType('Int'),'circle'),"
                            "VariantCandidate(PtrType(1,NamedType('Node')),'n')"
                        "),"
                        "'shape'"
                    ")"
                ")"
            ")"
        ")"));
}
