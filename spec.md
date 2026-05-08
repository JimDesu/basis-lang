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
 
## 3. Commands
 
A *command* is the unit of execution in Basis. Every operation a program performs — every effect, every computation, every dispatch — is structurally a command invocation. Commands take parameters of declared types and modes, may produce values into writeable parameter slots, may fail with a message, and compose hierarchically through indentation, block markers, and recovery contexts. Commands are first-class values: they may be referenced, partially applied, captured in lambdas, stored in fields, and invoked indirectly through dispatch.
 
This section describes the surface of commands — signatures, parameters, constructors, methods, frame-exit hooks, and calling conventions. The full parameter-mode discipline (the static analyses, the transitive READ contract, taint propagation) is in §6. The first-class command-typed value forms — command reference, command literal, lambda, fexpr — are in §8. The class-and-instance dispatch system that resolves method calls is in §9.
 
### 3.1 The Unit of Execution
 
A command has exactly two possible outcomes per invocation: it *succeeds*, or it *fails*. Success and failure are orthogonal to any value the command produces — they are not encoded as result tags or sentinel values, and a successful command may produce no value, while a failed command's writeable-parameter slots are guaranteed not to be modified (§4, §6). The success/failure axis composes through the failure-mode marks (`:`, `?`, `!` per §4.2) and through the block-marker constructs (§4.4); the value-production axis composes through the parameter-passing mechanics (§6.4) and the `<-` operator (§7.1).
 
Commands have *single-shot* semantics. Each invocation produces one outcome; there is no backtracking, no resumption, no Icon-style multi-valued evaluation. The block-marker constructs that compose commands — `?`, `?-`, `?:`, `??`, `^`, `|`, `@`, `@!`, `%`, `-` (§4.4) — combine single-shot outcomes; they do not introduce a generator-style multi-valued flow.
 
Surface forms that look like expression evaluation — the right-hand side of `<-`, parenthesized command invocations in argument position, the choice form `lhs <- a | b | c` — desugar into command sequences whose effects produce what the surface form denotes. The operational semantics has commands and failure flow; it has no separate expression-evaluation primitive (Appendix F). The expression-style sugar (§3.7, §3.8, §3.13) is uniformly resolved into command-call form by the parser; the typechecker and the analyses operate on the command-call form.
 
### 3.2 Signature Shape
 
A regular command's signature has the form:
 
```
.cmd ⟨mark⟩name : parmList / implicitParmList -> resultName = body
```
 
- The optional **failure-mark prefix** on the name is one of `?` (may-fail), `!` (must-fail), or none (never-fails). The mark is not part of the command's identifier name; it is a syntactic adornment on the declaration, governed by the failure-mode discipline of §4.2. A command named `parse` declared as `.cmd ?parse: …` is invoked by the bare name `parse`, not `?parse`.
- The **parameter list** `parmList` is a comma-separated list of `Type 'name`, `Type &name`, or `Type name` declarations — covering the three parameter modes PRODUCE, REFERENCE, and READ respectively (§3.3, §6). The type-then-name order is uniform: the type appears first, the (mode-marked) name follows. The form `name : Type` is not Basis syntax in any binding position, including parameter declarations.
- The **implicit context parameter list** `implicitParmList` follows an optional `/` separator and uses the same per-parameter syntax as the regular list. Implicit parameters are filled at the call site by uniqueness-of-type from the caller's lexical scope (§3.6).
- The **result designator** `-> resultName` names which already-declared parameter or receiver carries the expression-style result when the command is invoked in expression position (§3.7). The clause is required when the command has more than one writeable parameter; it may also be supplied to designate a READ parameter as the result.
- The **body** follows `=`. The body's structure determines the failure-state-lattice paths (§4.13); the typechecker confirms that the body conforms to the signature's failure mark and that every productive parameter is written exactly once on every successful path (§6.13).
A command that takes no regular parameters omits the parameter list:
 
```
.cmd hello = writeLn: "Hello, world!"
```
 
A command that takes regular parameters but no implicit parameters omits the slash and the implicit list:
 
```
.cmd double: Int 'result, Int x = 'result <- x + x
```
 
A command with implicit parameters has the slash and the implicit list:
 
```
.cmd writeAll: List[String] lines / Logger logger = ...
```
 
The signature variations for constructors (§3.9), single-receiver methods (§3.10), multi-receiver methods (§3.11), and frame-exit hooks (§3.12) modify the prefix portion of the signature shape — the part before the parameter list — but otherwise reuse the parameter, implicit, result-designator, and body grammar of the regular form.
 
### 3.3 Parameter Modes
 
Every parameter and every receiver carries one of three *modes*, which together determine the contract between caller and callee at that position:
 
- **READ** (no marker, bare name) — the callee may read through any storage path reachable from the parameter; it may not write through any such path. The transitive read-only contract (§6.5) makes this commitment trustworthy: the language does not allow the value to be smuggled into a writeable position downstream. The caller's slot is unchanged after the call regardless of outcome.
- **PRODUCE** (`'` marker on the name, e.g., `'result`) — the callee is statically obligated to write the parameter's slot exactly once on every successful return path. The caller may pass either an initialized or an uninitialized slot; on success the produced value is copy-restored to the caller's slot, and on failure the caller's slot is bit-identical to its pre-call state (§6.4).
- **REFERENCE** (`&` marker on the name, e.g., `&counter`) — the callee may read the parameter, may write the parameter, may do neither — there is no obligation in either direction. The caller must pass an already-initialized slot, since the callee is permitted to read. Copy-restore semantics apply on the writeability axis: on success any written value is committed to the caller's slot; on failure the caller's slot is bit-identical to its pre-call state.
The markers are part of the identifier itself, not separate tokens. The lexer recognizes `'name`, `&name`, and `name` as three identifier shapes of the same name. Every read and every write inside the command body uses the marker that matches the parameter's mode — the body of a command with a productive parameter `'result` writes `'result <- value`, never `result <- value`. The marker is visible at every use site, not only at the declaration. The same-scope rule (§6.3) prevents the three shapes from coexisting in the same scope.
 
The marker placement varies by syntactic context:
 
- In **named contexts** — parameter declarations, receiver declarations, lambda invoke-method parameters, capture-list entries — the marker travels with the name in identifier-shape: `Int 'result`, `Logger &counter`, `Showable s`.
- In **nameless contexts** — command-type expressions `:<...>`, `?<...>`, `!<...>`, where parameter types are listed without names — the marker attaches to the type as a suffix, leaving the type-prefix position free for the pointer marker `^`: `Int'`, `^String&`, `Logger`.
The two placements agree on what the markers mean; they differ only on placement to suit the surrounding syntax. The full mechanics — copy-restore at the call boundary, the transitive READ contract's access-path taint algorithm, the parameter-mode invariance under failure-mark subsumption, and the receiver-mode tables by signature shape — are in §6.
 
### 3.4 Productive Parameters
 
A productive parameter (`'name`) discharges the *write-once-on-success* contract: the callee's body must write the slot exactly once on every path that reaches a successful exit. Paths that reach a failure exit are exempt from the obligation — the failure-atomicity principle (§1.2.4) commits that productive slots are never partially written when a command fails.
 
Failure to write a productive parameter on some successful path is a static error; writing it more than once on the same path is also a static error. The discipline composes with the failure-state-lattice analysis (§4.13, Appendix E.3): the typechecker walks the body's CFG with both the failure-state lattice and the initialization lattice, confirming that every path to a `clear`-state exit has performed exactly one write to each productive parameter.
 
The write-once rule is structural, not stylistic — there is no "you may write twice if you also clear in between" alternative, and there is no in-place-update form for productive parameters. The full rule is in §6.13. The pattern most user code follows is *compute-then-commit*: the body computes its inputs into local slots, then performs a single productive `<-` write near its end. This pattern naturally satisfies the rule and supports the atomic-compound-construction guarantee (§7.11).
 
### 3.5 Reference Parameters
 
A reference parameter (`&name`) carries no write obligation: the callee may read it, write it, or do neither. The caller's pre-call slot must be initialized (the parameter is readable from the callee's perspective, so reading uninitialized would be a static error per §6.13's whole-slot tracking). Copy-restore at the call boundary preserves failure atomicity: any write the callee makes to the slot during its execution is committed back to the caller's slot only if the call succeeds; on failure the caller's slot reads identically to its pre-call state.
 
Reference parameters are the natural shape for in-place mutation (§6.9). A method that updates a counter, a parser that advances a position, a logger that mutates an output buffer — all use reference receivers or reference parameters. The contrast with productive mode is the obligation: PRODUCE commits to a write on every successful path, while REFERENCE commits to nothing.
 
The current parser implements reference mode partially: pointer-typed parameters (`^T`) with the postfix `^` and `&` operators serve where the dedicated `&`-marked reference mode is not yet wired through. The specification describes the intended language; the parser's current state is documented in the implementation's `task-handoff-failure-system-reference.md` and is not a constraint on this specification.
 
### 3.6 Implicit Context Parameters
 
Parameters listed after the `/` separator in a command's signature are *implicit context parameters* — the language's Scala-implicit-style mechanism for threading shared context through call chains. They are filled at the call site by *uniqueness-of-type* resolution against the caller's lexical scope: at the call point, the typechecker searches the caller's in-scope identifiers for one whose type matches the implicit parameter's declared type, and supplies that identifier as the argument. Ambiguity (two in-scope values of the same type) is a static error, resolvable by the caller passing the value explicitly. Absence (no in-scope value of the matching type) is also a static error.
 
```
.cmd writeAll: List[String] lines / Logger logger = ...
 
.cmd reportAll: List[String] lines =
    #log <- (Logger: "console")
    writeAll: lines           ; logger filled by uniqueness-of-type
```
 
The mechanism resolves at compile time. The provision-chain reading of the no-non-local-state principle (§1.4) treats implicit context parameters as a *syntactic* convenience — the value must actually exist in the caller's lexical scope to be eligible, so the implicit's resolution does not enable access to state that was not already accessible. The implicit list is not a way to reach ambient state; it is a way to thread accessible state through call chains without writing the parameter at every call site.
 
All three parameter modes are admitted in the implicit list. Implicit-list internal grammar — whether commas are required, how the slash binds against indentation, how the implicit list interacts with the `-> name` clause — is OQ-20, forwarded to the typechecker-implementation thread; the spec admits the form and forwards the syntactic detail.
 
### 3.7 The `-> name` Result Designator
 
A command invoked in *expression position* — as the right-hand side of `<-`, as a parenthesized argument in another command's call, or as a guard or scrutinee in a block-marker construct — produces a value that the surrounding context binds. The `-> name` clause names which already-declared parameter or receiver carries that expression-style value:
 
```
.cmd quotrem: Int 'q, Int 'r, Int n, Int d -> 'q =
    'q <- n / d
    'r <- n % d
```
 
A caller writes `#x <- quotrem: 'remainder, 10, 3` to bind the quotient to `x`, supplying `'remainder` as the slot for the remainder. The `->` clause does not introduce a new parameter — it designates which already-declared parameter participates in the expression-style sugar.
 
The clause is well-formed when `name` references a parameter or receiver of the command. The designated parameter is not required to be writeable. The expression-style value's type and reading depend on the designated parameter's mode:
 
- **PRODUCE `'name`** — the expression evaluates to the parameter's declared type; the value is the *post-write-back* value the callee committed to the caller's slot.
- **REFERENCE `&name`** — the expression evaluates to the parameter's declared type; the value is the *post-call* slot value, after copy-restore.
- **READ `name`** — the expression evaluates to the parameter's declared type; the value is the *initial* value the caller passed in. This admits a predicate-with-passthrough idiom — a may-fail check whose success makes the input value available to the next stage in an expression chain.
In all three cases, failure of the call propagates as expression failure, and the pre-call state of the caller's slot for `name` is preserved by failure atomicity.
 
### 3.8 Implicit `-> name` for Single-Writeable Commands
 
When a command has exactly one writeable parameter — productive (`'name`) or reference (`&name`) — that single slot is automatically the expression-style result, and no `-> name` clause is required. The implicit form applies the same per-mode rules of §3.7: the single writeable parameter's mode determines whether the expression value is the post-write-back value (PRODUCE) or the post-call slot value (REFERENCE).
 
```
.cmd square: Int x, Int 'result =
    'result <- x * x
 
#nine <- square: 3                ; 'result is the implicit expression-style result
```
 
When a command has two or more writeable parameters, the implicit selection is not available — the caller must specify which writeable parameter carries the expression-style result via an explicit `-> name`. When a command has zero writeable parameters, the command is not expression-callable unless an explicit `-> name` designates a READ parameter as the result.
 
This is a desugaring rule (per principle 10, §1.2): a single-writeable command with no `-> name` is treated as if it had `-> writeableParam` declared explicitly. The desugared form reaches the same typechecker rules; the only difference is surface convenience.
 
### 3.9 Constructors
 
A constructor is a command that produces a value of a specified type. Its signature names the type as the receiver, with the productive mode marker:
 
```
.cmd Widget 'w: Int x, Int y =
    'w <- ${x <- x, y <- y}
```
 
The receiver `Widget 'w` is the constructed-value slot. All other parameters (after the colon) are inputs. The body's job is to fill `'w`. The body typically uses an aggregate literal `${...}` (§7.4) to construct in place; the compute-then-commit pattern of §3.4 applies — the constructor performs whatever subordinate work is needed in local slots, then commits a single productive write to the receiver near the body's end.
 
Constructor receivers must be PRODUCE — the alternatives are not meaningful for the constructor signature shape. A REFERENCE receiver would describe in-place modification of an existing value, which is the method-with-REFERENCE-receiver case (§3.10). A READ receiver would describe constructing a value the caller cannot observe, which has no purpose. The receiver-mode-by-signature-shape table (§3.10) captures this restriction and the analogous restrictions for the other shapes.
 
A constructor is invoked by the type name as the call's prefix:
 
```
#w <- (Widget: 3, 4)              ; expression-style invocation
Widget: #w, 3, 4                  ; statement-style with receiver in argument position
```
 
The expression-style form omits the receiver from the argument list — the receiver slot is supplied by the surrounding `<-`. The statement-style form may supply the receiver as `#name` in argument position to introduce a fresh local that the constructor writes into; the two forms produce equivalent effects, and the choice is stylistic.
 
A `.implicit`-declared constructor (§7.8) is structurally a constructor with the additional property that the typechecker may insert calls to it automatically when a literal of the source type appears in a context expecting the constructed type. Apart from the additional registration, `.implicit` constructors share the constructor signature shape and the productive-receiver rule.
 
### 3.10 Methods with a Single Receiver
 
A *method* takes one or more receivers, then `::`, then the command name and any further parameters. With a single receiver — the common case — the receiver appears bare, without parentheses:
 
```
.cmd Logger logger :: log: String message = ...
.cmd Counter &c :: increment = c <- c + 1
.cmd Buffer 'b :: clear = 'b <- ${}
```
 
The receiver carries an explicit mode marker (`'`, `&`, or no marker for READ); there are no implicit defaults. Method receivers admit all three modes — PRODUCE, REFERENCE, and READ — each with a distinctive idiomatic use:
 
- **READ receiver — externalized effect.** The method operates *through* the receiver without modifying its slot. Logging is the canonical case: `myLogger :: log: "ready"` writes a log entry; the logger's state is unchanged from its caller's vantage, while the world (the log file, the stream) is changed. The receiver mediates an effect external to itself.
- **REFERENCE receiver — in-place modification.** The method may read the receiver's current state and may write to it. State transitions on objects, in-place updates, the "modify if needed" idiom — all use REFERENCE receivers.
- **PRODUCE receiver — re-initialize the receiver.** The method commits to writing the receiver on every successful return path. Combined with the call-site initialization rule (every receiver is initialized at the call site, regardless of mode — the *R1* rule of §6.7), the receiver is overwritten on success — a "factory method" or "complete reset" operation that happens to dispatch on the receiver's existing type. Unusual but coherent.
A method invocation uses the same `::` syntax, parenthesized only for multi-receiver methods (§3.11):
 
```
myLogger :: log: "ready"
counter :: increment
```
 
Methods dispatch on the runtime type of their receiver — a method named `log` is resolved against the receiver's type's class instance for the relevant class, with the dispatch site syntactically marked by `::`. The full dispatch mechanics — the witness-slot model, single-class dispatch, the dictionary structure, instance coherence — are in §9.
 
The receiver-mode-by-signature-shape table summarizes the per-shape mode restrictions:
 
| Signature shape | Valid receiver modes | Forbidden modes |
| --- | --- | --- |
| Constructor | PRODUCE `'` only | READ, REFERENCE |
| Method (single- or multi-receiver) | PRODUCE `'`, REFERENCE `&`, READ | none |
| At-stack `@` | REFERENCE `&`, READ | PRODUCE |
| At-stack `@!` | REFERENCE `&`, READ | PRODUCE |
 
The reasoning for the at-stack restriction is in §3.12; the constructor case is §3.9 above. Receivers are always carried at the marker placement of the named-context rule (§3.3) — `Type 'name`, `Type &name`, or `Type name`. The full receiver-mode discipline including the *R1* (call-site initialization) and *R2* (callee-body obligation) rules is in §6.7.
 
### 3.11 Multi-Receiver Methods
 
A method invocation over multiple receivers takes a tuple of receivers, parenthesized, dispatching based on all their types in concert:
 
```
.cmd (Logger logger, Severity severity) :: format: String 'result, String message =
    ; body authored against Logger and Severity classes;
    ; (logger :: emit) and (severity :: prefix) dispatch independently
    ...
 
(consoleLogger, warning) :: format: 'output, "couldn't open file"
```
 
The receiver tuple appears in parentheses both at declaration and at call. Multi-receiver methods are sometimes called "v-commands" in older project material (the term is parser-internal); the canonical term is *method* with the multi-receiver structure noted when the receiver-tuple shape needs to be emphasized.
 
The dispatch implementation composes per-receiver single-class dispatches — there is no joint-instance dictionary keyed on the tuple of receiver types. The combined behavior is the product of the receivers' types, but each receiver's dispatch resolves through its own class's dictionary independently. This admits methods that span receivers from different modules without requiring those modules to coordinate: the implementations of `Logger`'s `emit` method and `Severity`'s `prefix` method are looked up separately at the call site.
 
Each receiver in a multi-receiver method declaration carries its own mode marker. Different receivers may carry different modes — `(Logger logger, Counter &c)` is a valid receiver tuple with logger READ and counter REFERENCE. The R1 (call-site initialization) and R2 (callee-body obligation) rules apply to each receiver independently per its declared mode.
 
### 3.12 Frame-Exit Hooks: `@` and `@!`
 
A class or module may declare methods that run at frame exit. The forms are:
 
```
.class Resource:
    .decl Resource 'r: String name             ; constructor
    .decl @ Resource &r                        ; runs at frame exit (any path)
    .decl @! Resource &r                       ; runs at frame exit on failure only
```
 
`@` is read "at exit"; `@!` is "at exit on failure." The handler executes when the *frame slot* holding the value retires, not when the value itself is "destructed" — the language has no per-value destruction concept. A value buried inside a record's field, an object's field, or any other non-frame container does not fire its `@` handler when that container retires. Only values in frame slots fire their handlers.
 
This is more restrictive than C++'s destructor mechanism, which fires transitively through member fields of destroyed objects. It is intentionally so — the language tracks frame ownership for the purposes of the at-stack discipline, and the more restrictive rule keeps the model uniform with the no-hidden-control-flow principle (§1.4): the at-stack mechanism is *the* exception to "no hidden control flow," and the exception is contained to a single, type-visible category.
 
The handlers fire in **reverse order of registration** — the most-recently-introduced handler runs first. They compose with the failure system: if a frame is retiring on a failure path, the propagating failure remains in flight while the handlers run, then continues to propagate after they complete. Handlers themselves may invoke commands that fail internally and recover internally; what they may not do is generate a *new* in-flight failure during the exit cleanup, which would conflict with the single-in-flight invariant (§4.12).
 
`@`-handlers and `@!`-handlers are not destructors and should not be called such. They are RAII-equivalent in functional role; they are not RAII in mechanism. The drift toward "destructor" framing has occurred in earlier project material and was corrected; this specification uses "frame-exit hook" for the mechanism and "at-stack handler" for an instance of it firing.
 
The receiver-mode restriction for `@` and `@!` shapes is REFERENCE or READ — never PRODUCE. The at-stack handler runs on an existing object (the frame's slot is initialized; that's the precondition for retirement); productive mode would mean "construct the object as part of cleanup," which is meaningless. READ at-stack receivers are useful for observation-only cleanup; REFERENCE at-stack receivers are the typical case for resource-managing types.
 
The `@` and `@!` markers also appear inside command bodies as **block markers** that register an inline frame-exit hook against the enclosing command's frame:
 
```
.cmd processFile: String path =
    #handle <- openFile: path
    @ closeHandle: handle           ; runs whether we succeed or fail
    @! logIncomplete: path          ; runs only on failure path
    process: handle
```
 
The body-level forms have the same firing semantics as the class-declared forms — reverse-order at frame exit, `@!` only on failure paths — and compose into the same registration list. They are convenient for one-off cleanup at a specific call site without declaring a class method. The full block-marker semantics for `@` and `@!` are in §4.4.
 
The frame-exit hook is the language's mandatory framing mechanism for resource-managing types: where a type carries a class declaring `@` (or its full RAII-equivalent surface), instances must be in frame slots to participate in the discipline. The language enforces this by requiring such types to be introduced via the `#` form in a frame, not buried in long-lived containers; the precise rule is in §6.13's whole-slot-tracking discussion.
 
### 3.13 Calling Commands
 
A command invocation has four surface forms by signature shape, distinguished by the prefix preceding the colon-separated argument list:
 
- **Regular call** — bare command name as prefix:
  ```
  process: arg1, arg2
  ```
 
- **Constructor call** — type name as prefix:
  ```
  Widget: # 'w, x, y                ; statement-style
  #w <- (Widget: x, y)              ; expression-style, parenthesized
  ```
 
- **Single-receiver method call** — receiver, `::`, command name:
  ```
  myLogger :: log: "ready"
  counter :: increment
  ```
 
- **Multi-receiver method call** — parenthesized receiver tuple, `::`, command name:
  ```
  (consoleLogger, warning) :: format: 'output, "couldn't open file"
  ```
 
The argument list follows a `:` after the prefix. Inter-argument commas are required, even when arguments span multiple lines under indentation:
 
```
process: arg1,
    arg2,
    arg3
```
 
The "optional commas under indentation" form does not exist in v1; arguments are always comma-separated. The grammar treats the colon as the start of the argument list and the indentation/dedent transitions as standard whitespace, so the argument list can extend across lines when indented under the call's first line.
 
The `#` prefix on an argument introduces a fresh local in argument position. The form is the call-site dual of `#name <- expr` for local introduction (§7.1):
 
```
quotrem: #remainder, 10, 3         ; #remainder introduces a productive-bound local
```
 
The form is naturally suited to PRODUCE-mode parameter slots: the local is uninitialized at introduction and is filled by the call's productive write on success. At READ-mode parameter slots, an existing in-scope identifier or expression is supplied in the argument list rather than a fresh introduction. At REFERENCE-mode parameter slots, the caller-slot is required pre-initialized, so the local must be introduced and initialized by an earlier statement before the call.
 
Implicit context parameters (§3.6) are not supplied at the call site; they are filled by the typechecker from the caller's lexical scope. The call's argument list covers the regular parameters only.
 
For a command in expression position, the writeable parameter designated as the expression-style result (§3.7) is *omitted* from the argument list. The remaining parameters are supplied in declaration order:
 
```
.cmd quotrem: Int 'q, Int 'r, Int n, Int d -> 'q = ...
 
#x <- quotrem: 'remainder, 10, 3   ; 'q omitted (it's #x); arg list is 'r, n, d
```
 
REFERENCE-mode parameters always appear in the argument list — they cannot be omitted via the expression-style sugar, because the caller's slot is required pre-initialized regardless of whether the command is invoked as a statement or as an expression.
 
### 3.14 The `_` Placeholder
 
The `_` token is a single-character placeholder serving four distinct uses across the language. The uses are syntactically disjoint — context determines which is meant — and are listed here for completeness; the relevant sections cover each role in detail.
 
- **Discard at PRODUCE positions.** At a productive parameter position in a call, `_` says "I don't care about this output." The productive write happens (the callee's contract is unchanged), but the caller declines to bind the result.
  ```
  #x <- quotrem: _, 10, 3            ; remainder discarded; quotient bound to x
  ```
  Valid only at PRODUCE positions in a call's argument list. Not valid at READ or REFERENCE positions: a READ position has no result to discard, and a REFERENCE position requires an initialized caller-slot, which `_` does not supply.
- **Partial-application deferral.** Inside a command-reference form (§8.2), `_` marks a parameter as deferred — not bound at the partial-application site, supplied at later invocation:
  ```
  {add: 5, _}                        ; first argument bound, second deferred
  ```
 
- **Variant absent state.** In aggregate-literal positions for variant-typed fields, `_` stands in for the absent-state value (§5.13, §7.16):
  ```
  Container: ${data <- payload, optional <- _}    ; optional field in absent state
  ```
 
- **Variant absent test and reset.** With the `-<` operator, `_ -< v` tests whether `v` holds a non-absent state, and `v -< _` resets `v` to its absent state (§7.14):
  ```
  ?- _ -< shape                      ; runs when shape is absent
      handleAbsent
  shape -< _                         ; reset shape to absent
  ```
 
The four uses are syntactically distinguishable by surrounding context. The lexer treats `_` uniformly as a single token; the parser routes it to the appropriate non-terminal based on its position.
 
### 3.15 First-Class Commands
 
Commands are values. A command-typed value may be stored in a field, passed as an argument, returned from a constructor, or invoked indirectly. The language admits four constructional forms that produce command-typed values:
 
| Form | Surface | Captures? | Body? |
|---|---|---|---|
| Command reference | `{name}` or `{cmd: x, _, y}` | No | No (refers to existing command) |
| Command literal | `:<args>{body}` (also `?<...>`, `!<...>`) | No | Yes |
| Lambda | `:<args / caps>{body}` | Yes (explicit slash list) | Yes |
| Fexpr | `:{body}` (also `?{body}`, `!{body}`) | Yes (implicit by free name) | Yes |
 
The four forms cover the design space — function-pointer-style references, eagerly-evaluated thunks, closures over defining-frame state, and user-defined control-flow primitives — through a uniform set of constructions that share the failure-mode discipline (the `:` / `?` / `!` mark) and the four-way taxonomy.
 
Command-typed values are typed by command-type expressions of the form `:<paramTypes>`, `?<paramTypes>`, or `!<paramTypes>`, with mode markers as suffix on each parameter type per the nameless-context rule (§3.3): `:<Int, Int'>`, `?<String'>`, `!<>`. Fexpr-typed values are typed by the parallel family `:<*>`, `?<*>`, `!<*>` — the `*` distinguishes the fexpr family from ordinary command-typed values, and there is no subsumption across the family boundary. The full type-form tables and family rules are in §5.15 and §5.16.
 
Receivers are *always* applied at the partial-application site for command references (`{logger :: log}` resolves dispatch and bakes the receiver in immediately); non-receiver parameters may be applied or deferred (`_`). The full mechanics of partial application — including the mode-marker filter (PRODUCE deferred-only, REFERENCE applied with ceiling-tracking, READ flexible) — are in §9.14.
 
The four constructional forms — their capture rules, their ceiling computations, their mark-conformance rules, and the seven fexpr-restrictions A–G — are detailed in §8. The class-system extension that admits fexpr-typed parameters under fexpr-relevance taint is in §9.20.
 
---
 
## 4. Failure and Recovery
 
A *failure* in Basis is a propagating signal that some path of execution did not reach its intended outcome. Failures are first-class control flow, not an out-of-band channel: they are produced by the `.fail` directive, propagate up the call stack by skipping subsequent siblings at each indentation level, and are consumed by recovery contexts at well-defined source positions. Every command's signature declares what failures it admits and how it composes with callers; the typechecker's failure-state lattice (§4.13) verifies the declarations.
 
This section describes the failure-handling surface end-to-end: what a failure carries (§4.1), how the three failure-mode marks compose (§4.2), how `.fail` produces a failure (§4.3), how the eleven block markers enumerated in §3.1 dispatch failure flow (§§4.4–4.5), how recovery handlers match and bind failure values (§§4.6–4.8), how messages are declared in hierarchies and propagated through signature-level failure sets (§4.9), how a recovered value can be re-emitted as a fresh failure (§4.10), how at-stack handlers compose with failure flow (§4.11), why at most one failure may be in flight per thread (§4.12), and how the typechecker's six-state failure-state lattice gates body conformance (§4.13). The full transfer-function table for the lattice is in Appendix E; the operational rules for failure propagation are in Appendix F.
 
### 4.1 Failures Are Not Exceptions
 
A Basis failure is not an exception in the C++/Java/C# sense. There is no stack-unwinding mechanism that runs ad-hoc cleanup as it climbs frames, no try/catch construct, no special "exception state" lying outside the language's normal semantics. Failures are an ordinary state in a six-state lattice (§4.13), and propagation is structural: the failure-state lattice transitions at each block marker according to a transfer function that can be read off the source position, and propagation past a frame's body is by skipping subsequent siblings at each indentation level — not by unwinding.
 
A failure carries two pieces of information: an identity (the **message** — see §4.9) and an optional **payload value** whose type satisfies the message's bound class (§4.7). The message names the kind of failure; the payload carries any data the failure needs to convey to a downstream recovery handler. Both are stored in a fixed-size **failure slot** allocated as part of each command's frame; the slot is occupied only while a failure is in flight. Propagation copies the slot's contents up the stack — three words on a 64-bit target: the message identifier, a pointer to the payload's storage, and a class witness for the payload's concrete type with respect to the message's bound class — without copying or moving the payload value itself. The payload stays put until a recovery handler binds it (§4.6), at which point it moves into the handler's frame.
 
Cleanup that mainstream languages express through `try`/`finally` is expressed in Basis through frame-exit hooks `@` and `@!` (§§3.12, 4.4, 4.11). The hooks compose with the failure flow — `@` runs on every frame exit regardless of outcome, `@!` runs only on failure exits — but they are not part of the failure machinery itself. A frame's hooks fire on its retirement schedule; the failure that may be in flight at retirement is a separate thing the hooks neither see nor consume.
 
### 4.2 The Three Marks
 
Every command-typed value in Basis carries one of three **failure-mode marks** as part of its visible signature:
 
- The `:` mark — the **never-fails** mark — denotes a command whose body must reach exit on a non-failing path on every reachable execution. The `:` mark is the absence of any prefix on the command's name; a never-fails command is declared `.cmd name: ...`.
- The `?` mark — the **may-fail** mark — denotes a command whose body may either succeed or produce a propagating failure. A may-fail command is declared `.cmd ?name: ...`. The mark applies to every signature position where a callable appears: command-type slots, class-method declarations, lambda and fexpr signatures, and methods are all so marked.
- The `!` mark — the **must-fail** mark — denotes a command whose body must reach exit on a propagating-failure path on every reachable execution; no successful return is possible. A must-fail command is declared `.cmd !name: ...`. The `.fail` directive itself (§4.3) is the canonical `!`-call.
The marks classify command-typed values uniformly across all of the language's command-bearing constructs: named `.cmd` declarations, command-typed slots (`:<...>`, `?<...>`, `!<...>`), class methods, single- and multi-receiver methods, lambdas, and fexprs (§§3.10–3.11, 5.15–5.16, 8). The same conformance rules apply to all.
 
The marks form a partial order under subsumption: `: ⊑ ?` and `! ⊑ ?`, with `:` and `!` mutually incomparable. A `:`-marked or `!`-marked value may stand wherever a `?`-marked value is expected; a `?`-marked value may not stand for either of the more-specific marks. The reading is natural: `?` is the "may-or-may-not" supremum, with `:` and `!` as the two "definitely" specializations of it. A `:`-value at a `?`-call site is simply more useful — its failure edge is statically dead — but the call site's analysis is still that of a `?`-call. Subsumption permits the substitution; it does not narrow the analysis.
 
Parameter modes (§3.3) and parameter types are **invariant** under mark subsumption. A `:<Int 'r>` value is not interchangeable with `:<Int>` or `:<Int &r>`; the subsumption relation is solely on the failure mark. Invariance is essential for soundness: the per-mode discipline at the call site (productive obligations, reference-initialization preconditions, READ-taint contracts) breaks if the mode is permitted to vary.
 
### 4.3 The `.fail` Directive
 
A failure is produced by the `.fail` directive, which has three surface forms:
 
```
.fail
.fail Name
.fail Name: payload
```
 
A bare `.fail` produces a payload-less, message-less failure. `.fail Name` produces a failure carrying message `Name` with no payload. `.fail Name: payload` produces a failure carrying message `Name` with the value of `payload` as its payload; the typechecker requires that `payload`'s type satisfy the class bound to `Name` (§4.7).
 
The `.fail` directive is itself a `!`-call: its post-call lattice state is `failing(!)` carrying the propagating set `{Name}` (or empty, for bare `.fail`). The next ordinary statement in the surrounding block is reachable only along the failure-skip path, which means: not at all, unless caught by a recovery context.
 
Payload expressions whose surface contains `:` must be parenthesized to disambiguate them from a continuation of the `.fail` directive's syntax:
 
```
.fail Net::Disconnected: (lastError: u)
```
 
Without the parentheses, the parser cannot determine where the payload expression ends and a continuation of the directive begins.
 
Two forms are explicitly disallowed: a `.fail` whose right-hand side is a raw value with no message (`.fail (a + b)` — a value cannot carry the role of a failure message), and a `.fail` that uses a payload type's name in message position (`.fail Widget: x, y` — `Widget` is a type, not a message identifier; the constructor call `(Widget: x, y)` produces a payload value, but a separate message identifier is needed). Both are message-required-when-payload-present violations.
 
Re-failing a recovered value — passing a `|`-with-spec-bound payload forward as the payload of a fresh failure — is an ordinary `.fail` invocation in the handler's frame; it requires an explicit message because the recovery binding captures only the payload, not the original message. The full mechanics are in §4.10.
 
The exact surface syntax for `.fail` with payload, for the recovery-spec form, and for the corresponding signature-level failure-set declarations is **open**. The forms shown in this section are placeholders; the eventual surface may differ in punctuation, position, or both. *Open question: OQ-26.2.* See Appendix I.
 
### 4.4 Block Markers
 
Eleven block markers govern failure flow. They are enumerated as a syntactic list in §3.1, with `|` and its parametrized `|`-with-spec form treated as a single marker for the enumeration; this section enumerates all eleven by their **failure-mode role** — what each one consumes, what it permits to propagate, and where it places execution after consumption.
 
#### Guard-only recovery: `?`, `?-`, `?:`
 
The three guard-bearing markers consume their *guard*'s failure but not the failures of statements within their body.
 
`?` (DO_WHEN) runs its guard; on guard success, the body runs; on guard failure, the body is skipped and the failure is consumed. Either way, control proceeds to the marker's next sibling. Statements within the body have their failures propagate per the surrounding context.
 
`?-` (DO_WHEN_FAIL) is the mirror: the body engages iff the guard fails, and the failure is consumed regardless of which branch ran. Critically, `?-` consumes the *fact* of failure, not the *value*: programs that need access to the propagating failure value should use `|`-with-spec instead.
 
`?:` (DO_WHEN_SELECT) is the chaining variant. Multiple `?:` siblings at the same indentation level form a coherent multi-branch construct: the first guard to succeed engages its body and skips the rest of the chain. When a `?:` body engages on guard success, control exits the surrounding indentation level — making `?:` chains the natural primitive for if/elseif/else cascades, with a final non-`?:` sibling serving as the default arm. The chain has no syntactic wrapper; it is recognized by adjacency.
 
For all three, only the guard's failure is consumed. A recovery handler placed as a sibling of a `?`, `?-`, or `?:` block does not engage on the guard's failure — that failure has already been consumed — and engages only on failures from non-guard operations elsewhere in the surrounding scope.
 
#### Full-body recovery: `^`
 
`^` (DO_REWIND) is a sibling whose body controls whether execution loops back to the preceding sibling. When control reaches `^`, the body runs; on body success, control jumps to the preceding sibling at the same indentation level (the loop continues); on body failure, the failure is consumed, the body terminates at the failed statement, and control falls through to the marker's next sibling (the loop terminates).
 
Unlike the guard-only markers, `^` recovers *every* failure within itself. Every failing statement consumes its failure into `^`. This makes the body a unified test-and-adjust region: comparisons compose freely with adjustments, with the first-failure determining loop termination.
 
The body is optional. A bodiless `^` is treated as if its body had succeeded, producing an unconditional rewind to the preceding sibling. The bodiless form is the cleanest expression of "loop until the guard fails" and pairs naturally with `?? ?` to form the canonical loop (§4.5).
 
A `^` with no preceding sibling at the same indentation level is a static error: the rewind has no destination.
 
#### External-failure recovery: `|` and `|`-with-spec
 
`|` (DO_RECOVER) and its parametrized form are the only constructs that consume failures propagating from *preceding siblings*.
 
A bare `|` engages when a failure is propagating from any preceding sibling at the same indentation level. If no failure is propagating, the block is skipped. On engagement, the failure is consumed immediately and the body executes from no-active-failure. The body is not itself a recovery context; statements within it propagate failures normally. The bare form does not bind the failure value.
 
`|`-with-spec adds two refinements: filtering and binding. The spec acts as a predicate on the propagating failure's message; the block engages only if the message matches. Non-matching failures continue propagating *as if the block were not there*. When the block engages, the spec's identifier is bound to the payload value — not to the message and not to the whole failure — within the body's scope. The placeholder surface used in this section is:
 
```
| Name name -> body
```
 
Cascade chains of `|`-with-spec blocks (each filtering for a subset of messages, optionally followed by a bare `|` as catch-all) are the structural form for "handle this kind of failure here, that kind there, anything else uniformly." The first whose spec matches engages; later blocks in the cascade are also recovery destinations for *preceding* blocks' bodies, which is what makes the cascade compose. Programmers place more-specific specs first and broader specs last.
 
Matching is on **message identity** with subtype-inclusive semantics: a `| SomeRoot t -> body` block engages on any message at-or-below `SomeRoot` in `SomeRoot`'s hierarchy (§4.9). Value-style matching against payloads — `| 0 -> handleZero`, `| "fish" -> handleFish` — is not part of the language; recovery is on message identity only.
 
The surface syntax is open per OQ-26.2; the form shown above is a placeholder.
 
#### Escape elevator: `??`
 
`??` (DO_WHEN_MULTI) is a meta-marker. It pairs with a `?`-family block (a `?` or a `?-`) as its first executable inner construct, and modifies *where* the inner block's guard-failure recovery resumes execution. Without `??`, the inner guard's failure is consumed by the inner block and execution resumes at the next sibling of the inner block. With `??`, the failure is still consumed (by `??` itself), but execution resumes at the next sibling of the `??`-block — one indentation level higher.
 
`??` operates at a fixed elevation of one level; there is no `???` or further-elevating construct. Bare `??` (with no inner `?` or `?-`) is a static error.
 
The crucial corollary: a `|` placed as the next sibling after a `??`-block does **not** engage on the inner guard's failure, because that failure has already been consumed by `??`. There is no propagating failure for `|` to catch. A `|` after `??` engages only on failures from non-guard operations within the `??`-block.
 
#### Branch: `-`
 
`-` (DO_ELSE) attaches only to a preceding `?` or `?-` at the same indentation level. With `?`, the body engages on guard success and `-` engages on guard failure (standard if/else); with `?-`, the body engages on guard failure and `-` engages on guard success (inverted if/else). The `-`-body is not a recovery context: failures within it propagate per the enclosing rules. The branch consumes nothing; it merely supplies the alternative path.
 
`-` does not chain; for n-ary branching, use `?:`.
 
#### Compound grouping: `%`
 
`%` (DO_BLOCK) on its own has no control-flow effect: its body executes as ordinary sequential statements, and any unrecovered failure inside propagates out. This makes `%` directly usable as the **guard** of a `?`, `?-`, or `?:` block — when a `%`-block stands in guard position, the entire `%`-body must succeed for the guard to succeed.
 
The `%`-body provides logical conjunction without a new construct. Within a `%`-guard, `|`-chains supply disjunction: a `|` engaging on the immediately preceding statement's failure consumes that failure on engagement, so a sequence like
 
```
% testA: ...
  | testB: ...
  testC: ...
  | testD: ...
  | testE: ...
```
 
means `(testA OR testB) AND (testC OR (testD OR testE))`. Each `|`-chain locally resolves a disjunction; only the failure that ultimately escapes — from a conjunct whose disjunctive alternatives were exhausted — is what the enclosing `?`-family construct sees. The failure value surfaced is the failure of the last-tried alternative in whichever conjunct gave up.
 
`%` does not combine with `??` (no failure-escape to elevate) and does not pair with `-`.
 
#### Frame-exit hooks: `@` and `@!`
 
`@` (DO_ON_EXIT) registers a body that runs at frame exit, regardless of whether the frame exits via success or failure. `@!` (DO_ON_EXIT_FAIL) registers a body that runs only on failure exits. Neither construct sees nor consumes the propagating failure; the bodies execute as if no failure were active, even when the frame is exiting via failure.
 
The `@` and `@!` markers are **frame-lifetime-tied, not object-lifetime-tied**. They are tied to the frame slot holding the value, not transitively to fields of containing aggregates. This is the language's only intentional exception to the no-invisible-control-flow principle: an at-stack method on an object's type is conceptually a `@` or `@!` block the compiler inserts implicitly at the object-introducing position. The discipline that governs *when* these handlers fire under a propagating failure — particularly the corrected at-stack discipline for payload values — is in §4.11.
 
### 4.5 Block-Marker Composition
 
Block markers compose by adjacency at the same indentation level. The composition rules for the eleven markers are summarized below; the operational details of each marker individually are in §4.4.
 
| Marker | Combines with `??`? | Combines with `-`? | Body recovers failures? |
| --- | --- | --- | --- |
| `?` | yes (elevates guard escape) | yes (if/else) | guard only |
| `?-` | yes (elevates guard escape) | yes (inverted if/else) | guard only |
| `?:` | no | no (chain with more `?:`) | guard only |
| `-` | (paired only) | n/a | no |
| `%` | no | no | no (body propagates; usable as compound guard) |
| `^` | n/a (`^` is sibling, not inner) | no | yes (entire body) |
| `\|` | yes (catches non-guard `??`-block failures) | no | no |
| `\|`-spec | (same as `\|`) | no | no |
| `??` | (meta; inner is `?` or `?-`) | n/a | consumes only inner guard |
| `@` | no | no | no (runs alongside, doesn't recover) |
| `@!` | no | no | no (runs alongside, doesn't recover) |
 
The **canonical loop pattern** is `?? ?` paired with a sibling `^`, with `^` placed *inside* `??`'s body — not as a sibling of the `??`-block at the outer level:
 
```
?? ? hasMore: queue
        process: pop: queue
    ^
```
 
When `hasMore: queue` fails, `??` elevates past its own block, exiting the loop. When the `^` body succeeds (or, for the bodiless form, unconditionally), control rewinds to the `?` line, re-running the guard. Both paths converge past `??`. Placing `^` at the outer level — as a sibling of the `??`-block — is wrong: `^`'s preceding sibling would be whatever came before `??`, not the loop body, so the rewind would not iterate the right things.
 
A failure propagating out of a sibling-block at the same indentation level **skips subsequent siblings**. This is the fundamental composition rule that makes recovery contexts adjacent-sibling-shaped: if a body containing `acquire / loop / release` produces a failure inside the loop, the failure skips `release`, which is why cleanup that must run on every exit path is expressed as `@` or `@!` rather than as a sibling. The skip behavior also explains why a `|`-with-spec cascade composes — each `|` is a destination only for failures from earlier siblings, so non-engaging blocks let the failure continue to the next.
 
### 4.6 Typed Recovery
 
A `|`-with-spec block engages on failures whose message matches the spec, with subtype-inclusive matching against the relevant hierarchy. The placeholder surface is `| Name name -> body` (the final form is OQ-26.2). The spec engages on:
 
- A failure whose message is exactly `Name`.
- A failure whose message is a child of `Name` in `Name`'s hierarchy (recursively, to any depth).
Failures whose messages are not at-or-below `Name` continue propagating past the block. The bound name `name`, when present, is in scope throughout `body` and refers to the **payload only** — not the full failure, not the message identifier.
 
The presence of the bound name in the spec is governed by an asymmetric rule against the message's declaration. A payload-bearing `Name` admits the bound name (binding the payload) or its omission (engaging without binding; the engaged failure's payload is consumed unobserved). A payload-less `Name` requires the bound name's omission — supplying one is a static error, since there is no payload to bind to. The asymmetry permits the pattern of catching a class of failures uniformly without inspecting their payloads, while excluding the malformed case of a binding with no value behind it.
 
Match-by-message-identity, not match-by-payload-value, is a design commitment: failures are dispatched on what they *are*, not on what they *carry*. Forms like `| 0 -> handleZero` or `| "error" -> handleError` are not part of the language. Programs that need to distinguish payload values do so by inspecting the bound payload within the recovery body.
 
When a `|`-with-spec block engages, the failure's status as in-flight ends at the moment of binding. The handler body runs from no-active-failure, with the bound payload available for inspection, consumption, or forwarding into a re-fail (§4.10). The discipline governing the *value's* lifetime — distinct from the failure's status — and the timing of any at-stack handler on the bound value is in §4.11.
 
### 4.7 Class-Bound Payloads
 
A failure-message declaration may specify a **payload class** — a Haskell-style typeclass that any payload of that message must satisfy. The class is a *contract*: a set of operations the payload value supports, not a concrete type with a layout. The class itself has no `sizeof`, no allocation footprint, and no instances of its own; *types* satisfying the class are the things with layouts.
 
The corollary: different `.fail` sites for the same message may pass payload values of different concrete types, all satisfying the bound class. Refactoring the concrete type at one `.fail` site to a different type that also satisfies the class is non-breaking; refactoring the class itself — adding, removing, or changing operations — is breaking. The boundary is the class contract.
 
A class **witness** — a typeclass dictionary corresponding to the (concrete-payload-type, bound-class) pair — travels with the propagating failure as the third word of the failure slot (§4.1). The witness is selected at the `.fail` site: at compile time, the typechecker has both the concrete type of the payload expression and the bound class from the message's declaration, and emits the corresponding dictionary as a witness pointer. No runtime witness construction is required.
 
A `|`-with-spec consumer that binds a payload uses the witness to dispatch class operations on the bound name. The witness's identity — which concrete type's dictionary it is — is **opaque** to the consumer, which sees only the class's operations. This is the core of the failure system's open-extensibility property: a recovery handler defined against a class can consume payload values of types it has never heard of, as long as those types satisfy the class.
 
The full class system (typeclass declarations, instance declarations, dispatch mechanics, witness flow) is in §9. Failure messages and their bound classes participate in that system on identical terms; the failure machinery is a refinement of class dispatch applied to propagating failure values, not a separate parallel mechanism.
 
### 4.8 Messages Without Payloads
 
A failure message may be declared without a payload class. A `.fail` for such a message carries no value — the directive form is `.fail Name` with no following `:` or value — and a `|`-with-spec recovery for such a message has no bound identifier. The body of the recovery simply runs.
 
A payload-less message retains nominal identity, hierarchy position (§4.9), and full participation in failure-set inclusion. Its composition with payload-bearing messages within a hierarchy is constrained by recovery soundness: a handler `| ParentMsg p -> body` that binds `p` and applies `ParentMsg`'s class operations to it must remain sound for every descendant message that could engage the handler. Two rules follow.
 
**A payload-bearing child of a payload-less parent is well-formed.** The child introduces a payload class at its level; payload-less ancestors impose no class constraint on it. A `|`-with-spec at a payload-less ancestor produces a payload-less handler (no binding identifier) whose body simply runs when engaged — the engaged descendant's payload, if any, is consumed unobserved.
 
**A payload-less child of a payload-bearing parent is ill-formed.** A handler `| ParentMsg p -> body` would attempt to bind `p` to the engaged message's payload and apply `ParentMsg`'s class operations to it; a payload-less descendant engaging that handler would leave `p` with no value to bind. Once a payload class is introduced at some level in a hierarchy, every message at-or-below that level must also declare a payload class.
 
The soundness argument extends across multi-level chains, requiring **payload-class covariance** along ancestor-descendant paths: every payload-bearing message's class must be a subclass of (or equal to) every payload-bearing ancestor's class along the path from the message toward the root. The covariance chain ensures that a handler bound at any payload-bearing ancestor can apply its class operations soundly to any descendant message's payload, regardless of which descendant engages.
 
The recovery's binding shape is thus determined entirely by the spec: a payload-less spec produces a handler with no binding identifier; a payload-bearing spec produces a handler binding the engaged message's payload as a value of the spec's class, with class operations on the bound name dispatching through the engaged failure's class witness (§4.7).
 
### 4.9 Failure Messages and Hierarchies
 
Failure messages are declared with the `.msg` directive. The v1 surface is:
 
```
.msg Name[PayloadType] : ParentMessageType
```
 
Both the `[PayloadType]` brackets and the `: ParentMessageType` clause are optional. A message declared without a payload type is payload-less (§4.8); a message declared without a parent is the root of a new hierarchy. Examples:
 
```
.msg Net                                       ; root, payload-less
.msg Net::Disconnected[ConnectionInfo] : Net   ; child of Net, payload satisfies ConnectionInfo class
.msg Parse[String] : Net                       ; sibling of Disconnected, payload is a String
```
 
The exact surface syntax is tentative. *Open question: OQ-26.2.* See Appendix I.
 
Messages form a **forest** — a collection of independent hierarchies, each rooted at a user-declared root. There is no language-imposed Top message, no universal ancestor analogous to Java's `Exception`. This is a deliberate design commitment. A universal ancestor invites generic catch-and-bind-anything practice, the very pattern that makes failure handling sloppy in languages that have one. The forest structure forces programs that bind a payload to commit to which hierarchies they understand. Code that wants to catch anything without binding can use the bare `|` form — that idiom is preserved without admitting Top as a type.
 
A downstream module **cannot extend** the hierarchy of an imported message: it cannot add new children to a foreign root or to any imported descendant. The set of messages at-or-below a given message is fixed at the original declaration sites. This prevents a class of cross-module surprises, in which a `| SomeRoot t -> ...` handler in one module would be silently widened by another module adding new descendants to `SomeRoot`. The closure is **provisional**: legitimate cases (e.g., a library exposing an extensible hierarchy of error messages for downstream classification) are forbidden by the closed rule, and a relaxation under controlled conditions — perhaps an explicit "open hierarchy" declaration on the originating message — is being considered. *Open question: OQ-28.* See Appendix I.
 
A **failure set** is a set of messages. A command's signature carries a failure-set component declaring which messages the command may emit, expressed either as an explicit list or as the closure-at-or-below of a higher-level message. By convention, a set listing `Net` denotes "any message at-or-below `Net`"; a set listing several messages denotes the union of their respective at-or-below closures.
 
A set `A` **subsumes** a set `B` (notation `B ⊆ A`) iff for every message `m` in `B`, there exists a message `m'` in `A` such that `m` is at-or-below `m'` in `m'`'s hierarchy. Messages from different hierarchies are mutually incomparable; nothing in one hierarchy subsumes anything in another. The relation is reflexive, transitive, and antisymmetric (modulo at-or-below equivalence within a single hierarchy, which is trivial).
 
A caller invoking a command with declared failure set `D` is responsible for ensuring its enclosing signature's set `E` satisfies `D ⊆ E`, modulo any sets consumed by recovery within the caller. A caller may *widen* (`E` a superset of `D`) at the cost of precision; a caller may *not* narrow at a call site. A `|` consumes failures of certain messages: bare `|` consumes the entire propagating set; `| Name -> ...` consumes failures whose message is at-or-below `Name`. The post-block propagating set is the original set minus the at-or-below closure of the matched message.
 
How explicit a signature must be about its failure set is **partially open**. The tentative direction: failure sets are required-explicit at module boundaries (anything exported must annotate), with intra-module inference permitted (a callable defined and used within a single module need not annotate). The motivation is to make module-to-module contracts auditable while sparing intra-module code from boilerplate. *Open question: OQ-26.1.* See Appendix I.
 
A class method's failure set must be uniform across all instances. At a class-dispatched call, the typechecker knows the class and the method but not the instance; the failure set must therefore be a property of the (class, method) pair, not of the instance. An instance's implementation may emit any subset of the class-declared set; emitting a message outside the set is a static error.
 
A surface form for "my failure set is the union of my callees' sets, plus what I add directly" — needed for higher-order commands taking callable parameters — is not yet designed. Without it, every higher-order command must list every failure its callees might emit, with the combinatorial pressure familiar from Java's `throws`. *Open question: OQ-26.4.* See Appendix I.
 
### 4.10 Re-Failing
 
A common pattern is recovering a failure, deriving from its payload, and propagating a fresh failure with new identity:
 
```
| LowLevel value ->
    .fail DomainSpecific: (translate: value)
```
 
This is an **ordinary `.fail`** originating from the handler's frame; no re-fail-specific machinery exists in the language. The handler binds a payload (`value`); the new `.fail` is fired from the handler's frame with a freshly-named message and a payload of the handler's choosing. The original failure's identity is consumed at binding and does not survive.
 
The bound name binds the **payload only**, not the original message. To propagate the same message-and-payload combination unchanged, supply the message explicitly:
 
```
| FatalFailure f ->
    .fail FatalFailure: f
```
 
Forms that elide the message — `.fail f` to "re-raise" — are not valid: `f` is the payload, not the message; the directive cannot infer which message to attach.
 
When the new `.fail`'s payload is the bound name from a preceding `|`-with-spec, the value **moves** from its current holding frame (the recovery frame) into the new `.fail`'s originating frame (the handler's frame). The value's identity is preserved; only its holding frame changes. The witness for the new failure's slot is selected at the re-fail site at compile time, against the new message's bound class — which may differ from the original message's bound class. If the bound value's concrete type satisfies the original message's class but not the new message's class, the re-fail is a static error. The discipline composes cleanly: a chain of recover-and-re-fail handlers each performs a move into the next holding frame, until a final handler consumes the value without re-failing.
 
The value's lifetime, and any at-stack handler attached to it, is governed by the holding-frame model in §4.11. The corrected at-stack discipline is what makes re-fail chains coherent.
 
### 4.11 At-Stack Handlers Under Failure
 
The frame-exit hooks `@` and `@!` (§4.4) compose with failure flow as follows.
 
`@`-bodies run on every frame exit. When a failure propagates out of a body containing `@` blocks, the `@` bodies fire as part of the frame's retirement, in the order they were registered, before the failure propagates to the caller. The bodies execute as if no failure were active: they neither see nor consume the propagating failure. A failure produced *within* an `@`-body propagates through the cleanup machinery's own rules, not back into the original failure's flow.
 
`@!`-bodies run only on failure exits. The composition with the propagating failure is identical to `@`'s, restricted to the failure-exit case.
 
The discipline for **at-stack handlers on payload values** is more subtle, and bears close attention. A payload value may have an at-stack handler — for instance, releasing an underlying resource — registered as part of its concrete type's class instance for the message's bound class. The handler fires when the value's actual lifetime ends. **It does not fire at intermediate moves**, including not at binding and not at re-fail.
 
The motivating case: a payload whose class includes a `cleanup` operation registered as an at-stack handler. A recovery handler binds the value, inspects it, and decides to re-fail using it (passing the bound value forward as the payload of a new message). If `cleanup` had fired at binding, the bound value would be in a post-cleanup state at the moment the handler tries to re-fail; the re-fail's payload would be a corpse. The corrected discipline avoids this: `cleanup` fires at the actual end of the value's life, which is post-handler-body-exit (or post-final-consumption-of-the-re-fail-chain), not at any of the intermediate points where the value moves between holding frames.
 
The model that makes this tractable is the **holding frame**: at any moment, a payload value lives in exactly one frame, its holding frame. Initially the holding frame is the originating frame of the `.fail` that produced the value. Propagation up the call stack copies the failure slot's three words (message, pointer, witness) without moving the value — the holding frame does not change during propagation. The holding frame *does* change at two events: at **binding**, when a `|`-with-spec engages and the value moves into the recovery frame; and at **re-fail**, when a fresh `.fail` in the handler's frame takes the bound value as its payload and the value moves into the new originating frame. Both events are moves: the value's identity is preserved; only its holding frame changes.
 
The at-stack handler attached to the value travels with the value across these moves. It fires when the value's home actually retires — which means: when no further moves are pending. For a recovered failure whose handler completes without re-failing, that moment is at handler-body-exit. For a chain of recover-and-re-fail handlers, the at-stack handler fires at the eventual final consumption of the chain. For a failure that escapes to program termination, it fires at termination.
 
The corrected discipline is what permits values that own resources to be passed forward through recovery chains without those resources being prematurely freed. The cost is that at-stack timing is associated with the value, not with any single frame's retirement schedule; the implementation must track the value's holding-frame history along the failure-flow path, not merely register handlers at frame entry as it does for ordinary slots.
 
### 4.12 The Single-In-Flight Invariant
 
Basis maintains an invariant: **at most one in-flight failure exists per thread at any moment**. The invariant is preserved by the language's semantics: `.fail` is valid only when no failure is in flight at the firing point (a propagating failure would have failure-skipped past the `.fail` site already), and `|`-blocks consume the in-flight failure before any new one can be fired from the handler body. Multi-threaded programs may have one in-flight failure per thread; the invariant is thread-local.
 
A frame that has fired a failure does not retire on the normal "command returned" schedule. It enters a **deferred-retirement state**: the frame is still allocated, its payload-storage area still valid, but it is no longer executing. The deferred-retirement state ends at the moment of consumption — when a `|`-block somewhere up the stack engages and the value moves out — or at program termination. After consumption, the originating frame's payload-storage area is reclaimable (the value has moved); other resources held by the frame retire at the same moment, on the normal frame-retirement schedule. The single difference between deferred retirement and ordinary retirement is the timing: deferred retirement waits for consumption.
 
As a consequence of the single-in-flight invariant, at most one deferred-retirement frame exists per thread: the originating frame of the in-flight failure. The invariant is what permits the simple per-frame failure-slot representation (§4.1) to suffice; if multiple in-flight failures could coexist, the slot would need to be replaced by a per-failure structure with its own discriminator.
 
The single-in-flight invariant is also what makes the holding-frame discipline (§4.11) tractable: a payload value's holding-frame chain is a strict sequence — originate, propagate-and-bind, possibly re-fail-and-bind-again, eventually consume — with no interleaving of unrelated failures. The chain is a linear path through frames, never a tree.
 
### 4.13 Failure-State Lattice
 
The typechecker performs a forward-flow analysis over each command body's control-flow graph. At each program point the analysis maintains a state from a six-element lattice:
 
| State | Meaning |
| --- | --- |
| `clear` | No failure is propagating at this program point. |
| `failing(?)` | A `?`-mode failure is propagating. |
| `failing(!)` | A `!`-mode failure is propagating. |
| `mixed(?)` | Some incoming paths reach this point clear, others with `failing(?)` (possibly widened from `failing(!)`). |
| `mixed(!)` | Some incoming paths reach this point clear, others with `failing(!)`. |
| `unreachable` | No incoming path reaches this program point. |
 
The states are mutually exclusive: at any program point, the state is exactly one of the six. The lattice's bottom is `unreachable`, which is the identity for the join operation. Above it sit `clear`, `failing(!)`, and `failing(?)` as pairwise-incomparable peers. `mixed(!)` is the join of `clear` with `failing(!)`. `mixed(?)` is the supremum: every join involving a `?`-flavored failure or pairing a propagating-failure path with a clear path produces `mixed(?)`. There is no element above `mixed(?)`.
 
Two reading rules summarize the join behavior. **Widening of `!` to `?`**: whenever a `!`-flavored state is joined with a `?`-flavored state, the result is `?`-flavored. The `?` mark is the supremum among the three failure modes. **`clear` plus failing produces `mixed`**: a node reached by both a clear path and a failing path is reachable on both, and downstream code must consider both possibilities.
 
A command body is **conformant** with its declared mark when every reachable exit edge in its CFG analysis satisfies the state rule for that mark; unreachable exits impose no constraint:
 
- A `:`-marked body requires every reachable exit edge to have state `clear`. Any reachable exit in `failing(M)` or `mixed(M)` fails conformance — a propagating failure has reached the body's exit, contradicting the `:` declaration.
- A `?`-marked body imposes no constraint on the states of its reachable exits: they may be `clear`, `failing(?)`, `failing(!)`, `mixed(?)`, or `mixed(!)`.
- A `!`-marked body requires every reachable exit edge to have state `failing(?)` or `failing(!)`. Any reachable exit in `clear` or `mixed(M)` fails conformance: `clear` because the path is purely successful; `mixed(M)` because some path is successful, even if other paths are failing.
The duality between `:` and `!` is direct: `:` rejects every reachable-exit state with a propagating-failure contribution; `!` rejects every reachable-exit state with a successful-path contribution. The lattice was designed so these checks are simple state-membership tests.
 
Bodies whose every exit edge is unreachable — non-terminating commands, including the long-lived event loops typical of servers and other reactive systems — vacuously satisfy every conformance rule, since the universal quantification over reachable exits has an empty domain. Divergence is observationally consistent with all three marks: a caller of a `:`-marked diverging command never observes a failure, a caller of a `!`-marked diverging command never observes a successful return, and a caller of a `?`-marked diverging command observes neither — each consistent with what the corresponding mark promises.
 
Under typed failures (§4.9), the lattice refines: each `failing(M)` and `mixed(M)` state carries a propagating-set component, and the conformance check verifies that every exit edge's set is a subset of the body's declared failure set. The full transfer-function table — covering each block marker's state transitions, each call site's two-edge or one-edge pattern, and the joining rules at convergent CFG points — is in Appendix E. The integration with the initialization analysis (§6) is a parallel forward-flow analysis over the same CFG with the same join points; the two analyses are independent at transfer functions but share the walk.
 
A typechecker implementation may begin with the un-refined six-state lattice and add the failure-set component incrementally; dropping the failure-set component recovers the un-refined lattice exactly. *Open question: OQ-26.3 — whether `| Name name -> body` narrows the propagating set precisely (set minus at-or-below closure) or conservatively (no narrowing).* See Appendix I.
 
---

on 5 · MD
## 5. Types
 
This section defines the type forms of Basis. Every value the language admits inhabits exactly one of the forms enumerated here. The forms partition into two categories — buffer-backed (§§5.2–5.7) and non-buffer (§§5.10–5.15) — with three additional facilities (aliases §5.8, enums §5.9, and the cross-cutting subsumption rule §5.5) that operate over both. Each form's surface declaration syntax is given here in sketch; the full grammar is in Appendix B. Each form's construction surface — how values of the form are introduced and assigned — is in §7. Each form's interaction with parameter modes and class dispatch is in §6 and §9 respectively.
 
### 5.1 The Two-Layer Split
 
Every type in Basis falls into exactly one of two categories. The split is structural: it determines what positions a type may inhabit, how it is initialized, how it composes with other types, and how the static analyses treat it.
 
**Buffer-backed types** are types whose representation reduces — transitively — to bytes. Buffer-backed values are byte-copyable, fit inside other byte-aggregates, and are reclaimable at frame retirement without traversal. The category includes the buffer primitives `[N]` and `[]`, typed buffers `[N]T` and `[]T`, domains, records, and unions. Aliases that resolve to a buffer-backed type are buffer-backed; enums whose enumerated-type is buffer-backed are buffer-backed.
 
**Non-buffer types** are types whose representation includes references, identity, dispatch information, or other non-byte semantics. The category includes pointers, command-typed values (the `:<...>`, `?<...>`, `!<...>` family), fexpr-typed values (the `:<*>`, `?<*>`, `!<*>` family), objects, and variants.
 
The split is enforced by a **containment rule**: a buffer-backed type may contain only buffer-backed types. A record's fields must all be buffer-backed; a union's candidates must all be buffer-backed; a typed buffer `[N]T` requires `T` to be buffer-backed; a domain's parent must be buffer-backed. Non-buffer types may appear only at top-level positions — slots introduced via `#`, parameters, receivers — or as fields of objects or candidates of variants. Object fields and variant candidates are unrestricted as to category; they may hold buffer-backed or non-buffer types freely.
 
The grammar enforces this distinction directly: record-field and union-candidate type expressions admit only the buffer-backed forms; object-field and variant-candidate type expressions admit any type. No grammar change is needed to enforce the rule — the two-tier type-expression non-terminal has the partition built in.
 
The containment rule's load-bearing consequence is that mutation either succeeds fully or fails fully (principle 4, §1.2). A record's bytes are unambiguously a byte-aggregate, copy-restored to the caller's slot atomically on success and not at all on failure. A record-with-pointers would punch a hole in this — the pointer copies, the pointee doesn't, and partial-failure semantics drift. The containment rule eliminates the concern at the type level rather than relying on convention or analysis.
 
The split also determines whole-slot initialization tracking (§6.14): a buffer-backed slot is one byte-aggregate, not a graph, and tracking it as a unit is sound because no field can have non-byte semantics. The static analyses treat each form per its category, with the guarantees of one category not silently leaking into the other.
 
### 5.2 Buffer Primitives
 
The bracket form `[N]` denotes an `N`-byte buffer; the form `[]` denotes an unbounded byte buffer. The forms `[N]T` and `[]T` denote buffers laid out as a sequence of `T`-values, sized to `N` elements or unbounded respectively. Buffers are the substrate over which all value-like types are interpreted: a `[4]` is four bytes — what those bytes *mean* (a 32-bit integer, an RGBA pixel, a packed pair of 16-bit values, a Unicode code point) is determined by the domain layered on top of it (§5.3).
 
**Indexing into a buffer-shaped value uses the suffix `[index]` syntax.** Indexing is failable: out-of-bounds is a first-class failure, not undefined behavior. Indexing on a domain works because domains are themselves buffer-backed, transitively reducing to a buffer; whether indexing is *meaningful* on a particular domain (versus syntactically permitted but not idiomatic) is a domain-specific concern that the standard library and user code resolve at the domain level.
 
**C-style pointer arithmetic is not supported.** Stepping through buffer contents requires `[i]`. The restriction reflects two design commitments: indexing is a first-class operation that produces a checked failure on out-of-range access, while pointer arithmetic admits no analogous check; and the language reserves implementation latitude on pointer representation (§5.10), including handles that may relocate, which arithmetic against a stable address would alias incorrectly after relocation.
 
There are no privileged primitive types in Basis. The standard library defines `Int32`, `UInt32`, `Float32`, `Int8`, and similar names as domains over buffer-primitives of appropriate size, with associated intrinsics for arithmetic and comparison. User-defined domains use the same mechanism — there is no conceptual distinction between standard-library domains and user domains. The buffer primitives are the unifying substrate; everything else is a refinement.
 
### 5.3 Domains
 
A **domain** is a new type declared in terms of a parent type, where the parent must reduce — transitively, through any aliases — to a buffer-backed type. The declaration form is:
 
    .domain Inches : Int32
    .domain Centimeters : Int32
    .domain RGBA8 : [4]
    .domain Tagged : [12]
 
Pointers, command-typed values, fexpr-typed values, objects, and variants cannot serve as domain parents because the purpose of a domain is to give a refined interpretation to a definite chunk of bytes — a non-buffer parent has no chunk of bytes to interpret. The constraint is structural and enforced at the grammar level.
 
**Domains form a parent–child hierarchy with one-directional implicit upcasting.** A value of a child domain is implicitly accepted wherever the parent domain is expected, but a value of the parent domain is *not* implicitly a value of any specific child. So an `Inches` value is implicitly an `Int32`; an `Int32` value is not implicitly an `Inches`. **Sibling domains do not implicitly convert.** `Inches` and `Centimeters`, both parented at `Int32`, are mutually incomparable for implicit conversion; an explicit constructor invocation is required to move between them.
 
The implicit upcast is a **typing-acceptance rule, not a value-rewriting rule**. The bytes underlying the value are unchanged across the upcast boundary; the static analysis simply accepts the value at the broader type. What changes at the boundary is the type-lens through which the value is interpreted: methods declared on a type interpret that type's bytes per that type's conventions, and methods declared on a parent or ancestor type interpret the bytes per the parent's conventions, which may differ. Witnesses for class dispatch are constructed at the relevant slot boundary based on the slot's declared type at that point — a witness constructed for an upcast value uses the upcast slot's type, with no mechanism (and no reason) to look back to a value's earlier slot history. The full rule for when buffer-backed dispatch identity is captured at a class-typed slot, and the conditions under which a child type's identity carries through to dispatch versus is lost in transit through intermediate non-class-typed buffer-backed parameters, is in §9.18.
 
Domains are first-class types: they may be parameters, fields, receivers of class methods, expression-position results via `-> name`, and so on. The hierarchy is **open for child extension**: a downstream module may declare a child of an imported domain. The implicit-upcast relation is structurally stable across this extension because the upcast is one-directional (child → parent) and child declarations do not widen the upcast set for any existing type. The asymmetry with the failure-message hierarchy (§4.9), which is closed for downstream extension, is intentional: domain-hierarchy openness does not introduce the silent-widening risk that motivates the failure-tag closure.
 
### 5.4 Records
 
A **record** is a contiguous, byte-addressable buffer with named field offsets. Record values are value-like: copyable as bytes, with no identity beyond byte-content, and laid out at deterministic offsets within their containing storage. The declaration form is:
 
    .record Point : Int32 x, Int32 y
    .record RGBA   : UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha
    .record Header : UInt16 version, UInt32 length, [4] reserved
 
Record fields are constrained to buffer-backed types (the containment rule of §5.1), reflecting the requirement that every field have a definite byte width and offset. A record's total byte size is the sum of its fields' sizes plus any padding the implementation introduces for alignment; the surface specification does not currently pin down padding rules. Whether the language commits to deterministic record layout, admits implementation-determined padding subject to a stability convention, or admits per-record `.packed` annotations is registered as a minor open question; the catalog entry is in Appendix I.
 
Records are **nominally typed** (§5.1's containment rule makes the structural matching well-defined; nominality is the choice on top): two `.record` declarations with identical field structure produce two distinct types. Values of one are not interchangeable with values of the other on the basis of structural similarity alone. The discipline is uniform across the type system; it is what gives module-exposed types their abstraction story.
 
Records compose: a record may have a field whose type is another record, and the inner record's bytes lay out within the outer record's bytes contiguously at the inner record's offset. This is the natural way to express compound buffer-backed structures.
 
The record/object split (§5.11) is the surface manifestation of the buffer-backed/non-buffer division: records are byte-aggregates with no identity beyond their contents; objects are identity-bearing aggregates whose fields may include non-buffer types. The choice between them is the choice between byte-aggregate semantics and identity-bearing-aggregate semantics — not a graded distinction, but a categorical one.
 
Records also admit **inline forms** — anonymous declarations within a field or candidate position. An inline record nested inside an outer record's field declaration produces a nominally distinct type per declaration site; the same field structure appearing in two separate outer records yields two distinct inline types. Named records are typically preferred for re-use; inline forms are a convenience for one-off compound structure inside another declaration.
 
### 5.5 Buffer-Backed Subsumption
 
Buffer-backed types compose under a **uniform parent-chain subsumption** relation that operates across domains, records, unions, and the primitive buffer forms together. A value of any buffer-backed type subsumes upward through its parent chain — every ancestor along the chain accepts it implicitly — terminating at the relevant `[N]` or `[]` primitive. **An `Inches` is implicitly an `Int32`; an `Int32` is implicitly a `[4]`; a `[4]` is implicitly a `[]`.** A `Point` (record over an `[8]`-byte representation) is implicitly an `[8]`, and an `[8]` is implicitly a `[]`. The chain extends through every named refinement on the way; nothing is special about the named-domain step versus the record-to-primitive step.
 
**Sibling buffer-backed types subsume to their common ancestor without peer-conversion.** Two domains parented at `Int32` — `Inches` and `Centimeters` — are mutually incomparable as types; they do not implicitly convert to each other. Each individually subsumes upward through the shared `Int32` ancestor: a context expecting `Int32` accepts either; a context expecting `[4]` accepts either; a context expecting `[]` accepts either. A context expecting `Inches` rejects a `Centimeters` value, and a context expecting `Centimeters` rejects an `Inches` value. The sibling-to-common-ancestor pattern is what makes domain hierarchies usable for unit-style refinements without admitting silent unit conversions.
 
The subsumption is **type-acceptance, not value-rewriting**. The bytes are unchanged at the upcast boundary; the static analysis accepts the value at the broader type. Methods and class witnesses are selected at each slot boundary based on the slot's declared type at that point — different types may interpret the same bytes differently, and a witness constructed at any class-typed slot boundary reflects that boundary's slot type rather than any earlier slot type the value passed through. The rule for when buffer-backed dispatch identity is captured at a class-typed slot, and the conditions under which a child type's identity carries through to dispatch versus is lost in transit through intermediate non-class-typed buffer-backed parameters, is in §9.18.
 
The subsumption rule is one-directional: child to parent, never the reverse. A value of a parent type does not implicitly become a child value. Constructing a child value from a parent value requires an explicit constructor invocation (§3.9), which the typechecker recognizes as a deliberate cross-type movement.
 
A separate buffer-backed subsumption rule applies to unions: the **union → candidate-or-parent byte-reinterpretation** rule (§5.7) admits a union value into any buffer-backed slot whose type is on the parent chain of *at least one* of the union's declared candidates. This rule is given its own subsection because it differs from the parent-chain subsumption rule in two important respects: it is existential across the candidate set, not universal; and it is **not Liskov-preserving**, since the bytes' meaning depends on which candidate is currently active in the union — a property the language does not track.
 
The narrowness of the implicit-conversion story across buffer-backed types is a deliberate design commitment. Implicit conversions are a routine source of reasoning errors in languages that admit them. The parent-chain rule is the minimum that makes refinement-style domain hierarchies usable; the union byte-reinterpretation rule is the buffer-backed-side answer to the discriminated-overlay question that variants answer differently (§5.12). Every other type-crossing — record to record, sibling domain to sibling domain, anything to or from a non-buffer type — is explicit, requiring a constructor invocation, an interpretive cast against a union, or the dynamic-narrowing operator `-<` (§7.14).
 
### 5.6 Unions
 
A **union** is a byte-level overlay of declared candidate types. The union's storage is `max(candidate-sizes)` bytes; assigning a candidate value writes that candidate's bytes into the overlay. The declaration form is:
 
    .union Number : Int32, Float32
    .union AnyFour : Int32, [4], RGBA8
 
Union candidates must be buffer-backed (the containment rule of §5.1). This is what makes the byte-overlay coherent: every candidate has a definite byte-width, and the overlay is the maximum.
 
**The union carries no language-level discriminator.** A union slot is *just bytes*; the language tracks neither which candidate is currently active nor any tag identifying it. Discrimination is the user's responsibility — typically by storing an enumeration value alongside the union in a containing record, or by deriving the active candidate from contextual information already implicit in the program.
 
The choice not to attach a language-level discriminator to unions is structural. A discriminator on every union would force a discriminator-byte allocation cost on every union and would commit the language to a witness mechanism inside the buffer-backed side of the type system. Both costs are inappropriate for the lower-level coding role unions occupy: programs that want safe tagged sums use **variants** (§5.12), which carry both a tag and a witness; programs that want byte-overlay efficiency at the cost of programmer-managed discrimination use unions. The two surfaces serve different needs, and the distinction is preserved by giving each its own discrimination model. The union/variant pair is the buffer-backed/non-buffer surface manifestation of the byte-overlay-versus-tagged-sum design choice.
 
Unions are nominally typed (§5.4): two `.union` declarations with identical candidate sets produce two distinct types. Inline forms are admitted, with the per-declaration-site nominal-distinctness rule applying as for records.
 
Reading a candidate value out of a union is **interpretive casting** — a reinterpretation of the union's bytes at a candidate-typed slot view. The typechecker enforces only that the cast's target type is one of the union's declared candidates; it does not verify which candidate is actually active. The construction surface for interpretive casting is part of the broader byte-reinterpretation story (§5.7) and is treated in §7 alongside the rest of the construction surface.
 
### 5.7 Union → Candidate-or-Parent Byte-Reinterpretation
 
A union value implicitly subsumes — by zero-cost byte reinterpretation — to any buffer-backed type `T` such that `T` appears on the standard buffer-backed subsumption chain (§5.5) of *at least one* declared candidate of the union. The relation is the union's reading rule: a union value flows into any context expecting a candidate's type or any of that candidate's ancestors.
 
The relation is **existential** across the candidate set, not universal. A union with candidates `{A, B, C}` where `A`'s parent chain reaches `T`, regardless of whether `B`'s or `C`'s chain reaches `T`, admits the subsumption to `T`. The user's reading: if some candidate's bytes could plausibly be interpreted as `T`, the language admits the operation; the responsibility for ensuring the union's bytes *are* a valid `T`-value at the moment of reading rests with the user's discrimination machinery.
 
The relation is **one-way**: a union value flows into a `T`-typed slot. The reverse direction — a `T`-typed value into a union slot — is a construction operation handled by the standard surface (constructor invocation or aggregate literal targeting the union type); the implicit subsumption does not run backward.
 
The relation is **not Liskov-preserving**. The bytes are the same across the reinterpretation, but their *meaning* depends on which candidate is currently active in the union, which the language does not track. The user-side reading: the language admits the byte-reinterpretation, but the semantic validity — that the bytes the union currently holds *are*, in fact, a valid `T`-value — is the user's responsibility. The discipline is the C-style discipline: a union is a tool for byte-aliasing across known structurally-compatible representations; the user manages the discriminator and the validity. Under the standing lens of §1.5, the reinterpretation is *byte-reinterpretation subsumption*, distinct from the Liskov-preserving parent-chain subsumption of §5.5; the two are kept terminologically separate to avoid silently conflating them.
 
The dynamic-narrowing operator `-<` (§7.14) **does** apply to union slots. The admissibility test is the same existential parent-chain rule as the implicit byte-reinterpretation subsumption above: `-<` from a union to a target type `T` succeeds when `T` appears on the parent chain of *at least one* declared candidate, and fails when `T` lies on no candidate's chain. A union with candidates `{Int32, Int64}` admits `-<` to `[4]` (on `Int32`'s parent chain) and to `[8]` (on `Int64`'s parent chain), but fails `-<` to `[3]` or `[5]` — neither candidate reduces to those types. Unlike `-<` on variants and object hierarchies — which carry language-tracked tags or runtime type information for the operator to test — `-<` on a union has no runtime discriminator to consult; the admissibility check is made entirely at typecheck against the union's declared candidate set. The user-asserted-byte-validity discipline above still applies: an admissible `-<` is a byte-reinterpretation at runtime, and the user's discrimination machinery remains responsible for ensuring the union's bytes in fact represent a valid target-typed value.
 
The same `-<` operator also extends to class-typed targets on unions, under a stricter admissibility rule. **`-<` from a union to a class `C` succeeds when *exactly one* of the union's declared candidates is an instance of `C`, and fails otherwise.** When admissible, the constructed class-typed value carries the witness for the pair (qualifying-candidate, `C`); selection is fully static. A union with candidates `{Int32, Float32}` where only `Int32` has a `Showable` instance admits `-<` to `Showable`, producing a `Showable`-typed value with the (`Int32`, `Showable`) witness. The same union where *both* candidates implement `Showable` rejects `-<` to `Showable` as ambiguous: the language has no runtime discriminator to pick between the candidate witnesses, and the user must instead narrow to a specific candidate type first (`-<` to `Int32`, for instance) and let the resulting candidate-typed value flow into the class-typed slot through ordinary subsumption. A union where no candidate is an instance of `C` also rejects.
 
The exactly-one rule differs from the existential rule for buffer-backed targets because the underlying mechanisms differ. A buffer-backed target is a byte-view; the result type determines byte interpretation directly, so existential admissibility suffices. A class target requires constructing a witness specific to a particular candidate, so the rule must determine that candidate uniquely. Existential admissibility cannot pick between competing candidate witnesses; universal admissibility cannot either, since unions carry no runtime tag for per-candidate selection at use time. Singularity is the unique rule that produces a determinate static witness selection.
 
The class-target case is **`-<`-only** — explicit, never implicit. A union value does not implicitly subsume to a class-typed slot via the candidate-level instance rule, even when exactly one candidate qualifies. The reason is interaction with a separate, orthogonal possibility: a union type may itself be declared as an instance of a class — `.instance MyUnion: Showable` is a well-formed declaration that gives the union type its own `Showable` methods, with the union's bytes interpreted at that level rather than at any candidate's. When such a declaration is in scope, implicit subsumption from a `MyUnion` value to a `Showable`-typed slot uses the union's own instance, with the witness (`MyUnion`, `Showable`). Were the candidate-level rule also admitted implicitly, the two mechanisms would compete at the same use site and the language would have to choose one over the other silently. Keeping the candidate-level case explicit at `-<` keeps the two paths distinct: a direct pass (implicit subsumption) uses the union's own class instance, when declared; `-<` to a class uses a candidate's class instance, under the exactly-one rule. A programmer who wants a particular candidate's class instance in the multi-match case narrows to that candidate type first and lets the resulting value flow into the class-typed slot through the standard subsumption story.
 
The user-asserted-byte-validity discipline carries over to the class-target case. The witness selected by `-<` is the candidate's witness, and methods dispatched through it read the union's bytes as that candidate's bytes. The user's discrimination machinery remains responsible for ensuring the union's bytes do in fact represent a valid value of that candidate at the moment of the `-<`.
 
### 5.8 Aliases
 
An **alias** is a synonym — purely a human-ergonomics tool. The declaration form is:
 
    .alias UserId : Int64
    .alias Cookie : [16]
 
The alias name and its right-hand side are interchangeable in both directions in all contexts; aliases erase entirely from the type system's perspective. They introduce no new type identity. A `UserId` is the same type as `Int64`; a `Cookie` is the same type as `[16]`.
 
Aliases are useful as **type-level abstraction barriers**: a module exposes `.alias UserId : Int64`, downstream code uses `UserId` everywhere, and a later implementation change to (say) `[16]` (a UUID) requires changing only the alias declaration, assuming the change is otherwise compatible. The downstream code reads the same; the type identity has not changed because the alias erases.
 
Aliases compose with domains: a domain's parent may be an alias, which transparently resolves to the underlying type. The combination is permitted but rarely needed.
 
The line between alias and domain is the line between *no new type identity* and *new type identity with implicit-upcast to the parent*. An alias erases; a domain with the same right-hand side does not. The choice between them is the choice between a renaming-only abstraction and a typing-distinct refinement. A program that wants `UserId` to *not* be substitutable with arbitrary `Int64` values uses a domain (`.domain UserId : Int64`); a program that wants a clearer name for `Int64` without distinguishing it from other `Int64`-typed values uses an alias.
 
### 5.9 Enums
 
An **enumeration** is a compile-time-constant collection of named values of a single type. The declaration admits two surface forms:
 
    .enum Severity : info = 0, warning = 1, error = 2
    .enum HttpCode Status : ok = 200, notFound = 404, serverError = 500
 
The **one-name form** (`.enum Severity : ...`) names the enum's type (here `Severity`); the values' representational type is inferred from the literal values. The **two-name form** (`.enum HttpCode Status : ...`) makes the representational type explicit (here `HttpCode`) and names the enum's type second (here `Status`). The two-name form is the form to use when the literal values must inhabit a non-default representational type — for example, a domain over `Int16` with associated invariants — and a constructor is available that is callable on each literal. The two names are not a "second-level grouping"; they are simply *type constraint* and *enum type*, with no nesting semantics.
 
Enum values are compile-time constants. They are read-only at every use site, with values fixed at module compile time, and they participate in the type system as values of the enum's type — not as a special category. An enum's type is a buffer-backed type (it inherits the buffer-backed-ness of its representational type) and may serve as a record field, union candidate, parameter, receiver, and so on, on the same terms as any other buffer-backed type.
 
Enums are **the language's single principled exception to the no-non-local-state principle.** A reference to an enum value is, structurally, a reference to non-local state — the value lives at module scope, not in any frame. The language admits this exception because enums are *constants*: they cannot be mutated, they have no per-thread or per-frame state, and they exist solely for the convenience of naming a fixed set of distinguished values. Whether enum values are constructed up-front at module load or computed on-demand at first use is implementation-dependent and not visible to the language.
 
The principle behind the exception is precise: it admits *compile-time-constant non-local read*, nothing more. Mutable module-level state, ambient context, thread-local storage, and module-level singletons all remain forbidden. Enums are the carve-out, and the language does not extend the carve-out by analogy to other constructs.
 
### 5.10 Pointers
 
A **pointer type** is written `^T`. The caret is a type-prefix and may stack: `^^T` is a pointer to a pointer to `T`. Pointers are non-buffer types — they reference other storage rather than carrying byte-content directly — and may not appear as record fields, union candidates, or other buffer-backed positions. They appear at top-level slots (introduced via `#`), parameters, receivers, object fields, and variant candidates.
 
The expression-position operators on pointers are the suffix `^` (dereference) and the suffix `&` (address-of). A read through a pointer-typed slot uses `p^`; the address of a slot is `x&`. The full surface for these operators, including their interaction with the construction surface and the access-path discipline, is in §7 and §6.5 respectively.
 
The language commits to **abstracted pointer semantics**. The user-visible meaning of `^T` is "pointer to `T`." Whether the runtime implementation is a thin pointer (carrying only the pointee's address), a fat pointer (carrying the address plus dispatch metadata), or a handle (a pointer to a pointer, supporting relocation by the allocator) is an implementation choice, made per type by the compiler, and not user-visible. The implementation latitude permits the compiler to choose, on a per-`T` basis, the representation that best serves `T`'s actual needs: a thin pointer for a small fixed-size buffer-backed `T`; a fat pointer for an object-typed `T`, supporting class dispatch; a handle for any `T` allocated in a relocation-supporting allocator.
 
User-visible operations on `^T` are uniform across these choices: dereference, address-of, indexing into a buffer-typed pointee via `p[i]`, member access for object pointees, dispatch via `::`. The user reasons about pointers in the abstract; the implementation chooses the concrete shape.
 
**No C-style pointer arithmetic.** Stepping through buffer contents requires `p[i]`. Incrementing a pointer to advance through an array is not supported. Indexing produces a first-class failure on out-of-range access; pointer arithmetic would not. The restriction is also necessary for the handle implementation to be valid — a handle's underlying address may move, and an arithmetic offset against it would alias the wrong target after relocation.
 
A `^Object` parameter is conceptually a double-indirection. Object-typed values are themselves indirections (objects are stack/heap-allocated with potentially non-contiguous fields, accessed via fat pointers); a `^Object` therefore points to a *slot* containing an object reference, and a writeable `^Object` parameter allows the callee to point that slot at a different object on success. The double-indirection structure is what lets writeable-`^Object` parameters serve as object-yielding result slots in the no-return-values world.
 
Reading from `^T` and writing to `^T` do not, in themselves, violate the no-non-local-state principle: the pointer is itself a parameter (or transitively reachable from one through the provision chain), and operations through the pointer touch storage that arrived by explicit provision. The transitive READ contract (§6.5) refines this rule along access paths rooted at READ parameters: writes through a `^T` reached by such a path are forbidden, while writes through a `^T` reached by a REFERENCE-rooted or PRODUCE-rooted path are permitted normally.
 
### 5.11 Objects
 
An **object** is a stack-or-heap-allocated, identity-bearing aggregate. Object fields can be of any type — buffer-backed *or* non-buffer — including pointers, command-typed values, fexpr-typed values (subject to fexpr-specific restrictions; §8), other objects, and variants. The declaration form is:
 
    .object Logger : String name, ^File output, Severity threshold
    .object Cache  : [4096] storage, Int32 used, ^Cache next
 
Object-typed values are reference-semantics in the sense that they are normally manipulated through fat pointers to the object's storage rather than as bytes. The object's fields may be discontiguously laid out — the language does not commit to a particular field-layout strategy for objects, since the absence of a byte-aggregate constraint frees the implementation from offset-stability requirements that records carry.
 
The record/object split (§5.4) is the core split of the buffer-backed/non-buffer division. Records *cannot* contain pointers, command-typed values, fexpr-typed values, objects, or variants; objects *can* contain anything. Records have no identity beyond byte-content; objects have identity that survives byte-content equivalence. The two are not a graded distinction — they are categorically separated, and the choice between them is the choice between byte-aggregate semantics and identity-bearing-aggregate semantics.
 
Objects are nominally typed: two `.object` declarations with identical field structure produce two distinct types.
 
**Object lifetime ceiling.** The frame in which an object's storage is introduced is the object's lifetime ceiling: no mechanism in the language allows an object to outlive the frame that introduced it, except via transitive containment in another object whose ceiling is already higher. The type-side rule recorded here is that an object's *type* does not entail its lifetime — object types are first-class types — but every object *value* is bound to an introducing frame. The frame-ownership lens (§1.5) makes the rule concrete: an object lives in a frame's storage; a `^Object` parameter passed downward gives a callee access to it, but the callee does not become the owner. A writeable `^Object` parameter lets the callee swap which object the caller-owned slot points at, but the new object is allocated into the *caller's* frame on successful copy-restore. The lifetime ceiling is whichever frame ends up holding the slot at the binding moment.
 
**Frame-exit hooks via `@` and `@!` (§3.12) are tied to the frame slot holding the object, not to the object's identity transitively.** A class declaring `@` for an object type fires the handler when the *frame slot* holding the object retires. An object embedded as a field of another aggregate — a field of another object, or transported via an unusual containment — does not fire its `@` handler when that container retires; only the object directly held in a frame slot does. The discipline is **frame-lifetime-tied, not object-lifetime-tied**, which is more restrictive than C++'s destructor mechanism and is intentionally so: the language's at-stack mechanism is the single carve-out from the no-hidden-control-flow principle, and the more restrictive rule keeps the carve-out narrow.
 
Object types may have class instances declared for them (§9). The scope operator `::` works on object-typed values and on `^Object` values via the object's fat-pointer dispatch metadata. An object's class membership is a property of the object's type, not of any particular object value, and is determined at the type-declaration site or at instance-declaration sites.
 
### 5.12 Variants
 
A **variant** is a non-buffer tagged sum. Variant candidates may be of any type — buffer-backed or non-buffer — including pointers, objects, command-typed values, other variants, records, and domains. The declaration form is:
 
    .variant Shape : Circle, Rectangle, Polygon
 
The Record–Object–Union–Variant parallel is exact at the buffer-backed/non-buffer axis (§5.4 / §5.6 / §5.11): records and unions are buffer-backed; objects and variants are non-buffer. The variant-side surface answers the discriminated-overlay question that the union-side (§5.6) answers with a user-tracked discriminator: a variant carries both a tag identifying the active candidate and a class-witness component participating in class-system dispatch — the witness's specific role and population are class-system territory and are detailed in §9.
 
A variant value's runtime representation is a **3-word slot** — a triple of:
 
- **Tag identifier.** A small-integer identifier for the active candidate, slot-sized (32 bits comfortable, with spare bits available for occupancy or other small flags). The tag identifies which candidate the variant is currently storing or, distinctly, the absent state (§5.13).
- **Candidate pointer.** Pointer-sized; references the candidate value's storage. The storage is laid out per the candidate's type — a record candidate carries that record's bytes; an object candidate carries that object's identity-bearing storage; a pointer candidate carries a pointer-shaped value at the indirection's other end.
- **Class witness.** Pointer-sized; carries class-dispatch information for the variant value. The witness's role and population conditions are class-system territory; see §9 for the class-system mechanics that interact with the variant slot's witness component.
The same 3-word slot pattern appears in three places in the language: failure values (§4.1), variant slots (here), and Case-B class-typed parameter slots (§9.7–§9.9). The pattern is uniform across these uses; the witness component is what makes class dispatch on a slot's currently-active value coherent without the consumer knowing the concrete candidate type.
 
Variants are reference-semantics: variant values are normally manipulated through pointers to the variant's storage, parallel to objects. A variant value is non-buffer and may not appear in a buffer-backed position; it may appear in a top-level slot, in an object field, in a variant candidate (forming nested variants), or in any position the containment rule of §5.1 admits non-buffer types.
 
Variants are nominally typed: two `.variant` declarations with identical candidate sets produce two distinct types. The variant declaration's full surface — including any candidate parameterization and any hierarchy structure — is given in Appendix B; the variant construction surface is in §7.12.
 
### 5.13 The Absent State
 
Every variant slot inherently admits an **absent state** in addition to its declared candidate states. A variant in the absent state has no active candidate: its tag identifies "no candidate here," its candidate pointer is null, and its witness is correspondingly null.
 
The absent state is **intrinsic to variants alone**. No other type in the language has a "may-be-absent" form. Pointers, objects, command-typed values, fexpr-typed values, records, unions, aliases, enums, and named domains all contain what their type declaration says they contain — there is no slot for the absence of a value. Variants are the language's null-pointer-inclusive data structures *without* admitting NULL into the type system: the absent state is a structural property of variants, surfaced through the normal type-system mechanisms, not a special case grafted onto every reference type.
 
A bare introduction of a variant slot — `# SomeVariant x` with no initializer — produces a variant slot in the absent state. The 3-word slot's all-zero pattern (zero tag, null candidate pointer, null witness) is the absent state, by construction. The bare-introduction form is admitted for variants and for no other non-buffer type: pointers, command-typed values, fexpr-typed values, and objects all reject bare introduction because they have no zero-default representation that the language admits. Variants alone admit a zero-default, and that zero-default *is* the absent state.
 
A variant declared with exactly one candidate is the language's idiomatic optional: a slot of that variant type is either absent or holds the single candidate's value. The absent state stands for "no value present"; the candidate state stands for "value present." The pattern composes with the rest of the variant machinery — testing for absent uses the `?- _ -< v` form, and engaging on the candidate uses a `?: 'narrow -< v` form (§7.13) — without any dedicated optional-type machinery beyond the variant declaration itself.
 
The absent state is *always* a path through any variant. Code that operates on a variant's candidate must either explicitly handle the absent case or rely on a default arm in a `?:` chain catching it. The structural visibility of the absent path is what differentiates Basis variants from null-bearing reference types in languages where NULL is admitted everywhere a pointer is admitted: the absent state is acknowledged at the type level, surfaced syntactically through `_`, and addressed through standard composition rather than ignored until runtime.
 
The construction surface for variants — including the `${Candidate <- value}` form for non-absent introduction and the `_` markers in `-<` operations and aggregate literals — is treated in §7.12 alongside the rest of the variant construction story. The dynamic-narrowing operator `-<` and its absent-state forms (`_ -< v` to test non-absence, `v -< _` to clear to absent) are in §7.14.
 
### 5.14 Command-Typed Values
 
A **command-typed value** is a first-class value that wraps a command-shaped operation. The language admits four constructional forms that produce command-typed values — command reference, command literal, lambda, and fexpr — enumerated in §3.15 and detailed in §8. The type system describes command-typed values uniformly through the same family of type expressions.
 
The type-expression forms for ordinary command-typed values are:
 
    :<paramTypes>          ; never-fails command-typed value
    ?<paramTypes>          ; may-fail command-typed value
    !<paramTypes>          ; must-fail command-typed value
 
The angle-bracket list is a sequence of **parameter types only**, with no parameter names — at the type level there is nothing to refer to a name as. Mode markers in the list use the **suffix-on-type** placement of the nameless-context rule (§3.3): `Type` (no marker, READ), `Type'` (PRODUCE), `Type&` (REFERENCE). Pointer parameters carry the pointer-marker as prefix and the mode-marker as suffix on opposite sides — a pointer-to-`Int32` as a reference parameter is `^Int32&`, with `^` and `&` visually distinct on opposite ends of the type.
 
Examples:
 
    :<Int32, Int32>        ; never-fails command-typed value taking two Int32 (READ) parameters
    ?<Int32', String>      ; may-fail command-typed value with a productive Int32 and a READ String
    !<^Int32&>             ; must-fail command-typed value with a reference parameter of type ^Int32
 
Command-typed values are non-buffer types — they carry dispatch metadata and (for capture-bearing forms) capture information that does not reduce to bytes. They may not appear as record fields, union candidates, or other buffer-backed positions. They appear at top-level slots, parameters, receivers, object fields, and variant candidates (subject to the fexpr-specific restrictions of §5.15 for fexpr-typed values).
 
Command-typed values support every operation a value supports: binding to slots, passing as parameters, storing in object fields and variant candidates, capture by lambdas and fexprs (within the rules of §8), partial application (§9.14), and direct invocation. The scope operator `::` produces a command-typed value with the receiver(s) baked in: `(receiver :: name)` has the type the class declared for `name` minus the receiver position. The full operational mechanics of dispatch are in §9.
 
**The failure-mode marks on command-typed values follow the subsumption rule of §4.2.** A `:`-marked value is acceptable wherever a `?`-marked value is expected; a `!`-marked value is similarly acceptable; a `?`-marked value is not interchangeable with `:` or `!`. Subsumption is on the failure mark axis only. **Parameter modes and parameter types are invariant** under mark subsumption: a `:<Int32'>` is not interchangeable with `:<Int32>` or `:<Int32&>`. Invariance is essential for soundness — the per-mode discipline at the call site (productive obligations, reference-initialization preconditions, READ-taint contracts) breaks if the mode is permitted to vary.
 
The interaction with class dispatch — when a class method's signature is a command-typed value with given marks and modes, and how instances supply their per-receiver implementations — is in §9. The interaction with overloading (multiple commands sharing a name) is in §9.16.
 
### 5.15 Fexpr-Typed Values
 
**Fexpr-typed values** are nominally distinct from ordinary command-typed values at the type level. A fexpr is the user-defined-control-flow-combinator form: a body written with implicit access to the surrounding command's locals, bounded by its defining frame, and operating on the defining frame's state through implicit captures. The full fexpr mechanism is in §8.5; this section gives the type-side surface only.
 
The type-expression forms for fexpr-typed values are:
 
    :<*>                   ; never-fails fexpr-typed value
    ?<*>                   ; may-fail fexpr-typed value
    !<*>                   ; must-fail fexpr-typed value
 
The `*` inside the angle brackets denotes "this is a fexpr — opaque-to-invoker by design, ceiling-fixed to the defining frame." The `*` displaces the parameter-type list because fexprs have no invoker-side parameter surface beyond the failure mark: captures are lexical, drawn implicitly from the defining frame by free-name resolution, and the invoker's only commitment is to respect the failure-mark.
 
The typing rules for the fexpr family:
 
- **No subsumption between fexpr-typed and ordinary command-typed values.** A `:<*>` is not a `:<>`; a `?<*>` is not a `?<>`; a `!<*>` is not a `!<>`. The two families are nominally distinct, with no implicit conversion in either direction. The buffer-backed-hierarchy subsumption rules of §5.5 do not apply across the boundary; the failure-mark subsumption of §4.2 does not cross the boundary either. The two families have a **family boundary** that subsumption does not cross.
- **The standard mark-subsumption rule applies symmetrically within the fexpr family.** A `:<*>` is acceptable in a `?<*>`-typed slot (per `:` ⊑ `?`); a `!<*>` is acceptable in a `?<*>`-typed slot (per `!` ⊑ `?`). Mark subsumption operates within each family identically; what does not cross is the family-distinguishing `*` marker.
- **The typechecker enforces the defining-frame ceiling structurally.** Fexpr-typed values are recognized syntactically (by the `<*>` marker) and forbidden from assignment to anything that could outlive their defining frame: object fields holding fexpr-typed values are restricted; productive parameters of fexpr type are forbidden; pointers to fexpr-typed slots are forbidden; bare-identifier copy of fexpr values is forbidden; capture of fexpr values by lambdas is forbidden. The full enumeration of fexpr restrictions A–G is in §8.13; the type-side commitment recorded here is that the family-distinguishing type marker `<*>` is what makes the structural enforcement possible.
The motivating concern for the nominal distinction is ceiling-tracking. Under the defining-frame ceiling rule, the typechecker must distinguish fexpr-typed values from ordinary command-typed values syntactically — otherwise a fexpr would be assignable to any `:<>` slot and would escape its defining-frame ceiling. The `<*>` marker provides the required syntactic distinction at parameter declarations, in any other type-position where a fexpr-typed slot must be specifically declared, and at the assignment positions that the structural restrictions test.
 
The interaction of fexpr-typed parameters with class-method dispatch — including the `FexprFailure` standard tag, the fexpr-relevance taint axis parallel to the READ contract, and the per-instance defaults-incompatibility — is in §9.20. Variants with fexpr candidates are admissible only under specific containment conditions; the rule is in §8.5's Restriction C and §8 generally.
 
The fexpr-typed family rounds out the type forms of Basis. The type forms admitted by the language — buffer primitives, domains, records, unions, aliases, enums, pointers, objects, variants, command-typed values, and fexpr-typed values — together cover every value the language recognizes. Each form has a clear position in the buffer-backed/non-buffer split (§5.1), a clear construction surface (§7), and a clear interaction with parameter modes (§6) and class dispatch (§9). The compositions among them are governed by the containment rule (§5.1) and the subsumption rules (§5.5, §5.7, §4.2, §5.15); every other type-crossing is explicit.
