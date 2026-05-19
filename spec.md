# Basis Language Specification

**DRAFT**

## 1. Introduction

### 1.1 Purpose

Basis is a programming language designed for code that benefits from a direct semantic match to Hexagonal Architecture, that maintains tight bounded scope on side effects, and that supports both high-level polymorphic styles and low-level byte-faithful work in a single coherent surface. The language's specific design commitments are oriented toward making AI-generated and AI-reviewed code tractable: side-effect bounds on commands flow through their signatures, recovery contexts are structurally visible in the source, and the static analyses guarantee the runtime properties the signatures describe.

This specification describes the language at the level needed to implement a parser, a typechecker, the core static analyses, and an interpreter or naive AST-walking executor. It does not describe an intermediate representation, a code generator, a runtime support library, or the standard library. Where the language depends on standard-library or runtime presence — as for the `Int64`-arithmetic intrinsics, the `FexprFailure` and `CoercionFailure` standard messages, or the platform-specific allocator — the specification names the dependency without specifying its provider.

### 1.2 Guiding Principles

The language realizes a small set of guiding principles. Each principle has consequences that propagate through multiple sections of this specification; the principles themselves are stated here as the framing under which the rest of the language coheres.

1. **Strong typing saves lives.** Every value has a static type; every parameter, local, and field is typed at declaration; every cross-type movement is either explicitly authorized (constructor invocation, `.implicit` declaration, `-<` dynamic narrowing) or rejected at typechecking. There are no runtime-typed values that escape the static layer.

2. **No non-local state access.** A command body's reached state arrives by an explicit chain of provision: parameters and receivers, implicit context parameters resolved from the caller's lexical scope, or captured-and-traveling state borne by a value the body operates on. No global mutable state, no ambient context, no thread-local storage, no module-level mutable singletons. The single carve-out is enumerations (§5.9), which are compile-time constants.

3. **The fundamental datatype is a buffer.** Buffer-backed types — buffers, ranges, domains, records, unions — reduce transitively to bytes. They copy as bytes, fit inside other byte-aggregates, and are reclaimable at frame retirement without traversal. Non-buffer types (pointers, command-typed values, objects, variants) are confined to top-level slots and object fields.

4. **Mutation either succeeds fully or fails fully.** Every writeable parameter passing uses copy-by-value or copy-restore semantics: on success the callee's value is committed to the caller's slot atomically; on failure no write occurs and the caller's slot is bit-identical to its pre-call state. Failure-atomicity falls out of the calling convention with no transactional machinery and no rollback code in user programs.

5. **No hidden control flow.** Failures propagate, but only along structurally visible paths. There is no exception-with-stack-unwinding mechanism, no implicit dispatch through hidden vtables, no implicit destructor firing on member-field cleanup. Every control-flow effect is either explicit at the call site or marked by a block-construct in the source. The single exception, intentionally narrow, is the at-stack mechanism for frame-exit hooks (§3.13).

6. **Polymorphism and statecharts aren't just for object types.** The class system (Haskell-style typeclass dictionaries) operates uniformly over buffer-backed types, non-buffer types, and command-typed values. Variants admit class-witness slots, supporting dispatch through the active candidate. The same dispatch and propagation patterns appear across variants, failure messages, and class-typed parameters.

7. **Computational status is orthogonal to result state.** A command's success/failure axis (the `:` / `?` / `!` failure marks) is independent of any value the command produces. Failure carries a message; success carries the writes-to-writeable-parameters; neither subsumes the other. The two axes compose explicitly, never by fusion.

8. **Prefer small orthogonal concepts to rich overlapping ones.** Where a new mechanism's work can be done by composing existing language facilities, the composition is preferred. The `-<` operator handles dynamic type coercion across all type pairs (variants, class hierarchies, unions, object pointers) rather than introducing per-case operators. Variant pattern matching uses `?:` chains plus `-<` rather than a dedicated `match` keyword. Receiver-baked partial application reuses the four-form taxonomy rather than a separate "method-pointer" mechanism.

9. **Special forms should be visually distinct from user-defined forms.** Top-level definition keywords begin with `.` (dot-prefix), distinguishing them from user-defined identifiers. Block markers (`?`, `?-`, `?:`, `??`, `-`, `%`, `^`, `|`, `@`, `@!`) are punctuation, not keywords, ensuring they cannot be shadowed. The `${…}` and `$[…]` literal fences carry the `$` sigil that distinguishes them from any user-defined identifier or aggregate.

10. **Syntactic sugar is superior to semantic sugar.** Surface forms that desugar to other surface forms are admitted; surface forms that introduce new semantic primitives are admitted only when they cannot be expressed by composition. Expression-position calling conventions desugar into command calls with `#`-introduced argument-position locals; the `<-` choice form desugars into a chain of try-or-fall-through; the implicit `-> name` rule for single-writeable-parameter commands desugars into the explicit form. Each desugaring is one substitution; the underlying semantics has no hidden complexity.

11. **Fexprs for user-defined control flow.** The language admits user-defined control-flow primitives via the fexpr mechanism (§8.5), which captures defining-frame state implicitly and operates on it at invocation time.

12. **Syntactic whitespace improves legibility.** Indentation determines block structure. The visual layout of a Basis source file reflects its operational structure; constructs at a single indentation level are siblings; constructs at deeper indentation are children. The composition rules for block markers (§4.5) read off the indentation structure directly.

These principles are not separate features that the language assembles; they are the framing under which the rest of the language is written. The sections of this specification that realize each principle are noted in the relevant places.

### 1.3 Core Semantic Skeleton

Basis programs reduce under a small-step operational semantics over a state tuple $\langle V, \Phi, \Sigma \rangle$, where:

- **$V$** is the current verb to be executed. Verbs include user commands $\mathit{exec}(c)$, the failure-firing $\mathit{fail}(\phi)$, the recovery markers $\mathit{recover}$ and $\mathit{recover}(\phi, \sigma, c)$, the scope boundary markers $\mathit{scope}(c)$ and $\mathit{scopefail}(c)$, and the rewind verb $\mathit{rewind}(v)$ for loop continuation. The notation $\overrightarrow{v}$ denotes the continuation from $v$ — what runs next once $v$ has completed.

- **$\Phi$** is the failure register. When no failure is in flight, $\Phi$ holds the empty value $\epsilon$. When a failure is propagating, $\Phi$ holds a failure value $\phi$ — a message identity, an optional payload pointer, and an optional class witness (the runtime details are specified in §4 and Appendix F).

- **$\Sigma$** is the variable state — a mapping from in-scope names to slot identities and contents, partitioned by frame. The notation $\sigma/c$ denotes $\sigma$ bound within the lexical scope of the verb $c$.

The reduction rules below describe how the state tuple evolves. Each rule is read as: *given the antecedent state shape, the system reduces to the consequent state shape.* The notation $\langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi', \Sigma' \rangle$ denotes a complete sub-evaluation of $v$ that itself produced post-state $\langle v', \phi', \Sigma' \rangle$.

$$
\begin{aligned}
\text{(normal execution)} \quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma' \rangle \\
\text{(generating failure)} \quad & v = \mathit{fail}(\phi) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\
\text{(command failure)} \quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\
\text{(skips from failure)} \quad & v \in \{\mathit{exec}(c), \mathit{rewind}(w), \mathit{fail}(\gamma)\} \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\
\text{(generic recovery)} \quad & v = \mathit{recover} \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma \rangle \\
\text{(specific recovery)} \quad & v = \mathit{recover}(\phi, \sigma, c) \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma + \sigma/c \rangle, \quad \vec{c} \leftarrow \vec{v} \\
\text{(recovery failure)} \quad & v = \mathit{recover}(\alpha, \sigma, c) \quad \langle v, \phi, \Sigma \rangle \quad \phi \neq \alpha & \implies & \quad \langle \vec{v}, \phi, \Sigma \rangle \\
\text{(scope boundary)} \quad & v = \mathit{scope}(c) \quad \kappa \in \Phi \quad \langle v, \kappa, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma \rangle, \quad \vec{c} \leftarrow \langle \vec{v}, \kappa, \Sigma' \rangle \\
\text{(scope under success)} \quad & v = \mathit{scopefail}(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \quad \langle \vec{v}, \epsilon, \Sigma \rangle \\
\text{(scope under failure)} \quad & v = \mathit{scopefail}(c) \quad \langle v, \phi, \Sigma \rangle & \implies & \quad \langle c, \epsilon, \Sigma \rangle, \quad \vec{c} \leftarrow \langle \vec{v}, \phi, \Sigma' \rangle \\
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

The single intentional exception is the at-stack mechanism for frame-exit hooks (§3.13). When a frame retires, `@`-blocks and `@!`-blocks registered within that frame's body fire alongside any explicit cleanup code. This is implicit in the sense that no source-line at the registration site invokes the handler; the handler runs as a consequence of the frame retiring. The mechanism is the language's RAII-equivalent — necessary for ergonomic resource management, and confined to a single, syntactically-marked category (the `@` and `@!` block markers within command bodies).

**No non-local state.** A command body's reached state arrives by an explicit chain of provision. There are three kinds of provision:

1. **Parameters and receivers.** A frame's caller passes state as named arguments at the call site; the state is locally bound to the parameter or receiver names within the body.

2. **Implicit context parameters.** The `/`-list in a command's signature names parameters that are filled automatically at the call site from the caller's lexical scope, by uniqueness-of-type. The provision is implicit at the syntactic level but not at the semantic level — the value must actually exist in the caller's lexical scope to be eligible. This is the language's Scala-implicit-style mechanism (§3.6).

3. **Captured-and-traveling state.** A value (such as a fexpr or a lambda) carries with it references to slots in some frame, established at the moment the value was constructed. When the value is later invoked, those references are part of the value's identity. The provision was made at construction time, not at invocation time. This is the language's closure mechanism (§8).

Absence of any of these means absence of reach. A frame six levels above the current one is on the call stack, but if no explicit provision chain reaches up to it, its slots are not reached by the current command. There is no mechanism by which a command can opt into reaching state that was not provisioned for it.

The two principles compose. The first restricts what a command can do *implicitly*; the second restricts what a command can *reach*. Together they make signature-as-documentation work: a command's signature names everything the command can affect, and the marks plus block structure describe everything that can happen control-flow-wise.

The principles relax at `.program` and `.test` directives, which are the language's entry points and lie outside ordinary command scope. These directives may use language features that ordinary commands cannot — at minimum, they are the only contexts in which top-level failures are terminally consumed. The terminal-failure rules for each directive are given in §2.5 (`.program`) and §2.6 (`.test`).

### 1.5 Standing Lenses

Reasoning about Basis correctly requires a small set of mental lenses that are not always native to the reasoning patterns programmers carry from C++, Java, Rust, Go, ML, or Haskell. These lenses are stated here so that the rest of the specification can rely on them; sections that lean on a particular lens reference it where appropriate.

**Frame-ownership.** Every slot is owned by some frame. "Return values" in the conventional C++/Java/Rust sense do not exist: output flows through writeable parameter slots that the caller owns, written to by copy-restore on success. When reasoning about value semantics, mode contracts, aliasing, or lifetime ceilings, the question is *which frame owns this slot* — not "what does this function return."

**Each frame's static analysis is local.** Initialization tracking, failure-mode tracking, and access-path taint are properties of a single command body's analysis on its own parameters and locals. There is no cross-frame propagation. A PRODUCE or REFERENCE output of a downstream call appears in the caller's frame as a freshly-bound local; the access path is rooted at the caller's local, not at any of the caller's parameters. The discipline migrates to the frame that has the right context; the language does not propagate.

**Access paths, not storage.** The transitive READ contract operates on access paths — the named ways through which a value is reached — not on storage locations. The same storage may be reached at runtime through multiple access paths simultaneously, one of which is READ-rooted and another of which is REFERENCE-rooted; writes through the REFERENCE-rooted path are permitted while writes through the READ-rooted path are forbidden. The language does not detect aliasing; it tracks paths.

**Buffer-backed containment.** Byte-aggregate containers — records, unions, `[N]T`, and domain parents — admit only fixed-size buffer-backed contents. The runtime-length forms `[]` and `[]T` are excluded from these positions, as are all non-buffer types (pointers, command-typed values, objects, variants). Objects and variants, being identity-bearing or tagged-sum constructs rather than byte aggregates, admit any type in their fields or candidates respectively. This rule eliminates whole categories of static-analysis questions: cases that presuppose a byte-aggregate structure containing non-fixed-size components are not Basis cases.

**Buffer-backed subsumption is uniform within fixed-size types.** Every fixed-size buffer-backed type subsumes up its parent chain to its byte-width `[N]`. A `Point` (record over an `[8]`-byte parent) is implicitly acceptable where `[8]` is expected. Sibling types — two distinct records over `[8]`, or `Inches` and `Centimeters` both over `Int32` — subsume separately to their shared parent but do not implicitly convert peer-to-peer. The runtime-length types `[]` and `[]T` are leaves in this lattice: they encode their length in the value rather than in the type, and conversion to or from a fixed-size type is explicit.

**Variants are the only "may-be-absent" type.** Every variant slot inherently admits an absent state in addition to its declared candidate states. A single-candidate variant is an optional. A bare `# x : SomeVariant` introduction produces a variant slot in the absent state. No other type has this property: pointers, objects, command-typed values, records, unions, and named domains all contain what their type declaration says they contain. Variants alone admit an inherent "nothing here" state — the language's null-pointer-inclusive data structures without admitting NULL into the type system.

**`::` is the scope operator, not just class-resolution.** The `::` operator serves multiple roles: class-method resolution on a receiver, field-member access on aggregates, namespace and module resolution, partial-application bake-in. The unifying reading is *scope-into-a-namespace* — the surrounding context determines which role applies. Calling `::` "the class-resolution operator" is too narrow.

**Orthogonality of language and standard library.** Where a new mechanism's work can be done by composing existing language facilities, the composition is preferred over a new intrinsic or new syntactic form. The `-<` operator unifies dynamic type coercion across all type pairs without per-case operators. Variant pattern matching uses `?:` chains plus `-<` rather than a `match` keyword. The standard library is consumer of the language, not co-author of it.

**Liskov substitution as a design tool.** Where the language admits a subtyping relation, the design preserves Liskov substitutability. The buffer-backed parent-chain subsumption preserves Liskov by construction (the upcast is one-directional and carries no value rewriting). The class-instance system preserves Liskov by class-method-contract (instances must satisfy the class's declared shape). Where Liskov substitution fails — as it does for the union → candidate-or-parent byte-reinterpretation, where the bytes' meaning depends on the active candidate — the relation is given a distinct name (byte-reinterpretation subsumption) and not conflated with Liskov-preserving subsumption.

**Frame-bound region reclamation is the default storage discipline.** Value construction implicitly draws any variable-size storage it needs (the data behind runtime-length types `[]T` and `[]`, the storage behind constructed pointers and objects) from a frame-bound region that is freed at frame retirement. Non-default storage — heap, pool, arena, etc. — is opt-in at the construction site via the `\` allocator qualifier (§7.20); bringing an allocator value into scope does nothing on its own, and without an explicit `\` qualifier every construction's allocation is frame-bound. Non-buffer types remain confined to positions where ownership and lifetime are explicit (top-level slots, object fields with object-lifetime ceiling, frame-bound parameters and receivers).

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

- **`.domain` *Name*` : `*ParentType*** — Declares a domain — a fixed-size buffer-backed nominal refinement of *ParentType*. The parent must reduce transitively to a fixed-size buffer-backed type. Pointers, command-typed values, objects, variants, and the runtime-length forms `[]` / `[]T` cannot be domain parents.

- **`.enum`** — Declares an enumeration. Two forms: `.enum` *Name* `:` *literal entries* names *Name* as the enumeration's type; `.enum` *ConstraintType* *Name* `:` *literal entries* gives a type constraint that the literal values must inhabit and *Name* as the enumeration's type. Enumerations are compile-time constants and are the language's single principled exception to the no-non-local-state principle (§5.9).

- **`.implicit` *signature* `=` *body*** — Declares a constructor that the typechecker may insert automatically when a literal of one type appears in a context expecting another. Restricted to literal-typed source parameters (§7.8). Purely additive: a constructor declared `.implicit` is also callable explicitly via the standard parenthesized-call surface.

- **`.instance` *Type*` : `*ClassList*** — Declares that *Type* satisfies each class named in *ClassList*. An optional `(delegate `*fieldName*`)` clause beside any class name designates a field of *Type* whose existing instance for that class supplies the methods (§9.4). Multiple classes on the right side of `:` are independent instances.

- **`.intrinsic` *signature*** — Declares a command whose body is supplied by the compiler or the standard library, rather than by user-level code. Structurally a `.decl` for typechecking purposes; the implementation supplies the body. The standard `Int64`-arithmetic operations, the platform allocator primitives, and the language's primitive byte-reinterpretation operations are typical examples.

- **`.msg` *Name*`[`*PayloadType*`]` `:` *ParentMessageType*** — Declares a failure message type. Both the `[`*PayloadType*`]` brackets and the `:` *ParentMessageType* clause are optional. Messages declared without a payload-type are payload-less; messages declared without a parent are roots of new hierarchies. The full failure-message system is described in §4.

- **`.object` *Name*` : `*field declarations*** — Declares a non-buffer object type — an identity-bearing aggregate whose fields may be of any type (buffer-backed or non-buffer). Objects participate in the class system as receivers and have explicit lifetime ceilings tied to their introducing frames.

- **`.program = ` *expression*** — Declares the program's entry expression. The expression runs at program start. A compilation unit may contain at most one `.program` directive.

- **`.record` *Name*` : `*field declarations*** — Declares a buffer-backed record type — a named, contiguous, byte-addressable buffer with named field offsets. Field types must be fixed-size buffer-backed.

- **`.test "`*name*`" = ` *expression*** — Declares a named test. The expression runs as a test entry under the project's testing framework.

- **`.union` *Name*` : `*candidate declarations*** — Declares a buffer-backed union type — a byte-overlay of declared candidate types. Union candidates must be fixed-size buffer-backed. The union carries no language-level discriminator; discrimination is the user's responsibility (§5.6).

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

2. **Failure terminates the program with failure status.** A failure that escapes the `.program` body has no further frame to propagate to; the program terminates and exits with a **failure status** — the platform's standard mechanism for indicating non-success (a non-zero exit code on platforms with the Unix exit-code convention, or its equivalent elsewhere). The exact failure-message rendering and the platform-side mechanism for surfacing the failure are implementation-defined; the language commits that termination occurs and that the exit status indicates failure. A successful return from the `.program` body produces a success exit status.

The `.program` body may contain any commands, including may-fail commands; recovery contexts within the body work normally (§4). Only failures that escape the entire `.program` body trigger termination.

### 2.6 The `.test` Directive

The `.test` directive declares a named test:

```
.test "round-trip serialization" = roundTripCheck
.test "edge case: empty input" = edgeCaseEmpty: ""
```

The string following `.test` names the test; the right-hand side of `=` is the test's body expression. A compilation unit may contain any number of `.test` directives.

The `.test` body executes in a top-level frame with the same relaxations as `.program`: top-level effects compose freely, and failures escaping the body are observable. The test-framework integration is **injected on top of** the `.test` entries: the framework runs each test independently, capturing whether the body completed successfully or with a failure. A failure escaping a `.test` body marks that test as failed but **does not abort the overall test run**; the framework runs every `.test` entry in the compilation unit before reporting results. If any test fails, the test run as a whole exits with a failure status (parallel to `.program`'s failure-exit semantics); if every test succeeds, the run exits with success. The framework itself — test-execution order, failure-message rendering, and result-reporting form — is part of the standard testing library rather than the language proper.

`.test` directives are not invoked from `.program`; they are entry points that the testing framework discovers and invokes independently. A compilation unit containing only `.test` directives (no `.program`) is a valid test-only library. A compilation unit containing both is valid; the `.test` directives are independent of the `.program`.

### 2.7 Identifier Capitalization

The lexer enforces a capitalization discipline on identifiers:

- **Type names** begin with an uppercase letter. The category includes domains, records, unions, objects, variants, classes, and aliases — every nominal type. Message names (declared with `.msg`) are also type names and begin uppercase. Module-name segments separated by `::` are type names and begin uppercase.

- **Identifier names** begin with a lowercase letter. The category includes parameters, receivers, fields, locals, command names (when used as identifiers, before any leading punctuation), and all other binding identifiers.

The discipline is grammatical, not stylistic: a `.instance widget: Interface` declaration is a syntax error because `widget` is in a type position but is lowercase. A `cmd MyCommand: …` declaration is a syntax error because `MyCommand` is in a command-name position but is uppercase. The lexer recognizes the case at first character and routes the token to the appropriate non-terminal accordingly.

Mode-marker prefixes do not affect the capitalization rule. The identifier `'r` is lowercase (the `'` is the mode marker, not a letter); the identifier `&counter` is lowercase; the identifier `'Type` would be a syntax error (uppercase identifier in a binding-position name).

The discipline is enforced at the source-text level; it propagates through the rest of the language. The typechecker does not need a separate "is this name a type or an identifier" disambiguation rule, because the lexer has already partitioned the name space.

### 2.8 Bracket, Brace, and Indentation Composition

Basis has three independent structural nesting mechanisms: bracket pairs `(` `)` and `[` `]`, brace pairs `{` `}` (including the `${`-`}` and `$[`-`]` literal fences), and indentation-determined nested blocks. Each operates independently; constructs may interleave them naturally. A block at some indentation level may contain a bracket-delimited type expression, which may contain a brace-delimited body, which may contain its own indented content.

Indentation is the language's primary block delimiter. A line indented under another line is part of the construct begun by that line. Block markers (§4.4) carry a body whose extent is determined by indentation: the body comprises every line indented strictly more than the marker's own line, up to the first line not so indented. The block-marker composition rules — sibling adjacency for `?:` chains, `|` recovery cascades, `^` rewinds — operate on lines at the *same* indentation level as the marker. The user reading source can determine block structure by inspection; the language enforces the same structure during parsing.

The `${…}` and `$[…]` literal-fence tokens introduce a single bracketed group each, matched by a single `}` or `]`. The `$` prefix is unambiguous — the `$` character has no other use in Basis source — so `${` and `$[` are recognized as single tokens at lex time.

The full lexer specification — the token classes, the literal-token grammar, the disambiguation rules between `'`, `&`, and adjacent tokens, the placement of the `.inline` modifier, the `<*>` typing surface for fexpr-typed slots, and the `(T:Class)` constraint form at method receiver positions — is given in Appendix A.


---
 
## 3. Commands
 
A *command* is the unit of execution in Basis. Every operation a program performs — every effect, every computation, every dispatch — is structurally a command invocation. Commands take parameters of declared types and modes, may produce values into writeable parameter slots, may fail with a message, and compose hierarchically through indentation, block markers, and recovery contexts. Commands are first-class values: they may be referenced, partially applied, captured in lambdas, stored in fields, and invoked indirectly through dispatch.
 
This section describes the surface of commands — signatures, parameters, constructors, methods, subcommands, frame-exit hooks, and calling conventions. The full parameter-mode discipline (the static analyses, the transitive READ contract, taint propagation) is in §6. The first-class command-typed value forms — command reference, command literal, lambda, fexpr — are in §8. The class-and-instance dispatch system that resolves method calls is in §9.
 
### 3.1 The Unit of Execution
 
A command has exactly two possible outcomes per invocation: it *succeeds*, or it *fails*. Success and failure are orthogonal to any value the command produces — they are not encoded as result tags or sentinel values, and a successful command may produce no value, while a failed command's writeable-parameter slots are guaranteed not to be modified (§4, §6). The success/failure axis composes through the failure-mode marks (`:`, `?`, `!` per §4.2) and through the block-marker constructs (§4.4); the value-production axis composes through the parameter-passing mechanics (§6.4) and the `<-` operator (§7.1).
 
Commands have *single-shot* semantics. Each invocation produces one outcome; there is no backtracking, no resumption, no Icon-style multi-valued evaluation. The block-marker constructs that compose commands — `?`, `?-`, `?:`, `??`, `^`, `|`, `@`, `@!`, `%`, `-` (§4.4) — combine single-shot outcomes; they do not introduce a generator-style multi-valued flow.
 
Surface forms that look like expression evaluation — the right-hand side of `<-`, parenthesized command invocations in argument position, the choice form `lhs <- a | b | c` — desugar into command sequences whose effects produce what the surface form denotes. The operational semantics has commands and failure flow; it has no separate expression-evaluation primitive (Appendix F). The expression-style sugar (§3.7, §3.8, §3.14) is uniformly resolved into command-call form by the parser; the typechecker and the analyses operate on the command-call form.
 
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
 
The signature variations for constructors (§3.9), single-receiver methods (§3.10), and multi-receiver methods (§3.11) modify the prefix portion of the signature shape — the part before the parameter list — but otherwise reuse the parameter, implicit, result-designator, and body grammar of the regular form. Subcommands (§3.12) use the regular-command signature shape (no receiver, no class-bound dispatch) and follow the same body grammar; the `.sub` introducer and the lexical-scope restriction are what distinguish a subcommand from a regular `.cmd`. Frame-exit hooks (§3.13) are block markers within bodies, not signature shapes; they have no signature surface.
 
### 3.3 Parameter Modes
 
Every parameter and every receiver carries one of three *modes*, which together determine the contract between caller and callee at that position:
 
- **READ** (no marker, bare name) — the callee may read through any storage path reached from the parameter; it may not write through any such path. The transitive read-only contract (§6.5) makes this commitment trustworthy: the language does not allow the value to be smuggled into a writeable position downstream. The caller's slot is unchanged after the call regardless of outcome.
- **PRODUCE** (`'` marker on the name, e.g., `'result`) — the callee is statically obligated to write the parameter's slot exactly once on every successful return path. The caller may pass either an initialized or an uninitialized slot; on success the produced value is copy-restored to the caller's slot, and on failure the caller's slot is bit-identical to its pre-call state (§6.4).
- **REFERENCE** (`&` marker on the name, e.g., `&counter`) — the callee may read the parameter, may write the parameter, may do neither — there is no obligation in either direction. The caller must pass an already-initialized slot, since the callee is permitted to read. Copy-restore semantics apply on the writeability axis: on success any written value is committed to the caller's slot; on failure the caller's slot is bit-identical to its pre-call state.
The markers are part of the identifier itself, not separate tokens. The lexer recognizes `'name`, `&name`, and `name` as three identifier shapes of the same name. Every read and every write inside the command body uses the marker that matches the parameter's mode — the body of a command with a productive parameter `'result` writes `'result <- value`, never `result <- value`. The marker is visible at every use site, not only at the declaration. The same-scope rule (§6.3) prevents the three shapes from coexisting in the same scope.
 
The marker placement varies by syntactic context:
 
- In **named contexts** — parameter declarations, receiver declarations, lambda invoke-method parameters, capture-list entries — the marker travels with the name in identifier-shape: `Int 'result`, `Logger &counter`, `Showable s`.
- In **nameless contexts** — command-type expressions `:<...>`, `?<...>`, `!<...>`, where parameter types are listed without names — the marker attaches to the type as a suffix, leaving the type-prefix position free for the pointer marker `^`: `Int'`, `^String&`, `Logger`.
The two placements agree on what the markers mean; they differ only on placement to suit the surrounding syntax. The full mechanics — copy-restore at the call boundary, the transitive READ contract's access-path taint algorithm, the parameter-mode invariance under failure-mark subsumption, and the receiver-mode tables by signature shape — are in §6.
 
### 3.4 Productive Parameters
 
A productive parameter (`'name`) discharges the *write-once-on-success* contract: the callee's body must write the slot exactly once on every path that reaches a successful exit. Paths that reach a failure exit are exempt from the obligation — the failure-atomicity principle commits that productive slots are never partially written when a command fails.
 
Failure to write a productive parameter on some successful path is a static error; writing it more than once on the same path is also a static error. The discipline composes with the failure-state-lattice analysis (§4.13, Appendix E.3): the typechecker walks the body's CFG with both the failure-state lattice and the initialization lattice, confirming that every path to a `clear`-state exit has performed exactly one write to each productive parameter.
 
The write-once rule is structural, not stylistic — there is no "you may write twice if you also clear in between" alternative, and there is no in-place-update form for productive parameters. The full rule is in §6.13. The pattern most user code follows is *compute-then-commit*: the body computes its inputs into local slots, then performs a single productive `<-` write near its end. This pattern naturally satisfies the rule and supports the atomic-compound-construction guarantee (§7.11).
 
### 3.5 Reference Parameters
 
A reference parameter (`&name`) carries no write obligation: the callee may read it, write it, or do neither. The caller's pre-call slot must be initialized (the parameter is readable from the callee's perspective, so reading uninitialized would be a static error per §6.13's whole-slot tracking). Copy-restore at the call boundary preserves failure atomicity: any write the callee makes to the slot during its execution is committed back to the caller's slot only if the call succeeds; on failure the caller's slot reads identically to its pre-call state.
 
Reference parameters are the natural shape for in-place mutation (§6.9). A method that updates a counter, a parser that advances a position, a logger that mutates an output buffer — all use reference receivers or reference parameters. The contrast with productive mode is the obligation: PRODUCE commits to a write on every successful path, while REFERENCE commits to nothing.
 
### 3.6 Implicit Context Parameters
 
Parameters listed after the `/` separator in a command's signature are *implicit context parameters* — the language's Scala-implicit-style mechanism for threading shared context through call chains. They are filled at the call site by *uniqueness-of-type* resolution against the caller's lexical scope: at the call point, the typechecker searches the caller's in-scope identifiers for one whose type matches the implicit parameter's declared type, and supplies that identifier as the argument. Ambiguity (two in-scope values of the same type) is a static error, resolvable by the caller passing the value explicitly. Absence (no in-scope value of the matching type) is also a static error.
 
```
.cmd writeAll: List[String] lines / Logger logger = ...
 
.cmd reportAll: List[String] lines =
    #log <- (Logger: "console")
    writeAll: lines           ; logger filled by uniqueness-of-type
```
 
The mechanism resolves at compile time. The provision-chain reading of the no-non-local-state principle (§1.4) treats implicit context parameters as a *syntactic* convenience — the value must actually exist in the caller's lexical scope to be eligible, so the implicit's resolution does not enable access to state that was not already accessible. The implicit list is not a way to reach ambient state; it is a way to thread accessible state through call chains without writing the parameter at every call site.
 
All three parameter modes are admitted in the implicit list. Three rules govern the slash-list's internal grammar:

- **Commas are required.** Implicit parameters in the slash-list are separated by commas, exactly as in the regular parameter list. Whitespace alone is not a separator.
- **The `/` does not introduce a new indentation grouping level.** The slash is a separator within the signature, not the head of a nested block. A signature that wraps across lines does so under the signature's ordinary continuation context; the `/` does not change the indentation discipline.
- **The result designator (§3.7) may not name an implicit context parameter.** The `-> name` clause selects from regular parameters and receivers only. Implicit parameters are caller-injected context, not output channels; designating one as the expression-position result would conflate context resolution with productive output.
 
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
 
The constructor case is §3.9 above. Receivers are always carried at the marker placement of the named-context rule (§3.3) — `Type 'name`, `Type &name`, or `Type name`. The full receiver-mode discipline including the *R1* (call-site initialization) and *R2* (callee-body obligation) rules is in §6.7. The block markers `@` and `@!` (§3.13) are not signature shapes — they introduce bodies, not methods, and do not carry receivers at the signature level.
 
### 3.11 Multi-Receiver Methods
 
A method invocation over multiple receivers takes a tuple of receivers, parenthesized, dispatching based on all their types in concert:
 
```
.cmd (Logger logger, Severity severity) :: format: String 'result, String message =
    ; body authored against Logger and Severity classes;
    ; (logger :: emit) and (severity :: prefix) dispatch independently
    ...
 
(consoleLogger, warning) :: format: 'output, "couldn't open file"
```
 
The receiver tuple appears in parentheses both at declaration and at call.
 
The dispatch implementation composes per-receiver single-class dispatches — there is no joint-instance dictionary keyed on the tuple of receiver types. The combined behavior is the product of the receivers' types, but each receiver's dispatch resolves through its own class's dictionary independently. This admits methods that span receivers from different modules without requiring those modules to coordinate: the implementations of `Logger`'s `emit` method and `Severity`'s `prefix` method are looked up separately at the call site.
 
Each receiver in a multi-receiver method declaration carries its own mode marker. Different receivers may carry different modes — `(Logger logger, Counter &c)` is a valid receiver tuple with logger READ and counter REFERENCE. The R1 (call-site initialization) and R2 (callee-body obligation) rules apply to each receiver independently per its declared mode.

### 3.12 Subcommands

A **subcommand** is a command declared with the `.sub` keyword inside another command's body. Subcommands are lexically scoped to the enclosing command's body — they are not visible outside it, cannot be imported, and cannot be reached through any class-dispatch mechanism. The form is:

```
.cmd factorial: Int n, Int 'result =
    .sub factorialAcc: Int n, Int acc, Int 'r =
        ?: n == 0
            'r <- acc
        factorialAcc: n - 1, n * acc, 'r
    
    factorialAcc: n, 1, 'result
```

The `.sub` introducer takes the same signature surface as the regular `.cmd` form (parameters, implicit context parameters, mode markers, failure marks, the `-> name` result designator) and the same body grammar. The full failure-mark surface (`:`, `?`, `!` per §4.2) and the full signature surface (PRODUCE, REFERENCE, READ parameters; implicit context parameters; single or multiple writeable parameters) apply identically to subcommands and regular commands.

**Restrictions.** Subcommands are non-method, non-constructor commands:

- A subcommand has **no receiver**. It is not a method (no `Type ::` prefix) and not a multi-receiver method (no parenthesized receiver tuple).
- A subcommand is **not a constructor**. The productive-receiver-of-named-type form `Type 'r:` that introduces a constructor (§3.9) is not admitted on `.sub`.
- The **`.implicit` directive does not apply** to subcommands. `.implicit` registers a constructor at module-import time; subcommands are body-local and have no module-import surface to register against.

**Placement.** Subcommand declarations appear strictly at the top of the enclosing command's body — after the `=` sign and before the body's main statement group (or the `_` placeholder, where the body has no executable statements other than the subcommands themselves). They form a contiguous declaration block; once executable statements begin, no further `.sub` declarations are permitted in that body. The placement makes all sibling subcommands in scope for each other before any of their bodies execute, supporting mutual recursion among siblings.

**No capture from the enclosing frame.** A subcommand's body forms its own static scope. It does not capture from the enclosing command's frame — the subcommand's inputs are its explicit parameters and its implicit context parameters (resolved at each call site, §3.6); module-scope identifiers remain visible as in any command body. The enclosing command's locals are not directly visible inside the subcommand's body. Each subcommand invocation creates a fresh frame at the call site, with its own initialization tracking, failure-mode analysis, parameter-passing copy-restore semantics, and `@` / `@!` block-marker registration list (§3.13). Operationally, a subcommand call is indistinguishable from a regular command call except in how its name is resolved at compile time.

**Visibility and nesting.** A subcommand is visible from the body it is declared in and from any deeper bodies nested within that body — including the bodies of sibling subcommands and the bodies of subcommands nested deeper. Subcommands may nest: a subcommand's body may contain its own `.sub` declarations at the top, and those nested subcommands follow the same scoping discipline against their immediate enclosing body. A nested subcommand resolves names through its lexical chain: itself (for recursion), its siblings within the same enclosing body, and any subcommand visible at any outer enclosing body along the chain to the outermost enclosing command. The visibility chain follows the lexical nesting structure; it does not extend to the module or any other command.

**Two design purposes.** Subcommands serve two needs that would otherwise require separate language features:

1. **Helper commands internal to a command.** Many algorithms have a clean specification at the call boundary but an implementation that wants a private helper — a tail-recursive version with an accumulator, a worker function not exposed in the module's public surface, a body that wants to be expressed as several cooperating commands without polluting the module's namespace. The factorial-with-accumulator example above is the canonical case: `factorial` exposes the simple input-and-result signature its callers expect, while `factorialAcc` carries the tail-recursive implementation as a private helper, threading the productive `'r` slot through each recursive call.

2. **Ad-hoc scoping for `@` and `@!` blocks.** Basis has no unnamed block scope. Where another language might introduce one to ensure a destructor fires, Basis uses a subcommand whose frame retirement triggers the `@` and `@!` blocks registered within it:

   ```
   .cmd processFile: String path =
       .sub useFile: String filepath =
           #handle <- openFile: filepath
           @ closeHandle: handle      ; fires when useFile returns
           process: handle
       
       useFile: path
       ; closeHandle has already fired here
   ```

   The subcommand's frame retires when `useFile:` returns; the `@`-block registered in its body fires at that retirement. The enclosing command continues with the resource released. The pattern is uniform for any cleanup whose scope is smaller than the enclosing command's body.

Both purposes are served by the same mechanism — a lexically scoped command whose frame creates a fresh `@`/`@!` registration context — without introducing a separate ad-hoc-scope construct.

### 3.13 Frame-Exit Hooks: `@` and `@!`

A command body may register inline cleanup that runs at frame retirement using the `@` and `@!` **block markers**. The forms are:

```
.cmd processFile: String path =
    #handle <- openFile: path
    @ closeHandle: handle           ; runs whether we succeed or fail
    @! logIncomplete: path          ; runs only on failure path
    process: handle
```

`@` is read "at exit"; `@!` is "at exit on failure." Each marker introduces a body whose execution is deferred to the enclosing command's frame retirement, not to the source line where the marker appears.

The handlers fire in **reverse order of registration** — the most-recently-introduced handler runs first. They compose with the failure system: if a frame is retiring on a failure path, the propagating failure remains in flight while the handlers run, then continues to propagate after they complete. Handlers themselves may invoke commands that fail internally and recover internally; what they may not do is generate a *new* in-flight failure during the exit cleanup, which would conflict with the single-in-flight invariant (§4.12).

`@` and `@!` blocks are not destructors. They are RAII-equivalent in functional role; they are not RAII in mechanism.

**Block markers only; no class-level form.** Frame-exit hooks are admitted *only* as block markers within command bodies. There is no `.cmd @ Type ::` or similar class-method form that would auto-register a handler against every frame slot holding a value of a particular type. The block-marker form is the entire surface; cleanup tied to a resource-managing type is the user's explicit responsibility at each use site. The block-marker form provides deferred firing: cleanup is colocated with acquisition at the registration site, and the firing happens at frame retirement.

The receiver-mode rules of the parent command's body apply to the `@` and `@!` block's body the same way as to any other sub-block: variables in scope at the block's registration point are accessible inside the block, with their mode markers preserved. The block runs in the parent command's frame context at retirement; it has no parameter list of its own.

The full block-marker semantics for `@` and `@!`, including their composition with failure flow, are in §4.4 and §4.11.
 
### 3.14 Calling Commands
 
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
 
Arguments are always comma-separated. The grammar treats the colon as the start of the argument list and the indentation/dedent transitions as standard whitespace, so the argument list can extend across lines when indented under the call's first line.
 
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
 
### 3.15 The `_` Placeholder
 
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
 
### 3.16 First-Class Commands
 
Commands are values. A command-typed value may be stored in a field, passed as an argument, returned from a constructor, or invoked indirectly. The language admits four constructional forms that produce command-typed values:
 
| Form | Surface | Captures? | Body? |
|---|---|---|---|
| Command reference | `{name}` or `{cmd: x, _, y}` | No | No (refers to existing command) |
| Command literal | `:<args>{body}` (also `?<...>`, `!<...>`) | No | Yes |
| Lambda | `:<args / caps>{body}` | Yes (explicit slash list) | Yes |
| Fexpr | `:{body}` (also `?{body}`, `!{body}`) | Yes (implicit by free name) | Yes |
 
The four forms cover the design space — function-pointer-style references, eagerly-evaluated thunks, closures over defining-frame state, and user-defined control-flow primitives — through a uniform set of constructions that share the failure-mode discipline (the `:` / `?` / `!` mark) and the four-way taxonomy.
 
Command-typed values are typed by command-type expressions of the form `:<paramTypes>`, `?<paramTypes>`, or `!<paramTypes>`, with mode markers as suffix on each parameter type per the nameless-context rule (§3.3): `:<Int, Int'>`, `?<String'>`, `!<>`. Fexpr-typed values are typed by the parallel family `:<*>`, `?<*>`, `!<*>` — the `*` distinguishes the fexpr family from ordinary command-typed values, and there is no subsumption across the family boundary. The full type-form tables and family rules are in §5.14 and §5.15.
 
Receivers are *always* applied at the partial-application site for command references (`{logger :: log}` resolves dispatch and bakes the receiver in immediately); non-receiver parameters may be applied or deferred (`_`). The full mechanics of partial application — including the mode-marker filter (PRODUCE deferred-only, REFERENCE applied with ceiling-tracking, READ flexible) — are in §9.14.
 
The four constructional forms — their capture rules, their ceiling computations, their mark-conformance rules, and the seven fexpr-restrictions A–G — are detailed in §8. The class-system extension that admits fexpr-typed parameters under fexpr-relevance taint is in §9.20.
 
---
 
## 4. Failure and Recovery
 
A *failure* in Basis is a propagating signal that some path of execution did not reach its intended outcome. Failures are first-class control flow, not an out-of-band channel: they are produced by the `.fail` directive, propagate up the call stack by skipping subsequent siblings at each indentation level, and are consumed by recovery contexts at well-defined source positions. Every command's signature declares what failures it admits and how it composes with callers; the typechecker's failure-state lattice (§4.13) verifies the declarations.
 
This section describes the failure-handling surface end-to-end: what a failure carries (§4.1), how the three failure-mode marks compose (§4.2), how `.fail` produces a failure (§4.3), how the eleven block markers enumerated in §3.1 dispatch failure flow (§§4.4–4.5), how recovery handlers match and bind failure values (§§4.6–4.8), how messages are declared in hierarchies and propagated through signature-level failure sets (§4.9), how a recovered value can be re-emitted as a fresh failure (§4.10), how at-stack handlers compose with failure flow (§4.11), why at most one failure may be in flight per thread (§4.12), and how the typechecker's six-state failure-state lattice gates body conformance (§4.13). The full transfer-function table for the lattice is in Appendix E; the operational rules for failure propagation are in Appendix F.
 
### 4.1 Failures Are Not Exceptions
 
A Basis failure is not an exception in the C++/Java/C# sense. There is no stack-unwinding mechanism that runs ad-hoc cleanup as it climbs frames, no try/catch construct, no special "exception state" lying outside the language's normal semantics. Failures are an ordinary state in a six-state lattice (§4.13), and propagation is structural: the failure-state lattice transitions at each block marker according to a transfer function that can be read off the source position, and propagation past a frame's body is by skipping subsequent siblings at each indentation level — not by unwinding.
 
A failure carries two pieces of information: an identity (the **message** — see §4.9) and an optional **payload value** whose type satisfies the message's bound class (§4.7). The message names the kind of failure; the payload carries any data the failure needs to convey to a downstream recovery handler. Both are stored in a fixed-size **failure slot** allocated as part of each command's frame; the slot is occupied only while a failure is in flight. Propagation copies the slot's contents up the stack — three words on a 64-bit target: the message identifier, a pointer to the payload's storage, and a class witness for the payload's concrete type with respect to the message's bound class — without copying or moving the payload value itself. The payload stays put until a recovery handler binds it (§4.6), at which point it moves into the handler's frame.
 
Cleanup that mainstream languages express through `try`/`finally` is expressed in Basis through frame-exit hooks `@` and `@!` (§§3.13, 4.4, 4.11). The hooks compose with the failure flow — `@` runs on every frame exit regardless of outcome, `@!` runs only on failure exits — but they are not part of the failure machinery itself. A frame's hooks fire on its retirement schedule; the failure that may be in flight at retirement is a separate thing the hooks neither see nor consume.
 
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
 
Signature-level failure-set declarations — how a command's signature names the messages it may emit — are addressed in §4.9.
 
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
 
The `@` and `@!` markers are **frame-lifetime-tied**. They register a body against the enclosing command's frame; the body executes at that frame's retirement, in reverse registration order. This is the language's only intentional exception to the no-invisible-control-flow principle, and it is contained to the explicit registration via the `@` or `@!` marker — no class-level declaration causes implicit registration for frame slots of any particular type (§3.13). The discipline that governs *when* these blocks fire under a propagating failure is in §4.11.
 
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
 
A `|`-with-spec block engages on failures whose message matches the spec, with subtype-inclusive matching against the relevant hierarchy. The surface is `| Name name -> body`. The spec engages on:
 
- A failure whose message is exactly `Name`.
- A failure whose message is a child of `Name` in `Name`'s hierarchy (recursively, to any depth).
Failures whose messages are not at-or-below `Name` continue propagating past the block. The bound name `name`, when present, is in scope throughout `body` and refers to the **payload only** — not the full failure, not the message identifier. The bound name's static type is the **spec's payload class** — the class declared on `Name` (the spec) at its `.msg` declaration — regardless of whether the engaged failure was `Name` itself or one of its descendants in the hierarchy. A descendant message's payload-class (a subclass of the spec's class per §4.8) does not surface its more-specific operations to the handler; the handler is constrained to operate at the spec's class abstraction level. Programs needing access to a descendant's more-specific class operations catch at that descendant's message specifically, or apply class-narrowing (§7.14) within the handler body.
 
The presence of the bound name in the spec is governed by an asymmetric rule against the message's declaration. A payload-bearing `Name` admits the bound name (binding the payload) or its omission (engaging without binding; the engaged failure's payload is consumed unobserved). A payload-less `Name` requires the bound name's omission — supplying one is a static error, since there is no payload to bind to. The asymmetry permits the pattern of catching a class of failures uniformly without inspecting their payloads, while excluding the malformed case of a binding with no value behind it.
 
Match-by-message-identity, not match-by-payload-value, is a design commitment: failures are dispatched on what they *are*, not on what they *carry*. Forms like `| 0 -> handleZero` or `| "error" -> handleError` are not part of the language. Programs that need to distinguish payload values do so by inspecting the bound payload within the recovery body.
 
When a `|`-with-spec block engages, the failure's status as in-flight ends at the moment of binding. The handler body runs from no-active-failure, with the bound payload available for inspection, consumption, or forwarding into a re-fail (§4.10). The discipline governing the *value's* lifetime — distinct from the failure's status, and tracked through the holding-frame model — is in §4.11.
 
### 4.7 Class-Bound Payloads
 
A failure-message declaration may specify a **payload class** — a Haskell-style typeclass that any payload of that message must satisfy. The class is a *contract*: a set of operations the payload value supports, not a concrete type with a layout. The class itself has no `sizeof`, no allocation footprint, and no instances of its own; *types* satisfying the class are the things with layouts.
 
The corollary: different `.fail` sites for the same message may pass payload values of different concrete types, all satisfying the bound class. Refactoring the concrete type at one `.fail` site to a different type that also satisfies the class is non-breaking; refactoring the class itself — adding, removing, or changing operations — is breaking. The boundary is the class contract.
 
A class **witness** — a typeclass dictionary corresponding to the (concrete-payload-type, bound-class) pair — travels with the propagating failure as the third word of the failure slot (§4.1). The witness is selected at the `.fail` site: at compile time, the typechecker has both the concrete type of the payload expression and the bound class from the message's declaration, and emits the corresponding dictionary as a witness pointer. No runtime witness construction is required.
 
A `|`-with-spec consumer that binds a payload uses the witness to dispatch class operations on the bound name. The witness's identity — which concrete type's dictionary it is — is **opaque** to the consumer, which sees only the class's operations. This is the core of the failure system's open-extensibility property: a recovery handler defined against a class can consume payload values of types it has never heard of, as long as those types satisfy the class. Under payload-class covariance (§4.8) and the Liskov-covariance rule for payload classes (§9.17), this property extends across hierarchy depths: a handler at a parent message's class binding a descendant message's payload receives the value at the parent's class, with class operations dispatched through the descendant's witness — the parent's operations are a structural subset of the descendant's, so the witness's dictionary supplies them by the same single-pointer mechanism that backs all class dispatch (§9.7).
 
The full class system (typeclass declarations, instance declarations, dispatch mechanics, witness flow) is in §9. Failure messages and their bound classes participate in that system on identical terms; the failure machinery is a refinement of class dispatch applied to propagating failure values, not a separate parallel mechanism.
 
### 4.8 Messages Without Payloads
 
A failure message may be declared without a payload class. A `.fail` for such a message carries no value — the directive form is `.fail Name` with no following `:` or value — and a `|`-with-spec recovery for such a message has no bound identifier. The body of the recovery simply runs.
 
A payload-less message retains nominal identity, hierarchy position (§4.9), and full participation in failure-set inclusion. Its composition with payload-bearing messages within a hierarchy is constrained by recovery soundness: a handler `| ParentMsg p -> body` that binds `p` and applies `ParentMsg`'s class operations to it must remain sound for every descendant message that could engage the handler. Two rules follow.
 
**A payload-bearing child of a payload-less parent is well-formed.** The child introduces a payload class at its level; payload-less ancestors impose no class constraint on it. A `|`-with-spec at a payload-less ancestor produces a payload-less handler (no binding identifier) whose body simply runs when engaged — the engaged descendant's payload, if any, is consumed unobserved.
 
**A payload-less child of a payload-bearing parent is ill-formed.** A handler `| ParentMsg p -> body` would attempt to bind `p` to the engaged message's payload and apply `ParentMsg`'s class operations to it; a payload-less descendant engaging that handler would leave `p` with no value to bind. Once a payload class is introduced at some level in a hierarchy, every message at-or-below that level must also declare a payload class.
 
The soundness argument extends across multi-level chains, requiring **payload-class covariance** along ancestor-descendant paths: every payload-bearing message's class must be a subclass of (or equal to) every payload-bearing ancestor's class along the path from the message toward the root. The covariance chain ensures that a handler bound at any payload-bearing ancestor can apply its class operations soundly to any descendant message's payload, regardless of which descendant engages. The class-system mechanism that supports this — Liskov-covariant subsumption at payload-class positions — is §9.17.
 
The recovery's binding shape is thus determined entirely by the spec: a payload-less spec produces a handler with no binding identifier; a payload-bearing spec produces a handler binding the engaged message's payload as a value of the spec's class, with class operations on the bound name dispatching through the engaged failure's class witness (§4.7).
 
### 4.9 Failure Messages and Hierarchies
 
Failure messages are declared with the `.msg` directive. The surface is:
 
```
.msg Name[PayloadType] : ParentMessageType
```
 
Both the `[PayloadType]` brackets and the `: ParentMessageType` clause are optional. A message declared without a payload type is payload-less (§4.8); a message declared without a parent is the root of a new hierarchy. Examples:
 
```
.msg Net                                       ; root, payload-less
.msg Net::Disconnected[ConnectionInfo] : Net   ; child of Net, payload satisfies ConnectionInfo class
.msg Parse[String] : Net                       ; sibling of Disconnected, payload satisfies String class
```
 
**Payload-class covariance across the hierarchy.** A child message's payload class must be a subclass of (or equal to) every payload-bearing ancestor's payload class along the path to the root (§4.8). The rule rests on the Liskov-covariance discipline at payload-class positions (§9.17): a value satisfying the child's payload class also satisfies the parent's payload class via standard class-system subsumption, so a parent-class handler engaging on a child-message failure binds the payload at the parent's payload class and operates on it through the parent's class operations. The bound value's static type is the spec's class — descendant payload classes' additional operations are not visible at parent-class handlers. The discipline keeps recovery handlers operating at the level of abstraction the handler declared, not at the engaged failure's potentially more-specific class.
 
Messages form a **forest** — a collection of independent hierarchies, each rooted at a user-declared root. There is no language-imposed Top message, no universal ancestor analogous to Java's `Exception`. This is a deliberate design commitment. A universal ancestor invites generic catch-and-bind-anything practice, the very pattern that makes failure handling sloppy in languages that have one. The forest structure forces programs that bind a payload to commit to which hierarchies they understand. Code that wants to catch anything without binding can use the bare `|` form — that idiom is preserved without admitting Top as a type.
 
A downstream module **may extend** the hierarchy of an imported message by declaring new children of a foreign root or of any imported descendant. The failure-message hierarchy participates in the same coherence machinery as class instances (§9.15, §9.17): intra-module uniqueness, cross-module specificity ranking, orphan permissibility, import-time competition warning. Payload-class covariance (§4.8) keeps the extension safe — a parent-class handler observes any descendant's payload through the parent's class operations only, so new descendants cannot widen what a parent-class handler sees.
 
A **failure set** is a set of messages. A command's signature carries a failure-set component declaring which messages the command may emit, expressed either as an explicit list or as the closure-at-or-below of a higher-level message. By convention, a set listing `Net` denotes "any message at-or-below `Net`"; a set listing several messages denotes the union of their respective at-or-below closures.
 
A set `A` **subsumes** a set `B` (notation `B ⊆ A`) iff for every message `m` in `B`, there exists a message `m'` in `A` such that `m` is at-or-below `m'` in `m'`'s hierarchy. Messages from different hierarchies are mutually incomparable; nothing in one hierarchy subsumes anything in another. The relation is reflexive, transitive, and antisymmetric (modulo at-or-below equivalence within a single hierarchy, which is trivial).
 
A caller invoking a command with declared failure set `D` is responsible for ensuring its enclosing signature's set `E` satisfies `D ⊆ E`, modulo any sets consumed by recovery within the caller. A caller may *widen* (`E` a superset of `D`) at the cost of precision; a caller may *not* narrow at a call site. A `|` consumes failures of certain messages: bare `|` consumes the entire propagating set; `| Name -> ...` consumes failures whose message is at-or-below `Name`. The post-block propagating set is the original set minus the at-or-below closure of the matched message.
 
**Surface form for declaring a failure set.** A failure set is declared by attaching square brackets containing message names to the failure mark in a signature or command-type expression:

```
.cmd ?[Net] process: Foo a -> r = body                  ; declares failure set {at-or-below Net}
.cmd ?[Net, Parse] handle: Foo a -> r = body            ; declares {at-or-below Net} ∪ {at-or-below Parse}
.cmd ![Fatal] crash: Bar b = body                       ; must-fail with declared set
```

The brackets sit immediately after the mark, in whatever position the mark appears: before the command name in a regular declaration, before the method name in a method or method-with-PRODUCE-receiver signature, before the type-expr in a default constructor, and before the angle-bracket group `<...>` or fexpr marker `<*>` in command-typed values and types. The `:` (never-fails) mark cannot take brackets — the empty set is implicit. An empty bracket list (`?[]`) is a static error: a `?` mark declares may-fail, which contradicts an empty set; use `:` (never-fails) instead.

**Annotation is opt-in; if declared, the set is enforced as a contract.** A signature may omit the brackets, in which case the failure set is **inferred** from the body — the union of messages produced by `.fail` directives in the body and by called commands' declared (or inferred) failure sets, minus any messages consumed by recovery handlers within the body. When the brackets are present, the typechecker enforces them as the upper bound on the body's emitted failures: every message the body can emit must be at-or-below some message listed in the brackets (by the subsumption relation above). An emission outside the declared set is a static error at the offending `.fail` or call site.

The typechecker tracks the failure set of every command, annotated or not, for use at call sites. The inference algorithm — fixed-point analysis over the call graph, joined with the failure-state lattice (§4.13) — is implementation-defined beyond this characterization.

**Additive form for callee-propagated failures.** A failure-set annotation may opt into an **additive** mode by placing a leading `+` inside the brackets: `?[+ Net]` denotes "whatever propagates from callees, plus the closure-at-or-below of `Net`." The list following the `+` is the set of messages the command adds *directly*; the implicit component is the union of the failure sets of all callees reachable from the body — statically resolved for direct calls, polymorphically resolved through callable parameters at each call site of this command. The typechecker assembles the full set per call site.

The additive form is the surface for higher-order commands and for any command that prefers to let callee-propagated failures pass through without enumerating them. Forms:

```
.cmd ?[+] forEach: ?<Foo> f, []Foo input = body          ; exactly whatever f emits
.cmd ?[+ Net] withRetry: ?<Foo> f = body                 ; whatever f emits, plus Net
.cmd ?[+ Net, Parse] mapDecode: ?<Foo> f, ... = body     ; whatever f emits, plus Net and Parse
```

The restrictive form `?[stuff]` (no leading `+`) and the additive form `?[+ stuff]` are mutually exclusive — an annotation is either one or the other. There is no hybrid syntax, since a restrictive declaration *with* implicit callee inclusion would contradict the restrictive enforcement (callee-propagated messages outside the declared set would silently widen the contract).

The bare additive form `?[+]` is well-formed: it admits whatever callees emit, with no command-specific additions. The empty restrictive form `?[]` remains a static error (per the rule above).

A class method's failure set must be uniform across all instances. At a class-dispatched call, the typechecker knows the class and the method but not the instance; the failure set must therefore be a property of the (class, method) pair, not of the instance. An instance's implementation may emit any subset of the class-declared set; emitting a message outside the set is a static error. A class method declared additive (e.g., `.decl ?[+ Net] traverse: ?<T> f`) propagates the callable parameter's set uniformly across instances, with the substitution happening per call site.

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
 
When the new `.fail`'s payload is the bound name from a preceding `|`-with-spec, the value **moves** from its current holding frame (the recovery frame) into the new `.fail`'s originating frame (the handler's frame). The value's identity is preserved; only its holding frame changes. The witness for the new failure's slot is selected at the re-fail site at compile time, against the new message's bound class — which may differ from the original message's bound class. The static check at re-fail compares the bound payload's static class (the engaging handler's spec class) to the new message's payload class: the bound class must be a subclass of (or equal to) the new message's class under the Liskov-covariance rule (§9.17). If the bound class is not assignable to the new message's class under that rule, the re-fail is a static error. The check is on the static class only — the typechecker does not have the engaged failure's runtime concrete type at re-fail time, only the bound spec class. The discipline composes cleanly: a chain of recover-and-re-fail handlers each performs a move into the next holding frame, until a final handler consumes the value without re-failing.
 
The value's lifetime across a re-fail chain is governed by the holding-frame model in §4.11. Each level of the chain performs a move of the value into the next holding frame; the value's identity is preserved across these moves.
 
### 4.11 At-Stack Handlers Under Failure
 
The frame-exit hooks `@` and `@!` (§4.4) compose with failure flow as follows.
 
`@`-bodies run on every frame exit. When a failure propagates out of a body containing `@` blocks, the `@` bodies fire as part of the frame's retirement, in the order they were registered, before the failure propagates to the caller. The bodies execute as if no failure were active: they neither see nor consume the propagating failure. A failure produced *within* an `@`-body propagates through the cleanup machinery's own rules, not back into the original failure's flow.
 
`@!`-bodies run only on failure exits. The composition with the propagating failure is identical to `@`'s, restricted to the failure-exit case.

**Block hooks are frame-bound, not value-bound.** The `@` and `@!` registration list is a property of the registering frame, not of any value the block's body references. When a value referenced inside an `@`-block is moved out of the frame — for example, used as a payload in a `.fail` directive — the block's reference to that name continues to refer to the original frame slot, which after the move no longer holds the value. The user is responsible for ensuring that the order of operations within the frame body keeps the block's references valid until the block fires, or for registering cleanup at the destination frame instead. The language does not transport block-registered cleanup across move boundaries; the **holding frame** model below governs the *value's* location across the failure-flow path, while the `@` and `@!` registration list remains anchored to the frame in which the marker appeared.

**The holding-frame model.** At any moment, a payload value lives in exactly one frame, its *holding frame*. Initially the holding frame is the originating frame of the `.fail` that produced the value. Propagation up the call stack copies the failure slot's three words (message, pointer, witness) without moving the value — the holding frame does not change during propagation. The holding frame *does* change at two events: at **binding**, when a `|`-with-spec engages and the value moves into the recovery frame; and at **re-fail**, when a fresh `.fail` in the handler's frame takes the bound value as its payload and the value moves into the new originating frame. Both events are moves: the value's identity is preserved; only its holding frame changes. The model describes the value's location, not any handler timing — cleanup of a value that has moved out of its originating frame is the recovery handler's responsibility, expressed through ordinary statements within the handler body or through `@` and `@!` registrations within that body. There is no class-level mechanism that auto-registers cleanup for values of a particular type.
 
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
 
A typechecker implementation may begin with the un-refined six-state lattice and add the failure-set component incrementally; dropping the failure-set component recovers the un-refined lattice exactly.

When a recovery block engages, the propagating-set component is narrowed: a bare `|` consumes the entire set; a `|`-with-spec narrows per-root from the spec's at-or-below closure, with precise removal when the closure fully covers a root and conservative retention when it would only partially cover one (the set representation does not capture closures with holes). The formal per-root rule is in Appendix E.3 alongside the lattice's transfer functions.
 
---

on 5 · MD
## 5. Types
 
This section defines the type forms of Basis. Every value the language admits inhabits exactly one of the forms enumerated here. The forms partition into two categories — buffer-backed (§§5.2–5.7) and non-buffer (§§5.10–5.15) — with three additional facilities (aliases §5.8, enums §5.9, and the cross-cutting subsumption rule §5.5) that operate over both. Each form's surface declaration syntax is given here in sketch; the full grammar is in Appendix B. Each form's construction surface — how values of the form are introduced and assigned — is in §7. Each form's interaction with parameter modes and class dispatch is in §6 and §9 respectively.
 
### 5.1 The Two-Layer Split
 
Every type in Basis falls into exactly one of two categories. The split is structural: it determines what positions a type may inhabit, how it is initialized, how it composes with other types, and how the static analyses treat it.
 
**Buffer-backed types** are types whose representation reduces — transitively — to bytes. Buffer-backed values are byte-copyable, fit inside other byte-aggregates, and are reclaimable at frame retirement without traversal. The category includes the buffer primitives `[N]` and `[]`, typed buffers `[N]T` and `[]T`, domains, records, and unions. Aliases that resolve to a buffer-backed type are buffer-backed; enums whose enumerated-type is buffer-backed are buffer-backed.
 
**Non-buffer types** are types whose representation includes references, identity, dispatch information, or other non-byte semantics. The category includes pointers, command-typed values (the `:<...>`, `?<...>`, `!<...>` family), fexpr-typed values (the `:<*>`, `?<*>`, `!<*>` family), objects, and variants.
 
The split is enforced by a **containment rule**: byte-aggregate containers — records, unions, typed buffers `[N]T`, and domain parents — admit only **fixed-size buffer-backed** contents. A record's fields must all be fixed-size buffer-backed; a union's candidates must all be fixed-size buffer-backed; a typed buffer `[N]T` requires `T` to be fixed-size buffer-backed; a domain's parent must be fixed-size buffer-backed. The runtime-length forms `[]` and `[]T`, while themselves buffer-backed, do not have a static byte-width and are therefore excluded from these positions. Non-buffer types may appear only at top-level positions — slots introduced via `#`, parameters, receivers — or as fields of objects or candidates of variants. Object fields and variant candidates are unrestricted as to category; they may hold fixed-size buffer-backed, runtime-length buffer-backed, or non-buffer types freely.
 
The grammar enforces the byte-aggregate-vs-other distinction by partitioning the type-expression non-terminal: record-field, union-candidate, `[N]T`-element, and domain-parent positions admit a `fixed-size-type-expr` (Appendix B), which excludes the runtime-length forms `[]` and `[]T` and all non-buffer forms. Object-field and variant-candidate positions admit the full `type-expr`. Named-type references in `fixed-size-type-expr` positions are checked by the typechecker against the resolved type's category.
 
The containment rule's load-bearing consequence is that mutation either succeeds fully or fails fully (principle 4, §1.2). A record's bytes are unambiguously a byte-aggregate, copy-restored to the caller's slot atomically on success and not at all on failure. A record-with-pointers would punch a hole in this — the pointer copies, the pointee doesn't, and partial-failure semantics drift. The containment rule eliminates the concern at the type level rather than relying on convention or analysis.
 
The split also determines whole-slot initialization tracking (§6.14): a buffer-backed slot is one byte-aggregate, not a graph, and tracking it as a unit is sound because no field can have non-byte semantics. The static analyses treat each form per its category, with the guarantees of one category not silently leaking into the other.
 
### 5.2 Buffer Primitives
 
The bracket form `[N]` denotes an `N`-byte buffer with the length fixed in the type. The form `[]` denotes a runtime-length byte buffer: its length is carried alongside the bytes at runtime rather than in the type. The forms `[N]T` and `[]T` denote buffers laid out as a sequence of `T`-values, sized to `N` elements in the type (`[N]T`) or to a count carried in the value at runtime (`[]T`). Buffers are the substrate over which all value-like types are interpreted: a `[4]` is four bytes — what those bytes *mean* (a 32-bit integer, an RGBA pixel, a packed pair of 16-bit values, a Unicode code point) is determined by the domain layered on top of it (§5.3). The fixed-size forms (`[N]`, `[N]T`) and the runtime-length forms (`[]`, `[]T`) are different representational categories; conversion between them is explicit (§5.5).
 
**Indexing into a buffer-shaped value uses the suffix `[index]` syntax.** Indexing is failable: out-of-bounds is a first-class failure, not undefined behavior. Indexing on a domain works because domains are themselves buffer-backed, transitively reducing to a buffer; whether indexing is *meaningful* on a particular domain (versus syntactically permitted but not idiomatic) is a domain-specific concern that the standard library and user code resolve at the domain level.
 
**C-style pointer arithmetic is not supported.** Stepping through buffer contents requires `[i]`. The restriction reflects two design commitments: indexing is a first-class operation that produces a checked failure on out-of-range access, while pointer arithmetic admits no analogous check; and the language reserves implementation latitude on pointer representation (§5.10), including handles that may relocate, which arithmetic against a stable address would alias incorrectly after relocation.
 
There are no privileged primitive types in Basis. The standard library defines `Int32`, `UInt32`, `Float32`, `Int8`, and similar names as domains over buffer-primitives of appropriate size, with associated intrinsics for arithmetic and comparison. User-defined domains use the same mechanism — there is no conceptual distinction between standard-library domains and user domains. The buffer primitives are the unifying substrate; everything else is a refinement.
 
### 5.3 Domains
 
A **domain** is a new type declared in terms of a parent type, where the parent must reduce — transitively, through any aliases — to a fixed-size buffer-backed type. The declaration form is:
 
    .domain Inches : Int32
    .domain Centimeters : Int32
    .domain RGBA8 : [4]
    .domain Tagged : [12]
 
Pointers, command-typed values, fexpr-typed values, objects, and variants cannot serve as domain parents because the purpose of a domain is to give a refined interpretation to a definite chunk of bytes — a non-buffer parent has no chunk of bytes to interpret. The constraint is structural and enforced at the grammar level.
 
**Domains form a parent–child hierarchy with one-directional implicit upcasting.** A value of a child domain is implicitly accepted wherever the parent domain is expected, but a value of the parent domain is *not* implicitly a value of any specific child. So an `Inches` value is implicitly an `Int32`; an `Int32` value is not implicitly an `Inches`. **Sibling domains do not implicitly convert.** `Inches` and `Centimeters`, both parented at `Int32`, are mutually incomparable for implicit conversion; an explicit constructor invocation is required to move between them.
 
The implicit upcast is a **typing-acceptance rule, not a value-rewriting rule**. The bytes underlying the value are unchanged across the upcast boundary; the static analysis simply accepts the value at the broader type. What changes at the boundary is the type-lens through which the value is interpreted: methods declared on a type interpret that type's bytes per that type's conventions, and methods declared on a parent or ancestor type interpret the bytes per the parent's conventions, which may differ. Witnesses for class dispatch are constructed at the relevant slot boundary based on the slot's declared type at that point — a witness constructed for an upcast value uses the upcast slot's type, with no mechanism (and no reason) to look back to a value's earlier slot history. The full rule for when buffer-backed dispatch identity is captured at a class-typed slot, and the conditions under which a child type's identity carries through to dispatch versus is lost in transit through intermediate non-class-typed buffer-backed parameters, is in §9.18.
 
Domains are first-class types: they may be parameters, fields, receivers of class methods, expression-position results via `-> name`, and so on. The hierarchy is **open for child extension**: a downstream module may declare a child of an imported domain. The implicit-upcast relation is structurally stable across this extension because the upcast is one-directional (child → parent) and child declarations do not widen the upcast set for any existing type.
 
### 5.4 Records
 
A **record** is a contiguous, byte-addressable buffer with named field offsets. Record values are value-like: copyable as bytes, with no identity beyond byte-content, and laid out at deterministic offsets within their containing storage. The declaration form is:
 
    .record Point : Int32 x, Int32 y
    .record RGBA   : UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha
    .record Header : UInt16 version, UInt32 length, [4] reserved
 
Record fields are constrained to fixed-size buffer-backed types (the containment rule of §5.1), reflecting the requirement that every field have a definite byte width and offset. A record is **packed**: its total byte size is exactly the sum of its fields' sizes, with no implementation padding, and each field's offset is the sum of the preceding fields' sizes in declaration order. The packed rule is load-bearing for record-to-buffer subsumption (§5.5) — a record whose fields total N bytes is `[N]` exactly. Object types (§5.11) are not byte-aggregates and have no such constraint; object layout is implementation-determined and may include alignment padding.
 
Records are **nominally typed** (§5.1's containment rule makes the structural matching well-defined; nominality is the choice on top): two `.record` declarations with identical field structure produce two distinct types. Values of one are not interchangeable with values of the other on the basis of structural similarity alone. The discipline is uniform across the type system; it is what gives module-exposed types their abstraction story.
 
Records compose: a record may have a field whose type is another record, and the inner record's bytes lay out within the outer record's bytes contiguously at the inner record's offset. This is the natural way to express compound buffer-backed structures.
 
The record/object split (§5.11) is the surface manifestation of the buffer-backed/non-buffer division: records are byte-aggregates with no identity beyond their contents; objects are identity-bearing aggregates whose fields may include non-buffer types. The choice between them is the choice between byte-aggregate semantics and identity-bearing-aggregate semantics — not a graded distinction, but a categorical one.
 
**Field access.** A record's fields are reached through the scope operator `::` (§9.6). The named form `record :: fieldName` selects the field by name; the positional form `record :: N` selects the Nth field by 1-based positional index, where positions are counted through `.inline` field promotion (§A.7). Both surfaces account for `.inline` uniformly: `.inline`-marked fields contribute their inner records' named fields to the outer's namespace *and* their inner records' positions to the outer's positional sequence. The `.inline`-field's own declared name remains a valid grouping reference — `outer :: inner` returns the inner-typed view of the inner's bytes — but does not correspond to a positional index. Out-of-range positional indices and unknown field names are static errors.
 
Records also admit **inline forms** — anonymous declarations within a field or candidate position. An inline record nested inside an outer record's field declaration produces a nominally distinct type per declaration site; the same field structure appearing in two separate outer records yields two distinct inline types. Named records are typically preferred for re-use; inline forms are a convenience for one-off compound structure inside another declaration.
 
### 5.5 Buffer-Backed Subsumption
 
Fixed-size buffer-backed types compose under a **uniform parent-chain subsumption** relation that operates across domains, records, unions, and the fixed-size primitive buffer forms together. A value of any fixed-size buffer-backed type subsumes upward through its parent chain — every ancestor along the chain accepts it implicitly — terminating at the relevant `[N]` primitive. **An `Inches` is implicitly an `Int32`; an `Int32` is implicitly a `[4]`.** A `Point` (record over an `[8]`-byte representation) is implicitly an `[8]`. The chain extends through every named refinement on the way; nothing is special about the named-domain step versus the record-to-primitive step.
 
The runtime-length forms `[]` and `[]T` are **leaves in the subsumption lattice**. They participate in the buffer-backed category but have no implicit parents and no implicit children: implicit subsumption never crosses the boundary between fixed-size types (which encode length in the type) and runtime-length types (which encode length in the value). Movement between a fixed-size type and a runtime-length type is an explicit operation — a constructor that wraps the fixed-size bytes with a length tag, or that reads bytes from the runtime-length value into a fixed-size slot under a runtime length check.
 
**Sibling buffer-backed types subsume to their common ancestor without peer-conversion.** Two domains parented at `Int32` — `Inches` and `Centimeters` — are mutually incomparable as types; they do not implicitly convert to each other. Each individually subsumes upward through the shared `Int32` ancestor: a context expecting `Int32` accepts either; a context expecting `[4]` accepts either. A context expecting `Inches` rejects a `Centimeters` value, and a context expecting `Centimeters` rejects an `Inches` value. The sibling-to-common-ancestor pattern is what makes domain hierarchies usable for unit-style refinements without admitting silent unit conversions.
 
The subsumption is **type-acceptance, not value-rewriting**. The bytes are unchanged at the upcast boundary; the static analysis accepts the value at the broader type. Methods and class witnesses are selected at each slot boundary based on the slot's declared type at that point — different types may interpret the same bytes differently, and a witness constructed at any class-typed slot boundary reflects that boundary's slot type rather than any earlier slot type the value passed through. The rule for when buffer-backed dispatch identity is captured at a class-typed slot, and the conditions under which a child type's identity carries through to dispatch versus is lost in transit through intermediate non-class-typed buffer-backed parameters, is in §9.18.
 
The subsumption rule is one-directional: child to parent, never the reverse. A value of a parent type does not implicitly become a child value. Constructing a child value from a parent value requires an explicit constructor invocation (§3.9), which the typechecker recognizes as a deliberate cross-type movement.
 
A separate buffer-backed subsumption rule applies to unions: the **union → candidate-or-parent byte-reinterpretation** rule (§5.7) admits a union value into any buffer-backed slot whose type is on the parent chain of *at least one* of the union's declared candidates. This rule is given its own subsection because it differs from the parent-chain subsumption rule in two important respects: it is existential across the candidate set, not universal; and it is **not Liskov-preserving**, since the bytes' meaning depends on which candidate is currently active in the union — a property the language does not track.
 
The narrowness of the implicit-conversion story across buffer-backed types is a deliberate design commitment. Implicit conversions are a routine source of reasoning errors in languages that admit them. The parent-chain rule is the minimum that makes refinement-style domain hierarchies usable; the union byte-reinterpretation rule is the buffer-backed-side answer to the discriminated-overlay question that variants answer differently (§5.12). Every other type-crossing — record to record, sibling domain to sibling domain, anything to or from a non-buffer type — is explicit, requiring a constructor invocation, an interpretive cast against a union, or the dynamic-narrowing operator `-<` (§7.14).
 
### 5.6 Unions
 
A **union** is a byte-level overlay of declared candidate types. The union's storage is `max(candidate-sizes)` bytes; assigning a candidate value writes that candidate's bytes into the overlay. The declaration form is:
 
    .union Number : Int32, Float32
    .union AnyFour : Int32, [4], RGBA8
 
Union candidates must be fixed-size buffer-backed (the containment rule of §5.1). This is what makes the byte-overlay coherent: every candidate has a definite byte-width known at typecheck time, and the overlay is the maximum.
 
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
 
The expression-position operators on pointers are the suffix `^` (dereference) and the suffix `&` (address-of). A read through a pointer-typed slot uses `p^`; the address of a slot is `x&`. The full surface for these operators, including their interaction with the construction surface and the access-path discipline, is in §7 and §6.5 respectively. Construction of a pointer to fresh storage — as distinct from `&` taking the address of an existing slot — uses the value-construction surface of §7.20, with the storage placement (frame region by default, or the named allocator under a `\` qualifier) determined there.
 
The language commits to **abstracted pointer semantics**. The user-visible meaning of `^T` is "pointer to `T`." Whether the runtime implementation is a thin pointer (carrying only the pointee's address), a fat pointer (carrying the address plus dispatch metadata), or a handle (a pointer to a pointer, supporting relocation by the allocator) is an implementation choice, made per type by the compiler, and not user-visible. The implementation latitude permits the compiler to choose, on a per-`T` basis, the representation that best serves `T`'s actual needs: a thin pointer for a small fixed-size buffer-backed `T`; a fat pointer for an object-typed `T`, supporting class dispatch; a handle for any `T` allocated in a relocation-supporting allocator.
 
User-visible operations on `^T` are uniform across these choices: dereference, address-of, indexing into a buffer-typed pointee via `p[i]`, member access for object pointees, dispatch via `::`. The user reasons about pointers in the abstract; the implementation chooses the concrete shape.
 
**No C-style pointer arithmetic.** Stepping through buffer contents requires `p[i]`. Incrementing a pointer to advance through an array is not supported. Indexing produces a first-class failure on out-of-range access; pointer arithmetic would not. The restriction is also necessary for the handle implementation to be valid — a handle's underlying address may move, and an arithmetic offset against it would alias the wrong target after relocation.
 
A `^Object` parameter is conceptually a double-indirection. Object-typed values are themselves indirections (objects are stack/heap-allocated with potentially non-contiguous fields, accessed via fat pointers); a `^Object` therefore points to a *slot* containing an object reference, and a writeable `^Object` parameter allows the callee to point that slot at a different object on success. The double-indirection structure is what lets writeable-`^Object` parameters serve as object-yielding result slots in the no-return-values world.
 
Reading from `^T` and writing to `^T` do not, in themselves, violate the no-non-local-state principle: the pointer is itself a parameter (or transitively reachable from one through the provision chain), and operations through the pointer touch storage that arrived by explicit provision. The transitive READ contract (§6.5) refines this rule along access paths rooted at READ parameters: writes through a `^T` reached by such a path are forbidden, while writes through a `^T` reached by a REFERENCE-rooted or PRODUCE-rooted path are permitted normally.
 
### 5.11 Objects
 
An **object** is a stack-or-heap-allocated, identity-bearing aggregate. Object fields can be of any type — fixed-size buffer-backed, runtime-length buffer-backed (`[]` and `[]T`), or non-buffer — including pointers, command-typed values, fexpr-typed values (subject to fexpr-specific restrictions; §8), other objects, and variants. The declaration form is:
 
    .object Logger : String name, ^File output, Severity threshold
    .object Cache  : [4096] storage, Int32 used, ^Cache next
 
Object-typed values are reference-semantics in the sense that they are normally manipulated through fat pointers to the object's storage rather than as bytes. The object's fields may be discontiguously laid out — the language does not commit to a particular field-layout strategy for objects, since the absence of a byte-aggregate constraint frees the implementation from offset-stability requirements that records carry.
 
The record/object split (§5.4) is the core split of the buffer-backed/non-buffer division. Records *cannot* contain pointers, command-typed values, fexpr-typed values, objects, or variants; objects *can* contain anything. Records have no identity beyond byte-content; objects have identity that survives byte-content equivalence. The two are not a graded distinction — they are categorically separated, and the choice between them is the choice between byte-aggregate semantics and identity-bearing-aggregate semantics.
 
Objects are nominally typed: two `.object` declarations with identical field structure produce two distinct types.
 
**Field access.** An object's fields are reached through the scope operator `::` (§9.6). The named form `object :: fieldName` selects the field by name; the positional form `object :: N` selects the Nth field by 1-based **declaration order**. Because object layout is implementation-determined (the language does not commit to a particular field-layout strategy for objects, per the discussion above), positional access is by declared order rather than by byte offset; `obj :: 1` is always the first field as the type was declared, irrespective of how the implementation chose to lay out the storage. Out-of-range positional indices and unknown field names are static errors. (Objects have no `.inline` modifier, per the grammar in Appendix B, so the inline-promotion rule from records does not apply.)
 
**Object lifetime ceiling.** The frame in which an object's storage is introduced is the object's lifetime ceiling: no mechanism in the language allows an object to outlive the frame that introduced it, except via transitive containment in another object whose ceiling is already higher. The type-side rule recorded here is that an object's *type* does not entail its lifetime — object types are first-class types — but every object *value* is bound to an introducing frame. The frame-ownership lens (§1.5) makes the rule concrete: an object lives in a frame's storage; a `^Object` parameter passed downward gives a callee access to it, but the callee does not become the owner. A writeable `^Object` parameter lets the callee swap which object the caller-owned slot points at, but the new object is allocated into the *caller's* frame on successful copy-restore. The lifetime ceiling is whichever frame ends up holding the slot at the binding moment.
 
**Frame-exit hooks via `@` and `@!` (§3.13) are tied to the frame slot, not to the object's identity transitively.** An `@` or `@!` block registered against a frame's body runs when *that frame's slot* retires, regardless of the type of the value the block references. An object embedded as a field of another aggregate — a field of another object, or transported via an unusual containment — does not participate in any implicit per-type cleanup mechanism, because the language has no such mechanism: `@` and `@!` are block markers only (§3.13), and the user writes them at the registration site they want. The discipline is **frame-lifetime-tied, not object-lifetime-tied**, which is more restrictive than C++'s destructor mechanism and is intentionally so: the language's at-stack mechanism is the single carve-out from the no-hidden-control-flow principle, and the explicit-registration rule keeps the carve-out narrow.
 
Object types may have class instances declared for them (§9). The scope operator `::` works on object-typed values and on `^Object` values via the object's fat-pointer dispatch metadata. An object's class membership is a property of the object's type, not of any particular object value, and is determined at the type-declaration site or at instance-declaration sites.
 
### 5.12 Variants
 
A **variant** is a non-buffer tagged sum. Variant candidates may be of any type — fixed-size buffer-backed, runtime-length buffer-backed (`[]` and `[]T`), or non-buffer — including pointers, objects, command-typed values, other variants, records, and domains. The declaration form is:
 
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
 
A **command-typed value** is a first-class value that wraps a command-shaped operation. The language admits four constructional forms that produce command-typed values — command reference, command literal, lambda, and fexpr — enumerated in §3.16 and detailed in §8. The type system describes command-typed values uniformly through the same family of type expressions.
 
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
 
The interaction of fexpr-typed parameters with class-method dispatch — including the `FexprFailure` standard message, the fexpr-relevance taint axis parallel to the READ contract, and the per-instance defaults-incompatibility — is in §9.20. Variants with fexpr candidates are admissible only under specific containment conditions; the rule is in §8.5's Restriction C and §8 generally.
 
The fexpr-typed family rounds out the type forms of Basis. The type forms admitted by the language — buffer primitives, domains, records, unions, aliases, enums, pointers, objects, variants, command-typed values, and fexpr-typed values — together cover every value the language recognizes. Each form has a clear position in the buffer-backed/non-buffer split (§5.1), a clear construction surface (§7), and a clear interaction with parameter modes (§6) and class dispatch (§9). The compositions among them are governed by the containment rule (§5.1) and the subsumption rules (§5.5, §5.7, §4.2, §5.15); every other type-crossing is explicit.

---

## 6. Parameters and Mode Markers

This section specifies the parameter-mode discipline of Basis. Every parameter and every receiver carries one of three modes — READ, PRODUCE, or REFERENCE — that together determine the contract between caller and callee at that position. The mode markers are part of the identifier itself in named contexts and migrate to type-suffix position in nameless contexts. Two static analyses gate the discipline at the type-system level: the initialization analysis (whole-slot tracking, §6.14) verifies that productive parameters are written exactly once on every successful exit; the taint analyses (the transitive READ contract, §6.5; fexpr-relevance, §6.15) verify that read-only contracts and ceiling bounds are preserved across access paths.

The §3 surface for declaring parameters and receivers introduced the three modes informally; this section gives the full discipline. The full transfer-function tables for the static analyses are in Appendix E. The receiver-mode-by-signature-shape table introduced in §3.10 is restated here in the context of the receiver rules R1 (call-site initialization) and R2 (callee-body obligation) that govern receiver mode mechanics uniformly across signature shapes.

### 6.1 The Three Modes

A parameter or receiver in Basis carries one of three **modes** — **READ**, **PRODUCE**, or **REFERENCE** — that determine the contract between caller and callee at that position. The modes are roles, not directions: each describes what the callee is permitted or obligated to do; the caller's side of the contract is derivable from the callee's.

**READ** — the bare-name form, no marker. The callee may read through any access path reached from the parameter; it may not write through any such path. The transitive read-only contract (§6.5) makes this commitment trustworthy: the language statically prevents the value from being smuggled into a writeable position downstream. The caller's slot is bit-identical to its pre-call state after the call regardless of outcome.

**PRODUCE** — marked with `'` on the name (e.g., `'result`). The callee is statically obligated to write the parameter's slot exactly once on every successful return path — the **write-once-on-success** rule (§6.13). The caller may pass either an initialized or an uninitialized slot. On success, the produced value is committed to the caller's slot via copy-restore (§6.4); on failure, the caller's slot is bit-identical to its pre-call state.

**REFERENCE** — marked with `&` on the name (e.g., `&counter`). The callee may read the parameter, may write the parameter, may do neither — there is no obligation in either direction. The caller must pass an already-initialized slot, since the callee is permitted to read and reading uninitialized is forbidden by the whole-slot tracking discipline (§6.14). Copy-restore semantics apply on the writeability axis: on success, any write the callee performs is committed back to the caller's slot; on failure, no write-back occurs.

The three modes apply to receivers identically. A method's receiver-mode is part of its signature; the discipline that governs receivers uniformly across signature shapes is in §6.7 (the R1 and R2 rules) and §6.8 (the receiver-mode-by-signature-shape table). Receivers and parameters are dispatched the same way at the call boundary — call-by-value for READ, copy-restore for PRODUCE and REFERENCE — with R1 imposing a uniform call-site initialization requirement on receivers that does not apply to non-receiver parameters.

### 6.2 Marker Placement

The mode markers `'` and `&` are **part of the identifier itself**, not separate marker tokens. The lexer recognizes `'name`, `&name`, and `name` as three identifier shapes of the same name. Every read and every write inside the command body uses the marker that matches the parameter's mode — a body that has a productive parameter `'result` writes `'result <- value`, never `result <- value`. The marker is visible at every use site, not only at the declaration. The same-scope rule (§6.3) prevents the three shapes from coexisting in the same scope.

The placement convention varies by syntactic context, depending on whether names are available to carry the marker.

**Named contexts** are positions where parameters or bindings carry names — parameter declarations, receiver declarations, lambda invoke-method parameters, capture-list entries (§6.10), local introductions. In named contexts, the marker travels with the name in identifier-shape:

    Int 'result            ; PRODUCE parameter
    Logger &counter        ; REFERENCE parameter
    Showable s             ; READ parameter

The full type-bearing form is `Type 'name`, `Type &name`, or `Type name`, with the type preceding the (mode-marked) name. The form `name : Type` is not Basis syntax in any binding position.

**Nameless contexts** are positions where parameter types are listed without names — command-type expressions (§5.14, §5.15) most prominently, and any other type-position where a parameter type appears unaccompanied by a binding name. In nameless contexts, the marker attaches to the type as a suffix, leaving the type-prefix position free for the pointer marker `^`:

    :<Int>                 ; READ parameter (no marker)
    :<Int'>                ; PRODUCE parameter (suffix `'`)
    :<Int&>                ; REFERENCE parameter (suffix `&`)
    :<^Int>                ; READ parameter of pointer-to-Int type
    :<^Int'>               ; PRODUCE parameter of pointer-to-Int type
    :<^Int&>               ; REFERENCE parameter of pointer-to-Int type

A pointer-to-`Int32` as a reference parameter is `^Int32&`, with `^` as prefix and `&` as suffix on opposite sides of the type — visually distinct, no token blob. The two markers serve unrelated roles: `^` is a type former, indicating the parameter's type is a pointer; `&` is a mode marker on the parameter position the type inhabits. Their visual separation reflects that role separation.

The two placements agree on what the markers mean — `'` is PRODUCE and `&` is REFERENCE in both — and differ only on placement to suit the surrounding syntax. The named-context placement attaches the marker to the binding name because there is one; the nameless-context placement attaches the marker to the type because there is no name to attach it to.

### 6.3 The Same-Scope Rule

Within any scope, **two-or-more identifiers that differ only by mode-character marking must not coexist.** The rule is symmetric across all three pairs: `x` and `'x` are mutually exclusive; `x` and `&x` are mutually exclusive; `'x` and `&x` are mutually exclusive. The rule applies both to identifiers introduced into a scope by binding (parameters, receivers, captures, local slots) and to command-parameter-list declarations: a command may not be declared with two parameters whose names collide modulo marker.

The motivation is reader confidence. Two textually-similar names referring to genuinely different slots in the same context produce a class of subtle reader-bugs — did this read from the productive slot or the reference slot? Forbidding the coexistence costs a rename; the benefit is that scope-local reasoning never has to disambiguate near-identical names.

A separate concern is passing a single slot at multiple modes in one call. A caller passing a value `x` into a callee may legitimately pass `x` at one parameter position and `'x` (or `&x`) at another in the same call. The same-scope rule does not forbid this — the call-site argument list is not a scope, and the modes refer to roles at the call boundary, not to identifiers in the caller's scope. The aliasing concern this raises — the same caller-slot bound under two mode contracts simultaneously — is an aliasing question, separately addressed by the rule that disallows binding a single slot at PRODUCE and REFERENCE positions in the same call (since the resulting aliasing would defeat copy-restore failure-atomicity).

### 6.4 Call-Boundary Mechanics

The call-boundary mechanics realize the per-mode contracts at the operational level. The type-system rules of §§6.1–6.3 and the static analyses of §§6.5, 6.13, 6.14 commit to behavior; the call-boundary mechanism is what makes that behavior real.

**READ parameters** are passed by value at the observable level. The callee receives a copy of the caller's slot. The caller's slot is bit-identical to its pre-call state after the call, regardless of whether the call succeeds or fails. Implementation latitude: the implementation may pass READ parameters by reference under the hood, since the transitive READ contract (§6.5) precludes the writes that would make by-reference observable; the language commits to by-value at the *observable* level only.

**PRODUCE and REFERENCE parameters** are passed by **call-by-copy-restore**. The callee receives a copy of the caller's slot value, operates on the copy, and on *successful* return the (possibly-modified) copy is written back to the caller's slot. On *failure*, no write-back occurs; the caller's slot is bit-identical to its pre-call state.

The copy-restore mechanism is the operational realization of the language's "mutation either succeeds fully or fails fully" principle. Failure-atomicity falls out of the calling convention with no transactional machinery and no rollback code in user programs. The pre-call state of the caller's slot is preserved by the structure of the convention rather than by explicit restoration logic.

The pointer case composes cleanly. For a parameter of pointer type `^T`, the *pointer value* itself is what's copied (READ) or copy-restored (PRODUCE / REFERENCE), not the pointee. A writeable `^T` (i.e., `^T'` or `^T&` per the nameless-context rule of §6.2, or `^T 'name` / `^T &name` per the named-context rule) lets the callee swap which `T` the caller's slot points at; a non-writeable `^T` does not. The pointee's storage is reached *through* the pointer; reads and writes through the pointer touch storage that, by the no-non-local-state principle's provision-chain rule (§1.4), was reached by explicit provision.

For PRODUCE parameters specifically, the caller's slot may be uninitialized at the call boundary, and the copy-restore mechanism handles the initialization gracefully: the callee's local copy is a fresh slot of the parameter type that the callee is statically obligated to fill via the productive write, and on success the produced value is committed to the caller's slot. On failure the caller's slot remains in its pre-call state, which for a fresh `#`-introduction was uninitialized — the initialization analysis (§6.14) tracks this as the slot remaining uninitialized after the failed call. The caller's analysis records the call's two outcomes as separate edges on the failure-state lattice (§4.13), with the initialization analysis joining the per-edge results.

### 6.5 The Transitive READ Contract

A READ parameter introduces a contract: the callee may not write through any access path reached from the parameter. **Reachability is transitive** through pointer dereference, field access, indexing, and any other operation that extends an access path rooted at the parameter.

The contract is callee-side and verified at type-checking. A callee whose body would write through any reachable access path fails to type-check, regardless of whether that write would be "observable" at any particular call site. The discipline is structural; the language does not check for aliasing between the READ parameter and other writeable positions and then conditionally permit the write. The forbidding is unconditional: from a READ parameter's perspective, the storage it reaches is read-only.

The contract operates on **access paths**, not on storage locations (§1.5 standing lens). The same storage may be reached at runtime through multiple access paths simultaneously, one of which is READ-rooted and another of which is REFERENCE-rooted; writes through the REFERENCE-rooted path are permitted while writes through the READ-rooted path are forbidden. The language does not detect aliasing; it tracks paths.

The mechanism is enforced via a flow-sensitive **taint** property on slots and slot-views, propagated jointly with the failure-state lattice (§4.13) and the initialization analysis (§6.14) over the same CFG walk. The transfer-function rules:

- A READ parameter introduces a tainted slot at the callee's frame entry.
- Operations that yield slot-views reached through a tainted slot produce tainted slot-views — pointer dereference, field access, indexing, and any other access-path-extending operation propagate the taint.
- A `<-` assignment from a tainted source taints the destination slot.
- An attempt to write through a tainted slot-view, including via `^T` dereference, is a static error.

The classification of which operations propagate taint and which produce a fresh untainted value — for example, reading a record's `Int32` field from a tainted record-slot, where the resulting `Int32` value's bytes are byte-disjoint from any reference into the record's storage — follows a single rule: if the operation yields a value whose representation includes any reference into storage reached from the tainted source, the result is tainted; otherwise the result is untainted. Cases that need precise treatment include objects with non-buffer-backed fields (whose fields may be pointers, variants, command-typed values, or other objects, so reading such a field through a tainted object yields a tainted slot-view), variants over non-buffer candidates (extracting a candidate yields a tainted slot-view), and pointer dereference where the pointee's storage is reached through the tainted source. Domain values extracted from a tainted record's bytes are the contrasting case — the bytes are read and reinterpreted as a domain value, with the result byte-disjoint and untainted. The buffer-backed containment rule (§1.5, §5.1) eliminates the cases that would otherwise be problematic: records, unions, and typed buffers cannot contain non-buffer-backed components, so reading a buffer-backed field of a tainted buffer-backed compound never yields a slot-view onto storage outside the byte-aggregate itself. The full transfer-function table is in Appendix E.

A forbidden pointer-write pattern follows directly from the rule: a write through a `^T` reached from a READ parameter's binding is a static error. The `^T` is itself read-only-through-this-access for the duration of the call.

The contract composes with the capture mechanisms (§§6.10, 8.4, 8.5). A lambda that captures a free name binding a tainted slot, or a value derived from one, carries the taint into its body; the body sees the captured value at READ semantics. Fexprs propagate similarly. For partial application via the command-reference form `{...}` (§3.16), if the captured value at the partial-application site is itself READ-tainted, the bake-in carries the taint into the resulting command-typed value.

### 6.6 The Frame-Ownership Reading

The transitive READ contract is callee-side: it constrains what the callee's body may do during its own execution, in its own frame. The contract does not propagate across the call boundary into the caller's analysis.

**Each frame's static analysis is local.** Initialization tracking, failure-mode tracking, and access-path taint are properties of a single command body's analysis on its own parameters and locals. There is no cross-frame propagation. A PRODUCE or REFERENCE output of a downstream call appears in the caller's frame as a **freshly-bound local** — a local whose access path is rooted at the local itself, not at any of the caller's parameters. Taint on the caller's local is determined by the local's own access paths within the caller's body, independent of whatever taint analysis took place inside the callee.

A concrete pattern illustrates the locality. A callee `getField: ^MyObj o, ^FieldT 'fp` is permitted to write a pointer-into-`o`'s-field into `'fp`. The callee's body has a READ access path on `o` and a writeable access path on `'fp`; the productive write composes a value of type `^FieldT` from the READ-rooted access path. The write is permitted because the callee's contract on `o` is satisfied — there is no write through `o` — and the result is a fresh value of pointer type produced into `'fp`. After the call, the caller's resulting `^FieldT` slot is a fresh local in the caller's frame; the caller's analysis tracks that local on its own access paths, with no inherited taint from the call.

The frame-ownership lens is sufficient because each frame's signature carries the contracts that frame is responsible for. The callee's signature tells the callee's body what its parameters are, what their modes are, and what contracts its body must satisfy locally. The caller's signature tells the caller's body what its own parameters are. Each frame is its own analysis context. The discipline migrates to the frame that has the right context; the language does not propagate analysis state across the call boundary.

The aliasing question — whether a fresh local in the caller's frame might at runtime alias storage the caller passed at READ — is intentionally not addressed by the static taint analyses. The language tracks access paths, not storage locations (§1.5 standing lens). The READ contract's static soundness is the callee-side commitment that the callee will not perform writes through its own READ-rooted paths; the caller's analysis is its own concern, with the caller's signature determining what the caller's body may do. The two-sided locality is what keeps the analyses tractable.

### 6.7 The R1 and R2 Receiver Rules

Receivers participate in the same three-mode discipline as parameters, but the receiver-mode rules carry an additional structural constraint that parameters do not. The receiver in a method dispatch must exist as a real value at runtime, regardless of the receiver's declared mode, because dispatch resolves a method-bearing value from the receiver's type and invokes it on the receiver's value. Dispatching on an uninitialized slot is meaningless. Two rules govern receiver-mode mechanics uniformly across signature shapes:

**R1 — call-site initialization.** At any method call site, every receiver must be initialized at the call boundary, regardless of the receiver's declared mode. This is uniform across PRODUCE, REFERENCE, and READ receivers. The R1 rule is the structural commitment that makes dispatch well-defined: the receiver's runtime type identifies the instance dictionary (§9), and that identification requires a real value. PRODUCE receivers are initialized at the call site even though the callee will write them on success — initialization-then-overwrite is the rule, not initialization-on-success.

**R2 — callee-body obligation.** The body of a method, with respect to a receiver of mode `M`, has the same callee-side obligations as a parameter of mode `M` would have. PRODUCE receivers must be written exactly once on every successful return path. REFERENCE receivers carry no write obligation; the body may read, write, or do neither. READ receivers may read through any access path reached from the receiver but may not write through any such path (the transitive READ contract of §6.5). The R2 rule preserves the per-mode discipline at the callee body, identical to non-receiver parameters.

R1 lifts the caller-side obligation uniformly across all receiver modes — always initialized at the call site. R2 keeps the callee-side variation that the writeability marker is for. The two rules together say: dispatch always operates on a real receiver, and the mode marker tells the callee what it commits to doing with that receiver.

R1 and R2 apply to multi-receiver methods (§3.11) per receiver independently. Different receivers in a tuple may carry different modes; the per-receiver R1 and R2 rules apply to each per its declared mode. A multi-receiver method `(Logger logger, Counter &c) :: report: ...` has logger at READ (R1 requires logger initialized at the call site; R2 forbids the body from writing through any access path reached from logger) and counter at REFERENCE (R1 requires counter initialized; R2 imposes no obligation on whether the body writes counter).

### 6.8 Receiver Modes by Signature Shape

Each signature shape restricts the set of valid receiver modes to those that are semantically meaningful for that shape. The table summarizes the per-shape restrictions:

| Signature shape | Valid receiver modes | Forbidden modes |
| --- | --- | --- |
| Constructor | PRODUCE only | READ, REFERENCE |
| Method (single- or multi-receiver) | PRODUCE, REFERENCE, READ | none |

The reasoning behind the per-shape restrictions:

**Constructor receivers must be PRODUCE.** A constructor's job is to fill the receiver slot. A REFERENCE constructor receiver would mean "construct drawing on the receiver's existing state" — which is in-place modification, not construction; the method-with-REFERENCE-receiver case (§3.10) covers that idiom. A READ constructor receiver would mean "construct a thing the caller cannot observe" — forcibly producing an inaccessible object, which has no purpose. The PRODUCE-only rule preserves the constructor's defining feature: at a successful return, the receiver slot holds the constructed value.

**Method receivers admit all three modes.** The mode is declared per method; the choice between READ, REFERENCE, and PRODUCE is the choice between externalized-effect, in-place-mutation, and re-initialize semantics (§6.9). Each mode is structurally meaningful for methods, and the language admits them all.

The block markers `@` and `@!` (§3.13) are not signature shapes — they introduce bodies, not methods, and do not appear in this table.

The table is restated from §3.10 here in the context of the R1 and R2 rules of §6.7, which apply uniformly across the table. R1 holds for every row: every receiver, regardless of mode or shape, is initialized at the call site. R2 holds per-mode: a PRODUCE receiver carries the write-once obligation, a REFERENCE receiver carries no obligation, a READ receiver carries the transitive READ contract.

### 6.9 Idiomatic Uses by Mode

Each parameter or receiver mode has a distinctive idiomatic use; the choice of mode at a declaration site is a design decision about what the position commits to.

**READ — externalized effect.** The callee operates *through* the receiver or parameter without modifying its slot. Logging is the canonical case for READ receivers: `logger :: log: message` writes a log entry; the logger's slot is unchanged from the caller's vantage, while the world (the log file, the stream) is changed. The receiver mediates an effect external to itself. The pattern preserves the no-non-local-state principle (§1.4): the receiver is a parameter, so accessing its connection-state is local; the external effect happens through some lower-level command that itself takes the connection-state as a parameter at REFERENCE or PRODUCE mode. READ parameters serve the same role at non-receiver positions — a value the callee inspects without mutating, with the transitive contract guaranteeing the inspection cannot leak into mutation downstream.

**REFERENCE — in-place modification.** The callee may read the slot's current state and may modify it; the slot's identity is preserved across the call. State transitions on objects, in-place updates on counters or buffers, the "modify if needed" idiom — all use REFERENCE receivers or REFERENCE parameters. The mode commits to nothing: the body may read, write, or do neither. The lack of obligation is what makes REFERENCE the natural choice when the callee's behavior depends on data: the callee inspects state, decides whether to update, and does so or not as its logic dictates.

**PRODUCE — re-initialize the slot.** The callee commits to writing the slot on every successful return path. For non-receiver parameters, this is the natural shape of "output through a writeable parameter slot": the caller introduces a fresh slot via `#`, the callee fills it, and the caller binds the resulting value. For receivers, PRODUCE describes a "factory method" or "complete reset" operation that happens to dispatch on the receiver's existing type: the receiver is initialized at the call site (per R1), the callee dispatches based on its current type, and the callee overwrites it on success. Unusual but coherent.

The three modes form a complete discipline: every parameter and receiver in the language belongs to exactly one of them, and the choice is structural — the language has no "default" mode, no mode that emerges from absence of declaration. The READ mode appears as the bare-name form (no marker); it is the no-marker case in the syntax, but it is no less a deliberate declaration than the other two.

### 6.10 Capture-List Mode Constraints

Lambda forms (§8.4) accept an explicit capture list, written after the parameter list separated by `/`: `:<args / captures>{body}`. Each entry in the capture list carries a mode marker per the named-context rule of §6.2 — `'name`, `&name`, or bare `name` for PRODUCE, REFERENCE, and READ respectively.

**Only READ and REFERENCE modes are admitted in lambda capture lists; PRODUCE is forbidden.** Capturing a productive obligation into a lambda is not meaningful, for two related reasons. First, lambda values may outlive `D` (the defining frame; see §8.5 for the formal D/I/F three-frame model) through ceiling-flattening of reference captures (§8.4), at which point the productive slot may no longer exist. Second, lambdas have multi-invocation semantics — deferring a productive write to "whenever the lambda runs" would either have multiple lambda invocations attempt to write the same slot multiple times (violating the write-once rule) or have one invocation discharge the obligation while others receive a slot already-written (defeating the meaning of the capture). The productive obligation is to write at a specific call boundary in the defining frame, and the lambda capture list cannot carry that obligation across the construction boundary.

Fexprs have no such restriction. A fexpr body accesses defining-frame state by free-name resolution at invocation time, with the mode of each access inherited from the binding's mode in the defining frame (§8.5). A free name in a fexpr body that binds a productive parameter `'name` in the defining frame accesses the slot at PRODUCE mode; writes to it through the fexpr count toward the defining frame's write-once analysis exactly as if the fexpr's body were inlined at each invocation site. Two structural properties of fexprs make this sound where the analogous lambda case is not. The fexpr's lifetime is bounded by its defining frame (the ceiling discipline of §5.15 / §6.15), so the productive slot is guaranteed to still exist at every fexpr invocation. And the fexpr's invocations are tracked as inlining points in the defining frame's CFG analysis, so multi-invocation patterns that would violate write-once are caught by the same analysis that catches them in straight-line code in the defining frame.

The mode-and-taint discipline composes with the capture mechanisms uniformly. A captured READ or REFERENCE value — whether captured explicitly via a lambda's slash list or implicitly via a fexpr's free-name resolution — carries the defining-frame slot's taint into the closure body; the closure's invocations operate on the captured value at the captured-mode contract, with the taint discipline preserved.

### 6.11 Implicit Context Parameters Across Modes

Implicit context parameters (§3.6) — parameters listed after the `/` separator in a command's signature, filled at the call site by uniqueness-of-type from the caller's lexical scope — admit all three modes. An implicit parameter declared `Logger logger` is a READ implicit; one declared `Logger &counter` is a REFERENCE implicit; one declared `Int 'result` is a PRODUCE implicit. The resolution mechanism treats the modes uniformly: at the call site, the typechecker locates a value in the caller's lexical scope whose type matches the implicit parameter's declared type, and supplies that identifier as the argument with the implicit's declared mode contract.

The mode contracts at implicit parameters follow the same R1-and-R2-style discipline: at the call site, REFERENCE and READ implicits require an initialized matching slot in the caller's scope; PRODUCE implicits require either an initialized or uninitialized matching slot (the productive write makes either acceptable on the callee side). The caller's analysis records the implicit's call-site role exactly as it would for an explicit argument at the same mode.

A constraint specific to implicit context parameters: **implicit parameters cannot carry default declarations from class instances.** Where a class declares a method whose signature includes implicit parameters, the resolution at the dispatch site uses the dispatching frame's lexical scope, not the instance's defining-frame scope; the instance has no role in supplying values for the implicit slots. The motivation is to keep implicit resolution lexically transparent: the values come from where the call is made, not from where the dispatched implementation was defined. The constraint composes with the class-system mechanics in §9 without further interaction.

### 6.12 Parameter-Mode Invariance Under Mark Subsumption

The failure-mode marks (§4.2) form a subsumption order: `:` ⊑ `?` and `!` ⊑ `?`, with `:` and `!` mutually incomparable. A `:`-marked or `!`-marked command-typed value may stand wherever a `?`-marked value is expected. **Parameter modes and parameter types are invariant under this subsumption.** A `:<Int32'>` value is not interchangeable with `:<Int32>` or `:<Int32&>`; a `?<String'>` value is not interchangeable with `?<String>` or `?<String&>`. The subsumption relation is solely on the failure-mark axis.

The invariance is essential for soundness. The per-mode static rules at the call site break if the mode is permitted to vary. A productive parameter discharges a write-once obligation; substituting a READ parameter would lose the obligation entirely; substituting a REFERENCE parameter would lose the write-once-on-success commitment in favor of no commitment. A reference parameter requires its slot initialized at the call site; substituting a productive parameter would change the precondition (productive admits uninitialized); substituting a READ parameter would lose the writeability axis entirely. None of these substitutions preserve the per-mode contract, and none of them preserve the static analyses' soundness.

The type-system rule recorded here is that `Type X` and `Type Y` for distinct mode markings X and Y are **distinct types** that do not stand in any subsumption relation, on either side of the failure-mark axis. The invariance applies symmetrically at PRODUCE, REFERENCE, and READ; no pair of modes is exchangeable. The mark axis and the mode axis are orthogonal, and subsumption operates on the mark axis alone.

The invariance composes with the buffer-backed-hierarchy subsumption rules (§5.5) without conflict: a `:<Inches>` value may be acceptable at a `:<Int32>`-typed slot under the buffer-backed parent-chain rule, since `Inches` subsumes to `Int32` in the underlying parameter type. But the mode markers must match. A `:<Inches'>` value is not acceptable at a `:<Int32>`-typed slot, because the productive marker is missing on the latter; the parent-chain subsumption applies to the parameter type, not to the parameter type and mode together.

### 6.13 Productive Write-Once

A productive parameter (`'name`) discharges the **write-once-on-success** rule: the callee's body must write the slot exactly once on every path that reaches a successful exit. Paths that reach a failure exit are exempt from the obligation — the failure-atomicity principle commits that productive slots are never partially written when a command fails.

The rule is structural, not stylistic. There is no "you may write twice if you also clear in between" alternative, and there is no in-place-update form for productive parameters. The body must produce exactly one write to each productive slot per successful path. Failure to write a productive parameter on some successful path is a static error; writing it more than once on the same path is also a static error.

The compute-then-commit pattern (§3.4) is the natural shape that satisfies the rule. The body computes its inputs into local slots, then performs a single productive `<-` write near its end. The pattern composes with the atomic-compound-construction guarantee (§7.11): an aggregate literal `${...}` is a single expression that produces a complete value, and a productive write of an aggregate literal is a single write.

The discipline composes with the failure-state-lattice analysis (§4.13). The body's CFG is walked with both the failure-state lattice and the initialization analysis (§6.14) jointly; the typechecker confirms that every path to a `clear`-state exit has performed exactly one write to each productive parameter. The §4.13 well-formedness rule for body conformance — that conformance quantifies over reachable exits only, with non-terminating bodies vacuously conformant — applies here: a body with no reachable successful exit (a `!`-marked body, or a divergent body) imposes no productive-write obligation, since the universal quantification over reachable successful exits has an empty domain.

The interaction with REFERENCE parameters is stable. REFERENCE parameters may be written zero, one, or many times across any path; the write-once rule applies only to PRODUCE parameters. A method that writes a REFERENCE parameter twice on the same path is well-formed; the same method written with a PRODUCE parameter would be ill-formed.

The interaction with the `-> name` result designator (§3.7) is direct: a PRODUCE-typed `-> 'name` produces a post-write-back value in expression position; the post-write-back value is observable only when the call succeeded, since the productive write occurred on success per the write-once rule.

### 6.14 Whole-Slot Tracking

The initialization analysis tracks each slot as a unit, not as a graph of fields. A buffer-backed compound — a record, a union, a typed buffer — is initialized when its bytes are written as a whole; partial-field initialization is not a state the analysis tracks. The discipline applies uniformly across all type forms: buffer-backed types (records, unions, typed buffers, domains) and non-buffer types (pointers, command-typed values, fexpr-typed values, objects, variants) are each tracked as one slot per declaration, not as graphs of constituents.

The whole-slot tracking is what makes "exactly one write" a coherent statement for compounds (§6.13). A productive record parameter is written by a single aggregate-literal write that produces all its fields in one operation; the analysis records the slot as initialized after the write, with no intermediate state where some fields are written and others are not. The atomic compound construction guarantee (§7.11) is the construction-side facet of the same rule: an aggregate literal constructs the whole compound at once, with no observable per-field intermediate state.

The discipline eliminates whole categories of static-analysis questions. A record-with-pointers would punch a hole in per-slot tracking — the pointer copies, the pointee doesn't — but the buffer-backed containment rule (§1.5, §5.1) forbids buffer-backed types from containing pointers, and the rule extends transitively to all the buffer-backed compounds. The cases that presuppose a buffer-backed structure containing non-buffer-backed components are not Basis cases; the analysis can treat each buffer-backed slot as a uniform byte-aggregate without per-field decomposition.

For variants (§5.13), the absent state is a structural property of the type, not a partially-initialized state. A bare introduction `# SomeVariant x` produces a variant slot in the absent state; the slot is *initialized* in the analysis's sense — the absent state is a distinguishable, well-formed state of the variant — but holds no candidate. Variant operations that move the slot from absent to a non-absent candidate, or back to absent (via `v -< _`, §7.14), are whole-slot writes per the same rule as record-aggregate writes.

For non-buffer types other than variants — pointers, command-typed values, fexpr-typed values, objects — the slot has no zero-default state, and bare introduction is not admitted. A `# ^Int32 x` declaration is a static error; an initializing form (`#x <- ...` or `Type: #x, ...` in argument position) is required. The whole-slot tracking treats these slots as initialized only after a complete write; partial states are not modeled.

The implementation of the analysis is a forward-flow walk over the body's CFG, joining initialization states at convergent nodes; the join of `init` and `init` is `init`, of `uninit` and `uninit` is `uninit`, and `init` joined with `uninit` is a contradiction at the join point that gates a use of the slot at that point. The full transfer-function table is in Appendix E. The analysis runs in parallel with the failure-state lattice (§4.13) and the taint analyses (§§6.5, 6.15) over the same CFG with the same join points but with independent transfer functions per analysis.

### 6.15 Fexpr-Relevance Taint

A second taint axis operates parallel to the READ-taint discipline (§6.5): the **fexpr-relevance** taint. The two axes are orthogonal — a slot may be untainted, READ-tainted, fexpr-relevance-tainted, or both — and propagate by independent transfer functions on the same CFG walk.

The fexpr-relevance taint exists to enforce the **defining-frame ceiling** property of fexpr-typed values (§5.15). A fexpr cannot escape upward, cannot be stored in a long-lived position, cannot be captured by a value that outlives its defining frame. The taint axis is what tracks "this slot or value is reached from a fexpr-typed slot whose ceiling is the fexpr's defining frame `D`," propagating that information through the per-frame analysis so that any assignment that would violate the ceiling is a static error.

The sources of fexpr-relevance taint are fexpr-typed values — values of type `:<*>`, `?<*>`, or `!<*>` — and any slot that holds one. A parameter of fexpr type and a binding from a fexpr expression both produce fexpr-relevance-tainted slots. The taint propagates per the same rule pattern as READ-taint: operations that yield slot-views reached through a fexpr-relevance-tainted slot produce fexpr-relevance-tainted slot-views; assignments from a tainted source taint the destination; capture by a closure propagates the taint into the closure body, subject to the structural restrictions enumerated below.

The enforcement points where fexpr-relevance taint blocks an operation are the structural restrictions enumerated in §5.15 (and detailed in §8.13 as restrictions A–G):

- Object fields holding fexpr-typed values are restricted — an object's lifetime ceiling is the introducing frame, but a fexpr's ceiling is its defining frame `D`, which may be deeper; assignment is permitted only when the object's ceiling is at-or-below the fexpr's `D`.
- Productive parameters of fexpr type are forbidden — a fexpr cannot be the productive output of a call, since the produced value would escape the callee's frame upward.
- Pointers to fexpr-typed slots are forbidden — a pointer would let the fexpr escape its ceiling via indirection.
- Bare-identifier copy of a fexpr value into a non-fexpr-typed slot is forbidden — the copy would erase the fexpr type information and bypass the structural enforcement.
- Capture of fexpr-typed values by lambdas is forbidden — a lambda's capture-list ceiling is the lambda's defining frame, not the fexpr's, and the two need not coincide.

The propagation is **per-frame and ceiling-bound to the fexpr's `D`**. The taint flows through the local analysis until it reaches an enforcement point, where the typechecker either confirms the assignment respects the ceiling or rejects it. Fexpr-relevance taint, like READ-taint (§6.6), does not propagate across frame boundaries: a fexpr-typed parameter passed to a callee enters the callee's frame as a fresh fexpr-typed slot, with the type-level discipline enforced afresh in the callee's analysis. The structural restrictions enumerated above are absolute — they apply at every assignment and storage point, to every value carrying fexpr-relevance taint (whether the value's type is itself fexpr or whether the taint was acquired by binding a fexpr-typed argument into a command-reference per §8.12), regardless of which frame the operation occurs in — so each frame can enforce the ceiling discipline locally without needing to know specific identities of upstream frames.

The interaction with method dispatch — admitting fexpr-typed parameters in methods, the `FexprFailure` standard message, and the per-instance defaults-incompatibility — is in §9.20. The full transfer-function table for the fexpr-relevance analysis, parallel to the READ-taint table, is in Appendix E.

The two taint axes operate on access paths in the same way (§1.5 standing lens), with independent enforcement points. A value may carry both taints simultaneously — for example, a fexpr-typed value derived from a READ parameter — with each axis independently constraining the operations the value participates in. The READ-taint forbids writes through any access path reached from the value; the fexpr-relevance taint forbids assignments that would escape the fexpr's defining-frame ceiling. The two restrictions compose without conflict: a value that satisfies both constraints is admitted at any operation that respects both.
---

## 7. Construction and Initialization

This section specifies how values are constructed and how slots are initialized in Basis. Every construction in the language flows through the `<-` operator (§7.1), which writes a value into a slot and enforces the failure-atomicity discipline (§7.18). The right-hand side of `<-` accepts five distinct surface shapes (§7.3), each fitting a subset of left-hand-side types: parenthesized command call, aggregate literal `${...}`, sequence literal `$[...]`, bare identifier, and bare literal. The construction surfaces compose with the language's existing mechanisms — the failure system (§4), the parameter-mode discipline (§6), the type system (§5) — without introducing new control-flow primitives.

The `.implicit` mechanism (§7.8) admits bare literals at typed slots by registering type-conversion constructors. The `-<` dynamic-narrowing operator (§7.14, §7.15) is the runtime counterpart to `<-` for narrowing across type hierarchies, applied uniformly across variants, class hierarchies, and unions. The `=` defaults declaration (§7.10) supplies type-level default values for buffer-backed types whose bytes do not have a meaningful zero. The atomic compound construction discipline (§7.11) is the structural shape that satisfies the productive write-once obligation (§6.13) for compound types.

### 7.1 The `<-` Operator

The `<-` operator is the runtime placement primitive. It writes a value into a slot, observing the slot's parameter-mode contract and the language's failure-atomicity discipline. The operator appears in three syntactic positions:

- **Local introduction.** `#x <- value` introduces a fresh local slot `x` and binds it to the right-hand value; `# T x <- value` is the explicitly-typed variant (§7.2). The introduced slot's type is the right-hand value's type when no explicit type is supplied.
- **Productive write.** `'r <- value` writes a productive parameter or receiver. Under the write-once-on-success rule (§6.13), this is typically the constructor body's single Phase 2 commit (§7.11).
- **Slot rewrite.** `&x <- value` rewrites the slot bound to a REFERENCE-mode parameter (where `&` is the parameter's mode marker per §6.2); `x <- value` rewrites a regular previously-initialized local (where the bare-name form applies). In both cases the slot must already be initialized at the point of the write; the write replaces its value. PRODUCE parameters do not admit rewrite — the write-once rule allows only one write per successful path.

In all three positions, the typechecker enforces failure-atomicity (§7.18): a may-fail right-hand side whose evaluation fails leaves the left-hand slot in its pre-write state. For productive and reference writes, the pre-write state is the slot's contents before the `<-` evaluation began. For local introductions, the pre-write state is the slot's pre-introduction status — uninitialized for `#x <- value` whose right-hand side fails, since the `#`-introduction-with-initializer is a single atomic operation that either completes or does not.

### 7.2 Local Introduction Syntax

Local introduction has two surface forms:

    #x <- value              ; introduce x; type inferred from value
    # T x <- value           ; introduce x of type T; T is the upper bound

The unmarked form `#x <- value` introduces a local slot whose type is the right-hand value's type. The introduced slot accepts subsequent assignments at the inferred type.

The typed form `# T x <- value` declares the slot's type explicitly. The declared type T serves as the **upper bound** for the slot: subsequent assignments to `x` must produce a value acceptable at type T per the buffer-backed subsumption rules (§5.5) or the standard type-compatibility rules. The typed form is universally available; it costs nothing where unnecessary and is required in two cases:

- **Bare-literal initializers whose literal type admits multiple `.implicit` targets.** Without an explicit type, the typechecker cannot determine which `.implicit` constructor (§7.8) to apply. `# Float32 x <- 3.14` resolves to the `Float32`-from-`Decimal` constructor; `# Float64 x <- 3.14` resolves to the `Float64`-from-`Decimal` constructor. The unmarked form `#x <- 3.14` is rejected when multiple `.implicit` constructors match the literal's source type.
- **Bare introduction without initializer.** The form `# T x` (no `<- value`) is admitted only for variant types and for buffer-backed types whose bytes have a meaningful zero or have an `=` default declared (§7.10). For variants, the introduction produces an absent-state slot (§5.13); the equivalent `# T x <- ${}` initializer form (§7.4) is also admitted. For buffer-backed types with zero-meaningful bytes, the introduction produces a zero-filled slot. For other types, bare introduction is a static error and an initializing form is required.

The upper-bound semantics of typed introduction is uniform with the language's other type-bound positions: a parameter's declared type is the upper bound for the parameter, an object field's declared type is the upper bound for the field, and so on. A subsequent assignment to a typed local is type-checked against the local's declared type, not against the value-type the local currently holds.

### 7.3 The Five RHS Shapes

The right-hand side of `<-` accepts five distinct surface shapes, each fitting a subset of left-hand-side types:

| Shape | Surface | LHS types accepted | Section |
| --- | --- | --- | --- |
| Parenthesized call | `(cmd: args)` | Any (the cmd's productive output type) | §3.14 |
| Aggregate literal | `${...}` | Records, objects, unions, variants | §7.4 |
| Sequence literal | `$[...]` | Buffer primitives `[N]` / `[]`; typed buffers `[N]T` / `[]T` | §7.5 |
| Bare identifier | `name` | Buffer-backed types, pointers, command-typed values | §7.6 |
| Bare literal | `42`, `3.14`, `"hello"`, `0x41`, etc. | Whatever a matching `.implicit` constructor accepts | §7.7 |

The typechecker dispatches semantics on the right-hand-side shape and the left-hand-side type. Each shape's well-formedness against an LHS type is decided independently of context; a shape-vs-type mismatch is reported as a single error at the `<-` site, naming the LHS type and the shape encountered.

A summary acceptance matrix:

| LHS type category | Call | `${...}` | `$[...]` | Bare ident | Bare literal |
| --- | --- | --- | --- | --- | --- |
| Buffer primitive `[N]` / `[]` | yes | — | yes | yes (copy) | via `.implicit` |
| Typed buffer `[N]T` / `[]T` | yes | — | yes | yes (copy) | via `.implicit` |
| Plain domain | yes | — | — | yes (copy, on type-compat) | via `.implicit` |
| Record | yes | yes | — | yes (copy, identical type) | via `.implicit` |
| Union | yes | yes | — | yes (copy, identical type) | via `.implicit` |
| Object | yes | yes (named) | — | no (identity-bearing; §7.6) | via `.implicit` |
| Variant | yes | yes | — | no (reference-semantics; §7.6) | via `.implicit` |
| Pointer `^T` | yes | — | — | yes (copy of pointer value) | — |
| Command-typed value | yes | — | — | yes (copy of command value) | — |
| Literal — Integer / Decimal / Hex / String | yes | — | — | yes (copy, identical kind) | yes (matching kind only) |
| Literal — Aggregate | yes | yes (Aggregate Literal only — Rule 1, §7.4) | — | yes (copy) | — |
| Literal — Sequence | yes | — | yes (Sequence Literal only — Rule 1, §7.5) | yes (copy) | — |

A dash indicates the shape is ill-formed for that LHS type; any `<-` form not listed under the LHS row is a static error.

### 7.4 Aggregate Literals and Initialization Temporaries

The surface form `${...}` serves two roles, distinguished by the kinds of values it contains:

- An **Aggregate Literal** — a passive raw-data value with literal-kind tags at each position (§7.7), held directly when assigned to an `Aggregate`-typed slot or used as a constructor source.
- An **initialization temporary** — a non-Literal construction surface used to fill the fields of a record, object, union, or variant atomically. The temporary is consumed at the placement boundary and does not persist as a runtime value.

Both roles share the same syntax — opening token `${`, closing `}`, comma-separated entries either named (`fieldName <- value`) or positional (`value, value, ...`). The typechecker classifies any particular `${...}` per content, by these rules:

1. If the construct contains only Literals, it is an **Aggregate Literal**.
2. If the construct contains both Literals and non-Literals, it is an **initialization temporary**; each Literal at a structured position is bridged to that position's expected type via `.implicit` conversion (§7.8) — the same rule that applies to bare literals at non-Literal LHS positions (§7.7).
3. If the construct contains only non-Literals, it is an **initialization temporary**, with no `.implicit` conversion needed (each element is consumed at its position's expected type directly).

A **Literal** in this rule means a bare literal value written directly in source (`3.14`, `"hello"`, `0x41`, etc.) or a nested `${...}` / `$[...]` whose contents make it a Literal under the same rules. A **non-Literal** means an identifier, a constructor call, or any expression yielding a typed value.

The opening token `${` is unambiguous (the `$` character has no other use in current Basis source); the closing `}` is the matching brace. Inside the construct, `<-` is reused at the field-name-to-value separator position.

The empty form `${}` is a single token denoting a construct with no field entries; vacuously all-Literal under Rule 1, it is an Aggregate Literal. It is well-formed in two cases: when the LHS type is a record or object whose every field is defaulted (via `=` declarations, §7.10), in which case the literal constructs the all-defaulted value; and when the LHS type is a variant, in which case the literal produces the variant's absent state. The variant case is equivalent to bare introduction `# T x` with no initializer (§7.2): both forms produce an absent-state variant slot, and a programmer may use either at any contextually-typed variant LHS position.

**Field-name nominal matching.** The typechecker matches the construct's field names to the LHS type's declared field names. The matching is nominal, not positional: `${y <- 2, x <- 1}` and `${x <- 1, y <- 2}` are equivalent for an LHS type whose declared fields are `x` then `y`. Repeated field names are rejected at parse time. Field names not declared on the LHS type generate a compiler warning and are silently dropped at runtime; the discrepancy surfaces to the user without defeating tooling that emits constructs from heterogeneous sources. Required fields must be present unless covered by a default (`=` declaration, §7.10); a defaulted field may be omitted with a suppressible warning. Variant-typed fields are always required, with `_` as the absent value (§7.16).

**Positional form under contextual clarity.** The positional surface `${value, value, ...}` is admitted when the LHS type is contextually explicit — when the typechecker can determine the LHS type at the construct's position without reference to its contents. The contextually-clear positions are:

- The right-hand side of a typed local introduction `# T x <- ${...}`.
- An argument at a typed parameter position.
- The right-hand side of a productive write `'r <- ${...}` (the receiver's declared type provides context).
- The value of an outer construct's entry whose field is typed.

A positional construct in a context where the LHS type is not contextually explicit is rejected with a static error. The user resolves by switching to the named form (always well-formed) or by introducing a typed local. The element-to-field assignment under positional form is by declaration order: the LHS type's first declared field receives the first entry, the second receives the second, and so on. Defaults cannot be omitted from the positional form (the omission would create an off-by-one alignment hazard); a positional construct for an LHS type with a defaulted field must either supply a value for that field or switch to the named form.

**Aggregate-typed slots and positional field access.** A slot of literal type `Aggregate` (§7.7, §7.9) holds an Aggregate Literal value directly without further structural matching against a named record-or-object type. A `# Aggregate a <- ${...}` introduction admits an Aggregate Literal — named or positional — and the literal becomes the slot's held value. **`Aggregate`-typed slots accept only Aggregate Literals (Rule 1)**; an initialization temporary (Rule 2 or Rule 3) is not assignable to an `Aggregate`-typed slot, since init temps do not persist as runtime values. The typechecker rejects `# Aggregate a <- ${...}` whose RHS classifies as an init temp.

The motivating use case for `Aggregate`-typed slots is indirect passing: a single Aggregate Literal value can be held in a slot, and its fields used as constructor arguments at multiple call sites — rather than re-emitting the literal at each. The pattern is most useful when the aggregate captures parameters that initialize multiple objects with related shape; literal-to-typed conversion happens through the constructors at each call site, the same as anywhere else in the language (§7.8).

For an `Aggregate`-typed slot whose value was supplied as a positional literal — no field names declared — the fields are accessible by 1-based positional index via the scope operator `::`:

    # Aggregate a <- ${"fish", "normal", 3.7}
    # String name <- a::1                          ; "fish"
    # String mode <- a::2                          ; "normal"
    # Float64 weight <- (Float64: a::3)            ; convert the Decimal field 3.7 to Float64

Each Aggregate Literal (and Sequence Literal, §7.5) instance carries anonymous **literal-kind information** about each position — the per-position literal kind (`String`, `Integer`, `Decimal`, `Hex`, `Aggregate`, `Sequence`) is recorded by the typechecker per literal — so downstream constructor calls using the slot's fields are checkable against what's actually there. In the example above, `a::3` is statically known to carry a `Decimal` literal (from the third position of the initializing literal), so the constructor call `(Float64: a::3)` is well-formed when a `Float64`-from-`Decimal` constructor is in scope. Writing `(Float64: a::2)` instead would be a static error: the typechecker sees the request as `Float64`-from-`String` and rejects it unless a `Float64`-from-`String` constructor is in scope. The tracking is per-literal-instance, not a structural type the slot's `Aggregate` annotation declares; and the tracked information is the literal-kind tag only — literal values do not participate in the general typesystem (no class instances, no polymorphism, no variant or union candidacy, §7.7), so the tracking machinery covers exactly what literals support and no more.

For an `Aggregate`-typed slot whose value was supplied as a named literal, fields are accessible by declared name (`a::weight`, `a::name`, etc.). The positional-index form is admitted for unnamed-field aggregates only; mixing positional indices with named-form aggregates is a static error.

The same disambiguation rules and constraints apply to `$[...]` (Sequence Literals and sequence-shaped initialization temporaries, §7.5).

### 7.5 Sequence Literals

A sequence-shaped construct — `$[value, value, ...]` — initializes a positional, vector-like target. The same disambiguation rules of §7.4 apply: a `$[...]` containing only Literals is a **Sequence Literal**; a `$[...]` containing both Literals and non-Literals is an **initialization temporary** with literals bridged to each position's expected type via `.implicit` conversion (§7.8); a `$[...]` containing only non-Literals is an initialization temporary with each element consumed at its expected type directly. **`Sequence`-typed slots accept only Sequence Literals (Rule 1)**; an init temp is not assignable to a `Sequence`-typed slot.

The form initializes a buffer primitive `[N]` or `[]`, or a typed buffer `[N]T` or `[]T`. The opening token `$[` is unambiguous; the closing `]` is the matching bracket. The empty form `$[]` is a single token denoting a construct with zero elements; vacuously all-Literal, it is a Sequence Literal. It is well-formed for `[0]T` (the zero-length typed buffer) and for `[]T` (the unbounded typed buffer's empty case), and ill-formed for `[N]T` with N greater than zero.

The typechecker enforces two rules:

- **Element count must match the LHS type's expected count, unless the LHS is unbounded.** For `[N]T`, exactly N elements; for `[]T`, any count, with the count becoming part of the buffer's runtime length.
- **Each element must be a value of the LHS type's element type.** The element may come from any source that produces that type — an identifier of the type, a constructor call producing the type, an expression yielding the type, or a bare literal whose source type has a matching constructor in scope (§7.7, §7.8). A Sequence Literal `$[1, 2, 3]` for a `[3]Float32` LHS is well-formed when a constructor producing `Float32` from `Integer` is in scope (`1`, `2`, `3` are `Integer` literals; a Sequence Literal containing fractional values like `$[1.0, 2.0, 3.0]` would require a `Float32`-from-`Decimal` constructor instead).

For untyped buffer primitives `[N]` and `[]`, the element type is the byte (a one-byte cell with no domain interpretation); a sequence-shaped construct targeting `[N]` requires each element to be of byte width.

The aggregate and sequence forms are **disjoint** in the LHS types they accept. Sequences do not initialize records, objects, unions, or variants; aggregates do not initialize buffer primitives or typed buffers. The two surface forms occupy non-overlapping construction territory: records have *named* field structure (the aggregate form's nominal matching is the natural surface); typed buffers have *positional* element structure (the sequence form's element-by-position matching is the natural surface).

### 7.6 Bare-Identifier Value-Copy

The bare-identifier form `#y <- x` (or `'r <- x`, or `&y <- x`) is a primitive value-copy. No command runs, no literal is supplied, no `.implicit` conversion fires. The runtime is a byte-copy from `x`'s storage to the LHS slot, suitable for types whose values can be copied as bytes.

The form is well-defined for three categories of types:

- **Buffer-backed types** (records, plain domains, unions, buffer primitives `[N]` / `[]`, typed buffers `[N]T` / `[]T`). The bytes of `x` are copied to the LHS slot's storage. The LHS and RHS types must be compatible: identical, or one a parent of the other in the buffer-backed subsumption hierarchy (§5.5). Sibling domains do not implicitly convert peer-to-peer; the user resolves cross-sibling moves by invoking a conversion constructor or by routing through a shared ancestor.
- **Pointers `^T`.** The pointer value is copied; both pointers reference the same target. The pointed-to storage is unaffected.
- **Command-typed values.** The command-value is copied — including hidden capture fields for lambda values and slot references for fexpr values. Capture-ceiling and fexpr-relevance taint (§6.15) apply to the copy as they do to the source.

The form is **rejected at the typechecker** for two non-buffer types:

- **Objects.** Objects have identity. A primitive byte-copy of an object would either share the identity (which is a pointer-bind, not a copy) or duplicate the storage (which would require a class-specific copy constructor). Neither is the right meaning for a primitive `<-`. The user invokes a copy-constructor explicitly, or shares access via a `^Object` pointer (which is itself a pointer-copy, well-formed under the pointer case above).
- **Variants.** Variants have reference-semantics: the slot is a 3-word triple (tag, candidate-pointer, witness) whose candidate-pointer references shared candidate storage. A primitive byte-copy of the slot would create two slots referencing the same candidate storage — an aliasing the language does not track. The user invokes a constructor (which produces a fresh variant slot) or shares access via a `^Variant` pointer.

Both rejections are typechecker-side; the diagnostic names the type's identity-bearing-or-reference character and points the user at the appropriate constructor or pointer surface.

### 7.7 Bare Literals

A **bare literal** is a literal value supplied directly as the right-hand side of `<-`, without a parenthesized constructor call. The language admits six literal types:

- **Integer** — numeric literals without fractional component, e.g., `0`, `42`, `-100`.
- **Decimal** — numeric literals with fractional component, e.g., `3.14`, `-0.5`, `0.0`.
- **Hex** — hexadecimal numeric literals, e.g., `0x41`, `0xDEADBEEF`.
- **String** — character-sequence literals delimited by double quotes, e.g., `"hello"`. The language assigns no encoding to the characters; encoding is the responsibility of the constructor that consumes the literal (§7.8).
- **Aggregate** — the `${...}` form, also serving as the value held in `Aggregate`-typed slots (§7.4, §7.9).
- **Sequence** — the `$[...]` form, also serving as the value held in `Sequence`-typed slots (§7.9).

`Integer` and `Decimal` are deliberately separate literal kinds rather than a unified "Number" kind. Conflating them would force constructor authors to discriminate at runtime between fractional-bearing and integer-only values, or would push that discrimination into the constructor's type — neither composes well with the encoding-deferral discipline of §7.7. With the kinds separate, an `.implicit Float32 'r: Integer i` constructor and an `.implicit Float32 'r: Decimal d` constructor are independent declarations, each handling one literal kind cleanly.

There is deliberately no separate `Char` literal type. A single byte is the buffer primitive `[1]` (§5); a single user-perceived character lacks a coherent definition without imposing encoding assumptions, and the language declines to impose them at the literal level. A single-character `String` literal — `"A"` — covers the use case where a character literal would otherwise be sought, and the encoding question is deferred to the constructor that consumes it.

A bare literal at a typed slot is admitted in two distinct cases:

- **LHS is a literal-typed slot.** A slot of the matching literal type holds the literal value directly, with no conversion. `# Decimal d <- 3.14` introduces a `Decimal`-typed slot holding the literal; `# String s <- "hello"` introduces a `String`-typed slot; `# Aggregate a <- ${...}` and `# Sequence s <- $[...]` are the parameterized-literal cases (§7.9) for aggregate and sequence literals respectively.
- **LHS is a non-literal typed slot, with `.implicit` conversion in scope.** An `.implicit` constructor (§7.8) whose source-parameter type matches the literal's source type and whose productive output matches the slot's type bridges the literal into the LHS type. The typechecker inserts the call: writing `# Float32 x <- 3.14` reads to the typechecker as `# Float32 x <- (Float32: 3.14)` if a matching `.implicit Float32 'r: Decimal d` constructor is in scope.

The six literal types are a category of types distinct from buffer-backed types (records, plain domains, unions, buffer primitives, typed buffers) and from non-buffer types (pointers, command-typed values, fexpr-typed values, objects, variants). Literals are neither buffer-backed nor non-buffer in the §5 classification — they are *literal types*, a third category. Structurally, they are **raw data values** carrying neither behavioral information nor encoding assumptions: a `String` literal is a sequence of characters with no declared encoding, a `Decimal` literal is a numeric value with no declared precision or representation, and an `Aggregate` is a fielded grouping whose shape — the literal-kind tag at each position — is determined per-literal at the construction site rather than declared at the type level. Encoding and typed-representation choices are made at the constructor boundary, where a constructor consumes the literal and produces a value of the target type; constructors invoked via `.implicit` (§7.8) elide this step at the bare-literal surface, but the conversion is the same constructor in either case.

**Literals do not participate in the general typesystem.** The "type information" tracked for literal values is the literal-kind tag (`String`, `Integer`, `Decimal`, `Hex`, `Aggregate`, `Sequence`) — nothing more. Literal types **cannot be instances of any class** (no behavior to dispatch on), **cannot appear as candidates of unions or variants** (no consistent representation for a discriminated container), and **cannot be the subject of polymorphic or generic type machinery**; they are passive data with a kind tag, useful for the construction surface and for anything-derivable-from-the-kind-tag, and not for anything else. The variant-typed-field explicit-`_` rule of §7.16 **does not extend to `Aggregate`-typed slots** (the rule governs structured LHS types whose schemas pre-declare which fields are variant-typed; an `Aggregate`'s shape is determined per-literal at the construction site, so no pre-declared variant field exists for the rule to govern).

The motivating use case for literal-typed slots is indirect passing — capturing a literal value in a slot for reuse — rather than computation in the literal type itself. Literal types' utility ends at the constructor boundary, where they are consumed; downstream operations work on the constructed typed values, which participate fully in the general typesystem.

### 7.8 The `.implicit` Mechanism

`.implicit` is a constructor-declaration prefix parallel to `.cmd`. A constructor declared with `.implicit` registers as the elision target for the case "literal of type L appearing in a context expecting T." The typechecker, upon seeing a bare literal at a typed slot, looks up an `.implicit` constructor whose parameter type matches the literal's source type and whose productive output type matches the slot's type, and inserts the call.

The declaration form is parallel to `.cmd`'s constructor form, with `.implicit` as the prefix:

    .implicit Float32 'r: Decimal d =
        ; constructor body

This declares an `.implicit` constructor that:

- Produces values of type `Float32` (the receiver's type).
- Takes a single parameter of literal type `Decimal`.
- May be invoked explicitly as `(Float32: 3.14)`.
- May be invoked implicitly when a `Decimal` literal appears in a `Float32`-expecting context.

The mechanism is purely additive: an `.implicit` constructor is also an explicit constructor. `(Float32: 3.14)` works regardless of whether `Float32`'s constructor is declared `.implicit` or `.cmd`. The implicit elision is a convenience on top of the explicit form, not a replacement.

`.implicit` carries three structural restrictions:

- **Constructor-only.** `.implicit` may only be applied to constructor commands (commands with a productive `'r` receiver of buffer-backed type, per the constructor signature shape of §3.9). Applying `.implicit` to a method, a regular command, or any other shape is a static error.
- **Literal-source restriction.** The single non-receiver parameter must be a literal type — one of `Integer`, `Decimal`, `Hex`, `String`, `Aggregate`, `Sequence`. Implicit conversions whose source is not a literal type would multiply the resolution candidates and make call-site reasoning brittle; the literal-source restriction confines the mechanism to its motivating cases.
- **No implicit context parameters.** `.implicit` constructors may not declare implicit context parameters (the `/`-separated list of §3.6). The implicit mechanism elides a construction step at the surface; admitting context parameters in `.implicit` constructors would compound elision (literal elision plus context resolution elision), making call-site reasoning brittle. Constructor commands declared `.cmd` retain the standard implicit-context-parameter capability.

The resolution algorithm for a bare literal at a typed slot: at compile time, the typechecker collects `.implicit` constructors in scope whose parameter type matches the literal's source type and whose productive output type is at-or-above the slot's type per the buffer-backed subsumption hierarchy. If exactly one matches, the typechecker inserts the call; if multiple match at the same specificity, the resolution is ambiguous and the user must disambiguate by typed introduction (§7.2). Resolution is per-call-site; no transitive `.implicit` chains are formed.

### 7.9 Parameterized Literal Types

The literal types `Aggregate` and `Sequence` are parameterized: their concrete shape — field types and order for `Aggregate`, element type and count for `Sequence` — is declared as part of the literal-type specification.

The `Aggregate` form, when used as an `.implicit` constructor source, names the field names and types it accepts:

    .implicit Point 'r: ${Decimal x, Decimal y} src = ...

This declares an `.implicit` constructor that takes an aggregate literal of shape `${Decimal x, Decimal y}` and produces a `Point`. The declaration is the literal-type-mirror form: the aggregate-literal surface that would construct values of this shape (§7.4) and the parameterized type that captures its shape are notationally aligned.

The `Sequence` form, similarly:

    .implicit InitialValues 'r: $[3]Integer src = ...

This declares an `.implicit` constructor that takes a sequence literal of length 3 with elements of literal kind `Integer` and produces an `InitialValues` value.

The element type in a parameterized `Sequence` form, and the field types in a parameterized `Aggregate` form, must themselves be **literal kinds** — `Integer`, `Decimal`, `Hex`, `String`, `Aggregate`, `Sequence` — not buffer-backed types or non-buffer types. A form like `$[3]Int32` is ill-formed: `Int32` is a buffer-backed domain (§5), not a literal kind, and a literal value cannot have a buffer-backed element type without dragging the buffer-backed domain into the literal category, which would defeat the literal/typesystem split of §7.7. The buffer-backed conversion happens at the constructor: in the example above, the `InitialValues` constructor body consumes the `Integer` literals and produces a value whose internal representation may include `Int32`-typed slots, but the literal-side input is `Integer`-kinded.

The `.implicit` resolution for parameterized literal types follows the same rules as for simple literal types: the typechecker matches the literal's parameterized shape against the source-parameter declaration and inserts the call when an exact-shape match is found.

### 7.10 The `=` Defaults Declaration

A buffer-backed type whose bytes do not have a meaningful zero declares its default value via the definitional `=` form. The form supplies the value used when the type's bare introduction (`# T x` without initializer) would otherwise be rejected:

    .domain Positive: Int32 = (Int32: 1)

A `Positive` slot introduced as `# Positive p` (no initializer) acquires the declared default value at introduction; the slot is initialized from inception.

The split between `=` (definitional) and `<-` (runtime placement) is exact:

- `=` appears in declaration positions (`.cmd ... =`, `.domain T = ...`, `.record T : ... = ...`).
- `<-` appears in expression-position placement (`#x <- ...`, `'r <- ...`, etc.).

Both consume the same five-shape RHS grammar (§7.3). A reader sees `=` and knows they are looking at a declaration; sees `<-` and knows they are looking at a runtime operation. The unification of the RHS grammar means the user-side surface forms — aggregate, sequence, parenthesized call, bare identifier, bare literal — are uniform across both positions.

A record default uses the aggregate form:

    .record Point :
        Float32 x
        Float32 y
        = ${x <- 0.0, y <- 0.0}

A typed-buffer default uses the sequence form:

    .domain InitialValues : [3]Int32 = $[1, 2, 3]

In both examples, the `=` RHS is an Aggregate Literal (Rule 1 of §7.4) or Sequence Literal whose contents are bare literals — `0.0` is a `Decimal` literal, `1` and `2` and `3` are `Integer` literals — bridged to the LHS field/element types (`Float32`, `Int32`) via `.implicit` constructors at the field/element boundary. Defaults are **typically** Literal-form because they describe compile-time-constant values; the language admits initialization-temporary form (Rule 2 or Rule 3) at `=` positions when the default involves runtime-computed components, but the literal form is preferred where applicable for clarity and for compile-time constant-folding.

The `=` form is **not** an exception to the no-defaults discipline (§5.1, §6.14) but the user-facing mechanism for declaring a type's invariant-respecting default. The discipline is:

- **Buffer-backed types whose bytes have a meaningful zero** acquire a zero-default by composition. No `=` declaration is needed; `# T x` produces a zero-filled initialized slot.
- **Buffer-backed types whose invariants forbid zero** declare an explicit default via `=`. Without the declaration, `# T x` is rejected; only `# T x <- value` works.
- **Non-buffer types** (pointers, command-typed values, fexpr-typed values, objects, variants) cannot have `=` defaults. Their bare introduction is rejected at the type level; the only valid construction is via `<-` at every use site. Variants are the structural exception within the non-buffer category — their absent state is the inherent representation of an "empty" slot, and bare `# T x` produces an absent slot directly without any `=` mechanism.

The `=` form's right-hand side is evaluated at declaration time, not at every use site. Implementations may treat this as a compile-time constant fold or as a per-use-site lazy initialization (the latter when the right-hand side has runtime-determined components). The semantic guarantee is that a `# T x` introduction acquires the declared-default value, irrespective of when the right-hand side is realized.

The programmer is responsible for deciding whether a buffer-backed type's invariants permit zero-default. For types whose bytes admit zero meaningfully (counts, indices, accumulator patterns), no `=` is needed. For types whose invariants exclude zero (positive numbers, nonempty strings, validated formats), the `=` declaration is required to admit bare introduction.

### 7.11 Atomic Compound Construction

The productive write-once rule (§6.13) requires every successful return path through a constructor's body to write the productive receiver `'r` exactly once. Combined with the whole-slot tracking discipline (§6.14) — which tracks the slot as a unit, not as a graph of fields — the rule implies that compound construction is **atomic**: a single conceptual write fills the entire slot.

The constructor body's natural shape is two-phase:

- **Phase 1: Compute the constituent values.** The body invokes whatever commands are needed to produce the values that will populate the new instance. Each computed value lives in an ordinary local slot — typically a parameter, a `#`-introduced local, or the result of a may-fail command in expression-position. These slots are tracked individually under the standard whole-slot rules; their initialization states are independent.
- **Phase 2: Atomically initialize the receiver.** The body performs a single `<-` to the productive receiver `'r`. This single `<-` fills the entire `'r` slot. Before this write, `'r` is uninitialized; after this write, `'r` is initialized; there is no intermediate state where some fields are written and others are not.

The Phase 2 right-hand side is, in practice, an aggregate literal (for records, objects, unions, variants), a sequence literal (for typed buffers), a parenthesized call (where another constructor produces the value), a bare identifier (for value-copy, when applicable), or a bare literal (when a matching constructor is in scope, §7.8). Whatever the right-hand side shape, the single `<-` discharges the write-once obligation.

The pattern is structurally aligned with the language's other commitments. A partially-built compound cannot exist; therefore, a failure during construction cannot leave a half-built thing behind. The constructor body reads top-to-bottom: compute values, then commit. No hidden reordering, no implicit zero-fill, no constructor-chain hidden in an initializer list.

Edge cases the rule enforces:

- **Conditional re-writing rejected.** A pattern that writes `'r` once and then conditionally rewrites it — e.g., `'r <- a` followed by a conditional `'r <- b` on some path — is rejected because the condition-true path writes twice. The fix is the natural rewrite: choose the value first via a `?:` chain, write once.
- **Conditional initial-write rejected.** A pattern that writes `'r` only on some paths is rejected because the omitted paths fail the at-least-once requirement. The fix is to provide a default arm or to introduce a recovery path that writes.
- **Loops cannot write productive slots.** A productive write inside a loop body would mean N writes per N-iteration completion, violating exactly-once. Loops that compute values into a productive slot must do so via a single write after the loop body, with the per-iteration values accumulated in an ordinary local that is then committed once to `'r`.

### 7.12 Variant Construction

Variant construction has three distinct surfaces, each fitting a different intended outcome:

- **Constructing with a chosen candidate.** A construct whose single entry names the candidate:

      'r <- Shape: ${Circle <- radius}              ; init temp (Rule 3): `radius` is a non-Literal slot
      'r <- Shape: ${Circle <- 5.0}                 ; Aggregate Literal (Rule 1): `5.0` is a Decimal literal

  The type prefix `Shape:` provides the type context the construct needs in argument-position contexts. Where the LHS type is contextually explicit (a typed productive parameter, a typed local introduction, a typed field), the prefix may be elided: `'r <- ${Circle <- radius}` is well-formed when `'r` is declared of type `Shape`. The classification per §7.4's rules determines whether the construct is an Aggregate Literal (Rule 1) or an initialization temporary (Rule 2 or 3); both are accepted at variant LHS positions, with the candidate's parameter type providing the conversion target for any Literal contents (the Decimal literal `5.0` above bridges to `Float32` — Circle's parameter type — via `.implicit`).

- **Introducing the absent state.** Two surfaces produce an absent-state variant slot:

      # Shape state                         ; bare introduction; absent
      # Shape state <- ${}                   ; equivalent: empty Aggregate Literal

  Both forms produce the same result: a 3-word slot zero-filled (zero tag, null candidate-pointer, null witness) and in the absent state. The bare-introduction form is admitted only for variants among the non-buffer types; pointers, command-typed values, fexpr-typed values, and objects all reject bare introduction. The empty-aggregate form is the explicit-initializer counterpart and is available wherever a `${...}` RHS is admitted, including assignment to an existing variant slot (`state <- ${}` resets `state` to absent, parallel to `state -< _` of §7.14).

- **Clearing an existing variant to absent.** The `-<` discard form `v -< _` (§7.14) clears `v` to its absent state.

For multi-argument candidates (e.g., a `Rectangle : Float32, Float32` declaration in the variant), the candidate's value is itself an aggregate-shaped or sequence-shaped construct, depending on how the candidate's argument list is declared:

    'r <- Shape: ${Rectangle <- ${width <- w, height <- h}}    ; nested aggregate (init temp if w, h non-Literal)
    'r <- Shape: ${Triangle <- $[a, b, c]}                     ; nested sequence (init temp if a, b, c non-Literal)

The construction-side principle is uniform: the candidate's value is a value of the candidate's declared type, supplied by whatever surface form fits that type, with §7.4's disambiguation rules classifying any nested `${...}` or `$[...]` per content.

### 7.13 Variant Pattern Matching Idiom

Variant case analysis composes via `?:` chains plus the `-<` dynamic-narrowing operator (§7.14), with `?-` available for explicit absent-only tests. No dedicated `match` keyword is introduced.

The canonical pattern for a variant `v` with candidates `A`, `B`, ... and an absent case to handle:

    ?- _ -< v                              ; absent test
        handleAbsent
    ?: A 'narrowA -< v                     ; narrow to candidate A on success
        handleA: 'narrowA                  ; uses 'narrowA as the A-candidate value
    ?: B 'narrowB -< v                     ; narrow to candidate B on success
        handleB: 'narrowB
    handleUnmatchedCandidate               ; default arm; reaches here if v has a candidate not narrowed above

The pieces:

- The first guard `_ -< v` (§7.14) tests whether `v` has any candidate. It succeeds iff `v` is non-absent; it fails iff `v` is absent. The `?-` block engages on guard failure — when `v` is absent — running its body to handle the absent case.
- Each `?:` block's guard `T 'narrow -< v` attempts to narrow `v` into the typed productive slot `'narrow` of declared type `T`. The narrow succeeds if `v`'s active candidate's type is at-or-below `T` (per the variant hierarchy rules of §5.13); on success the narrowed value is bound to `'narrow` and the body runs with that binding in scope. On mismatch, the narrow fails, the `?:` body is skipped, and the chain advances.
- The trailing non-`?:` sibling is the chain's default arm, reached when no `?:` guard succeeded.

Coverage discipline is the user's responsibility. The typechecker does not enforce exhaustive variant-case coverage at `?:` chains; a chain that omits a candidate falls through to the default arm with the variant in its current state.

### 7.14 The `-<` Dynamic-Narrowing Operator

The `-<` operator performs a runtime type check and, on success, binds the narrowed value (or no value, in the discard cases) to its left-hand slot. On mismatch, it produces a propagating failure — the same surface form as the rest of the failure system. The operator is parallel to `<-` syntactically and contrasts on the static-vs-dynamic axis: `<-` writes a value the typechecker has confirmed at compile time; `-<` writes a value the typechecker has confirmed at compile time *might* fit, with a runtime check making the determination.

The operator's surface forms and their meanings:

| Form | Domain | Meaning | Failure mode |
| --- | --- | --- | --- |
| `T 'narrow -< v` | variant `v` | If `v`'s active candidate's type is at-or-below `T`, bind the candidate value to `'narrow`. | `TagMismatch` failure on candidate type-mismatch; `TagMismatch` failure on `v` absent. |
| `T 'narrow -< obj` | object `obj` (or `^Object`) | If `obj`'s runtime type is at-or-below `T`, bind the narrowed reference to `'narrow`. | `TagMismatch` failure on type-mismatch. |
| `T 'narrow -< p` | pointer `^P` admitting class-hierarchy narrowing | Same as object case applied to pointee type. | `TagMismatch` failure on type-mismatch. |
| `T 'narrow -< u` | union `u` (T buffer-backed) | Existential subsumption check at compile time: `T` must appear on at least one declared candidate's subsumption chain. On compile-pass, the narrowing always succeeds at runtime. | Compile-time error if `T` is on no candidate's chain; otherwise no runtime failure (§7.15). |
| `C 'narrow -< u` | union `u` (C a class) | Exactly-one-candidate admissibility: exactly one of `u`'s candidates must have a `C` instance. On compile-pass, the narrowing always succeeds at runtime, with the qualifying candidate's witness selected statically. | Compile-time error if no candidate or more than one candidate qualifies; otherwise no runtime failure (§7.15). |
| `v -< _` | variant `v` | Clear `v` to absent state. | Always succeeds. |
| `_ -< v` | variant `v` | Test whether `v` is non-absent. | `TagMismatch` failure if `v` is absent. |

In the form `T 'narrow -< value`, the binding follows the standard type-then-name convention (§3.3): the type T is the upper bound for the productive slot `'narrow`, which is bound to the narrowed value on success. The form is parallel to a parameter declaration `T 'name` — same syntax, same meaning at the binding site.

The narrowed slot `'narrow` is productive — the operator's productive-side commits a value on success, paralleling the `<-` form's productive write. The same write-once obligation applies if `'narrow` is the constructor's productive receiver (which would be unusual; `'narrow` is most commonly a `#`-introduced local).

The operator's failure-set is `{TagMismatch}` — a single failure message the language emits for `-<` mismatches and for variant absent-cases on the `T 'narrow -< v` and `_ -< v` forms. The message integrates with typed failures (§4) per the standard machinery: a `?:` chain implicitly emits the `TagMismatch` message on the failure path; a `|: TagMismatch` recovery elsewhere catches it.

### 7.15 The `-<` Operator Generalized

The `-<` operator is the language's unified dynamic-narrowing surface across multiple type-pair scenarios: variant-to-candidate, object-to-subclass, pointer-to-subclass-of-pointee, union-to-candidate-or-parent, and union-to-class. The unification follows principle 8 of §1.2 — small orthogonal concepts over rich overlapping ones: existing language facilities compose to do the work; reaching for a per-case operator would violate the orthogonality preference.

The operator's run-time behavior varies by domain while its surface form does not. For variants and class hierarchies (objects, object-pointers), the narrowing is genuinely runtime-checked: the value's tag or runtime type is compared against the target type, and a `TagMismatch` failure fires on inequality. For unions, the operator has no runtime discriminator to consult; admissibility is checked entirely at compile time, with two distinct rules per the §5 union-section coverage. For a buffer-backed target type `T`, admissibility is *existential*: `-<` succeeds when `T` appears on the buffer-backed subsumption chain of at least one declared candidate. For a class target `C`, admissibility is *exactly-one*: `-<` succeeds when exactly one of the union's candidates has a `C` instance, with the language selecting that candidate's witness statically; admissibility fails if no candidate or more than one candidate qualifies. In both union cases, an admissible `-<` always succeeds at runtime — the failure-recovery branch attached to a `-<` on a union is statically unreachable, which the typechecker may flag as an unreachable-code warning while preserving the code form for uniformity with the variant and class cases.

For union targets, the static admissibility check is not a guarantee that the union's bytes in fact represent a valid target-typed value. The language admits the byte-reinterpretation; the programmer's discrimination machinery — typically a discriminator field in a containing record, or program logic surrounding the union — remains responsible for ensuring the union's current bytes are a valid value of the target type. This is the standard user-asserted-byte-validity discipline for unions (§5).

The implicit byte-reinterpretation subsumption (§5 buffer-backed parent-chain rule for unions) is the *static* counterpart to `-<` on unions: the language admits a union value at a candidate-or-parent-typed slot via implicit byte-reinterpretation without an operator. The two surfaces are complementary — `-<` gives the user an explicit narrowing form, while implicit byte-reinterpretation gives the user a syntactically lighter assignment surface — and the user chooses based on how prominent the discrimination should be in the code's surface.

A `-<` operation on type pairs not enumerated in §7.14 or in §5's union coverage is a static error reported at the operator. The user resolves by invoking an explicit conversion constructor or by routing through the appropriate language-admitted subsumption.

### 7.16 Aggregate Variant-Typed Fields Require Explicit `_`

A variant-typed field of a structured aggregate-literal LHS type — a record or object whose declared fields include variant-typed fields — requires an explicit value entry, including, when the absent state is intended, an explicit `_` marker. Variants admit an absent state inherently (§5.13); were variant fields admitted to fall back to a "default" of absent, positional-form aggregate literals would risk silent off-by-one alignment when a variant field's entry was simply omitted. The explicit-`_` rule preserves positional-transcription integrity. The rule does not extend to `Aggregate`-typed slots (§7.4, §7.7) — an `Aggregate`'s shape is determined per-literal at the construction site rather than pre-declared at the type level, so there is no pre-declared variant field for the rule to govern.

Concretely, given an LHS record type with a variant-typed field `kind`:

    ${
        ; ... other fields ...,
        kind <- _                          ; named form, explicit absent
    }

    ${ /* other fields, */ _ }             ; positional form, explicit absent

Omission — `${ /* other fields */ }` with `kind` missing — is rejected. The diagnostic names the variant-typed field whose entry is absent and reminds the user that variant fields require explicit `_` for the absent state.

The `_` marker has multiple roles across the language: in the construction surface covered here, as the absent value for variant-typed fields and as the discard marker on either side of `-<` for variant operands (§7.14); elsewhere in the language, as the discard marker at PRODUCE positions in calls and as the partial-application deferral marker (covered with the relevant facilities). Within this section, the two construction-related roles share the same lexical token, the same "no value or value-discard" reading, and the same restriction: `_` is well-formed only at variant-related positions of the construction surface.

### 7.17 Choice Expression

The choice expression `lhs <- a | b | c` is a syntactic-sugar form that desugars into a chain of try-or-fall-through. The semantics: evaluate `a` first; if it succeeds, the result is committed to `lhs` and the chain stops; if it fails, evaluate `b`; if `b` succeeds, the result is committed and the chain stops; if `b` fails, evaluate `c`; and so on. The chain commits the first-success result; if every alternative fails, the choice expression as a whole fails with the last alternative's failure.

The desugaring is one substitution: `lhs <- a | b | c` becomes a `?:` chain with each alternative as a guard whose success commits to `lhs` and whose failure advances the chain. The underlying semantics has no hidden complexity; the choice expression is a notational convenience for expressing a left-to-right first-success pattern.

The `lhs` is constrained to be a slot of a single type; the alternatives must each produce a value compatible with the slot. For a typed introduction, `# T x <- a | b | c` declares the slot's type explicitly, with each alternative checked against `T`.

The choice expression composes with the failure system (§4): a `:`-marked alternative is statically certain to succeed and short-circuits the chain at that position; a `?`-marked alternative is the typical case; a `!`-marked alternative is statically certain to fail and is statically equivalent to skipping it. The typechecker reasons about the chain's overall failure mode based on its alternatives.

### 7.18 Failure Atomicity at `<-`

A `<-` whose right-hand side contains may-fail subexpressions is itself a may-fail operation. The discipline:

- **Atomic failure.** A failure in any subexpression aborts the whole `<-` operation. The LHS slot's pre-write state is preserved.
- **Order of evaluation is left-to-right.** For aggregate literals, fields are evaluated in textual order (whether named or positional). For sequence literals, elements are evaluated in textual order. For parenthesized calls, arguments are evaluated in textual order before the call fires.
- **Short-circuit on failure.** Once a subexpression fails, subsequent subexpressions in the same `<-` are not evaluated. The whole operation propagates the failure.
- **Failure mode propagates from the `<-` form.** The form acquires the failure mode of any may-fail subexpression: a `<-` containing one or more `?`-marked subexpressions is may-fail; a `<-` containing a `!`-marked subexpression is must-fail. The standard failure-system conformance rules apply (§4.13).

The atomicity falls out of copy-restore (§6.4): the language commits no half-written state on failure, regardless of which subexpression failed and at what point. No new transactional machinery is introduced.

The implementation device: each subexpression's value is computed into a temporary slot until every subexpression of the `<-` has succeeded; only then does the placement into the LHS slot fire. The user-side observation is that the LHS is unchanged across the entire `<-` operation if any subexpression fails.

A nested construction `'r <- ${field <- (innerCtor: ...)}` composes failure-atomicity transitively. A `?`-marked failure inside `innerCtor` propagates to its caller; copy-restore at the `innerCtor` call leaves the field's temporary slot uninitialized; the outer `<-`'s atomic-failure rule sees a failed field-value computation; the outer `<-` aborts; the outer `'r` is unchanged. The chain reaches arbitrary depth without new mechanism. The frame-ownership lens (§1.5) applies cleanly: each level's productive slot is *that level's caller's* slot, and each level's failure leaves that caller's slot in its pre-call state.

### 7.19 Embedded Objects in Aggregate Literals

When a record or object has an embedded object field, the aggregate literal references the embedded object by a previously-introduced local name:

    .cmd buildContainer: Container 'r =
        # contained <- (InnerObj: someArgs)         ; Phase 1: construct constituent
        'r <- ${inner <- contained, count <- 5}     ; Phase 2: commit

The pre-construction in Phase 1 introduces the embedded object as a local in the constructor's frame. The Phase 2 aggregate-literal references the local by name, supplying it as the value of the `inner` field.

The embedded object's at-stack registration migrates to the new container's owning frame at the moment of the aggregate literal's atomic placement. This composes with the existing frame-migration mechanism for objects (§3.13); no new mechanism is introduced. If `buildContainer` fails — say, a `?`-call inside Phase 1 — the partially-constructed `contained` object is registered with `buildContainer`'s frame and is cleaned up via the failure-exit machinery there. The atomicity story holds: no half-built container exists at any caller's frame.

The pattern composes with the productive write-once rule (§6.13): the Phase 2 commit is a single `<-` to `'r`, which discharges the productive obligation in one write. The Phase 1 locals (`contained` here) are ordinary locals, tracked individually, with their initialization states independent of `'r`'s.

### 7.20 Allocation and the `\` Qualifier

Value construction in Basis involves two distinct allocation activities, separated by where the storage lives:

- **Slot allocation** is the frame storage for the slot itself — the fixed-size bytes that hold a local's value, fat pointer, or 3-word slot. This happens at `#` introduction and is implicit in the frame's existence; the user writes `# T x ...` and the slot's storage is provided by the frame.

- **Memory allocation** is the storage for any variable-size data the slot's value references — the element bytes behind a `[]T` value, the pointee behind a constructed `^T`, the storage backing an object's identity. This happens at value construction.

Slot allocation never needs a qualifier: the frame owns its slots.

Memory allocation has a default and an override. The default is the **frame-bound region**: storage is drawn from a region tied to the current frame and freed at frame retirement. The default requires no syntax — `# []Int32 x <- $[1, 2, 3]` constructs a runtime-length buffer whose three-element data area lives in the current frame's region.

When the user needs non-default storage — heap allocation for data that outlives the frame, an arena scoped to a logical phase, a pool with a recycling discipline — the `\` qualifier post-fixes the construction expression with the allocator value to use:

    # []Int32 x <- $[1, 2, 3] \ heapAlloc
    # ^Cache p  <- (Cache: arg1, arg2) \ heapAlloc

The allocator (here `heapAlloc`) is a first-class value: an in-scope name of a type that implements the `Allocator` class. **Bringing an allocator into scope does nothing on its own** — the value is used only when the user writes `\ allocator-expr` at a construction site. No ambient context affects allocation; every non-default allocation event is visible at the source where it happens.

The `\` qualifier is **per-construction-expression**, not per-statement or per-frame. Different constructions on different statements may use different allocators:

    # []Int32 a <- $[1, 2, 3] \ heapAlloc
    # []Int32 b <- $[4, 5, 6]                ; this one uses the frame region

The qualifier is **syntactic sugar**: at typecheck and code generation, the construction desugars to explicit method invocations on the named allocator. The default-case construction (no `\`) similarly desugars to invocations on the language-provided frame-region allocator. The exact `Allocator` class shape — its methods, their signatures, the semantics of cross-allocator transfer — is part of the standard library and is not pinned down in this specification.

**Lifetime ceiling rules are unaffected by the qualifier.** A `\ heapAlloc` qualifier on a construction places the value's variable-size data in the named allocator's storage but does not extend the value's lifetime ceiling. The language's visibility rules (§5.11 for objects, §6 for parameter modes) continue to govern reachability of the value through the language's slots and access paths; the allocator governs only where the underlying bytes live and when they are released. A heap-allocated object reached only through a frame-bound local is still subject to the local's lifetime ceiling at the language level, even though the heap-resident bytes persist in the allocator's own discipline.

---

## 8. Lambda and Fexpr

The four constructional forms producing command-typed values — command reference, command literal, lambda, and fexpr — share a common umbrella: each produces a value of a command type, may be invoked, and (subject to per-form restrictions) may be captured, passed as an argument, or stored in a slot. The forms differ along three axes: whether they have a body of their own (command literals, lambdas, and fexprs do; command references refer to an existing command's body); what state they capture from their construction site (command literals capture nothing, lambdas capture explicitly via a slash list, fexprs capture implicitly by free name, command references bind partial-application arguments); and how their lifetimes relate to the construction site (lambdas and fexprs are ceiling-tracked; command literals have no construction-site ties; command references inherit ceiling from any reference-bound arguments).

This section specifies each form's surface and semantics, the failure-mark conformance discipline that governs how command-typed values may be invoked and assigned, the context-variables umbrella across the forms, the constraints on combining fexprs with the other forms, and the seven fexpr restrictions that ensure fexpr ceiling-tracking remains sound. The mode-and-taint mechanisms governing captures (§6.10, §6.15) are referenced rather than repeated; the reader is assumed familiar with the lambda capture-list mode constraints (READ and REFERENCE only, §6.10), the fexpr free-name-access mode inheritance from the defining frame (§6.10, §6.15), and the fexpr-relevance taint discipline (§6.15).

### 8.1 The Four Constructional Forms

The four forms are summarized below.

| Form | Surface | Body? | Captures | Lifetime |
| --- | --- | --- | --- | --- |
| Command reference | `{cmd}`, `{cmd: x, _, y}`, `{receiver :: methodName}` | no (refers to an existing command's body) | partial-application bindings only | ordinary; ceiling-tracked iff any `&`-bound argument |
| Command literal | `:<args>{body}` | yes | none | ordinary object lifecycle |
| Lambda | `:<args / captures>{body}` | yes | explicit slash list, READ and REFERENCE modes only (§6.10) | ceiling computed from captures (§8.4) |
| Fexpr | `:{body}`, `?{body}`, `!{body}` | yes | implicit by free name; mode inherited from defining frame's binding | ceiling = `D` (the defining frame), uniformly (§8.5) |

The `:<args>{body}` and `:<args / captures>{body}` forms share the outer notation; the presence or absence of the slash discriminates command literal from lambda. The fexpr forms `:{body}`, `?{body}`, `!{body}` are distinct: they have no angle-bracketed parameter list, since fexprs take no arguments of their own — their effective parameters are the free names in the body, resolved against the defining frame at invocation. The leading mark on a fexpr (`:`, `?`, `!`) is the body's failure character (§4) and appears in the fexpr's type signature `:<*>`, `?<*>`, `!<*>` correspondingly.

The pure-thunk form `:<>{body}` is a command literal with an empty parameter list — a zero-argument body. It is a special case of the command literal form, useful for deferring the execution of a body without needing a parameterized signature.

### 8.2 Command Reference

A command reference is a value of command type produced by enclosing a command name in braces, optionally with partial-application bindings. The forms are:

- **`{cmd}`** — bare command reference to a non-method command; produces a command-typed value with `cmd`'s full signature.

- **`{cmd: x, _, y}`** — positional partial application; binds positional arguments at the construction site, with `_` placeholders at unbound positions. Produces a command-typed value whose signature is the original minus the bound positions.

- **`{receiver :: methodName}`** and **`{receiver :: methodName: x, _, y}`** — method reference. The `::` operator binds the receiver, and this binding is **required** for method references: a method cannot be referenced without specifying its receiver, since the receiver is structurally part of the method's parameter list. The optional `: positional args` suffix adds positional partial application on the remaining parameters, exactly as on a non-method command reference. The result is a command-typed value with the receiver position and any explicitly-bound positional positions elided. The dispatch resolution is performed once at the construction site; subsequent invocations skip the per-call dispatch lookup — the canonical "tight-loop optimization" for class-instance method dispatch, a deliberate user-level choice rather than an automatic compiler transform.

The receiver-binding via `::` and positional-argument binding via `: ...` are orthogonal mechanisms. Both are partial application — the operation is uniform across the two surfaces. The difference is *which positions* each mechanism can bind: `::` binds the receiver position and is mandatory for method references; `: ...` binds positional argument positions and is always optional. The "receiver-elision" name describes a consequence of binding the receiver (the receiver position drops out of the visible signature), not a distinct operation. The structural rules of partial application — ceiling-tracking, mode restrictions, reference-chain flattening — apply uniformly to whichever positions are bound, by whichever mechanism.

A command reference has no body of its own; it refers to the underlying command's body. Invocation through a command reference is dispatched the same as a direct call to the underlying command, except that the bound arguments (in partial-application or receiver-elision) are supplied automatically and the dispatch lookup (in receiver-elision) is short-circuited.

**Mode restrictions on bound arguments.** A command reference may bind arguments at READ or REFERENCE positions of the underlying command's parameter list. PRODUCE positions cannot be bound: a productive parameter discharges its write at the invocation site, where the receiving frame owns the slot, and pre-binding would either capture a slot in the construction frame (defeating frame-ownership) or defer the slot identity to invocation (which is not partial application). The user resolves by leaving the productive position open with `_`, allowing each invocation to supply the productive slot. The rule parallels the lambda capture-list mode constraint (§6.10): PRODUCE is forbidden for the same structural reason in both surfaces.

**Reference-chain flattening.** When a command reference binds a `&`-mode argument (e.g., `{cmd: &x}`), the binding flattens through any reference chain: if `&x` is itself a reference parameter of the constructing command's frame, the binding chains through to the origin slot's frame. The command reference's effective ceiling is the origin slot's owning frame — the frame at which the slot lives, not the immediate constructing frame. A command reference with no `&`-bound arguments has no ceiling beyond the ordinary object lifecycle; it can be returned, stored, or moved freely.

**Excluded combinations with fexprs.** Per §8.12, command references can bind fexpr-typed arguments under the fexpr-tainting discipline of §6.15: the resulting command-typed value carries the fexpr-relevance taint of any fexpr-typed bound argument, propagating the `D`-ceiling constraint of the bound fexpr to the command-reference value.

### 8.3 Command Literal

A command literal is a command-typed value whose body is supplied at the construction site, evaluated eagerly. The surface form is:

    :<args>{body}

The angle-bracketed parameter list declares the command's parameters using the standard parameter-declaration syntax of §3 (mode markers and types). The body — between the matching braces — is the command's executable code, subject to the same parameter-mode discipline (§6) as any other command body.

**Eager evaluation.** A command literal's body is evaluated when the literal is invoked, not when it is constructed. Construction of the command literal produces a command-typed value; the body runs at each invocation site against the supplied arguments. The eager evaluation is identical to a regular `.cmd`-declared command's invocation semantics.

**No captures, no upstack ties.** A command literal captures nothing from its construction site. The body may reference only its own parameters and any names in scope at the construction site that have language-defined visibility independent of capture — for example, top-level commands and constants. A command literal value may be stored, passed, or returned freely; it has no construction-site lifetime constraint.

**The pure-thunk form.** The form `:<>{body}` is a zero-argument command literal — a body that takes no parameters. It is useful where a deferred-body value is needed without parameterization. The pure-thunk type is `:<>`, distinct from the fexpr types `:<*>`, `?<*>`, `!<*>`. There is no subsumption across the family boundary — a fexpr-typed slot is not assignable to a pure-thunk slot, even though both are never-fails.

A command literal's body is subject to the **transitive READ contract** (§6.5): if the body reads or modifies state reached through a parameter, the parameter's mode contract — including any taint propagated through the storage graph in the constructing frame's analysis — applies to the body's operations on that state. The contract is local to the construction frame's analysis; the command literal value, once constructed, carries the obligations as a property of the value but does not propagate further analysis state across the frame boundary.

### 8.4 Lambda

A lambda is a command-typed value with an explicit capture list. The surface form is:

    :<args / captures>{body}

The slash separates the parameter list from the capture list. The capture list enumerates names from the construction site that the body may reference; the body may reference its own parameters, the captured names, and any language-visible names independent of capture (top-level commands, constants), and nothing else.

**Capture modes.** The capture list admits READ-mode and REFERENCE-mode captures only. PRODUCE-mode captures are forbidden (§6.10): a productive obligation belongs to a specific call boundary in the defining frame and cannot be carried across a closure construction boundary. The two reasons (§6.10): lambdas may outlive the defining frame through ceiling-flattening of reference captures, leaving the productive slot potentially nonexistent at invocation; and lambdas have multi-invocation semantics, where deferring a productive write to invocation time fails the write-once rule.

**Reference-chain flattening for `&` captures.** A `&`-mode capture is flattened to the origin slot's owning frame, parallel to the rule for command-reference `&` bindings (§8.2). If `&x` captures a name that is itself a reference parameter of the constructing frame, the capture chains through to the origin; the lambda's ceiling becomes the origin slot's owning frame.

**Per-invocation copy-restore for `&` captures.** For each invocation of a lambda with `&` captures, the captured slot reference is bound at invocation time per the standard copy-restore discipline of §6.4: the invocation observes the slot's current value, may mutate it during the invocation, and the mutations are written back at the invocation's successful completion. Failures during the invocation leave the captured slot's pre-invocation value preserved, per failure-atomicity (§7.18). This is distinct from command-reference `&` *bindings* (§8.2), which are partial-application and operate on the bound slot directly without per-invocation copy-restore.

**Ceiling computation.** A lambda's ceiling is computed from its captures: a lambda with no `&` captures has no ceiling beyond the ordinary object lifecycle; a lambda with `&` captures has the ceiling of the most-restrictive reference-flattened owning frame across all `&` captures. The ceiling controls where the lambda value may be moved, returned, or stored; movement to a frame longer-lived than the ceiling is rejected.

**Two phenotypes.** Lambdas have two implementation phenotypes: a **lightweight** phenotype, used when the lambda has no `&` captures and the body's analysis fits a specific size budget — the lambda value is a function-pointer-with-bound-IN-captures pair; and a **ceiling-tracked** phenotype, used when the lambda has `&` captures or otherwise requires ceiling tracking — the lambda value carries the ceiling annotation alongside the function-pointer-and-captures payload. The phenotype is determined at compile time based on the construction-site analysis; the user-side semantics are identical across phenotypes.

**Visible-signature representation.** A lambda value's externally-visible type is its invoke-method signature — the parameter list and failure mark — without the capture list. The capture list is implementation detail; consumers of the lambda value see only what they invoke. This composes with the failure-mark conformance discipline (§8.6): a lambda's signature carries its failure mark, and conformance applies symmetrically.

### 8.5 Fexpr

A fexpr is a command-typed value whose body executes in the *defining frame's* state, accessing names from that frame at invocation time by free-name resolution. Fexprs are the language's mechanism for user-defined control-flow primitives — patterns where a downstream callee needs to read or modify the originating frame's state. The surface forms are:

    :{body}                    ; never-fails fexpr
    ?{body}                    ; may-fail fexpr
    !{body}                    ; must-fail fexpr

The leading mark — `:`, `?`, `!` — is the body's failure character (§4) and appears in the fexpr's type signature (`:<*>`, `?<*>`, `!<*>`). A fexpr has no parameter list of its own; its effective parameters are the free names referenced in the body, resolved against the defining frame at each invocation.

**The three-frame model.** Fexpr semantics involve three distinct frames at invocation time:

- **`D` — the defining frame.** The frame in which the fexpr was constructed (the `:{body}` was evaluated). The fexpr's body executes against `D`'s state at invocation; free names in the body resolve to bindings in `D`.
- **`I` — the invoking frame.** The frame from which the fexpr was invoked. `I` is downstream of `D` in the call stack — fexprs travel only down-stack from `D`, never up — and `I` may equal `D` (a same-frame invocation) or be a deeper frame (the fexpr was passed down through one or more calls).
- **`F` — the fexpr-execution frame.** A virtual frame within `D`'s scope where the fexpr's body executes. From `F`'s perspective, free-name accesses go to `D`; control-flow obligations (write-once for productive parameters of `D`, failure paths for `D`'s body, etc.) are tracked as if the body were inlined at the invocation site within `D`'s analysis.

The model captures the structural property that a fexpr's body, when invoked, modifies `D`'s state as if the body were inlined at the call site — but the call site is at `I`, not at `D` (the inlining is logical, not lexical). The `D` ceiling discipline ensures the fexpr cannot escape `D`: the fexpr's lifetime is bounded by `D`'s frame retirement.

**Implicit captures by free name.** A fexpr's body references names that are not its own parameters (it has no parameters); those references resolve at invocation time against the defining frame `D`. Each captured access carries the mode of the binding in `D` (§6.10): a `&x` in `D` is reference-accessible from the fexpr body; a READ `x` is read-only; a productive `'x` is PRODUCE-accessible (writes to it through the fexpr count toward `D`'s write-once analysis as if the body were inlined). The fexpr cannot escalate access; it can only do what `D`'s mode-marking already permits.

**Direct captured-slot access (not copy-restore).** Unlike lambda `&` captures, fexpr free-name accesses do not undergo per-invocation copy-restore. The body accesses `D`'s slots directly at each invocation; mutations are applied to `D`'s state in place. The structural justification is that fexprs cannot escape `D` (the ceiling discipline) and `D`'s analysis sees fexpr invocations as inlining points in its own CFG, so the standard frame-local discipline (§6.4) governs without an additional copy-restore layer.

**The `D` ceiling, uniformly.** Every fexpr's ceiling is `D` itself — the defining frame. A fexpr cannot be assigned upward (to a productive or reference parameter of `D` — Restriction G of §8.13), cannot be returned from a constructor (Restriction F), cannot be captured by a lambda (Restriction E), cannot be embedded in long-lived containers (Restriction C), and cannot be referenced by pointer (Restriction B). The ceiling discipline is uniform across all fexpr-typed slots and is structurally enforced by the seven restrictions (§8.13).

**Fexpr-relevance taint composes with the model.** Per §6.15, a fexpr-relevance taint is computed at the fexpr's construction site against `D`'s state graph: any of `D`'s slots whose value may be observed or modified by the body's free-name accesses is fexpr-relevance-tainted in `D`'s analysis. The taint discipline is local to `D`; it does not propagate across frame boundaries (per the local-frame analysis principle of §1.5). The taint flows into the fexpr value at construction time and travels with the value down-stack to invocation sites. At the invocation site (in `I`), no additional analysis state is added — the invocation is treated as inlining of the body into `D`'s analysis at the invocation point, with the body's free-name accesses operating on `D`'s slots through the taint discipline `D` already computed.

### 8.6 Failure-Mark Conformance

Command-typed values carry a failure mark (`:`, `?`, or `!`) as part of their type. The failure-mark conformance discipline governs how command-typed values may be invoked, assigned, and composed.

**Two-sided conformance.** The discipline operates on two sides:

- **Definition-side body conformance.** The body of a command-typed value (whether a command literal, a lambda body, or a fexpr body) must conform to the value's declared failure mark, with conformance measured against the body's **exit paths** (where each reachable execution arrives at exit), not the body's internal operations:
  - A `:`-marked body must reach exit on a non-failing path on every reachable execution. May-fail or must-fail operations are permitted within the body provided every reachable execution path either avoids them or recovers from them before exit; what conformance requires is that no execution can leave the body with a propagating failure.
  - A `?`-marked body may reach exit on either a non-failing path or a propagating-failure path on any given reachable execution. May-fail and must-fail operations are permitted; some paths may recover, others may propagate.
  - A `!`-marked body must reach exit on a propagating-failure path on every reachable execution. Non-failing operations and recovery are permitted within the body provided no path can reach a non-failing exit; every execution must terminate the body with a propagating failure.
- **Invocation-side mark guarantee.** When a command-typed value is invoked, the invocation site sees the value's declared mark and reasons about its own failure-mode conformance against that mark. A `:`-marked invocation is statically certain to succeed; a `?`-marked invocation may fail; a `!`-marked invocation is statically certain to fail.

**Mark-subsumption within each family.** Within the command-typed-value family (command references, command literals, lambdas) and within the fexpr family (fexprs), the failure marks form a **partial order**: `: ⊑ ?` and `! ⊑ ?`, with `:` and `!` mutually incomparable (§4). A `:`-marked or `!`-marked value may stand wherever a `?`-marked value is expected — `?` is the "may-or-may-not" supremum, with `:` and `!` as the two "definitely" specializations of it — but a `:`-marked value cannot fill a `!`-marked slot, nor a `!`-marked value a `:`-marked slot. The two definite-marks make incompatible guarantees; neither can substitute for the other. Subsumption permits the upward substitutions (toward `?`); it does not narrow the analysis at the use site.

**No subsumption across the fexpr / non-fexpr family boundary.** A `:<*>` (never-fails fexpr) is not assignable to a `:<args>` (never-fails command literal) slot, and vice versa, even though both are never-fails. The two families have distinct invocation semantics — fexprs execute in `D`'s state, command literals execute in their own frame — so the type identity is family-specific.

**Per-form conformance rules.** Command references' failure marks come from the underlying command's signature; command literals' from the body's analysis; lambdas' from the body's analysis (with capture-list-derived constraints folded in); fexprs' from the leading mark on the construction surface (`:`, `?`, `!`). Conformance at each construction site checks the body's analysis against the declared mark; mismatches are static errors at the construction site.

### 8.7 Capture-Shadowing

**Lambda permits capture-shadowing.** A lambda body may declare a local of the same name as a captured name; the local shadows the captured name within the body's scope, per the standard lexical-scoping rules of §6.3. The captured name is unaffected outside the body; the lambda's invocation accesses the local where the body declares one and the captured slot where it does not.

**Fexpr forbids capture-shadowing.** A fexpr body's free-name resolution is by name against `D`'s scope; introducing a local of the same name in the fexpr body would create ambiguity at invocation, since the fexpr's body could be re-resolved against `D` (resolving to `D`'s binding) or against the body's local (resolving to the local). The language declines to make this choice ad hoc and forbids the configuration: a fexpr body's local introduction with the same name as any free-name reference in the body is a static error. The user resolves by choosing distinct names.

The asymmetry is structurally grounded: lambdas have explicit captures (the capture list enumerates the names), so the body's locals are clearly distinct from the named captures. Fexprs have implicit captures (any free name resolves against `D`), so the body's locals and the body's free names occupy the same identifier namespace, and shadowing introduces ambiguity the language cannot resolve cleanly.

### 8.8 Context Variables Umbrella

The language has three mechanisms by which a command body may reference state from outside its own parameter list, collectively the **context-variables umbrella**:

- **Implicit context parameters at the call site (§3.6).** A command's signature may declare implicit context parameters after the `/` separator; at the call site, the typechecker locates a value in the caller's lexical scope whose type matches the implicit parameter's type and supplies it as the argument with the implicit's declared mode contract.
- **Lambda explicit captures (§8.4).** A lambda's slash list enumerates names captured at construction; the body references the captured names and the captured access carries the construction-site slot's mode.
- **Fexpr implicit captures by free name (§8.5).** A fexpr's body's free names resolve at invocation against the defining frame `D`'s scope, with mode inherited from `D`'s binding.

The three mechanisms differ in their resolution site:

| Mechanism | Resolution site | Mode source |
| --- | --- | --- |
| Implicit context parameters | Caller's lexical scope (per call) | Declared on the implicit parameter |
| Lambda captures | Construction frame's scope (once) | Declared on the capture-list entry |
| Fexpr captures | Defining frame's scope (per invocation) | Inherited from `D`'s binding |

The mechanisms compose: a fexpr's body may reference names that are themselves implicit context parameters of `D`'s signature (§8.9); a lambda's capture list may bind implicit context parameters of the construction frame at construction time (§8.10).

### 8.9 Fexpr Inheritance of `D`'s Implicit Context Parameters

A fexpr's body may freely reference names that are implicit context parameters of `D`'s signature. The free-name resolution mechanism of §8.5 looks up the name in `D`'s scope; since `D`'s implicit context parameters are bound in `D`'s scope (the typechecker resolved them at `D`'s call site), they are available to the fexpr body the same as any other binding in `D`.

The implication: a fexpr can use `D`'s implicit context parameters without enumerating them, paralleling the way the rest of `D`'s body uses them. The mode of the access is the implicit's declared mode (READ, REFERENCE, or PRODUCE), and the standard fexpr mode-inheritance rule applies.

### 8.10 Lambda Non-Inheritance of `D`'s Implicit Context Parameters

A lambda's body may *not* reference `D`'s implicit context parameters by free name. The lambda's body sees only its own parameters, its capture list, and language-visible names; `D`'s implicit context parameters are not in scope.

The user resolves by **explicitly capturing** the implicit context parameter at the lambda's construction site. If `D` declares `Logger logger` as an implicit context parameter, a lambda within `D` that wants to use `logger` writes `:<args / Logger logger>{body}` — capturing `logger` as a READ capture. The capture list is then explicit about which of `D`'s state the lambda uses, consistent with the lambda discipline of explicit captures only.

The asymmetry across fexpr and lambda — fexpr inherits `D`'s implicit context parameters by free-name resolution, lambda does not — is structurally grounded: fexprs operate as if inlined into `D`, so `D`'s lexical scope is the fexpr's lexical scope; lambdas operate as their own bodies with a separately-tracked capture list, so what's in scope for the body is exactly what's in the capture list (plus parameters and language-visible names).

### 8.11 Re-Entry, Nesting, and Sub-Fexpr Scoping

A fexpr body may itself construct another fexpr (a sub-fexpr) whose `D` is the outer fexpr's body's frame `F`. The sub-fexpr's defining frame is the inner fexpr-execution frame, not the original `D`; sub-fexpr free-name resolution operates against `F`'s scope, which transitively reaches the outer `D` via the standard lexical-scoping chain.

The ceiling discipline tracks correctly: the sub-fexpr's ceiling is the inner `F`, which is bounded by the outer fexpr's invocation, which is itself bounded by the outer `D`'s frame retirement. Sub-fexprs are no harder to track than top-level fexprs; the ceiling is just one level deeper in the call stack.

Re-entry — the same fexpr being invoked multiple times during `D`'s lifetime — is governed by `D`'s static analysis. Each invocation is treated as an inlining point in `D`'s CFG; the analysis computes the cumulative effect across the inlining points and rejects patterns that violate per-path obligations (write-once for productive parameters of `D`, etc.). The discipline is uniform with how `D`'s body's straight-line code is analyzed.

### 8.12 Excluded Combinations

The umbrella rule: fexprs travel exclusively *down-stack* from `D` — passed as arguments to callees, captured implicitly by sub-fexprs, transitively reached from `D`'s state — and never up-stack. Every constructional surface that could let a fexpr escape `D` is structurally excluded by the seven fexpr restrictions of §8.13. Specifically:

- **Lambdas cannot capture fexpr-typed slots** (Restriction E of §8.13). Lambdas may outlive their construction frame through ceiling-flattening of reference captures; admitting a fexpr capture would create a lambda value that could outlive its captured fexpr's `D`. The lambda capture mechanism does not currently propagate fexpr-relevance taint, so the exclusion is structural.
- **Pointers to fexpr-typed slots are forbidden** (Restriction B). A `^F` (where `F` is a fexpr type) would be a value that could outlive the fexpr's `D`; the type form is rejected at the type-system level.
- **Fexpr-typed values cannot be returned from constructors** (Restriction F). A constructor's productive output is the standard upward-migration channel; a fexpr-typed productive output would migrate the fexpr from the callee's frame to the caller's slot, violating the `D`-ceiling.
- **Fexpr-typed values cannot be assigned to writeable parameters of `D`** (Restriction G). A productive or reference parameter of `D` is `D`'s caller's slot (per the frame-ownership lens, §1.5); writing a fexpr to such a parameter would expose the fexpr to `D`'s caller, violating the `D`-ceiling.
- **Fexpr-typed values cannot be embedded in long-lived containers** (Restriction C). Object fields, record fields, and non-local-slot variant candidates of fexpr type are forbidden, since the container can outlive the fexpr's `D`.

**One admitted combination — command references binding fexpr-typed arguments — under the fexpr-tainting discipline of §6.15.** A command reference of the form `{cmd: ..., my_fexpr, ...}` is well-formed when the fexpr-relevance taint of `my_fexpr` is propagated to the resulting command-typed value. The taint marks the command-typed value as carrying a `D`-ceiling constraint inherited from the bound fexpr; the value cannot be moved beyond `my_fexpr`'s `D`. The fexpr-tainting machinery makes this binding tractable: the ceiling constraint travels through the partial-application boundary on the command-reference value, and the typechecker enforces the constraint at every assignment, store, or move involving the resulting value.

If the typechecker subsequently shows that the fexpr-tainting machinery is insufficient to admit other combinations, those combinations remain structurally excluded; the language admits exactly the combinations whose ceiling constraints can be carried by the existing tainting machinery, and no others.

### 8.13 The Seven Fexpr Restrictions

The seven restrictions govern fexpr-typed slots and fexpr-typed values throughout the language. Each restriction prevents a specific channel by which a fexpr could escape its defining frame `D`.

- **Restriction A — No productive or reference fexpr-typed parameters.** A parameter of fexpr type may carry only the READ mode marker (or no marker, equivalently). The productive `'` and reference `&` markers are forbidden on fexpr-typed parameters. *Rationale:* a productive fexpr-typed parameter would be an upward-migration channel (the callee writes a fexpr the caller receives); a reference fexpr-typed parameter would alias a fexpr slot in the caller, a slot that may not even exist in the caller's frame layout. The READ mode is the only mode that admits a fexpr — the parameter is a copy of the fexpr value at the call site, and copies of fexpr values track the same `D` as the source.

- **Restriction B — No pointers to fexpr-typed slots.** The type `^F` where `F` is any fexpr type is forbidden. *Rationale:* pointers can outlive the slots they reference (subject to standard pointer-validity discipline), and a `^F` would be a value that could outlive the fexpr's `D`. The exclusion at the type-system level prevents the situation from arising.

- **Restriction C — No fexpr fields or candidates in containers.** Object fields, record fields, and variant candidates may not have fexpr type. *Rationale:* objects and records can be moved across frames or stored in long-lived containers; a fexpr field would migrate with the container, escaping its `D`. Variants are subject to the same exclusion (§9.21): a variant slot's lifetime can exceed the lifetimes of values stored at its candidates, making fexpr candidates incompatible with the variant's potentially-longer lifetime.

- **Restriction D — No bare-identifier copy of fexpr values.** The bare-identifier `<-` form (`# f2 <- f1` where both have fexpr type) is forbidden. *Rationale:* the construction site (`# f <- :{...}`) is the only valid initialization for a fexpr-typed slot; subsequent migration via bare-identifier copy would create a second fexpr-typed slot whose `D` may differ from the source's, and the language does not currently track per-copy `D` provenance. The restriction confines fexpr-typed slots to direct construction.

- **Restriction E — No fexpr captures by lambdas.** A lambda may not capture a fexpr-typed slot, whether by READ copy or by `&` reference. *Rationale:* lambdas may outlive their construction frame through ceiling-flattening; admitting a fexpr capture would create a lambda value that could outlive its captured fexpr's `D`. The lambda capture mechanism does not currently propagate fexpr-relevance taint, so the exclusion is structural.

- **Restriction F — No fexpr from a constructor's productive output.** A constructor cannot produce a fexpr-typed value via its productive `'r` parameter. *Rationale:* a constructor's productive output is the standard upward-migration channel — the callee constructs a value, the caller receives it. A fexpr produced this way would migrate from the callee's defining frame to the caller's slot, violating the `D`-ceiling. Fexprs are exclusively created as literals in their defining frame and travel only downward.

- **Restriction G — No fexpr written to defining-frame writeable parameters.** A fexpr cannot be assigned to any productive or reference parameter of `D` itself. *Rationale:* a productive or reference parameter of `D` is `D`'s caller's slot (per the frame-ownership lens, §1.5); writing a fexpr to such a parameter would expose the fexpr to `D`'s caller, violating the `D`-ceiling. The fexpr is `D`-bounded — passed down-stack only, never assigned up-stack directly or transitively.

**Transitive application to fexpr-relevance-tainted values.** The seven restrictions apply not only to values of fexpr type but also to **fexpr-relevance-tainted** values — values that have inherited a `D`-ceiling constraint via the fexpr-tainting discipline of §6.15. The principal case is a command-reference value that binds a fexpr-typed argument (§8.12): the resulting `:<...>` value is tainted and carries the bound fexpr's `D`-ceiling. Such a tainted command-reference is, for the purposes of the seven restrictions, treated as if it were a fexpr-typed value — it cannot be a productive or reference parameter (A), cannot be pointed to (B), cannot appear in an object field, record field, or variant candidate (C), cannot be bare-identifier-copied (D), cannot be captured by a lambda (E), cannot be a constructor's productive output (F), and cannot be written to a writeable parameter of its `D` (G). Any operation that would be illegal for a fexpr-typed value is equally illegal for a fexpr-relevance-tainted value of any underlying type.

The seven restrictions are jointly necessary for fexpr ceiling-tracking to be sound under static analysis. Each restriction closes a channel by which a fexpr could escape `D`; absent any one of them, the soundness argument requires per-channel reasoning that the language declines to undertake. The collected restrictions are conservative — some channels closed by them might admit a more nuanced rule under additional analysis machinery (the §6.15 fexpr-tainting axis is one such direction, applied selectively in §8.12 to admit command-reference fexpr-typed bindings).
---

## 9. Classes, Instances, and Dispatch

Classes are Basis's mechanism for declaring **type contracts** — named interfaces that a type may declare to satisfy, with each satisfaction providing concrete implementations of the contract's required commands. Classes are not constructors and not classes-in-the-OOP-sense; the language has no class-level inheritance, no virtual methods on instances, and no notion of "instance state" tied to a class. A class is a static declaration of "what a type must support to be a member"; an instance is a static declaration of "this type supports this class, by way of these implementations."

This section specifies the class-and-instance declaration surfaces, the single-implementing-type discipline, the dispatch resolution sequence, the witness-slot model that backs dispatch at runtime, the coherence rules that govern multiple competing instance declarations across modules, and the interactions with overloading, partial application, buffer-backed-type identity, variants, and fexprs.

### 9.1 Classes

A class declaration introduces a contract that types may declare to satisfy. The form is:

    .class C : signature-list
    .class C[type-parameters] : signature-list

where `C` is the class name (uppercase-initial), the optional bracket form declares zero or more type parameters of the class, and `signature-list` is the sequence of command signatures the class requires its members to provide. Each signature may include a default body (making it `.cmd` form) or omit one (making it `.decl` form, signature only); the default body, when present, is used as the implementation for any instance that does not provide its own.

Examples:

    .class Eq : ...                              ; no type parameters
    .class Set[T] : ...                          ; one type parameter T
    .class Map[K, V] : ...                       ; two type parameters K, V
    .class Container[T:Itemable] : ...           ; one type parameter T, bounded to satisfy Itemable

Each type parameter may carry a **bound** — a class-constraint of the form `:Bound` after the parameter name — requiring that any binding of the parameter satisfy `Bound`. The class's signatures may reference the class's type parameters at any type position; bounds make available the class-method calls of the bound class on the parameter's value.

**Parameterized class names cannot be referenced bare.** `Container` alone is a static error; only `Container[A]`, `Container[B]`, etc. are valid type references. Basis does not perform Java-style type erasure: `Container[A]` and `Container[B]` are distinct types unless `A` and `B` are the same type.

**Class names used as types are existentially qualified per appearance.** When a class name (parameterized or not) appears as a parameter type in any signature, each appearance is *independently* existentially qualified — the appearances do not co-vary. Consider two signature shapes for a hypothetical standalone command:

    .decl equals: Eq a, Eq b                     ; (a)  existential per appearance
    .decl equals: (T:Eq) a, T b                  ; (b)  bound type variable

These mean different things. (a) admits `a` and `b` of any two types that each satisfy `Eq` — possibly different types from each other; the only constraint is that each individually satisfies `Eq`. (b) introduces a type variable `T` constrained to satisfy `Eq` and requires `a` and `b` to be of the *same* type. The two forms are the surface mechanisms for the existential and the bound-type-variable styles respectively, formalized as Case B and Case A at §9.9.

A class is **not final**: a class may be extended by adding new instance declarations at any module that imports it. The no-final discipline matches the open-world nature of typeclass-style mechanisms; a library that defines a class admits instance declarations from downstream modules without coordination.

### 9.2 Combined Classes

A **combined class** declaration names the conjunction of two or more existing classes, optionally adding signatures of its own. The forms are:

    .class CombinedC : C1, C2                ; pure conjunction; no new signatures

    .class CombinedC : C1, C2                ; conjunction plus additional signatures
        .decl method1 : ...
        .cmd method2 : ... = ...

The combined class declares that any type satisfying `CombinedC` must satisfy `C1`, must satisfy `C2`, and must provide implementations for any signatures declared in the combined class's own body. The combined-class form is a convenience over writing the two parent-class constraints separately at every parameter position — a single name `CombinedC` replaces the longer constraint repetition at use sites — and is the surface for declaring a class that extends multiple existing contracts simultaneously.

A combined class may carry type parameters of its own, like any class: `.class OrderedMap[K, V] : Ord[K], MapLike[K, V]` declares a combined class with two type parameters (`K`, `V`) whose instances satisfy both `Ord[K]` and `MapLike[K, V]`. The type parameters of the combined class are bound at instance declaration time and propagated to each parent class's parameters at the matching positions.

**Implicit inhabitation for pure-conjunction combined classes.** When a combined class declares no own signatures (pure conjunction), any type satisfying all the parent classes is *automatically* an instance of the combined class. No explicit `.instance T : CombinedC` declaration is required; the combined-class membership is derived by the typechecker from the type's existing parent-class instances. Unlike explicit instance declarations whose dictionary is finalized at declaration time (§9.5), implicit-inhabitation membership is checked at each use site: when the typechecker encounters a type `T` at a `CombinedC`-typed position, it verifies that `T`'s parent-class instances are visible at the use site's module import graph.

**Explicit instance required when the combined class adds signatures.** When the combined class declares one or more own signatures, implicit inhabitation is *disallowed*: an explicit `.instance T : CombinedC` declaration is required. The implementations come from the standard channels (§9.4): top-level methods on `T` matching the combined class's own signatures, defaults declared in the combined class itself, or delegation. The parent-class parts of the dictionary continue to be picked up from the type's existing parent-class instances per the co-location rule (§9.5).

The split is structurally clean: implicit inhabitation works when the combined class's contract is fully determined by its parents — every method the combined class requires has an implementation through one of the parents. When the combined class adds its own signatures, those need bodies that cannot be derived from the parent-class implementations alone, so an explicit instance is needed to provide them.

**Witness construction.** A combined class's witness is a **single pointer** to a dictionary, the same form as any class witness (§9.7). The dictionary the pointer references is composed at compile time from the parent-class dictionary entries for the same type (and, in the with-signatures case, the combined class's own method bodies). The composition is performed once per `(type, combined class)` pair the program uses; the typechecker locates the type's parent-class instance dictionaries and synthesizes a combined-class dictionary covering the union of methods. Dispatch on a combined-class value through any of the inherited methods proceeds through this single combined-class dictionary directly; the spec does not prescribe the dictionary's internal layout (a flattened inlining of parent entries, a composition with parent-dictionary pointers, or another structure), only that the runtime witness is a single pointer regardless of the layout.

The combined-class form composes recursively: a class composed of `C1, C2, C3` declares a member type must satisfy all three; combinations of combined classes work the same way, with the conjunction flattening at the witness-composition step. The implicit-inhabitation rule applies at each level of composition: a pure-conjunction combined class chains implicit inhabitation through its parents.

### 9.3 Class Type Parameters

A class's type parameters are declared in brackets attached to the class header:

    .class Set[T] : ...                          ; one type parameter, no bound
    .class Log[T:Serializable] : ...             ; one type parameter, bounded
    .class Map[K, V] : ...                       ; two type parameters

Each type parameter may carry a class-bound (`:Bound`) requiring any binding to satisfy the named bound. The bound is scoped to the class's signatures: a method that uses a bounded type parameter at a value position relies on the bound for any class-method calls on that parameter's value.

The brackets are part of the class's identity. As §9.1 stated, parameterized class names cannot be referenced bare — `Container` alone is a static error; `Container[Int32]` and `Container[String]` are distinct types, the parameter bindings part of the type's identity.

The bracket-form `[T:Bound]` for class type parameters is **distinct** from the inline-form `(T:Class)` used at command signatures (§9.9, Case A). The two surfaces are not interchangeable:

- Bracket form attaches to the class header and is scoped class-wide: all signatures in the class body share the same type parameter binding when an instance is declared.
- Inline form attaches to a standalone command's parameter list and is scoped to that single signature: the type variable lives only within that one command.

Mixing the two surfaces — using inline form on a class method, or bracket form on a standalone command — is rejected with a static error. The two forms exist because they serve structurally different cases: class-wide type parameters are shared across all the class's methods (the bracket-form scope), while standalone-command type variables are local to a single signature (the inline-form scope).

**A type variable may be introduced only once in any given signature.** The form `(T:C1, T:C2)` — repeating the same type variable name with two class bounds — is a static error. The surface admits a single bound per type variable introduction; multi-class constraints on a single type variable are expressed via combined classes (§9.2): the user declares `.class CombinedC : C1, C2` and writes `(T:CombinedC)` at the inline position. The single-bound-per-introduction rule keeps the inline form minimal — `(T:Class)` is a constraint-on-T-by-the-named-class, not a mini-language for arbitrary type-variable predicates.

### 9.4 Instances

An instance declaration declares that a type satisfies one or more classes. The form is:

    .instance T : C1, C2, ...

where `T` is the **implementing type** (the type being declared to satisfy the listed classes) and `C1, C2, ...` is one or more classes (or combined classes). The instance declaration is a *membership statement*: it carries **no body** — no signatures, no implementations. Its purpose is to state that `T` satisfies the listed classes. At declaration time, the typechecker validates that every implementation the listed classes require is visible per the co-location rule (§9.5); if any required implementation is missing, compilation fails with a diagnostic naming the missing methods.

The actual class-method implementations come from elsewhere. Methods in Basis are top-level items, declared in their own modules:

    .cmd MyType :: methodName : params = body

A `.cmd T :: methodName` form declares a method on `T` at the top level of the containing module. The typechecker reads this declaration as "this is one of `T`'s methods" — independently of any instance declarations. When an `.instance T : C` declaration is later made (or already exists), the typechecker matches `T`'s top-level methods against `C`'s required class methods by name and signature. The matching is by name; the implementation is the one already declared at top level.

A single instance declaration may list any number of classes; each class produces a separate dispatch entry for the type. The form `.instance T : C1, C2, C3` produces three dispatch entries: `(T, C1)`, `(T, C2)`, `(T, C3)`.

For parameterized classes, the type-parameter bindings are specified in brackets on the right side:

    .instance MyType : Eq
    .instance MyList : Set[Int32]
    .instance MyList[T] : Set[T]
    .instance MyMap[K, V] : Map[K, V], Ord[K]

The pair `(T, C[bindings])` — implementing type with each class's parameter bindings — fully determines a dispatch entry. Two instances of the same class with different parameter bindings are distinct entries; `.instance MyList[Int32] : Set[Int32]` and `.instance MyList[String] : Set[String]` produce distinct dispatch entries.

**Delegation.** A class in the instance declaration may be marked with a `(delegate fieldName)` clause indicating that the class's methods on `T` are obtained by delegating to a field of `T` whose own type already satisfies the class:

    .instance T : C1 (delegate fieldA), C2, C3 (delegate fieldC)

The delegation clause is per-class: any subset of the listed classes may carry their own `(delegate ...)` clause. In the example above, `C1` and `C3` are delegated to `fieldA` and `fieldC` respectively; `C2` is satisfied through `T`'s own top-level methods (or `C2`'s defaults). Mixing delegated and non-delegated classes in a single instance declaration is admitted.

The delegation surface is intentionally **receiver substitution**, not implementation forwarding. The distinction matters: under receiver substitution, the body of each delegated class method is the *delegated type's* implementation, not a thin wrapper that calls through. Each class method's receiver of type `T` is substituted with the field's value (of the field's type), and the field's type's existing instance for the delegated class is dispatched through. Optimization, inlining, and call-graph analysis observe the delegated implementation directly; there is no intermediate forwarding frame.

A type may declare instances for many classes across many `.instance` declarations; each declaration contributes one or more `(T, C[bindings])` dispatch entries. The set of all such entries — across all modules — is the dispatch table the typechecker resolves against.

### 9.5 The Co-Location Rule

Every `.instance T : C` declaration must, at the point of declaration, produce a **complete dictionary** for `C` on `T`. The check depends on whether the instance declared a `(delegate fieldName)` clause for the class:

- **With delegate clause:** The delegate field's type must have a complete instance for `C` visible at the declaration site. If yes, all of `C`'s methods on `T` are satisfied via the delegate; if no, compilation fails with no fallback to other channels. The explicit delegate is the user's stated source, and the co-location check verifies that source alone.

- **Without delegate clause:** Each of `C`'s required methods must be satisfied by one of two channels:
  - **A top-level method declared on `T`** (or on `T`'s parent chain for buffer-backed types) whose name and signature match the class method.
  - **A default body declared in `C`** for that method.

  Any method that is satisfied by neither channel causes compilation to fail with a static error naming the missing method.

In-scope visibility at the declaration site is what matters: an instance declaration in module `M` produces its dictionary by looking up implementations through `M`'s import graph at declaration time, not at use sites.

Delegation may chain across instances: if the delegate field's type's instance for `C` is itself a delegating instance, the resolution walks the chain at compile time. A method is missing only if no provider exists anywhere along the chain. The "no fallback" rule applies to the outer instance only — within a delegate's own instance, the standard rules apply (delegate-or-not, then top-level method + default).

The co-location rule ensures that an instance, once declared, has a complete dictionary fixed at that point — the dictionary is not reconstructed at each use site and does not depend on the use site's import graph.

### 9.6 The `::` Scope Operator

The `::` operator has four roles in Basis, distinguished by the kind of name to its right and the kind of value to its left:

- **Class-method resolution on a receiver.** `receiver :: methodName` resolves `methodName` against the class system: the receiver's type's instance dictionaries are consulted, and the dispatch is performed. This is the dispatch surface that drives the class-and-instance system (§9.12).

- **Field-member access on aggregate-shaped values.** `value :: fieldName` or `value :: N` selects a field from an aggregate-shaped value — an `Aggregate`-typed slot (§7.4), a record (§5.4), or an object (§5.11). For records, both forms account for `.inline` field promotion (§A.7): named promotion makes inner-record field names accessible directly, and positional promotion splices the inner record's positions into the outer's sequence. For objects, positional access is by declaration order (object layout is implementation-determined).

- **Namespace and module resolution.** `ModuleName :: name` or `ClassName :: methodName` selects a name from a namespace (a module, or a class as a namespace for its declared methods). The namespace resolution is static; no dispatch is involved.

- **Partial-application bake-in** at command-reference construction (§8.2). `{receiver :: methodName}` produces a command reference that binds the receiver at construction time; subsequent invocations skip the per-call dispatch lookup. This is the canonical "tight-loop optimization" form.

The four roles share the `::` token but resolve to distinct operations at the typechecker. Disambiguation is by the syntactic context: at a value-position with a receiver to the left, role 1 (dispatch) or role 4 (partial-application bake-in, when inside a `{...}` reference); at a value-position with an aggregate-shaped value (record, object, or `Aggregate`-typed slot) to the left, role 2 (field access); at a name-position with a module or class to the left, role 3 (namespace). The typechecker resolves each `::` occurrence per these rules without ambiguity.

### 9.7 The Witness-Slot Model

Class dispatch in Basis is backed by a **three-layer composition**:

- **The dictionary.** A record-like value of command-typed values — one entry per class method. The dictionary is constructed at compile time from the instance declaration; each entry is a command-typed value that wraps the method's implementation for the instance's type.
- **The hidden-parameter witness (Case A).** For class-bounded type-variable parameters, the dictionary flows once per call as a hidden parameter. The witness identifies which instance the call site uses.
- **The 3-word slot (Case B).** For existential class-typed parameters, the dictionary is captured at the construction site of the value and travels with it as a 3-word slot — the same 3-word slot pattern used for variants (§5.12) and failures (§4).

The two cases (A and B) are the substantive distinction in how class-typed parameters appear at the type-system level; the dictionary structure is uniform across them.

**The witness is always a single pointer.** Regardless of how the dictionary is composed internally — a simple record for single-class instances, a composition of parent dictionaries for combined classes (§9.2), or other implementation-specific structure — the language-visible witness is one pointer. The single-pointer commitment is what enables the uniform 3-word slot pattern (Case B) and the hidden-parameter passing form (Case A); both flow patterns rely on the witness being a single referentially-transparent value the runtime can copy or pass without composition awareness. The spec does not prescribe the dictionary's internal layout; it does prescribe that any witness, regardless of underlying complexity, is a single pointer at the language-visible boundary.

### 9.8 The Dictionary

A class `C` with `k` commands has a dictionary type of `k` command-typed entries, each entry a command-typed value of the corresponding class signature with the implementing type and any class type parameters resolved per the instance's bindings. For an instance `.instance T : C` (or with bindings: `.instance T : C[bindings]`), the dictionary is constructed at compile time according to whether the instance declares a delegate clause for the class.

**With a delegate clause `(delegate fieldName)`:** all dictionary entries for the class are constructed as receiver-substitution wrappers, each routing the corresponding class method to the delegate field's type's existing instance for the class. There is no per-method fallback to other channels — the explicit delegate is the sole source for every method of this class. The co-location rule (§9.5) ensures the delegate field's type's instance is complete before the dictionary is constructed; if the delegate's instance for the class is missing or incomplete at the declaration site, compilation fails with no recourse to other channels.

**Without a delegate clause:** the dictionary is constructed per method, with the following precedence:

- If `T` has a top-level method declared (on `T` or on `T`'s parent chain for buffer-backed types) whose name and signature match the class method, the dictionary entry wraps that method. Method matching is by name, with the class method's implementing-type and type-parameter positions resolved per the instance's bindings before comparison.
- Otherwise, if `C` declares a default body for this method, the dictionary entry wraps the default. The default's type variables are resolved per the instance's bindings before the entry is constructed.

The two channels are consulted in this order per method: top-level method on `T` first, then class default. A method present at both channels takes the top-level method on `T` (the more specific source); the default applies as a fallback only when no top-level method matches. The co-location rule ensures every method falls into one of these two channels; otherwise compilation has already failed at the `.instance` declaration.

The dictionary is **immutable** once constructed; instance declarations do not produce mutable state. The dictionary is referenced at dispatch sites by pointer; the witness mechanism (Case A or Case B) determines which dictionary the dispatch uses.

### 9.9 Cases A and B

A class-typed parameter appears in two distinct forms at command signature positions:

**Case A — type-variable bound.** A type variable `T` is declared with a class bound at the first occurrence's parameter position, and subsequent parameters refer to the same `T`:

    .cmd compare: (T:Ord) 'r, T a, T b = ...

The command takes a productive `T` slot `r` and two READ `T` values `a`, `b`; `T` is a single type that the caller picks at the call site by supplying same-type arguments. The `(T:Ord)` form at the first parameter position introduces `T` and constrains it to satisfy `Ord`; subsequent uses of `T` refer to the same type variable. The witness for `Ord` flows once per call as a hidden parameter — the dictionary is passed at the call boundary alongside the visible arguments. Inside the body, `T` is the same type for both `a` and `b`; the dispatch via `a :: someOrdMethod` uses the witness flowed in.

The inline `(T:Class)` form is the standalone-command surface for introducing a class-bound type variable. Class headers use the bracket form `[T: Bound]` (§9.3) instead — brackets attach to the class as a whole, scoped across all the class's signatures; the inline form attaches to one signature's first occurrence.

The natural slot representation under Case A is an **ordinary value** of type `T` — no extra runtime metadata is needed at the slot level, since the witness lives in the hidden-parameter channel. Slot layout is the same as it would be for an unparameterized `T`-typed parameter.

**Case B — existential at parameter position.** A class name appears directly as a parameter type, without a type variable:

    .cmd describe: Ord o = ...

The command takes a value of class `Ord` — any type that satisfies `Ord` may be supplied. Different call sites may supply different types; the same call site may supply different types at different invocations. The witness for `Ord` cannot flow once per call (there is no single `T` for the call); instead, the dictionary travels with the value as part of the value's runtime representation.

The natural slot representation under Case B is a **3-word slot**:

    word 1: tag identifying the runtime type of the held value
    word 2: pointer-to-value (the value's data)
    word 3: pointer-to-dictionary (the witness)

The 3-word slot is the same structural pattern as the variant slot (§5.12) and the failure slot (§4); each of the three uses populates the words per its semantics (a variant's absent state leaves all three null; a failure's `clear` state leaves all three null; a Case B slot in a well-initialized state has all three populated). The uniform pattern means a Case B class-typed value has the same storage layout as a variant or a failure — three words, populated at construction, copied as a unit.

**The substantive distinction.** Case A is "the parameter takes a value of a *specific type* the caller chooses, constrained to satisfy a class." Case B is "the parameter takes a value of *any type that satisfies the class*, with different invocations potentially using different types." The difference shows in slot representation (ordinary vs. 3-word), in witness flow (hidden-parameter vs. embedded), and in callee body's static knowledge (`T` is one type across the call vs. unknown type per invocation).

### 9.10 Bidirectional Existentials Under Case B

Case B applies symmetrically to input and output positions of a command signature. A productive parameter at class type is **as much an existential** as a READ parameter at class type:

    .cmd produceSomeOrd: Ord 'r = ...

The callee constructs a value satisfying `Ord` and supplies it; the caller receives a 3-word slot whose runtime type may be any class member. Subsequent uses by the caller dispatch through the embedded witness exactly as with a READ existential.

The symmetry is structural — the 3-word slot is the same in both directions; the witness population timing is the only difference (at construction for productive output; at call binding for READ input). Both directions use the same dictionary lookup and the same 3-word slot copy-as-a-unit discipline.

### 9.11 RTTI

The language's class-typed slots carry **runtime type information** (RTTI) sufficient for dispatch — the tag-or-discriminant in the 3-word slot identifies which instance the value belongs to. RTTI is **implementation-internal**: it is consulted by the dispatch machinery and by the `-<` operator (§7.14, §9.18) when narrowing through class hierarchies, but it is not directly programmer-visible. There is no `typeOf` operator or RTTI-query surface in the language.

The narrowing operator `-<` on a class-typed slot inspects the RTTI to determine which candidate-or-parent the runtime value matches; the operator's grammar and semantics are in §7.14. The dispatch resolution sequence (§9.12) uses the witness, not the RTTI directly — the witness already incorporates the type identity at dispatch time.

### 9.12 Single-Class Dispatch

Resolution of a class-method call proceeds in four steps:

1. **Overload resolution.** Identify the candidate command(s) at the call site, applying the three-layer overloading rules (§9.16) to disambiguate against same-name candidates in scope.
2. **Dispatch type identification.** Determine which type's instance is being dispatched against — for a `receiver :: method` form, the receiver's static type at the call site; for a Case B parameter, the runtime tag in the 3-word slot.
3. **Dictionary lookup.** Locate the dictionary for the `(class, type)` pair identified in step 2. For Case A, the dictionary is flowed in as a hidden parameter. For Case B, the dictionary is the third word of the 3-word slot.
4. **Invoke.** Call the command-typed value at the dictionary's entry for the resolved method.

Witness flow rules per case:

- **Case A:** the witness flows as a hidden parameter on entry to the command's frame. Subsequent dispatches inside the body that reference the same type-variable bound use the witness already in scope; no further flow is needed.
- **Case B:** the witness is embedded in the 3-word slot. Dispatch reads the dictionary pointer from the slot; subsequent dispatches on the same slot reuse the same pointer.

Cross-case combinations follow uniformly: a Case A type variable bound to a class may flow its witness through to a sub-call that is itself Case A on the same bound. A Case B value passed to a sub-call that takes a Case B parameter copies the 3-word slot unchanged. A Case A receiver dispatched to a method whose result is Case B at the same class produces a 3-word slot at the caller's slot.

### 9.13 Multiple Dispatch Over Receiver Tuples

Methods with multiple receivers (multi-receiver methods, §6) participate in dispatch through the standard single-class dispatch mechanism, applied independently to each receiver. The language has no joint-instance dictionary keyed on receiver tuples: dispatch is **composed** out of single-class dispatches, not implemented as a multi-key lookup.

A method `.cmd (A a, B b) :: methodName : ... = body` with two receivers `a: A` and `b: B`, when each receiver type is class-typed (e.g., `(C1, C2)`), dispatches by:

1. Resolving `a`'s instance for `C1` via single-class dispatch on `a`.
2. Resolving `b`'s instance for `C2` via single-class dispatch on `b`.
3. Composing the resolved methods according to the method's body declaration.

The composition is at the language-surface level — the method's declared body sees `a` and `b` as ordinary receivers, and any dispatches within the body proceed through their respective witnesses. No higher-order dispatch on `(A, B)` as a pair is performed; the typechecker enforces that the method's signatures at each receiver position match the class declarations independently.

### 9.14 Partial Application Beyond Receiver-Elision

The receiver of a method reference is always specified — receiver-binding via `::` is mandatory for method references (§8.2). The non-receiver parameters of the method may, at the call site or at command-reference construction time, be partially applied or deferred according to the standard partial-application discipline (§8.2). The combined form is `{receiver :: methodName: x, _, y}` — receiver bound via `::`, positional parameters bound via `: arg, ..., _, ...`, leaving `_` positions open for subsequent invocations.

**Mode-marker filter.** The same mode constraints from §8.2 apply: PRODUCE positions cannot be bound (deferred only); REFERENCE positions, if bound, capture a slot with ceiling tracking; READ positions are flexible — bound or deferred at the user's choice.

**The `_` deferral marker.** A positional `_` in the partial-application surface marks "this position remains open." A subsequent invocation supplies the value for that position. The marker is uniform with the `_` used in non-method command references and elsewhere in the language.

**Why receivers must be applied.** A method's signature lists the receiver as a structural component of its parameter list. A partial application that omits the receiver leaves a structurally incomplete reference — the method cannot be dispatched without knowing which instance to use, and the dispatch lookup is part of the method-reference machinery. The mandatory receiver binding closes this hole: every method reference has its receiver determined at construction.

**Unambiguity under overload resolution.** When the underlying command name is overloaded (§9.16), the partial application's supplied positions — the receiver and any bound positional arguments — must unambiguously identify a single overload. If multiple overloads remain compatible with the supplied positions (including the `_`-deferred positions, whose types must be deducible at construction), the partial application is a static error. The user resolves by either supplying additional positions until the overload is unique or by qualifying the reference explicitly.

**Uniformity through `(T:C)` constraints.** When the method's receiver is class-bounded — `(T:C) self :: ...` — receiver-elision via `{x :: methodName}` produces a command reference whose visible signature has the type variable resolved to the static type of `x`. The mechanism by which the class witness flows — Case A as a hidden parameter from the enclosing frame, Case B embedded in the receiver's 3-word slot (§9.12) — is implementation-internal and does not affect the visible signature. The two cases produce command references of the same shape; ceiling-tracking treats the command reference as bound by whichever lifetime is more restrictive (the captured receiver's slot, or the witness's source frame).

### 9.15 Instance Coherence

When more than one instance declaration of `(T, C)` is in scope at a call site, the typechecker applies the coherence rules:

- **Intra-module duplicates are static errors.** Two `.instance T : C` declarations in the same module are rejected at module-compile time. The user resolves by removing one declaration.
- **Cross-module duplicates are ranked by module specificity** under the "more specialized module wins" rule. The most-specific module's instance is selected; if multiple modules are equally specific (incomparable), the dispatch is ambiguous and is rejected with a static error.
- **Orphan instances are permitted.** A module may declare `.instance T : C` for types `T` and classes `C` that originate in other modules — the instance does not need to be in the module of `T` or `C`. The permissiveness composes with the specificity ranking: an orphan instance loses to a more-specific non-orphan, but wins over a less-specific non-orphan.
- **Import-time competition warning.** When a module is imported and brings into scope an instance that competes with an instance already in scope (at the import boundary), the compiler emits a warning identifying both candidates. The warning surfaces the coherence collision for the user without blocking the import; the actual dispatch uses the more-specific instance per the ranking.

The full algorithm for module specificity ranking — including how "module hierarchy" is structured for the comparison — is in Appendix H.

### 9.16 Overloading on Dispatched Commands

Three layers of overloading apply to dispatched commands:

- **Within-class overloading.** Multiple class methods within a single class may share a name when their signatures differ in argument shape; the typechecker disambiguates at the call site by argument types.
- **Cross-class overloading.** Two distinct classes may each declare a method with the same name. If a single type satisfies both classes, calls to that name on a value of that type are ambiguous unless disambiguated.
- **Non-dispatched overloading.** Ordinary `.cmd`-declared commands and class methods may share a name when their declared types disambiguate at the call site.

The resolution rule is **most-specific-candidate-wins**: among all candidates matching the call's argument shape, the candidate whose declared signature most-specifically matches is selected. Ties — two candidates equally specific — are rejected at the call site with an "ambiguous call" error; the user resolves by adding a type annotation or by using the `{C::method}` disambiguation form.

**The `{C::method}` disambiguation form** is the surface for naming a class-method by its class explicitly. The form is a command reference (§8.2) whose left operand of `::` is a class name (not a value):

    {Ord::compare}

This produces a command-typed value that is unambiguously the `compare` method from class `Ord`. The value can then be invoked or stored as any other command-typed value. The form is especially useful at cross-class overloading boundaries where the type-driven disambiguation cannot decide between candidates from different classes.

### 9.17 Liskov-Style Opening of the Failure-Message Hierarchy

The failure-message hierarchy (§4) admits class-system-style opening — a payload class may extend an existing payload class, with subsumption rules governing how a child class may be raised at a parent's expected position. The rule is **Liskov covariance**: a payload whose declared class is a subclass of (or equal to) the position's expected class is well-formed at that position. A `.fail` site emitting a payload satisfying a child payload class is acceptable at any position expecting a parent class along the same class hierarchy; the dispatch flows through the more-specific class's witness, and class operations expected at the parent's level are dispatched soundly by the standard class-system subsumption (a child-class witness has, by construction, all the operations of every parent class in its chain).

The covariance rule is what makes payload-class covariance across message hierarchies (§4.8) work end-to-end: a child message's payload class is a subclass of (or equal to) its parent's, so the parent-class handler's bound payload value — observed through the parent's class operations only — is operating on a value whose actual class supports those operations and more. The handler does not see the more-specific class's operations: the bound name's static type is the spec's class, and the handler is constrained to operate at that level of abstraction. This is the design commitment that lets recovery handlers participate in extensible message hierarchies without coupling to concrete payload-type details.

The bright-line rule is the same one that governs instance coherence (§9.15): intra-module uniqueness, cross-module specificity ranking, orphan permissibility, import-time competition warning. The rules are stated once and applied uniformly to instances and to failure-message hierarchies.

### 9.18 Buffer-Backed Dispatch Identity Capture

When a buffer-backed value (a value of a domain, record, union, or buffer primitive — §5) passes through a class-typed slot at any point in its lifetime, its dispatch identity is captured at that boundary. The captured identity is the child-type identity from before the upcast — the specific named domain or record, not the upcast type the slot's parent chain reaches.

The capture point is the **first class-typed slot the value occupies** along the flow path. At any later slot, the value's dispatch is determined by the previously-captured identity; subsequent transits through ordinary buffer-backed parameters do not re-capture identity.

**Identity is lost when the value passes through an intermediate non-class-typed parent buffer-backed parameter.** If a `Point` (record over `[8]`) is passed to a parameter of type `[8]` (the parent of its layout), the parent parameter does not capture or carry the `Point` identity; the value is observed as `[8]`-typed by that parameter, and any later class-typed slot the value enters captures whatever identity is available at that point — which is `[8]`, not `Point`.

The motivating use case is class dispatch on values that flow through generic code: a class-typed receiver knows the value's specific identity (a `Point`, a `Date`, etc.), and the methods declared on that identity are dispatched correctly. Generic code that operates on `[8]` does not know — and cannot infer — the specific identity, and its dispatch is bound to the generic level.

### 9.19 Variant Class-Witness Slot

A variant slot's 3-word structure (§5.12) includes a third word for a class witness, populated at construction when the variant's active candidate has class-typed methods or when the variant value will be dispatched through a class boundary. The witness population is performed at the construction site of the variant — the same boundary that selects the active candidate. Consumption of the variant through a class-typed `::` dispatch reads the witness from the slot and proceeds per §9.12.

The variant in the absent state has no active candidate; the witness slot is null. Dispatching through `::` on an absent-state variant slot is **undefined** at the language level: the typechecker rejects the dispatch with a static error when the absent state is structurally reachable, and the variant must be narrowed (`-<`) into a candidate state before dispatch. The narrowing operator handles the absent case explicitly per its grammar (§7.14).

The variant-class-witness slot composes with both Case A and Case B (§9.9):

- **Case A.** A variant slot in a Case A position requires the type-variable bound to be satisfied by the active candidate's type. The dispatch flows through the hidden-parameter witness channel; the variant's word-3 witness is not involved.

- **Case B, direct reuse.** When the variant type `V` is declared as an instance of the target class `C` (`.instance V : C`), the variant's word-3 witness is the V-as-C witness, populated at construction. The variant slot satisfies the Case B C-typed parameter by **direct slot copy**: the parameter receives a 3-word slot whose third word is exactly the dictionary the dispatch needs.

- **Case B, wrap insertion.** When `V` is not declared as an instance of `C` but every candidate of `V` is individually an instance of `C`, the typechecker inserts a **wrap** at the call boundary. The wrap is a tag-dispatched construction: it branches on the variant's word-1 tag and, per branch, builds a fresh Case B slot whose word 3 is the (candidate, `C`) instance dictionary. The wrap is statically determined per candidate; the runtime cost is a single tag-branch plus the fresh slot construction.

- **Statically rejected.** When `V` is not declared as an instance of `C` and some candidate of `V` is not an instance of `C`, the value is not well-typed at the C-typed parameter position and the call is rejected at typecheck.

The reuse-vs-wrap disambiguation is **static and local**: the typechecker examines only the variant type `V`, the target class `C`, and the visible instance declarations. No flow analysis of construction-time class context is required.

### 9.20 Class-Method Fexpr-Typed Parameters

A class method's signature may declare a fexpr-typed parameter, subject to the fexpr-tainting discipline of §6.15. The class-method dispatch boundary is treated as a partial-application boundary for tainting purposes: any fexpr-typed argument flows into the dispatched method's frame as a READ-mode parameter, with the fexpr-relevance taint carried through as if the dispatch were a direct call.

**The `FexprFailure` standard message.** When a fexpr-tainting violation is detected at a dispatch boundary — a fexpr-typed argument whose ceiling cannot be honored by the dispatched implementation — the language raises a `FexprFailure` failure with a payload identifying the violation. `FexprFailure` is a built-in failure message; its payload class and hierarchy are part of the standard library.

**Defaults are incompatible with fexpr-typed parameters.** A class method with a fexpr-typed parameter cannot have a default body in the class declaration: the default body would need to be analyzable at class-declaration time, but the fexpr's defining frame `D` is not in scope at that point. The user resolves by either omitting the default (making the class method `.decl`-form only) or by avoiding fexpr-typed parameters on commands that need defaults.

**Partial application is compatible.** A command reference `{cmd: ..., my_fexpr, ...}` that binds a fexpr-typed argument at a class method's parameter position is well-formed under the §8.2 partial-application rules and §6.15 fexpr-tainting: the resulting command-typed value carries the fexpr-relevance taint, and its ceiling tracks the bound fexpr's `D`.

### 9.21 Variants with Fexpr Candidates

A variant whose candidate type-list contains a fexpr-typed candidate is **disallowed**. The structural difficulty is that a variant slot's lifetime can exceed the lifetimes of values stored at its candidates — by the variant's transit through parameters, returns, or storage — and a fexpr candidate's `D`-ceiling is incompatible with the variant's potentially-longer lifetime.

The exclusion does not extend to **fexpr-typed local slots themselves**: a local fexpr-typed slot is well-formed (§6.15). The exclusion *does* extend, via the transitive application of §8.13's restrictions, to **fexpr-relevance-tainted command-reference candidates** — a candidate that is a command-reference value binding a fexpr-typed argument per §8.12 carries the bound fexpr's `D`-ceiling and is rejected at the variant candidate position by the same rule (Restriction C applied transitively). The forbidden combinations are direct fexpr-typed candidates and fexpr-tainted command-reference candidates; the fexpr-typing restrictions of §8.13 enumerate the full set of forbidden positions.


---

## Appendix A. Lexical Structure

This appendix specifies the lexical surface of Basis: the token classes the lexer recognizes, the identifier shapes the parser distinguishes, the literal-token forms, the indentation discipline, and the disambiguation rules that resolve potentially-ambiguous adjacencies.

### A.1 Token Inventory

The lexer recognizes the following token classes:

**Dot-prefixed keywords.** Top-level definition keywords begin with `.` to distinguish them from user-defined identifiers:

```
.alias    .class    .cmd      .decl     .domain   .enum     .fail
.implicit .import   .inline   .instance .intrinsic .module  .msg
.object   .program  .record   .sub      .test     .union    .variant
```

The `.sub` keyword introduces subcommands at body-internal scope (§3.12); the rest are top-level forms (§2.2). The dot-prefix is part of the keyword token; the lexer does not produce `.` followed by a separate identifier.

**Punctuation tokens.** Single-character and short-sequence punctuation:

| Token | Role |
| --- | --- |
| `,` | Argument separator |
| `;` | Comment marker (line comment to end-of-line) |
| `:` | Argument-list introducer, signature separator, instance-class separator |
| `::` | Scope operator (§1.5) |
| `=` | Definition introducer, default declaration, equality (in guard positions) |
| `==` | Equality test (in guard positions) |
| `->` | Result-designator clause |
| `<-` | Placement operator (§7.1) |
| `-<` | Dynamic narrowing operator (§7.14) |
| `^` | Pointer prefix; rewind block marker |
| `&` | Reference mode marker (identifier-shape); pointer-of operator |
| `'` | Productive mode marker (identifier-shape) |
| `_` | Placeholder token (§3.15) |
| `(` `)` | Argument-list and grouping brackets |
| `[` `]` | Type-parameter brackets, indexing brackets |
| `{` `}` | Command-reference/literal/lambda fences |
| `${` `}` | Aggregate-literal fence (open-close pair) |
| `$[` `]` | Sequence-literal fence (open-close pair) |
| `<` `>` | Command-type-expression angles |
| `?` `?-` `?:` `??` | Guard-bearing block markers (§4.4) |
| `\|` | Recovery block marker (§4.4) |
| `@` `@!` | Frame-exit hook block markers (§3.13) |
| `%` | Block-grouping marker (§4.4) |
| `-` | Branch marker (§4.4); subtraction |
| `+` `*` `/` | Arithmetic operators |
| `#` | Local-introduction prefix |
| `/` | Implicit-context-parameter list separator (in signatures) |

The `$` character is reserved for the literal-fence prefixes (`${`, `$[`); it has no other lexical role. The `@!` token is recognized as a single token, not as `@` followed by `!`. The `?-`, `?:`, `??` tokens are similarly recognized as single tokens.

**Mode-marker placement.** The `'` and `&` mode markers attach to the immediately following identifier with no intervening whitespace: `'name` and `&name` are each a single identifier token, not a punctuation token followed by an identifier. The same-character-different-token rule does not arise — `'` and `&` standing alone in non-identifier-prefix position are syntax errors.

**Comment.** A `;` starts a line comment that extends to the end of the physical line. There are no nested comments and no block-comment form. The line containing a `;` is logically truncated at the `;` for all parsing purposes; the lexer skips the comment text and emits a newline as if the `;` were the line's end.

### A.2 Identifier Shapes

Basis recognizes three shapes for an identifier of any given name:

- `name` — bare identifier; READ access in a parameter declaration or use site
- `'name` — productive identifier; PRODUCE access
- `&name` — reference identifier; REFERENCE access

The lexer recognizes `'name`, `&name`, and `name` as three distinct identifier tokens of the same name. The same-scope rule (§6.3) prevents the three shapes from coexisting in the same scope: introducing `x` and `'x` in the same scope is a static error, since both refer to the same logical name with different mode contracts.

The mode-marker prefix is part of the identifier token; it does not lex as separate punctuation. A type expression `Int 'r` has tokens `Int`, then `'r` (a single identifier token), not `Int`, then `'`, then `r`.

Identifier-shape detection is purely lexical. The parser does not need context to determine whether `'r` is an identifier or a marker followed by an identifier — the leading `'` immediately followed by a name character produces the productive-identifier token.

### A.3 Type Names vs. Identifier Names

Basis distinguishes type names from value-identifier names by capitalization:

- **Type names** begin with an uppercase letter. Domains, records, unions, objects, variants, classes, aliases, message types, and module-name segments are all type names.
- **Value-identifier names** begin with a lowercase letter. Parameters, locals, and ordinary identifiers are value-identifier names.

The capitalization rule is structural — the lexer routes identifier tokens to either the type-name or value-identifier-name production based on the initial character. The mode-marker prefixes `'` and `&` do not affect the routing: `'r` is a productive value-identifier (lowercase initial after the marker); `'R` is a syntax error (the productive marker cannot prefix a type name).

Type names may include `::`-separated qualifiers: `Module::TypeName`, `Module::Nested::TypeName`. Each `::`-separated segment is a type name and must begin uppercase. The full qualified name routes through the module-system resolution rules (Appendix G, Appendix H).

### A.4 Literal Token Forms

The lexer recognizes six categories of literal token, corresponding to the six literal types of §7.7:

**Integer literals.** Whole-number literals admit decimal, hexadecimal, and binary forms:

```
42       0       1000000        ; decimal
0xFF     0x1234  0xCAFEBABE     ; hexadecimal
0b1010   0b1     0b0            ; binary
```

The literal-kind tag of an integer literal is `Integer`. The literal's value is the numeric value the digits express. Negative values are expressed using the prefix `-` operator applied to the literal at parse time, not as a sign-bearing literal token; `-42` is `-` (operator) applied to `42` (Integer literal).

**Decimal literals.** Numeric literals with a fractional part:

```
3.14      0.5      0.0      1.0       2.71828
```

The literal-kind tag is `Decimal`. Decimal literals do not include exponent notation.

**Hexadecimal literals as `Hex`.** Hex-shaped literals admitted at byte-positional positions carry the literal-kind tag `Hex`, distinct from `Integer`:

```
$[0xDE, 0xAD, 0xBE, 0xEF]      ; a Hex Sequence of bytes
```

The distinction matters at construction sites: a `Hex`-kind literal may inhabit a `Byte`-typed position; an `Integer`-kind literal may inhabit `Int8`/`Int16`/`Int32`/`Int64` positions per `.implicit` registrations.

**String literals.** Sequences of characters in double quotes:

```
"hello"           "with spaces"           "\n\t"           ""
```

The literal-kind tag is `String`. String literals carry no encoding declaration at the literal level — encoding is the constructor's concern. Escape sequences (`\n`, `\t`, `\\`, `\"`, `\xFF` for byte-explicit) follow the conventional surface.

**Aggregate literals.** The `${...}` fence introduces a positional-or-named aggregate at the construction site (§7.4):

```
${1, 2, 3}                    ; positional 3-element aggregate
${x <- 1, y <- 2}             ; named-field aggregate
${}                            ; empty (zero-default)
```

The literal-kind tag is `Aggregate`. The bracket pair `${` ... `}` is recognized as a fence — `${` is a single token at lex time, balanced by a `}` token (the lexer does not treat the trailing `}` specially; the parser matches via brace balancing per A.6).

**Sequence literals.** The `$[...]` fence introduces a homogeneous-element sequence (§7.5):

```
$[1, 2, 3, 4]                 ; integer sequence
$[]                            ; empty sequence
```

The literal-kind tag is `Sequence`. Like aggregates, the `$[` is a single open-fence token; the closing `]` matches via bracket balancing per A.6.

### A.5 Whitespace and Indentation

Basis uses indentation as the primary block-structure delimiter. Indentation at line start determines whether a line continues the current block, opens a nested block, or closes one or more enclosing blocks. The user-facing rules are below; the implementation strategy by which a lexer/parser recognizes these transitions is not specified by this document.

**Significant whitespace within a line.** Spaces and tabs within a line separate tokens but carry no further significance — `a , b` and `a,b` parse identically. The lexer does not require alignment.

**Indentation at line start.** A line's leading whitespace determines its indentation level, compared against the indentation of the immediately enclosing block:

- Equal — the line continues the current block.
- Greater — the line enters a new nested block, beginning the body of whatever construct the prior line introduced (a block marker, a record/object/union/variant body, a `cmd` body, etc.).
- Less — the line exits one or more enclosing blocks until the indentation matches an enclosing-level boundary.

The initial state is the zero-level boundary corresponding to the top level of the source file.

**Tab vs. space treatment.** A tab is conventionally treated as eight columns. Mixing tabs and spaces in leading whitespace is admitted but discouraged; an implementation may emit a warning.

**Comments and blank lines.** A line consisting only of a comment or only of whitespace is ignored for the purpose of indentation comparison and is logically skipped.

**Bracket-mode indent suspension.** When inside a bracketed or braced construct (`(...)`, `[...]`, `{...}`, `${...}`, or `$[...]`), the indentation rule is suspended. Multi-line argument lists, multi-line aggregate literals, and similar bracketed constructs may span lines with any indentation that aids readability:

```
process: arg1,
    arg2,
        arg3
```

The closing bracket re-engages indentation: the line containing the closing bracket is compared against the line that opened the bracket, and subsequent lines resume normal block-structure determination.

### A.6 Bracket, Brace, and Block Balance

Basis distinguishes three independent kinds of nesting:

- **Bracket nesting** — pairs `(` ... `)` and `[` ... `]`.
- **Brace nesting** — pairs `{` ... `}`, `${` ... `}`, `$[` ... `]`. The opening tokens `${` and `$[` are distinct from `{` and `[`; the closing tokens `}` and `]` are matched against the most-recently-opened compatible pair.
- **Block nesting** — indentation-determined nested blocks per A.5.

At end-of-input, every opening must have been closed, and the source must have returned to its top-level indentation. Unbalanced brackets, unbalanced braces, or unclosed nested blocks at end-of-input are static errors.

Within a bracketed or braced construct, block-nesting changes are suspended (A.5). This is the rule that admits multi-line argument lists and aggregate literals without dictating their internal indentation.

### A.7 Disambiguation Rules

A small set of disambiguation rules govern lexical adjacencies that would otherwise be parser-ambiguous:

**`<*>` distinct from `<>`.** The fexpr-typing surface uses `<*>` as the family-distinguishing marker:

```
:<>            ; never-fails ordinary command-typed value (empty parameter list)
:<*>           ; never-fails fexpr-typed value
?<*>           ; may-fail fexpr-typed value
!<*>           ; must-fail fexpr-typed value
```

The lexer recognizes `<*>` as the three-token sequence `<`, `*`, `>` and the parser matches the production; the `*` inside angle brackets is the syntactic marker that distinguishes the fexpr family from the ordinary family (§5.15). The lexer does not produce a `<*>` single-token; the parser's type-expression production handles the disambiguation.

**`(T:Class)` constraint form.** At parameter and receiver positions, the form `(T:Class)` declares a type-variable-bound parameter where `T` is a type variable and `Class` is the bound class (§9.9 Case A). The lexer produces `(`, identifier-or-typename `T`, `:`, identifier-or-typename `Class`, `)`; the parser recognizes this token sequence at parameter-position productions as a single constrained-parameter form. Outside parameter and receiver positions, the form parses as an ordinary parenthesized expression.

**`.inline` modifier.** The `.inline` modifier is applicable only to record-typed fields of a record declaration. Its semantic role is to promote the inner record's structure — both its named fields and its positional sequence — into the outer record's accessible surface:

- **Named promotion.** Each inner-record field name becomes accessible directly on the outer (e.g., `outer :: a` for a field `a` of the inner). Name collisions through `.inline` promotion are a static error at the outer record's declaration site.
- **Positional promotion.** The inner record's positions are spliced into the outer's positional sequence at the `.inline` field's declared position. Non-`.inline` fields each occupy one position; an `.inline` field contributes its inner record's positions (recursively, if the inner record itself contains `.inline` fields).
- **The grouping reference remains.** The `.inline` field's declared name (e.g., `outer :: inner`) is a valid named reference returning the inner-typed view of the inner's bytes. It does not correspond to a positional index, since the inner's positions have been spliced into the outer's sequence.
- **Byte layout is unaffected.** Records are packed regardless of `.inline` (§5.4); the modifier operates at the namespace-and-positional-structure level only.

Applying `.inline` to a non-record-typed field (a domain, primitive, union, or buffer form) is a static error. The modifier appears before the field declaration in the form `.inline Field : Type`, as captured by the grammar in Appendix B.

**Method-reference braces and angle-bracket-marked types.** The command-reference fence `{...}` (§3.15) and the command-literal fence `:<>{...}` (and parallel `?<>{...}`, `!<>{...}`, `:<*>{...}`, etc.) introduce command-typed values. The lexer treats `{` and `}` as brace-stack tokens; the parser matches `:<>{...}` by recognizing the type-expression `:<>` followed by a brace-fenced body. There is no `:<>{` single-token; the disambiguation is parser-side.

**Negative-number adjacency.** A `-` token followed immediately by a digit could be either subtraction or unary negation. The disambiguation is parser-side and follows the standard rule: in expression position where a value is expected, `- N` is unary negation; in expression position where a binary operator is expected (after a value), `- N` is subtraction. The lexer produces the same `-` and `42` (Integer literal) tokens in both cases.

**Slash adjacency.** A `/` token is overloaded: it separates the implicit-context-parameter list in signatures, and it is the division operator. The disambiguation is parser-side: at signature position (after the parameter list, before the body's `=`), `/` introduces the implicit list; in expression position, `/` is division.

**Underscore disambiguation.** The `_` placeholder serves four roles (§3.15). The lexer produces a single `_` token uniformly; the parser routes to the appropriate role based on grammatical position (PRODUCE-discard at argument position; partial-application deferral inside `{...}`; variant absent state in aggregate-literal positions; variant absent test in `-<` operations).

---

## Appendix B. Grammar (Concrete Syntax)

This appendix specifies the concrete syntax of Basis in BNF-style productions. Terminal symbols are written in `code` font (matching the lexer's token output of Appendix A); non-terminal symbols are written in *italics*; alternatives are separated by `|`; `?` denotes optional; `*` denotes zero-or-more; `+` denotes one-or-more. The productions are organized to mirror the language's surface structure rather than to minimize the production count.

**The productions in this appendix are descriptive, not implementation-ready.** They do not encode the indentation-sensitive structure of Basis, which is load-bearing for actually parsing source. A parser built directly from these productions without the indentation discipline (§2.8, §3.1, §4.5) will not be correct. The productions specify the *structure* the parser must recognize; the indentation-handling strategy is the implementation's concern.

### B.1 Source File

```
source-file       ::= module-decl? import-decl* top-level-decl*
module-decl       ::= .module qualified-name
import-decl       ::= .import qualified-name (as qualified-name)?
                    | .import string-literal
qualified-name    ::= TypeName ( :: TypeName )*
```

A source file consists of an optional module declaration, zero or more imports, and zero or more top-level definitions. The module declaration's qualified name uses `::` separators between segments per §2.4. The `.import` directive has two forms: a name-qualified module import and a file-qualified path import; per §2.4 the surface for the path form may admit additional aliasing.

### B.2 Top-Level Definition Forms

```
top-level-decl    ::= alias-decl    | class-decl    | cmd-decl
                    | decl-decl     | domain-decl   | enum-decl
                    | implicit-decl | instance-decl | intrinsic-decl
                    | msg-decl      | object-decl   | program-decl
                    | record-decl   | test-decl     | union-decl
                    | variant-decl
```

Each is introduced by its corresponding dot-prefixed keyword (§2.2). The `.sub` keyword is *not* a top-level form — it introduces subcommands at body-internal scope (§3.12); the subcommand-declaration production appears under *cmd-body* below.

```
alias-decl        ::= .alias TypeName : type-expr
class-decl        ::= .class TypeName type-params? : class-body
class-body        ::= class-entry+
class-entry       ::= decl-decl | cmd-decl
cmd-decl          ::= .cmd cmd-signature = cmd-body
decl-decl         ::= .decl cmd-signature
domain-decl       ::= .domain TypeName : fixed-size-type-expr
enum-decl         ::= .enum TypeName (: type-expr)? : enum-entries
enum-entries      ::= enum-entry+
enum-entry        ::= identifier (-> literal)?
implicit-decl     ::= .implicit cmd-signature = cmd-body
instance-decl     ::= .instance type-expr : class-list
class-list        ::= class-ref ( , class-ref )*
class-ref         ::= TypeName ( ( delegate identifier ) )?
intrinsic-decl    ::= .intrinsic cmd-signature
msg-decl          ::= .msg TypeName ( [ TypeName ] )? (: TypeName)?
object-decl       ::= .object TypeName : object-fields
object-fields     ::= field-decl+
field-decl        ::= identifier : type-expr
program-decl      ::= .program = expr
record-decl       ::= .record TypeName : record-fields
record-fields     ::= record-field+
record-field      ::= ( .inline )? identifier : fixed-size-type-expr ( = expr )?
test-decl         ::= .test string-literal = expr
union-decl        ::= .union TypeName : union-candidates
union-candidates  ::= union-candidate+
union-candidate   ::= identifier : fixed-size-type-expr
variant-decl      ::= .variant TypeName : variant-candidates
variant-candidates ::= variant-candidate+
variant-candidate ::= identifier : type-expr
```

### B.3 Type Expressions

```
type-expr         ::= TypeName type-args?              ; nominal type, optionally parameterized
                    | ^ type-expr                       ; pointer
                    | [ N ] fixed-size-type-expr        ; typed buffer of N fixed-size elements
                    | [ ] fixed-size-type-expr          ; runtime-length typed buffer
                    | [ N ]                             ; bracket form (N bytes)
                    | [ ]                               ; runtime-length bracket form
                    | command-type-expr                 ; :<…>, ?<…>, !<…>, with or without *
                    | TypeName :: TypeName              ; module-qualified type

fixed-size-type-expr ::= TypeName type-args?            ; nominal type (must resolve to fixed-size buffer-backed)
                       | [ N ] fixed-size-type-expr     ; typed buffer of N fixed-size elements
                       | [ N ]                          ; bracket form (N bytes)
                       | TypeName :: TypeName           ; module-qualified type (must resolve to fixed-size buffer-backed)

type-args         ::= [ type-arg-list ]
type-arg-list     ::= type-arg ( , type-arg )*
type-arg          ::= type-expr                         ; concrete type argument
                    | TypeName : TypeName               ; class-bounded type parameter (declaration only)
type-params       ::= [ type-param-list ]
type-param-list   ::= type-param ( , type-param )*
type-param        ::= TypeName                          ; bare type parameter
                    | TypeName : TypeName               ; class-bounded type parameter
```

The bracket forms `[N]` and `[]` are the buffer-backed root types (§5.5). A `Type` with parameters is a parameterized type; the parameter list at declaration is `type-params`, and at use-site is `type-args`. The class-bounded form `(T : C)` appears as a type parameter declaration; uses of the bound type variable use just the bare `T`.

The `fixed-size-type-expr` non-terminal is a syntactic subset of `type-expr` that excludes the runtime-length forms (`[]` and `[]T`), pointers, command-types, and fexpr-types. It is used in positions requiring fixed-size buffer-backed contents: record fields, union candidates, `[N]T` and `[]T` element types, and domain parents (§5.1). The typechecker further verifies that any `TypeName` reference in a `fixed-size-type-expr` position resolves to a type satisfying the fixed-size buffer-backed predicate (Appendix D.2); a grammatically well-formed `fixed-size-type-expr` whose named-type reference resolves to a non-buffer type or a runtime-length type is rejected statically.

### B.4 Command-Type Expressions

```
command-type-expr ::= : < param-type-list > result-designator-cmd?    ; never-fails ordinary
                    | ? < param-type-list > result-designator-cmd?    ; may-fail ordinary
                    | ! < param-type-list > result-designator-cmd?    ; must-fail ordinary
                    | : < * >                                          ; never-fails fexpr
                    | ? < * >                                          ; may-fail fexpr
                    | ! < * >                                          ; must-fail fexpr
param-type-list   ::= ( param-type ( , param-type )* )?
param-type        ::= type-expr mode-marker?
mode-marker       ::= '                                ; productive
                    | &                                ; reference
                                                       ; (READ has no marker)
result-designator-cmd ::= -> identifier
```

The mode marker on a parameter type in nameless context attaches as a *suffix* on the type per §3.3's "nameless context" rule. The `<*>` family carries the fexpr distinction; the parameter list is replaced by the single `*` token in fexpr types per §5.15.

### B.5 Command Signatures

```
cmd-signature     ::= regular-signature
                    | constructor-signature
                    | single-method-signature
                    | multi-method-signature

regular-signature ::= identifier : param-list? implicit-list? result-designator?
constructor-signature ::= type-expr ' identifier : param-list? implicit-list? result-designator?
single-method-signature ::= receiver :: identifier : param-list? implicit-list? result-designator?
multi-method-signature  ::= ( receiver-list ) :: identifier : param-list? implicit-list? result-designator?

receiver          ::= type-expr mode-marker? identifier         ; positional name-following
                    | type-expr ' identifier                     ; PRODUCE receiver shorthand
                    | type-expr & identifier                     ; REFERENCE receiver shorthand
                    | ( TypeName : TypeName ) identifier        ; class-bounded receiver
receiver-list     ::= receiver ( , receiver )*

param-list        ::= param ( , param )*
param             ::= type-expr mode-name
                    | ( TypeName : TypeName ) identifier        ; class-bounded parameter
mode-name         ::= identifier                                ; READ (bare)
                    | ' identifier                              ; PRODUCE
                    | & identifier                              ; REFERENCE

implicit-list     ::= / param-list
result-designator ::= -> identifier

optional-mark     ::= ? failure-set?                             ; may-fail, optional declared set (§4.9)
                    | ! failure-set?                             ; must-fail, optional declared set (§4.9)
                    | (none)                                     ; never-fails (no prefix on the identifier)
failure-set       ::= [ TypeName ( , TypeName )* ]               ; restrictive: body emits only these (closure-at-or-below)
                    | [ + ( TypeName ( , TypeName )* )? ]        ; additive: callee union plus these (§4.9)
```

The `optional-mark` prefixes the identifier (or the type-expr, in the default-constructor signature shape, where the type-expr fills the name slot). The signature productions above are written without the mark to keep them legible; in source, an `optional-mark` may precede the identifier in any signature shape.

The signature shapes implement the four command kinds (regular, constructor, single-receiver method, multi-receiver method) and the subcommand kind (which uses `regular-signature` since it has no receiver). The receiver-position productions admit class-bounded form `(T:Class)` per §9.9 Case A. The `'` and `&` mode markers attach to the immediately-following identifier per A.2.

### B.6 Body Expressions and Statements

```
cmd-body          ::= body-content
                    | _                                          ; empty body marker
body-content      ::= subcommand-decl* statement+

subcommand-decl   ::= .sub cmd-signature = cmd-body              ; lexically scoped (§3.12)

statement         ::= assignment | call | block-marker-construct | local-intro

local-intro       ::= # type-expr? identifier ( <- expr )?
                    | # ' identifier <- expr                     ; PRODUCE intro shorthand
                    | # & identifier <- expr                     ; REFERENCE intro shorthand

assignment        ::= identifier <- expr                         ; rewrite existing local
                    | ' identifier <- expr                       ; productive write
                    | & identifier <- expr                       ; reference rewrite
                    | field-access <- expr                       ; field write

field-access      ::= expr :: identifier                         ; named field access
                    | expr :: integer-literal                    ; positional field access (1-based)

allocator-qualified-expr ::= expr \ expr                         ; value construction qualified by allocator (§7.20)

call              ::= regular-call | method-call | multi-method-call | constructor-call
regular-call      ::= identifier ( : arg-list )?
method-call       ::= receiver-expr :: identifier ( : arg-list )?
multi-method-call ::= ( receiver-expr-list ) :: identifier ( : arg-list )?
constructor-call  ::= TypeName ( : arg-list )?                   ; statement form
                    | ( TypeName : arg-list )                    ; expression form

receiver-expr     ::= expr                                       ; same expression grammar as values
receiver-expr-list ::= receiver-expr ( , receiver-expr )*

arg-list          ::= arg ( , arg )*
arg               ::= expr | # identifier | _
```

Subcommand declarations appear at the head of *body-content* per §3.12's strict placement rule. After the contiguous subcommand-declaration block, only *statement*s are admitted.

### B.7 Block Markers and Indentation-Sensitive Composition

```
block-marker-construct ::= guard-block | rewind-block | recovery-block
                         | else-block | group-block | branch-block | at-block

guard-block       ::= ? expr cmd-body                            ; ? DO_WHEN
                    | ?- expr cmd-body                           ; ?- DO_WHEN_FAIL
                    | ?: expr cmd-body                           ; ?: DO_WHEN_SELECT
                    | ?? guard-block                             ; ?? DO_WHEN_MULTI (elevator)

rewind-block      ::= ^ cmd-body                                 ; ^ DO_REWIND
                    | ^                                          ;   bodiless rewind

recovery-block    ::= | recovery-spec? cmd-body                  ; | DO_RECOVER
recovery-spec     ::= TypeName identifier? -> 

else-block        ::= - cmd-body                                 ; - DO_ELSE
group-block       ::= % cmd-body                                 ; % DO_BLOCK
at-block          ::= @ cmd-body                                 ; @ DO_ON_EXIT
                    | @! cmd-body                                ; @! DO_ON_EXIT_FAIL
```

The eleven markers (§3.1, §4.4) are sequenced as adjacent siblings at the same indentation level. A *cmd-body* with multiple statements is an indented block of statements (per §A.5); statements within a body include further *block-marker-construct*s, producing arbitrarily deep nesting.

### B.8 The `<-` and `-<` Operator Forms

```
placement-stmt    ::= local-intro                                ; #x <- expr
                    | assignment                                 ; x <- expr or 'x <- expr or &x <- expr
                    | field-access <- expr

narrow-stmt       ::= type-expr ' identifier -< expr             ; T 'narrow -< v
                    | _ -< expr                                  ; _ -< v   (presence test)
                    | expr -< _                                  ; v -< _   (clear to absent)
                    | type-expr -< expr                          ; T -< v   (existential narrowing for unions)
```

The `<-` operator is the placement primitive (§7.1). The `-<` operator is the dynamic-narrowing primitive (§7.14, §7.15). The `_` token in `-< _` and `_ -<` positions is the variant absent-state placeholder (§3.15).

### B.9 The `${...}` and `$[...]` Literal Fences

```
aggregate-literal ::= ${ aggregate-entry-list? }
aggregate-entry-list ::= aggregate-entry ( , aggregate-entry )*
aggregate-entry   ::= expr                                       ; positional
                    | identifier <- expr                         ; named field
                    | identifier <- _                            ; named variant-typed field, absent state
                    | identifier <- candidate-name <- expr       ; named variant-typed field with active candidate

sequence-literal  ::= $[ sequence-entry-list? ]
sequence-entry-list ::= expr ( , expr )*
candidate-name    ::= identifier
```

Aggregate literals admit positional, named, and mixed forms (§7.4). Sequence literals are positional-only with implicit element-type uniformity (§7.5). The `_` placeholder in named-field position carries the variant absent-state semantics (§7.16).

### B.10 Command-Reference, Command-Literal, Lambda, and Fexpr Forms

```
command-ref       ::= { cmd-ref-body }
cmd-ref-body      ::= identifier                                 ; bare command name
                    | identifier : partial-arg-list              ; with positional partial application
                    | receiver-expr :: identifier                ; receiver-baked method reference
                    | receiver-expr :: identifier : partial-arg-list
                    | TypeName :: identifier                     ; class-disambiguated method reference (§9.10)
partial-arg-list  ::= partial-arg ( , partial-arg )*
partial-arg       ::= expr                                       ; applied
                    | _                                          ; deferred

command-literal   ::= : < param-type-list > { cmd-body }
                    | ? < param-type-list > { cmd-body }
                    | ! < param-type-list > { cmd-body }

lambda            ::= : < param-list-with-captures > { cmd-body }
                    | ? < param-list-with-captures > { cmd-body }
                    | ! < param-list-with-captures > { cmd-body }
param-list-with-captures ::= param-list? ( / capture-list )?
capture-list      ::= capture-entry ( , capture-entry )*
capture-entry     ::= identifier                                 ; READ capture
                    | & identifier                               ; REFERENCE capture

fexpr             ::= : { cmd-body }                             ; never-fails fexpr
                    | ? { cmd-body }                             ; may-fail fexpr
                    | ! { cmd-body }                             ; must-fail fexpr
```

The four constructional forms producing command-typed values (§8) share the brace-fenced body shape. The fexpr forms `:{…}`, `?{…}`, `!{…}` carry no parameter list — captures are implicit via free-name resolution against the defining frame (§8.5).

### B.11 Class Declarations and Instance Declarations

```
class-decl        ::= .class TypeName type-params? : class-body
class-body        ::= class-entry+
class-entry       ::= decl-decl                                  ; signature-only requirement
                    | cmd-decl                                   ; default-implementation body
                    | combined-class-decl                        ; (T:C1, T:C2) form

instance-decl     ::= .instance type-expr : class-list
class-list        ::= class-ref ( , class-ref )*
class-ref         ::= TypeName ( ( delegate identifier ) )?

combined-class-decl ::= .class TypeName type-params? : parent-class-list class-body?
parent-class-list ::= TypeName ( , TypeName )*
```

Class bodies enumerate the methods the class declares as required (`.decl`-form) or admits with a default (`.cmd`-form per §9.3). Combined classes form conjunctions of parent classes (§9.2). Instance declarations carry no body — the implementing methods are top-level commands on the type or delegated via the optional `(delegate fieldName)` clause (§9.4).

### B.12 The `(T:Class)` Constraint Form

```
class-bounded-param ::= ( TypeName : TypeName ) identifier
                      | ( TypeName : TypeName ) ' identifier   ; PRODUCE form (rare)
                      | ( TypeName : TypeName ) & identifier   ; REFERENCE form
```

At parameter and receiver positions, the `(T:Class)` form declares a type-variable-bound parameter. The bound name `T` is local to the signature; the bound class `Class` constrains which types may be supplied at each call site (§9.9 Case A). The bound binding is per-position — two parameters using the same `T` denote the same type at the call site.

### B.13 Fexpr Typing Surface `<*>`

```
fexpr-type        ::= : < * >                                    ; never-fails fexpr type
                    | ? < * >                                    ; may-fail fexpr type
                    | ! < * >                                    ; must-fail fexpr type
```

The `<*>` marker is the family-distinguishing surface for fexpr-typed values at type positions (§5.15). The marker replaces the parameter-type list of ordinary command-typed values; fexprs have no invoker-side parameter surface.

### B.14 The `(T:Class)` Form at Multi-Receiver Method Positions

The same `(T:Class)` form admitted at single-receiver positions is admitted at each position in a multi-receiver tuple:

```
multi-method-signature ::= ( receiver-list ) :: identifier : param-list? implicit-list? result-designator?
                         ; where any receiver may be class-bounded form
```

Each receiver in the tuple may independently use the bare receiver form, mode-marked form, or class-bounded form. The bound names introduced by `(T:C)` receivers are local to the signature and may be referenced in parameter types.

### B.15 The `{C::method}` Disambiguation Form

```
class-disambiguated-method ::= { TypeName :: identifier }
                             | { TypeName :: identifier : partial-arg-list }
```

When a method name is overloaded across multiple visible classes at a call site, the user may disambiguate by writing `{ClassName :: methodName}` (or `{ClassName :: methodName : partialArgs}`) to specify which class's method is intended (§9.10). The disambiguation form is part of the command-reference family — it produces a command-typed value with the receiver position open for the eventual call.

---

## Appendix C. AST Node Types

This appendix enumerates the AST node types the parser produces and the typechecker consumes. Each node type is presented with its constituent fields, the grammar production it derives from (Appendix B), and notes on its role. Implementations are free to choose internal representations (records, classes, tagged unions); the node-type inventory below is the conceptual model the rest of the specification refers to.

### C.1 Top-Level Definition Nodes

| Node | Fields | Source Production |
| --- | --- | --- |
| `AliasDecl` | `name: TypeName`, `target: TypeExpr` | *alias-decl* |
| `ClassDecl` | `name: TypeName`, `typeParams: [TypeParam]`, `entries: [ClassEntry]` | *class-decl* |
| `CombinedClassDecl` | `name: TypeName`, `typeParams: [TypeParam]`, `parents: [TypeName]`, `entries: [ClassEntry]` | *combined-class-decl* |
| `CmdDecl` | `signature: CmdSignature`, `body: CmdBody` | *cmd-decl* |
| `DeclDecl` | `signature: CmdSignature` | *decl-decl* |
| `DomainDecl` | `name: TypeName`, `parent: TypeExpr` | *domain-decl* |
| `EnumDecl` | `name: TypeName`, `constraint: TypeExpr?`, `entries: [EnumEntry]` | *enum-decl* |
| `ImplicitDecl` | `signature: CmdSignature`, `body: CmdBody` | *implicit-decl* |
| `InstanceDecl` | `type: TypeExpr`, `classes: [ClassRef]` | *instance-decl* |
| `IntrinsicDecl` | `signature: CmdSignature` | *intrinsic-decl* |
| `MsgDecl` | `name: TypeName`, `payloadClass: TypeName?`, `parent: TypeName?` | *msg-decl* |
| `ObjectDecl` | `name: TypeName`, `fields: [FieldDecl]` | *object-decl* |
| `ProgramDecl` | `body: Expr` | *program-decl* |
| `RecordDecl` | `name: TypeName`, `fields: [RecordField]` | *record-decl* |
| `TestDecl` | `name: String`, `body: Expr` | *test-decl* |
| `UnionDecl` | `name: TypeName`, `candidates: [UnionCandidate]` | *union-decl* |
| `VariantDecl` | `name: TypeName`, `candidates: [VariantCandidate]` | *variant-decl* |
| `ImportDecl` | `target: QualifiedName \| String`, `alias: QualifiedName?` | *import-decl* |
| `ModuleDecl` | `name: QualifiedName` | *module-decl* |

`ClassEntry` is a disjoint union of `DeclDecl` and `CmdDecl` for class members (§9.3). `ClassRef` carries an optional `delegate: identifier` field for the `(delegate fieldName)` clause (§9.4).

### C.2 Type-Expression Nodes

| Node | Fields |
| --- | --- |
| `NamedType` | `name: TypeName`, `args: [TypeArg]?` |
| `QualifiedType` | `module: QualifiedName`, `name: TypeName` |
| `PointerType` | `pointee: TypeExpr` |
| `TypedBufferType` | `size: Int?`, `element: TypeExpr` |
| `BracketType` | `size: Int?` |
| `CommandTypeExpr` | `mark: { ':', '?', '!' }`, `params: [ParamType]`, `resultDesignator: identifier?` |
| `FexprTypeExpr` | `mark: { ':', '?', '!' }` |

The distinction between `CommandTypeExpr` and `FexprTypeExpr` reflects the family-boundary rule of §5.15: the two are nominally distinct type-expression nodes, mirroring the `<*>` vs. `<…>` syntactic distinction.

`ParamType` is a record `{ type: TypeExpr, mode: { READ, PRODUCE, REFERENCE } }`.

### C.3 Command-Signature Nodes

| Node | Fields |
| --- | --- |
| `RegularSignature` | `name: identifier`, `params: [Param]`, `implicits: [Param]?`, `resultDesignator: identifier?` |
| `ConstructorSignature` | `receiverType: TypeExpr`, `receiverName: identifier`, `params: [Param]`, `implicits: [Param]?`, `resultDesignator: identifier?` |
| `SingleMethodSignature` | `receiver: Receiver`, `methodName: identifier`, `params: [Param]`, `implicits: [Param]?`, `resultDesignator: identifier?` |
| `MultiMethodSignature` | `receivers: [Receiver]`, `methodName: identifier`, `params: [Param]`, `implicits: [Param]?`, `resultDesignator: identifier?` |
| `SubcommandSignature` | (same fields as `RegularSignature`) |

`Param` is a record `{ type: TypeExpr | ClassBoundedType, mode: Mode, name: identifier }`.

`Receiver` is `Param` with the additional invariant that the receiver's mode is constrained per the per-shape table (§6.8): PRODUCE only for constructors; PRODUCE/REFERENCE/READ for methods.

`ClassBoundedType` records the `(T:Class)` form: `{ typeVar: TypeName, boundClass: TypeName }`.

`SubcommandSignature` is structurally identical to `RegularSignature`; the type distinction at the AST level signals the subcommand semantics (§3.12) to the analyzer.

### C.4 Body-Statement Nodes (per Block Marker)

| Node | Block Marker | Fields |
| --- | --- | --- |
| `DoWhen` | `?` | `guard: Expr`, `body: CmdBody` |
| `DoWhenFail` | `?-` | `guard: Expr`, `body: CmdBody` |
| `DoWhenSelect` | `?:` | `guard: Expr`, `body: CmdBody` |
| `DoWhenMulti` | `??` | `inner: DoWhen \| DoWhenFail` |
| `DoRewind` | `^` | `body: CmdBody?` |
| `DoRecover` | `\|` | `spec: RecoverySpec?`, `body: CmdBody` |
| `DoElse` | `-` | `body: CmdBody` |
| `DoBlock` | `%` | `body: CmdBody` |
| `DoOnExit` | `@` | `body: CmdBody` |
| `DoOnExitFail` | `@!` | `body: CmdBody` |
| `DoBranch` | (paired with `?` or `?-`) | (the `-` is a sibling, recorded in the parent body's statement list) |

`RecoverySpec` carries the spec for `|`-with-spec: `{ messageName: TypeName, binding: identifier? }`. The recovery production `| Name name -> body` populates `messageName = Name`, `binding = name`, with `body` as the recovery body. The bare `|` form has `spec = null`.

The chain composition of `?:` blocks (§4.4) is recorded as adjacent `DoWhenSelect` nodes in the parent body's statement list; no AST-level "chain" node is constructed.

### C.5 Expression Nodes

| Node | Fields | Notes |
| --- | --- | --- |
| `Identifier` | `name: String`, `mode: Mode` | One of READ, PRODUCE, REFERENCE per identifier shape |
| `IntegerLit` | `value: Int`, `kind: IntegerKind` | Decimal, Hex (literal-token), Binary forms |
| `DecimalLit` | `value: Decimal` | |
| `StringLit` | `value: String` | |
| `AggregateLit` | `entries: [AggregateEntry]` | (§7.4) |
| `SequenceLit` | `entries: [Expr]` | (§7.5) |
| `Call` | `target: CallTarget`, `args: [Arg]`, `implicits: [Arg]?` | Regular call |
| `MethodCall` | `receiver: Expr`, `methodName: identifier`, `args: [Arg]`, `implicits: [Arg]?` | Single-receiver |
| `MultiMethodCall` | `receivers: [Expr]`, `methodName: identifier`, `args: [Arg]`, `implicits: [Arg]?` | Multi-receiver |
| `ConstructorCall` | `type: TypeExpr`, `args: [Arg]`, `implicits: [Arg]?` | (§3.13) |
| `FieldAccess` | `base: Expr`, `field: identifier` | `obj :: field` for namespace use of `::` |
| `Narrow` | `targetType: TypeExpr?`, `binding: identifier?`, `subject: Expr`, `direction: NarrowKind` | (§7.14) |
| `Choice` | `alternatives: [Expr]` | The `lhs <- a \| b \| c` choice form (§7.17) |
| `BinaryOp` | `op: Operator`, `left: Expr`, `right: Expr` | `+`, `-`, `*`, `/`, `==`, etc. |
| `UnaryOp` | `op: Operator`, `arg: Expr` | `-x`, etc. |
| `Underscore` | (no fields) | The `_` placeholder; semantic role determined by position |

`Arg` is `{ kind: { Expr, FreshIntro, Placeholder }, value: Expr? }`. `FreshIntro` corresponds to `#name` in argument position (§3.13); `Placeholder` is `_` in argument-discard position.

`NarrowKind` is `{ Test, Bind, AbsentClear }` corresponding to `_ -< v`, `T 'narrow -< v`, and `v -< _` respectively.

`AggregateEntry` is `{ kind: Positional | Named | VariantAbsent | VariantActive, name: identifier?, value: Expr?, candidate: identifier? }`.

### C.6 Construction-Form Nodes

The five RHS shapes admitted at `<-` positions (§7.3):

| Node | Fields |
| --- | --- |
| `ParenCall` | `call: ConstructorCall \| Call` |
| `AggregateLit` | (per C.5) |
| `SequenceLit` | (per C.5) |
| `BareIdentifier` | `name: identifier`, `mode: Mode` |
| `BareLiteral` | `value: Lit` |

The construction-form classification is determined at the parser by the RHS shape; the typechecker dispatches construction-rule machinery (§7) accordingly.

`Placement` is the AST node for the `<-` operator itself: `{ lhs: Lvalue, rhs: ConstructionForm }`. `Lvalue` is one of `{ LocalIntro, Assignment, FieldWrite }`, mirroring §7.1's three syntactic positions.

### C.7 Lambda, Fexpr, Command-Literal, and Command-Reference Nodes

| Node | Fields |
| --- | --- |
| `CommandRef` | `target: identifier \| QualifiedMethodName`, `receiver: Expr?`, `partialArgs: [PartialArg]?` |
| `CommandLiteral` | `mark: { ':', '?', '!' }`, `params: [Param]`, `body: CmdBody` |
| `Lambda` | `mark: { ':', '?', '!' }`, `params: [Param]`, `captures: [Capture]`, `body: CmdBody` |
| `Fexpr` | `mark: { ':', '?', '!' }`, `body: CmdBody` |

`PartialArg` is `{ kind: Applied \| Deferred, value: Expr? }`. The `_` token in partial-application positions produces `Deferred`.

`QualifiedMethodName` carries the `{ClassName :: methodName}` disambiguation form: `{ className: TypeName, methodName: identifier }`.

`Capture` is `{ name: identifier, mode: { READ, REFERENCE } }` per §6.10.

### C.8 Class / Instance / Declaration Nodes

| Node | Fields |
| --- | --- |
| `ClassDecl` | (per C.1) |
| `CombinedClassDecl` | (per C.1) |
| `InstanceDecl` | (per C.1) |
| `ClassEntry` | `kind: DeclEntry \| CmdEntry`, `body: DeclDecl \| CmdDecl` |
| `ClassRef` | `name: TypeName`, `delegate: identifier?` |

The class-system AST distinguishes `.class` (which has a body listing required and default methods) from `.instance` (which has no body — the implementing methods are top-level commands on the type). The `ClassEntry` records both kinds; the `kind` discriminator routes the analyzer to either signature-only checking (`DeclEntry`) or default-implementation checking (`CmdEntry`).

### C.9 Recovery-Context Nodes

The `|` and `|`-with-spec block markers introduce recovery contexts (§4.4, §4.6). Their AST representation:

| Node | Fields |
| --- | --- |
| `DoRecover` | `spec: RecoverySpec?`, `body: CmdBody` |
| `RecoverySpec` | `messageName: TypeName`, `binding: identifier?` |

The bare `|` form has `spec = null` and engages on any propagating failure. The `|`-with-spec form has a populated `spec` field and engages only on failures whose message is at-or-below `spec.messageName` (§4.9). The `binding` identifier, when present, is in scope throughout the recovery body and refers to the bound payload value (§4.6).

Recovery contexts are part of the body-statement-node hierarchy (C.4): a `DoRecover` is a statement node appearing in a sibling list at some indentation level. The recovery's engagement rules and binding semantics are typechecker concerns (Appendix D); the AST records the syntactic form unmodified.

### C.10 AST Invariants

Several invariants are maintained at AST construction:

- **Identifier-shape consistency.** An `Identifier` node's `mode` field is determined entirely by the leading character of the source token: `name` → READ, `'name` → PRODUCE, `&name` → REFERENCE. The parser does not infer mode from context.
- **Productive-receiver-only-for-constructors.** A `ConstructorSignature` node's receiver mode is always PRODUCE. The parser rejects constructor signatures with non-PRODUCE receiver markers.
- **Subcommand placement.** A `SubcommandSignature` may only appear in `subcommand-decl` positions at the head of a *body-content* — the parser enforces this with a positional check during body parsing.
- **Block-marker indentation.** The body-statement parser determines sibling-vs.-child relationships between block markers from the indentation discipline of §A.5; no AST-level indentation information is recorded. The specific mechanism by which the parser tracks indentation is an implementation choice.

---

## Appendix D. Typechecking Rules (Judgment Forms)

This appendix specifies the typechecker as a collection of judgment-form rules. The rules are organized by what they conclude (well-formed types, well-formed expressions, well-formed signatures, well-formed bodies); the structure parallels the body's §§3–9.

### D.1 Notation Conventions

The judgments use the following metavariables and forms:

- **Γ** — typing environment: a finite map from identifiers to their type and mode. Entries include both lexical-scope locals and the enclosing command's parameters.
- **τ, σ** — types (well-formed type expressions).
- **e** — expressions.
- **s** — statements.
- **m** — mode marks (READ, PRODUCE, REFERENCE).
- **φ** — failure marks (`:`, `?`, `!`).
- **F** — failure sets (sets of message names).
- **M** — module-import graph context (for instance-resolution).
- **C** — class context (for class-method-dispatch and instance-coherence rules).

Judgment forms:

- `Γ ⊢ τ wf` — τ is a well-formed type in environment Γ.
- `Γ ⊢ e : τ` — e has type τ in Γ.
- `Γ ⊢ s ok` — s is a well-formed statement in Γ.
- `Γ ⊢ τ <: σ` — τ is a subtype of σ.
- `Γ ⊢ sig wf` — sig is a well-formed signature.
- `Γ ⊢ body : φ ; F ok` — body is well-formed with failure mark φ and failure set F.

Premises sit above the line; conclusion below. Multiple premises read as conjunction. Side conditions appear in parentheses to the right.

### D.2 Type Formation Rules

A type expression is well-formed if its components are well-formed and their composition is admitted by the language:

```
                                    (NamedType-Decl)
Γ ⊢ TypeName declared in Γ
─────────────────────────
Γ ⊢ TypeName wf


                                    (TypedBuffer)
Γ ⊢ τ wf      τ buffer-backed
─────────────────────────────
Γ ⊢ [N]τ wf


                                    (Pointer)
Γ ⊢ τ wf
──────────
Γ ⊢ ^τ wf


                                    (CmdType)
Γ ⊢ τᵢ wf for each parameter type τᵢ
mark ∈ {:, ?, !}
─────────────────────────────────
Γ ⊢ mark<τ₁ mode₁, …, τₙ modeₙ> wf


                                    (FexprType)
mark ∈ {:, ?, !}
────────────────
Γ ⊢ mark<*> wf
```

**Buffer-backed and fixed-size buffer-backed predicates.** A `[N]τ` requires τ to be fixed-size buffer-backed; a record-field, union-candidate, or domain-parent declaration also requires fixed-size buffer-backed:

```
Γ ⊢ τ wf      τ ∉ {pointer, object, variant, command-typed, fexpr-typed, …}
──────────────────────────────────────────────────────────────────────────
Γ ⊢ τ buffer-backed


Γ ⊢ τ buffer-backed      τ ∉ {[], []σ for any σ}
────────────────────────────────────────────────
Γ ⊢ τ fixed-size buffer-backed
```

The disjoint cases enumerate all the non-buffer types; the union of all named-type-as-buffer-backed plus the literal types covers the buffer-backed case (§5.1). The runtime-length forms `[]` and `[]σ` (for any σ) are buffer-backed but not fixed-size; they participate in the buffer-backed category but are excluded from byte-aggregate containment positions (§5.1).

### D.3 Subsumption Rules

The subtyping relation `<:` is the union of multiple sources of subsumption:

**Buffer-backed parent-chain subsumption (§5.5):**

```
Γ ⊢ τ : .domain T : σ
──────────────────────
Γ ⊢ τ <: σ


Γ ⊢ τ fixed-size buffer-backed     τ has byte-width N
─────────────────────────────────────────────────────
Γ ⊢ τ <: [N]
```

The runtime-length types `[]` and `[]T` are leaves in this lattice: no rule produces them as a supertype of any fixed-size type, and no rule produces a fixed-size type as a supertype of either of them (§5.5).

**Reflexivity and transitivity:**

```
                  (Refl)                              (Trans)
                                Γ ⊢ τ <: σ      Γ ⊢ σ <: ρ
──────                          ────────────────────────────
τ <: τ                                 Γ ⊢ τ <: ρ
```

**Failure-mark subsumption (§4.2):**

```
                                    : ⊑ ?            ! ⊑ ?
                                    (compatible mark substitution within a family)
```

The relation `⊑` applies to mark positions; assignment from a `:`-marked or `!`-marked call to a `?`-marked slot is admitted, but neither `:` nor `!` is the other's subtype.

**Class-typed-family invariance (§5.5):**

```
Class types do not subtype each other except through explicit class hierarchies (§9).
A Container[A] is not a Container[B] unless A = B.
```

**Family-boundary non-subsumption (§5.15):**

```
:<*> is not a :<>;  ?<*> is not a ?<>;  !<*> is not a !<>.
The fexpr family and the ordinary command-typed family are nominally distinct.
```

**Payload-class covariance for failure messages (§4.8, §9.17):**

```
Γ ⊢ M child of M' in message hierarchy
Γ ⊢ M's payload class C is a subclass of M's parent class C'
─────────────────────────────────────────────────────────
Γ ⊢ payload of M acceptable where M'-payload expected
```

### D.4 Expression-Position Typing

A command invoked in expression position (right-hand side of `<-`, parenthesized argument, guard, scrutinee) produces a value of the type designated by the `-> name` clause (§3.7):

```
                                    (ExprCall)
Γ ⊢ cmd : sig    sig has -> param-name
Γ ⊢ args satisfy sig's argument positions (excluding param-name)
sig's param-name has mode m and type τ
─────────────────────────────────────
Γ ⊢ (cmd: args) : τ                    (with reading-from-m semantics)


                                    (ExprNarrow)
Γ ⊢ subject : variant-type      Γ ⊢ T narrowing-target of variant
─────────────────────────────────────────────────────────────
Γ ⊢ (T 'narrow -< subject) : T          (with TagMismatch on failure)
```

### D.5 Construction-Form Typing

The five RHS shapes admitted at `<-` positions (§7.3) each have their own typing rule:

**Parenthesized command call:**

```
Γ ⊢ (cmd: args) : τ      (D.4 ExprCall)
Γ ⊢ lhs has type σ      Γ ⊢ τ <: σ
─────────────────────────────────────
Γ ⊢ lhs <- (cmd: args) ok
```

**Aggregate literal:**

```
Γ ⊢ lhs has type σ
σ admits Aggregate-shape construction (§7.4)
Each entry's RHS satisfies the corresponding position
────────────────────────────────────────────────────
Γ ⊢ lhs <- ${...} ok
```

**Sequence literal:**

```
Γ ⊢ lhs has element type τ (lhs is a typed buffer or sequence-admitting type)
Each sequence entry has type ⊆ τ (with .implicit bridging)
─────────────────────────────────────────────────────────
Γ ⊢ lhs <- $[...] ok
```

**Bare identifier:**

```
Γ ⊢ rhs : τ      τ value-copyable (buffer-backed)
Γ ⊢ lhs has type σ      Γ ⊢ τ <: σ
───────────────────────────────────
Γ ⊢ lhs <- rhs ok        (with byte-copy semantics)
```

**Bare literal:**

```
Γ ⊢ rhs is a literal of kind K
Γ ⊢ lhs has type σ
σ admits K-kind literals directly OR .implicit registers K → σ
──────────────────────────────────────────────────────────
Γ ⊢ lhs <- rhs ok        (with .implicit insertion if needed)
```

### D.6 Call-Site Typing

**Regular call:**

```
Γ ⊢ cmd : (τ₁ m₁, …, τₙ mₙ) → mark
Each argᵢ supplies the call's i-th position per its mode mᵢ
Implicit context parameters resolve uniquely from Γ (D.7)
─────────────────────────────────────────────────────────
Γ ⊢ cmd: args ok                       (with failure mark = mark)
```

**Method call:**

```
Γ ⊢ receiver : R     Γ ⊢ R ∈ class containing method m
Γ ⊢ m : (R receiver-mode, τ₁ m₁, …, τₙ mₙ) → mark
Each argᵢ satisfies position i per mᵢ
Receiver mode satisfies the R1 (call-site initialization) rule of §6.7
─────────────────────────────────────────────────────────────
Γ ⊢ receiver :: m: args ok                  (with failure mark = mark)
```

**Multi-receiver method call:**

```
Γ ⊢ each receiverᵢ : Rᵢ      each Rᵢ in a class containing m
Γ ⊢ m : ((R₁, …, Rₖ) receiver-tuple, τ₁ m₁, …, τₙ mₙ) → mark
Each receiver and arg satisfies its position
Joint instance resolution per §3.11 / §9.4
──────────────────────────────────────────
Γ ⊢ (receiver₁, …, receiverₖ) :: m: args ok
```

### D.7 Implicit Context Parameter Resolution

Implicit context parameters (§3.6) are filled at the call site by uniqueness-of-type:

```
                                    (ImplicitResolve)
Implicit parameter declared with type τ_impl and mode m
Γ contains exactly one identifier `x` with type τ_impl
mode-compatibility: caller's `x` mode satisfies callee's m
─────────────────────────────────────────────────
Γ ⊢ implicit position satisfied with `x` ok


                                    (ImplicitFail-Ambiguous)
Γ contains multiple identifiers with type τ_impl
─────────────────────────────────────────────────
Γ ⊢ implicit resolution ambiguous (static error)


                                    (ImplicitFail-Absent)
Γ contains no identifier with type τ_impl
──────────────────────────────────────────
Γ ⊢ implicit resolution absent (static error)
```

The user-resolvable rescue path: pass the value explicitly at the call site, bypassing the resolution algorithm.

### D.8 The `-<` Operator Typing

The dynamic-narrowing operator (§7.14) admits multiple type-pair scenarios:

**Variant narrowing:**

```
Γ ⊢ v : variant V with candidate types T₁, …, Tₙ
Γ ⊢ T is at-or-below some Tᵢ in T's subsumption chain
───────────────────────────────────────────────
Γ ⊢ T 'narrow -< v : T               (may-fail: TagMismatch if v's tag ≠ Tᵢ or v absent)


                                    (Variant absent test)
Γ ⊢ v : variant V
─────────────────
Γ ⊢ _ -< v : ok      (may-fail: TagMismatch if v is in absent state)


                                    (Variant absent clear)
Γ ⊢ v : variant V
─────────────────
Γ ⊢ v -< _ : ok      (always-succeeds; v becomes absent)
```

**Class hierarchy narrowing:**

```
Γ ⊢ obj : object type O      Γ ⊢ T at-or-below O in class hierarchy
─────────────────────────────────────────────────────
Γ ⊢ T 'narrow -< obj : T          (may-fail: TagMismatch on type-mismatch)


                                    (Pointer narrowing)
Γ ⊢ p : ^P      Γ ⊢ T at-or-below P
────────────────────────────────────
Γ ⊢ T 'narrow -< p : ^T              (may-fail: TagMismatch on type-mismatch)
```

**Union narrowing (compile-time only):**

```
Γ ⊢ u : union U     Γ ⊢ T appears on at least one union-candidate's subsumption chain
───────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T 'narrow -< u : T               (no runtime failure; the recovery branch is unreachable)


                                    (Class-narrowing on union)
Γ ⊢ u : union U     Γ ⊢ exactly one union-candidate type has a C-class instance
─────────────────────────────────────────────────────────────────────────
Γ ⊢ C 'narrow -< u : C-witness-typed binding
```

### D.9 Lambda, Fexpr, Command-Literal, and Command-Reference Typing

**Command literal:**

```
Γ ⊢ body : mark ; F under (Γ extended with params)
─────────────────────────────────────────────────
Γ ⊢ mark<params>{body} : mark<param-types> (lifted to command-typed value)
```

**Lambda:**

```
Γ ⊢ each capture entry resolves in Γ at READ or REFERENCE mode
Γ ⊢ body : mark ; F under (Γ-captures-extended)
ceiling = D (lambda's defining frame, §8.4)
───────────────────────────────────────────
Γ ⊢ mark<params / captures>{body} : mark<param-types>
```

**Fexpr:**

```
Γ ⊢ body : mark ; F under (Γ at fexpr-relevance taint)
no free names resolve to long-lived storage (Restrictions A–G of §8.13)
──────────────────────────────────────────────────────────────────
Γ ⊢ mark{body} : mark<*>


D = current frame; F = fexpr-relevance-tainted (passes through every taint check)
```

**Command reference:**

```
Γ ⊢ underlying cmd resolves in Γ as either bare or method form
Γ ⊢ each partial arg type-checks at its position
PRODUCE positions are not bound (must be deferred via _)
──────────────────────────────────────────────────────
Γ ⊢ {receiver :: name : args} : underlying-type with bound positions elided
```

### D.10 Class-Method Dispatch Typing

**Case A — Type-variable-bound parameter (§9.9):**

```
Γ ⊢ at call site, (T:C) parameter receives concrete type τ
Γ ⊢ τ ∈ class C in the module-import graph M
───────────────────────────────────────────
Γ ⊢ method call dispatches through τ's C-instance dictionary


Witness is a hidden parameter; dispatch is at call-site type τ.
```

**Case B — Existential class-typed parameter (§9.9):**

```
Γ ⊢ at call site, C-typed parameter receives a slot whose runtime type τ ∈ C
Γ ⊢ slot is a 3-word triple (tag identifying τ, payload pointer, witness)
─────────────────────────────────────────────────────────────────
Γ ⊢ method call dispatches through the slot's stored witness


Tag is consulted at runtime; witness chosen at construction site.
```

### D.11 Overload Resolution

Per §9.10, overload resolution proceeds in three layers:

1. **Argument-shape filter.** Eliminate candidates whose signature does not match the call's argument shape (arity, types, modes).
2. **Most-specific-candidate ranking.** Among remaining candidates, prefer the one whose signature is most-specifically matched by the call. The specificity ordering is: type variables less specific than concrete types; broader buffer-backed types (`[N]`, `[]`) less specific than narrower (named domain, record).
3. **Tie rejection.** If two candidates are equally specific (incomparable), the call is ambiguous and rejected with a static error. The user resolves via type annotation or via the `{C::method}` disambiguation form (§9.10).

The judgment form:

```
candidates_after_arg_filter = {c₁, c₂, …, cₖ}
most-specific candidate exists and is unique → c
────────────────────────────────────────────
Γ ⊢ call resolves to c
```

### D.12 Partial-Application Typing

A command reference with partial application (§9.14) produces a command-typed value with bound positions elided:

```
Γ ⊢ underlying cmd has type (R receiver, τ₁ m₁, …, τₙ mₙ) → mark
Γ ⊢ partial-app applies: receiver (always); some subset of args; PRODUCE positions deferred via _
ceiling: derived from captured mode markers (REFERENCE captures yield ceiling = D)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ {receiver :: cmd : applied-args} : mark<deferred-arg-types>
```

The ceiling-tracking rule for partial application (§9.14):

- READ captures: no ceiling implication
- REFERENCE captures: ceiling = the captured-slot's frame D
- PRODUCE: forbidden in bound positions; must be deferred

### D.13 Instance-Coherence Typing

For class instances (§9.15):

**Intra-module uniqueness.** Two `.instance T : C` declarations of the same `(T, C)` pair in the same module is a static error.

**Cross-module specificity ranking.** When multiple modules declare `.instance T : C` for the same `(T, C)` pair, the more-specific module's instance wins. Specificity is determined by the module hierarchy (Appendix H.5).

**Orphan-instance permissibility.** An `.instance T : C` declaration is admitted in any module whose import graph reaches both T's declaration module and C's declaration module — no co-location restriction. Orphan instances enable the desired cross-module extensibility (§9.15).

**Import-time competition warning.** If a module's import graph causes two different modules' instances of the same `(T, C)` to both be reachable, an import-time warning fires.

### D.14 Failure-Mark Conformance

A command body's failure mark φ_body must conform to its declared mark φ_decl:

```
                                    (Conform)
Γ ⊢ body : φ_body ; F_body
Γ ⊢ φ_body ⊑ φ_decl     F_body ⊆ F_decl
───────────────────────────────────
Γ ⊢ body conforms with (φ_decl, F_decl)
```

**Six-state failure-state lattice.** The body's failure state at each program point is one of:

1. `clear` — no in-flight failure; subsequent statements execute.
2. `failing(:)` — never-fails-mark; impossible (no transitions land here).
3. `failing(?)` — may-fail-mark with failure in flight; subsequent statements skip until recovery.
4. `failing(!)` — must-fail-mark with failure in flight; behaves as `failing(?)` for control-flow but with a stricter conformance check.
5. `mixed(?)` — at convergence (CFG join), some predecessors are `failing(?)` and others are `clear`.
6. `mixed(!)` — at convergence, some predecessors are `failing(!)` and others are `clear`.

The transfer-function table for each block marker and each call-site shape is in Appendix E.

### D.15 Subcommand Visibility Typing

A subcommand's name resolves only within the enclosing command's body (§3.12):

```
                                    (SubVisibility)
Γ ⊢ subcommand s declared at body B
Use site of s is within B or any deeper subcommand body of B
──────────────────────────────────────────────────────────
Γ ⊢ s reachable at use site


                                    (SubCapture-Forbidden)
subcommand body operates against parameters and module scope only
no use of enclosing command's local identifiers
─────────────────────────────────────────────────────
Γ ⊢ subcommand body has no capture (well-formed)
```

The no-capture rule (§3.12) is enforced as a typechecker invariant on subcommand bodies: a name resolution that would resolve to an identifier in the enclosing command's body (other than implicit-context-parameter resolution at the call site, which is parameter-supplied) is a static error.

---

## Appendix E. Static Analyses (Joint CFG-Walking Composition)

This appendix specifies the four static analyses the Basis typechecker performs over each command body's control-flow graph: initialization tracking, failure-state lattice, READ-taint, and fexpr-relevance taint. The analyses share a single forward-flow walk over the CFG and produce a *joint state vector* at each program point; their join points and transfer functions interact only at the boundaries enumerated below.

### E.1 The Shared CFG Walk

Each command body is compiled to a control-flow graph (CFG) at parse time. Nodes correspond to statements; edges correspond to control transitions (sequential succession, branch via block markers, recovery on failure). The CFG admits joins at convergent points and may contain loops (via `^` rewinds).

The typechecker performs a single forward-flow walk over this CFG, maintaining a **state vector** at each program point:

```
StateVector = (init: InitLattice, failure: FailLattice, readTaint: TaintLattice, fexprTaint: TaintLattice)
```

Each component is a separate lattice; the joint state-vector forms the product lattice. At convergent CFG points, the join is performed component-wise — each analysis joins per its own lattice rules.

Transfer functions update the state vector at each statement; the per-component transfer functions are described in E.2–E.5. Some statement types have transfer functions that touch multiple components (e.g., a `.fail` statement updates both `failure` and may affect taint propagation); these cross-component interactions are noted at each transfer function.

### E.2 Initialization Analysis

**Lattice.** Per-slot initialization state:

```
InitLattice = { uninit, init, uncertain } per slot
                  ⊥        ⊤
            (with uncertain = ⊤ as the join of uninit and init)
```

The lattice is per-slot. The state vector tracks initialization for every named slot in the command body's scope. The initial state at the body's entry: parameters are `init` for READ and REFERENCE modes, `uninit` for PRODUCE modes (the callee must produce them); locals are `uninit` until introduced.

**Transfer functions.**

| Statement | Transfer |
| --- | --- |
| `#x <- expr` | `x` transitions from `uninit` to `init` |
| `'r <- expr` | `r` transitions from `uninit` to `init` (write-once check applies) |
| `x <- expr` (rewrite) | `x` must be `init` pre-write; remains `init` post-write |
| `&x <- expr` (reference rewrite) | `x` must be `init`; remains `init` |
| call site with PRODUCE arg `#name` | `name` transitions from `uninit` to `init` on success path |
| read of `x` | `x` must be `init` at the read point |

**Joins.** At convergent CFG points:

| Predecessor states | Join |
| --- | --- |
| `init`, `init` | `init` |
| `uninit`, `uninit` | `uninit` |
| `init`, `uninit` | `uncertain` (read-then-use is a static error) |

**Conformance.** At every successful exit edge of a command body, every productive parameter must have state `init`. Otherwise, the body fails to typecheck (the productive write-once obligation per §6.13).

### E.3 Failure-State Lattice Analysis

**Lattice.** Six states per program point:

```
{ clear, failing(:), failing(?), failing(!), mixed(?), mixed(!) }
```

- **`clear`** — no in-flight failure; subsequent statements execute.
- **`failing(:)`** — the `:` mark is never-fails; this state is unreachable in well-formed code (no statement can produce it). Present in the lattice for completeness; behaves as ⊥.
- **`failing(?)`** — may-fail-mark failure in flight; subsequent statements are skipped until a recovery context engages.
- **`failing(!)`** — must-fail-mark failure in flight; subsequent statements are skipped; the body cannot reach a `clear` exit through this path.
- **`mixed(?)`** — at a convergence point, some predecessors are `failing(?)` and others are `clear`. Means: a may-fail path is in flight on some predecessor branches; subsequent statements are reachable only on the `clear`-path branches.
- **`mixed(!)`** — analogous to `mixed(?)` but the failing branches are must-fail.

**Join table** (rows are state-A, columns are state-B; cell is the joined state):

| A \ B | clear | failing(:) | failing(?) | failing(!) | mixed(?) | mixed(!) |
| --- | --- | --- | --- | --- | --- | --- |
| **clear** | clear | clear | mixed(?) | mixed(!) | mixed(?) | mixed(!) |
| **failing(:)** | clear | failing(:) | failing(?) | failing(!) | mixed(?) | mixed(!) |
| **failing(?)** | mixed(?) | failing(?) | failing(?) | mixed(?) | mixed(?) | mixed(?) |
| **failing(!)** | mixed(!) | failing(!) | mixed(?) | failing(!) | mixed(?) | mixed(!) |
| **mixed(?)** | mixed(?) | mixed(?) | mixed(?) | mixed(?) | mixed(?) | mixed(?) |
| **mixed(!)** | mixed(!) | mixed(!) | mixed(?) | mixed(!) | mixed(?) | mixed(!) |

(The lattice is symmetric so the table folds along the diagonal; presented in full for ease of lookup.)

**Transfer functions** (per block marker):

| Marker | Body engages on | Body's effect on the failure state |
| --- | --- | --- |
| `?` (DO_WHEN) | guard `clear` | guard's failure consumed; body's effects propagate per ordinary rules |
| `?-` (DO_WHEN_FAIL) | guard `failing` | guard's failure consumed; body's effects propagate |
| `?:` (DO_WHEN_SELECT) | guard `clear` | guard's failure consumed; control exits surrounding indentation level on engagement |
| `??` (DO_WHEN_MULTI) | wraps inner `?` or `?-` | failure consumed by `??`; control elevates one level |
| `^` (DO_REWIND) | (no engagement condition) | failure from any body statement consumed by `^`; control rewinds on success, falls through on failure |
| `|` (DO_RECOVER) | preceding sibling `failing` | failure consumed; body executes from `clear`; propagating-set narrowed to ∅ |
| `|`-with-spec | preceding sibling failing with matching message | failure consumed; payload bound; body executes from `clear`; propagating-set narrowed per the per-root rule below |
| `-` (DO_ELSE) | paired with preceding `?` or `?-` | runs on the alternative branch; failure handling per parent |
| `%` (DO_BLOCK) | (no engagement condition) | body executes as ordinary statements; unrecovered failure propagates |
| `@` (DO_ON_EXIT) | (fires at frame retirement) | body runs at exit; failure not propagated through body |
| `@!` (DO_ON_EXIT_FAIL) | (fires at frame failure-exit) | body runs only on failure exits |

**Propagating-set narrowing for `|`-with-spec.** Under typed failures (§4.9), the lattice carries a propagating-set component represented as a union of at-or-below closures with declared roots. When a `|`-with-spec block engages on `at-or-below(Name)`, the post-set is computed per-root: for each root `R` in the pre-set,

- If `at-or-below(R) ⊆ at-or-below(Name)`: `R` is **removed** (entirely consumed).
- If `at-or-below(R) ∩ at-or-below(Name) = ∅`: `R` is **retained unchanged** (the consumption doesn't touch it).
- Otherwise — `Name` is a proper descendant of `R`, so `at-or-below(Name) ⊊ at-or-below(R)` — `R` is **retained unchanged** conservatively. The set representation is a union of at-or-below closures; subtracting a strict-descendant closure would leave a closure-with-holes, which the representation does not capture. The conservative retention preserves soundness at the cost of precision.

The post-set is the union of `at-or-below(R)` for each retained root `R`. The rule is precise when the spec's closure either fully covers or fully misses each pre-set root, and conservative only when the spec sits strictly inside some pre-set root's closure.

**Conformance.** At every reachable exit edge:

- If the exit is success, the failure state must be `clear` and the declared mark must allow `:` or `?`.
- If the exit is failure, the failure state must be `failing(?)`, `failing(!)`, `mixed(?)`, or `mixed(!)`, and the declared mark must allow the corresponding failure mode.
- Mark conformance: `:` requires every reachable exit to be `clear`; `?` admits `clear` or `failing(?)`/`mixed(?)` exits; `!` requires every reachable exit to be `failing(!)`.

### E.4 READ-Taint Analysis

**Lattice.** Per-slot taint state:

```
TaintLattice = { untainted, READ-tainted } per slot
                     ⊥             ⊤
```

A slot is READ-tainted if its access path is rooted at a READ parameter. The taint propagates through:

- **Field access.** Reading field `f` of a READ-tainted slot yields a READ-tainted result.
- **Pointer dereference.** Dereferencing a `^T` whose slot is READ-tainted yields a READ-tainted result.
- **Indexing.** Indexing a typed buffer or sequence whose slot is READ-tainted yields a READ-tainted result.
- **Bare-identifier copy.** Copying a READ-tainted value into a fresh slot yields a fresh slot whose READ-taint status depends on whether the value carries any sub-storage references; for buffer-backed values, the result is *untainted* (the byte-copy produces a byte-disjoint result). For non-buffer values, the result is READ-tainted (the slot carries references into READ-rooted storage).

**Sources.** READ parameters (mode = READ). Locals introduced from a READ-tainted source inherit the taint.

**Transfer functions.**

| Statement | Effect on READ-taint |
| --- | --- |
| `#x <- expr` | `x` taint = expression's taint (per the construction-form rules above) |
| field/pointer/index access of tainted source | result inherits taint |
| bare-identifier `<-` of buffer-backed | result is untainted (byte-disjoint copy) |
| bare-identifier `<-` of non-buffer | result inherits taint (slot-view propagation) |
| call site with arg tainted | the argument is rejected if the call site's parameter is REFERENCE or PRODUCE; the call is admitted if the call's parameter is READ |

**Conformance.** The body fails to typecheck if at any program point:

- A REFERENCE or PRODUCE write is attempted through a READ-tainted access path.
- A READ-tainted slot is passed to a call's REFERENCE or PRODUCE parameter position.

### E.5 Fexpr-Relevance Taint Analysis

**Lattice.** Per-slot:

```
TaintLattice = { untainted, fexpr-tainted } per slot
                     ⊥           ⊤
```

A slot is **fexpr-tainted** if its value is, or may transitively reach, a fexpr-typed value (§6.15, §8.13).

**Sources.**

1. **Direct fexpr-typed slots.** A slot whose declared type is `:<*>`, `?<*>`, or `!<*>`.
2. **Composite slots.** A slot whose type structurally contains a fexpr-typed component (forbidden at field level for buffer-backed types; admitted only in narrow circumstances per §8.13).
3. **Command-reference values.** A command reference `{cmd: …}` whose underlying definition reaches a fexpr through its body or captures.
4. **Lambda captures of fexpr-typed values.** Forbidden (Restriction E of §8.13); the structural restriction prevents this case from arising.

**Transfer functions.** Fexpr-relevance propagates parallel to READ-taint but along its own access paths:

| Statement | Effect on fexpr-taint |
| --- | --- |
| `#x <- expr` where expr reaches a fexpr-typed value | `x` becomes fexpr-tainted |
| field/pointer/index access of fexpr-tainted source | result inherits taint |
| call site with fexpr-typed arg | argument's fexpr-taint flows into the callee's analysis |

**Conformance.** Body fails to typecheck if any of the seven fexpr restrictions (§8.13, A–G) are violated:

- A — fexpr-typed value assigned to long-lived storage (e.g., a global, a module-scope identifier).
- B — pointer to a fexpr-typed slot.
- C — fexpr-typed value in a field of a long-lived containment (object field with object-ceiling outliving the fexpr's defining frame).
- D — fexpr-typed value as a candidate of a variant.
- E — fexpr-typed value in a lambda's capture list.
- F — fexpr-typed value returned via a constructor.
- G — fexpr-typed value written to a productive or reference parameter of the defining frame.

### E.6 Joint Analysis Composition

The four analyses share the CFG walk and the join points, but their transfer functions are independent. The composed analysis runs as:

```
state ← state_vector_at_entry
for each statement s in CFG order:
    state.init  ← init_transfer(s, state.init)
    state.failure ← failure_transfer(s, state.failure)
    state.readTaint ← read_taint_transfer(s, state.readTaint)
    state.fexprTaint ← fexpr_taint_transfer(s, state.fexprTaint)
    
    on CFG branch (multiple successors):
        propagate state to each successor

at each convergence point:
    state ← (init join, failure join, read_taint join, fexpr_taint join)
            of all predecessor states (component-wise)
```

The components do not interact at transfer functions except at specific cross-component boundaries:

- The `.fail` statement updates `failure` and may propagate READ-taint (if the failure payload's value is READ-tainted, the failure is marked accordingly for downstream recovery analysis).
- A productive write `'r <- expr` updates `init` and verifies READ-taint conformance simultaneously (a tainted RHS rejected if `'r` is a productive write through a READ-rooted path).

### E.7 Reachability Rider

Every conformance check applies only to **reachable** program points. A statement after an unrecoverable `failing(!)` state is unreachable per the lattice and contributes no conformance requirement.

The reachability is computed alongside the CFG walk: a node is reachable if any incoming edge has `clear` or `mixed(?)` or `mixed(!)` state at the predecessor.

A body whose every exit edge is unreachable (a non-terminating body) trivially satisfies every conformance rule — the universal quantification over reachable exits has an empty domain.

### E.8 Worked Example

Consider the factorial subcommand:

```
.cmd factorial: Int n, Int 'result =
    .sub factorialAcc: Int n, Int acc, Int 'r =
        ?: n == 0
            'r <- acc
        factorialAcc: n - 1, n * acc, 'r
    
    factorialAcc: n, 1, 'result
```

The CFG for `factorialAcc`'s body has three nodes:

1. **Entry.** `n: init/untainted/untainted`, `acc: init/untainted/untainted`, `'r: uninit/untainted/untainted`.
2. **After `?: n == 0` body (the `'r <- acc` branch).** init: `'r` becomes `init`. failure: `clear`. taint: untainted.
3. **After `factorialAcc: n - 1, n * acc, 'r` (the default-arm branch).** init: the call's productive arg `'r` becomes `init`. failure: `clear` (the call succeeded). taint: untainted.

At the **convergence (exit) point**, both predecessor states have `'r: init/clear/untainted`. The join is `'r: init/clear/untainted`.

Conformance:

- init: `'r` is `init` at every reachable exit ✓ (write-once obligation satisfied).
- failure: exit is `clear`; the body's mark is `:` (never-fails); the declared mark allows `clear` ✓.
- READ-taint: no READ-tainted slots written ✓.
- fexpr-relevance: no fexpr-typed slots in scope ✓.

The body typechecks.

### E.9 Composed-Analysis Performance

The joint state-vector grows linearly with the number of named slots in scope; each component lattice contributes its per-slot dimension. For a typical command body with O(10) slots, the state vector is small enough that the analysis terminates in O(N · V) time where N is CFG node count and V is the per-state-vector size — well within compile-time budgets for any practical body.

Loop bodies (via `^` rewinds) require fixpoint iteration: the lattice's finite height bounds the iteration count to O(slots × max-lattice-height) per loop. The lattices' heights are small (init: 3, failure: 6, taint: 2), so fixpoint convergence is fast.

---

## Appendix F. Operational Semantics

This appendix formalizes the operational semantics sketched in §1.3, elaborating each reduction rule with full transfer-function detail and integrating the structural rules introduced throughout the body.

### F.1 The State Tuple ⟨V, Φ, Σ⟩

Program state at any reduction step is a triple:

- **V** — the *current verb*, i.e., the next reduction step to apply. The verb category includes user commands `exec(c)`, the failure-firing verb `fail(φ)`, the recovery markers `recover` and `recover(φ, σ, c)`, the scope boundary markers `scope(c)` and `scopefail(c)`, and the loop rewind verb `rewind(v)`. The notation `→v` denotes the continuation that runs after v completes.

- **Φ** — the *failure register*. Holds the value ε when no failure is in flight; holds a failure value `φ` when a failure is propagating. `φ` is a triple `(message, payload-pointer, witness)` where:
  - `message` is the failure message's identifier (per §4.1).
  - `payload-pointer` is either null (for payload-less messages) or a pointer to the payload value's storage.
  - `witness` is either null (for payload-less messages) or a pointer to the typeclass dictionary for the (concrete-payload-type, message's-payload-class) pair (§4.7).

- **Σ** — the *variable state*. A mapping from in-scope names to slot identities and contents, partitioned by frame. The notation `σ/c` denotes σ bound within the lexical scope of the verb c.

A program executes by repeated application of reduction rules; the rules collectively transform ⟨V, Φ, Σ⟩ to a new triple ⟨V', Φ', Σ'⟩.

### F.2 Reduction Rules

The reduction rules are presented in the form `⟨V, Φ, Σ⟩ → ⟨V', Φ', Σ'⟩`. The rules form a small-step semantics.

**R1 — Sequential composition.** A verb `c₁; c₂` (semicolon-separated, or block-marker-sibling-separated) reduces by running c₁ first; if Φ remains ε, control proceeds to c₂; if Φ becomes non-ε (a failure fires), c₂ is skipped via the failure-skip rule.

```
⟨c₁; c₂, ε, Σ⟩ → ⟨c₁ →c₂, ε, Σ⟩
⟨c₁; c₂, φ, Σ⟩ → ⟨c₂, φ, Σ⟩         (failure-skip)
```

**R2 — Successful exit of a verb.** When `c₁` reduces fully without producing a failure, the continuation runs:

```
⟨c₁ →v, ε, Σ⟩ → ⟨v, ε, Σ⟩            (when c₁ has fully reduced and Φ = ε)
```

**R3 — Failure firing (`.fail`).** The `.fail Name: payload` directive populates Φ:

```
⟨.fail Name: payload, ε, Σ⟩ → ⟨ε, (Name, &payload, W), Σ⟩
```

where `W` is the witness selected at the `.fail` site for the (concrete-payload-type, Name's payload class) pair, and `&payload` is the pointer to the payload's storage. For payload-less messages, the second and third components are null.

**R4 — Failure propagation through siblings.** With Φ non-ε, the next ordinary statement at the same indentation level is skipped:

```
⟨c, φ, Σ⟩ → ⟨ε, φ, Σ⟩                (the next statement is skipped; control still advances)
```

(The rule is implicit in R1: a non-ε Φ causes subsequent c's to skip.)

**R5 — Scope boundary.** Entering a recovery context (a block-marker construct with a body) introduces a scope verb `scope(c)`; exiting it produces a scopefail or scoperestore based on whether the body's failure was consumed.

```
⟨scope(c), ε, Σ⟩ → ⟨c →scopepop, ε, Σ⟩
⟨scope(c), φ, Σ⟩ → ⟨recover(φ, Σ, c) →scopepop, ε, Σ⟩    (recovery engages)
```

**R6 — Recovery engagement.** A `|`-with-spec block engages on a propagating failure whose message matches the spec:

```
⟨recover(φ, Σ_pre, c), ε, Σ⟩ → ⟨c[binding := φ's payload], ε, Σ ∪ {binding}⟩
                                              if φ.message ≤ spec
                                              otherwise → ⟨ε, φ, Σ_pre⟩  (propagate past)
```

**R7 — Guard-bearing block engagement.** A `?`, `?-`, or `?:` block runs its guard; the body engages based on guard outcome:

```
⟨? guard body, ε, Σ⟩ → ⟨guard →when(body), ε, Σ⟩
⟨when(body), ε, Σ⟩ → ⟨body, ε, Σ⟩           (guard succeeded)
⟨when(body), φ, Σ⟩ → ⟨ε, ε, Σ⟩              (guard failed; failure consumed)
```

Analogous rules for `?-` (engage on guard failure), `?:` (chain semantics: first guard to succeed engages, chain exits).

**R8 — Rewind.** A `^` block re-enters the preceding sibling on body success:

```
⟨^ body, ε, Σ⟩ → ⟨body →rewind_to_preceding, ε, Σ⟩
⟨rewind_to_preceding, ε, Σ⟩ → ⟨preceding_sibling →^body, ε, Σ⟩
⟨^body's body, φ, Σ⟩ → ⟨ε, ε, Σ⟩      (body failed; loop exits)
```

**R9 — Frame entry.** A command call introduces a new frame. The callee's frame is allocated; arguments are copy-restored into the callee's slots per their modes (§6.4). The current frame's slots remain in scope but are not directly accessible to the callee.

**R10 — Frame exit (success).** When a command body reaches a `clear` exit, the frame retires. `@`-blocks and `@!`-blocks registered against this frame fire in reverse registration order. After all blocks fire, the frame's storage is reclaimable.

```
⟨frame_exit, ε, Σ⟩ → fire @-blocks in reverse → reclaim frame → ⟨..., ε, Σ_caller⟩
```

**R11 — Frame exit (failure).** When a command body reaches a `failing` exit, the failure propagates. `@`-blocks fire (every-exit) and `@!`-blocks fire (failure-only) in reverse order, then the failure continues propagating to the caller's frame.

```
⟨frame_exit, φ, Σ⟩ → fire @-blocks and @!-blocks in reverse → propagate φ → ⟨..., φ, Σ_caller⟩
```

The originating-frame deferred-retirement rule of §4.12 applies: the frame holding the payload value cannot retire until consumption, but the `@` and `@!` blocks fire at the failure-exit moment (before consumption).

### F.3 Frame Model

A *frame* is the operational unit corresponding to a single command invocation. Each frame has:

- **Slot storage** — bytes for the frame's parameters, locals, and other named storage.
- **Block-marker registration list** — the `@` and `@!` blocks registered within this frame, in order of registration.
- **Failure slot** — three words holding any in-flight failure originating from this frame.
- **Caller link** — a pointer to the calling frame, for control return.

Frames are allocated on the call stack at frame entry and retired at frame exit. Retirement reclaims the slot storage; deferred retirement (§4.12) postpones reclamation when the frame is the originating frame of an in-flight failure.

### F.4 The Failure Slot

The failure slot is a fixed-size three-word structure populated at `.fail` and consumed at recovery:

- **Word 1: Message identifier.** A small-integer tag identifying which message type is in flight. Resolved at compile time to a unique-per-program identifier; the message-hierarchy descent rules (§4.9) use this identifier directly.
- **Word 2: Payload pointer.** Pointer into the originating frame's storage; the payload value's address. Null for payload-less messages.
- **Word 3: Class witness.** Pointer to the typeclass dictionary for the (concrete-payload-type, message's-payload-class) pair. Constructed at compile time and emitted at the `.fail` site. Null for payload-less messages.

Failure propagation copies the three-word slot up the call stack without moving the payload value itself. The payload stays put in the originating frame's storage until a recovery handler binds it, at which point the value moves into the recovery frame.

### F.5 Holding-Frame Discipline

A payload value's *holding frame* is the frame whose slot storage currently contains the value. The holding-frame model (§4.11):

- **Initial holding frame.** When `.fail Name: payload` fires, the payload value resides in the firing frame's slot storage. That frame becomes the holding frame.
- **Propagation.** As the failure propagates up the call stack, the holding frame does *not* change — only the failure-slot triple (message, pointer, witness) is copied. The payload value stays in its originating frame's storage.
- **Binding event.** When a `|`-with-spec recovery engages, the bound payload value moves from its originating frame to the recovery frame. The holding frame becomes the recovery frame; the originating frame's payload storage is now invalid for this value (the move is the value's transfer).
- **Re-fail event.** When a recovery handler re-emits the payload as a fresh failure (§4.10), the value moves into the new originating frame. The holding frame becomes the new originating frame.
- **Consumption event.** When a recovery handler completes without re-failing, the value is consumed; the holding frame retires normally.

The model describes the value's location across the failure-flow path. The `@` and `@!` block-marker registrations remain frame-bound (per §3.13, the block markers are not value-bound).

### F.6 The Frame-Exit Hook Discipline (Block-Form Only)

`@`-blocks and `@!`-blocks registered within a frame's body fire at that frame's retirement (§3.13). The discipline:

- **Registration time.** A `@ body` or `@! body` block at the source level adds the block to the current frame's registration list. The block is registered at the point of execution flow, not at the body's source-level declaration: a `@ body` inside a conditional is registered only if the conditional engages.
- **Firing order.** At frame retirement (success or failure exit), blocks fire in *reverse registration order*. The most-recently-registered block runs first.
- **Failure-exit filtering.** `@!` blocks fire only when the frame exits via failure (Φ non-ε at exit). `@` blocks fire on every exit.
- **No value-attached firing.** Frame-exit hooks are not tied to any value's lifetime; they are tied to the frame's retirement. A value that has moved out of the frame (via failure-payload move, etc.) is not in the registration list's responsibility.

### F.7 The Single-In-Flight Invariant

At most one in-flight failure exists per thread at any moment (§4.12). This is maintained by:

- `.fail` is valid only when Φ = ε at the firing point. A propagating failure would have failure-skipped past the `.fail` site already, so the case where Φ is non-ε at a `.fail` site doesn't arise in well-formed code.
- A recovery `|`-block consumes the in-flight failure before any new statement (including a new `.fail`) is reached in the handler body. So Φ is back to ε when the handler body runs.

Multi-threaded programs may have one in-flight failure per thread; the invariant is thread-local.

### F.8 Object Lifetime Ceiling

An object's lifetime is bounded by its *introducing frame* (§5.11):

- The frame in which an object's storage is introduced is the object's lifetime ceiling.
- A `^Object` parameter passed downward gives the callee access; the callee does not become the owner.
- A productive `^Object` parameter lets the callee swap which object the caller-owned slot points at, but the new object is allocated into the caller's frame on successful copy-restore.
- Transitive containment: an object embedded as a field of another object inherits the containing object's lifetime ceiling.

At frame retirement, every object whose lifetime ceiling is this frame is reclaimed.

### F.9 Variant Three-Word Slot

A variant slot occupies three words (§5.12):

- **Word 1: Tag.** A small-integer identifier for the active candidate or for the absent state.
- **Word 2: Candidate pointer.** Pointer to the active candidate's storage. Null when the variant is in the absent state.
- **Word 3: Class witness.** Pointer to the witness for the active candidate's class participation (when applicable). Null when no class dispatch is engaged for this variant.

A variant's bytes are *not* a value-copyable byte aggregate — the candidate-pointer references shared storage (§7.6). Variant assignment uses the constructor form `${...}` or shares access via `^Variant` pointer.

### F.10 Class-Typed-Value Three-Word Slot (Case B)

An existential class-typed parameter slot (§9.9 Case B) carries a three-word representation:

- **Word 1: Tag.** Identifies the runtime type of the held value.
- **Word 2: Value pointer.** Pointer to the value's storage.
- **Word 3: Class witness.** Pointer to the (runtime-type, declared-class) instance dictionary.

The witness is selected at the construction site (where the value is converted to its class-typed slot); the tag is the runtime type's identifier. Dispatch through the slot consults the witness's method table.

### F.11 Frame-Exit Hook Firing Sequence

The full firing sequence at frame retirement:

```
1. Identify retirement type (success or failure exit).
2. For each registered block in reverse registration order:
   a. If block is @, fire its body.
   b. If block is @! and retirement is failure-exit, fire its body.
   c. (If block is @! and retirement is success-exit, skip.)
3. Reclaim slot storage (unless deferred-retirement state is active per F.7).
4. Return to caller's frame.
```

A failure during a block's body propagates per the standard rules — the block's body is itself a frame-bound context. The single-in-flight invariant (F.7) prevents a block-fired failure from coexisting with the outer failure: blocks fire from a no-active-failure state, and any failure they fire is consumed locally or propagates to the caller's frame after the outer failure has already advanced.

### F.12 Per-Invocation Fexpr Frame (`F`)

The D/I/F three-frame model of §8.5 specifies a *virtual* per-invocation frame `F` for each fexpr invocation:

- `D` — the defining frame (where the fexpr was constructed). Holds the slots the fexpr's body accesses by free-name resolution.
- `I` — the invoking frame (where the fexpr is invoked). May be `D` itself or any deeper frame.
- `F` — the fexpr-execution frame. A virtual sub-frame within `D`'s scope; the fexpr's body executes as if inlined at `D` at the invocation site `I`.

The operational mechanism: at fexpr invocation, the fexpr's body runs against `D`'s state with its free names resolving to `D`'s slots. `F` is virtual — it does not allocate fresh slots; it shares `D`'s storage. The body's effects are mutations of `D`'s state at `I`'s execution point.

### F.13 Witness Construction

At a `.fail` site (or at any class-typed value construction site), the class witness is selected at *compile time* from the visible instances at that point in the source. The witness construction:

1. The typechecker has the concrete payload type `T` (from the expression supplied at `.fail`).
2. The typechecker has the message's payload class `C` (from the message declaration's `[PayloadType]` clause).
3. The typechecker locates the visible instance `T : C` declaration and selects its dictionary.
4. The dictionary pointer is emitted as the third word of the failure slot at the `.fail` site.

No runtime witness construction is required. The covariance rule for payload classes (§9.17) ensures that even when a failure travels up a hierarchy of messages with related payload classes, the witness selected at `.fail` time supports the broader-class operations through standard class-system subsumption (the witness's dictionary contains all parent-class methods).

### F.14 Operational Summary

Basis's operational semantics is small: 11 reduction rules, four slot structures (failure, variant, class-typed Case B, frame's failure slot), one lifecycle table (frame entry/exit and retirement). The model is conservative — no implicit dispatch, no hidden control flow, no garbage-collection daemons. Every reduction step is locally determined by the current verb, the failure register, and the lexical state — meeting the no-non-local-state and no-hidden-control-flow commitments of §1.4.

---

## Appendix G. Identifier Resolution and Name Binding

This appendix specifies how names are resolved at use sites: which scope holds a given binding, in what order scopes are searched, and how the language's identifier-shape distinction interacts with the resolution algorithm.

### G.1 Lexical Scope

Basis uses *lexical* scoping: a name's binding is determined by the source structure surrounding the use site, not by the call stack at runtime. Scopes are introduced by:

- **Module scope.** Each module forms a top-level scope containing all its top-level declarations.
- **Command body scope.** Each command body (including subcommand bodies, class-method bodies, lambda bodies, fexpr bodies) forms a scope that contains the command's parameters, implicit context parameters, and any locals introduced via `#name` placement.
- **Block-marker body scope.** Each block-marker construct (`?` body, `?:` body, `|` body, `@` body, etc.) introduces a sub-scope inheriting from its enclosing scope. Names introduced inside a block-marker body are visible only within that body.
- **Subcommand body scope.** A subcommand's body has its own lexical scope per §3.12; the subcommand does *not* capture from the enclosing command's scope.

A scope holds a set of `(name, type, mode)` triples. Multiple bindings of the same name in the same scope is a static error (§6.3's same-scope rule for identifier shapes specifically).

### G.2 Identifier-Shape Distinction

A name binding in a scope is associated with an identifier shape per the mode of the binding:

- READ binding: the name resolves as `name`.
- PRODUCE binding: the name resolves as `'name`.
- REFERENCE binding: the name resolves as `&name`.

The lexer emits one of these three identifier-token-shapes per use site (per A.2). The resolution algorithm searches for a binding whose identifier shape matches the token shape at the use site.

**Same-scope rule (§6.3).** A scope may contain at most one binding of any given name across the three identifier shapes. Introducing `x` and `'x` in the same scope is a static error: the two refer to the same logical name with conflicting mode contracts, and the language refuses to admit the ambiguity. The rule prevents the bug-prone pattern of having the same name available in two different modes within a single body.

The rule applies to bindings *within a single scope only*; nested scopes may rebind the name at different modes if needed (shadowing).

### G.3 Same-Scope Rule Enforcement

At each scope's construction (during parsing or during typechecking, depending on implementation), the typechecker verifies the same-scope rule:

```
For each scope S:
    For each pair of bindings (b1, b2) in S:
        If b1.name == b2.name (regardless of identifier shape):
            Static error: name 'X' bound multiple times in same scope
```

The check operates on the underlying name (after stripping any `'` or `&` prefix); the three identifier shapes are recognized as the same name.

### G.4 Name Resolution at Use Site

At each use site, the typechecker searches scopes in the following order, returning the first binding found:

1. **Current lexical scope.** The scope of the enclosing block-marker body, command body, or subcommand body.
2. **Enclosing scopes.** Each successively outer scope, walking up the lexical-nesting chain.
3. **Module scope.** Top-level declarations within the current module.
4. **Imported module scopes.** Names imported via `.import` declarations (see §2.4 and Appendix H.2).

For subcommands (§3.12), the resolution chain skips the enclosing command's body scope when resolving a name used inside a subcommand body — the subcommand does not capture. The chain instead goes from the subcommand's own scope directly to:

- Sibling subcommands' names (siblings are reached through the enclosing body's sibling-subcommands set).
- Enclosing subcommand bodies' siblings (if nested).
- Outermost enclosing command's module scope.
- Imported modules.

The skip is per §3.12: subcommands "do not capture from the enclosing frame; they get their inputs from their formal parameters."

### G.5 Type-Name Resolution

Type names (uppercase initial) are routed to a parallel namespace from value-identifier names (lowercase initial):

- The lexer distinguishes type-name tokens from value-identifier tokens at lex time (per A.3).
- Type-name resolution searches the same scope chain but matches against type declarations only.
- A type name in expression position (after a `:` in a type expression, or before `::` in a method call) resolves to a `.class`, `.domain`, `.record`, `.union`, `.object`, `.variant`, `.alias`, `.enum`, or `.msg` declaration.

Module-qualified type names `Module::TypeName` route through the imported-module's namespace per §2.4 / Appendix H.2.

### G.6 Class-Name Resolution

Class names are type names with the additional admittance at parameter positions for the `(T:Class)` constraint form (§9.9) and as standalone parameter types for the existential class-typed-parameter form (Case B of §9.9):

- In a `(T:C)` constraint, `C` resolves as a type name to a `.class` declaration.
- As a parameter type (without constraint), a class name `C` resolves as a type name; the parameter's runtime representation is the three-word slot (Case B per §9.9, F.10).

The class-name namespace is shared with the type-name namespace — a name like `Showable` is both a type expression and a class. Distinguishing class declarations from non-class type declarations is by checking the declaration kind at the resolution site.

### G.7 Method-Name Resolution Under `::`

The `::` operator is the scope operator (§1.5). Its role at name resolution depends on the LHS:

- **Object/class-typed receiver.** `obj :: methodName` resolves `methodName` in the namespace of the class(es) `obj`'s type is an instance of. If multiple classes contain a method of this name, the resolution is ambiguous; the user disambiguates with `{ClassName :: methodName}` (§9.10, the `{C::method}` form, B.15).

- **Type prefix.** `TypeName :: identifier` resolves identifier as a static member of `TypeName` (e.g., a class method declared via `.cmd ClassName :: methodName: ...` form). In §9.4 this is the top-level form for instance methods.

- **Module prefix.** `ModuleName :: TypeName` (or `ModuleName :: identifier`) resolves through the imported-module's namespace.

- **Aggregate-shaped field access.** `value :: fieldName` or `value :: N` accesses a field of an aggregate-shaped value: an `Aggregate`-typed slot (§7.4), a record (§5.4), or an object (§5.11). For records, both forms account for `.inline` field promotion (§A.7). For objects, positional access is by declaration order.

The disambiguating context for `::` is the LHS's type and the namespace's contents.

### G.8 The `{C::method}` Disambiguation Form

When a method name is overloaded across multiple visible classes, the `{ClassName :: methodName}` form (B.15) explicitly disambiguates which class's method is intended:

```
{Showable :: show}             ; resolves to Showable's show method specifically
{Showable :: show: x}          ; with partial application
```

The form is part of the command-reference family — it produces a command-typed value with the receiver position open for the eventual call. The disambiguation is parser-side; the resulting command-typed value carries the specific class binding.

The form is necessary when:

- A method name like `show` is defined on two visible classes that both apply to a target type.
- The user wants to be explicit about which class's instance dispatches.

In ordinary calls `obj :: show`, the dispatch is by the most-specific class containing `show` that `obj`'s type is an instance of; ties are static errors with the `{C::method}` form as the resolution mechanism.

### G.9 Module-Qualified Names

A module-qualified name `Module::Name` (or `Module::SubModule::Name`) resolves through the import graph:

1. The current module's `.import` declarations specify which modules are visible.
2. The qualified name's leading segment `Module` must match an imported module's name (or its alias if `.import Module as Alias`).
3. Subsequent segments resolve within that module's namespace.

Module-qualified type names share the type-name namespace; module-qualified value-identifier names share the value-identifier namespace. The same `::` operator is used uniformly.

### G.10 Shadowing

Inner scopes may shadow outer scopes' bindings:

```
.cmd outer: Int x =
    .sub inner: =
        # Int x <- 42        ; this 'x' shadows the outer 'x'  
        ; but wait — inner is a subcommand, which doesn't capture, 
        ; so the outer 'x' was never in scope here anyway
        ...
    
    inner:
```

For subcommand bodies, "shadowing" is moot — the outer scope's names aren't visible to begin with. For block-marker bodies and other lexical sub-scopes, ordinary shadowing applies: a name introduced inside the inner scope hides any outer binding of the same name within that scope.

The same-scope rule (G.3) prohibits intra-scope shadowing across identifier shapes; cross-scope shadowing (an inner scope introducing a binding of the same name) is admitted.

### G.11 Forward References

Top-level declarations are forward-referenceable within a single source file — every top-level name is visible throughout the file once declared, regardless of source-position order (per §2.2). This admits:

```
.cmd factorial: Int n, Int 'result =
    helper: n, 1, 'result

.cmd helper: Int n, Int acc, Int 'r = ...      ; declared after factorial
```

The file's top-level declarations are collected into the module scope before name resolution begins. Source-position order within a single file is not significant for visibility.

Cross-file references require `.import` declarations (§2.4); the import statement makes another module's top-level names visible at the current source file.

### G.12 Resolution Failure Cases

A name resolution that finds no binding is a static error: "name `X` not in scope." The error message indicates the scope chain that was searched and which top-level/imported scopes were considered.

A name resolution that finds multiple bindings (e.g., two imported modules both export a name `X`) is also a static error: "name `X` ambiguous between modules `M1` and `M2`." The user resolves with explicit module qualification: `M1::X` or `M2::X`.

A use-site identifier shape that does not match any binding's shape is a more specific error: "no `'x` binding in scope at this position; `x` is bound at READ mode." This catches typos that confuse the user about a binding's mode.

---

## Appendix H. Module System and Instance Visibility

This appendix specifies the module system that links Basis source files into compilation units, the import declaration's full surface, the visibility rules governing what is exported from a module, and the instance-visibility and instance-coherence rules that govern cross-module class participation.

### H.1 Module Declaration

A Basis source file may begin with a single `.module` declaration:

```
.module App::Domain::User
```

The `.module` directive names the module that the file's declarations belong to. The module name is `::`-qualified; segments form a hierarchy where `App` contains `Domain` contains `User`. A file with no `.module` declaration declares an anonymous unnamed module containing the file's declarations only.

The module hierarchy is the structural basis for *module specificity* in the instance-coherence rule (H.5).

### H.2 Import Declarations

Imports admit two surface forms:

**Named-module import:**

```
.import Other::Module
.import Other::Module as Alias
```

The named form makes a previously-declared module's exports visible at the current source file. Without the `as` clause, the imported module's exports are reachable by their declared names with `Other::Module::` qualifier. With `as Alias`, the qualifier becomes `Alias::`.

**File-path import:**

```
.import "some/file.basis"
```

The file-path form admits direct linking by file path; the imported file's module declaration determines the imported namespace. This form is useful for incremental development and for files that aren't yet placed in the module hierarchy.

Multiple imports may be combined:

```
.import Standard::Strings
.import App::Domain::User as User
.import "helpers/internal.basis"
```

Imports are processed at the file's lexical start, before any top-level declarations; cycles in the import graph are a static error.

### H.3 Visibility Rules

Every top-level declaration in a module is exported by default. There is no `private` or `internal` keyword to restrict visibility. The user resolves "private helpers" by either:

- Placing the helper at body-internal scope (using `.sub`, §3.12).
- Placing the helper in a separate module that is not imported by any module outside the intended scope.

The visibility rules in detail:

- A top-level declaration `D` in module `M` is visible in module `N` if and only if `N` has an `.import M` declaration (with or without aliasing).
- Subcommand declarations (`.sub`) are *not* exported — they are confined to their enclosing command's body per §3.12.
- Class members declared with `.decl` (signature-only) are visible to instance writers; class members declared with `.cmd` (default implementations) are visible to instance writers and to method dispatch sites.
- Instance declarations (`.instance T : C`) are visible whenever both `T`'s module and `C`'s module are visible at the current source file (transitive closure of imports).

### H.4 Instance Visibility

An `.instance T : C` declaration in module `M` is visible to any source file whose import graph reaches both `T`'s declaration module and `C`'s declaration module. This admits *orphan instances*: an instance whose `T` is declared in one module and whose `C` is declared in another, with the instance itself declared in a third module that imports both.

The motivating use case is consumer-driven extensibility: a downstream module can adapt an existing type to satisfy an existing class without modifying either's source. Orphan instances are admitted in Basis (per §9.15); the cost is the cross-module coherence question (H.5–H.6).

The import graph is the transitive closure of `.import` declarations. If module `M` imports `T_module` and `C_module`, and the user's current file imports `M`, then `M`'s instance `T : C` is reachable transitively at the current file.

### H.5 Instance Coherence Resolution Algorithm

Multiple visible modules may each declare an `.instance T : C` for the same `(T, C)` pair. The conflict-resolution rule (§9.15): **most-specialized module wins**.

The full algorithm:

1. **Collect candidates.** At a dispatch site where an instance `T : C` is needed, collect all visible modules that declare an instance for `(T, C)`.

2. **Specificity ranking.** Module `M1` is more specific than module `M2` if `M1`'s declaration site is "more specialized" in the module hierarchy. The specificity ordering:

   - **Sub-module is more specific than parent module.** `App::Models::User` is more specific than `App::Models`, which is more specific than `App`. A submodule's instance wins over a parent module's instance when both are visible.
   
   - **Importing-module is more specific than imported-module.** A module that declares an instance for an imported type-and-class pair is more specific than the upstream module that didn't declare it.
   
   - **Co-declared-with-T wins over co-declared-with-C, with both winning over orphan.** If an instance is declared in `T`'s home module, it wins over an instance declared in `C`'s home module, which wins over a third-party orphan.

3. **Uniqueness check.** If exactly one candidate is most-specific (strictly more specific than all others), it is selected. The dispatch succeeds.

4. **Ambiguity error.** If multiple candidates are equally specific (incomparable in the specificity ordering), the dispatch is ambiguous and is rejected with a static error. The user resolves by:
   - Adding an explicit `.instance` declaration in a more-specific module.
   - Using the `{C::method}` disambiguation form (§9.10) to specify which class's method is intended.

5. **Stale-import warning.** A warning is emitted if the import graph causes the ambiguity (rather than a same-module declaration); see H.6.

### H.6 Import-Time Competition Warning

When a module's import graph causes two different upstream modules' instances of the same `(T, C)` to become reachable, an import-time warning fires:

```
Warning: importing M1 and M2 brings two competing instances of (T, C) into scope.
  M1's instance is at <location>.
  M2's instance is at <location>.
  The dispatch will use the most-specific instance per H.5; consider whether the
  composition is intended.
```

The warning is informational — the language admits the import and resolves via H.5. Tooling may provide a suppression mechanism; the surface for it is not specified here.

The motivation for the warning: silent instance composition through transitive imports is a recurring class of bugs in extensible class systems. Surfacing the composition explicitly at import time gives the user a chance to inspect the resolution.

### H.7 Domain-Hierarchy Extension

Domain types (§5.3) admit downstream extension: a downstream module may declare a child of an imported domain.

```
; In module Upstream:
.domain Length : Int32

; In module Downstream:
.import Upstream
.domain Inches : Upstream::Length        ; child of imported domain
```

The implicit-upcast relation (§5.5) is structurally stable across this extension: every Inches subsumes to Length to Int32 to the buffer-backed root. The extension does not widen any other type's upcast set; the asymmetry is one-directional.

### H.8 Failure-Message Hierarchy Extension

A downstream module *may* add new children to an imported failure-message hierarchy (§4.9), declaring children of a foreign root or of any imported descendant. The hierarchy participates in the same cross-module coherence machinery as class instances (§9.15, §9.17).

Payload-class covariance (§4.8, §9.17) keeps the extension safe: a `| SomeRoot t -> ...` handler binds the payload at `SomeRoot`'s payload class and operates on it through that class's operations only. Descendant payload classes are covariant subclasses by construction, so they support those operations; adding new descendants cannot widen what a parent-class handler can observe.

### H.9 Cross-Module Visibility Summary

| What | Where it's declared | What sees it |
| --- | --- | --- |
| Top-level type/class/cmd | Module M | Any module N that imports M |
| Subcommand (`.sub`) | Body of cmd C | Only within C's body and deeper subcommand bodies |
| Class member (`.decl`) | Class declaration in M | Any instance writer; any dispatch site |
| Class member (`.cmd` default) | Class declaration in M | Any instance writer; any dispatch site |
| Instance (`.instance T:C`) | Module N | Any module that has T and C visible |
| Failure message (`.msg`) | Module M | Any module that imports M |
| Failure-message child | Any module that imports parent | Open; downstream extension admitted |
| Domain child | Any module that imports parent | Open; downstream extension admitted |

### H.10 Module-Specificity Algorithm Details

The full algorithm for ranking module specificity (H.5 step 2):

Given two modules `M1` and `M2`:

```
function more_specific(M1, M2):
    if M1 is a submodule of M2: return M1
    if M2 is a submodule of M1: return M2
    
    if M1 contains a declaration of T or C, and M2 does not: return M1
    if M2 contains a declaration of T or C, and M1 does not: return M2
    
    if both contain or neither contains T/C: return incomparable
```

The "submodule of" relation: `App::Models::User` is a submodule of `App::Models`, which is a submodule of `App`. Submodule-of is determined by the `::`-qualified name's prefix relation.

Cross-tree imports (modules in unrelated subtrees of the module hierarchy): both modules are equally specific if neither is a submodule of the other and neither contains T or C. The resolution is incomparable — a static error per H.5 step 4.
