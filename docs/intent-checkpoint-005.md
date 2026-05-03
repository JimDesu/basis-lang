# Basis Language — Intent Checkpoint 005

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-004.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27. This checkpoint resolves OQ-11 (marker syntax) and OQ-12 (receiver modes), establishes the R1+R2 rules governing v-command receivers, distinguishes three v-command receiver modes by their idiomatic use (including the externalized-effects pattern for IN receivers), pins down the constructor and at-stack receiver constraints, and introduces the downcast intrinsic as a named topic to be designed in a dedicated future thread.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Marker Syntax: `'` and `&` Are Identifier Shapes

The productive (`'`) and reference (`&`) writeability markers introduced in checkpoint 004 are **part of the identifier itself**, not separate marker tokens.

This is a meaningful structural commitment. The identifier `'r` is a different identifier from `r`. The identifier `&x` is a different identifier from `x`. Every read and every write in the command body uses the marked form, so the marker is visible at every use site, not only at the declaration.

### 1.1 Why Identifier-Shape

Markers as separate attribute tokens require the reader to remember the declaration to know what kind of slot they are looking at. Markers as identifier shape put the information at every reference. References vastly outnumber declarations in real code, so visibility-at-references is where the convention earns its keep.

A line like `'r <- compute: x` reads at sight as "write to the success-emitted result." A line like `log: 'r` reads as "log the success-emitted result." There is no need to consult the signature.

Symmetrically for `&x`: every read or write of `&x` in the body announces "this is the reference parameter the caller handed me; modifying it modifies the caller's slot, leaving it alone leaves the caller's slot alone."

### 1.2 The Same-Scope Rule

Within any scope, a productive identifier `'x` and a non-productive identifier `x` (with the same trailing name) **must not both exist**. Their coexistence is a semantic error.

This applies symmetrically: `&x` and `x` should also be considered for the same restriction (see OQ-14 below — flagged as a small open sub-question).

The rule exists to prevent the subtle reader-confusion case where two textually-similar names refer to genuinely different slots in the same context. The cost of forbidding it is essentially zero (rename one of them); the benefit is that scope-local reasoning never has to disambiguate near-identical names.

### 1.3 Lexer and Grammar Implications

The current grammar (`Grammar2.cpp` master) recognizes apostrophe-bearing identifiers via the existing identifier production. The reference marker `&` requires a lexer/grammar update so that `&x` parses as an identifier in the same way `'x` does.

The existing postfix `&` (the `CALL_EXPR_ADDR` production: `someValue&` for address-of) **keeps its current role**. The new prefix-`&`-as-identifier is added in parallel; disambiguation is by context (parameter position vs. expression suffix position), and the contexts do not overlap.

### 1.4 The Asymmetry With Command-Type-Expressions

Command-type-expressions (`:<...>`, `?<...>`, `!<...>`) have no parameter names — they specify only the parameter types. The productive marker therefore cannot travel with a name in those contexts and falls back to its current placement: **suffix on the type**, e.g. `:<Int32', String>`.

This asymmetry is principled, not accidental. Pointers (`^`) attach as prefix-on-type. Putting the productive marker also as prefix-on-type would create a single visual blob (`'^Type` or `^Type'`-as-prefix) that is hard to read. Putting `^` as prefix and `'` as suffix keeps the two markers visually distinct on opposite sides of the type. The asymmetry is the language's response to whether names are available to carry the marker:

- **Names available** (command definitions, parameter declarations, receivers): markers travel with names. `Type 'name`, `Type &name`.
- **Names absent** (command-type-expressions): markers attach to the type as suffix, leaving the prefix free for `^`.

The two placements agree on what the markers *mean* (productive-writeable, reference-writeable) but differ on placement to suit the surrounding syntax.

A future cleanup may revisit whether the reference marker should also appear in command-type-expressions, and if so what its placement should be. For now, command-type-expressions retain the current single suffix marker (the apostrophe), which serves the productive-writeable role in those contexts.

### 1.5 Resolution of OQ-11

OQ-11 (final marker syntax) is resolved on the terms above: `'` and `&` as identifier-shape in named contexts, with the productive marker keeping its current suffix-on-type placement in nameless command-type-expressions.

---

## 2. Receiver Modes and the R1+R2 Rules (Resolution of OQ-12)

V-command receivers are explicitly mode-marked, like all writeable parameters. There are no implicit defaults: every receiver in every signature shape carries an explicit mode marker.

This is "Approach A" from the prior dialog: maximally uniform, maximally explicit.

### 2.1 The Two Rules

V-command receivers follow two rules that together replace the simpler "receivers are reference parameters" instinct:

**Rule R1 (call-site obligation).** At a v-command call site, every receiver must be initialized — independent of the receiver's mode. Dispatch fundamentally requires the receiver to exist as a real value at runtime, because the dispatch mechanism resolves a method-bearing value from the receiver's type and invokes it on the receiver's value. Dispatching on an uninitialized slot is meaningless.

**Rule R2 (callee-body obligation).** A v-command's body, with respect to a receiver of mode M, has the same callee-side obligations as a parameter of mode M would, per checkpoint 004 §3:

- **Productive `'`** — must write on every successful return path.
- **Reference `&`** — no write obligation; may read and may write.
- **IN (no marker)** — may read; may not write.

R1 lifts the caller-side obligation uniformly across all receiver modes (always initialized at the call site). R2 keeps the callee-side variation that the writeability marker is for. The two rules together say: dispatch always operates on a real receiver, and the marker tells the callee what it commits to doing with that receiver.

### 2.2 Receiver Modes and Their Idiomatic Uses

Each of the three v-command receiver modes has a distinctive idiomatic use:

**IN receiver — externalized effect system.** An IN-mode receiver is a method that operates *through* the receiver without modifying it. The classic case is logging: `logger :: log: message` writes a log entry. The logger's state is unchanged; the world (the log file, the log stream, whatever the logger is connected to) is changed. The receiver mediates an effect external to itself — the effect happens through the connection-state the receiver carries, not on the receiver itself.

This pattern preserves the no-non-local-state principle. The receiver is a parameter, so accessing its connection-state is local. The external effect happens through some lower-level command that itself takes the connection-state as a parameter. The chain of parameter-passing is what licenses the external effect; nothing reaches for non-local state.

Other examples: network sockets used to send (the socket is the channel, the bytes are the effect on the wire); metrics emitters; event publishers; file handles in append-only mode where the handle is itself unchanged.

**Reference `&` receiver — modify the receiver in place.** The method may read the receiver's current state and may modify it. Pre-call: initialized. Post-call (success or failure): initialized, possibly with a different value.

This pattern fits cleanup methods (the at-stack-method form, treated separately in §3 below), state transitions in objects, in-place updates, and the "modify if needed" idiom from the statechart example in checkpoint 004 §5.4.

**Productive `'` receiver — re-initialize the receiver.** The method commits to writing the receiver on every successful return path (R2 for productive). Combined with R1, the receiver must be initialized pre-call and the method overwrites it on success.

This pattern is unusual — it is specifically a "factory method" or "complete reset" operation that happens to dispatch on the receiver's existing type. The pattern is preserved as a coherent option rather than ruled out, on the principle that clean rules are better than special-case wrinkles. Concrete use cases will emerge or they won't; either way, the language need not treat productive v-command receivers as a special case.

### 2.3 Constructor Receivers

Constructors retain their current grammar shape: `<Type> <ReceiverName> : <parm1>, <parm2>, ...`. The `DEF_CMD_CTOR` production is unchanged in structure.

What changes under this checkpoint: **the receiver name must carry the productive marker `'`**, and only the productive mode is valid. The constructor signature form is therefore:

```
T 'r : a, b
```

where `T` is the type being constructed, `'r` is the productive receiver named `r`, and `a, b` are IN parameters (or whatever modes the user marks them).

The receiver mode is not implicit — the apostrophe must appear. Reference (`&r`) and IN (no marker) are forbidden for constructor receivers. The reasons:

- A reference-mode constructor receiver would mean "construct drawing on the receiver's existing state" — which is not construction; it is in-place modification, which is what reference v-command methods are for.
- An IN-mode constructor receiver would mean "construct a thing the caller cannot observe" — forcibly producing an inaccessible object, which has no purpose in the language.

The expression-position calling convention from the README and prior checkpoints continues to apply: `# r <- (T: a, b)` desugars to `# r; T: r, a, b`, with the calling context supplying the productive slot. The receiver-name in the signature exists for the body to refer to — the call site provides the slot via expression-position context.

### 2.4 At-Stack Method Receivers

At-stack methods (`DEF_CMD_RECEIVER_ATSTACK` for `@` and `DEF_CMD_RECEIVER_ATSTACK_FAIL` for `@!`) take a single receiver. Under this checkpoint:

- The receiver's mode marker **must be explicit**.
- The valid modes are **IN** and **reference `&`**.
- The productive mode `'` is **forbidden** for at-stack receivers.

The reasoning:

- At-stack methods run at frame exit on objects that exist (otherwise there would be nothing to clean up). Productive mode would mean "construct the object as part of cleanup," which is meaningless — the object's existence is the precondition for cleanup, not its outcome.
- IN at-stack receivers are useful for cleanup that does not modify the object — for example, "log this object's final state at frame exit." The object is observed but not changed.
- Reference `&` at-stack receivers are the typical case — cleanup that may inspect, finalize, or mark the object as released. Resource-managing types use this form.

### 2.5 Resolution of OQ-12

OQ-12 (default mode for receivers in different signature shapes) is resolved: there are no defaults; explicit markers are required everywhere they are valid. Each signature shape restricts the set of valid modes to those that are semantically meaningful for that shape:

| Signature shape | Valid receiver modes | Forbidden modes |
| --- | --- | --- |
| Constructor (`DEF_CMD_CTOR`) | productive `'` only | reference, IN |
| V-command (`DEF_CMD_VCOMMAND`) | productive `'`, reference `&`, IN | none |
| At-stack `@` (`DEF_CMD_RECEIVER_ATSTACK`) | reference `&`, IN | productive |
| At-stack `@!` (`DEF_CMD_RECEIVER_ATSTACK_FAIL`) | reference `&`, IN | productive |

In all cases the marker placement is identifier-shape per §1.

---

## 3. The Downcast Intrinsic (Named Topic, Design Deferred)

During the dialog leading to this checkpoint, a design idea emerged that is worth recording here as a *named topic* for a future dedicated thread: a built-in **type-checking / type-narrowing intrinsic** that allows code to test whether a value satisfies a more-specific type than its current static type indicates, and on success, statically narrow the type within a code region.

### 3.1 The Idea, Tentatively Stated

A type-generic intrinsic, with working name `?typeCheck[T]: &x` (the exact name and generics syntax are placeholders), behaves operationally as a no-op at runtime — its work is done at compile time and via whatever validation the type's invariant requires. It takes a reference parameter and:

- **Succeeds** if the parameter's value satisfies type `T` (where `T` is more specific than the parameter's current static type).
- **Fails** otherwise.
- On the **success path**, the typechecker narrows the parameter's type to `T` within the code region where the success path is statically reached.
- On the **failure path**, the parameter retains its original type.

The natural use site is inside a `?`-block guard:

```
?? ? ?typeCheck[Foo]: &x
        ; in this block, x has narrowed type Foo
        ...use x as a Foo...
```

Within the body, `x` is `Foo`-typed; outside, `x` retains its broader type.

### 3.2 Why This Matters

Earlier in the dialog the question was raised whether constructors could pull double duty as downcasters — `Foo: x` meaning either "construct a `Foo` from `x`" or "test whether `x` is a `Foo`," disambiguated by the receiver's mode. That double duty was rejected as syntactically ambiguous and semantically muddled: construction and downcasting are genuinely different operations, and giving them the same surface form forces every reader to consult the declaration to know which is happening.

Making the downcast its own intrinsic gives the type-narrowing effect a syntactic home of its own. The type-narrowing — the *interesting* semantic move — becomes the defining feature of the operation, rather than an implementation detail of a constructor variant.

### 3.3 What the Future Thread Needs to Settle

The full design has been deferred and is recorded here as **OQ-15**. The questions to settle:

- The exact intrinsic name and call shape.
- The generics syntax (the parameterization over `T` — likely `[T]` per the existing `TYPE_NAME_ARGS` form, but to be confirmed).
- Which types support being the target `T`. Domain children of the parameter's static type are the obvious case; pointers and fat pointers introduce their own concerns (which connect to the open Note 2 thread on static vs. dynamic dispatch and OQ-1 on union discriminator representation).
- The precise rules for narrowing scope: clear in `?`-blocks and `?-`-blocks; less clear at joins where some incoming paths have the narrowing and others do not (the narrowing is presumably lost at such joins, by intersection-of-paths reasoning, but should be made explicit).
- Interaction with class dispatch — does the narrowed type's class instance govern subsequent dispatch within the narrowed region?
- Interaction with unions and variants — does the intrinsic test which candidate a union value is, narrowing to that candidate's domain on success?
- Interaction with the initialization analysis — `&x` per checkpoint 004 §3.5 already requires `x` to be initialized, so the intrinsic does not need to handle uninitialized inputs.

This is a real thread, not a small one, and worth a checkpoint of its own.

---

## 4. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. Connects to Note 2. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Open | Partial application beyond receiver-only. |
| OQ-7 | Open | Block-quote macro/fexpr semantics. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Open | Field-level vs. whole-slot initialization tracking. |
| OQ-10 | Open | Composite initializers (`=` vs. `<-`). |
| OQ-11 | **Resolved** | Markers are identifier-shape; suffix-on-type only in nameless command-type-expressions. |
| OQ-12 | **Resolved** | Approach A (explicit markers required). Per-shape valid-mode tables in §2.5. |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | **New** | Same-scope rule for `&x` and `x`: by analogy with the `'x`/`x` rule, presumably forbidden. To confirm. |
| OQ-15 | **New** | Full design of the downcast intrinsic (`?typeCheck[T]: &x` working name). Deferred to a dedicated thread. |

---

## 5. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-004.md, and the `Grammar2.cpp` (master branch) inspection performed for checkpoint 004.

**Grammar updates implied by this checkpoint:**
- Lexer/parser: extend `&x` to be recognized as an identifier shape, parallel to the existing apostrophe-bearing identifier handling for `'x`. The existing postfix `&` (`CALL_EXPR_ADDR`) is unaffected.
- `DEF_CMD_PARM`: no structural change needed, since the marker now travels with the identifier.
- `DEF_CMD_RECEIVER`: no structural change needed for the same reason; constructor and v-command and at-stack receiver shapes already accommodate identifier-shaped names.
- `TYPE_CMDEXPR_ARG`: unchanged in this checkpoint. The existing suffix-`'` continues to mean productive-writeable. A future cleanup may add a reference-mode marker in this nameless context if uniformity is wanted.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-005.md`). Per the prior plan, OQ-9 (field-level vs. whole-slot initialization tracking) is the next thread, followed by OQ-10 (composite initializers `=` vs. `<-`). After those, the failure-mode-and-typechecker thread proper is the natural major work to pick up.
