#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>
#include <cstddef>

namespace basis {

// Forward declarations of all concrete node types
struct CompilationUnit;
struct ModuleDecl;
struct ImportDecl;
struct TypeExpr;
struct AliasDecl;
struct DomainDecl;
struct EnumDecl;
struct EnumItem;
struct FieldDecl;
struct RecordDecl;
struct ObjectDecl;
struct InstanceType;
struct InstanceDecl;
struct CmdDecl;
struct CmdDef;
struct IntrinsicDecl;
struct ClassDecl;
struct ProgramDecl;
struct TestDecl;
struct CmdBody;
struct CallGroup;
struct CallInvoke;
struct CallAssignment;
struct CallExpression;
struct Block;
struct Literal;
struct IdentifierExpr;
struct SubcallExpr;
struct CallQuote;
struct CmdLiteral;
struct CallParameter;

// ---- Visitor interface ----
struct Visitor {
    virtual ~Visitor() = default;
    virtual void visit(CompilationUnit&) = 0;
    virtual void visit(ModuleDecl&)      = 0;
    virtual void visit(ImportDecl&)      = 0;
    virtual void visit(TypeExpr&)        = 0;
    virtual void visit(AliasDecl&)       = 0;
    virtual void visit(DomainDecl&)      = 0;
    virtual void visit(EnumDecl&)        = 0;
    virtual void visit(EnumItem&)        = 0;
    virtual void visit(FieldDecl&)       = 0;
    virtual void visit(RecordDecl&)      = 0;
    virtual void visit(ObjectDecl&)      = 0;
    virtual void visit(InstanceType&)    = 0;
    virtual void visit(InstanceDecl&)    = 0;
    virtual void visit(CmdDecl&)         = 0;
    virtual void visit(CmdDef&)          = 0;
    virtual void visit(IntrinsicDecl&)   = 0;
    virtual void visit(ClassDecl&)       = 0;
    virtual void visit(ProgramDecl&)     = 0;
    virtual void visit(TestDecl&)        = 0;
    virtual void visit(CmdBody&)         = 0;
    virtual void visit(CallGroup&)       = 0;
    virtual void visit(CallInvoke&)      = 0;
    virtual void visit(CallAssignment&)  = 0;
    virtual void visit(CallExpression&)  = 0;
    virtual void visit(Block&)           = 0;
    virtual void visit(Literal&)         = 0;
    virtual void visit(IdentifierExpr&)  = 0;
    virtual void visit(SubcallExpr&)     = 0;
    virtual void visit(CallQuote&)       = 0;
    virtual void visit(CmdLiteral&)      = 0;
    virtual void visit(CallParameter&)   = 0;
};

// ---- Base node ----
struct AstNode {
    size_t line = 0;
    size_t col  = 0;
    virtual ~AstNode() = default;
    virtual void accept(Visitor&) = 0;
};
using spAstNode = std::shared_ptr<AstNode>;

// ---- TypeExpr ----
// Covers TYPE_EXPR, TYPE_EXPR_DOMAIN, TYPE_EXPR_CMD, TYPE_EXPR_PTR, TYPE_EXPR_RANGE,
// TYPE_NAME_Q, TYPEDEF_NAME_Q and their argument/parameter sub-productions.
struct TypeExpr : AstNode {
    enum class Kind    { Named, Pointer, Range, Command, Domain };
    enum class CmdKind { NoFail, MayFail, Fails };

    Kind    kind    = Kind::Named;
    CmdKind cmdKind = CmdKind::NoFail;

    std::string                          typeName;   // for Named/Domain: qualified text
    std::vector<std::shared_ptr<TypeExpr>> typeArgs; // type arguments / parameters
    int                                  ptrDepth = 0;
    std::shared_ptr<TypeExpr>            inner;      // Pointer inner type / Range element
    std::string                          rangeSize;  // empty = unbounded
    std::vector<std::shared_ptr<TypeExpr>> cmdArgs;  // Command arg types
    bool                                 writeable = false;

    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Module / Import ----
struct ModuleDecl : AstNode {
    std::string name;   // qualified typename text
    void accept(Visitor& v) override { v.visit(*this); }
};

struct ImportDecl : AstNode {
    enum class Kind { File, Standard };
    Kind        kind      = Kind::Standard;
    std::string path;       // file path string literal (Kind::File)
    std::string qualifier;  // optional prefix e.g. "Std" in "Std:Core" (Kind::Standard)
    std::string name;       // module name (Kind::Standard)
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Enum ----
struct EnumItem : AstNode {
    std::string name;
    std::string value;   // literal text
    void accept(Visitor& v) override { v.visit(*this); }
};

struct EnumDecl : AstNode {
    std::string enumName;
    std::string typeName;   // optional second typename (empty if absent)
    std::vector<std::shared_ptr<EnumItem>> items;
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Record / Object fields (shared FieldDecl) ----
struct FieldDecl : AstNode {
    std::shared_ptr<TypeExpr> type;
    std::string               name;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct RecordDecl : AstNode {
    std::string                             name;
    std::vector<std::shared_ptr<FieldDecl>> fields;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct ObjectDecl : AstNode {
    std::string                             name;
    std::vector<std::shared_ptr<FieldDecl>> fields;
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Instance ----
struct InstanceType : AstNode {
    std::string typeName;
    std::string delegate;   // identifier inside (...), empty if absent
    void accept(Visitor& v) override { v.visit(*this); }
};

struct InstanceDecl : AstNode {
    std::string                                name;
    std::vector<std::shared_ptr<InstanceType>> types;
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Alias / Domain ----
struct AliasDecl : AstNode {
    std::string               name;
    std::shared_ptr<TypeExpr> type;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct DomainDecl : AstNode {
    std::string               name;
    std::shared_ptr<TypeExpr> parent;
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Command parameters / receivers / signature (plain structs, not nodes) ----
struct CmdParam {
    std::shared_ptr<TypeExpr> type;
    std::string               name;
    bool                      isTypeVar   = false;
    std::string               typeVarName;   // the T in (T : SomeType)
};

struct CmdReceiver {
    std::shared_ptr<TypeExpr> type;
    std::string               name;
};

struct CmdSignature {
    enum class Kind { Regular, VCommand, Constructor, Destructor, FailHandler };
    Kind                     kind    = Kind::Regular;
    std::string              name;
    bool                     mayFail = false;
    bool                     fails   = false;
    std::vector<CmdReceiver> receivers;
    std::vector<CmdParam>    params;
    std::vector<CmdParam>    implicitParams;
    std::string              returnVal;
};

// ---- Command declarations / definitions ----
struct CmdDecl : AstNode {
    CmdSignature signature;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct IntrinsicDecl : AstNode {
    CmdSignature signature;
    void accept(Visitor& v) override { v.visit(*this); }
};

// Forward-declared here; defined below after statement nodes.
struct CmdBody;

struct CmdDef : AstNode {
    CmdSignature              signature;
    std::shared_ptr<CmdBody>  body;   // nullptr when body is DEF_CMD_EMPTY (_)
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Class ----
struct ClassDecl : AstNode {
    std::string              name;
    std::vector<spAstNode>   members;   // CmdDecl or CmdDef nodes
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Program / Test ----
struct ProgramDecl : AstNode {
    spAstNode entryPoint;   // CallInvoke node
    void accept(Visitor& v) override { v.visit(*this); }
};

struct TestDecl : AstNode {
    std::string label;
    spAstNode   body;   // CallGroup node
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Statement / expression nodes ----

// Literal covers DECIMAL, HEXNUMBER, BINARY, NUMBER, STRING
struct Literal : AstNode {
    std::string text;
    void accept(Visitor& v) override { v.visit(*this); }
};

// Covers IDENTIFIER (possibly qualified) and ALLOC_IDENTIFIER (#ident)
struct IdentifierExpr : AstNode {
    std::string text;
    bool        isAlloc = false;   // true when prefixed with #
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_PARAMETER: either empty (_) or an expression
struct CallParameter : AstNode {
    bool      isEmpty = false;
    spAstNode expr;   // SubcallExpr or IdentifierExpr; null when isEmpty
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_INVOKE: vcommand, constructor, or regular command call
struct CallInvoke : AstNode {
    enum class Kind { Command, Constructor, VCommand };
    Kind                               kind = Kind::Command;
    std::string                        target;     // command name / type name
    std::string                        subquote;   // non-empty when target is a subquote
    std::vector<std::string>           receivers;  // VCommand receiver identifiers
    std::vector<std::shared_ptr<CallParameter>> params;
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_ASSIGNMENT: target <- expr [| expr]* [op rhs]*
struct CallAssignment : AstNode {
    std::shared_ptr<IdentifierExpr>    target;
    std::vector<spAstNode>             exprs;    // SubcallExpr chain (pipe-separated)
    std::vector<std::pair<std::string, spAstNode>> postOps; // operator + rhs
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_EXPRESSION: term op term [op term]*  (requires at least one operator)
struct CallExpression : AstNode {
    std::vector<spAstNode>   terms;     // alternating: term, op-as-IdentifierExpr, term ...
    void accept(Visitor& v) override { v.visit(*this); }
};

// SUBCALL_EXPRESSION: optional-operator expression used inside assignments/params
struct SubcallExpr : AstNode {
    std::vector<spAstNode>   terms;     // same layout as CallExpression
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_QUOTE / block quotes
struct CallQuote : AstNode {
    enum class Kind { Subquote, NoFail, MayFail, Fails };
    Kind      kind = Kind::Subquote;
    spAstNode body;   // CallInvoke (subquote) or CallGroup (block); null if empty
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_CMD_LITERAL: inline command lambda  :<  ?<  !<
struct CmdLiteral : AstNode {
    TypeExpr::CmdKind            cmdKind = TypeExpr::CmdKind::NoFail;
    std::vector<CmdParam>        params;
    spAstNode                    body;   // CallGroup
    void accept(Visitor& v) override { v.visit(*this); }
};

// BLOCK: BLOCK_HEADER + CALL_GROUP
struct Block : AstNode {
    enum class Kind {
        DoWhen, DoWhenMulti, DoWhenFail, DoElse, DoUnless,
        DoBlock, DoRewind, DoRecover, DoRecoverSpec,
        OnExit, OnExitFail
    };
    Kind        kind = Kind::DoBlock;
    std::string recoverType;   // DoRecoverSpec: type name
    std::string recoverName;   // DoRecoverSpec: bound identifier
    spAstNode   body;          // CallGroup
    void accept(Visitor& v) override { v.visit(*this); }
};

// CALL_GROUP: sequence of statements
struct CallGroup : AstNode {
    std::vector<spAstNode> statements;   // CallAssignment | CallExpression | CallInvoke | Block
    void accept(Visitor& v) override { v.visit(*this); }
};

// DEF_CMD_BODY: = _ | = CALL_GROUP
struct CmdBody : AstNode {
    bool                      isEmpty = false;
    std::shared_ptr<CallGroup> group;   // null when isEmpty
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- Top-level compilation unit ----
struct CompilationUnit : AstNode {
    std::shared_ptr<ModuleDecl>              module;       // nullable
    std::vector<std::shared_ptr<ImportDecl>> imports;
    std::vector<spAstNode>                   definitions;  // any top-level Decl node
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- DefaultVisitor: walks the tree by default ----
// Placed after all node definitions so inline method bodies can reference member fields.
struct DefaultVisitor : Visitor {
    // Leaf nodes (no AstNode children)
    void visit(ModuleDecl&)      override {}
    void visit(ImportDecl&)      override {}
    void visit(EnumItem&)        override {}
    void visit(InstanceType&)    override {}
    void visit(Literal&)         override {}
    void visit(IdentifierExpr&)  override {}

    // Nodes whose only AstNode children are TypeExprs
    void visit(TypeExpr& n) override {
        if (n.inner) n.inner->accept(*this);
        for (auto& a : n.typeArgs) a->accept(*this);
        for (auto& a : n.cmdArgs)  a->accept(*this);
    }
    void visit(FieldDecl& n)  override { if (n.type) n.type->accept(*this); }
    void visit(AliasDecl& n)  override { if (n.type) n.type->accept(*this); }
    void visit(DomainDecl& n) override { if (n.parent) n.parent->accept(*this); }

    // Nodes with list children
    void visit(EnumDecl& n)     override { for (auto& i : n.items)      i->accept(*this); }
    void visit(RecordDecl& n)   override { for (auto& f : n.fields)     f->accept(*this); }
    void visit(ObjectDecl& n)   override { for (auto& f : n.fields)     f->accept(*this); }
    void visit(InstanceDecl& n) override { for (auto& t : n.types)      t->accept(*this); }
    void visit(ClassDecl& n)    override { for (auto& m : n.members)    m->accept(*this); }
    void visit(CallGroup& n)    override { for (auto& s : n.statements) s->accept(*this); }

    // Command declarations: walk TypeExprs in signature receivers/params/implicitParams
    void visit(CmdDecl& n)       override { visitSignature(*this, n.signature); }
    void visit(IntrinsicDecl& n) override { visitSignature(*this, n.signature); }
    void visit(CmdDef& n)        override {
        visitSignature(*this, n.signature);
        if (n.body) n.body->accept(*this);
    }

    // Nodes with a single body child
    void visit(CmdBody& n)       override { if (n.group)       n.group->accept(*this); }
    void visit(ProgramDecl& n)   override { if (n.entryPoint)  n.entryPoint->accept(*this); }
    void visit(TestDecl& n)      override { if (n.body)        n.body->accept(*this); }
    void visit(Block& n)         override { if (n.body)        n.body->accept(*this); }
    void visit(CallQuote& n)     override { if (n.body)        n.body->accept(*this); }
    void visit(CmdLiteral& n)    override { if (n.body)        n.body->accept(*this); }
    void visit(CallParameter& n) override { if (n.expr)        n.expr->accept(*this); }

    // Expression nodes
    void visit(CallInvoke& n)    override { for (auto& p : n.params) p->accept(*this); }
    void visit(CallExpression& n) override { for (auto& t : n.terms) t->accept(*this); }
    void visit(SubcallExpr& n)   override { for (auto& t : n.terms)  t->accept(*this); }
    void visit(CallAssignment& n) override {
        if (n.target) n.target->accept(*this);
        for (auto& e : n.exprs) e->accept(*this);
        for (auto& [op, rhs] : n.postOps) if (rhs) rhs->accept(*this);
    }

    // Top-level unit
    void visit(CompilationUnit& n) override {
        if (n.module) n.module->accept(*this);
        for (auto& i : n.imports)     i->accept(*this);
        for (auto& d : n.definitions) d->accept(*this);
    }

protected:
    // Walk TypeExprs in a CmdSignature's receivers, params, and implicitParams
    static void visitSignature(Visitor& v, const CmdSignature& sig) {
        for (auto& r : sig.receivers)      { if (r.type) r.type->accept(v); }
        for (auto& p : sig.params)         { if (p.type) p.type->accept(v); }
        for (auto& p : sig.implicitParams) { if (p.type) p.type->accept(v); }
    }
};

} // namespace basis

#endif // AST_H


