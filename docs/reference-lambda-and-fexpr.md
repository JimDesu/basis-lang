# Basis Language — Reference: Lambda and Fexpr

**Status:** Topic-organized authoritative reference for the four command-typed-value-producing forms — command reference, command literal, lambda, and fexpr — and for the open questions about partial application, capture-list semantics, parameter-passing under the transitive READ contract, and visible-signature representation that those forms surface. Consolidates material from CP007 (the three-way brace-bearing-form taxonomy; the provision-chain rule; the lambda-as-object-with-implicit-fields treatment), CP008 (the slash-list capture mechanism for command-literal-with-captures forms; ceiling-tracking and reference-chain flattening; per-invocation copy-restore for reference captures; the closure rule and aliasing rule; the lightweight-vs-ceiling-tracked phenotypes; OQ-19 resolution carved into type-system §4.4), CP010 (the fexpr design: implicit-capture-by-free-name; the per-invocation invocation frame; the locality rule and its five auxiliary restrictions; direct rather than copy-restore captured-slot access; OQ-7 resolution and OQ-25 introduction), CP015 §8.4 (the `.implicit`-excludes-`.context` constraint, which closes part of OQ-21 from the construction-reference side), and the 2026-05-04 dialog (the four-form taxonomy renaming; the fexpr-as-`D`-closure sharpening; the `<*>` typing distinction; the variant-with-fexpr-candidate relaxation; the fexpr non-escape rules sharpened beyond CP010's locality rule; the OQ-2(b) / OQ-6 capture-side / OQ-18 / OQ-21 lambda-side / OQ-25 resolutions).

**Date:** 2026-05-04

**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` (2026-04-29), continued with `reference-operational-semantics.md` (2026-04-29), `reference-type-system-and-modes.md` (2026-05-02), and `reference-construction-and-initialization.md` (2026-05-03). The 2026-05-04 scope-confirmation and clarification dialog produced a substantial taxonomic correction (the bare `{...}` form was misnamed "lambda" in CP007/CP008; correctly it is "command reference"; the angle-bracket form `:<...>{body}` was treated monolithically and is now split into "command literal" without captures and "lambda" with captures), several mechanism sharpenings (fexpr ceiling exclusively at the defining frame, with non-escape via constructor-productive-output, object-field-storage, and writeable-parameter-passing all forbidden), and the resolution commitments enumerated in the §15 provenance.

**Authority statement.** Where this reference differs from the source checkpoints on a covered topic, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative. Where this reference and an *earlier* reference differ on overlapping material, this reference is authoritative on the lambda-and-fexpr side and the earlier reference is authoritative on its own domain, with cross-references flagging the boundaries. The 2026-05-04 dialog produced four-form taxonomy decisions that supersede CP007 §2's three-form taxonomy on naming and refine the angle-bracket form's structure; the construction reference's §6 / §11 contain assumptions about command-typed-value uniformity that need touch-up per the consolidations enumerated in §15.

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]*. Section-level connective tissue carries footer citations identifying the section spans drawn upon. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints — typically resolving an ambiguity, recording a post-checkpoint correction, or carving in a refinement after the 2026-05-04 dialog. **Cross-references** to earlier references take the form *[failure-system §x]*, *[op-sem §x]*, *[type-system §x]*, *[construction §x]*; **forward references** to planned but undrafted references are flagged as such.

**Standing principles carried into this reference.** Five lenses from prior references and the operating-principles document govern the treatment here:

1. **Frame-ownership** *[type-system standing principle; OPS §1.1]*. Every slot is owned by some frame; "return" in the conventional sense does not exist. Command-typed values arise from construction operations whose destination slot is owned by some frame; the construction operation does not own the slot it writes. The four forms each have specific rules about which frames can own them, which frames they can travel to, and which frames they can reach.

2. **No cross-frame analysis propagation** *[op-sem §5.2; OPS §1.2]*. Each frame's static analysis (initialization, failure-mode, access-path taint) is local. Command-typed values that travel to other frames are seen by those frames' analyses as freshly-bound locals; the captures and bound arguments are part of the value's identity, not of the receiving frame's analysis context.

3. **The provision-chain rule** *[op-sem §3.3; CP007 §1.2]*. A command body's reachable state must arrive there by an explicit chain of provision: parameters/receivers, context parameters, or captured-and-traveling state. The four forms each instantiate the third category in different ways; this reference details how.

4. **Ceiling tracking is uniform across value kinds** *[CP008 §IN-6]*. A value bound to stack-resident state has a ceiling at the frame where that state lives. The rule applies identically to objects with reference-bearing fields (the original case), to lambdas with reference captures, to fexprs (whose entire defining frame is the ceiling), and to command references with reference-bound arguments. The four-form taxonomy in this reference is the ceiling-rule's full population.

5. **Compute-then-commit applies in capture-bearing constructors** *[construction §1.4; §8]*. Constructors that produce command-typed values (lambdas in particular) compose Phase 1 (computing capture values, resolving captures) with Phase 2 (the atomic `<-` write that places the value in the productive-receiver slot). Failure during capture computation does not leave a partially-built lambda visible to the caller; standard copy-restore preserves the receiver's pre-call state.

---

## 2. The Four-Form Surface Map

This section lays out the four command-typed-value-producing forms — what each looks like, what each is for, and how each instantiates the unified design. Subsequent sections (§§3–6) treat each form in detail; this section establishes the taxonomy.

### 2.1 The Three-Axis Distinction

Four forms emerge from the combination of three independent syntactic axes:

| Axis | Marker | What it signals |
|---|---|---|
| Failure-mark prefix | Present (`:`, `?`, `!`) vs. absent | Whether the form has a body whose failure semantics need explicit advertisement |
| Angle-bracket parameter list | Present (`<…>`) vs. absent | Whether the form declares its own invoke-method signature |
| Slash-list captures | Present (`/ caps`) vs. absent | Whether the form binds defining-frame state by explicit capture |

The four reachable combinations:

| Form | Surface | Failure mark | Angle brackets | Slash captures |
|---|---|---|---|---|
| **Command reference** | `{cmd}` or `{cmd: x, _, &y}` | absent | absent | absent |
| **Command literal** | `:<args>{body}` (and `?<…>`, `!<…>`) | present | present | absent |
| **Lambda** | `:<args / caps>{body}` (and `?<…>`, `!<…>`) | present | present | present |
| **Fexpr** | `:{body}`, `?{body}`, `!{body}` | present | absent | absent |

The user reading source distinguishes the four forms at sight. A bare `{...}` is a command reference; an angle-bracket form `:<...>{...}` without a `/` inside the angle brackets is a command literal; an angle-bracket form `:<... / ...>{...}` with a `/` inside is a lambda; a failure-marked brace without angle brackets is a fexpr.

*[Reconciliation: the four-form taxonomy and naming are settled in the 2026-05-04 dialog. CP007 §2 introduced a three-form taxonomy ("bare brace-quote" / "failure-marked brace-quote" / "command-literal" — also called "lambda-with-body") and used "lambda" loosely as an umbrella term for both the bare form and the command-literal form. CP008 then extended the angle-bracket form with the slash-list capture mechanism, but kept the umbrella usage of "lambda." The 2026-05-04 dialog decisively names the bare form **command reference** (corresponding to CP007's "bare brace-quote"), splits the angle-bracket form by capture-presence into **command literal** (no slash list) and **lambda** (with slash list), and retains **fexpr** for the failure-marked brace-quote (per CP010). The renaming makes "lambda" a precise term — only angle-bracket forms with captures qualify — and frees "command reference" to denote what it always was: a function pointer plus partial application, not a body-bearing capture-form. Operational-semantics §7 currently encodes the prior CP007 three-form taxonomy with the bare form labeled "lambda"; this is among the consolidations forwarded to a future pass.]*

### 2.2 The Umbrella: Command-Typed Value

All four forms produce values whose type is a **command-typed value** — the umbrella type-system category covering anything invokable as a command. Command-typed values are non-buffer values *[type-system §3.6; CP001 §3.4]*. Their static type names the invoke-method signature: parameter modes, parameter types, and the failure mark.

The four forms differ in *how* the body, captures, and bound arguments are tied together at construction time, not in *what kind of value* the construction produces:

| Form | What the value carries |
|---|---|
| Command reference | The resolved command-value (a function pointer); plus, for partial-application forms, the bound argument values (or references) |
| Command literal | The body code; no captures; no bound arguments |
| Lambda | The body code; the captured values (IN captures, by-copy) and captured slot references (`&` captures) |
| Fexpr | The body code; slot references for every defining-frame name the body uses, computed by the typechecker |

Each form's type-system identity carries the relevant ceiling and locality information necessary to enforce the frame-ownership and ceiling-tracking discipline. §§3–6 detail the per-form rules; §8 unifies the ceiling story.

### 2.3 What Each Form Is For

| Form | Canonical use |
|---|---|
| Command reference | Function-pointer-style dispatch capture (the tight-loop optimization for v-command method dispatch); explicit partial application |
| Command literal | Eagerly-evaluated thunks and short bodies that don't need defining-frame state — pure callbacks with explicit parameter lists |
| Lambda | Callbacks and ad-hoc closures that need to read or mutate defining-frame state, with explicit capture lists declaring the dependence |
| Fexpr | User-defined control-flow combinators where the body is written inline at the call site and needs implicit access to the surrounding scope's locals |

The mental model: command references are about *what* command to call; command literals are about *what code* to run with no upstack state; lambdas are about *what code plus what captured state* to run; fexprs are about *what inline body* to run in a control-flow combinator.

### 2.4 Failure Marks Across Forms

A command-typed value's failure mark — `:` (never-fails), `?` (may-fail), or `!` (must-fail) — is the static-type-level signal for failure conformance. Forms with bodies (command literals, lambdas, fexprs) advertise the mark explicitly via the prefix on the angle-bracket or brace-quote. Command references inherit their failure mark from the underlying command they reference; the form itself carries no marker.

The mark-subsumption rule from failure-system §1.3 applies symmetrically to all command-typed values:

- A `:`-marked value is acceptable wherever a `?`-typed slot is expected (`:` ⊑ `?`).
- A `!`-marked value is acceptable wherever a `?`-typed slot is expected (`!` ⊑ `?`).
- `:` and `!` are not interchangeable.

The rule applies uniformly to ordinary commands, command references, command literals, lambdas, and fexprs; the failure mark is a property of the command-typed value's type, not of its constructional form. *[failure-system §1.3; type-system §4.5.]*

### 2.5 The Empty-Angle-Bracket Pure-Thunk Form

A degenerate command-literal form `:<>{body}` (and `?<>`, `!<>`) has empty angle brackets — no parameters, no captures. This is a **pure thunk**: it runs its body when invoked, with no inputs and no capture-frame ties. *[CP008 §9.4 syntactic shape `:<>{body}`.]* Pure thunks are command literals, not lambdas (no captures); they have no ceiling beyond the ordinary object lifecycle. From the invoker's vantage, a pure thunk and a fexpr are visually similar but structurally very different — the fexpr has implicit capture from its defining frame; the pure thunk has no captures at all.

The §6 fexpr treatment uses `:<*>`, `?<*>`, `!<*>` for fexpr-typed parameter slots to distinguish them from pure thunks at the type level (per the §6.7 typing rule); `:<>` accepts pure thunks but not fexprs.

*Sources for §2: CP007 §§2.1, 2.2 (the original three-way taxonomy and per-form summary table); CP008 §§1, 9.4 (the angle-bracket form's slash-list extension and grammar); CP010 §1 (the fexpr summary); the 2026-05-04 dialog (the four-form renaming and the `<*>` typing distinction).*

---

## 3. Command Reference

A command reference is a value-typed reference to an existing named command, optionally with some of the command's parameters pre-bound. It is the function-pointer mechanism: a way to capture *which command to call* into a value that can be passed downward, stored in a local, or used in a tight-loop optimization to avoid repeated method dispatch. It is *not* a body-bearing form — a command reference has no body of its own; invoking it is invoking the underlying command (with whatever bound arguments are pre-applied). *[CP007 §2.1.]*

### 3.1 Surface Forms

```
{cmd}                          ; pure reference: no bound args
{cmd: x, _, y}                 ; partial application: 1st and 3rd args bound, 2nd open
{receiver :: methodName}       ; receiver-elision via the scope operator
{receiver :: methodName: x, _} ; receiver-elision plus partial application
```

The bare braces `{` and `}` are the command-reference fence. The contents must reference an existing named command (or a method through receiver-elision via `::`); they may include positional argument values, with `_` marking unbound positions. The surface admits arguments in the same modes the underlying command accepts: an underlying parameter declared `&y` is bound by writing `&actual` at that position; an underlying productive `'r` cannot be bound (productive parameters discharge their write at the invocation site, not at the partial-application site). *[CP007 §3; CP008 §9.5.]*

### 3.2 What the Value Carries

A command reference value carries:

- **The resolved command-value** — a function pointer to the underlying command's body, including any class-instance-dispatch information already resolved at construction time. *[op-sem §5.6 for the `::` resolution.]*
- **The bound argument values or references** — for any positional arguments supplied at construction, the corresponding values (for IN-mode bindings) or slot references (for `&`-mode bindings) are stored as part of the command reference.

The unbound positions remain part of the invoke-method signature; the command reference has the type of the underlying command with the bound positions removed and the unbound positions retained.

### 3.3 No Body

A command reference has no body of its own. Invoking it invokes the underlying command, with the bound arguments supplied at their bound positions and the unbound arguments supplied at the invocation site. The failure mark of the command reference is the failure mark of the underlying command — the form carries no failure marker because it has no independent failure structure to advertise. *[CP007 §2.1; §2.2 of this reference.]*

This is the structural property that distinguishes command references from command literals and lambdas: those have a body, and a body-side failure mark; command references do not.

### 3.4 Mode Restrictions on Bound Arguments

Bound arguments at partial-application time follow the underlying command's parameter modes:

- **IN-mode parameter** — the bound argument is a value of the parameter's type; the typechecker enforces type compatibility and (where the value is a literal) admits `.implicit` constructors per the standard rules. *[construction §5; CP008 §6.1.]*
- **`&`-mode parameter (reference)** — the bound argument is `&local-name`, where `local-name` is a slot in the construction frame's lexical scope. The slot reference is captured at construction time; subsequent invocations operate on the bound slot through this reference. The construction frame's slot must be initialized at the binding moment. *[CP008 §6.1.]*
- **Productive-mode parameter (`'r`)** — bound arguments at productive positions are forbidden. Productive parameters discharge their write at the invocation site, where the receiving frame owns the slot. Pre-binding a productive position would either (a) require a slot in the construction frame (which the invocation could write — preserving frame-ownership) or (b) require deferring the slot identity to the invocation site (which is not partial application). The cleaner rule is that productive positions cannot be bound. The user who wants productive-result-as-output uses an unbound `_` at the productive position. *[CP008 §6.1; reconciliation: this rule is consistent with CP008 §4.4's "productive captures forbidden" rule on lambdas; productive-mode-binding-forbidden is the same rule applied to command references.]*

### 3.5 Ceiling Tracking — Reference-Bound Arguments Pin the Ceiling

A command reference with one or more `&`-mode bound arguments carries a ceiling at the *shallowest* of the bound slots' defining frames. The ceiling rule is the same one CP008 §5.2 articulates for lambdas with reference captures, applied to command references' bound-argument slots instead of slash-list captures. *[CP008 §5.2; CP008 §IN-6; reconciliation: the unified-ceiling-rule framing applies to command references by direct analogy.]*

A command reference with no `&`-mode bound arguments has no ceiling beyond the ordinary object lifecycle; it can travel freely through writeable parameters or be stored in object fields without any escape concern. This is the **lightweight phenotype** of command references, paralleling CP008's lightweight lambda phenotype. *[CP008 §5.3.]*

A command reference that *does* bind reference arguments enters the **ceiling-tracked phenotype**: the typechecker tracks its ceiling and rejects assignments to slots whose own ceilings would let the command reference outlive any bound `&` argument's defining frame.

### 3.6 Reference-Chain Flattening

A bound `&`-argument may itself be a reference parameter of the construction frame (e.g., `helper` is called with `&actual` as a `&y`-parameter; `helper` then constructs `{cmd: x, &y}` binding its own parameter `&y` into the command reference). The captured slot's identity is *flattened* through the chain to its origin, exactly as CP008 §5.1 specifies for lambda reference captures: the command reference's ceiling is the origin frame, not the construction frame. *[CP008 §5.1; reconciliation: the same flattening rule applies uniformly across command references and lambdas.]*

```
.cmd helper : :<>' ref, &Counter accum =
    'ref <- {Counter::increment: &accum}     ; & flattens: ceiling = caller's frame, not helper's

.cmd outer =
    # state : Counter <- (Counter: 0)
    # incrementer
    helper: 'incrementer, &state              ; flattened: incrementer's ceiling = outer's frame
    nestedCmd: incrementer                    ; sound: incrementer travels downward, within ceiling
```

### 3.7 Excluded: Command References to Fexprs and Fexpr-Typed Bound Arguments

A command reference cannot reference a fexpr-typed command (`{some_fexpr}` is rejected); a command reference cannot bind a fexpr-typed argument (`{some_cmd: ..., my_fexpr, ...}` is rejected). The exclusion is conservative-by-default — the safety analysis for the interaction is non-trivial, and a future thread may relax this if motivated, but the current reference disallows it. *[Reconciliation: this rule is settled in the 2026-05-04 dialog. The concrete rationale: command references can outlive their construction site (subject to ceiling tracking); fexprs cannot escape their defining frame; combining the two would create a value that *might* outlive its embedded fexpr, with all the lifetime-soundness concerns that implies. The clean rule is to forbid the combination outright.]*

### 3.8 Receiver-Elision via `::`

The `(receiver :: methodName)` form (op-sem §5.6) produces a command reference with the receiver baked in. The result's type is the underlying method's type with the receiver position elided. A subsequent `{receiver :: methodName: x, _}` form chains receiver-elision with positional partial application, producing a command reference that can be invoked with whatever positions remain unbound. *[op-sem §5.6; CP001 §3.4.]*

```
.cmd loop_optimization : Logger logger, [N]Item items =
    # logFn <- {logger :: log}              ; one dispatch lookup at construction
    ^                                        ; loop body
        # i : Int32 <- 0
        ?- (lessThan: i, N)
            logFn: items[i]                  ; direct invocation, no dispatch
            (increment: &i)
```

The dispatch resolution is performed once at the `{logger :: log}` construction site; subsequent invocations of `logFn` skip the class-instance lookup. This is the canonical "tight-loop optimization" for v-command method dispatch.

### 3.9 Worked Examples

```
.cmd partialApplyExample =
    # add3 <- {add: 3, _}                    ; partial: 1st arg bound, 2nd open
    # result : Int32 <- (add3: 5)            ; invocation: 5 + 3 = 8

.cmd referenceCapture : &Counter outer =
    # incrementer <- {increment: &outer}     ; & bound: ceiling = caller's frame
    incrementer                              ; invocation increments outer
    incrementer                              ; second invocation; cumulative effect
```

In the second example, `incrementer` carries a slot reference to `outer`'s caller's `Counter` slot (via reference-chain flattening); each invocation of `incrementer` mutates that slot directly. The mutations are not copy-restored across invocations because the `&` binding is not a per-invocation copy — the bound `&outer` is a *partial-application* binding, not a *capture*, so the binding is fixed at construction time; subsequent invocations operate on the bound slot directly through the function-pointer-with-bound-args representation.

*[Reconciliation: CP008 §IN-7 specifies per-invocation copy-restore for lambda reference *captures* — the slash-list mechanism — but does not address command-reference reference *bindings*. The 2026-05-04 dialog clarifies that command references' bound `&` arguments behave as direct-binding (not copy-restore) per partial-application's semantics: `{cmd: &x}` is "call cmd with `&x` at every invocation," and the invocation's own copy-restore semantics for the underlying `&` parameter are what govern mid-invocation atomicity. The command reference itself does no additional per-invocation copy-restore beyond what the underlying command does.]*

*Sources for §3: CP007 §§2.1, 2.2 (the bare-form taxonomy and properties); CP008 §§5.1, 5.2, 6.1, IN-6 (reference-chain flattening, ceiling computation, bare-form bound-argument modes, unified-ceiling principle); op-sem §5.6 (receiver-elision); the 2026-05-04 dialog (fexpr-exclusion rule; command-reference-as-direct-binding semantics).*

---

## 4. Command Literal

A command literal is an eagerly-evaluated command-typed value with an explicit body and an explicit invoke-method signature, and **no captures** of defining-frame state. It is the "fresh body, fresh frame, no upstack ties" form — a pure callback whose only inputs are its invoke-method parameters. *[CP007 §2.1 (as "command-literal"); CP008 §1.1 (as "lambda-with-body" without slash list); the 2026-05-04 dialog (renaming: "command literal" denotes the no-capture variant of the angle-bracket form).]*

### 4.1 Surface Forms

```
:<>{body}                          ; pure thunk: no parameters, no captures
:<Int32 x>{body}                   ; one IN parameter
:<Int32 x, Int32 'r>{body}         ; one IN parameter, one productive
:<Int32 x, Int32 &y>{body}         ; one IN parameter, one reference
?<Int32 x>{body}                   ; same shapes with may-fail mark
!<Int32 x>{body}                   ; same shapes with must-fail mark
```

The angle-bracket parameter list declares the invoke-method signature: positional parameters with their types and identifier-shape mode markers (per type-system §4 / CP008 §3). Each parameter's name is bound inside the body for the duration of the invocation. There is no slash list — no defining-frame captures. The body is a sequence of statements separated by indentation, the same way a regular command's body is structured. *[CP008 §§3, 9.4 syntactic shapes.]*

### 4.2 Eager Evaluation

A command literal is eagerly evaluated at the literal's construction site: the typechecker resolves the body and its parameter-list at compile time, the construction operation produces a command-typed value at runtime, and the value is placed in the lhs slot of the surrounding `<-` (or assigned to the productive `'r` if the literal is the rhs of a constructor's productive-write). The body is *not* executed at construction; eager evaluation refers to the *value's construction*, not to the body's invocation. *[CP007 §2.1.]*

The body's free names — names referenced in the body that are not bound by parameters or by body-introduced `#` locals — must be resolvable as either top-level names (commands, types, classes, instances) or as parameters of the literal itself. A command literal cannot reach into the defining frame for non-top-level names. *[Reconciliation: this is the closure rule from CP008 §4 ("body referencing a free name not in the parameter list and not in the slash list is a compile-time error"), specialized to command literals: since command literals have no slash list, the closure rule reduces to "free names must be parameters or top-level."]*

### 4.3 No Captures, No Upstack Ties

A command literal does not capture defining-frame state. Its body's only inputs are its invoke-method parameters; its body's only outputs are through writeable parameters (productive `'r`, reference `&y`); and its body's only side-effects are those that flow through its parameters. *[Reconciliation: this is the "lightweight" structural property the 2026-05-04 dialog emphasized. Where CP007 §3 / CP008 §3 sometimes used "lambda" to refer to angle-bracket forms generally, the 2026-05-04 renaming reserves "command literal" for the strictly-no-captures case.]*

The structural consequence is that a command literal has **no ceiling beyond the ordinary object lifecycle**. It can be:

- Stored in an object field
- Passed to a writeable parameter (reference or productive)
- Returned from a constructor's productive output
- Written through a pointer
- Used as a variant candidate

Each of these would be forbidden for a fexpr (per §6) and constrained for a lambda with reference captures or a command reference with `&` bound arguments (per §3, §5).

### 4.4 Body Obligations

The body is held to the same obligations as any command body, per CP004 §3.3:

- **Productive parameters are written exactly once on every successful return path.** *[CP009 §2; construction §8.2.]*
- **Reference parameters are read-after-write or write-after-read per the standard CP013 rules.** *[type-system §4.4.]*
- **READ parameters are not written through any reachable access path.** *[type-system §4.6.]*
- **The body's failure-mode advertisement matches the body's actual failure structure** (per §7 below). *[failure-system §1.4; CP008 §1.1.]*

The compute-then-commit pattern from construction §8 applies inside command-literal bodies just as in regular command bodies: productive writes are atomic, made by a single `<-` per path.

### 4.5 The Pure Thunk Form

The empty-angle-bracket form `:<>{body}` (and `?<>`, `!<>`) is the pure thunk: no parameters, no captures, just a body. The form is occasionally useful for deferred computation that needs no inputs and no upstack state, for testing scaffolds, or as the type-system identity of an empty closure. From the type-system's perspective, a pure thunk's type is `:<>` (or `?<>` / `!<>`), distinct from any fexpr type (per §6.7's `<*>` rule). *[CP008 §9.4.]*

### 4.6 Parameter-Passing Under the Transitive READ Contract — OQ-2(b) Resolution

OQ-2(b), forwarded from CP013 §7.3 / op-sem §8.1 to this reference, asks how the block-quote `:{...}` (fexpr) and command-literal `:<...>{...}` forms' parameter-passing should be specified under the transitive READ contract. The treatment for command literals is settled here; the fexpr treatment is settled in §6.

**Resolution for command literals:** parameter-passing follows the standard CP013 rules unchanged. *[Reconciliation: the resolution is settled in the 2026-05-04 dialog. The framing the dialog landed on: a command literal is, structurally, a regular command with an explicit signature and body — the only differences from `.cmd`-declared commands are (a) it is a literal expression rather than a top-level declaration and (b) its body cannot reach into the defining frame's non-top-level names. Neither difference affects parameter-passing semantics. The transitive READ contract from type-system §4.6 / CP013 applies to a command literal's parameters identically to how it applies to any other command's parameters: READ parameters carry the transitive-READ taint into the body; PRODUCE/REFERENCE retain copy-restore. The implementation latitude rules from CP013 §7.3 (a) — pass-by-reference under the unobservable-divergence rule — apply identically.]*

A subtle case worth recording: a command literal whose body invokes a captured-frame-name (which it cannot, per §4.2) or holds upstack state through some hidden mechanism (which it does not, per §4.3) would interact with the transitive READ contract differently. Since command literals cannot do either, the interaction is null; the rule is "exactly the same as for declared commands."

This resolves OQ-2(b) for the command-literal half. The fexpr half is in §6.6.

### 4.7 Worked Examples

```
.cmd higherOrder : :<Int32 x, Int32 'r> callback, Int32 'result =
    callback: 5, 'result          ; invoke the callback, productive output flows to 'result

.cmd useIt =
    higherOrder:
        :<Int32 x, Int32 'r>{ 'r <- (multiply: x, 2) },     ; command-literal callback
        'doubled
```

The command literal `:<Int32 x, Int32 'r>{...}` is constructed at the call site, passed as the `callback` parameter, invoked from inside `higherOrder`'s body, and discarded when `higherOrder` returns. Its body has two parameters (`Int32 x`, `Int32 'r`) and a single statement (productively writing `2 * x` to `'r`). No captures from `useIt`'s frame; the literal has no ceiling beyond the ordinary object lifecycle.

```
.record Subscription :
    :<Event e> handler

.cmd subscribe : Subscription 'r, :<Event e> h =
    'r <- ${handler <- h}       ; the handler field receives the command-literal
```

Storing a command literal in an object field is permitted because the literal has no upstack ties. The Subscription object owns the handler value; the handler's lifetime is the Subscription's. If `subscribe`'s caller produced the `h` argument as a command literal, the literal lives as long as the resulting Subscription.

*Sources for §4: CP007 §§2.1, 2.2 (command-literal-as-eagerly-evaluated; the table); CP008 §§1.1, 3, 9.4 (mode markers, body obligations, slash-list-syntactic-shapes); CP013 §7.3 / op-sem §8.1 (OQ-2(b) framing); the 2026-05-04 dialog (resolution of OQ-2(b) for command literals; the four-form renaming).*

---

## 5. Lambda

A lambda is a command literal with **captures** — an angle-bracket-form whose body needs access to defining-frame state and declares that access via a slash list. The slash list separates invoke-method parameters from captures, with mode markers on the captures specifying how each defining-frame name is brought into the body's scope. *[CP008 §§1, 4 (the slash-list mechanism); the 2026-05-04 dialog (renaming: "lambda" denotes the with-captures variant of the angle-bracket form).]*

### 5.1 Surface Forms

```
:<args / capName>{body}            ; one IN capture (by-copy)
:<args / Type &capName>{body}      ; one reference capture (by-slot-reference)
:< / capA, &capB>{body}            ; captures only, no parameters (thunk with state)
```

The slash `/` divides the angle-bracket interior into two regions: parameters before the slash, captures after. Each capture is a name (with optional `&` marker) of a slot in the defining frame's lexical scope. *[CP008 §9.4.]*

The slash list captures only the *named* slots; any defining-frame name *not* in the slash list and *not* in the parameter list is rejected as a free name in the body (the closure rule, §5.4 below). This is the explicit-only-capture rule — no implicit inheritance, no "convenient" capture of context variables. *[CP008 §1.2; the 2026-05-04 dialog clarification: "Absolutely no capture except for items specifically marked. ... so that the programmer can develop an intuition of which variables might be affected up-stack."]*

### 5.2 Capture Modes

Captures may be IN or reference; productive captures are forbidden:

| Mode | Marker | Semantics |
|---|---|---|
| IN (READ) | bare name (`capName`) | The defining-frame slot's value is **copied** into a hidden field of the lambda at construction time. Subsequent invocations read the hidden field; mutations to the defining-frame slot after construction do not propagate. |
| Reference | `&capName` | The defining-frame slot's reference is **captured** as part of the lambda's identity. Subsequent invocations read and write through the captured reference, with per-invocation copy-restore (§5.6 below). |
| Productive | `'capName` | **Forbidden.** Productive contracts presuppose initialization-of-uninitialized-slots, which doesn't compose with multi-invocation lambda semantics. *[CP008 §1.3.]* |

The IN-capture-by-copy semantics means: a lambda's IN captures are *snapshots* of the defining-frame state at construction time, not live views. Subsequent mutations to the defining-frame slot after the lambda is constructed are not observed by the lambda's invocations. *[CP008 §5; CP007 §3.]*

The reference-capture semantics provides live, copy-restore-bracketed access to the defining-frame slot. The body can read and write the captured slot through the captured name; per-invocation copy-restore preserves failure-atomicity (§5.6).

### 5.3 Reference-Chain Flattening

A `&capName` capture where `capName` is itself a reference parameter of the constructor's frame is *flattened* through the chain to its origin. The flattening is structural and computed at compile time. *[CP008 §5.1.]*

```
.cmd helper : :<>' lam, &Counter accum =
    'lam <- :<>{ / &accum}{ Counter::increment: &accum}    ; & flattens through helper's parameter

.cmd outer =
    # state : Counter <- (Counter: 0)
    # incrementer
    helper: 'incrementer, &state                           ; flattened: incrementer's ceiling = outer's
    nestedCmd: incrementer                                 ; sound: incrementer travels down within ceiling
```

Inside `helper`, the lambda captures `&accum`. Without flattening, the captured slot would be `accum`-the-parameter-slot in `helper`'s frame, which ceases to exist when `helper` returns. With flattening, the captured slot is `state` in `outer`'s frame; the lambda's ceiling is `outer`; the productive return of the lambda via `'lam` is sound; subsequent invocation through `nestedCmd` operates on `outer`'s `state`. *[CP008 §5.1 worked example, abridged.]*

If `accum` were an IN parameter of `helper` (rather than a reference parameter), capturing `&accum` would be forbidden — there's no slot to reference, only a value-copy local to `helper`'s frame, which would be lost when `helper` returned. *[CP008 §5.1.]*

### 5.4 The Closure Rule

Body free names must appear in the parameter list or the slash list. A body referencing a free name not in either is a compile-time error. *[CP008 §1.2.]*

The closure rule eliminates accidental captures: a refactor that introduces a new name in the body — say, the user types `accum` instead of the intended local `acc` — is caught at compile time as an unbound free name, rather than silently capturing a defining-frame slot the user didn't intend to bind.

The rule is **uniform across capture modes**: a name in the slash list is a declared capture (with mode-marker per §5.2); a name in the parameter list is a declared parameter; a name in the body's `#`-introductions is a body local; any other name in the body must resolve to a top-level name (commands, types, classes, instances). *[CP008 §1.2.]*

The closure rule does *not* permit context-variable inheritance from the defining frame's signature. Even if the defining frame has implicit context parameters in its `/`-list, those are not visible inside a lambda body unless the lambda's slash list explicitly captures them. *[Reconciliation: this is the OQ-21 (lambda-side) resolution settled in the 2026-05-04 dialog, encoded from inception. The construction reference §13 / OPS §1 unifies "context variables" as the umbrella concept; the regular-command `/` list and the lambda `/` list are two mechanisms (implicit vs. explicit) for binding context variables. A lambda cannot inherit defining-frame context variables implicitly; it must capture them explicitly. This preserves OPS §1.2's no-cross-frame-analysis-propagation principle and OPS §1.8's orthogonality preference.]*

### 5.5 Productive Captures Are Forbidden

A capture of the form `'capName` is rejected. The reasoning *[CP008 §4.4]*:

A productive `'`-mode contract is a write-once-on-every-successful-path obligation. The discharge of this obligation happens at the moment of the productive write; it is, structurally, a one-shot event. A lambda may be invoked many times; if a lambda were to capture a productive slot, each invocation would face the question "is this the invocation that discharges the productive obligation?" with no language-level mechanism for distinguishing first-from-subsequent invocations.

The clean rule is "no productive captures." A constructor wanting to bake a productive output into a callback uses a different mechanism — typically, the invoke-method parameter list (which can be productive) rather than the capture list.

### 5.6 Per-Invocation Copy-Restore for Reference Captures

Each invocation of a lambda with one or more reference captures performs standard copy-restore against the captured slot, separately for each reference capture *[CP008 §5.4]*:

- **At invocation entry**, the captured slot is read into a per-invocation local (via the flattened reference).
- **The body operates on the local.**
- **On successful return**, the local is written back to the captured slot.
- **On failed return**, no write-back occurs; the captured slot retains its pre-invocation contents.

This is per-invocation, not per-construction. Each invocation's mutations are independent; failures during one invocation don't bleed into subsequent invocations. *[CP008 §5.4.]*

The copy-restore semantics preserves the language's failure-atomicity guarantee uniformly. The contrast with fexprs (§6.4 below) is meaningful: fexprs use direct slot access (no copy-restore), so failures mid-invocation can leave the captured slot in a partial state. Lambdas do not.

### 5.7 Aliasing Rule

A captured slot may not also appear as an explicit invoke-method argument at the same invocation site. *[CP008 §5; OQ-19's resolution context.]*

```
.cmd suspect : :<Int32 'r / &Int32 x> lam, Int32 &y =
    lam: 'y                          ; aliasing: y is both captured AND passed as 'r
                                      ; (typechecker rejects when it can detect this)
```

The rule guards against the case where a single slot is being mutated through two paths in the same invocation — the captured-reference path and the productive-parameter path. The mutations would race in undefined order, and copy-restore would write back inconsistent contents. The typechecker rejects such aliasing where it can detect it statically; runtime aliasing through more elaborate paths (e.g., a captured slot that is also bound in a deeper call) is the user's discipline.

### 5.8 Ceiling Computation

A lambda's ceiling is computed as *[CP008 §5.2]*:

```
ceiling(lambda) = min over c in capture-list of:
    if c is an IN capture:           ⊤  (no contribution)
    if c is a reference capture &y:  origin-frame(y)  (per §5.3)
```

A lambda with no reference captures (only IN captures, or no captures — though a no-capture form is a command literal, not a lambda) has `⊤` as its ceiling, reducing to the introducing-frame ceiling per the ordinary object lifecycle. A lambda with one or more `&`-captures has a ceiling at the shallowest captured slot's origin frame, after reference-chain flattening. *[CP008 §5.2.]*

Once computed, the ceiling governs the lambda's lifecycle:

- The lambda may be passed downward freely (to commands called from any frame at or below the ceiling).
- The lambda may be assigned to slots whose own ceilings are at or below its ceiling.
- The lambda may migrate upward via writeable-parameter copy-restore, up to but not above its ceiling.
- Assignment to a slot whose ceiling is above the lambda's ceiling is rejected at compile time. *[CP008 §5.2.]*

This is structurally the same rule as for command references with reference-bound arguments (§3.5), and as for fexprs (§6.5, with a tighter constraint). The unified ceiling story is in §8.

### 5.9 Two Phenotypes

A lambda's typechecker behavior depends on whether its capture list contains any reference captures *[CP008 §5.3]*:

- **Lightweight lambda** — only IN captures. Ceiling = `⊤` (introducing frame). Migrates freely upward via copy-restore. No new typechecker machinery beyond ordinary object lifecycle.
- **Ceiling-tracked lambda** — one or more reference captures. Ceiling = min of `&`-captured slots' origin frames. Migration constrained; typechecker tracks the ceiling and rejects upward escapes.

The phenotypes are syntactically indistinguishable at construction; the typechecker detects the difference from the capture list. The user's code either compiles or doesn't; error messages explain which capture is constraining the ceiling. *[CP008 §5.3.]*

### 5.10 Capture-Shadowing — OQ-25 Resolution

A body-introduced local (`# x`) may share a name with a captured slot. The natural rule is **shadowing**: within the body, `x` refers to the local; the captured slot is inaccessible for the duration of the local's scope. This parallels how parameter names shadow enclosing-scope names in regular commands. *[Reconciliation: OQ-25 was registered in CP010 §2.4 against fexprs but applies equally to lambdas. The 2026-05-04 dialog confirmed shadowing as the rule for both. Encoded from inception.]*

The stricter alternative — make the same-name case a compile-time error, paralleling type-system §4.3's same-scope rule for `&x` and `x` — was considered and rejected. The reasoning: body-locals are in a different (invocation) frame from captures, so there is no actual collision; the shadowing is structural, not coincidental. *[CP010 §2.4 framing of the alternative.]*

```
:<Int32 'r / &Int32 acc>{
    # acc : Int32 <- (multiply: acc, 2)     ; body-local 'acc' shadows the captured 'acc'
                                              ; reads of captured-acc happen in the rhs (Phase 1)
                                              ; the body-local takes over thereafter
    'r <- acc                                 ; refers to the body-local
}
```

The shadowing is scoped to the body-local's textual extent; outside that scope, the captured name is in scope normally. The typechecker may emit a stylistic warning to flag the shadowing — implementation-defined, not language-required.

### 5.11 Visible-Signature Representation — OQ-18 Resolution

OQ-18, registered in CP008 §11, asks how a lambda's signature appears at the type-system surface — what the user writes in a parameter declaration that accepts a lambda, what the typechecker reasons over, and what shape a partially-applied lambda has. *[Reconciliation: settled in the 2026-05-04 dialog. The resolution: lambdas surface to callers via their **invoke-method signature** only; capture details are hidden.]*

The resolved rule:

- **A lambda's visible type is its invoke-method signature** — parameter modes, parameter types, and the failure mark — exactly as for any other command-typed value. The capture list is not part of the visible type.
- **A parameter that accepts a lambda is declared with the invoke-method signature of the expected lambda**, in the standard command-type-expression grammar: `:<Int32 x, Int32 'r>` (for a lambda accepting an `Int32 x` and producing an `Int32 'r`, never-fails mark).
- **Capture-list details are not surfaced to callers.** The caller knows what the lambda accepts and produces (and at what failure mark); the caller does not know what defining-frame state the lambda is bound to.

The rationale: capture details are an implementation matter of the lambda construction site, not of the lambda's interface. A callee receiving a lambda parameter cannot do anything different based on what the lambda captured; the callee can only invoke the lambda. The visible signature names what the callee can do.

The corollary for partial application: a partial-application result (a command reference, per §3) has the underlying command's signature with the bound positions removed. The bound positions become part of the value's identity (its captures or bound args), not part of its visible type. A `{cmd: x, _, &y}` result has type `:<TypeOfMiddleArg>` (or whatever mode/mark applies), with the bound `x` and `&y` not appearing in the type.

The OQ-18 resolution applies symmetrically to lambdas and command references: in both cases, the "what's bound" detail is hidden, the "what's invokable" surface is shown.

*Sources for §5: CP008 §§1.1–1.5, 3, 4, 5 (slash-list mechanism, capture modes, productive-forbidden rule, closure rule, ceiling computation, copy-restore, aliasing rule, phenotypes); CP010 §2.4 (capture-shadowing baseline, applicable to lambdas by analogy); the 2026-05-04 dialog (OQ-21 lambda-side resolution: explicit-only no inheritance; OQ-25 resolution: shadowing permitted; OQ-18 resolution: invoke-method signature is the visible type; the four-form renaming).*

---

## 6. Fexpr

A fexpr is a body-bearing form whose captures are computed implicitly from the body's free names against the defining frame's lexical scope. It is the user-defined-control-flow-combinator form: a body written inline at the call site of a higher-order combinator, with implicit access to the surrounding command's locals. The fexpr's lifecycle is bounded by its **defining frame** — the frame in which the fexpr literal was written; the fexpr cannot escape upward, cannot be stored in long-lived structures, and cannot be passed to anything that could outlive its defining frame. *[CP010 in full; the 2026-05-04 dialog (sharpening of fexpr-as-`D`-closure model; the `<*>` typing distinction; explicit non-escape rules).]*

### 6.1 Surface Forms

```
:{body}                            ; never-fails fexpr
?{body}                            ; may-fail fexpr
!{body}                            ; must-fail fexpr
```

A failure-mark prefix (`:`, `?`, `!`) is required; absence-of-mark is the command-reference form (§3). There is no angle-bracket parameter list — fexprs have no parameters from the invoker's vantage; from the invoker, a fexpr is opaque (a thunk-shape). *[CP007 §2.1; CP010 §1.]*

The body is a sequence of statements, separated by indentation, the same way a regular command's body is structured. *[CP010 §1.6.]*

### 6.2 The Three-Frame Model

A fexpr's evaluation involves three frames:

| Frame | Role |
|---|---|
| **Defining frame `D`** | The frame in which the fexpr literal was written. The fexpr's captures are resolved against `D`'s lexical scope; the fexpr's lifetime is bounded by `D`. |
| **Invoker frame `I`** | The frame from which the fexpr is invoked. `I` is opaque to the fexpr body — the body cannot access `I`'s slots, cannot pass arguments to the fexpr (there are no arguments), cannot do anything except observe the fexpr's failure-mark. |
| **Per-invocation frame `F`** | A fresh sub-frame established at each invocation. `F` provides storage for the body's `#`-locals; `F` is the introducing frame for any objects, lambdas, or sub-fexprs constructed by the body. `F` is destroyed at fexpr return. |

*[Reconciliation: the `D`/`I`/`F` naming is settled in the 2026-05-04 dialog and is the canonical naming throughout this reference. CP010 §3.1 calls `D` the "capture frame" and `F` the "invocation frame"; the renaming aligns the three frames into a parallel set. The crucial point that the 2026-05-04 dialog sharpened, beyond CP010's wording: the fexpr body is a **closure over `D`**, with `I` opaque except for the failure-mark contract. CP010 §3.1 says `F` is "structurally a sub-frame of the invoker's frame," referring to call-stack placement; this is correct and not in tension with the closure-over-`D` framing — the body's *lexical* environment is `D`, while the *call-stack position* of `F` is below `I`.]*

### 6.3 Implicit Capture by Free Name

A fexpr body's free names — names referenced in the body that are not bound by body-introduced `#` locals — are partitioned by the typechecker *[CP010 §2.1]*:

- Names resolving to **slots** in `D`'s lexical scope (locals, parameters, receivers, captured names from a containing fexpr's scope when the fexpr is nested) are **captured**. The typechecker emits, at fexpr-construction time, a slot reference for each captured name.
- Names resolving to **top-level definitions** (commands, types, classes, instances) are **not captured**. They are referenced via the language's normal name-resolution.

The capture set is fully determined by the body's text; the user does not write a capture list. *[CP010 §2.1.]*

This is a deliberate departure from lambdas (§5), which use explicit capture lists. The asymmetry is part of the language's design vocabulary: explicit captures (lambdas) signal "this value travels"; implicit captures (fexprs) signal "this value stays local." A user reading `:<args / caps>{body}` knows the value may travel; a user reading `?{body}` knows it does not. *[CP010 §2.2.]*

### 6.4 Captured-Slot Access Is Direct

When a fexpr body reads or writes a captured `D`-slot, it operates on the slot directly — *not* via copy-restore. *[CP010 §3.2, §5.]*

The body reads the current contents of the captured slot at the moment of the read; the body's writes to the captured slot are visible immediately to subsequent reads in the same invocation, to other concurrent invocations of the same fexpr (re-entry / nesting), and to the defining frame's body once the invocation returns. Failures mid-body do **not** roll back captured-slot writes — partial mutation is the fexpr's signature property. *[CP010 §3.2.]*

This contrasts deliberately with lambda reference captures (§5.6), which use per-invocation copy-restore. The motivation for the direct-access rule is the canonical fexpr use case — control-flow combinators where partial-mutation-on-failure is *the desired semantics*. A loop-combinator fexpr that mutates a counter on each iteration should leave the counter at its mid-loop value if the loop fails partway through; the user wants to inspect the counter to see how far the loop got. Copy-restore would defeat this by rolling back every iteration's mutations on the loop's eventual failure. *[CP010 §5.]*

The mode-marking discipline still applies: when the fexpr body accesses a captured `D`-name, the access mode is whatever mode that name has in `D`. A `&x` in `D` is reference-accessible; a READ `x` is read-only; a productive `'x` is productively writeable. The fexpr body cannot escalate access — exactly as if those names had been explicitly captured at their `D`-frame modes. The up-stack-effect intuition the user retains: same mode-mark, same possible effects, whether you're looking at an explicit lambda capture or an implicit fexpr access. *[Reconciliation: this mode-marking-discipline-through-implicit-capture rule is sharpened in the 2026-05-04 dialog beyond CP010's wording; CP010 §3.2 says the body operates on captured slots "as if they were locals" without explicitly tying the access-mode to `D`'s declaration. The sharpening is added to ensure programmer reasoning about up-stack effects survives the implicit-capture mechanism.]*

### 6.5 The Locality Rule and the Fexpr Ceiling Is `D`

A fexpr value's ceiling is *uniformly* `D`, the defining frame. There is no per-instance ceiling computation; every fexpr is `D`-bounded. *[Reconciliation: this is sharpened in the 2026-05-04 dialog. CP010 §1.3 / §4 establishes the locality rule with auxiliary restrictions; the dialog's framing is that "a fexpr's ceiling IS its defining frame," which is structurally the same outcome but stated as a uniform rule rather than as a restriction-derived consequence.]*

The auxiliary restrictions that enforce the `D`-ceiling are *[CP010 §4.2; refined in 2026-05-04 dialog]*:

**Restriction A: No productive or reference fexpr-typed parameters.** A parameter of fexpr type may carry only the IN mode marker (or no marker, equivalently). The productive `'` and reference `&` markers are forbidden on fexpr-typed parameters.

**Restriction B: No pointers to fexpr-typed slots.** The type `^Fexpr` (where `Fexpr` is any fexpr type) is forbidden.

**Restriction C: No fexpr fields in objects, records, or non-local-slot variants.** Object fields and record fields may not have fexpr type. Variant candidates may have fexpr type **only** when the containing variant inhabits a local slot — a `#`-introduced variant in `D` whose lifecycle is `D`-bounded — and not when the variant is embedded in an object, embedded in another buffer-backed type, or otherwise able to outlive the local slot. *[Reconciliation: this is a relaxation of CP010 §4.2 Restriction C in the 2026-05-04 dialog. CP010 forbade fexpr-typed variant candidates outright; the dialog permitted them in local-slot variants only, since the variant itself is then `D`-bounded and the fexpr's `D`-ceiling is preserved. A consolidations note is added to the construction reference §3.5 / type-system §3 / OPS §1.6 (per §15).]*

**Restriction D: No bare-identifier copy of fexpr values.** The bare-identifier `<-` form (`# f2 <- f1`) is forbidden when both slots have fexpr type. The construction site (`# f <- :{...}`) is the only valid assignment; subsequent migration is forbidden. *[CP010 §4.2 Restriction D.]*

**Restriction E: No fexpr captures by lambdas.** A lambda may not capture a fexpr-typed slot (whether by IN copy or by `&` reference). *[CP010 §4.2 Restriction E.]*

**Restriction F (carved in 2026-05-04): No fexpr from a constructor's productive output.** A constructor cannot produce a fexpr-typed value via its productive `'r` parameter. Fexprs are exclusively created as literals, in-place at their defining frame, and travel only downward. *[Reconciliation: this restriction is explicit in the 2026-05-04 dialog, sharpening CP010 §4.2's Restriction A: where Restriction A forbids productive fexpr-typed parameters generally, this sub-rule names the constructor-productive case specifically — a constructor's productive output is the standard mechanism by which a value is "produced upward" from a callee to its caller, and a fexpr produced this way would migrate from the callee's defining frame to the caller's slot, violating the `D`-ceiling. The restriction is structurally implied by Restriction A but worth stating explicitly for the constructor case.]*

**Restriction G (carved in 2026-05-04): No fexpr written to defining-frame writeable parameters.** A fexpr cannot be assigned to any reference or productive parameter of `D` itself. The result of such an assignment would expose the fexpr to `D`'s caller, violating the `D`-ceiling. *[Reconciliation: this is implicit in CP010 §4.2 Restriction A but explicit in the 2026-05-04 dialog. The dialog framing: "fexprs are passed DOWN-stack; a fexpr cannot be assigned to any of the defining stack's writeable parameters, as that would allow the fexpr to escape upwards."]*

Together, these restrictions ensure that a fexpr value cannot reach any frame above `D` at any point in its lifetime. The proof is structural *[CP010 §4.3]*:

- The fexpr is constructed in `D` and assigned to a `#`-local there (or kept anonymous in a parameter-position invocation, which the IN-only-passing rule constrains to downstack).
- The only further operations on the value are invocation and argument-passing as IN to downstack commands.
- IN argument-passing pushes the value into a sub-frame's parameter slot; that slot's lifetime is bounded by the sub-frame, which is below `D`.
- No other operation can place the value in a frame above `D`.
- When `D` exits, all sub-frames have already returned (their fexpr-bearing parameter slots are destroyed); the fexpr's `#`-local is destroyed; the value ceases to exist. No reference to the fexpr can persist.

### 6.6 The Per-Invocation Frame `F` and Its Cleanup

Each invocation establishes a fresh `F`. `F` *[CP010 §3.1, §3.3]*:

- Is created at the fexpr's invocation entry; destroyed at invocation exit.
- Provides storage for body-introduced locals (`# x` declarations in the body).
- Is the **introducing frame** for any objects, lambdas, sub-fexprs, or other ceiling-bearing values constructed in the body; their lifecycles are bounded by `F`.
- Has its own at-stack registration list for cleanup of body-introduced objects.
- Is destroyed at invocation exit; at-stack registrations fire; failures (if any) propagate to the invoking frame after cleanup.

`F` is hygienic: body-locals do not escape to `D` or to `I`; locals do not leak between invocations; locals do not interfere with `I`'s locals. *[CP010 §3.1.]*

A `#`-introduction in the body whose name collides with a `D`-visible name is a compile-time error (per type-system §4.3's same-scope rule, applied across the `F`/`D` boundary as the **shadowing-prohibited** rule for fexprs). *[Reconciliation: this is the 2026-05-04 dialog clarification of OQ-25 for fexprs specifically. Where OQ-25 for lambdas (§5.10) permits shadowing — body-locals shadow captures within their scope — the fexpr case is stricter: name collision between body-locals and `D`-visible names is rejected. The reasoning: fexprs use direct captured-slot access; a body-local with the same name as a captured slot would obscure whether the body is operating on the local or on the captured slot, defeating the up-stack-effect intuition. The stricter rule preserves the intuition.]*

### 6.7 The `<*>` Typing — OQ-32 Forwarded Surface

Fexpr-typed values are nominally distinct from ordinary command-typed values at the type level. A fexpr is not, at the type level, a `:<>` (or `?<>` / `!<>`); it is a `:<*>` (or `?<*>` / `!<*>`). The `*` marker inside the angle brackets denotes "this is a fexpr — opaque-to-invoker by design, ceiling-fixed-to-`D`." *[Reconciliation: the `<*>` syntax and its semantic role are settled in the 2026-05-04 dialog. The motivating concern is ceiling-tracking: under the `D`-ceiling rule, the typechecker must distinguish fexpr-typed values from ordinary command-typed values syntactically — otherwise a fexpr would be assignable to any `:<>` slot and would escape its `D`-ceiling. The `<*>` marker provides the syntactic distinction at parameter-declaration sites and in any other type-position where a fexpr-typed slot must be specifically declared.]*

The `<*>` displaces the signature contents because fexprs have no invoker-side signature surface beyond the failure mark. Captures are lexical (implicit-by-free-name from `D`); there are no parameters from the invoker's vantage; the invoker's only commitment is to respect the failure-mark. The `*` reads as "the part that would normally be the signature is intentionally not present, because there is no invoker-side surface here."

The typing rules for `<*>`:

- **No subsumption between fexpr-typed and ordinary command-typed values.** A `:<*>` is not a `:<>` (and vice versa); they are nominally distinct families. The buffer-backed-hierarchy subsumption rules from type-system §5 do not apply across the boundary.
- **The standard mark-subsumption rule applies symmetrically within the fexpr family.** A `:<*>` is acceptable in a `?<*>`-typed slot (per `:` ⊑ `?`); a `!<*>` is acceptable in a `?<*>`-typed slot (per `!` ⊑ `?`). *[failure-system §1.3.]* The mark-subsumption does not cross the fexpr/non-fexpr boundary: a `:<*>` is not acceptable in a `:<>`-typed slot or vice versa.
- **The typechecker enforces the `D`-ceiling structurally.** Fexpr-typed values are recognized syntactically (by the `<*>` marker) and forbidden from assignment to anything that could outlive `D`: object fields, reference parameters of upstack-bound destinations, productive parameters of the same. The five-plus-two restrictions of §6.5 are typechecker-enforced via the type-level distinction.

OQ-32 is registered to track the remaining ramifications:

- Whether `<*>` extends naturally to `f:<>` or other surface variants for any reason.
- The interaction of fexpr-typed parameters with class instance dispatch (probably permitted; see §10 / §12).
- The detailed grammar production for the `<*>` token inside command-type-expressions (added to OQ-23's implementation-thread subitems).
- Variant-with-fexpr-candidate's interaction with the dynamic-narrowing operator `-<` from construction §11 (the user's stated read: "I do not understand how the narrowing operator can apply to fexprs, but I'm happy to entertain the follow-up later").

The `<*>` syntax itself is settled; the ramifications are deferred to OQ-32 (§14.5).

### 6.8 Failure-Mark Conformance for Fexprs — OQ-2(b) Resolution Half

The fexpr half of OQ-2(b) — block-quote `:{...}` parameter-passing under the transitive READ contract — is settled here. Since a fexpr has no parameters from the invoker's vantage, the OQ-2(b) framing for fexprs reduces to: how does the captured-`D`-slot access interact with the transitive READ contract?

**Resolution:** captured-`D`-slot access respects `D`-side mode markers. A captured `D`-slot whose declared mode in `D` is READ is read-only inside the fexpr body, with the transitive READ contract from type-system §4.6 applying to any access path rooted at the captured name. A captured `D`-slot whose declared mode is `&` (reference) is reachable for read and write. A captured `D`-slot whose declared mode is `'` (productive) is captured at its productive-discharged state — the fexpr body sees it post-discharge. *[Reconciliation: settled in the 2026-05-04 dialog. The mode-marking-discipline-through-implicit-capture rule from §6.4 is the mechanism by which the transitive READ contract reaches the fexpr body; the contract is enforced on captured slots exactly as it would be on parameters of the same modes.]*

The implementation-latitude rules from CP013 §7.3 (a) — pass-by-reference under unobservable-divergence — apply to `D`-side READ slots accessed by the fexpr body, since the captured-slot access is read-only and the transitive READ contract makes the divergence unobservable. PRODUCE/REFERENCE captured slots retain copy-restore semantics... *except* that fexprs use direct access (§6.4), not copy-restore. The contrast with lambdas: lambdas wrap reference captures in per-invocation copy-restore (§5.6); fexprs do not. The contract still holds — the fexpr body's mutations to a captured `&` slot are visible to `D`'s post-fexpr-invocation analysis, and the fexpr's failure-mark advertisement constrains what mutations are visible on which paths — but the rollback-on-failure property is forfeited by design.

### 6.9 Re-Entry, Nesting, and Sub-Fexpr Scoping

A fexpr may invoke itself recursively (each invocation gets its own `F`); a fexpr body may construct other fexprs, lambdas, command literals, command references, or objects (each with its own lifecycle). *[CP010 §1.7.]*

A fexpr literal written *inside another fexpr's body* has the inner fexpr's `D` set to the outer fexpr's `F`, not to the outer fexpr's `D` and not to the original invoker. The inner fexpr's captures are resolved against the outer fexpr's body locals and any captures the outer fexpr made (which are slot references to the outer fexpr's `D`'s slots). *[Reconciliation: this is implicit in CP010's nesting model but stated explicitly in the 2026-05-04 dialog. The user's framing: "if one of those locals is itself a fexpr, then the locals inside that fexpr are bound to the local fexpr's defining frame."]*

The inner fexpr's ceiling is the outer fexpr's `F`, not the outer fexpr's `D`. The inner fexpr cannot escape upward beyond the outer `F`; in particular, the inner fexpr cannot be assigned to a slot in the outer fexpr's body that would persist beyond the outer fexpr's invocation. *[CP010 §9.4 worked example.]*

```
.cmd dispatcher : :<*>{} pre, :<*>{} post, :<*>{} body =
    pre
    body
    post

.cmd useIt =
    # x : Int32 <- (Int32: 0)
    dispatcher: ?{x <- (add: x, 1)},                  ; outer fexpr 1: D = useIt
                ?{x <- (subtract: x, 1)},             ; outer fexpr 2: D = useIt
                ?{                                      ; outer fexpr 3: D = useIt
                    # localFexpr <- ?{
                        x <- (multiply: x, 2)         ; inner fexpr: D = outer fexpr 3's F
                    }
                    localFexpr
                    localFexpr
                }
```

The inner `?{x <- (multiply: x, 2)}` captures `x` — but through the outer fexpr's capture chain, since `x` is in `useIt`'s frame, not directly in the outer fexpr's body. The inner fexpr's `D` is the outer fexpr's `F`; the inner fexpr cannot escape that `F`; it is invoked twice within the outer fexpr's body and discarded when the outer fexpr's invocation returns.

### 6.10 Worked Examples

**Loop combinator:**

```
.cmd while : :<*>{} cond, :<*>{} body =
    ?- cond
        ; cond failed; exit loop
    body
    while: cond, body                        ; tail-recursive (or compile-to-loop)

.cmd useWhile =
    # i : Int32 <- (Int32: 0)
    while: ?{lessThan: i, 10},               ; cond fexpr, captures i
           ?{                                 ; body fexpr, captures i
               (process: i)
               (increment: &i)
           }
```

The `cond` and `body` fexprs both capture `useWhile`'s `i`. The `while` combinator invokes them alternately; each invocation establishes a fresh `F` for the body's `#`-locals (here, none); the `i` mutations are visible to `cond`'s subsequent invocations because of the direct-access rule. The whole pattern is a `D`-bounded loop; when `useWhile` returns, both fexprs cease to exist.

**Failure propagation:**

```
.cmd attempt : ?<*>{} action =
    action

.cmd useFailure =
    ?- attempt: ?{
        ?(operationOne: x)
        ?(operationTwo: y)
        # localResult <- (Int32: 42)
        x <- localResult                     ; mutation visible to D after partial success
    }
        ; recovery: x has been mutated up to the point of failure
```

If `?(operationTwo: y)` fails, the fexpr's body terminates at the failure, the failure propagates to `attempt`, which propagates it to `useFailure`'s `?-` recovery. The mutation `x <- localResult` is *not* reached. But if the failure had been at `?(operationOne: x)`, the body had already executed up to that point — partial mutations to *other* captured slots would persist; this is the fexpr partial-mutation property. (In this specific example, no mutations precede the failure, but in general the body may have mutated state before failing.)

### 6.11 OQ-7 Closed Historically

OQ-7 (block-quote macro/fexpr semantics, registered in CP002 §2 and refined through CP007 §2 / CP010 §1) is closed by CP010's resolution and is recorded as such here. The 2026-05-04 dialog confirmed OQ-7 as resolved and made no further design moves on it; the historical thread is preserved in the provenance section (§15). *[Reconciliation: the dialog's confirmation: "OQ-7 is, to my understanding, resolved now." Recorded as historical-only in §14 / §15.]*

*Sources for §6: CP010 in full (the fexpr design — implicit capture by free name; the per-invocation frame; the locality rule and auxiliary restrictions; direct rather than copy-restore captured-slot access; OQ-7 resolution and OQ-25 introduction); the 2026-05-04 dialog (sharpening of fexpr-as-`D`-closure model; the `<*>` typing distinction; the variant-with-fexpr-candidate relaxation; the fexpr non-escape rules sharpened beyond CP010's locality rule; OQ-2(b) fexpr-side resolution); CP013 §7.3 / op-sem §8.1 (OQ-2(b) framing).*

---

## 7. Failure-Mark Conformance for Command-Typed Values

The four forms each carry a failure mark (or, in the command-reference case, inherit one from the underlying command). The mark is part of the value's static type and constrains what the body's failure structure must look like (definition-side) and what the invoker can rely on (invocation-side). This section consolidates the rules; per-form treatments in §§3–6 reference them. *[failure-system §§1.4–1.5.]*

### 7.1 The Three Marks Recap

The three failure marks, per failure-system §1:

| Mark | Surface | Body-side rule | Invoker-side guarantee |
|---|---|---|---|
| `:` | Never-fails | Every reachable path through the body must reach exit *without* a propagating failure. Any may-fail call inside must be fully recovered before exit. | Invocation never fails; no recovery needed. |
| `?` | May-fail | Any body-shape (subject to reachability); some paths may produce a propagating failure, others may not. | Invocation may fail; standard recovery contexts apply. |
| `!` | Must-fail | *Every* reachable path through the body must produce a propagating failure that reaches exit. A fully-recovered path that reaches exit cleanly is a `!`-conformance error; that path must `.fail` or be restructured so the failure is not fully recovered. | Invocation always fails; `:`-context callers cannot use it; `?`-context callers must consume the failure. |

Both halves of `!` matter: a `:` body cannot contain unrecovered may-fail calls; a `!` body cannot contain a path that successfully reaches exit. *[failure-system §1.5; the 2026-05-04 dialog explicitly confirmed this.]*

### 7.2 Two-Sided Conformance

Failure-mark conformance is two-sided:

**Definition-side conformance.** The body, at the construction site, is checked against the advertised mark. A `:`-marked construction whose body contains an unrecovered `?`-call is a compile-time error; a `!`-marked construction whose body has a path reaching exit cleanly is a compile-time error. The checker applies the standard CFG-based analysis from failure-system §3, with the body's exit edges as the conformance check points. *[failure-system §3, §1.5.]*

**Invocation-side conformance.** The invoker treats the command-typed value's mark as binding. Invoking a `?`-marked value in a `:`-context is a compile-time error (the `:`-context cannot have unrecovered `?`-calls); invoking a `!`-marked value in a `:`-context is a compile-time error for the same reason; and invoking a `?`-marked value in a `!`-context — the `!`-body's must-fail rule — is permitted iff the invocation either (a) produces a guaranteed-failing path or (b) is one of multiple branches converging on the must-fail exit. *[failure-system §1.4.]*

The two sides compose to give the standard mark-system guarantees: a `:`-marked value can be invoked anywhere; a `?`-marked value requires recovery context; a `!`-marked value requires the invoking context to itself be `!`-conformant on the invoking path.

### 7.3 Mark-Subsumption Across Command-Typed Values

The standard mark-subsumption from failure-system §1.3 applies to command-typed values:

- `:` ⊑ `?` — a `:`-marked value is acceptable wherever a `?`-typed slot is expected.
- `!` ⊑ `?` — a `!`-marked value is acceptable wherever a `?`-typed slot is expected.
- `:` and `!` are not interchangeable. *[failure-system §1.3; type-system §4.5.]*

The subsumption applies symmetrically across the four-form taxonomy and across fexpr/non-fexpr boundaries — except that the `<*>` typing of fexprs (§6.7) prevents subsumption *across* the family boundary. A `:<*>` is acceptable in a `?<*>`-typed slot; a `:<*>` is *not* acceptable in a `:<>`-typed slot, even though both have the `:` failure mark, because the family distinction (fexpr vs. non-fexpr) is enforced at the nominal-type level. *[Reconciliation: the family-boundary rule is settled in the 2026-05-04 dialog. The mark-subsumption is symmetric within each family; the family distinction is orthogonal.]*

### 7.4 Per-Form Conformance Rules

| Form | Mark source | Body-side rule | Invocation-side rule |
|---|---|---|---|
| Command reference | Inherited from underlying cmd | n/a (no body) | Standard mark-system rules apply at invocation per the inherited mark |
| Command literal | Explicit on the form | Body conforms in the construction frame's analysis | Invocation respects the advertised mark |
| Lambda | Explicit on the form | Body conforms in the *invocation's* analysis context — captures are part of the body's input, but the body's CFG is its own | Invocation respects the advertised mark; the invocation includes per-invocation copy-restore for `&` captures |
| Fexpr | Explicit on the form | Body conforms in `D`'s analysis — the body is checked against `D`'s frame and `D`'s parameter modes, since captures are `D`-slots | Invocation respects the advertised mark |

The fexpr case is the structurally interesting one: the body's conformance is checked *as a body in `D`*, not as a free-standing body. This means a `?{x <- (mayFail: y)}` fexpr's body is checked against `D`'s knowledge of `x`'s and `y`'s modes; the conformance is what `D`'s analysis would say if the fexpr's body were inlined into `D` at the construction point. *[Reconciliation: the dialog's framing: "the fexpr body must pass conformance checking in the defining frame." Encoded from inception.]*

### 7.5 Worked Example — `:` and `!` Conformance

```
.cmd buildCallbacks : :<>' neverFail, !<>' alwaysFail =
    'neverFail <- :<>{
        ; never-fails body: every path reaches exit cleanly
        (logSomething: "starting")
        (logSomething: "finishing")
    }
    'alwaysFail <- !<>{
        ; must-fail body: every path produces a propagating failure
        ?- (precondition:)
            .fail: standard_failure         ; recovery path also fails
        ?(somethingThatMustFail:)            ; primary path fails
    }
```

Each command literal's body is conformance-checked at the construction site against its advertised mark. The `:<>{...}` form's body must have no unrecovered may-fail calls (the `logSomething` calls are themselves `:`-marked, so this passes). The `!<>{...}` form's body must have every path producing a failure: the recovery path uses `.fail`; the primary path uses `?(somethingThatMustFail:)`, which is an unrecovered `?`-call on a path that reaches exit — wait, an unrecovered `?`-call's failure-edge propagates, which is a propagating failure on that path; the standard analysis treats this correctly per failure-system §3.

*Sources for §7: failure-system §§1.3–1.5 (mark-subsumption; `:` and `!` rules; the dual-conformance treatment); CP011 §§5–6 (the conformance analysis); CP008 §1.1 (lambda body obligations carry through); CP010 §1.9 (fexpr failure-mode-propagation rule); the 2026-05-04 dialog (two-sided conformance framing for fexprs; the `:` and `!` precision recall).*

---

## 8. The Context-Variables Umbrella Across Forms

This section consolidates how the four forms interact with **context variables** — the unifying concept established in the 2026-05-04 dialog for any name that is lexically resolved (rather than positionally passed) into a body's environment. Context variables are the bridge between the construction reference's §13 (regular-command `/`-list implicit context parameters) and the capture mechanisms here. *[Reconciliation: this section is a 2026-05-04 dialog product. Prior treatments (CP001 §3.5, op-sem §5.5, construction §13) covered the regular-command `/`-list mechanism in isolation; CP008's lambda capture list was treated as a separate mechanism; CP010's fexpr implicit capture was treated as yet another. The dialog's framing — "context variables" as the umbrella, with implicit / explicit / by-free-name as three resolution mechanisms — unifies them.]*

### 8.1 Context Variables Defined

A **context variable** is any binding visible inside a body that arrived there via lexical resolution — not via positional argument-passing. The unifying property: the receiving body sees the variable bound to a value (or slot reference) selected by the language's machinery, not by the caller's argument list.

Three mechanisms produce context-variable bindings:

| Mechanism | Where declared | When resolved | Form using it |
|---|---|---|---|
| **Implicit context parameters** | `/`-list in regular command's signature | Call time, by type-uniqueness in caller's lexical scope | Regular commands *[CP001 §3.5; op-sem §5.5; construction §13]* |
| **Explicit captures** | Slash list in lambda's `:<sig / caps>{body}` | Construction time, by name in defining frame's lexical scope | Lambda *[CP008 §1.2; this reference §5]* |
| **Implicit captures by free name** | Implicit (body's free names) | Construction time, by name in defining frame's lexical scope | Fexpr *[CP010 §2.1; this reference §6.3]* |

Command literals (no captures) and command references (no captures of their own; bound arguments are positional, not context-variable, bindings) do not introduce context-variable bindings of their own. *[Reconciliation per 2026-05-04 dialog.]*

### 8.2 Resolution Site Differs Across Mechanisms

The three mechanisms differ in *when* and *where* the resolution happens:

- **Implicit context parameters** resolve at the *call site*, in the *caller's* lexical scope, by type-uniqueness. The chosen value is supplied as if explicitly passed via `/` at the call site. *[CP001 §3.5; op-sem §5.5.]*
- **Explicit captures** resolve at the *construction site* (the literal's source position), in the *defining frame's* lexical scope, by *name*. The chosen value (or slot reference, for `&` captures) becomes part of the lambda's identity at construction. *[CP008 §1.2.]*
- **Implicit captures by free name** also resolve at the *construction site*, in the *defining frame's* lexical scope, by *name* (the body's free names). The chosen slot references become part of the fexpr's identity at construction. *[CP010 §2.1.]*

The contrast: implicit context parameters are *call-time, by-type, in invoker's scope*; lambda and fexpr captures are *construction-time, by-name, in defining-frame's scope*. Different mechanisms, sharing the unifying property that resolution is done by the language rather than by positional argument passing.

### 8.3 Context Variables and Mode-Marking

In all three mechanisms, the receiving body's view of the resolved variable carries a mode (READ, REFERENCE, PRODUCE):

- **Implicit context parameters** carry the mode declared in the regular command's `/`-list. *[construction §13.4.]*
- **Explicit lambda captures** carry the mode declared in the slash list (IN by default; `&` for reference; `'` forbidden). *[§5.2.]*
- **Implicit fexpr captures** carry the mode the captured name has *in `D`* — the captured access inherits the defining-frame's mode-marking. A `&x` in `D` is reference-accessible from the fexpr body; a READ `x` is read-only. *[§6.4.]*

The third mechanism's inheritance is what makes the up-stack-effect intuition work for fexprs: the user reads a fexpr body and knows its mutations to captured names will affect `D`-side state at exactly the modes `D` declared. The fexpr cannot escalate access; it can only do what `D`'s mode-marking already permits. *[Reconciliation per 2026-05-04 dialog: "a fexpr body cannot escalate access — exactly as if those names had been explicitly captured at their `D`-frame modes."]*

### 8.4 No Inheritance of Defining-Frame Context Parameters by Lambdas

A lambda's defining frame may itself have implicit context parameters (in *its* `/`-list). Those context parameters are *not* automatically inherited into the lambda's body; the lambda's slash list must explicitly capture any of them the body needs. *[2026-05-04 dialog; OQ-21 lambda-side; encoded from inception in §5.4.]*

Concretely:

```
.cmd outer / Logger logger =        ; outer has an implicit context parameter `logger`
    # callback : :<>'cb
    'cb <- :<>{
        ; This body cannot use `logger` — it is not captured.
        ...
    }
    'cb <- :<>{ / logger}{
        ; This body can use `logger` — it is captured by IN copy.
        (logger :: log: "from lambda")
    }
```

The non-inheritance rule is the orthogonal choice: the lambda's slash list is the *complete* statement of the lambda's context-variable bindings, mirroring how a regular command's positional list + `/`-list are the complete inventory of its dependencies. The user reading the lambda literal sees its full upstack tie-in at sight; nothing is hidden in the surrounding command's signature. *[Operating-principles §1.2's no-cross-frame-analysis-propagation principle reinforced.]*

### 8.5 Fexpr Defining-Frame Context Parameters Are Visible

A fexpr's defining frame may itself have implicit context parameters in its `/`-list. Those context parameters *are* visible inside the fexpr body — by the implicit-capture-by-free-name rule, any free name in the fexpr body that resolves against `D`'s lexical scope (which includes `D`'s context parameters) is captured. *[Reconciliation per 2026-05-04 dialog: this follows from the implicit-capture-by-free-name model, applied uniformly to all `D`-visible names. CP010 §2.1's "resolved against the constructor's lexical scope" framing includes the `/`-list.]*

The fexpr's body sees `D`'s context parameters at the modes `D`'s `/`-list declared them. A `?{x <- (logger :: log: "msg")}` fexpr defined inside `outer` (above) captures `logger` implicitly and uses it at the mode `outer`'s `/`-list declared (typically READ). The fexpr can re-invoke `logger`-using commands or pass `logger` to sub-frame calls in its body, with the standard implicit-context-parameter resolution applied at those call sites *as if `logger` were a name in `D`'s scope* — which it is, by virtue of the fexpr's lexical inheritance of `D`'s scope.

### 8.6 The Asymmetry Across Forms

The asymmetry of context-variable handling across forms is summarized:

| Form | Has its own context-variable bindings? | If so, by what mechanism? |
|---|---|---|
| Command reference | No | n/a |
| Command literal | No | n/a |
| Lambda | Yes | Explicit captures; no inheritance from `D`'s `/`-list |
| Fexpr | Yes | Implicit captures by free name; `D`'s `/`-list is visible |

The asymmetry between lambdas (no inheritance) and fexprs (inheritance) is intentional: lambdas can travel and need a complete upstack-tie inventory at the slash list to be auditable at sight; fexprs cannot travel above `D`, so the inheritance is bounded by the same `D`-ceiling that bounds the fexpr itself, and there's no audit-at-sight concern.

*Sources for §8: CP001 §3.5 (implicit context parameters); op-sem §5.5; construction §13; CP008 §1.2 (lambda explicit captures); CP010 §2.1 (fexpr implicit captures by free name); the 2026-05-04 dialog (the context-variables umbrella; OQ-21 lambda-side resolution; mode-marking inheritance through fexpr capture).*

---

## 9. Provision-Chain Compliance

The provision-chain rule from CP007 §1.2 — that a command body's reachable state must arrive there by an explicit chain of provision (parameters, receivers, context parameters, captures, or other declared inputs) — applies uniformly to all four forms. *[CP007 §1.2.]*

### 9.1 Why The Provision Chain Matters Here

The rule is the language's commitment to "no non-local state": no body reaches state that wasn't supplied to it through some declared mechanism. This rules out global mutable state, ambient context, runtime-determined name resolution, and other forms of hidden coupling between bodies. *[CP007 §1.2.]*

The four forms here introduce per-form analyses (the lambda body's analysis; the fexpr body's analysis), and each must preserve the provision-chain rule. This section confirms that each form preserves it, and identifies the mechanism in each case.

### 9.2 Per-Form Compliance

| Form | What the body sees | Provision mechanism |
|---|---|---|
| Command reference | n/a — no body | The bound arguments are explicitly listed at the construction site |
| Command literal | Parameters from the signature; top-level definitions | All inputs in the angle-bracket signature; no captures |
| Lambda | Parameters; explicit captures; top-level definitions | All inputs in the angle-bracket signature (parameters + slash list captures); closure rule rejects free names |
| Fexpr | Implicit captures by free name from `D`'s scope; top-level definitions | Captures resolved at construction time against `D`; the body's free-name set *is* the inventory |

For lambdas, the closure rule (§5.4) is the structural enforcement: any body free name not in the parameter list and not in the slash list is a compile-time error. The body's full inventory of upstack ties is therefore *in the slash list*; nothing arrives at the body without explicit declaration. *[CP007 §1.4; CP008 §1.2.]*

For fexprs, the inventory is implicit but determinable: the body's free-name set is enumerable at compile time, and the captured-slot list is the typechecker's record of that enumeration. The user does not write the list; the typechecker computes it. The provision-chain rule is preserved structurally — the body cannot reach state outside its captures — even though the inventory is not explicitly written. *[CP010 §2.1; CP007 §1.3.]*

### 9.3 Why Implicit Capture Is Acceptable for Fexprs

The provision-chain rule's purpose is to make a body's dependencies *auditable*. For lambdas (which can travel), the audit happens at the slash list; the user reading the literal sees the full inventory. For fexprs (which cannot travel above `D`), the audit happens at `D`'s body — the user reads `D`'s body and the fexpr literal together, and can determine what the fexpr captures by reading the body's free-name resolutions against `D`'s scope.

The asymmetry mirrors the travel asymmetry: travel-bearing forms (lambdas) require explicit-at-sight auditability; non-traveling forms (fexprs) can rely on the local-context audit. *[Reconciliation per 2026-05-04 dialog and CP007 §2.3's three-mechanism rationale, recast in the four-form taxonomy.]*

### 9.4 No-Hidden-Control-Flow Preservation

The provision-chain rule's sister principle, no-hidden-control-flow (CP002 §1.4), is also preserved: each form's invocation does what the source code makes visible. A command-reference invocation calls the referenced command (visibly named at the construction site). A command-literal / lambda invocation runs the body (visibly written in the literal). A fexpr invocation runs the body (visibly written) with the captured-slot inheritance the user can read off `D`'s scope. No hidden execution paths arise from any of the four forms. *[CP002 §1.4; CP007 §1.5.]*

The at-stack-exit mechanism (op-sem §6) is the language's single intentional exception to no-hidden-control-flow; it is not introduced by any of the four forms (their bodies may register at-stack handlers via standard operations, but the four forms themselves don't add new exceptions).

*Sources for §9: CP007 §§1.2, 1.3, 1.4, 1.5 (provision-chain rule and its application to each form); CP002 §1.4 (no-hidden-control-flow); CP008 §1.2 (lambda closure rule); CP010 §2.1 (fexpr implicit-capture-by-free-name); the 2026-05-04 dialog (audit-asymmetry framing).*

---

## 10. Open Questions

This section catalogs the lambda-and-fexpr-related open questions. Resolutions covered by this reference are noted at the section where they are settled (with cross-references here for completeness); forwarded sub-questions are recorded; genuinely open items registered by this reference are described in full.

### 10.1 OQ-7 — Closed Historically (§6.11)

OQ-7 (block-quote macro/fexpr semantics) was registered in CP002 and refined through CP007. The fexpr design articulated in CP010 §§1–7 resolved the design question; this reference's §6 codifies CP010's resolution into reference form. OQ-7 is closed; no further design work is needed. *[CP010 §1; this reference §6.]*

### 10.2 OQ-2(b) — Resolved Here (§§4.6, 6.8)

OQ-2(b) asked how the block-quote `:{...}` (now: fexpr) and command-literal `:<...>{...}` forms' parameter-passing should be treated under CP013's transitive READ contract. The resolution: standard transitive READ rules apply uniformly; the two forms differ in *what is passed* (fexpr: nothing — there are no parameters from the invoker's vantage; command literal: parameters per the angle-bracket signature, treated like any other command's parameters), not in how the transitive READ contract bites once parameters exist. *[Per §4.6 for command literals; per §6.8 for fexprs; encoded from inception per the 2026-05-04 dialog.]*

The construction reference's failure-atomicity rules (§7) apply uniformly to invocations of all four forms. The op-sem reference's parameter-passing mechanics (§5) apply uniformly. The lambda-and-fexpr-specific consideration is the capture-list / implicit-capture mechanism, which is treated in §§5–6 as separate concerns from invocation parameter-passing.

### 10.3 OQ-6 (Capture-Side) — Resolved Here (§§5.2–5.5)

OQ-6 (partial application beyond receiver-elision) split three ways per the 2026-05-04 dialog: receiver-elision via `::` cross-references op-sem §5.6; capture-style partials are owned here (lambda and command-reference partial application); class-system-style partials with bound receivers are forwarded to the class-system reference.

The capture-side (lambda explicit captures and command-reference bound arguments) is resolved in §§3.4–3.6 (command reference) and §§5.1–5.7 (lambda). The class-system-style partials (where instance dispatch chooses a method to bind) remain forwarded. The receiver-elision case is settled by op-sem §5.6 cross-reference.

### 10.4 OQ-18 — Resolved Here (§5.11)

OQ-18 asked how a lambda's signature appears at the type-system surface — what the user writes, what the typechecker reasons over, and what shape a partially-applied lambda has. The resolution at §5.11: the visible signature *hides* capture-list details and *surfaces* invoke-method-parameter modes (`'`, `&`, IN). A lambda's type is the signature with parameters and modes visible; the captures are an implementation matter of the lambda's identity, not part of its type. A partially-applied lambda's type is the signature with the bound positions removed and the remaining positions visible. *[CP008 §11 OQ-18 framing; this reference §5.11 resolution.]*

### 10.5 OQ-21 (Lambda-Side) — Resolved Here (§5.4)

OQ-21 asked whether lambda captures can cover implicit-context-parameter inheritance from the defining frame's `/`-list, or whether the closure rule needs an exemption. The resolution at §5.4: **no inheritance**. Lambdas must explicitly capture any defining-frame context variables they need; the closure rule does *not* exempt context-variable references. The capture list is the lambda's complete statement of upstack ties. The construction reference's §13.7 records the construction-side half of OQ-21 (already resolved by CP015 §8.4). The fexpr side is treated in §6.3 / §8.5 — fexprs *do* see `D`'s context parameters, by virtue of the implicit-capture-by-free-name model. *[CP008 §11 OQ-21 framing; this reference §5.4 resolution; CP015 §8.4 construction-side; the 2026-05-04 dialog.]*

### 10.6 OQ-25 — Resolved Here (§§5.10, 6.6)

OQ-25 (capture-shadowing) asked how a capture interacts with a lexical name that the body might also bind. The resolution differs by form:

- **For lambdas (§5.10):** body-locals are permitted to shadow captures within their scope; the standard same-scope-rule applies — the body-local takes precedence within its visible region; outside that region, the capture is visible. Standard lexical-scoping rules from type-system §4.3, applied to the lambda body's CFG.
- **For fexprs (§6.6):** body-locals *cannot* shadow captures; a `#`-introduction whose name collides with a `D`-visible name is a compile-time error. The stricter rule preserves the up-stack-effect intuition that fexpr bodies operate on `D`-state under `D`'s mode-markings.

The asymmetry is intentional: lambdas have explicit captures and the user can read them off the slash list at the literal site, so shadowing is auditable. Fexprs have implicit captures and the user reads `D`'s body to determine the capture set, so shadowing would defeat the audit. *[CP010 §2.4 OQ-25 framing; this reference §§5.10, 6.6; the 2026-05-04 dialog.]*

### 10.7 OQ-32 — Newly Registered: Fexpr-Typing Ramifications

The 2026-05-04 dialog settled the surface (`<*>` typing for fexpr-typed slots in command-type-expressions) but left several ramifications open. Registered as OQ-32, deferred for a future thread:

- **Fexpr-typed return-from-call patterns.** A constructor that produces a fexpr-typed `'r` is forbidden by §6.5 Restriction F. But what if a non-productive call returns a fexpr through some other channel — a `-> name` mechanism on a non-productive parameter, an error-payload variant whose candidate is fexpr-typed, or some other indirect channel? The structurally consistent rule is "any channel that moves a fexpr above `D` is forbidden," but each channel needs its enforcement worked out.

- **Class-method fexpr-typed parameters.** §11 Restriction A allows IN-mode fexpr-typed parameters, including on class methods. The typechecker treats fexpr-typed parameters under instance dispatch identically to other parameters (the instance is just a dispatch mechanism; the fexpr's ceiling is set at the call site, not at method-resolution). Edge cases — particularly partial-application of class methods that take fexpr parameters — need spelling out.

- **Command references and fexpr-typed referents.** §3.7 disallows command-reference-to-fexpr (`{some_fexpr}` is rejected) and fexpr-typed bound arguments (`{some_cmd: ..., my_fexpr, ...}` is rejected). The conservative-by-default choice; whether a future relaxation is sound (e.g., a command-reference scoped to be itself fexpr-typed-ceiling-respecting) is open.

- **Fexpr-typed values as variant candidates inside variants inside ... .** §6.5 Restriction C permits fexpr-candidate variants only when the variant is in a local slot. The transitive containment rule — a variant containing a fexpr cannot itself appear in object fields, in non-local-slot positions, etc. — is consistent in spirit but needs precise typechecker rules. The construction reference's §3.5 will receive a touch-up note (per work plan §6).

- **The narrowing operator (`-<`) and fexpr-typed values.** Construction §11's `-<` for variants admits any non-fexpr candidate. For a fexpr candidate, the question is whether narrowing is even meaningful when the value cannot escape `D` and cannot be value-moved. The user's read in dialog: "I do not understand how the narrowing operator can apply to fexprs." Bracketed for follow-up.

Resolution requires a separate design dialog. This reference notes the gap with `see OQ-32` cross-references where ramifications would otherwise need spelling out. *[Reconciliation: OQ-32 is registered new in this reference per the 2026-05-04 dialog.]*

### 10.8 Forwarded to Other References

The following items, mentioned in this reference but owned elsewhere, are recorded here:

| Item | Owner | Status |
|---|---|---|
| OQ-2(a), (c) | Closed by CP013's transitive READ contract | Already resolved per op-sem §8.1 bridge note |
| OQ-2.1, OQ-2.2 | typechecker-implementation thread | Forwarded; closed-without-action per type-system §§7.13–7.14 |
| OQ-5 (instance coherence sub-questions) | Class-system reference | Forwarded |
| OQ-6 (class-system-style partials with bound receivers) | Class-system reference | Forwarded; capture-side resolved here |
| OQ-13 | Construction reference §13 | Resolved 2026-05-03; class-system uninvolved |
| OQ-16 (overloading on dispatched commands) | Class-system reference | Forwarded |
| OQ-19 | type-system §4.4 | Resolved per CP008 |
| OQ-20 (slash-list grammar details) | Implementation thread | Forwarded |
| OQ-23 (`.inline`-placement remainder) | Implementation thread | Forwarded; lexer-disambiguation half closed by CP015 §2 |
| OQ-29 (Liskov failure-tag opening) | Class-system reference | Forwarded |
| OQ-30 (heap allocation) | Future heap-allocation thread | Deferred |
| OQ-31 (tuple-positional access) | Future thread | Deferred |
| OQ-32 (fexpr-typing ramifications) | Future thread | **Newly registered here** |

*Sources for §10: per-section cross-references; CP008 §11 (OQ-18, OQ-19, OQ-20, OQ-21 origins); CP010 §10 (OQ-25 origin); CP015 §§8, 10 (OQ-13, OQ-22, OQ-24 resolutions, OQ-30, OQ-31 registrations); the 2026-05-04 dialog (OQ-32 registration).*

---

## 11. Provenance

**Authored:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, on 2026-05-04, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` (2026-04-29), continued with `reference-operational-semantics.md` (2026-04-29), `reference-type-system-and-modes.md` (2026-05-02), and `reference-construction-and-initialization.md` (2026-05-03). Drafted in a fresh single-session pass per the 2026-05-04 scope-confirmation and clarification dialog, encoding all dialog commitments from inception with reconciliation markers identifying each amendment.

**Source materials:** intent-checkpoint-001.md (foundational command-typed-value first-class status, the `/` separator for context parameters, expression-position calling convention; OQ-6 partial-application origin); intent-checkpoint-002.md (block-marker constructs cross-referenced for body-internal failure flow); intent-checkpoint-007.md (the original three-form syntactic distinction, with terminology revised per 2026-05-04 dialog: bare `{...}` was "lambda" / "dispatch-capture" — corrected to "command reference"; angle-bracket form was "lambda-with-body" — corrected to split into "command literal" without captures and "lambda" with captures; the provision-chain rule); intent-checkpoint-008.md (the slash-list capture mechanism for lambdas; mode-marker uniformity in invoke-method parameters; ceiling-tracking for reference captures; reference-chain flattening; per-invocation copy-restore for `&` captures; the closure rule; the aliasing rule; lightweight-vs-ceiling-tracked phenotypes; OQ-19 resolved into type-system §4.4); intent-checkpoint-010.md (the fexpr design — implicit-capture-by-free-name, the per-invocation invocation frame, the locality rule, direct-access captured-slot semantics, OQ-7 resolution, OQ-25 introduction; the framing of the fexpr as closure over the defining frame, sharpened by 2026-05-04 dialog); intent-checkpoint-013.md (transitive READ contract, applied here at §§4.6, 6.8 for OQ-2(b)); intent-checkpoint-015.md §8.4 (the `.implicit`-excludes-`.context` constraint, which closes the construction-side half of OQ-21).

Secondary references consulted for cross-cutting context: `reference-failure-system.md` §§1.3–1.5 (mark-subsumption and conformance rules, applied uniformly across the four forms in §7); `reference-operational-semantics.md` §§3.3, 5.5, 5.6, 6, 7 (provision-chain rule, implicit context parameter resolution, scope operator's invoke role, object lifecycle, surface distinction now superseded by this reference's terminology); `reference-type-system-and-modes.md` §§3, 4, 5 (command-typed values in the type system, mode-marking discipline, subsumption); `reference-construction-and-initialization.md` §§2, 6, 11, 13 (construction surface for command-typed values, bare-identifier value-copy with fexpr exclusion needed, `-<` operator's command-typed cases needing fexpr-exclusion verification, context-parameter mechanics); `project-operating-principles.md` §§1.1, 1.2, 1.4, 1.6, 1.8, 2.1, 2.9 (frame-ownership, no-cross-frame-propagation, buffer-backed containment, variant absent state, orthogonality of language and standard library, scope-confirmation discipline, topic-closure discipline).

**Resolutions recorded in this reference:**

- **OQ-2(b)** resolved at §§4.6, 6.8 — block-quote (fexpr) and command-literal parameter-passing under transitive READ. Standard transitive READ rules apply uniformly; the two forms differ in *what is passed*, not in how the contract bites. *[CP013 §7.3 forwarded; resolved here per 2026-05-04 dialog.]*
- **OQ-6 (capture-side)** resolved at §§3.4–3.6 (command-reference) and §§5.1–5.7 (lambda). Receiver-elision case via op-sem §5.6 cross-reference; class-system-style partials with bound receivers remain forwarded. *[CP001 §6 OQ-6 framing; resolved per 2026-05-04 dialog three-way split.]*
- **OQ-18** resolved at §5.11 — visible signature surfaces invoke-method-parameter modes; hides capture-list details. *[CP008 §11 OQ-18 framing.]*
- **OQ-21 (lambda-side)** resolved at §5.4 — no inheritance of defining-frame context parameters; explicit-capture-only. *[CP008 §11 OQ-21 framing; CP015 §8.4 construction-side; resolved per 2026-05-04 dialog.]*
- **OQ-25** resolved at §§5.10 (lambda permits shadowing) and §6.6 (fexpr forbids shadowing). *[CP010 §10 OQ-25 framing; the asymmetry encoded per 2026-05-04 dialog.]*
- **OQ-7** confirmed historically resolved by CP010, recorded in §6.11.

**Forwarded sub-questions:**

- **OQ-6 (class-system-style partials with bound receivers)** forwarded to the class-system reference per the 2026-05-04 three-way split.
- **OQ-20 (slash-list internal grammar details)** forwarded to the implementation thread.

**Newly registered open questions:**

- **OQ-32 — Fexpr-typing ramifications.** Recorded at §10.7. Resolution requires a separate design dialog. Sub-items: fexpr-typed return-from-call edge channels; class-method fexpr-typed parameter edge cases; command-reference relaxation possibilities; transitive variant-fexpr-candidate containment rules; `-<` operator's interaction with fexpr-typed slots.

**Reference-document impact (pending consolidations).** The following revisions to other references are required as a consequence of the commitments in this reference; collected in the work plan's §6 for a future consolidated pass rather than applied here:

- **Operational-semantics reference §7** — supersede with a brief bridge note pointing here. The existing §7 uses superseded terminology ("lambda" for bare `{...}`); the bridge note acknowledges supersession and forwards.
- **Operational-semantics reference §5.5** (per CP015 §8.5) — redirect OQ-13's class-system forwarding to construction reference. Already noted in the construction-reference §15 batch; recapitulated here.
- **Construction reference §6** (bare-identifier value-copy) — exclude fexpr-typed values from the "command-typed values" permitted-copy category. The exclusion is needed under the locality rule; the construction reference's current text covers all command-typed values uniformly.
- **Construction reference §11** (`-<` operator) — verify command-typed-value cases don't apply to fexpr-typed values; flag if they do. *[OQ-32 sub-item.]*
- **Construction reference §3.5** — variant-fexpr-candidate constraint: variant candidates may have fexpr type only when the variant inhabits a local slot. Touch-up to current §3.5's treatment of variant candidates.
- **Construction reference §13** — terminology shift from "context parameters" to "context variables" with "implicit context parameters" attaching specifically to the regular-command-side mechanism. Already noted in the construction-reference §15 batch (per 2026-05-04 dialog).
- **Type-system reference §3** (subsumption-relations description) — fexpr-typed (`<*>`) and ordinary command-typed (`<>`) are non-subsuming families; mark-subsumption applies symmetrically within each family but not across the boundary.
- **Type-system reference §4.4** (mode-marking discipline) — fexpr-typed values' non-escape rule articulated as a structural mode-marking rule. The related grammar item — the `<*>` typing surface in command-type-expressions — is added to OQ-23's implementation-thread grammar batch (next to `.inline`-placement and other small lexer-disambiguation matters).
- **Type-system reference §3** (variants) — variant-with-fexpr-candidate constraint added; cross-reference to construction reference §3.5 and this reference §11 Restriction C.
- **Operating-principles document** — three pending updates already batched (§§3.9, 2.10, 1.5 from CP015 dialog); two additional from 2026-05-03 dialog (§2.1 sharpening, §2.9 new); none new from this 2026-05-04 dialog (the OPS §3 fexpr-defining-frame-closure miscalibration is left at reconciliation-marker level per user judgment).

**Carried-in commitments encoded from inception (not from prior reference's framing):**

- **Four-form taxonomy and naming** (2026-05-04 dialog): command reference, command literal, lambda, fexpr, with command-typed value as the umbrella. Encoded throughout this reference; supersedes CP007 §2 / CP008 / op-sem §7 terminology.
- **Fexpr as closure over `D`** (2026-05-04 dialog sharpening of CP010): the body's lexical environment is `D`, not `I`; the invoker is opaque except for the failure mark. Encoded in §6 from inception.
- **The `D`/`I`/`F` frame naming** (2026-05-04 dialog): defining frame, invoker frame, per-invocation frame. Used throughout §6.
- **The `<*>` typing surface for fexpr-typed slots** (2026-05-04 dialog): nominal-type distinction at parameter-declaration sites; mark-subsumption applies symmetrically within the family but not across the boundary. Encoded in §§3.1, 6.7, 7.3 from inception.
- **Mode-mark-subsumption symmetry** (2026-05-04 dialog confirmation): standard `:` ⊑ `?` and `!` ⊑ `?` apply within both families; family boundary is non-subsuming. Encoded throughout §7.
- **Bidirectional failure-mark conformance for fexprs** (2026-05-04 dialog): body conforms in `D`'s analysis context; invoker respects advertised mark. Encoded in §§6.8, 7.2.
- **Lambda non-inheritance of defining-frame context parameters** (2026-05-04 dialog OQ-21 lambda-side): explicit-capture-only; no implicit inheritance. Encoded in §5.4 from inception.
- **Fexpr inheritance of defining-frame context parameters** (2026-05-04 dialog): implicit-capture-by-free-name extends to `D`'s `/`-list context parameters. Encoded in §§6.3, 8.5 from inception.
- **Capture-shadowing asymmetry** (2026-05-04 dialog OQ-25 resolution): lambdas permit; fexprs forbid. Encoded in §§5.10, 6.6.
- **Variant-with-fexpr-candidate relaxation** (2026-05-04 dialog): permitted only when variant inhabits a local slot. Encoded in §6.5 Restriction C; CP010 §4.2's outright prohibition is relaxed.
- **Constructor cannot produce fexpr** (2026-05-04 dialog Restriction F): explicit beyond CP010's §4.2 Restriction A. Encoded in §6.5.
- **Fexpr cannot be assigned to defining-frame writeable parameters** (2026-05-04 dialog Restriction G): explicit beyond CP010's §4.2 Restriction A. Encoded in §6.5.
- **Command references cannot reference or bind fexpr-typed values** (2026-05-04 dialog): conservative-by-default; encoded in §3.7.

**Recommended next step.** User review section-by-section, revisions in place — applying the §2.9 topic-closure discipline. After this reference is settled, the next topic-organized reference per the agreed sequence is the **Class System** (#6) — owning OQ-5 (instance coherence), OQ-6 (class-system-style partials), OQ-16 (overloading on dispatched commands), OQ-29 (Liskov-style opening of failure-tag hierarchy under payload-covariance), Haskell-style dictionary passing details, and the cross-module Liskov question for failure tags.

The pending consolidations enumerated above (op-sem §7 supersession; construction §§3.5, 6, 11, 13 touch-ups; type-system §§3, 4.4 touch-ups; OPS §§2.1 / 2.9 / 3.9 / 2.10 / 1.5 batched updates) remain in the work plan §6 batch, to be applied in a single consolidated pass at a convenient point — they are individually small revisions and benefit from being batched.

A grammar item warrants explicit registration with the implementation thread (work plan §6 / OQ-23 batch): the `<*>` form for fexpr-typed signature positions in command-type-expressions, parallel to but distinct from the existing `<sig>` form for ordinary command-typed values.
