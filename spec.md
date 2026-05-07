# Basis Language Specification

**Status:** Working draft. This document consolidates the Basis design intent into a single specification that drives implementation work at the AST level — lexical structure, grammar, AST shape, typechecking rules, semantic analyses, and operational semantics. Material below the AST level (intermediate representation, code generation, runtime support library, standard-library design) is out of scope and not addressed here.

**Date:** Session 1 (2026-05-06).

**Provenance.** Distilled from the topic-organized references (`reference-failure-system.md`, `reference-operational-semantics.md`, `reference-type-system-and-modes.md`, `reference-construction-and-initialization.md`, `reference-lambda-and-fexpr.md`, `reference-class-system.md`), the project README, and the supporting checkpoint stream (CP001–CP017), with the corrections recorded in `project-drift-control.md` applied throughout. The intent captured in these documents is in some places ahead of the current parser implementation; the specification reflects the intended language, with implementation gaps tracked as work-plan items rather than spec compromises.

**Authority hierarchy for this specification.** Where two project documents differ on the same matter, the later document is authoritative — concretely: the README and `project-drift-control.md` win where their coverage is clear; the topic-organized references win on their respective domains where the README is silent; the source checkpoints are historical and are cited only where their context aids understanding. The implementation source code (`Grammar2.cpp`, `Parsing2.cpp`, `test_grammar2.cpp` in the project repository) is reference material indicating what is currently parseable; it is a subset of what this specification covers and does not constrain the spec where the intent has moved beyond what the parser admits.

**Notation conventions.** Type expressions use the form `Type 'name`, `Type &name`, or `Type name` for parameter and binding positions, with the mode marker as a prefix on the name. Class-constraint forms use `(T:Class)` for type-variable-bound parameters; `Class 'name` for existential class-typed parameters. Command-type expressions use `:<…>`, `?<…>`, `!<…>` for ordinary command-typed values and `:<*>`, `?<*>`, `!<*>` for fexpr-typed values. Decision-level cross-references between sections of this specification take the form *[§X.Y]*. References to forwarded open questions take the form *[OQ-N]*; the open-questions appendix lists every catalogued OQ. Reconciliation between this specification and the source documents is global and silent — where the source documents disagreed and this specification settles the disagreement, the resolution stands without inline marker.

**Document organization.** The body of this specification (§§1–9) follows the README's outside-in structure, deepened to specification grade. The appendices (A–I) carry implementor-facing detail — lexical structure, full grammar, AST node taxonomy, typechecking rules in judgment form, the static analyses with their full transfer functions, the formal operational semantics, identifier resolution, the module system and instance-coherence algorithm, and the open-questions catalogue.

---

## 1. Introduction

### 1.1 Purpose

Basis is a programming language designed for code that benefits from a direct semantic match to Hexagonal Architecture, that maintains tight bounded scope on side effects, and that supports both high-level polymorphic styles and low-level byte-faithful work in a single coherent surface. The language's specific design commitments are oriented toward making AI-generated and AI-reviewed code tractable: side-effect bounds on commands flow through their signatures, recovery contexts are structurally visible in the source, and the static analyses guarantee the runtime properties the signatures describe.

This specification describes the language at the level needed to implement a parser, a typechecker, the core static analyses, and an interpreter or naive AST-walking executor. It does not describe an intermediate representation, a code generator, a runtime support library, or the standard library. Where the language depends on standard-library or runtime presence — as for the `Int64`-arithmetic intrinsics, the `FexprFailure` and `CoercionFailure` standard messages, or the platform-specific allocator — the specification names the dependency without specifying its provider.

### 1.2 Guiding Principles

The language realizes a small set of guiding principles. Each principle has consequences that propagate through multiple sections of this specification; the principles themselves are stated here as the framing under which the rest of the language coheres.

1. **Strong typing saves lives.** Every value has a static type; every parameter, local, and field is typed at declaration; every cross-type movement is either explicitly authorized (constructor invocation, `.implicit` declaration, `-<` dynamic narrowing) or rejected at typechecking. There are no runtime-typed values that escape the static layer.

2. **No non-local state access.** A command body's reachable state arrives by an explicit chain of provision: parameters and receivers, implicit context parameters resolved from the caller's lexical scope, or captured-and-traveling state borne by a value the body operates on. No global mutable state, no ambient context, no thread-local storage, no module-level mutable singletons. The single carve-out is enumerations (§5.9), which are compile-time constants.

3. **The fundamental datatype is a buffer.** Buffer-backed types — buffers, ranges, domains, records, unions — reduce transitively to bytes. They copy as bytes, fit inside other byte-aggregates, and are reclaimable at frame retirement without traversal. Non-buffer types (pointers, command-typed values, objects, variants) are confined to top-level slots and object fields.

4. **Mutation either succeeds fully or fails fully.** Every writeable parameter passing uses copy-by-value or copy-restore semantics: on success the callee's value is committed to the caller's slot atomically; on failure no write occurs and the caller's slot is bit-identical to its pre-call state. Failure-atomicity falls out of the calling convention with no transactional machinery and no rollback code in user programs.

5. **No hidden control flow.** Failures propagate, but only along structurally visible paths. There is no exception-with-stack-unwinding mechanism, no implicit dispatch through hidden vtables, no implicit destructor firing on member-field cleanup. Every control-flow effect is either explicit at the call site or marked by a block-construct in the source. The single exception, intentionally narrow, is the at-stack mechanism for frame-exit hooks (§3.12).

6. **Polymorphism and statecharts aren't just for object types.** The class system (Haskell-style typeclass dictionaries) operates uniformly over buffer-backed types, non-buffer types, and command-typed values. Variants admit class-witness slots, supporting dispatch through the active candidate. The same dispatch and propagation patterns appear across variants, failure messages, and class-typed parameters.

7. **Computational status is orthogonal to result state.** A command's success/failure axis (the `:` / `?` / `!` failure marks) is independent of any value the command produces. Failure carries a message; success carries the writes-to-writeable-parameters; neither subsumes the other. The two axes compose explicitly, never by fusion.

8. **Prefer small orthogonal concepts to rich overlapping ones.** Where a new mechanism's work can be done by composing existing language facilities, the composition is preferred. The `-<` operator handles dynamic type coercion across all type pairs (variants, class hierarchies, unions, object pointers) rather than introducing per-case operators. Variant pattern matching uses `?:` chains plus `-<` rather than a dedicated `match` keyword. Receiver-baked partial application reuses the four-form taxonomy rather than a separate "method-pointer" mechanism.

9. **Special forms should be visually distinct from user-defined forms.** Top-level definition keywords begin with `.` (dot-prefix), distinguishing them from user-defined identifiers. Block markers (`?`, `?-`, `?:`, `??`, `-`, `%`, `^`, `|`, `@`, `@!`) are punctuation, not keywords, ensuring they cannot be shadowed. The `${…}` and `$[…]` literal fences carry the `$` sigil that distinguishes them from any user-defined identifier or aggregate.

10. **Syntactic sugar is superior to semantic sugar.** Surface forms that desugar to other surface forms are admitted; surface forms that introduce new semantic primitives are admitted only when they cannot be expressed by composition. Expression-position calling conventions desugar into command calls with `#`-introduced argument-position locals; the `<-` choice form desugars into a chain of try-or-fall-through; the implicit `-> name` rule for single-writeable-parameter commands desugars into the explicit form. Each desugaring is one substitution; the underlying semantics has no hidden complexity.

11. **Fexprs and macros, not fexprs vs macros.** The language admits user-defined control-flow primitives via the fexpr mechanism (§8.5), which captures defining-frame state implicitly and operates on it at invocation time. Macros — compile-time syntax-rewriting — are not part of the v1 language but the design preserves the option. Where other languages forced a choice between hygienic macros and run-time eval-style fexprs, Basis uses fexprs as the primary user-defined control-flow mechanism and reserves macros as a future extension that does not conflict.

12. **Syntactic whitespace improves legibility.** Indentation determines block structure. The lexer maintains an indent stack alongside its bracket and brace stacks; the parser sees the resulting token stream as a structured tree. The visual layout of a Basis source file reflects its operational structure; constructs at a single indentation level are siblings; constructs at deeper indentation are children. The composition rules for block markers (§4.5) read off the indentation structure directly.

These principles are not separate features that the language assembles; they are the framing under which the rest of the language is written. The sections of this specification that realize each principle are noted in the relevant places.

### 1.3 Core Semantic Skeleton

Basis programs reduce under a small-step operational semantics over a state tuple $\langle V, \Phi, \Sigma \rangle$, where:

- **$V$** is the current verb to be executed. Verbs include user commands $\mathit{exec}(c)$, the failure-firing $\mathit{fail}(\phi)$, the recovery markers $\mathit{recover}$ and $\mathit{recover}(\phi, \sigma, c)$, the scope boundary markers $\mathit{scope}(c)$ and $\mathit{scopefail}(c)$, and the rewind verb $\mathit{rewind}(v)$ for loop continuation. The notation $\overrightarrow{v}$ denotes the continuation from $v$ — what runs next once $v$ has completed.

- **$\Phi$** is the failure register. When no failure is in flight, $\Phi$ holds the empty value $\epsilon$. When a failure is propagating, $\Phi$ holds a failure value $\phi$ — a message identity, an optional payload pointer, and an optional class witness (the runtime details are specified in §4 and Appendix F).

- **$\Sigma$** is the variable state — a mapping from in-scope names to slot identities and contents, partitioned by frame. The notation $\sigma/c$ denotes $\sigma$ bound within the lexical scope of the verb $c$.

The reduction rules below describe how the state tuple evolves. Each rule is read as: *given the antecedent state shape, the system reduces to the consequent state shape.* The notation $\langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi', \Sigma' \rangle$ denotes a complete sub-evaluation of $v$ that itself produced post-state $\langle v', \phi', \Sigma' \rangle$.

$$
\begin{aligned}
\text{(normal execution)} \quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma' \rangle \\[2pt]
\text{(generating failure)} \quad & v = \mathit{fail}(\phi) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\[2pt]
\text{(command failure)} \quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\[2pt]
\text{(skips from failure)} \quad & v \in \{\mathit{exec}(c), \mathit{rewind}(w), \mathit{fail}(\gamma)\} \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\[2pt]
\text{(generic recovery)} \quad & v = \mathit{recover} \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma \rangle \\[2pt]
\text{(specific recovery)} \quad & v = \mathit{recover}(\phi, \sigma, c) \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma + \sigma/c \rangle, \quad \vec{c} \leftarrow \vec{v} \\[2pt]
\text{(recovery failure)} \quad & v = \mathit{recover}(\alpha, \sigma, c) \quad \langle v, \phi, \Sigma \rangle \quad \phi \neq \alpha & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\[2pt]
\text{(scope boundary)} \quad & v = \mathit{scope}(c) \quad \kappa \in \Phi \quad \langle v, \kappa, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma \rangle, \quad \vec{c} \leftarrow \langle \vec{v}, \kappa, \Sigma' \rangle \\[2pt]
\text{(scope under success)} \quad & v = \mathit{scopefail}(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma \rangle \\[2pt]
\text{(scope under failure)} \quad & v = \mathit{scopefail}(c) \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma \rangle, \quad \vec{c} \leftarrow \langle \vec{v}, \phi, \Sigma' \rangle \\[2pt]
\text{(looping)} \quad & v = \mathit{rewind}(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma \rangle
\end{aligned}
$$

A program executes by repeated application of these rules, beginning with the verb sequence corresponding to a `.program` directive's body and an empty failure register. The full operational semantics — including the details of how user commands $\mathit{exec}(c)$ interact with parameter passing, how the failure register's $\phi$ is realized as a slot in an activation record, and how the scope and scopefail verbs map onto block markers — is given in §3 (commands), §4 (failure and recovery), and Appendix F (formal operational semantics).

A few features of the reduction system are worth flagging at this level:

- **Single-shot semantics.** Each rule produces exactly one outcome. There is no backtracking, no resumption, no demand-driven multi-valued evaluation. The Icon-influenced combinator structure of the block markers (§4.4) composes single-shot outcomes; it does not introduce generator-style multi-valued flow.

- **No expression-level evaluation primitive.** The reduction system has commands and failure flow; it has no evaluation-to-a-value primitive. Surface forms that look like expression evaluation — the right-hand side of `<-`, parenthesized command invocations in argument position — desugar into command sequences whose effects produce what the surface form denotes. The operational semantics has no expression layer beneath the command layer.

- **Failure as a parallel axis.** The failure register $\Phi$ is independent of the verb $V$ and the variable state $\Sigma$. A failure in flight does not modify $\Sigma$; it skips past subsequent verbs that would execute under no-failure conditions. The recovery rules consume $\Phi$ explicitly. The two axes — command flow and failure flow — never confuse for each other in the reduction system.

- **Scope as the unit of structural recovery.** The `scope` and `scopefail` verbs map onto the block-marker constructs that establish failure-recovery contexts (`?`, `?-`, `?:`, `^`, `|`, `??`). The mapping is given in §4.4. The invariant the reduction system enforces is that every failure in flight either consumes at a structurally-visible recovery point or propagates upward to the program's top frame.

### 1.4 The Two Foundational Principles

Two principles shape what code in Basis is *able* to do. Both are stated as commitments the language makes; the operational semantics enforces them; the type system tracks what can be inferred from them.

**No hidden control flow.** Every control-flow transition is either explicit at the call site (the `::` operator marks class dispatch) or marked by a block-construct in the source (a block marker establishes a recovery context). There is no `try`/`catch`/`finally` parallel; recovery is at sibling positions in the source structure, where the reader can identify them by inspection. There is no implicit destructor firing on object-field cleanup. There is no implicit conversion at parameter boundaries — the only implicit type movements are the buffer-backed parent-chain upcast and the `.implicit`-licensed literal coercion, both of which are statically determined. A reader of a command signature knows what may happen — the failure mark and the parameter list together name every effect — and a reader of a command body can identify every recovery context, every dispatch site, every place a failure may consume. Reasoning is signature-bounded.

The single intentional exception is the at-stack mechanism for frame-exit hooks (§3.12). When a frame retires, registered `@`-handlers and `@!`-handlers fire alongside any explicit cleanup code. This is implicit in the sense that no source-line at the registration site invokes the handler; the handler runs as a consequence of the frame retiring. The mechanism is the language's RAII-equivalent — necessary for ergonomic resource management, and confined to a single, type-visible category (the `@` and `@!` declarations on a class).

**No non-local state.** A command body's reachable state arrives by an explicit chain of provision. There are three kinds of provision:

1. **Parameters and receivers.** A frame's caller passes state as named arguments at the call site; the state is locally bound to the parameter or receiver names within the body.

2. **Implicit context parameters.** The `/`-list in a command's signature names parameters that are filled automatically at the call site from the caller's lexical scope, by uniqueness-of-type. The provision is implicit at the syntactic level but not at the semantic level — the value must actually exist in the caller's lexical scope to be eligible. This is the language's Scala-implicit-style mechanism (§3.6).

3. **Captured-and-traveling state.** A value (such as a fexpr or a lambda) carries with it references to slots in some frame, established at the moment the value was constructed. When the value is later invoked, those references are part of the value's identity. The provision was made at construction time, not at invocation time. This is the language's closure mechanism (§8).

Absence of any of these means absence of reach. A frame six levels above the current one is on the call stack, but if no explicit provision chain reaches up to it, its slots are unreachable from the current command. There is no mechanism by which a command can opt into reaching state that was not provisioned for it.

The two principles compose. The first restricts what a command can do *implicitly*; the second restricts what a command can *reach*. Together they make signature-as-documentation work: a command's signature names everything the command can affect, and the marks plus block structure describe everything that can happen control-flow-wise.

The principles relax at `.program` and `.test` directives, which are the language's entry points and lie outside ordinary command scope. These directives may use language features that ordinary commands cannot — at minimum, they are the only contexts in which top-level failures are terminally consumed. The precise relaxations for these directives are an open question (OQ-27 for top-level failure handling; see Appendix I).

### 1.5 Standing Lenses

Reasoning about Basis correctly requires a small set of mental lenses that are not always native to the reasoning patterns programmers carry from C++, Java, Rust, Go, ML, or Haskell. These lenses are stated here so that the rest of the specification can rely on them; sections that lean on a particular lens reference it where appropriate.

**Frame-ownership.** Every slot is owned by some frame. "Return values" in the conventional C++/Java/Rust sense do not exist: output flows through writeable parameter slots that the caller owns, written to by copy-restore on success. When reasoning about value semantics, mode contracts, aliasing, or lifetime ceilings, the question is *which frame owns this slot* — not "what does this function return."

**Each frame's static analysis is local.** Initialization tracking, failure-mode tracking, and access-path taint are properties of a single command body's analysis on its own parameters and locals. There is no cross-frame propagation. A PRODUCE or REFERENCE output of a downstream call appears in the caller's frame as a freshly-bound local; the access path is rooted at the caller's local, not at any of the caller's parameters. The discipline migrates to the frame that has the right context; the language does not propagate.

**Access paths, not storage.** The transitive READ contract operates on access paths — the named ways through which a value is reached — not on storage locations. The same storage may be reached at runtime through multiple access paths simultaneously, one of which is READ-rooted and another of which is REFERENCE-rooted; writes through the REFERENCE-rooted path are permitted while writes through the READ-rooted path are forbidden. The language does not detect aliasing; it tracks paths.

**Buffer-backed containment.** A buffer-backed type may contain only buffer-backed types. Records cannot have pointer fields, command-typed-value fields, object fields, or variant fields. Unions cannot either. Typed buffers `[N]T` require `T` to be buffer-backed. This rule eliminates whole categories of static-analysis questions: cases that presuppose a buffer-backed structure containing non-buffer-backed components are not Basis cases.

**Buffer-backed subsumption is uniform and broad.** Every buffer-backed type subsumes up its parent chain to `[N]` and `[]`. A `Point` (record over an `[8]`-byte parent) is implicitly acceptable where `[8]` is expected, and where `[]` is expected. Sibling types — two distinct records over `[8]`, or `Inches` and `Centimeters` both over `Int32` — subsume separately to their shared parent but do not implicitly convert peer-to-peer.

**Variants are the only "may-be-absent" type.** Every variant slot inherently admits an absent state in addition to its declared candidate states. A single-candidate variant is an optional. A bare `# x : SomeVariant` introduction produces a variant slot in the absent state. No other type has this property: pointers, objects, command-typed values, records, unions, and named domains all contain what their type declaration says they contain. Variants alone admit an inherent "nothing here" state — the language's null-pointer-inclusive data structures without admitting NULL into the type system.

**`::` is the scope operator, not just class-resolution.** The `::` operator serves multiple roles: class-method resolution on a receiver, field-member access on aggregates, namespace and module resolution, partial-application bake-in. The unifying reading is *scope-into-a-namespace* — the surrounding context determines which role applies. Calling `::` "the class-resolution operator" is too narrow.

**Orthogonality of language and standard library.** Where a new mechanism's work can be done by composing existing language facilities, the composition is preferred over a new intrinsic or new syntactic form. The `-<` operator unifies dynamic type coercion across all type pairs without per-case operators. Variant pattern matching uses `?:` chains plus `-<` rather than a `match` keyword. The standard library is consumer of the language, not co-author of it.

**Liskov substitution as a design tool.** Where the language admits a subtyping relation, the design preserves Liskov substitutability. The buffer-backed parent-chain subsumption preserves Liskov by construction (the upcast is one-directional and carries no value rewriting). The class-instance system preserves Liskov by class-method-contract (instances must satisfy the class's declared shape). Where Liskov substitution fails — as it does for the union → candidate-or-parent byte-reinterpretation, where the bytes' meaning depends on the active candidate — the relation is given a distinct name (byte-reinterpretation subsumption) and not conflated with Liskov-preserving subsumption.

**Region-style memory reclamation is a latent constraint.** The language is designed so that frame-bound region-style reclamation remains a feasible implementation strategy, even if the v1 implementation does not pursue it. Buffer-backed values do not own non-local storage; they live in their owning frame's bytes. Non-buffer types are confined to positions where ownership and lifetime are explicit (top-level slots, object fields with object-lifetime ceiling, frame-bound parameters and receivers). Design decisions that would punch holes in region feasibility — for example, by introducing values that span frame boundaries with implicit ownership transfer — are flagged where they arise.

These lenses recur throughout this specification. Sections that depend on a particular lens for their correctness call it out at the relevant point.

---

## 2. Source Files and Modules

This section defines the structure of a Basis source file, the top-level definition forms it may contain, the module system that links source files into compilation units, and the lexical conventions that govern identifier shape and indentation. Material below the source-file level — the full grammar, the lexer's token classes, the indentation algorithm, and the disambiguation rules — is treated in Appendix A and Appendix B.

### 2.1 File Structure

A Basis source file is a sequence of three optional sections, in order:

1. A single `.module` declaration naming the file's module. If absent, the file inhabits a default unnamed module whose contents are visible only within the same compilation unit.
2. Zero or more `.import` declarations bringing names from other modules into the file's lexical scope.
3. Zero or more top-level definitions (§2.2).

A file with no `.module` declaration, no imports, and no definitions is a well-formed but empty file. A file with a `.module` declaration alone declares a module containing no definitions and importing nothing, which is occasionally useful as a placeholder during incremental development.

### 2.2 Top-Level Definition Forms

A Basis source file may contain any of sixteen top-level definition forms, each introduced by a `.`-prefixed keyword. The dot-prefix marks special forms so they are visually distinct from user-defined identifiers; the keyword itself names the kind of definition. The forms, alphabetically:

- **`.alias` *Name*` : `*TypeExpression*** — Declares *Name* as a synonym for *TypeExpression*. Aliases erase entirely from the type system; the alias name and its right-hand side are interchangeable in both directions in all contexts. They introduce no new type identity.

- **`.class` *Name*` : `*body*** — Declares a class — a single-parameter type contract enumerating commands that any instance of the class must provide. The body contains `.decl` (signature-only) and `.cmd` (default-implementation) entries. The class itself has no size and no allocation footprint; it is a contract, not a layout.

- **`.cmd` *signature* `=` *body*** — Declares a regular command, a constructor, or a method. The signature shape and body conformance are specified in §3.

- **`.decl` *signature*** — Declares a command signature without a body. Used inside class bodies to require that instances supply the named method, and at top level for forward references and for the small set of intrinsic primitives that the standard library implements.

- **`.domain` *Name*` : `*ParentType*** — Declares a domain — a buffer-backed nominal refinement of *ParentType*. The parent must reduce transitively to a buffer-backed type. Pointers, command-typed values, objects, and variants cannot be domain parents.

- **`.enum`** — Declares an enumeration. Two forms: `.enum` *Name* `:` *literal entries* names *Name* as the enumeration's type; `.enum` *ConstraintType* *Name* `:` *literal entries* gives a type constraint that the literal values must inhabit and *Name* as the enumeration's type. Enumerations are compile-time constants and are the language's single principled exception to the no-non-local-state principle (§5.9).

- **`.implicit` *signature* `=` *body*** — Declares a constructor that the typechecker may insert automatically when a literal of one type appears in a context expecting another. Restricted to literal-typed source parameters (§7.8). Purely additive: a constructor declared `.implicit` is also callable explicitly via the standard parenthesized-call surface.

- **`.instance` *Type*` : `*ClassList*** — Declares that *Type* satisfies each class named in *ClassList*. An optional `(delegate `*fieldName*`)` clause beside any class name designates a field of *Type* whose existing instance for that class supplies the methods (§9.4). Multiple classes on the right side of `:` are independent instances.

- **`.intrinsic` *signature*** — Declares a command whose body is supplied by the compiler or the standard library, rather than by user-level code. Structurally a `.decl` for typechecking purposes; the implementation supplies the body. The standard `Int64`-arithmetic operations, the platform allocator primitives (when OQ-30 resolves), and the language's primitive byte-reinterpretation operations are typical examples.

- **`.msg` *Name*`[`*PayloadType*`]` `:` *ParentMessageType*** — Declares a failure message type. Both the `[`*PayloadType*`]` brackets and the `:` *ParentMessageType* clause are optional. Messages declared without a payload-type are payload-less; messages declared without a parent are roots of new hierarchies. The full failure-message system is described in §4. *(The exact surface for `.msg` is tentative and subject to OQ-26.2; the form given here is the v1 starting target.)*

- **`.object` *Name*` : `*field declarations*** — Declares a non-buffer object type — an identity-bearing aggregate whose fields may be of any type (buffer-backed or non-buffer). Objects participate in the class system as receivers and have explicit lifetime ceilings tied to their introducing frames.

- **`.program = ` *expression*** — Declares the program's entry expression. The expression runs at program start. A compilation unit may contain at most one `.program` directive.

- **`.record` *Name*` : `*field declarations*** — Declares a buffer-backed record type — a named, contiguous, byte-addressable buffer with named field offsets. Field types must be buffer-backed.

- **`.test "`*name*`" = ` *expression*** — Declares a named test. The expression runs as a test entry under the project's testing framework.

- **`.union` *Name*` : `*candidate declarations*** — Declares a buffer-backed union type — a byte-overlay of declared candidate types. Union candidates must be buffer-backed. The union carries no language-level discriminator; discrimination is the user's responsibility (§5.6).

- **`.variant` *Name*` : `*candidate declarations*** — Declares a non-buffer variant type — a tagged sum whose candidates may be of any type. Variants admit an inherent absent state in addition to their declared candidates (§5.13).

The full grammar for each of these forms is given in Appendix B. Several of the forms admit parameterization (`[T]` for type parameters, `[T:Class]` for class-bounded parameters); the parameterization syntax is uniform across them and is detailed in the relevant per-form sections (§5 for type forms, §3 for command forms, §9 for class forms).

The order of definitions within a file is not significant for visibility — every top-level name is visible throughout the file once declared at top level, regardless of source-position order. Forward references within a file are well-formed. Cross-file references require an `.import` declaration (§2.4).

### 2.3 Module Names and Namespaces

A `.module` declaration at the top of a source file names the module the file inhabits. Module names use `::` to qualify namespaces, supporting nested module hierarchies:

```
.module App::Models
.module Std::Core
.module Net::Protocols::HTTP
```

Module names are not source-file paths; they are logical names. The mapping from module names to source-file locations is a build-system concern, not a language concern. A single module may span multiple source files, in which case all files declaring `.module App::Models` contribute their definitions to the same module; the module's exported namespace is the union of contributions. Conflicts (multiple files declaring the same name within the same module) are static errors at compilation.

Module-qualified names — names that include a `::` qualifier referencing a module path — are resolved against the importing scope's import declarations (§2.4) plus the file's own module identity. A name `App::Models::User` written inside module `App::Models` is interpreted as the `User` definition within the same module; the same name written inside module `App::Views` requires that `App::Models` be imported (or that `App::Models::User` be reachable through some imported module's re-exports, which the language does not currently provide).

There is no top-level module — no implicit ancestor of all modules, no `Std::` root that contains all standard-library modules by virtue of name structure. Module names are nominal; the `::` separator does not imply containment. `Std::Core` and `Std::Utils` are independent modules whose names happen to share a prefix; importing one does not import the other.

### 2.4 Imports

An `.import` declaration brings names from another module into the current file's lexical scope. Two surface forms:

```
.import App::Models
.import "filepath"
```

The first form, the *named-module form*, names a module by its logical name. All top-level definitions in the named module become visible in the current file under their qualified names — `App::Models::User`, `App::Models::Account`, and so on. The unqualified short form (writing `User` to mean `App::Models::User`) requires either that no other in-scope import provide a same-named definition (in which case the short form unambiguously resolves) or that the user supply an alias for the import.

An aliased form is admitted, replacing the bare module name with `Alias:Module` — a single colon between the alias and the module path:

```
.import Models:App::Models
```

Names from the imported module are then accessible via the alias prefix: `Models::User`, `Models::Account`. The alias is a file-local rename; it does not shadow the original qualified name.

The second form, the *file-path form*, names a source file directly:

```
.import "shared/utils.bsl"
```

The path is interpreted relative to the importing file's location (the exact resolution rule is build-system-specific). The named file is loaded as a source file and its top-level definitions become visible. The file-path form is intended for single-file utilities and prototype work where module declarations have not yet been established; production code is expected to use the named-module form.

Imports are not transitively re-exported. A module that imports `Std::Core` does not thereby re-export `Std::Core`'s names to its own importers; each importer must explicitly `.import Std::Core` if it wishes to use names from there. The discipline keeps the import graph readable: every name visible at a use site is traceable to a specific import in the same file.

Instance visibility flows through imports as well. An `.instance T: C` declaration in some module is visible to any file that imports the module declaring it. The cross-module instance coherence rule — what happens when multiple imported modules declare instances for the same `(class, type)` pair — is given in §9.15 and Appendix H.5.

### 2.5 The `.program` Directive

The `.program` directive declares the program's entry expression:

```
.program = runSimulation: (getCommandLine)
```

The right-hand side of `=` is an expression that runs at program start. A compilation unit (the set of source files compiled together to produce a single executable) may contain at most one `.program` directive; multiple `.program` directives in the same compilation unit are a static error.

The `.program` body executes in a top-level frame whose properties differ in two respects from ordinary command frames:

1. **Top-level effects compose freely.** Within the `.program` body, side effects are admitted as the program's intended behavior; the no-non-local-state principle (§1.4) restricts what *commands* may do, not what the program-as-a-whole may do.

2. **Failure terminates the program.** A failure that escapes the `.program` body has no further frame to propagate to; the program terminates with the failure as its diagnostic. The exact terminal-handling semantics (whether the language standardizes a failure-rendering form, how exit codes are determined, whether the runtime provides an implicit recovery point) is an open question (OQ-27); the language commits that termination occurs but reserves the precise mechanism.

The `.program` body may contain any commands, including may-fail commands; recovery contexts within the body work normally (§4). Only failures that escape the entire `.program` body trigger termination.

### 2.6 The `.test` Directive

The `.test` directive declares a named test:

```
.test "round-trip serialization" = roundTripCheck
.test "edge case: empty input" = edgeCaseEmpty: ""
```

The string following `.test` names the test; the right-hand side of `=` is the test's body expression. A compilation unit may contain any number of `.test` directives.

The `.test` body executes in a top-level frame with the same relaxations as `.program`: top-level effects compose freely, and failures escaping the body are observable. The relevant difference is that test-framework integration determines what "failure escapes the body" means — typically a test failure is a test-framework outcome rather than a process termination. The exact rules are framework-dependent (OQ-27 also covers the test case).

`.test` directives are not invoked from `.program`; they are entry points that the testing framework discovers and invokes independently. A compilation unit containing only `.test` directives (no `.program`) is a valid test-only library. A compilation unit containing both is valid; the `.test` directives are independent of the `.program`.

### 2.7 Identifier Capitalization

The lexer enforces a capitalization discipline on identifiers:

- **Type names** begin with an uppercase letter. The category includes domains, records, unions, objects, variants, classes, and aliases — every nominal type. Message names (declared with `.msg`) are also type names and begin uppercase. Module-name segments separated by `::` are type names and begin uppercase.

- **Identifier names** begin with a lowercase letter. The category includes parameters, receivers, fields, locals, command names (when used as identifiers, before any leading punctuation), and all other binding identifiers.

The discipline is grammatical, not stylistic: a `.instance widget: Interface` declaration is a syntax error because `widget` is in a type position but is lowercase. A `cmd MyCommand: …` declaration is a syntax error because `MyCommand` is in a command-name position but is uppercase. The lexer recognizes the case at first character and routes the token to the appropriate non-terminal accordingly.

Mode-marker prefixes do not affect the capitalization rule. The identifier `'r` is lowercase (the `'` is the mode marker, not a letter); the identifier `&counter` is lowercase; the identifier `'Type` would be a syntax error (uppercase identifier in a binding-position name).

The discipline is enforced at the source-text level; it propagates through the rest of the language. The typechecker does not need a separate "is this name a type or an identifier" disambiguation rule, because the lexer has already partitioned the name space.

### 2.8 Lexer Stack Discipline

The lexer maintains three coordinated stacks: one for square brackets `[` `]`, one for curly braces `{` `}`, and one for indentation. A token is bound to its opening partner via the stack appropriate to its kind: a closing bracket pops the bracket stack; a closing brace pops the brace stack; a dedent pops the indentation stack. The parser sees a token stream that respects this stack structure — every closing token has a matching opening token, and the relative nesting order is maintained.

The user-visible consequence is that all three structural features — brackets, braces, indentation — are first-class, and constructs may interleave them naturally. A block at some indentation level may contain a bracket-delimited type expression, which may contain a brace-delimited block-quote, which may contain its own indented content. The lexer keeps the three structures straight; the parser sees a well-formed nested token stream.

Indentation is the language's primary block-delimiter. A line indented under another line is part of the construct begun by that line. Block markers (§4.4) carry a body whose extent is determined by indentation: the body comprises every line indented strictly more than the marker's own line, up to the first line not so indented. The block-marker composition rules — sibling adjacency for `?:` chains, `|` recovery cascades, `^` rewinds — operate on lines at the *same* indentation level as the marker. The user reading source can determine block structure by inspection; the grammar enforces the same structure via the lexer's indent stack.

The `${…}` and `$[…]` literal-fence tokens introduce a single bracketed group each (the brace stack and bracket stack respectively), matched by a single `}` or `]`. The `$` prefix is unambiguous to the lexer — the `$` character has no other use in current Basis source — so `${` and `$[` are recognized as single tokens at lex time and contribute their bracket or brace to the stack. This eliminates the disambiguation problem the prior `{- … -}` and `[- … -]` forms presented (where a `{` followed by `-` could begin either a literal fence or a block-quote with a leading minus); the `$`-prefix is the v1 surface.

The full lexer specification — the token classes, the literal-token grammar, the disambiguation rules between `'`, `&`, and adjacent tokens, the placement of the `.inline` modifier, the `<*>` typing surface for fexpr-typed slots, and the `(T:Class)` constraint form at v-command receiver positions — is given in Appendix A.

---
