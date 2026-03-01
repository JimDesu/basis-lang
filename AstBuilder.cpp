#include "AstBuilder.h"
#include "Productions.h"

using namespace basis;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

// Return the first token text reachable by descending spDown then spNext.
std::string firstTokenText(const ParseTree* pt) {
    if (!pt) return {};
    if (pt->pToken) return pt->pToken->text;
    return firstTokenText(pt->spDown.get());
}

// Copy source location from the first terminal token found under pt.
void setLoc(AstNode& node, const ParseTree* pt) {
    for (const ParseTree* n = pt; n != nullptr; n = n->spDown.get()) {
        if (n->pToken) {
            node.line = n->pToken->lineNumber;
            node.col = n->pToken->columnNumber;
            return;
        }
    }
}

// Collect all token texts in the spNext chain (used for qualified names).
// Handles both flat token chains and QUALIFIED_TYPENAME group nodes.
std::string collectQualifiedName(const ParseTree* pt) {
    std::string result;
    for (const ParseTree* n = pt; n != nullptr; n = n->spNext.get()) {
        if (n->pToken) {
            if (!result.empty() && n->production != Production::DCOLON) result += "::";
            if (n->production != Production::DCOLON) result += n->pToken->text;
        } else if (n->production == Production::QUALIFIED_TYPENAME) {
            // Recurse into QUALIFIED_TYPENAME children
            if (!result.empty()) result += "::";
            result += collectQualifiedName(n->spDown.get());
        }
    }
    return result;
}

// Resolve a TYPENAME node (which may be QUALIFIED_TYPENAME or a plain TYPENAME match)
// into a string.
std::string resolveTypeName(const ParseTree* pt) {
    if (!pt) return {};
    if (pt->production == Production::QUALIFIED_TYPENAME)
        return collectQualifiedName(pt->spDown.get());
    if (pt->pToken) return pt->pToken->text;
    return firstTokenText(pt);
}

// Forward declarations of all builders
spAstNode build(const ParseTree* pt);
std::shared_ptr<TypeExpr>       buildTypeExpr(const ParseTree* pt);
std::shared_ptr<ModuleDecl>     buildModuleDecl(const ParseTree* pt);
std::shared_ptr<ImportDecl>     buildImportDecl(const ParseTree* pt);
std::shared_ptr<EnumDecl>       buildEnumDecl(const ParseTree* pt);
std::shared_ptr<RecordDecl>     buildRecordDecl(const ParseTree* pt);
std::shared_ptr<ObjectDecl>     buildObjectDecl(const ParseTree* pt);
std::shared_ptr<InstanceDecl>   buildInstanceDecl(const ParseTree* pt);
std::shared_ptr<AliasDecl>      buildAliasDecl(const ParseTree* pt);
std::shared_ptr<DomainDecl>     buildDomainDecl(const ParseTree* pt);
CmdSignature                    buildCmdSignature(const ParseTree* pt);
std::shared_ptr<CmdDecl>        buildCmdDecl(const ParseTree* pt);
std::shared_ptr<CmdDef>         buildCmdDef(const ParseTree* pt);
std::shared_ptr<IntrinsicDecl>  buildIntrinsicDecl(const ParseTree* pt);
std::shared_ptr<ClassDecl>      buildClassDecl(const ParseTree* pt);
std::shared_ptr<ProgramDecl>    buildProgramDecl(const ParseTree* pt);
std::shared_ptr<TestDecl>       buildTestDecl(const ParseTree* pt);
std::shared_ptr<CmdBody>        buildCmdBody(const ParseTree* pt);
std::shared_ptr<CallGroup>      buildCallGroup(const ParseTree* pt);
spAstNode                       buildCallInvoke(const ParseTree* pt);
spAstNode                       buildCallAssignment(const ParseTree* pt);
spAstNode                       buildCallExpression(const ParseTree* pt);
spAstNode                       buildSubcallExpr(const ParseTree* pt);
spAstNode                       buildBlock(const ParseTree* pt);
std::shared_ptr<CallParameter>  buildCallParameter(const ParseTree* pt);
spAstNode                       buildCallQuote(const ParseTree* pt);
spAstNode                       buildCmdLiteral(const ParseTree* pt);
spAstNode                       buildLiteral(const ParseTree* pt);
spAstNode                       buildIdentifierExpr(const ParseTree* pt);

// ---------------------------------------------------------------------------
// TypeExpr builders
// ---------------------------------------------------------------------------

// Build a TypeExpr from a TYPE_NAME_Q or TYPEDEF_NAME_Q node.
// Children: TYPENAME [TYPE_NAME_ARGS | TYPEDEF_PARMS]
std::shared_ptr<TypeExpr> buildNamedTypeExpr(const ParseTree* pt, TypeExpr::Kind kind) {
    auto node = std::make_shared<TypeExpr>();
    node->kind = kind;
    setLoc(*node, pt);
    const ParseTree* child = pt->spDown.get();
    // Collect the typename (may be QUALIFIED_TYPENAME with spNext chain, or single TYPENAME)
    if (child && child->production == Production::QUALIFIED_TYPENAME) {
        node->typeName = collectQualifiedName(child->spDown.get());
        child = child->spNext.get();
    } else if (child && (child->production == Production::TYPENAME)) {
        node->typeName = child->pToken ? child->pToken->text : firstTokenText(child);
        child = child->spNext.get();
    }
    // Optional type args / typedef params
    while (child) {
        switch (child->production) {
            case Production::TYPE_NAME_ARGS:
            case Production::TYPEDEF_PARMS:
                for (const ParseTree* arg = child->spDown.get(); arg; arg = arg->spNext.get()) {
                    switch (arg->production) {
                        case Production::TYPE_ARG_TYPE:
                        case Production::TYPEDEF_PARM_TYPE:
                            node->typeArgs.push_back(buildTypeExpr(arg->spDown.get()));
                            break;
                        case Production::TYPE_ARG_VALUE:
                        case Production::TYPEDEF_PARM_VALUE: {
                            // value arg: represent as a Named TypeExpr with the text
                            auto va = std::make_shared<TypeExpr>();
                            va->kind = TypeExpr::Kind::Named;
                            va->typeName = firstTokenText(arg);
                            node->typeArgs.push_back(va);
                            break;
                        }
                        case Production::LBRACKET: case Production::RBRACKET:
                        case Production::COMMA:
                            break; // structural punctuation — no-op
                        default:
                            break; // any other structural node — no-op
                    }
                }
                break;
            case Production::LBRACKET: case Production::RBRACKET:
            case Production::COMMA:
                break; // structural punctuation — no-op
            default:
                break;
        }
        child = child->spNext.get();
    }
    return node;
}

// Build a pointer TypeExpr from TYPE_EXPR_PTR + inner TYPE_EXPR
std::shared_ptr<TypeExpr> buildPtrTypeExpr(const ParseTree* ptPtr, const ParseTree* ptInner) {
    auto node = std::make_shared<TypeExpr>();
    node->kind = TypeExpr::Kind::Pointer;
    setLoc(*node, ptPtr);
    // Count carats in the PTR node's children
    for (const ParseTree* c = ptPtr->spDown.get(); c; c = c->spNext.get())
        if (c->production == Production::CARAT) node->ptrDepth++;
    if (ptInner) node->inner = buildTypeExpr(ptInner);
    return node;
}

// Build a range TypeExpr from TYPE_EXPR_RANGE[_FIXED] + optional inner
std::shared_ptr<TypeExpr> buildRangeTypeExpr(const ParseTree* ptRange, const ParseTree* ptInner) {
    auto node = std::make_shared<TypeExpr>();
    node->kind = TypeExpr::Kind::Range;
    setLoc(*node, ptRange);
    for (const ParseTree* c = ptRange->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::IDENTIFIER: case Production::NUMBER:
                if (c->pToken) node->rangeSize = c->pToken->text;
                break;
            case Production::LBRACKET: case Production::RBRACKET:
                break; // structural — no-op
            default:
                break;
        }
    }
    if (ptInner) node->inner = buildTypeExpr(ptInner);
    return node;
}

// ---------------------------------------------------------------------------
// buildTypeExpr — dispatches on the production of the node passed in
// ---------------------------------------------------------------------------
std::shared_ptr<TypeExpr> buildTypeExpr(const ParseTree* pt) {
    if (!pt) return nullptr;
    switch (pt->production) {
        // Named type in usage context
        case Production::TYPE_NAME_Q:
            return buildNamedTypeExpr(pt, TypeExpr::Kind::Named);
        // Named type in definition context (with optional typedef params)
        case Production::TYPEDEF_NAME_Q:
            return buildNamedTypeExpr(pt, TypeExpr::Kind::Named);
        // Domain-restricted type expression
        case Production::TYPE_EXPR_DOMAIN:
            // spDown is either TYPE_NAME_Q or TYPE_EXPR_RANGE_FIXED
            return buildTypeExpr(pt->spDown.get());
        // General type expression wrapper
        case Production::TYPE_EXPR: {
            const ParseTree* child = pt->spDown.get();
            if (!child) return nullptr;
            switch (child->production) {
                case Production::TYPEDEF_NAME_Q:
                    return buildNamedTypeExpr(child, TypeExpr::Kind::Named);
                case Production::TYPE_EXPR_CMD:
                    return buildTypeExpr(child);
                case Production::TYPE_EXPR_PTR: {
                    // next sibling is the inner TYPE_EXPR
                    return buildPtrTypeExpr(child, child->spNext.get());
                }
                case Production::TYPE_EXPR_RANGE:
                case Production::TYPE_EXPR_RANGE_FIXED: {
                    return buildRangeTypeExpr(child, child->spNext.get());
                }
                // Structural nodes that should not appear here — no-op
                case Production::LBRACKET: case Production::RBRACKET:
                case Production::COMMA: case Production::COLON:
                    return nullptr;
                default:
                    return nullptr;
            }
        }
        case Production::TYPE_EXPR_PTR:
            return buildPtrTypeExpr(pt, pt->spNext.get());
        case Production::TYPE_EXPR_RANGE:
        case Production::TYPE_EXPR_RANGE_FIXED:
            return buildRangeTypeExpr(pt, pt->spNext.get());
        // Command type expression  :<  ?<  !<
        case Production::TYPE_EXPR_CMD: {
            auto node = std::make_shared<TypeExpr>();
            node->kind = TypeExpr::Kind::Command;
            setLoc(*node, pt);
            for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::COLANGLE:  node->cmdKind = TypeExpr::CmdKind::NoFail;  break;
                    case Production::QLANGLE:   node->cmdKind = TypeExpr::CmdKind::MayFail; break;
                    case Production::BANGLANGLE:node->cmdKind = TypeExpr::CmdKind::Fails;   break;
                    case Production::TYPE_CMDEXPR_ARG:
                        node->cmdArgs.push_back(buildTypeExpr(c));
                        break;
                    case Production::RANGLE: case Production::COMMA:
                        break; // structural — no-op
                    default:
                        break; // any other structural node — no-op
                }
            }
            return node;
        }
        // TYPE_CMDEXPR_ARG: like TYPE_EXPR but with optional writeable marker
        case Production::TYPE_CMDEXPR_ARG: {
            auto node = std::make_shared<TypeExpr>();
            node->kind = TypeExpr::Kind::Named;
            setLoc(*node, pt);
            for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::TYPE_NAME_Q:
                        node = buildNamedTypeExpr(c, TypeExpr::Kind::Named);
                        break;
                    case Production::TYPE_EXPR_CMD:
                        node = buildTypeExpr(c);
                        break;
                    case Production::TYPE_EXPR_PTR:
                        node = buildPtrTypeExpr(c, c->spNext.get());
                        break;
                    case Production::TYPE_EXPR_RANGE:
                    case Production::TYPE_EXPR_RANGE_FIXED:
                        node = buildRangeTypeExpr(c, c->spNext.get());
                        break;
                    case Production::APOSTROPHE:
                        node->writeable = true;
                        break;
                    // Structural punctuation — no-op
                    case Production::COMMA: case Production::RANGLE:
                        break;
                    default:
                        break;
                }
            }
            return node;
        }
        // Structural / terminal nodes that are not type expressions — no-op
        case Production::LBRACKET: case Production::RBRACKET:
        case Production::COMMA:    case Production::COLON:
        case Production::TYPENAME: case Production::QUALIFIED_TYPENAME:
            return nullptr;
        default:
            return nullptr;
    }
}

// ---------------------------------------------------------------------------
// CmdParam / CmdReceiver helpers
// ---------------------------------------------------------------------------
CmdParam buildOneCmdParam(const ParseTree* pt) {
    // pt is DEF_CMD_PARM: spDown -> DEF_CMD_PARM_TYPE, spNext -> DEF_CMD_PARM_NAME
    CmdParam p;
    const ParseTree* typeNode = pt->spDown.get();
    const ParseTree* nameNode = typeNode ? typeNode->spNext.get() : nullptr;
    if (typeNode && typeNode->production == Production::DEF_CMD_PARM_TYPE) {
        const ParseTree* inner = typeNode->spDown.get();
        if (inner && inner->production == Production::DEF_CMD_PARMTYPE_VAR) {
            // (T : TypeExpr) form
            p.isTypeVar = true;
            const ParseTree* c = inner->spDown.get();
            // LPAREN TYPENAME COLON DEF_CMD_PARMTYPE_NAME RPAREN
            for (; c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::TYPENAME:
                        if (c->pToken) p.typeVarName = c->pToken->text;
                        break;
                    case Production::DEF_CMD_PARMTYPE_NAME:
                        p.type = buildTypeExpr(c->spDown.get());
                        break;
                    case Production::LPAREN: case Production::RPAREN: case Production::COLON:
                        break; // structural — no-op
                    default:
                        break;
                }
            }
        } else if (inner && inner->production == Production::DEF_CMD_PARMTYPE_NAME) {
            p.type = buildTypeExpr(inner->spDown.get());
        }
    }
    if (nameNode && nameNode->production == Production::DEF_CMD_PARM_NAME && nameNode->pToken)
        p.name = nameNode->pToken->text;
    return p;
}

CmdReceiver buildOneCmdReceiver(const ParseTree* pt) {
    // pt is DEF_CMD_RECEIVER: spDown -> DEF_CMD_PARMTYPE_NAME, spNext -> DEF_CMD_PARM_NAME
    CmdReceiver r;
    const ParseTree* typeNode = pt->spDown.get();
    const ParseTree* nameNode = typeNode ? typeNode->spNext.get() : nullptr;
    if (typeNode && typeNode->production == Production::DEF_CMD_PARMTYPE_NAME)
        r.type = buildTypeExpr(typeNode->spDown.get());
    if (nameNode && nameNode->production == Production::DEF_CMD_PARM_NAME && nameNode->pToken)
        r.name = nameNode->pToken->text;
    return r;
}

// Collect comma-separated DEF_CMD_PARM nodes from a sibling chain
std::vector<CmdParam> collectParams(const ParseTree* pt) {
    std::vector<CmdParam> result;
    for (const ParseTree* n = pt; n; n = n->spNext.get()) {
        switch (n->production) {
            case Production::DEF_CMD_PARM:
                result.push_back(buildOneCmdParam(n));
                break;
            case Production::COMMA: case Production::COLON: case Production::SLASH:
                break; // structural — no-op
            default:
                break;
        }
    }
    return result;
}

// Collect comma-separated DEF_CMD_RECEIVER nodes from a sibling chain
std::vector<CmdReceiver> collectReceivers(const ParseTree* pt) {
    std::vector<CmdReceiver> result;
    for (const ParseTree* n = pt; n; n = n->spNext.get()) {
        switch (n->production) {
            case Production::DEF_CMD_RECEIVER:
                result.push_back(buildOneCmdReceiver(n));
                break;
            case Production::COMMA: case Production::LPAREN: case Production::RPAREN:
            case Production::DCOLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// buildCmdSignature — shared by DEF_CMD, DEF_CMD_DECL, DEF_CMD_INTRINSIC
// pt->spDown is the first child after the keyword token
// ---------------------------------------------------------------------------
CmdSignature buildCmdSignature(const ParseTree* pt) {
    CmdSignature sig;
    const ParseTree* c = pt ? pt->spDown.get() : nullptr;
    if (!c) return sig;

    // Skip keyword token (DECLARE, COMMAND, INTRINSIC) — first child of exclusiveGroup
    if (c->production == Production::DECLARE ||
        c->production == Production::COMMAND ||
        c->production == Production::INTRINSIC)
        c = c->spNext.get();
    if (!c) return sig;

    // Determine kind from first meaningful child
    switch (c->production) {
        case Production::ON_EXIT:      // @ DEF_CMD_RECEIVER
            sig.kind = CmdSignature::Kind::Destructor;
            c = c->spNext.get();
            if (c && c->production == Production::DEF_CMD_RECEIVER)
                sig.receivers.push_back(buildOneCmdReceiver(c));
            return sig;

        case Production::ON_EXIT_FAIL: // @! DEF_CMD_RECEIVER
            sig.kind = CmdSignature::Kind::FailHandler;
            c = c->spNext.get();
            if (c && c->production == Production::DEF_CMD_RECEIVER)
                sig.receivers.push_back(buildOneCmdReceiver(c));
            return sig;

        case Production::DEF_CMD_RECEIVERS: { // VCommand: (recvs):: name vparms imparms
            sig.kind = CmdSignature::Kind::VCommand;
            sig.receivers = collectReceivers(c->spDown.get());
            c = c->spNext.get();
            // DEF_CMD_NAME_SPEC
            if (c && c->production == Production::DEF_CMD_NAME_SPEC) {
                for (const ParseTree* ns = c->spDown.get(); ns; ns = ns->spNext.get()) {
                    switch (ns->production) {
                        case Production::DEF_CMD_MAYFAIL: sig.mayFail = true; break;
                        case Production::DEF_CMD_FAILS:   sig.fails   = true; break;
                        case Production::DEF_CMD_NAME:
                            if (ns->pToken) sig.name = ns->pToken->text;
                            break;
                        default: break;
                    }
                }
                c = c->spNext.get();
            }
            // DEF_CMD_PARMS (optional) and DEF_CMD_RETVAL
            for (; c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::DEF_CMD_PARMS:
                        sig.params = collectParams(c->spDown.get());
                        for (const ParseTree* n = c->spDown.get(); n; n = n->spNext.get())
                            if (n->production == Production::DEF_CMD_RETVAL)
                                sig.returnVal = firstTokenText(n->spDown.get());
                        break;
                    case Production::DEF_CMD_RETVAL:
                        sig.returnVal = firstTokenText(c->spDown.get());
                        break;
                    case Production::DEF_CMD_IMPARMS:
                        sig.implicitParams = collectParams(c->spDown.get());
                        break;
                    default: break;
                }
            }
            return sig;
        }

        case Production::DEF_CMD_RECEIVER: {
            // Could be constructor: RECEIVER : PARM, ...
            // Peek at next sibling
            const ParseTree* next = c->spNext.get();
            if (next && next->production == Production::COLON) {
                sig.kind = CmdSignature::Kind::Constructor;
                sig.receivers.push_back(buildOneCmdReceiver(c));
                sig.params = collectParams(next->spNext.get());
            } else {
                // Treat as regular with receiver (shouldn't normally occur at top level)
                sig.kind = CmdSignature::Kind::Regular;
                sig.receivers.push_back(buildOneCmdReceiver(c));
            }
            return sig;
        }

        case Production::DEF_CMD_NAME_SPEC: { // Regular command
            sig.kind = CmdSignature::Kind::Regular;
            for (const ParseTree* ns = c->spDown.get(); ns; ns = ns->spNext.get()) {
                switch (ns->production) {
                    case Production::DEF_CMD_MAYFAIL: sig.mayFail = true; break;
                    case Production::DEF_CMD_FAILS:   sig.fails   = true; break;
                    case Production::DEF_CMD_NAME:
                        if (ns->pToken) sig.name = ns->pToken->text;
                        break;
                    default: break;
                }
            }
            c = c->spNext.get();
            for (; c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::DEF_CMD_PARMS:
                        // DEF_CMD_RETVAL is a child of DEF_CMD_PARMS, not a sibling
                        sig.params = collectParams(c->spDown.get());
                        for (const ParseTree* n = c->spDown.get(); n; n = n->spNext.get())
                            if (n->production == Production::DEF_CMD_RETVAL)
                                sig.returnVal = firstTokenText(n->spDown.get());
                        break;
                    case Production::DEF_CMD_RETVAL:
                        sig.returnVal = firstTokenText(c->spDown.get());
                        break;
                    case Production::DEF_CMD_IMPARMS:
                        sig.implicitParams = collectParams(c->spDown.get());
                        break;
                    default: break;
                }
            }
            return sig;
        }

        default:
            return sig;
    }
}

// ---------------------------------------------------------------------------
// Top-level declaration builders
// ---------------------------------------------------------------------------

std::shared_ptr<ModuleDecl> buildModuleDecl(const ParseTree* pt) {
    auto node = std::make_shared<ModuleDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_MODULE_NAME:
                node->name = resolveTypeName(c->spDown.get());
                break;
            case Production::MODULE:
                break; // keyword — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<ImportDecl> buildImportDecl(const ParseTree* pt) {
    auto node = std::make_shared<ImportDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_IMPORT_FILE:
                node->kind = ImportDecl::Kind::File;
                node->path = firstTokenText(c);
                break;
            case Production::DEF_IMPORT_STANDARD: {
                node->kind = ImportDecl::Kind::Standard;
                // Children: [TYPENAME COLON] TYPENAME
                const ParseTree* ic = c->spDown.get();
                if (ic && ic->spNext.get() &&
                    ic->spNext.get()->production == Production::COLON) {
                    // qualifier : name
                    if (ic->pToken) node->qualifier = ic->pToken->text;
                    ic = ic->spNext.get()->spNext.get(); // skip COLON
                }
                if (ic && ic->pToken) node->name = ic->pToken->text;
                break;
            }
            case Production::IMPORT:
                break; // keyword — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<EnumDecl> buildEnumDecl(const ParseTree* pt) {
    auto node = std::make_shared<EnumDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_ENUM_NAME1:
                if (c->pToken) node->name1 = c->pToken->text;
                break;
            case Production::DEF_ENUM_NAME2:
                if (c->pToken) node->name2 = c->pToken->text;
                break;
            case Production::DEF_ENUM_ITEM_NAME: {
                // Sibling chain: ITEM_NAME EQUALS LITERAL [COMMA ITEM_NAME EQUALS LITERAL ...]
                auto item = std::make_shared<EnumItem>();
                setLoc(*item, c);
                if (c->pToken) item->name = c->pToken->text;
                // value is two siblings ahead (skip EQUALS)
                const ParseTree* eq  = c->spNext.get();
                const ParseTree* lit = eq ? eq->spNext.get() : nullptr;
                if (lit) item->value = firstTokenText(lit);
                node->items.push_back(item);
                break;
            }
            case Production::ENUMERATION: case Production::COLON:
            case Production::EQUALS:      case Production::COMMA:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<FieldDecl> buildFieldDecl(const ParseTree* pt) {
    auto node = std::make_shared<FieldDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_RECORD_FIELD_DOMAIN:
            case Production::DEF_OBJECT_FIELD_TYPE:
                node->type = buildTypeExpr(c->spDown.get());
                break;
            case Production::DEF_RECORD_FIELD_NAME:
            case Production::DEF_OBJECT_FIELD_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::COMMA:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<RecordDecl> buildRecordDecl(const ParseTree* pt) {
    auto node = std::make_shared<RecordDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_RECORD_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::DEF_RECORD_FIELDS:
                for (const ParseTree* f = c->spDown.get(); f; f = f->spNext.get()) {
                    switch (f->production) {
                        case Production::DEF_RECORD_FIELD:
                            node->fields.push_back(buildFieldDecl(f));
                            break;
                        case Production::COMMA:
                            break; // structural — no-op
                        default:
                            break;
                    }
                }
                break;
            case Production::RECORD: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<ObjectDecl> buildObjectDecl(const ParseTree* pt) {
    auto node = std::make_shared<ObjectDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_OBJECT_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::DEF_OBJECT_FIELDS:
                for (const ParseTree* f = c->spDown.get(); f; f = f->spNext.get()) {
                    switch (f->production) {
                        case Production::DEF_OBJECT_FIELD:
                            node->fields.push_back(buildFieldDecl(f));
                            break;
                        case Production::COMMA:
                            break; // structural — no-op
                        default:
                            break;
                    }
                }
                break;
            case Production::OBJECT: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<InstanceDecl> buildInstanceDecl(const ParseTree* pt) {
    auto node = std::make_shared<InstanceDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_INSTANCE_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::DEF_INSTANCE_TYPES:
                for (const ParseTree* t = c->spDown.get(); t; t = t->spNext.get()) {
                    switch (t->production) {
                        case Production::TYPENAME: {
                            auto it = std::make_shared<InstanceType>();
                            setLoc(*it, t);
                            if (t->pToken) it->typeName = t->pToken->text;
                            // optional delegate is the next sibling DEF_INSTANCE_DELEGATE
                            const ParseTree* del = t->spNext.get();
                            if (del && del->production == Production::DEF_INSTANCE_DELEGATE) {
                                it->delegate = firstTokenText(del);
                            }
                            node->types.push_back(it);
                            break;
                        }
                        case Production::DEF_INSTANCE_DELEGATE:
                        case Production::COMMA:
                            break; // structural — no-op
                        default:
                            break;
                    }
                }
                break;
            case Production::INSTANCE: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<AliasDecl> buildAliasDecl(const ParseTree* pt) {
    auto node = std::make_shared<AliasDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::TYPEDEF_NAME_Q:
                node->name = firstTokenText(c);
                break;
            case Production::TYPE_EXPR:
                node->type = buildTypeExpr(c);
                break;
            case Production::ALIAS: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<DomainDecl> buildDomainDecl(const ParseTree* pt) {
    auto node = std::make_shared<DomainDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_DOMAIN_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::DEF_DOMAIN_PARENT: {
                const ParseTree* inner = c->spDown.get();
                if (!inner) break;
                // DEF_DOMAIN_PARENT wraps any(TYPENAME, [...]) — TYPENAME is transparent,
                // so inner is either a QUALIFIED_TYPENAME group or a TYPENAME match node.
                if (inner->production == Production::TYPENAME ||
                    inner->production == Production::QUALIFIED_TYPENAME) {
                    auto te = std::make_shared<TypeExpr>();
                    te->kind = TypeExpr::Kind::Named;
                    te->typeName = resolveTypeName(inner);
                    node->parent = te;
                } else {
                    node->parent = buildTypeExpr(inner);
                }
                break;
            }
            case Production::DOMAIN: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<CmdDecl> buildCmdDecl(const ParseTree* pt) {
    auto node = std::make_shared<CmdDecl>();
    setLoc(*node, pt);
    node->signature = buildCmdSignature(pt);
    return node;
}

std::shared_ptr<CmdDef> buildCmdDef(const ParseTree* pt) {
    auto node = std::make_shared<CmdDef>();
    setLoc(*node, pt);
    node->signature = buildCmdSignature(pt);
    // Find DEF_CMD_BODY in the children
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        if (c->production == Production::DEF_CMD_BODY) {
            node->body = buildCmdBody(c);
            break;
        }
    }
    return node;
}

std::shared_ptr<IntrinsicDecl> buildIntrinsicDecl(const ParseTree* pt) {
    auto node = std::make_shared<IntrinsicDecl>();
    setLoc(*node, pt);
    node->signature = buildCmdSignature(pt);
    return node;
}

std::shared_ptr<ClassDecl> buildClassDecl(const ParseTree* pt) {
    auto node = std::make_shared<ClassDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_CLASS_NAME:
                node->name = firstTokenText(c);
                break;
            case Production::DEF_CLASS_CMDS:
                for (const ParseTree* m = c->spDown.get(); m; m = m->spNext.get()) {
                    switch (m->production) {
                        case Production::DEF_CMD_DECL:
                            node->members.push_back(buildCmdDecl(m));
                            break;
                        case Production::DEF_CMD:
                            node->members.push_back(buildCmdDef(m));
                            break;
                        default:
                            break; // structural — no-op
                    }
                }
                break;
            case Production::CLASS: case Production::COLON:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<ProgramDecl> buildProgramDecl(const ParseTree* pt) {
    auto node = std::make_shared<ProgramDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::CALL_COMMAND:
            case Production::CALL_CONSTRUCTOR:
            case Production::CALL_VCOMMAND:
                node->entryPoint = buildCallInvoke(c);
                break;
            case Production::PROGRAM: case Production::EQUALS:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

std::shared_ptr<TestDecl> buildTestDecl(const ParseTree* pt) {
    auto node = std::make_shared<TestDecl>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::STRING:
                if (c->pToken) node->label = c->pToken->text;
                break;
            case Production::CALL_GROUP:
                node->body = buildCallGroup(c);
                break;
            case Production::TEST: case Production::EQUALS:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

// ---------------------------------------------------------------------------
// Command body
// ---------------------------------------------------------------------------
std::shared_ptr<CmdBody> buildCmdBody(const ParseTree* pt) {
    auto node = std::make_shared<CmdBody>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_CMD_EMPTY:
                node->isEmpty = true;
                break;
            case Production::CALL_GROUP:
                node->group = buildCallGroup(c);
                break;
            case Production::EQUALS:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

// ---------------------------------------------------------------------------
// Statement / expression builders
// ---------------------------------------------------------------------------

std::shared_ptr<CallParameter> buildCallParameter(const ParseTree* pt) {
    auto node = std::make_shared<CallParameter>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::CALL_PARM_EMPTY:
                node->isEmpty = true;
                break;
            case Production::CALL_PARM_EXPR:
                // spDown is SUBCALL_EXPRESSION or IDENTIFIER
                node->expr = build(c->spDown.get());
                break;
            default:
                break; // structural — no-op
        }
    }
    return node;
}

spAstNode buildCallInvoke(const ParseTree* pt) {
    auto node = std::make_shared<CallInvoke>();
    setLoc(*node, pt);
    switch (pt->production) {
        case Production::CALL_COMMAND: {
            node->kind = CallInvoke::Kind::Command;
            for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::CALL_CMD_TARGET:
                        node->target = firstTokenText(c);
                        break;
                    case Production::CALL_PARAMETER:
                        node->params.push_back(buildCallParameter(c));
                        break;
                    case Production::COLON: case Production::COMMA:
                        break; // structural — no-op
                    default:
                        break;
                }
            }
            break;
        }
        case Production::CALL_CONSTRUCTOR: {
            node->kind = CallInvoke::Kind::Constructor;
            for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::TYPE_NAME_Q:
                        node->target = firstTokenText(c);
                        break;
                    case Production::CALL_PARAMETER:
                        node->params.push_back(buildCallParameter(c));
                        break;
                    case Production::COLON: case Production::COMMA:
                        break; // structural — no-op
                    default:
                        break;
                }
            }
            break;
        }
        case Production::CALL_VCOMMAND: {
            node->kind = CallInvoke::Kind::VCommand;
            for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
                switch (c->production) {
                    case Production::IDENTIFIER:
                        // Before DCOLON = receiver; after = command name
                        // IDENTIFIER is a group node (pToken=null), use firstTokenText
                        if (node->target.empty())
                            node->receivers.push_back(firstTokenText(c));
                        else
                            node->target = firstTokenText(c);
                        break;
                    case Production::DCOLON:
                        // Signals transition from receivers to command name
                        node->target = " "; // sentinel; overwritten by next IDENTIFIER
                        break;
                    case Production::CALL_PARAMETER:
                        node->params.push_back(buildCallParameter(c));
                        break;
                    case Production::LPAREN: case Production::RPAREN:
                    case Production::COLON:  case Production::COMMA:
                        break; // structural — no-op
                    default:
                        break;
                }
            }
            // trim sentinel
            if (node->target == " ") node->target.clear();
            break;
        }
        default:
            break;
    }
    return node;
}

spAstNode buildCallQuote(const ParseTree* pt) {
    auto node = std::make_shared<CallQuote>();
    setLoc(*node, pt);
    switch (pt->production) {
        case Production::CALL_BLOCK_NOFAIL:  node->kind = CallQuote::Kind::NoFail;   break;
        case Production::CALL_BLOCK_FAIL:    node->kind = CallQuote::Kind::Fails;    break;
        case Production::CALL_BLOCK_MAYFAIL: node->kind = CallQuote::Kind::MayFail;  break;
        default:                             node->kind = CallQuote::Kind::Subquote; break;
    }
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::CALL_GROUP:
                node->body = buildCallGroup(c);
                break;
            case Production::CALL_COMMAND:
            case Production::CALL_CONSTRUCTOR:
            case Production::CALL_VCOMMAND:
                node->body = buildCallInvoke(c);
                break;
            case Production::DEF_CMD_EMPTY:
                break; // empty body — no-op
            case Production::COLBRACE: case Production::BANGBRACE:
            case Production::QBRACE:   case Production::LBRACE:
            case Production::RBRACE:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

spAstNode buildCmdLiteral(const ParseTree* pt) {
    auto node = std::make_shared<CmdLiteral>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::COLANGLE:   node->cmdKind = TypeExpr::CmdKind::NoFail;  break;
            case Production::QLANGLE:    node->cmdKind = TypeExpr::CmdKind::MayFail; break;
            case Production::BANGLANGLE: node->cmdKind = TypeExpr::CmdKind::Fails;   break;
            case Production::DEF_CMD_PARM:
                node->params.push_back(buildOneCmdParam(c));
                break;
            case Production::CALL_GROUP:
                node->body = buildCallGroup(c);
                break;
            case Production::RANGLE: case Production::LBRACE: case Production::RBRACE:
            case Production::COMMA:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

spAstNode buildLiteral(const ParseTree* pt) {
    auto node = std::make_shared<Literal>();
    setLoc(*node, pt);
    if (pt->pToken) node->text = pt->pToken->text;
    return node;
}

spAstNode buildIdentifierExpr(const ParseTree* pt) {
    auto node = std::make_shared<IdentifierExpr>();
    setLoc(*node, pt);
    if (pt->production == Production::ALLOC_IDENTIFIER) {
        node->isAlloc = true;
        node->text = firstTokenText(pt->spDown.get());
    } else {
        // IDENTIFIER node: optional TYPENAME:: prefix chain + IDENTIFIER token
        node->text = "";
        for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
            switch (c->production) {
                case Production::TYPENAME:
                    if (c->pToken) {
                        if (!node->text.empty()) node->text += "::";
                        node->text += c->pToken->text;
                    }
                    break;
                case Production::IDENTIFIER:
                    if (c->pToken) {
                        if (!node->text.empty()) node->text += "::";
                        node->text += c->pToken->text;
                    }
                    break;
                case Production::DCOLON:
                    break; // structural — no-op
                default:
                    if (c->pToken && !c->pToken->text.empty()) {
                        if (!node->text.empty()) node->text += "::";
                        node->text += c->pToken->text;
                    }
                    break;
            }
        }
        // Fallback: if no children, use own token
        if (node->text.empty() && pt->pToken) node->text = pt->pToken->text;
    }
    return node;
}

// Build a term list (for CallExpression / SubcallExpr) from a sibling chain
// Each term is an expression node; operators become IdentifierExpr nodes.
std::vector<spAstNode> buildTermList(const ParseTree* first) {
    std::vector<spAstNode> terms;
    for (const ParseTree* c = first; c; c = c->spNext.get()) {
        switch (c->production) {
            // Operators
            case Production::CALL_OPERATOR: {
                auto op = std::make_shared<IdentifierExpr>();
                setLoc(*op, c);
                op->text = firstTokenText(c);
                terms.push_back(op);
                break;
            }
            // Terminals / sub-expressions
            case Production::DECIMAL: case Production::HEXNUMBER:
            case Production::BINARY:  case Production::NUMBER:
            case Production::STRING:
                terms.push_back(buildLiteral(c));
                break;
            case Production::IDENTIFIER:
            case Production::ALLOC_IDENTIFIER:
                terms.push_back(buildIdentifierExpr(c));
                break;
            case Production::SUBCALL_EXPRESSION:
                terms.push_back(buildSubcallExpr(c));
                break;
            case Production::CALL_COMMAND:
            case Production::CALL_CONSTRUCTOR:
            case Production::CALL_VCOMMAND:
                terms.push_back(buildCallInvoke(c));
                break;
            case Production::CALL_QUOTE:
            case Production::CALL_BLOCK_NOFAIL:
            case Production::CALL_BLOCK_FAIL:
            case Production::CALL_BLOCK_MAYFAIL:
                terms.push_back(buildCallQuote(c));
                break;
            case Production::CALL_CMD_LITERAL:
                terms.push_back(buildCmdLiteral(c));
                break;
            case Production::ENUM_DEREF: {
                // TYPENAME [ IDENTIFIER ] — represent as IdentifierExpr "Type[member]"
                auto id = std::make_shared<IdentifierExpr>();
                setLoc(*id, c);
                id->text = firstTokenText(c);
                terms.push_back(id);
                break;
            }
            case Production::CALL_EXPR_DEREF:
            case Production::CALL_EXPR_ADDR:
            case Production::CALL_EXPR_INDEX:
                // Suffix modifiers: attach as a trailing IdentifierExpr for now
                {
                    auto suf = std::make_shared<IdentifierExpr>();
                    setLoc(*suf, c);
                    suf->text = firstTokenText(c);
                    terms.push_back(suf);
                }
                break;
            case Production::LPAREN: case Production::RPAREN:
            case Production::COMMA:  case Production::PIPE:
                break; // structural — no-op
            default:
                break;
        }
    }
    return terms;
}

spAstNode buildSubcallExpr(const ParseTree* pt) {
    auto node = std::make_shared<SubcallExpr>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::CALL_CMD_LITERAL:
                node->terms.push_back(buildCmdLiteral(c));
                break;
            case Production::CALL_QUOTE:
            case Production::CALL_BLOCK_NOFAIL:
            case Production::CALL_BLOCK_FAIL:
            case Production::CALL_BLOCK_MAYFAIL:
                node->terms.push_back(buildCallQuote(c));
                break;
            default:
                // Everything else: build as term list
                for (auto& t : buildTermList(c))
                    node->terms.push_back(t);
                break;
        }
    }
    return node;
}

spAstNode buildCallAssignment(const ParseTree* pt) {
    auto node = std::make_shared<CallAssignment>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::IDENTIFIER:
            case Production::ALLOC_IDENTIFIER: {
                auto id = std::static_pointer_cast<IdentifierExpr>(buildIdentifierExpr(c));
                node->target = id;
                break;
            }
            case Production::SUBCALL_EXPRESSION:
                node->exprs.push_back(buildSubcallExpr(c));
                break;
            case Production::CALL_OPERATOR: {
                // post-assignment operator: next sibling is the rhs
                std::string op = firstTokenText(c);
                const ParseTree* rhs = c->spNext.get();
                if (rhs) {
                    spAstNode rhsNode = build(rhs);
                    node->postOps.emplace_back(op, rhsNode);
                }
                break;
            }
            case Production::LARROW: case Production::PIPE:
                break; // structural — no-op
            default:
                break;
        }
    }
    return node;
}

spAstNode buildCallExpression(const ParseTree* pt) {
    auto node = std::make_shared<CallExpression>();
    setLoc(*node, pt);
    node->terms = buildTermList(pt->spDown.get());
    return node;
}

spAstNode buildBlock(const ParseTree* pt) {
    auto node = std::make_shared<Block>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::BLOCK_HEADER:
                for (const ParseTree* h = c->spDown.get(); h; h = h->spNext.get()) {
                    switch (h->production) {
                        case Production::DQMARK:        node->kind = Block::Kind::DoWhenMulti;  break;
                        case Production::QMARK:         node->kind = Block::Kind::DoWhen;       break;
                        case Production::QMINUS:        node->kind = Block::Kind::DoWhenFail;   break;
                        case Production::DO_ELSE:       node->kind = Block::Kind::DoElse;       break;
                        case Production::BANG:          node->kind = Block::Kind::DoUnless;     break;
                        case Production::PERCENT:       node->kind = Block::Kind::DoBlock;      break;
                        case Production::DO_REWIND:     node->kind = Block::Kind::DoRewind;     break;
                        case Production::DO_RECOVER:    node->kind = Block::Kind::DoRecover;    break;
                        case Production::DO_RECOVER_SPEC:
                            node->kind = Block::Kind::DoRecoverSpec;
                            // PIPE RECOVER_SPEC RARROW
                            for (const ParseTree* rs = h->spDown.get(); rs; rs = rs->spNext.get()) {
                                if (rs->production == Production::RECOVER_SPEC) {
                                    const ParseTree* rc = rs->spDown.get();
                                    if (rc && rc->production == Production::TYPE_NAME_Q) {
                                        node->recoverType = firstTokenText(rc);
                                        rc = rc->spNext.get();
                                        if (rc && rc->pToken) node->recoverName = rc->pToken->text;
                                    } else if (rc) {
                                        node->recoverName = firstTokenText(rc);
                                    }
                                }
                            }
                            break;
                        case Production::ON_EXIT:      node->kind = Block::Kind::OnExit;     break;
                        case Production::ON_EXIT_FAIL: node->kind = Block::Kind::OnExitFail; break;
                        case Production::DO_WHEN_SELECT: node->kind = Block::Kind::DoWhen; break; // select variant
                        case Production::PIPE: case Production::RARROW:
                            break; // structural — no-op
                        default:
                            break;
                    }
                }
                break;
            case Production::CALL_GROUP:
                node->body = buildCallGroup(c);
                break;
            default:
                break; // structural — no-op
        }
    }
    return node;
}

std::shared_ptr<CallGroup> buildCallGroup(const ParseTree* pt) {
    auto node = std::make_shared<CallGroup>();
    setLoc(*node, pt);
    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::CALL_ASSIGNMENT:
                node->statements.push_back(buildCallAssignment(c));
                break;
            case Production::CALL_EXPRESSION:
                node->statements.push_back(buildCallExpression(c));
                break;
            case Production::CALL_COMMAND:
            case Production::CALL_CONSTRUCTOR:
            case Production::CALL_VCOMMAND:
                node->statements.push_back(buildCallInvoke(c));
                break;
            case Production::DO_BLOCK:
                node->statements.push_back(buildBlock(c));
                break;
            default:
                break; // structural — no-op
        }
    }
    return node;
}

// ---------------------------------------------------------------------------
// Central dispatch — handles any node that can appear as a child
// ---------------------------------------------------------------------------
spAstNode build(const ParseTree* pt) {
    if (!pt) return nullptr;
    switch (pt->production) {
        // Top-level declarations
        case Production::COMPILATION_UNIT:  return nullptr; // handled by buildAst() directly
        case Production::DEF_MODULE:        return buildModuleDecl(pt);
        case Production::DEF_IMPORT:        return buildImportDecl(pt);
        case Production::DEF_ALIAS:         return buildAliasDecl(pt);
        case Production::DEF_CLASS:         return buildClassDecl(pt);
        case Production::DEF_CMD:           return buildCmdDef(pt);
        case Production::DEF_CMD_DECL:      return buildCmdDecl(pt);
        case Production::DEF_CMD_INTRINSIC: return buildIntrinsicDecl(pt);
        case Production::DEF_DOMAIN:        return buildDomainDecl(pt);
        case Production::DEF_ENUM:          return buildEnumDecl(pt);
        case Production::DEF_INSTANCE:      return buildInstanceDecl(pt);
        case Production::DEF_OBJECT:        return buildObjectDecl(pt);
        case Production::DEF_PROGRAM:       return buildProgramDecl(pt);
        case Production::DEF_RECORD:        return buildRecordDecl(pt);
        case Production::DEF_TEST:          return buildTestDecl(pt);
        // Statements
        case Production::CALL_ASSIGNMENT:   return buildCallAssignment(pt);
        case Production::CALL_EXPRESSION:   return buildCallExpression(pt);
        case Production::CALL_COMMAND:
        case Production::CALL_CONSTRUCTOR:
        case Production::CALL_VCOMMAND:     return buildCallInvoke(pt);
        case Production::DO_BLOCK:          return buildBlock(pt);
        case Production::CALL_GROUP:        return buildCallGroup(pt);
        // Expressions
        case Production::SUBCALL_EXPRESSION:return buildSubcallExpr(pt);
        case Production::CALL_CMD_LITERAL:  return buildCmdLiteral(pt);
        case Production::CALL_QUOTE:
        case Production::CALL_BLOCK_NOFAIL:
        case Production::CALL_BLOCK_FAIL:
        case Production::CALL_BLOCK_MAYFAIL:return buildCallQuote(pt);
        case Production::LITERAL:           return build(pt->spDown.get()); // transparent
        case Production::DECIMAL: case Production::HEXNUMBER:
        case Production::BINARY:  case Production::NUMBER:
        case Production::STRING:            return buildLiteral(pt);
        case Production::IDENTIFIER:
        case Production::ALLOC_IDENTIFIER:  return buildIdentifierExpr(pt);
        // Type expressions — not directly statement-level, but reachable via build()
        case Production::TYPE_EXPR:
        case Production::TYPE_EXPR_DOMAIN:
        case Production::TYPE_EXPR_CMD:
        case Production::TYPE_EXPR_PTR:
        case Production::TYPE_EXPR_RANGE:
        case Production::TYPE_EXPR_RANGE_FIXED:
        case Production::TYPE_NAME_Q:
        case Production::TYPEDEF_NAME_Q:    return buildTypeExpr(pt);
        // Sub-productions that are structural / transparent — no-op, recurse into children
        case Production::DEF_MODULE_NAME:
        case Production::DEF_IMPORT_FILE:
        case Production::DEF_IMPORT_STANDARD:
        case Production::DEF_ENUM_NAME1:
        case Production::DEF_ENUM_NAME2:
        case Production::DEF_ENUM_ITEM_NAME:
        case Production::DEF_ENUM_ITEM_LIST:
        case Production::DEF_RECORD_NAME:
        case Production::DEF_RECORD_FIELDS:
        case Production::DEF_RECORD_FIELD:
        case Production::DEF_RECORD_FIELD_NAME:
        case Production::DEF_RECORD_FIELD_DOMAIN:
        case Production::DEF_OBJECT_NAME:
        case Production::DEF_OBJECT_FIELDS:
        case Production::DEF_OBJECT_FIELD:
        case Production::DEF_OBJECT_FIELD_NAME:
        case Production::DEF_OBJECT_FIELD_TYPE:
        case Production::DEF_CLASS_NAME:
        case Production::DEF_CLASS_CMDS:
        case Production::DEF_INSTANCE_NAME:
        case Production::DEF_INSTANCE_DELEGATE:
        case Production::DEF_INSTANCE_TYPES:
        case Production::DEF_CMD_NAME:
        case Production::DEF_CMD_NAME_SPEC:
        case Production::DEF_CMD_MAYFAIL:
        case Production::DEF_CMD_FAILS:
        case Production::DEF_CMD_PARM:
        case Production::DEF_CMD_PARM_NAME:
        case Production::DEF_CMD_PARM_TYPE:
        case Production::DEF_CMD_PARMTYPE_NAME:
        case Production::DEF_CMD_PARMTYPE_VAR:
        case Production::DEF_CMD_RECEIVER:
        case Production::DEF_CMD_RECEIVERS:
        case Production::DEF_CMD_PARMS:
        case Production::DEF_CMD_IMPARMS:
        case Production::DEF_CMD_RETVAL:
        case Production::DEF_CMD_BODY:
        case Production::DEF_CMD_EMPTY:
        case Production::DEF_DOMAIN_NAME:
        case Production::DEF_DOMAIN_PARENT:
        case Production::CALL_PARAMETER:
        case Production::CALL_PARM_EMPTY:
        case Production::CALL_PARM_EXPR:
        case Production::CALL_CMD_TARGET:
        case Production::CALL_IDENTIFIER:
        case Production::CALL_OPERATOR:
        case Production::CALL_EXPR_DEREF:
        case Production::CALL_EXPR_ADDR:
        case Production::CALL_EXPR_INDEX:
        case Production::BLOCK_HEADER:
        case Production::RECOVER_SPEC:
        case Production::DO_RECOVER_SPEC:
        case Production::DO_RECOVER:
        case Production::DO_WHEN:
        case Production::DO_WHEN_MULTI:
        case Production::DO_WHEN_FAIL:
        case Production::DO_WHEN_SELECT:
        case Production::DO_ELSE:
        case Production::DO_UNLESS:
        case Production::DO_REWIND:
        case Production::CALL_QUOTED:
        case Production::ENUM_DEREF:
        case Production::TYPEDEF_PARMS:
        case Production::TYPEDEF_PARM_TYPE:
        case Production::TYPEDEF_PARM_VALUE:
        case Production::TYPE_NAME_ARGS:
        case Production::TYPE_ARG_TYPE:
        case Production::TYPE_ARG_VALUE:
        case Production::TYPE_CMDEXPR_ARG:
        case Production::QUALIFIED_TYPENAME:
            return build(pt->spDown.get()); // transparent structural node
        // Pure terminal / punctuation tokens — no-op
        case Production::TYPENAME:
        case Production::ALIAS:    case Production::CLASS:    case Production::COMMAND:
        case Production::DECLARE:  case Production::DOMAIN:   case Production::ENUMERATION:
        case Production::IMPORT:   case Production::INSTANCE: case Production::INTRINSIC:
        case Production::MODULE:   case Production::OBJECT:   case Production::PROGRAM:
        case Production::RECORD:   case Production::TEST:
        case Production::AMBANG:   case Production::AMPERSAND: case Production::AMPHORA:
        case Production::APOSTROPHE: case Production::ASTERISK: case Production::BANG:
        case Production::BANGBRACE:  case Production::BANGLANGLE: case Production::CARAT:
        case Production::COMMA:    case Production::COLON:    case Production::COLANGLE:
        case Production::COLBRACE: case Production::DCOLON:   case Production::DOLLAR:
        case Production::EQUALS:   case Production::EXTRACT:  case Production::GREQUALS:
        case Production::INSERT:   case Production::LANGLE:   case Production::LEQUALS:
        case Production::LARROW:   case Production::LBRACE:   case Production::LBRACKET:
        case Production::LPAREN:   case Production::MINUS:    case Production::PERCENT:
        case Production::PIPE:     case Production::PLUS:     case Production::POUND:
        case Production::QBRACE:   case Production::QLANGLE:  case Production::QMARK:
        case Production::QMINUS:   case Production::DQMARK:   case Production::RANGLE:
        case Production::RARROW:   case Production::RBRACE:   case Production::RBRACKET:
        case Production::RPAREN:   case Production::SLASH:    case Production::UNDERSCORE:
        case Production::ON_EXIT:  case Production::ON_EXIT_FAIL:
            return nullptr; // terminal token — no-op
    }
    return nullptr;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
std::shared_ptr<CompilationUnit> basis::buildAst(const spParseTree& pt) {
    if (!pt || pt->production != Production::COMPILATION_UNIT)
        return nullptr;

    auto node = std::make_shared<CompilationUnit>();
    setLoc(*node, pt.get());

    for (const ParseTree* c = pt->spDown.get(); c; c = c->spNext.get()) {
        switch (c->production) {
            case Production::DEF_MODULE:
                node->module = buildModuleDecl(c);
                break;
            case Production::DEF_IMPORT:
                node->imports.push_back(buildImportDecl(c));
                break;
            // All possible top-level definition productions
            case Production::DEF_ALIAS:
                node->definitions.push_back(buildAliasDecl(c));
                break;
            case Production::DEF_CLASS:
                node->definitions.push_back(buildClassDecl(c));
                break;
            case Production::DEF_CMD:
                node->definitions.push_back(buildCmdDef(c));
                break;
            case Production::DEF_CMD_DECL:
                node->definitions.push_back(buildCmdDecl(c));
                break;
            case Production::DEF_CMD_INTRINSIC:
                node->definitions.push_back(buildIntrinsicDecl(c));
                break;
            case Production::DEF_DOMAIN:
                node->definitions.push_back(buildDomainDecl(c));
                break;
            case Production::DEF_ENUM:
                node->definitions.push_back(buildEnumDecl(c));
                break;
            case Production::DEF_INSTANCE:
                node->definitions.push_back(buildInstanceDecl(c));
                break;
            case Production::DEF_OBJECT:
                node->definitions.push_back(buildObjectDecl(c));
                break;
            case Production::DEF_PROGRAM:
                node->definitions.push_back(buildProgramDecl(c));
                break;
            case Production::DEF_RECORD:
                node->definitions.push_back(buildRecordDecl(c));
                break;
            case Production::DEF_TEST:
                node->definitions.push_back(buildTestDecl(c));
                break;
            // Structural nodes that wrap the above (e.g. from maybe/oneOrMore) — transparent
            case Production::COMPILATION_UNIT:
                break; // should not occur as a child, but guard it — no-op
            default:
                break; // any other structural wrapper — no-op
        }
    }
    return node;
}


