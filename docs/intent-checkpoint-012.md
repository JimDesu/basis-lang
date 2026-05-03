# Basis Language — Intent Checkpoint 012

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-011.md; supersedes them where they conflict. This checkpoint substantially resolves OQ-26 (typed failures), introduced as the natural next major thread by checkpoint 011. A new open question (OQ-27) is recorded for top-level failure handling at `.program`/`.test`.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-29. This checkpoint specifies: the structure of failure types (hierarchical tags forming user-defined forests, no language-imposed Top); the binding of tags to Haskell-style classes as the payload contract; the semantics of `.fail` and `|`-with-spec under typed failures; structural set inclusion as the subsumption rule for failure sets in command signatures; and the implementation discipline for failure payloads (carried in the originating frame, with a class witness in the propagating slot, under deferred-retirement of the originating frame until consumption-or-termination). The interaction with the failure-mode lattice from checkpoint 011 is sketched as a refinement direction; full lattice refinement to incorporate failure-set tracking at program points is left for the typechecker-implementation checkpoint that follows. Several smaller sub-questions are noted as open for incremental resolution.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands the following decisions:

1. **Failure types are hierarchical tags arranged in user-declared forests.** Each tag has a name and a position in some hierarchy (or is a hierarchy root). There is no language-imposed Top — no universal ancestor analogous to Java's `Exception`. Each module declares its own roots; hierarchies do not span modules implicitly.

2. **A failure tag optionally binds to a Haskell-style class — its payload contract.** The class is a typeclass in the Basis sense (a constraint on types, not a concrete type with a layout). When a tag binds to a class, every value passed at a `.fail` site for that tag must be of a type that satisfies the class; consumers binding the value via the recovery-spec form see the value at the class. A tag may also be declared without a payload class, in which case `.fail` for that tag carries no value and recovery-spec consumers do no value-binding.

3. **Different `.fail` sites for the same tag may pass values of different concrete types**, all satisfying the bound class. The tag-to-class binding is the contract; concrete-type identity is per-call-site implementation detail not visible to the consumer.

4. **Set inclusion on failure sets is structural over the hierarchy.** A failure set `A` subsumes a failure set `B` iff every tag in `B` is at-or-below some tag in `A` in their respective hierarchies. Tags from different hierarchies are incomparable; nothing in one hierarchy subsumes anything in another.

5. **Command signatures advertise their failure set as part of the failure-mode mark.** A `?`- or `!`-marked command may name an explicit failure set; the absence of an explicit set is a separate question (see §5.4 and OQ-26.1). Failure sets compose at call sites by union; subsumption rules from §4 govern signature compatibility.

6. **The failure-payload propagation slot is fixed-size: tag plus payload-pointer plus class-witness.** The payload value lives in the originating frame — the frame that fired `.fail`. Propagation copies tag, pointer, and witness up the stack; it does not copy or move the payload value itself. Consumption reads the payload through the witness; the witness exposes the class's operations to the consumer.

7. **The originating frame defers retirement until the failure it fired is consumed.** A frame that has fired an unhandled failure is not reclaimed on the normal frame-exit schedule; it stays alive until the failure is consumed (binding in a recovery handler) or until program termination. At-stack handlers attached to the payload value fire on the normal "frame retires" schedule, which is delayed on the failure path until consumption.

8. **Consumption is at the binding point, not at handler-body completion.** When `|`-with-spec engages and binds the failure value, the failure is consumed at the moment of binding. The originating frame retires at that point. The handler body executes from no-active-failure; failures fired from within the handler body originate from the handler's frame, not the originally-failing frame.

9. **Re-fail is ordinary `.fail` from the handler frame.** Where a recovery handler binds a failure value and subsequently fires a new failure (whether passing the bound value forward, transforming it, or supplying an unrelated value), this is an ordinary `.fail` originating from the handler's frame. No re-fail-specific machinery exists; the language sees a fresh failure with a fresh originating frame.

10. **Top-level failure escape is permitted at `.program` and `.test` only.** Failures may propagate up to the body of `.program` or `.test`; nothing propagates further. The precise semantics of unhandled-failure-at-top-level is recorded as **OQ-27** (see §11) and is not resolved in this checkpoint.

11. **The CP011 failure-mode lattice is consistent with this checkpoint and admits a refinement to track failure sets at program points.** The six-state lattice from CP011 §3 is type-agnostic; with typed failures, each `failing(M)` and `mixed(M)` state may be refined to carry the set of tags that may be propagating. The refinement is sketched in §9 of this checkpoint and is left to the typechecker-implementation thread for full specification.

---

## 2. Hierarchical Failure Tags

### 2.1 The Forest, Not the Tree

Failure tags form a **forest**: a collection of independent hierarchies, each rooted at a user-declared root tag. There is no language-imposed Top. The reasoning:

- A universal ancestor invites code to catch-and-bind generically, which is precisely the practice that makes failure-handling sloppy in languages that have one. Bare `|` (CP2 §2.9) already provides catch-anything-without-binding; that is sufficient for "clean up and propagate" patterns. Code that wants to *bind* a payload must commit to which hierarchies it understands.
- Cross-module composition under a forest is by explicit acknowledgment. A consumer of a foreign module's failure types must name those types in its signature; the forest structure prevents accidental capture of unrelated failures.
- The forest structure preserves the language's hexagonal-architecture-friendly properties: nominal identity, no implicit shared state across modules, no hidden dependencies among unrelated subsystems.

### 2.2 Tag Declaration Form (Sketch)

The surface syntax for declaring failure tags is **not pinned down by this checkpoint**. The semantic content of a declaration is, at minimum:

- A name introducing the tag into the surrounding scope.
- A position in a hierarchy: either as a new root, or as a child of some named parent.
- Optionally, a class binding: the typeclass that values passed at `.fail` sites for this tag must satisfy.

Declarations may be nested syntactically (a parent tag's declaration may contain its children's declarations), or they may be flat with each child naming its parent. The choice is a surface-syntax matter; both are semantically equivalent.

OQ-26.2 (see §11) records the surface-syntax question.

### 2.3 Hierarchy Semantics

A tag's **hierarchy position** is determined at declaration. A tag declared as a child of some parent is at-or-below that parent (and at-or-below all of that parent's ancestors); a tag declared as a root is at-or-below itself only.

The hierarchy is acyclic (a tag cannot be its own ancestor) and unambiguous (each tag has at most one parent). Multiple inheritance is not permitted in failure hierarchies; a tag is a member of exactly one hierarchy.

Tags from different hierarchies are mutually incomparable. This is structural: there is no language operator that reaches across hierarchies, and no implicit promotion of any tag to any other.

### 2.4 Hierarchies and Modules

Failure-tag hierarchies are declared in modules and follow the language's normal scoping and visibility rules. A module may export some, all, or none of its failure tags. Tags imported into another module retain their hierarchy position (they are still at-or-below their declared ancestors); a downstream module cannot extend the hierarchy of an imported tag (cannot add new children to a foreign tag).

This last restriction prevents a class of cross-module surprises: the set of tags at-or-below a given tag is fixed at the tag's declaration site, so a `|: SomeRootTag` handler in module `A` cannot be silently widened by module `B` adding new descendants.

---

## 3. Class-Bound Payloads

### 3.1 The Class as Contract

Every tag-with-payload binds to a class. The class is the consumer-facing contract: the operations a recovery handler may invoke on the bound value, and the only operations it may invoke. The class also constrains `.fail`: a value passed at a `.fail` site must be of a type satisfying the class.

Crucially, the class is not a concrete type. It has no layout, no size, no `sizeof`. It is a typeclass in the Haskell-style sense — a constraint on types asserting the availability of a set of operations.

This means:

- Different `.fail` sites for the same tag may pass values of different concrete types, provided each type satisfies the class.
- A consumer binding the value via the recovery-spec form does so at the class; it sees the operations the class exposes, not whatever operations the underlying concrete type happens to support.
- Refactoring the concrete type at one `.fail` site (changing it to a different type that also satisfies the class) is non-breaking. Refactoring the class (changing its operations) is breaking.

### 3.2 Tags Without Payloads

A tag may be declared without a payload class. In this case:

- `.fail` for that tag carries no value. The `.fail` form for payload-less tags accepts no payload argument (surface syntax TBD; see OQ-26.2).
- A recovery-spec consumer for that tag binds nothing. The recovery-spec form for payload-less tags accepts no identifier (or the identifier is grammatically absent, at the language's choice).
- The tag still has nominal identity, hierarchy position, and full participation in set inclusion.

A single hierarchy may freely mix payload-bearing and payload-less tags. The typechecker enforces the appropriate `.fail` and recovery-spec shapes per-tag; nothing in the hierarchy mechanism requires uniformity.

### 3.3 The `.fail` Site

`.fail Tag: ‹value›` (placeholder syntax — see OQ-26.2) requires:

- `Tag` is a declared failure tag in scope.
- If `Tag` has a bound class `C`, then `‹value›` is an expression whose type satisfies `C`. If `Tag` has no bound class, `‹value›` is grammatically absent.
- `Tag` is in the failure set declared by the enclosing command's signature (or — depending on resolution of OQ-26.1 — the surrounding signature is treated as accepting any tag, with stricter rules requiring explicit declaration).

The post-`.fail` lattice state is `failing(!)` per CP011 §5.9, refined under §9 of this checkpoint to carry the set `{Tag}` as the propagating tag set.

### 3.4 The Recovery-Spec Site

`|: Tag t ‹body›` (placeholder syntax — see OQ-26.2) requires:

- `Tag` is a declared failure tag in scope.
- If `Tag` has a bound class `C`, then `t` is an identifier introduced into the body's scope at type "any value satisfying `C`" — concretely, `t` is usable wherever `C`'s operations are required. If `Tag` has no bound class, `t` is grammatically absent.
- The block engages on a propagating failure whose tag is at-or-below `Tag` in the relevant hierarchy. Failures whose tags are not at-or-below `Tag` continue propagating per CP2 §2.10.

When the block engages and binds `t`, **the failure is consumed at the moment of binding** (per Decision 8). The originating frame retires; the handler body executes from no-active-failure. If the handler body itself fires `.fail`, that is a new failure originating from the handler's frame.

---

## 4. Set Inclusion and Subsumption

### 4.1 Failure Sets

A **failure set** is a set of failure tags. A command signature's failure-set component is the set of tags the command may emit — equivalently, the union of tags appearing at `.fail` sites in the body, including those produced by called commands' propagating failures.

A set may be expressed as an explicit list of tags, or as the closure-at-or-below of a higher-level tag. By convention, a set listing tag `T` denotes "any tag at-or-below `T` in `T`'s hierarchy"; a set listing several tags denotes the union of their respective at-or-below closures.

### 4.2 Structural Subsumption

A set `A` **subsumes** a set `B` (notation: `B ⊆ A`) iff for every tag `t` in `B`, there exists a tag `t'` in `A` such that `t` is at-or-below `t'` in `t'`'s hierarchy. Tags in `B` whose hierarchies have no representative in `A` violate the subsumption.

This is the natural set-theoretic inclusion lifted to the hierarchy. It is reflexive, transitive, and antisymmetric (modulo the at-or-below equivalence within a single hierarchy, which is trivial).

### 4.3 Subsumption at Call Sites

A caller invoking a command with declared failure set `D` is responsible for ensuring its own enclosing signature accommodates `D`. If the caller's signature declares set `E`, then `D ⊆ E` must hold (modulo any consumed-by-recovery sets within the caller — see §4.5).

A caller may *widen*: declare `E` as a superset of `D`. The typechecker accepts this. The cost is loss of precision at the caller's signature; widening is not free.

A caller may *not* narrow at a call site: declaring `E` such that `D ⊄ E` is a static error. Recovery within the caller may consume some tags from `D` before they reach the caller's exit (see §4.5), but at the call site itself the full `D` is what propagates.

### 4.4 Subsumption with Recovery

A `|`-block (bare or with-spec) consumes failures of certain tags. After the consumption, the propagating failure set narrows — the tags consumed by the block are removed from the set propagating past it.

Precisely:

- A bare `|` consumes all propagating failures. The propagating set after the bare `|` is empty (modulo failures fired *by* the handler body itself, which are fresh).
- A `|: Tag` consumes failures whose tag is at-or-below `Tag`. The propagating set after the block is the original set minus the at-or-below closure of `Tag`.

The exact representation of "set minus at-or-below closure" is an implementation matter for the typechecker's failure-set tracking. The simple approach (sets of covering nodes with subtraction support) is straightforward; the conservative approach (don't narrow on `|`-with-spec, treat the propagating set as the original) is acceptable initially per Decision 11 of CP011 and §9 below.

### 4.5 Body Conformance

Per CP011 §6, a body's exit failure-states must conform to the body's declared mark. With typed failures, this sharpens:

- A `:`-marked body's exit edges must have empty propagating set.
- A `?`-marked body's exit edges' union of propagating sets must be a subset of the body's declared failure set.
- A `!`-marked body's exit edges must have non-empty propagating set, and the union must be a subset of the body's declared failure set.

The CP011 conformance rules are unchanged in their failure-mode dimension; the new dimension is failure-set containment, which is independent and composes by intersection.

### 4.6 Recovery Narrowing — The Punt

For initial implementation, the typechecker may treat `|: Tag` as not-narrowing — the propagating set after the block is the same as before. This is conservative (it overstates the set, never understates) and acceptable for first-cut soundness. Refinement to track narrowing precisely is a typechecker-implementation matter and may be added as an optimization once the basic mechanism is in place.

OQ-26.3 (see §11) records the question of whether and when to commit to precise narrowing.

---

## 5. Signature Surface

### 5.1 Inline Failure-Set Lists

The primary signature surface for failure sets is an inline list of tag names. Surface syntax is open (OQ-26.2), but the semantic content is: a comma-separated list of tag names, each interpreted as the at-or-below closure of that tag.

Example (placeholder syntax): a command with mark `?` declaring failures `Net.Timeout` and any descendant of `Parse` would carry, in some position in its signature, a list-form failure set `{Net.Timeout, Parse}`.

### 5.2 Named Failure-Set Aliases

A module may declare named aliases for failure sets:

> *(placeholder syntax)* `failureset MyFailures = {Net, Parse.Malformed}`

A command signature may then reference the alias by name: the alias expands to its declared set at use. Aliases are pure sugar over the inline form; subsumption operates on the expanded set.

The motivation is refactoring economy: when several commands share a failure profile, the alias allows changes to that profile to propagate by editing the alias declaration rather than every command's signature.

### 5.3 Higher-Order Propagation Form (Deferred)

A higher-order command — one taking a callable as parameter — currently has no surface form to express "my failure set is the union of my callees' failure sets, plus whatever I add directly." Without such a form, every higher-order command must list every failure its callees might emit, with the same combinatorial explosion that plagues Java's `throws`.

A propagation form — something like "`?<T.failures>`" or similar surface — is desirable but not designed in this checkpoint. The design space is shared with the fexpr-signature thread (refined OQ-7), and the two are best resolved together: they are the same shape of problem (signatures parameterized over the callee's signature), with overlapping concerns about variance, polymorphism, and inference.

OQ-26.4 (see §11) records the higher-order propagation form and pairs it with refined OQ-7.

### 5.4 Implicit Failure Sets

When a command signature has a `?` or `!` mark but no explicit failure set, the typechecker has two possible interpretations:

- **Implicit-empty**: The command may not emit any tagged failure. Only `?` invocations of mode-marked-but-set-less callees, and bare `.fail` (if such a thing exists) are valid. This is restrictive and probably not useful.
- **Implicit-any**: The command may emit any tag from any hierarchy in scope. This is permissive but defeats the static-checking purpose of typed failures.
- **Inferred-from-body**: The typechecker infers the failure set from the body's `.fail` sites and called commands' propagating failures. This is convenient but interacts badly with separate compilation (a downstream consumer cannot know the inferred set without re-typechecking the body).
- **Required-explicit**: The typechecker rejects `?`- or `!`-marked signatures without an explicit set. This is the strictest option and the most clearly sound.

OQ-26.1 (see §11) records this question. The author's tentative recommendation is **required-explicit at module boundaries, with intra-module inference permitted** — a callable defined and used within a single module need not annotate its failure set, but anything exported must do so. This balances ergonomics (intra-module casual use is unimpeded) with separate-compilation soundness (cross-module signatures are explicit). Final resolution is left to the typechecker-implementation thread.

---

## 6. Implementation: Payload Lifecycle

### 6.1 The Slot

Each command frame carries a fixed-size **failure slot** capable of holding:

- A **tag identifier**: a small integer (sized to accommodate the language's full tag namespace; 32 bits is comfortable).
- A **payload pointer**: a pointer-sized value, pointing to the payload in the originating frame (or null for payload-less tags).
- A **class witness**: a pointer-sized value (typically the address of the typeclass dictionary for the payload's concrete type with respect to the bound class), or null for payload-less tags.

Total slot size: three words on a 64-bit target (with the tag identifier's spare bits reusable for slot-occupancy or other small flags). The slot is part of the activation record, allocated at frame entry, occupied only when a failure is in flight.

### 6.2 The Originating Frame

When `.fail Tag: ‹value›` fires:

1. `‹value›` is evaluated normally. Evaluation may itself fail; in that case the new failure replaces the intended one (the `.fail` does not fire, the evaluation's failure propagates).
2. The evaluated value is written into a payload-storage area in the *current* frame — the frame executing the `.fail` site. This frame becomes the **originating frame** for this failure.
3. The current frame's slot (or, if the failure is propagating to the parent, the parent's slot — see §6.3) is populated with the tag, a pointer to the payload-storage area, and the witness for the value's concrete type with respect to the tag's bound class.
4. The frame's normal exit is suppressed in favor of the failure-exit path; the failure-state lattice transitions from `clear` to `failing(!)` with propagating set `{Tag}`.

The payload-storage area's size in any given frame is bounded by the maximum payload size *for that frame's `.fail` sites* — not by the full failure set the frame's command may emit. A frame that fires `.fail` at exactly two sites, with two different concrete payload types, needs storage for the larger of the two; a frame that propagates failures from called commands needs no payload storage of its own (the payload lives in the callee's originating frame, which is somewhere down the call stack).

### 6.3 Propagation Up the Stack

A failure propagating from a callee to a caller does **not** copy the payload. The callee's slot is propagated by copying its three words (tag, pointer, witness) into the caller's slot. The pointer continues to reference the originating frame's payload-storage area; the originating frame remains alive (per §6.4).

This is the structural reason the slot is fixed-size: the slot is identity-of-failure, not contents-of-failure. The contents live where they were created.

### 6.4 Deferred Retirement of the Originating Frame

A frame that has fired a failure does not retire on the normal "command returned" schedule. Instead, the frame enters a **deferred-retirement state**: still allocated, payload-storage area still valid, but no longer executing.

The deferred-retirement state ends at one of two events:

- **Consumption**: A `|`-block (bare or with-spec) somewhere up the call stack engages on the propagating failure. At the moment of binding (per Decision 8), the originating frame is signaled to retire. Its at-stack handlers fire on the normal schedule (the payload's at-stack handler, plus any others associated with the frame's local values that hadn't already retired); the frame is reclaimed.
- **Termination**: The program terminates without the failure being consumed. The originating frame is reclaimed as part of program-end teardown; at-stack handlers fire as part of that teardown.

Multiple deferred-retirement frames can coexist on the stack only if they correspond to different in-flight failures — but per Decision 1 of CP012 (and explicit user constraint), a thread has at most one in-flight failure at any time. Therefore at most one deferred-retirement frame exists per thread. (Multi-thread Basis programs may have one per thread; this is consistent with thread-local activation records.)

### 6.5 Witness Construction

The class witness in the slot is the typeclass dictionary corresponding to (concrete-type-of-payload, bound-class-of-tag). At the `.fail` site, the typechecker has both: it knows the concrete type of the evaluated `‹value›` expression, and it knows the bound class from the tag declaration. The dictionary is selected at compile time and emitted as a witness pointer.

This is standard typeclass dictionary-passing, sliced through the failure machinery. No runtime witness construction is required; no lookup at the consumer site.

The consumer side, at a `|: Tag t` block, uses the witness to dispatch class operations on `t`. The witness's identity — which concrete type's dictionary it is — is opaque to the consumer; the consumer sees only the class's operations.

### 6.6 At-Stack Handler Discipline

The payload value, like any other Basis value, may have an at-stack handler. Under the deferred-retirement rule, the handler fires **once**, at the moment the originating frame retires (consumption-at-binding, or termination). It does not fire on each frame the failure propagates through.

This is consistent with the language's general at-stack rule: the handler fires when the value's home retires, not at intermediate waypoints.

The handler's effects are visible in the consumer's environment: by the time the consumer's body executes after binding, the handler has already fired (the originating frame has already retired). This is a consequence of "consumption at binding" — by the time `t` is in scope in the handler body, the failure machinery has run its course.

If this ordering is undesirable for some specific class of payloads, the workaround is at the application level: arrange the payload's class operations to capture whatever the consumer needs *before* binding (via expressions in the recovery spec itself, if grammar permits) or *during* the body's first uses (since `t`'s class operations remain valid even after the frame's retirement, provided the operations themselves don't require frame liveness — and well-designed class operations should not).

### 6.7 Re-Fail

When a recovery handler binds a failure value and subsequently fires `.fail` from its body — whether passing the bound value forward, transforming it, or emitting an entirely new value — the new `.fail` is an ordinary `.fail` originating from the handler's frame. The original failure's deferred-retirement frame has already retired (per Decision 8); the new failure has its own originating frame (the handler's), its own payload-storage area, its own slot population.

If the new `.fail`'s value expression is `t` (the bound name), then `t` is read out of the original frame's payload-storage area before that frame retires. Wait — this creates an ordering question: does `t`'s read happen before or after the original frame's retirement?

The answer falls out of normal expression evaluation: `t` is bound when the handler block engages; the handler body executes; when the body fires `.fail t`, the value `t` is read at that moment, which is during the handler body's execution. The original frame's retirement happens at the moment of binding, *before* the handler body begins. So the value of `t` must remain valid throughout the handler body, which requires the original payload-storage area to remain readable until the body completes.

Two interpretations resolve this:

- **(I-a)**: The original frame retires at binding, and `t` is bound by *value-copy* (or by-class-witness-and-pointer to a copy that lives in the handler's frame). The handler body sees `t` in its own frame; the original frame is fully reclaimed.
- **(I-b)**: The original frame's reclamation is in two phases: at binding, the at-stack handler fires; the payload-storage area remains valid for the duration of the handler body's execution, and is reclaimed at the body's exit. This is a softer "deferred retirement" — the at-stack handler runs on the binding schedule, but storage reclamation is on the handler-body-exit schedule.

(I-a) is conceptually cleaner but requires a copy on every recovery binding. (I-b) avoids the copy but splits frame retirement into two events. The practical difference is small for most workloads; the choice probably doesn't affect surface semantics. **OQ-26.5** records this for incremental settling.

For the user-facing semantics, both interpretations agree: the bound name `t` is valid throughout the handler body and may be referenced including in subsequent `.fail t` re-fail expressions. The original frame is gone — by binding or by handler-body-exit; the user does not distinguish.

---

## 7. Top-Level Failure Escape

### 7.1 The Constraint

Failures may propagate up to the body of `.program` or `.test`. Nothing propagates further; these directives are the lattice's top frame. The bodies of `.program` and `.test` admit normal recovery constructs (`|`, `|`-with-spec, `?`, `?-`, `?:`, `^`, `%`, `??`, `@`, `@!`) per CP002 and CP003; a failure propagating within such a body can be caught by recovery siblings at the body's level (with or without enclosing `%`-block organization).

A failure that escapes the entire `.program`/`.test` body — i.e., reaches the body's terminus with no recovery left — cannot propagate further. Some terminal handling must occur.

### 7.2 The Open Question (OQ-27)

The precise terminal-handling semantics is not resolved in this checkpoint. The candidate rules are summarized in §11.2; the choice has implications for diagnostic reporting, test-framework integration, and the standard library's failure-class hierarchy.

The deferred-retirement rule from §6.4 composes naturally with whichever terminal rule is chosen: the originating frame retires as part of program-end teardown, with at-stack handlers firing as part of that teardown. The remaining question is what the program (or test) *does* with the failure before terminating: render it for diagnostic output, signal a specific exit code, mark a test as failed, etc.

### 7.3 Why This Is Top-Level-Only

The OQ-27 question is uniquely a top-level question because the rest of the language requires every failure to be either consumed by a recovery construct or propagated to a frame that *will* recover it (modulo the conformance rules that statically prevent unhandled-at-`:`-frame-exit). The top-level frame has no parent to propagate to; the rule has to terminate somewhere, and that somewhere is `.program`/`.test`.

This is also why no analogous question arises for ordinary commands: an unhandled failure in an ordinary command's body either violates the body's mark conformance (a static error) or propagates up. There is no "what happens at the end" question because there is no end short of the top.

---

## 8. Interaction with Class Dispatch

A class method's failure set must be uniform across all instances. This is implied by the call-site typing rule: at a class-dispatched call, the typechecker knows the class and the method, but not the instance; therefore the failure set must be a property of the (class, method) pair and not the instance.

This means:

- A class declaration that includes a `?`- or `!`-marked method must declare that method's failure set at the class level (or arrange for the failure set to be a class-level property by some other mechanism).
- An instance's implementation may emit any subset of the class-declared failure set; emitting tags outside the class-declared set is a static error.
- Subsumption applies: an instance's actual failure set is a subset of the class's declared set, and the class's declared set is what callers observe.

This composes cleanly with §4 and §5 of this checkpoint. The remaining detail is the surface syntax for class-level failure-set declarations, which is part of the broader signature-surface question (OQ-26.2).

A class with multiple `?`- or `!`-marked methods may declare a separate failure set per method, or share an alias across methods — both are pure surface conveniences.

---

## 9. Refinement of the CP011 Lattice

The CP011 failure-mode lattice tracks *whether* and *with what mode* a failure may be propagating at each program point. With typed failures, the lattice can be refined to track *which set of tags* may be propagating at each program point. The refinement direction is:

- Each `failing(M)` state carries an associated failure-set component `T`, becoming `failing(M, T)`.
- Each `mixed(M)` state similarly becomes `mixed(M, T)`.
- `clear` is unchanged (no failure means no failure-set).
- `unreachable` is unchanged (lattice bottom).

The join operation refines: `failing(M₁, T₁) ⊔ failing(M₂, T₂) = failing(M₁ ⊔ M₂, T₁ ∪ T₂)`, where `M₁ ⊔ M₂` is the CP011 mode-join (`! ⊔ ?  =  ?`) and `T₁ ∪ T₂` is set union over tags.

Transfer functions refine analogously:

- A call to a command with declared failure set `T` produces an outgoing state with that set on the failure edge.
- `.fail Tag: ‹value›` produces an outgoing state with set `{Tag}`.
- `|: Tag` consumes failures whose tags are at-or-below `Tag`; the propagating set narrows accordingly (per §4.6's punt-or-not choice).
- Bare `|` consumes the entire propagating set; the post-`|` propagating set is empty.

Body conformance refines per §4.5 of this checkpoint.

The refinement is compatible with the CP011 lattice: dropping the failure-set component recovers the CP011 lattice exactly. Implementations may begin with the CP011 lattice and add the failure-set component incrementally.

The exhaustiveness analysis for `|`-with-spec chains, deferred in CP011 §4.8 due to the absence of typed failures, becomes feasible under this refinement: a chain of `|: Tag₁ ... | Tag₂ ... | Tag₃ ...` can be statically verified to cover the full propagating set iff the union of the at-or-below closures of `Tag₁`, `Tag₂`, `Tag₃` includes the propagating set at the chain's entry. A chain that fails this check and lacks a bare `|` fallback emits a static warning (or error, at the language's discretion).

Full specification of the refined lattice and its transfer functions is left to the typechecker-implementation thread that follows this checkpoint.

---

## 10. Grammar Implications

### 10.1 Tag Declaration Syntax

The grammar requires a new declaration form for failure tags. The form should accept:

- A tag name (an identifier).
- A position in a hierarchy (root, or child-of-named-parent — surface syntax TBD).
- An optional class binding (some surface form binding the tag to a named class).
- An optional declaration of children (if the surface syntax is nested rather than flat).

The form should be a top-level module declaration, parallel to other type-introducing forms. Proposed name: `.failure` (or `.failures` for a hierarchy block, if nested syntax is chosen). Final name TBD.

### 10.2 Failure-Set Alias Syntax

The grammar requires a declaration form for named failure-set aliases. The form should accept:

- An alias name.
- An expression denoting a failure set (a list of tag names, with the at-or-below closure semantics from §4).

Proposed name: `.failureset` (or similar). Final name TBD.

### 10.3 Signature-Surface Productions

The grammar's command-signature productions require an optional failure-set position, applicable when the failure mark is `?` or `!`. The exact placement (immediately after the mark, in a separate clause, in the return-type position, or elsewhere) is a surface-syntax choice and is open.

### 10.4 `.fail` Argument Form

CP011 §5.9 specified `.fail` as a `!`-call with an explicit failure value. Under typed failures, the form needs to accept a tag and an optional payload value:

- `.fail Tag` for payload-less tags.
- `.fail Tag: ‹value›` (or similar) for payload-bearing tags.

The colon is illustrative; the actual separator is a grammar choice.

### 10.5 Recovery-Spec Argument Form

The recovery-spec form (CP002 §2.10's `|`-with-spec) currently filters and binds in some grammar-defined way. Under typed failures, the spec must accept:

- A tag name (filter: engage iff propagating tag is at-or-below this tag).
- An optional binding identifier (bind: introduce identifier into body scope at the bound class).

The current grammar may not match; the user has noted that the recovery-spec syntax is "almost guaranteed to require alteration." A coordinated grammar update for `.fail` and the recovery-spec form is implied.

### 10.6 Higher-Order Propagation Form

Deferred to OQ-26.4. No grammar change in this checkpoint.

---

## 11. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | **Constrained by this checkpoint** | Union discriminator representation. Failure tags now serve as discriminators for failure values; the choice of tag representation (small-integer encoding) is one instance of the broader OQ-1 question. The general-union-type discriminator question remains open, but the failure-value subcase is settled by the slot's tag-identifier component. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Substantially resolved (per checkpoint 008) | |
| OQ-7 | Resolved (per checkpoint 010), but see OQ-26.4 | The fexpr-signature thread is resolved, but the higher-order-failure-propagation form (OQ-26.4 below) is structurally similar and may inform a future refinement. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Resolved (per checkpoint 006) | |
| OQ-10 | Resolved (per checkpoint 009) | |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |
| OQ-16 | Open | Overloading restriction on dynamically-dispatched commands. |
| OQ-17 | Substantially resolved (per checkpoint 009) | |
| OQ-18 | Open | Lambda visible-signature representation. |
| OQ-19 | Resolved (per checkpoint 008) | |
| OQ-20 | Open | Slash-list internal grammar details. |
| OQ-21 | Open | Capture-list interaction with implicit context parameters. |
| OQ-22 | Open | Surface form for parameterized literal types in `.implicit` declarations. |
| OQ-23 | Open | Lexer disambiguation rules for `{-`, `-}`, `[-`, `-]`, `{-}`, `[-]`. |
| OQ-24 | Open | The phasing of `.implicit` Aggregate/Sequence support. |
| OQ-25 | Open | Shadowing of captured names by body-introduced locals in fexpr/lambda bodies. |
| OQ-26 | **Substantially resolved by this checkpoint** | Typed failures: hierarchical tags with class-bound payloads, structural set inclusion, payload-in-originating-frame implementation. Several sub-questions remain (OQ-26.1 through OQ-26.5 below). |
| OQ-27 | **New** | Top-level failure handling at `.program` and `.test` — see §11.2. |

### 11.1 Sub-Questions Under OQ-26

These are open details left for incremental resolution:

- **OQ-26.1**: Treatment of `?`- or `!`-marked signatures without an explicit failure set. Candidates: implicit-empty, implicit-any, inferred-from-body, required-explicit, or a hybrid (explicit at module boundaries, inferred intra-module). The author's tentative recommendation is the hybrid; final resolution is left to the typechecker-implementation thread.

- **OQ-26.2**: Surface syntax for tag declarations, failure-set aliases, signature failure-set positions, `.fail` with payload, and the recovery-spec form. These are several related grammar questions that should be settled together since they share design constraints. The current grammar's recovery-spec form is acknowledged by the user to require alteration.

- **OQ-26.3**: Recovery-narrowing in the lattice. Whether `|: Tag` narrows the propagating set precisely (set minus at-or-below closure) or conservatively (no narrowing). The simple precise form is implementable; the conservative form is initially acceptable. Choice can be deferred to typechecker implementation.

- **OQ-26.4**: Higher-order failure propagation form. A surface for "my failure set is the union of my callees', plus what I add directly," paralleling the fexpr-signature thread (refined OQ-7). Should be designed jointly with any future refinement of OQ-7.

- **OQ-26.5**: Re-fail and the timing of the original frame's reclamation. Whether bound names from a recovered failure are copied into the handler frame at binding (interpretation I-a) or remain readable in the original frame's storage until handler-body-exit (interpretation I-b). Both yield the same user-facing semantics; the choice is implementation detail.

### 11.2 OQ-27: Top-Level Failure Handling at `.program` and `.test`

A failure that escapes the entire body of `.program` or `.test` cannot propagate further. Some terminal handling is required. Candidate rules:

- **Strict termination**: The program (or test) terminates immediately on the unhandled failure. The originating frame retires as part of termination teardown; at-stack handlers fire. Diagnostic output is whatever the payload class's standard rendering produces. For `.test`, this counts as a test failure.

- **Implicit top-level recovery**: The runtime provides an implicit `|` at the top of every `.program`/`.test`, which catches anything unhandled, runs a standard rendering operation on the payload, and terminates. Structurally equivalent to strict termination, but framed as a recovery rather than a special case.

- **Expected-failure declarations for `.test`**: A `.test` block intended to verify a specific failure may declare that failure as expected; reaching the top with that failure is a *pass*, with anything else (or with no failure) a fail. Test-framework territory; should be designed alongside the broader `.test` mechanism.

- **`.program`/`.test` failure-set declarations**: Allow `.program`/`.test` to declare the failure set they are willing to terminate on, as a partial form of static documentation. Probably overkill for `.program`; potentially useful for `.test`.

The choice depends in part on what operations the standard failure-payload class hierarchy mandates (specifically, whether all payloads can be rendered for diagnostic output, which is the main thing top-level termination needs to do). Resolution of OQ-27 should be coordinated with whatever standard-library design pins down the failure-payload class hierarchy.

OQ-27 is independent of OQ-26 in design, but downstream of it in dependency: OQ-27 cannot be fully resolved until OQ-26's class-binding mechanism is in use.

---

## 12. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| Failure types | Untyped (only failure-mode marks) | Hierarchical tags with optional class-bound payloads (§§2–3) |
| Set inclusion | N/A | Structural over the hierarchy (§4) |
| Signature failure-set surface | N/A | Inline lists and named aliases (§5); higher-order form deferred |
| Failure-payload representation | Operational (Φ value, untyped) | Tag + payload pointer + class witness; payload in originating frame (§6) |
| Frame retirement on failure path | Implicit | Deferred until consumption-or-termination (§6.4) |
| At-stack handler firing on failure path | Unspecified | Once, at originating frame's retirement (§6.6) |
| Re-fail | Implicit pattern (CP002 §2.3) | Ordinary `.fail` from handler frame (§6.7) |
| `|`-with-spec exhaustiveness | Not statically verifiable | Becomes verifiable under refined lattice (§9) |
| Top-level failure handling | Unspecified | Open as OQ-27, scope clarified (§7, §11.2) |
| Class dispatch and failures | Open | Failure set is class-method-level property (§8) |
| CP011 lattice | Type-agnostic | Refinement direction sketched; full spec deferred (§9) |

---

## 13. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-29.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-011.md.

**Grammar changes implied:** Several new productions are needed (tag declaration form, failure-set alias declaration, signature failure-set position, refined `.fail` argument form, refined recovery-spec form). Surface syntax for these is open (OQ-26.2). The current recovery-spec form is acknowledged by the user as requiring alteration. None of the implied grammar changes are detailed here; they are design work jointly tracked with OQ-26.2.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-012.md`). The natural next thread is one of:

1. **OQ-26.2 (surface syntax)**, which unblocks grammar work for `.fail`, recovery-spec, tag declarations, and signature failure sets. This is a small-but-load-bearing thread.

2. **OQ-27 (top-level failure handling)**, which is independent of OQ-26.2 and can proceed in parallel.

3. **The CP011 lattice refinement** to incorporate failure-set tracking, which is the typechecker-implementation thread that this checkpoint and CP011 jointly enable.

4. **One of the smaller open questions** (OQ-13 implicit context parameters, OQ-14 same-scope `&x`/`x`, OQ-15 downcast intrinsic, OQ-16 overloading restriction, OQ-22 parameterized literal types, OQ-25 capture-shadowing) — these can all be settled inline with implementation work or in small dedicated checkpoints, and are not blocked by anything current.

The author's recommendation is **OQ-26.2 next** if grammar work is the natural near-term focus, **OQ-27 next** if standard-library design is the natural near-term focus, and **lattice refinement next** if typechecker implementation is the focus. The choice is downstream of the implementation roadmap rather than design priority.
