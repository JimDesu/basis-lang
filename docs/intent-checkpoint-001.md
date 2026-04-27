# Basis Language — Intent Checkpoint 001

**Status:** Working draft, supersedes the repository README where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Distilled from a one-on-one intent dialog between the language designer (JimDesu) and Claude (Opus 4.7) on 2026-04-26, conducted as preparation for spec-driven development of the Basis compiler. Each section below records intent that the designer explicitly confirmed during that dialog. Claims that have not been confirmed are marked as inferences or open questions.

**Authority statement.** Where the repository README and this document differ, this document is authoritative. Where the source code (Productions.h, Grammar2.h/cpp, Parsing2.h, Token.h, Lexer.h, Ast.h, AstBuilder.h/cpp) and this document differ, **the code is authoritative for syntactic and structural matters**, and this document is authoritative for semantic intent. The file `language_notes.txt` is historical only and should not be relied upon. The grammar2.md file has been removed from the repository.

---

## 1. Foundational Design Principles

Basis is a general-purpose imperative programming language whose design naturally facilitates hexagonal architecture: the language's semantics make clean separation of concerns and absence of non-local state the path of least resistance, rather than a discipline imposed on top.

The principles below are stated in their confirmed forms; some have been refined or qualified during the intent dialog and are presented here as they should be read going forward.

**Strong typing.** All types are checked statically. There are no privileged primitive types: `Int32`, `UInt32`, `Float32`, etc. are standard-library domains over byte-buffers. Intrinsic functions exist for built-in types, but these types are not privileged in the type system itself; user-defined types operate on equal footing. Anything the user adds within the language's rules must function as well as the built-in types.

**No non-local state access.** Commands cannot arbitrarily access or modify external state. State that a command operates on must be passed in as a parameter, including indirect state reachable through pointers, objects, and similar reference-bearing values. Commands themselves may be referenced freely (passed around, stored, captured) — the restriction applies to *state*, not to *commands*. The exceptions are `.test` and `.program`, which are entry points outside command scope. The practical consequence is that **a command's parameter list is a complete inventory of its potential effects on the universe**.

**The fundamental datatype is a buffer.** Buffers are encapsulated by the bracket syntax: `[]` is an unbounded byte buffer, `[N]` is an N-byte buffer, `[]T` and `[N]T` are typed views over the corresponding byte storage. There is no separate `Buffer` keyword or `BufferType` AST node; the bracket form *is* the buffer concept. Named scalar types like `Int32` are domains (refinements with their own identity and operations) over `[N]`.

**Mutation either succeeds fully or fails fully.** This is not enforced by a special transactional mechanism but falls out of call-by-copy-restore semantics for INOUT (writeable) parameters: the restore step is skipped on failure, leaving caller slots bit-identical to their pre-call state. There is therefore no observable partial-mutation window.

**No hidden control flow.** Failures propagate, but only along structurally visible paths. There are no exceptions in the C++/Java sense. Indirection through dispatch is explicit (the `::` syntax) or visible at the call site. The programmer can read source code and know what executes when, and can reason about effects from signatures alone.

**Failure semantics are first-class.** Failures are neither exceptions nor error codes. A failure carries a value (designated `Φ` in the operational semantics). Failures propagate by skipping subsequent commands until they reach a structurally-marked recovery context. The static type system tracks which commands may fail, must fail, or never fail.

**Syntactically significant whitespace.** Layout (indentation) is part of the grammar. The lexer's bracket/brace/indent stacks bind tokens to their closing partners, which is what allows the combinator-based parser's `bound`/`boundedGroup`/`exclusiveGroup` constructs to operate.

**Command-based execution model.** There is no statement/function dichotomy. Commands take parameters, may produce output through writeable parameters, may fail (with the specific failure-mode declared in the signature), and compose. Commands are first-class values: they can be referenced, stored, partially applied, and invoked indirectly.

**Formal operational semantics.** The language is given an operational semantics over a state tuple `⟨V, Φ, Σ⟩`, where V is the value environment, Φ is the failure register, and Σ is the store. The README's operational semantics block remains valid and authoritative for the reduction rules; this document does not duplicate them.

**Native compilation as the long-term target.** Bootstrapping alternatives are acceptable in the interim. The IR design is open (issue #24).

**Inspirations:** Zig, Julia, Scala, Icon, Haskell, Kernel.

---

## 2. The Type System

### 2.1 Buffers and Ranges

The bracket form `[N]` denotes an N-byte buffer. The form `[]` denotes an unbounded byte buffer. The form `[N]T` and `[]T` denote buffers laid out as a sequence of values of type T, sized either to N or unbounded. Buffers are the substrate over which all value-like types are interpreted.

A `[4]` is four bytes — nothing more. What those four bytes *mean* (a 32-bit integer, an RGBA pixel, a packed pair of int16, a Unicode code point) is determined by the **domain** layered on top of it. There is no privileged interpretation of any byte width.

**Indexing into a buffer-shaped value uses the suffix `[index]` syntax**. C-style pointer arithmetic is not supported: one cannot increment a pointer to step through elements. Indexing through `[i]` is the only mechanism for stepping through buffer contents, and indexing is failable (out-of-bounds is a first-class failure, not undefined behavior). Indexing on a domain works because domains are themselves buffers; indexing is not, however, *guaranteed* to be meaningful for every domain — domain-specific operations may forbid it.

### 2.2 Domains

A domain is a **new type** declared in terms of a parent type, where the parent must reduce (transitively, through any aliases) to a value-like type — a named domain or a range form. Pointers, command-typed values, objects, and variants cannot be domain parents because the purpose of a domain is to give a refined interpretation to a definite chunk of bytes.

Domains form a parent–child hierarchy. The relationship is **one-directional implicit upcasting**: a value of a child domain is implicitly accepted wherever the parent domain is expected, but a value of the parent domain is *not* implicitly a value of any specific child. So an `Inches` is implicitly an `Int32`, but an `Int32` is not implicitly an `Inches`. Sibling domains (e.g., `Inches` and `Centimeters` both parented at `Int32`) do not implicitly convert to each other.

Domains are first-class types: they can be parameters, fields, return values (via the `-> name` mechanism), receivers of class instances, and so on. Class dispatch on a domain resolves to the domain's instance, not the parent's instance (subject to OQ-5 sub-point (c)).

**Notable consequence:** because there are no privileged primitive types, the standard library defines `Int32`, `UInt32`, `Float32`, `Int8`, etc. as domains over `[N]` for the appropriate N, with associated intrinsics for arithmetic and comparison. User-defined domains use the same mechanism — there is no conceptual distinction between the standard library's domains and a user's domains.

### 2.3 Aliases

An alias is a **synonym** — purely a human-ergonomics tool. The alias name and the right-hand side are interchangeable in both directions in all contexts; aliases erase entirely from the type system's perspective. They introduce no new type identity.

This makes aliases useful as **type-level abstraction barriers**: a module can expose `alias UserId: Int64`, downstream code uses `UserId` everywhere, and a later implementation change to `[16]` (a UUID) requires changing only the alias declaration — assuming the change is otherwise compatible. This is a real architectural tool, not just sugar.

Aliases compose with domains: a domain's parent may be an alias (which transparently resolves to the underlying type). This would be an edge case but is permitted.

### 2.4 Records and Objects

Records and objects share a parallel surface syntax (named, field-based aggregates) but are semantically distinct in ways that propagate through the rest of the type system.

**Record:** a *contiguous, byte-addressable buffer* with named field offsets. Value-like semantics: copyable as bytes, no identity beyond byte-content, fields laid out at deterministic offsets. Record fields are constrained to *value-like* types (the `TYPE_EXPR_DOMAIN` form in the grammar), reflecting the requirement that every field have a definite byte width and offset. Records are the natural choice when you want a struct-of-bytes that can sit inside another buffer or be passed as a value.

**Object:** a *stack-or-heap-allocated, identity-bearing aggregate*. Its fields may be discontiguously laid out. Object-typed values are reference-semantics in the sense that they're normally manipulated through (fat) pointers to the object rather than as bytes. Object fields can be arbitrary types including pointers, command-typed values, other objects, and variants.

**Fat pointers and runtime type information.** Records are intended to carry runtime type information through fat pointers when accessed via pointer. This enables safe downcasting and union/variant discrimination. (See OQ-1 for the related question of where union discriminators live.)

The Record/Object split parallels the Union/Variant split (Section 2.5) exactly: value-like aggregates with `TYPE_EXPR_DOMAIN`-restricted parts, vs. reference-bearing aggregates with full `TYPE_EXPR` parts. The same pattern, twice.

### 2.5 Unions and Variants

A **union** is a *byte-level discriminated overlay*. Its candidates are all domains over byte-buffers, the union itself occupies max(candidate-sizes) bytes (plus a discriminator), and its identity is value-like. Two unions with the same tag and the same byte content are the same value. Unions go inside records, get copied by value, fit in buffers.

A **variant** is an *object-level tagged sum*. Its candidates may be anything (objects, pointers, command types, even other variants). The variant itself is reference-semantics. The relationship is exactly Record:Object :: Union:Variant, and the grammar enforces this with the same `TYPE_EXPR_DOMAIN` vs. `TYPE_EXPR` distinction in candidate types.

The discriminator must prevent the synthesis of incorrectly-typed pointers to a union — a pointer-cast that lies about which candidate the bytes represent must not be expressible. The mechanism that achieves this is OQ-1.

### 2.6 Pointers

Pointer types are written `^T`. The carat is a type-prefix and may stack: `^^T` is a pointer to a pointer to T. A pointer's *expression-position* operators are the `^` suffix (dereference) and the `&` suffix (address-of).

The language commits to **abstracted pointer semantics**. The user-visible meaning of `^T` is "pointer to T." Whether the runtime implementation is a thin pointer, a fat pointer (with RTTI), or a handle (a pointer to a pointer, for relocation support) is implementation-decided per type, not user-visible. Handles may turn out to be the right implementation under the hood; this should not be apparent to language users.

**No C-style pointer arithmetic.** Stepping through buffer contents requires the indexing syntax `p[i]`. Incrementing a pointer to advance through an array is not supported. Indexing produces a first-class failure on out-of-range access; pointer arithmetic would not. This restriction is also necessary for the handle implementation to be valid.

**`^Object` is essentially a double-pointer.** Object-typed values are themselves indirection (objects are stack/heap-allocated with non-contiguous fields, accessed via fat pointers). A `^Object` parameter therefore points to a *slot* containing an object reference, and a writeable `^Object` parameter allows the callee to point that slot at a different object on success.

**Pointers and the no-non-local-state principle.** Reading from `^T` and writing to `^T` *do not* violate the no-non-local-state principle. The principle restricts what *names* a command can resolve, not what reachable state a command can touch. State reachable through a parameter is, by construction, state the caller granted access to. The principle is about plumbing, not about reachability.

### 2.7 Command Types

Command types are written `:<...>`, `?<...>`, or `!<...>` for never-fails, may-fail, and must-fail commands respectively. The argument list inside the angle brackets specifies the parameter types, with the apostrophe `'` marker indicating writeable (INOUT) parameters.

Command-typed values are first-class. They can be:
- Stored in variables and fields
- Passed as parameters
- Returned (through writeable parameters via the `-> name` mechanism)
- Captured by command literals and quotes
- Partially applied via the class-resolution operator `::`

This first-class-ness is what makes the optimization in Section 3.4 possible: a class-method dispatch can be hoisted out of a loop by binding the resolved command to a local variable.

---

## 3. Commands

### 3.1 Signatures

A regular command signature is `cmd-name : parm-list / implicit-parm-list -> result-name`. The result-name is the optional `-> name` clause; the `/` separates regular parameters from implicit (context) parameters. The command's failure mode is declared in its name-spec via the `?` (may-fail) or `!` (must-fail) prefix; absence of either means never-fails.

A v-command signature has receivers in front: `(recv1, recv2, ...) :: cmd-name : parms / iparms -> result`. Constructors and destructors use receiver-bearing forms with the `@`/`@!` markers. (See Section 5.)

### 3.2 Parameter Passing — IN vs INOUT

Parameters are either IN (default) or INOUT (apostrophe-marked). Both kinds can be freely overwritten *inside* the command body — within the command, a parameter is just a local variable. The IN/INOUT distinction governs only what happens at the call boundary.

**IN parameters: call-by-value.** The callee receives a copy. Mutations to the callee's local are not visible to the caller. After the call returns (success or failure), the caller's slot is bit-identical to its pre-call state.

**INOUT parameters: call-by-copy-restore.** The callee receives a copy. On *successful* return, the (possibly-modified) copy is written back to the caller's slot. On *failure*, no write-back occurs; the caller's slot remains bit-identical to its pre-call state.

This is the mechanism by which "mutation either succeeds fully or fails fully" is realized: failure-atomicity falls out of copy-restore for free, with no separate transactional machinery and no rollback code in user programs.

The pointer case composes cleanly: the *pointer value* itself is what's copied (IN) or copy-restored (INOUT), not the pointee. A writeable `^T` lets the callee swap which T the caller's slot points at; a non-writeable `^T` does not (though the callee can still read from and write through the pointed-to storage during the call, subject to whatever invariants the type system enforces).

**Implementation latitude:** see OQ-2.

### 3.3 The `-> name` Result Designator

A command's *true* return is its success/failure status (with failures carrying their `Φ` value). Commands do not have value-typed return slots in the C/Haskell sense. Output flows through writeable (INOUT) parameters, full stop.

The `-> name` clause is **syntactic sugar** to make commands usable in expression position without forcing the caller to introduce named temporaries. When `do: x, y -> r` is in scope and the caller writes `# r <- (do: x, y)`, the desugarer rewrites to:

1. Introduce `r` (with default initialization for its type — see OQ-4).
2. Pass `r` as the writeable parameter selected by `-> name`.
3. Execute the call.
4. On success, copy-restore writes back into `r`. On failure, no restore; `r` retains its default-initialized value.

The well-formedness rule is that the name on the right of `->` must be one of the command's parameters or receivers — full stop. It is *not* required to be writeable (see OQ-3 for the question of what designating a non-writeable parameter means semantically).

**Implicit `-> name`:** if a command has no explicit `-> name` clause but has *exactly one* writeable parameter, that parameter is implicitly the expression-position result. Commands with no writeable parameters, or with multiple writeable parameters and no explicit `-> name`, cannot be used in expression position; the caller must use the explicit form.

### 3.4 Commands as First-Class Values

Commands are values. They can be bound to variables. The resolution of a class-method or v-command dispatch produces a command-typed value with the receiver(s) baked in. This enables a programmer-visible optimization for tight loops:

```
# logFn <- (logger :: log)        ; one dictionary lookup
... loop ...
   logFn: "iteration " + i        ; direct invocation, no dispatch
... end loop ...
```

The dispatch occurs once when `logFn` is bound; thereafter `logFn` is invoked directly. The compiler may perform this hoisting as an optimization, but the language gives the programmer the explicit lever.

This pattern is safe when the receiver doesn't change *in a way that affects dispatch* over the loop's lifetime. The programmer holds the discipline; the language enables the move.

### 3.5 Implicit (Context) Parameters

The `/` separator in a command signature divides regular parameters from **implicit context parameters**, which are intended to behave like Scala's implicit parameters. At a call site, an implicit parameter is automatically supplied from the caller's lexical scope when *exactly one* in-scope value matches the parameter's type. Ambiguity (multiple in-scope matches) and absence (no match) are both call-site errors that the user can resolve by passing the value explicitly.

This preserves the no-non-local-state principle: implicit resolution is doing *plumbing*, not *reaching*. The value must actually be in the caller's lexical scope to be eligible. The principle's invariant — that a command's parameter list is a complete inventory of its dependencies — is preserved at the *signature* level; the implicit mechanism reduces the *syntactic noise at call sites* without weakening the semantic guarantee.

The implicit-parameter mechanism is conceptually parallel to but distinct from instance delegation (Section 4): implicits resolve at call sites by uniqueness-of-type; instance delegation resolves at type-class lookup by explicit naming.

---

## 4. Classes, Instances, and Dispatch

### 4.1 Classes

A class is a single-parameter type contract — structurally analogous to a Haskell single-parameter typeclass, a Java interface, or a Scala trait. A class is parameterized over *one* type (the implementing type / receiver), not over a tuple of types; there is no multi-parameter class concept.

A class body contains command members, each of which is either a **declaration** (`declare ...`, with no body) or a **definition** (`command ...`, with a body). Declarations *must* be supplied by every instance. Definitions provide a **default implementation** that any instance may override. **There are no "final" implementations.** Every default may be overridden.

### 4.2 Instances

An instance declaration says: "implementing type T satisfies classes C1, C2, ...". The grammar's instance-types form (`instance Name: TYPENAME, TYPENAME, ...`) is *not* a tuple-keyed instance — it is "one type implementing several classes simultaneously," analogous to Haskell's `instance (C1 a, C2 a) => ...`. There is no multi-class joint-instance machinery.

Instance bodies provide implementations for the class's declared and defaulted commands, where the implementing type takes the role of the receiver.

**Delegation.** An instance may delegate its implementation of a class to a designated field (or component) of the implementing type. The grammar's `(delegate)` clause names the delegate explicitly. **Delegation is always explicit**: even if the implementing type has multiple fields whose types could provide an implementation of the class, only the *named* delegate is used. This rule overrides the type-based singleton resolution that applies elsewhere (e.g., to context parameters): in instance delegation, the explicit delegate trumps any uniqueness-of-type consideration. This preserves "no surprises": the caller always knows what implementation will be invoked.

The asymmetry between context-parameter resolution (uniqueness-of-type) and instance delegation (explicit naming) is deliberate. Context parameters operate at value level where uniqueness-of-type is reliable; delegation operates at type level where multiple compatible fields could plausibly exist by accident, so explicit naming is required.

### 4.3 Dispatch via Haskell-Style Dictionary Passing

Class dispatch is implemented via **Haskell-style dictionary passing**. Compilation of a class-constrained call:

1. Identifies the class constraint the call requires.
2. Identifies the type providing the implementation.
3. Looks up the instance dictionary for `(Class, Type)`.
4. Compiles the call as an indirect invocation through that dictionary.

A dictionary is a record-like value containing command-typed values for each class member, populated according to the matching `instance` declaration. Default implementations from the class body fill in for any commands the instance did not override.

**This commits the language to:**
- **Domains stay byte-faithful.** An `Inches` is `[4]` bytes, period. Class dispatch information lives in the dictionary, not in the value. Adding `instance Showable: Inches` does not change the runtime size of an `Inches`.
- **Records stay packable.** Records containing class-constrained fields are still exactly the size of their fields' bytes. No hidden alignment or padding for vtables.
- **Polymorphic commands** receive the dictionary as a hidden parameter, exactly as in GHC's underlying representation.
- **Monomorphization is an optimization, not the semantics.** The semantic model is dictionary passing; specialization to direct calls is a compiler optimization the implementation may perform when type information is statically known. The programmer can also force this manually by binding the resolved command to a local variable (Section 3.4).

**Fat-pointer RTTI** (mentioned in Section 2.4 for records) is a *separate* runtime-type-info mechanism, used for downcast safety on pointers. It is not the same as the class-dispatch dictionary. Different jobs, different mechanisms.

### 4.4 V-Commands and Multiple Dispatch

A v-command takes a tuple of receivers: `(r1, r2, ...) :: cmd-name : args`. The user-visible behavior is multiple dispatch — the receivers' types together determine the command's behavior, in the spirit of Julia's open multiple dispatch.

**The implementation is composition of single-class dispatches, not joint-instance dispatch.** A v-command's body is authored against the classes its receivers individually satisfy. The body invokes class methods on each receiver as needed; each such invocation is an ordinary single-class dispatch using the receiver's own class dictionary. The combined behavior of a v-command call is the product of its receivers' types — different receiver-type combinations produce different behaviors — but no jointly-keyed dictionary exists in the system.

**Concrete example.** Given a v-command `(logger, severity) :: format`, calling it with `(consoleLogger, warning)` and with `(consoleLogger, error)` will produce different behaviors *because* `Warning` and `Error` are distinct domains satisfying the `Severity` class, and their `Severity`-class methods dispatch to different implementations. The v-command's body sees those different behaviors when it invokes `Severity` class methods on the second receiver.

**Consequences:**
- Single-receiver v-commands are class-method dispatch on that receiver.
- Multi-receiver v-commands are dispatch composed from per-receiver class lookups.
- There is no tuple-keyed dispatch table.
- Coherence is the union of single-class coherence properties (see OQ-5); no separate multi-parameter coherence theory is needed.
- **Separate compilation works cleanly.** Module A defining `Logger` and module B defining `Severity` need not coordinate: a v-command using both classes resolves each independently.

This design is deliberately structured so that "open multiple dispatch like Julia's" is achievable without the joint-instance complexity that breaks separate compilation and modular reasoning.

### 4.5 Class Resolution Yields a Command-Typed Value

The expression `(receiver :: name)` resolves the class method `name` on `receiver` and produces a command-typed value with the receiver baked in. The result's type matches the class's declaration of `name`, with the receiver position elided. So if `Logger :: log: String` is declared in class `Loggable`, then `(myLogger :: log)` has type `:<String>` — receiver-baked-in, ready to be invoked with just the string argument.

This is the mechanism Section 3.4's tight-loop optimization relies on: class resolution returns a value, and values can be bound, stored, and reinvoked at zero dispatch cost. Partial application beyond this case (e.g., partial application of regular commands or v-commands on subsets of their receivers/parameters) is intended to use the same conceptual mechanism but is not yet fully designed (OQ-6).

---

## 5. Lifecycle: `@` and `@!`

The `@` (AMPHORA) and `@!` (AMBANG) markers introduce **frame-exit hooks**. These are **fundamentally different from C++ destructors**: they fire on *call/scope exit*, not on *value destruction*.

**The hook is a property of the call**, not of the value. When a command's stack frame unwinds — at the end of the body in normal flow, or as a failure propagates out — registered exit hooks for that frame execute. Exit hooks do **not** fire when:
- A value is overwritten (assignment is not a destructor trigger)
- A value is copied (no double-destruction concerns)
- A value is moved or rebound
- A pointer to a value goes out of scope (the value is not "owned" by the pointer)
- A `^Object` is rebound to a different object (the old object is not destructor-called)

**`@` blocks** in a command body register cleanup that runs **on any exit** — normal or failure. **`@!` blocks** register cleanup that runs **only on failure exit**.

The natural pattern for resource management is: the command that *acquires* the resource also registers its release.

```
# h <- (open: filename)
@! close: h            ; runs only if a later step fails
... do work ...        ; if this succeeds, h survives the frame
```

Or for unconditional cleanup:

```
# h <- (open: filename)
@ close: h             ; runs regardless of success or failure
... do work ...
```

**Resources cannot meaningfully be "owned" by data structures across command boundaries** in the way they are in C++. You cannot put a file handle in a record and have the record's destruction release the file — there is no "destruction of a value" event in the language. Resources are tied to the *lifetime of the call that acquired them*. If a resource needs to outlive the acquiring command, the acquiring command must not register an exit hook for it, and release responsibility must move elsewhere (typically to a containing frame that registered the cleanup before delegating acquisition). The language does not track this; it is a discipline.

**Constructor and destructor signature forms.** The grammar's `DEF_CMD_CTOR` is the constructor signature shape; `DEF_CMD_RECEIVER_ATSTACK` is the destructor; `DEF_CMD_RECEIVER_ATSTACK_FAIL` is the fail-handler. These signature forms, like the block markers, fire at frame-exit time for the frames that registered them — they are not value-bound.

---

## 6. Open Questions

These are intent-level questions that have surfaced during the dialog and need resolution before or during the corresponding spec work. Each is stated in the form: title, problem, sub-questions, and any user-stated preference.

### OQ-1: Union discriminator representation

Unions are value-like byte-overlays whose candidates all reduce to known byte widths, but distinct candidates may have the same byte-width and bit-pattern. To prevent the synthesis of incorrectly-typed pointers to a union (i.e., a pointer-cast that lies about which candidate the bytes represent), some discriminator is required.

**Sub-questions:**
- (a) Inline discriminator (cost on every union value, but works for stack-allocated unions and unions inside records)
- (b) Discriminator in the fat pointer / RTTI (leaner per-value, but constrains all union access through pointers carrying such info)
- (c) Hybrid (split between the two depending on how the union is held)

The rule that must hold globally: there must be no syntactic or semantic path by which a candidate's bytes can be reinterpreted as a different candidate. The design pressure from records-containing-unions is toward (a) or (c), since records-internal storage cannot rely on a fat pointer being present.

### OQ-2: Implementation latitude for IN parameter passing

The language semantics commit to call-by-value for IN parameters and call-by-copy-restore for INOUT parameters: programmers must be able to reason about parameter passing as if these conventions held literally. The implementation may diverge where divergence is unobservable.

**Sub-questions:**
- (a) Conditions under which large IN parameters may be passed by reference under the hood (no aliasing through other parameters, no escape of the parameter address, no observation of mutation through the reference)
- (b) Interaction with macro/fexpr-like semantics: the `:{...}` block-quote and `:<...>{...}` command-literal forms are believed to need different parameter-passing treatment from each other; the design has not been finalized
- (c) Correctness criterion: precisely what "unobservable divergence" means in a language with first-class commands and writeable parameters

Resolution must specify both the observable semantics for each call form and the optimizations the implementation is permitted to make.

### OQ-3: `-> name` result designator on non-writeable parameters

The well-formedness rule for the `->` result designator requires that `name` be a parameter or receiver, but does not require that the named slot be writeable. The semantic meaning of designating a non-writeable parameter as the expression-position result — given that non-writeable parameters are call-by-value and the callee's copy evaporates on return — needs characterization.

**Possible readings:**
- (a) The result is the *initial* value of the parameter (echoing back what the caller passed in; useful for combinator/identity-shaped commands)
- (b) The result is the value at the moment of return *as observed inside the callee*, with the callee's local lifted out as the expression value at the call boundary
- (c) The case is intentionally restricted to specific patterns yet to be specified

**User intent:** the designer flagged this as worth leaving open and indicated the case should not be over-constrained without thought.

### OQ-4: Default initialization across the type system

The desugaring of `# r <- (call: ...)` requires `r` to be default-initialized prior to the call so that on failure the slot is well-defined. The `# x` introduction (without RHS call) is similarly load-bearing: it produces a fresh variable that must hold *something*.

**Sub-questions:**
- (a) Should default initialization be a mandatory property of every type that can appear in a writeable parameter slot, or should some types be excluded from expression-position use?
- (b) For types with a defined default, is the default an inherent property of the type (declared at type-definition time) or computed structurally from constituent parts?
- (c) How does default initialization interact with the "mutation either succeeds fully or fails fully" principle when a default-initialized slot represents an *invalid* state for the type — for example, a `domain Positive: [4]` whose default would be zero but whose semantic constraint excludes zero?
- (d) What is the relationship between default initialization and constructors — does having a constructor for a type imply it cannot be default-initialized, or do constructors and defaults coexist?

**User intent:** the designer flagged sub-question (c) — domains with invariants — as already a known concern without satisfactory resolution. This is a load-bearing question with implications for desugaring rules, the validity of bare `# x` introductions, the design of object lifecycle, and the type system's treatment of refinement.

### OQ-5: Single-class instance coherence

Class dispatch is single-parameter, with multiple-dispatch expressiveness emerging from composition of single-class dispatches across v-command receivers. The single-class coherence question remains.

**Sub-questions:**
- (a) Are duplicate instances for the same `(class, type)` pair a static error globally, or are they ranked?
- (b) Is there an orphan-instance restriction limiting where instances for a given class can be declared (only in the module declaring the class, only in the module declaring the type, or anywhere)?
- (c) **Domain-specific dispatch [user intent].** Class dispatch resolves on the *most specific known type* of the receiver at the call site, not on any implicitly-upcast parent type. If `Inches` is a domain over `Int32`, and both `instance Showable: Int32` and `instance Showable: Inches` exist, an `Inches` value dispatches to the `Inches` instance — even when the calling context's static expectations would have allowed implicit upcast to `Int32`. This preserves the typeclass-with-newtype pattern (analog of Haskell's `newtype Sum = Sum Int` with `instance Monoid Sum`) which is one of the principal motivations for distinct domains. **The constraint is captured as user intent contingent on absence of unforeseen problems.** There is potential tension with implicit-upcast behavior at parameter boundaries, which must be resolved so that upcast either does not lose domain identity (so dispatch from inside the callee still sees `Inches`) or happens only for type-checking acceptance, with the runtime value retaining its domain identity.
- (d) When importing a module with overriding instance definitions, **the user's preference is to follow Julia's "more specialized module wins" pragmatics**, with openness to rebuttals if it produces unfortunate consequences in the broader design.

Resolution affects program acceptance, separate compilation guarantees, and modular reasoning.

### OQ-6: Partial application beyond receiver-only

Resolving a class command on a receiver — `(receiver :: name)` — yields a command-typed value with the receiver baked in; the value's type matches the class's declaration of the command. Partial application beyond this case (e.g., partially applying a v-command on a subset of its receivers, or partially applying a regular command on a subset of its non-receiver parameters) is intended to use the same conceptual mechanism but the design has not been worked out.

**Sub-questions:**
- (a) What syntax produces a partial application beyond the receiver-elision case?
- (b) What is the resulting command-typed value's type?
- (c) How does partial application interact with the writeable / IN / INOUT distinctions on parameter slots?
- (d) How does partial application interact with implicit context parameters?

---

## 7. Topics Not Yet Covered in Intent Dialog

These topics have been *mentioned* during the dialog but not addressed substantively, and remain open for future intent sessions before full spec work begins.

**Failure semantics integration with the type system.** The README's operational semantics block specifies the dynamic behavior precisely. The static checking story — what the typechecker verifies about may-fail and must-fail commands, what makes a program fail-mode-correct, and the structural rules for recovery contexts (the `?`, `??`, `?-`, `?:`, `|`, `|:`, `^` block markers) — has not been worked through. This is the natural next major thread.

**Command literals and quotes.** The grammar distinguishes `:{...}` / `?{...}` / `!{...}` block quotes from `:<...>{...}` / `?<...>{...}` / `!<...>{...}` command literals. These differ in capture semantics and parameter handling (and OQ-2 flags this as an open question). The full intent — what each form means, how scoping and capture work, when they are equivalent — needs to be addressed.

**Modules, imports, scopes, and visibility.** The grammar references modules but the rules around naming, resolution, visibility, separate compilation, and import precedence are not yet established. This intersects with OQ-5 (instance coherence and import-priority resolution).

**Tests and programs as first-class entry points.** The grammar has `.test "label" = group` and program-level forms. These are described in the README as the only places where "no non-local state" relaxes; the precise rules for what they may do that commands may not, and what their static checking looks like, need to be worked through.

**IR design (issue #24).** The internal representation between AST and codegen has not been chosen.

**FFI design (issue #23).** Whether and how Basis interoperates with other languages has not been determined.

**Railroad diagrams (issue #8).** A presentational matter rather than a design matter; flagged for completeness.

---

## 8. Glossary

**AMPHORA** — the `@` character. Marks frame-exit hooks (always-on-exit) when used as a block marker, and destructor signatures when used in a command-signature receiver position.

**AMBANG** — the `@!` character pair. Marks frame-exit hooks (failure-only-on-exit) when used as a block marker, and fail-handler signatures when used in a command-signature receiver position.

**Buffer** — a contiguous byte storage region. Encapsulated by the bracket form `[]`, `[N]`, `[]T`, `[N]T`.

**Class** — a named, single-parameter contract bundling command signatures (declarations and defaults). Implemented by instances. Analogous to a Haskell typeclass / Java interface / Scala trait.

**Command** — the unit of execution. Takes parameters, may produce output through writeable parameters, may fail. First-class value.

**Context parameter** — an implicit parameter (after `/` in the signature) resolved at the call site by uniqueness-of-type from the caller's lexical scope, in the manner of Scala implicits.

**Copy-restore** — the calling convention for INOUT parameters: callee receives a copy, and on successful return the modified copy is written back to the caller's slot. On failure, no write-back. The mechanism by which mutation atomicity is achieved.

**Dictionary** — the runtime record of command-typed values that implements a class for a particular type. Looked up at call sites for dispatch; passed as a hidden parameter to polymorphic commands.

**Domain** — a refined type declared in terms of a value-like parent (a domain or a range form). Introduces a new, distinct type identity. Implicitly upcasts to its parent but not vice versa, and not to siblings.

**Fat pointer** — a pointer that carries runtime type information. Intended for records (and possibly other types) to enable safe downcasting and discrimination.

**Frame-exit hook** — a piece of cleanup code registered via `@` or `@!`. Fires when the registering command's stack frame unwinds (always-exit for `@`, fail-only-exit for `@!`). Distinct from C++ destructors.

**Implicit parameter** — see *context parameter*.

**Instance** — a declaration that an implementing type satisfies one or more classes, with implementations supplied directly or by delegation to a named field.

**INOUT parameter** — a parameter marked writeable with the apostrophe `'`. Call-by-copy-restore: callee mutations land in the caller's slot on success, evaporate on failure.

**IN parameter** — a parameter without the writeable marker. Call-by-value: callee mutations are local-only.

**Object** — a stack-or-heap-allocated, identity-bearing aggregate with non-contiguous fields. Reference-semantics. Distinct from records.

**Range** — the bracket-form buffer types `[]`, `[N]`, `[]T`, `[N]T`. The buffer-as-fundamental-datatype manifestation.

**Receiver** — a parameter of a v-command (or constructor/destructor) that participates in class dispatch.

**Record** — a contiguous, byte-addressable buffer with named field offsets. Value-semantics. Distinct from objects.

**Result designator (`-> name`)** — the syntactic-sugar clause naming which parameter or receiver of a command "is" the expression-position result, enabling the command to be used as the right-hand side of `<-` without a named temporary.

**Union** — a value-like byte-overlay tagged sum. Candidates restricted to domains. Distinct from variants.

**Variant** — a reference-semantics tagged sum. Candidates may be arbitrary types. Distinct from unions.

**V-command** — a command with one or more receivers; dispatch composes single-class dispatches across the receivers' types.

**Writeable** — see *INOUT parameter*.

---

## 9. Provenance and Status

**Authored:** Distilled from intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-26.

**Source materials read:** `README.md` (with caveats — outdated where contradicted by code), `Token.h`, `Lexer.h`, `Productions.h`, `Grammar2.h`, `Grammar2.cpp`, `Parsing2.h`, `Ast.h`, `AstBuilder.h`, partial read of `AstBuilder.cpp`. The repository's `language_notes.txt` and removed `grammar2.md` were *not* used as authority. The test files `basis_tests/test_grammar2.cpp` and `basis_tests/test_ast.cpp` were not directly readable from the chat interface and were not consulted.

**Recommended next step:** This document should be committed to the repository (suggested path: `docs/intent-checkpoint-001.md`) so it becomes durable and accessible to Claude Code in future spec-driven sessions. Subsequent intent dialogs should be captured as further checkpoints, building toward a complete language specification.
