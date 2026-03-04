#include "doctest.h"
#include "../Ast.h"
#include <map>
#include <string>
/*
using namespace basis;

// =============================================================================
// CountingVisitor — counts total visits and per-type visits
// =============================================================================
namespace {

struct CountingVisitor : DefaultVisitor {
    int total = 0;
    std::map<std::string, int> byType;

    // Pre-order: count self, then walk children via DefaultVisitor
    void visit(CompilationUnit& n)  override { count("CompilationUnit");  DefaultVisitor::visit(n); }
    void visit(ModuleDecl& n)       override { count("ModuleDecl");        DefaultVisitor::visit(n); }
    void visit(ImportDecl& n)       override { count("ImportDecl");        DefaultVisitor::visit(n); }
    void visit(TypeExpr& n)         override { count("TypeExpr");          DefaultVisitor::visit(n); }
    void visit(AliasDecl& n)        override { count("AliasDecl");         DefaultVisitor::visit(n); }
    void visit(DomainDecl& n)       override { count("DomainDecl");        DefaultVisitor::visit(n); }
    void visit(EnumDecl& n)         override { count("EnumDecl");          DefaultVisitor::visit(n); }
    void visit(EnumItem& n)         override { count("EnumItem");          DefaultVisitor::visit(n); }
    void visit(FieldDecl& n)        override { count("FieldDecl");         DefaultVisitor::visit(n); }
    void visit(RecordDecl& n)       override { count("RecordDecl");        DefaultVisitor::visit(n); }
    void visit(ObjectDecl& n)       override { count("ObjectDecl");        DefaultVisitor::visit(n); }
    void visit(InstanceType& n)     override { count("InstanceType");      DefaultVisitor::visit(n); }
    void visit(InstanceDecl& n)     override { count("InstanceDecl");      DefaultVisitor::visit(n); }
    void visit(CmdDecl& n)          override { count("CmdDecl");           DefaultVisitor::visit(n); }
    void visit(CmdDef& n)           override { count("CmdDef");            DefaultVisitor::visit(n); }
    void visit(IntrinsicDecl& n)    override { count("IntrinsicDecl");     DefaultVisitor::visit(n); }
    void visit(ClassDecl& n)        override { count("ClassDecl");         DefaultVisitor::visit(n); }
    void visit(ProgramDecl& n)      override { count("ProgramDecl");       DefaultVisitor::visit(n); }
    void visit(TestDecl& n)         override { count("TestDecl");          DefaultVisitor::visit(n); }
    void visit(CmdBody& n)          override { count("CmdBody");           DefaultVisitor::visit(n); }
    void visit(CallGroup& n)        override { count("CallGroup");         DefaultVisitor::visit(n); }
    void visit(CallInvoke& n)       override { count("CallInvoke");        DefaultVisitor::visit(n); }
    void visit(CallAssignment& n)   override { count("CallAssignment");    DefaultVisitor::visit(n); }
    void visit(CallExpression& n)   override { count("CallExpression");    DefaultVisitor::visit(n); }
    void visit(Block& n)            override { count("Block");             DefaultVisitor::visit(n); }
    void visit(Literal& n)          override { count("Literal");           DefaultVisitor::visit(n); }
    void visit(IdentifierExpr& n)   override { count("IdentifierExpr");    DefaultVisitor::visit(n); }
    void visit(SubcallExpr& n)      override { count("SubcallExpr");       DefaultVisitor::visit(n); }
    void visit(CallQuote& n)        override { count("CallQuote");         DefaultVisitor::visit(n); }
    void visit(CmdLiteral& n)       override { count("CmdLiteral");        DefaultVisitor::visit(n); }
    void visit(CallParameter& n)    override { count("CallParameter");     DefaultVisitor::visit(n); }

private:
    void count(const char* name) { ++total; ++byType[name]; }
};

// Post-order variant: walk children first, then count self
struct PostOrderCountingVisitor : DefaultVisitor {
    int total = 0;
    std::map<std::string, int> byType;

    void visit(CompilationUnit& n)  override { DefaultVisitor::visit(n); count("CompilationUnit"); }
    void visit(ModuleDecl& n)       override { DefaultVisitor::visit(n); count("ModuleDecl"); }
    void visit(ImportDecl& n)       override { DefaultVisitor::visit(n); count("ImportDecl"); }
    void visit(TypeExpr& n)         override { DefaultVisitor::visit(n); count("TypeExpr"); }
    void visit(AliasDecl& n)        override { DefaultVisitor::visit(n); count("AliasDecl"); }
    void visit(DomainDecl& n)       override { DefaultVisitor::visit(n); count("DomainDecl"); }
    void visit(EnumDecl& n)         override { DefaultVisitor::visit(n); count("EnumDecl"); }
    void visit(EnumItem& n)         override { DefaultVisitor::visit(n); count("EnumItem"); }
    void visit(FieldDecl& n)        override { DefaultVisitor::visit(n); count("FieldDecl"); }
    void visit(RecordDecl& n)       override { DefaultVisitor::visit(n); count("RecordDecl"); }
    void visit(ObjectDecl& n)       override { DefaultVisitor::visit(n); count("ObjectDecl"); }
    void visit(InstanceType& n)     override { DefaultVisitor::visit(n); count("InstanceType"); }
    void visit(InstanceDecl& n)     override { DefaultVisitor::visit(n); count("InstanceDecl"); }
    void visit(CmdDecl& n)          override { DefaultVisitor::visit(n); count("CmdDecl"); }
    void visit(CmdDef& n)           override { DefaultVisitor::visit(n); count("CmdDef"); }
    void visit(IntrinsicDecl& n)    override { DefaultVisitor::visit(n); count("IntrinsicDecl"); }
    void visit(ClassDecl& n)        override { DefaultVisitor::visit(n); count("ClassDecl"); }
    void visit(ProgramDecl& n)      override { DefaultVisitor::visit(n); count("ProgramDecl"); }
    void visit(TestDecl& n)         override { DefaultVisitor::visit(n); count("TestDecl"); }
    void visit(CmdBody& n)          override { DefaultVisitor::visit(n); count("CmdBody"); }
    void visit(CallGroup& n)        override { DefaultVisitor::visit(n); count("CallGroup"); }
    void visit(CallInvoke& n)       override { DefaultVisitor::visit(n); count("CallInvoke"); }
    void visit(CallAssignment& n)   override { DefaultVisitor::visit(n); count("CallAssignment"); }
    void visit(CallExpression& n)   override { DefaultVisitor::visit(n); count("CallExpression"); }
    void visit(Block& n)            override { DefaultVisitor::visit(n); count("Block"); }
    void visit(Literal& n)          override { DefaultVisitor::visit(n); count("Literal"); }
    void visit(IdentifierExpr& n)   override { DefaultVisitor::visit(n); count("IdentifierExpr"); }
    void visit(SubcallExpr& n)      override { DefaultVisitor::visit(n); count("SubcallExpr"); }
    void visit(CallQuote& n)        override { DefaultVisitor::visit(n); count("CallQuote"); }
    void visit(CmdLiteral& n)       override { DefaultVisitor::visit(n); count("CmdLiteral"); }
    void visit(CallParameter& n)    override { DefaultVisitor::visit(n); count("CallParameter"); }

private:
    void count(const char* name) { ++total; ++byType[name]; }
};

// Helper: make a named TypeExpr
auto makeTypeExpr(TypeExpr::Kind kind = TypeExpr::Kind::Named) {
    auto t = std::make_shared<TypeExpr>(); t->kind = kind; return t;
}
// Helper: make a CmdSignature with one receiver, one param, one implicitParam (each with a TypeExpr)
CmdSignature makeSig() {
    CmdSignature sig;
    sig.receivers.push_back({makeTypeExpr(), "recv1"});
    sig.receivers.push_back({makeTypeExpr(), "recv2"});
    sig.params.push_back({makeTypeExpr(), "p", false, ""});
    sig.implicitParams.push_back({makeTypeExpr(), "ip", false, ""});
    return sig;
}

} // namespace

// =============================================================================
// Build the maximally-populated test tree.
// Every node type appears at least once; every child relationship is exercised.
//
// Expected per-type counts (total = 69):
//   CompilationUnit:1  ModuleDecl:1    ImportDecl:1
//   AliasDecl:1        DomainDecl:1    EnumDecl:1      EnumItem:2
//   RecordDecl:1       ObjectDecl:1    FieldDecl:2     InstanceDecl:1  InstanceType:2
//   CmdDecl:2          IntrinsicDecl:1 CmdDef:1        ClassDecl:1
//   ProgramDecl:1      TestDecl:1
//   TypeExpr:21        CmdBody:1       CallGroup:4     CallInvoke:5
//   CallParameter:2    CallAssignment:1 CallExpression:1 SubcallExpr:1
//   Block:1            CallQuote:1     CmdLiteral:1
//   IdentifierExpr:5   Literal:3
// =============================================================================
namespace {

std::shared_ptr<CompilationUnit> buildFullTree() {
    auto cu = std::make_shared<CompilationUnit>();

    // module + import
    cu->module = std::make_shared<ModuleDecl>();
    cu->imports.push_back(std::make_shared<ImportDecl>());

    // AliasDecl: type = TypeExpr(Named, typeArgs=[TypeExpr(Named)])
    {
        auto alias = std::make_shared<AliasDecl>();
        auto t = makeTypeExpr();
        t->typeArgs.push_back(makeTypeExpr());   // typeArgs child
        alias->type = t;
        cu->definitions.push_back(alias);
    }

    // DomainDecl: parent = TypeExpr(Pointer, inner=TypeExpr(Named))
    {
        auto dom = std::make_shared<DomainDecl>();
        auto t = makeTypeExpr(TypeExpr::Kind::Pointer);
        t->inner = makeTypeExpr();               // inner child
        dom->parent = t;
        cu->definitions.push_back(dom);
    }

    // EnumDecl with two EnumItems
    {
        auto en = std::make_shared<EnumDecl>();
        en->items.push_back(std::make_shared<EnumItem>());
        en->items.push_back(std::make_shared<EnumItem>());
        cu->definitions.push_back(en);
    }

    // RecordDecl: field with TypeExpr(Range, inner=TypeExpr(Named))
    {
        auto rec = std::make_shared<RecordDecl>();
        auto fd = std::make_shared<FieldDecl>();
        auto t = makeTypeExpr(TypeExpr::Kind::Range);
        t->inner = makeTypeExpr();               // inner child
        fd->type = t;
        rec->fields.push_back(fd);
        cu->definitions.push_back(rec);
    }

    // ObjectDecl: field with TypeExpr(Command, cmdArgs=[TypeExpr(Named)])
    {
        auto obj = std::make_shared<ObjectDecl>();
        auto fd = std::make_shared<FieldDecl>();
        auto t = makeTypeExpr(TypeExpr::Kind::Command);
        t->cmdArgs.push_back(makeTypeExpr());    // cmdArgs child
        fd->type = t;
        obj->fields.push_back(fd);
        cu->definitions.push_back(obj);
    }

    // InstanceDecl with two InstanceTypes
    {
        auto inst = std::make_shared<InstanceDecl>();
        inst->types.push_back(std::make_shared<InstanceType>());
        inst->types.push_back(std::make_shared<InstanceType>());
        cu->definitions.push_back(inst);
    }

    // CmdDecl (top-level): sig with receiver, param, implicitParam (3 TypeExprs)
    {
        auto cd = std::make_shared<CmdDecl>();
        cd->signature = makeSig();
        cu->definitions.push_back(cd);
    }

    // IntrinsicDecl: sig with receiver, param, implicitParam (3 TypeExprs)
    {
        auto id = std::make_shared<IntrinsicDecl>();
        id->signature = makeSig();
        cu->definitions.push_back(id);
    }

    // CmdDef: sig (3 TypeExprs) + body containing all statement/expression node types
    {
        auto def = std::make_shared<CmdDef>();
        def->signature = makeSig();

        auto group = std::make_shared<CallGroup>();

        // stmt 1: CallInvoke -> CallParameter -> IdentifierExpr
        {
            auto inv = std::make_shared<CallInvoke>();
            auto param = std::make_shared<CallParameter>();
            param->expr = std::make_shared<IdentifierExpr>();
            inv->params.push_back(param);
            group->statements.push_back(inv);
        }

        // stmt 2: CallAssignment: target=IdentifierExpr,
        //         exprs=[SubcallExpr(Literal, IdentifierExpr)],
        //         postOps=[("op", Literal)]
        {
            auto asgn = std::make_shared<CallAssignment>();
            asgn->target = std::make_shared<IdentifierExpr>();
            auto sub = std::make_shared<SubcallExpr>();
            sub->terms.push_back(std::make_shared<Literal>());
            sub->terms.push_back(std::make_shared<IdentifierExpr>());
            asgn->exprs.push_back(sub);
            asgn->postOps.push_back({"op", std::make_shared<Literal>()});
            group->statements.push_back(asgn);
        }

        // stmt 3: CallExpression: terms=[Literal, IdentifierExpr]
        {
            auto expr = std::make_shared<CallExpression>();
            expr->terms.push_back(std::make_shared<Literal>());
            expr->terms.push_back(std::make_shared<IdentifierExpr>());
            group->statements.push_back(expr);
        }

        // stmt 4: Block -> CallGroup -> CallQuote -> CallInvoke -> CallParameter -> IdentifierExpr
        {
            auto block = std::make_shared<Block>();
            auto bg = std::make_shared<CallGroup>();
            auto quote = std::make_shared<CallQuote>();
            auto qinv = std::make_shared<CallInvoke>();
            auto qparam = std::make_shared<CallParameter>();
            qparam->expr = std::make_shared<IdentifierExpr>();
            qinv->params.push_back(qparam);
            quote->body = qinv;
            bg->statements.push_back(quote);
            block->body = bg;
            group->statements.push_back(block);
        }

        // stmt 5: CmdLiteral -> CallGroup -> CallInvoke (no params)
        {
            auto cmdlit = std::make_shared<CmdLiteral>();
            auto lg = std::make_shared<CallGroup>();
            lg->statements.push_back(std::make_shared<CallInvoke>());
            cmdlit->body = lg;
            group->statements.push_back(cmdlit);
        }

        auto body = std::make_shared<CmdBody>();
        body->group = group;
        def->body = body;
        cu->definitions.push_back(def);
    }

    // ClassDecl: one CmdDecl member with one param TypeExpr
    {
        auto cls = std::make_shared<ClassDecl>();
        auto cd = std::make_shared<CmdDecl>();
        cd->signature.params.push_back({makeTypeExpr(), "p", false, ""});
        cls->members.push_back(cd);
        cu->definitions.push_back(cls);
    }

    // ProgramDecl: entryPoint = CallInvoke (no params)
    {
        auto prog = std::make_shared<ProgramDecl>();
        prog->entryPoint = std::make_shared<CallInvoke>();
        cu->definitions.push_back(prog);
    }

    // TestDecl: body = CallGroup -> CallInvoke (no params)
    {
        auto test = std::make_shared<TestDecl>();
        auto tg = std::make_shared<CallGroup>();
        tg->statements.push_back(std::make_shared<CallInvoke>());
        test->body = tg;
        cu->definitions.push_back(test);
    }

    return cu;
}

void checkCounts(const std::map<std::string, int>& byType, int total) {
    CHECK_EQ(total,                          69);
    CHECK_EQ(byType.at("CompilationUnit"),    1);
    CHECK_EQ(byType.at("ModuleDecl"),         1);
    CHECK_EQ(byType.at("ImportDecl"),         1);
    CHECK_EQ(byType.at("AliasDecl"),          1);
    CHECK_EQ(byType.at("DomainDecl"),         1);
    CHECK_EQ(byType.at("EnumDecl"),           1);
    CHECK_EQ(byType.at("EnumItem"),           2);
    CHECK_EQ(byType.at("RecordDecl"),         1);
    CHECK_EQ(byType.at("ObjectDecl"),         1);
    CHECK_EQ(byType.at("FieldDecl"),          2);
    CHECK_EQ(byType.at("InstanceDecl"),       1);
    CHECK_EQ(byType.at("InstanceType"),       2);
    CHECK_EQ(byType.at("CmdDecl"),            2);
    CHECK_EQ(byType.at("IntrinsicDecl"),      1);
    CHECK_EQ(byType.at("CmdDef"),             1);
    CHECK_EQ(byType.at("ClassDecl"),          1);
    CHECK_EQ(byType.at("ProgramDecl"),        1);
    CHECK_EQ(byType.at("TestDecl"),           1);
    CHECK_EQ(byType.at("TypeExpr"),          21);
    CHECK_EQ(byType.at("CmdBody"),            1);
    CHECK_EQ(byType.at("CallGroup"),          4);
    CHECK_EQ(byType.at("CallInvoke"),         5);
    CHECK_EQ(byType.at("CallParameter"),      2);
    CHECK_EQ(byType.at("CallAssignment"),     1);
    CHECK_EQ(byType.at("CallExpression"),     1);
    CHECK_EQ(byType.at("SubcallExpr"),        1);
    CHECK_EQ(byType.at("Block"),              1);
    CHECK_EQ(byType.at("CallQuote"),          1);
    CHECK_EQ(byType.at("CmdLiteral"),         1);
    CHECK_EQ(byType.at("IdentifierExpr"),     5);
    CHECK_EQ(byType.at("Literal"),            3);
}

} // namespace

// =============================================================================
// Tests
// =============================================================================

TEST_CASE("AST DefaultVisitor: pre-order traversal visits all nodes with correct counts") {
    auto cu = buildFullTree();
    CountingVisitor v;
    cu->accept(v);
    checkCounts(v.byType, v.total);
}

TEST_CASE("AST DefaultVisitor: post-order traversal visits all nodes with correct counts") {
    auto cu = buildFullTree();
    PostOrderCountingVisitor v;
    cu->accept(v);
    checkCounts(v.byType, v.total);
}
*/