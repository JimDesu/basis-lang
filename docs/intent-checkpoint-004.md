# Basis Language — Intent Checkpoint 004

**Status:** Working draft. Builds on intent-checkpoint-001.md, intent-checkpoint-002.md, and intent-checkpoint-003.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27. This checkpoint resolves OQ-4 (default initialization) and reframes part of OQ-3, at the cost of introducing a non-trivial typechecker analysis. The decision is load-bearing for the entire failure-mode-and-typechecker thread that comes next.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

**Basis adopts Position 4 — no default initialization — combined with a two-flavor distinction between writeable parameter modes.**

Concretely:

1. There are no language-level default values. Types do not declare defaults. Bare `# x` introduces an uninitialized slot.
2. Reading from an uninitialized slot is a compile-time error, statically enforced by flow-sensitive initialization analysis in the typechecker.
3. Writeable parameters come in two flavors:
   - **Productive** (mandatory-write): the callee is statically obligated to write the parameter on every successful return path. The caller may pass either an initialized or an uninitialized slot.
   - **Reference** (optional-write, INOUT): the callee may read, write, or do neither. The caller must pass an already-initialized slot.
4. Pointers `^T` have no default value. There is no null pointer in Basis.

This resolves OQ-4 by eliminating the question — the design no longer needs defaults.

---

## 2. Rationale

### 2.1 Why Position 4

The four positions for default initialization considered during the dialog were:

- Position 1: universal zero-fill;
- Position 2: mandatory type-declared defaults;
- Position 3: optional defaults with restricted expression-position use;
- Position 4: no defaults; statically-tracked initialization.

Position 4 is the only one that simultaneously:

- Honors domain invariants by construction (a `Positive` value never exists in an invalid state because no `Positive` slot is ever default-zero);
- Eliminates the null-pointer problem at the root (`^T` simply has no representable null value);
- Allows variants and unions to dispense with a "default candidate";
- Allows records-with-non-defaultable-fields to exist normally;
- Strengthens the "mutation either succeeds fully or fails fully" principle (failure not only doesn't leave partial state — it doesn't leave invalid initial state either).

The cost is implementation complexity in the typechecker: definite-assignment analysis on the callee side, initialization-state tracking on the caller side, and the integration of both with the failure-mode analysis. This is significant work but conceptually local and doesn't affect separate compilation.

### 2.2 Why Two Flavors of Writeable Parameter

The naive Position 4 has a structural problem: if every writeable parameter requires the callee to write on success, then INOUT patterns become impossible. A statechart's "leave the receiver unchanged when no transition fires" pattern, an "update if needed, otherwise no-op" pattern, and any other read-modify-or-not idiom all fail the obligation.

The resolution is to split writeable-ness into two distinct semantic modes:

- **Productive** parameters express "I will produce a value into this slot." Their callees commit to writing on success. Their callers may pass uninitialized slots, expecting the callee to initialize them.
- **Reference** parameters express "I may modify this slot, drawing on its current contents." Their callees commit to nothing about writing. Their callers must pass already-initialized slots (because the callee may read, and reading uninitialized is forbidden).

The two modes correspond to two genuinely different uses of writeable storage. Productive is for constructors, value-builders, "compute-and-place" operations. Reference is for in-place updates, state transitions, "modify if needed" operations.

This split gives the language the cake (no defaults; sound domain invariants; no null pointers) and lets the user eat it (genuine INOUT semantics where needed for statecharts and similar patterns).

---

## 3. The Typechecker's Job

This section spells out the analysis the typechecker must perform. It is more sophisticated than the analyses Basis has needed so far. The good news is that it integrates naturally with the failure-mode analysis (which is flow-sensitive for separate reasons), so the two can share machinery.

### 3.1 Initialization State as a Per-Slot, Per-Program-Point Property

For every variable slot (parameter, receiver, locally-introduced via `#`, or field thereof) at every program point inside a command body, the typechecker tracks whether the slot is **initialized**, **uninitialized**, or **uncertain** (initialized on some incoming control-flow paths but not others, where "uncertain" is treated as uninitialized for purposes of read-checking).

A slot's state changes at specific program-point transitions:

- **Introduction.** `# x` introduces `x` as uninitialized. `# x <- (call: ...)` desugars to `# x; call: x, ...` and is analyzed as such — `x` is uninitialized at the point of the call, and the call's success/failure transition determines `x`'s state thereafter.
- **Successful return from a productive call.** A slot passed to a productive `'` parameter becomes initialized after a successful call.
- **Failed call.** A slot passed to any kind of call retains its pre-call initialization state on failure (because copy-restore does not write back on failure).
- **Successful return from a reference call.** A slot passed to a reference `&` parameter retains its initialization state (which must have been "initialized" pre-call).
- **Successful return from an IN parameter call.** The slot's state is unchanged (the call cannot affect the caller's slot at all).
- **Branching.** At control-flow joins, the slot's state is the per-path intersection — initialized only if initialized on every incoming path.

### 3.2 Read-Checking

Every read of a slot is checked against the slot's initialization state at that program point. A read of an uninitialized or uncertain slot is a compile-time error.

A "read" includes:

- Using the slot's value as an argument to a call (whether IN, productive, or reference; productive parameters may write but may also read first, so the caller's pre-call state matters even to productive parameters if the callee reads);
- Using the slot's value in any expression-form;
- Indexing into the slot (`x[i]` reads `x`);
- Dereferencing the slot (`p^` reads `p`).

There is one subtlety: a productive `'` parameter does not require the caller's slot to be initialized at the call site, **but** if the callee's body reads the parameter before writing it, the callee's body itself fails its own initialization analysis. The caller can therefore safely pass an uninitialized slot to any productive parameter without worrying about whether the callee will read first — the callee was statically prevented from reading-before-writing.

This is the cleanest version of the contract: productive callers don't need to know the callee's internal access pattern, because the typechecker has already proven it doesn't matter.

### 3.3 Definite-Assignment in Callee Bodies

For each productive `'` parameter of a command, the typechecker must prove that every successful return path through the command's body writes to that parameter. "Writes to" means the parameter appears as the recipient of an assignment (`<-` or by being the destination of a productive sub-call) on every successful path.

This is **standard definite-assignment analysis**, the same kind that Java performs on local variables, that C# performs on `out` parameters, and that Rust performs on `let` bindings. Basis adds one wrinkle: it must distinguish successful from failed return paths. Failed paths are exempt from the obligation (the caller's slot doesn't get written-back on failure, so what the callee did is irrelevant).

The integration with failure semantics is clean. The analysis walks the command body's control-flow graph; at each fail-point (an `!` call, a `?` call whose failure isn't recovered, etc.), the analysis branches the path tracking. Successful-return paths are the paths that exit the body without a propagating failure. The definite-assignment obligation applies only to those.

### 3.4 Reference Parameters Have No Definite-Assignment Obligation

The callee of a reference `&` parameter has no obligation to write the parameter. The typechecker does not attempt to prove anything about whether the callee writes; it accepts any pattern of reads and writes on the parameter, including no writes at all.

The cost of this freedom is paid by the caller: the caller must prove the slot is initialized at the call site, because the callee may read.

### 3.5 The Caller-Side Analysis at Each Call Site

At every call site, for each writeable argument:

- **Productive `'` call site:** The slot's pre-call initialization state is unconstrained. On the success path, the slot is initialized. On the failure path, the slot's state is unchanged.
- **Reference `&` call site:** The slot must be initialized pre-call (compile error otherwise). On the success path, the slot remains initialized. On the failure path, the slot remains initialized.
- **IN call site:** The slot must be initialized pre-call. The slot's state is unchanged regardless of outcome.

This is straightforward forward-flow analysis, parameterized by the call's success/failure outcome at the program point following the call.

### 3.6 Loops and Fixed Points

Inside a `^` rewind loop, the analysis must reach a fixed point. The analysis pass walks the loop body; if a slot is initialized at the loop entry, its state at the rewind point must be at least "initialized" for the analysis to converge cleanly. The standard work-list algorithm for forward-flow analyses handles this.

The interesting case is a slot introduced inside a loop body. Such a slot is uninitialized on every loop iteration's entry (the introduction is re-encountered each pass), and any read along that iteration's body is checked against that fresh-uninitialized state. This works correctly without special handling.

### 3.7 Recovery Contexts and Initialization

When a `|` or `|`-with-spec block engages, it does so on the failure path of its preceding siblings. The recovery body's initialization analysis takes its starting state from the failure-path states of the slots, which is the pre-call state of any productive write attempts that didn't complete.

This integrates the failure-recovery story with initialization. The recovery body sees the slots as they were before the failed work, not as they might have been if the failed work had succeeded. This is consistent with the operational semantics — failure means no write-back, so the recovery body genuinely is operating on pre-call state.

### 3.8 What the Typechecker Does Not Need

The typechecker does **not** need to:

- Track partial initialization of compound types in the basic case (record/object field-by-field tracking is an open question — see §8.2 below).
- Analyze object internals during construction (constructors are productive in the receiver and follow the standard rule).
- Reason about default values (there are none).
- Handle nullability (no nulls exist).
- Decide what "the default value" of a type is in any context (the question never arises).

These simplifications are real. The flow-sensitive work is bounded: it's per-command-body, and the per-slot state is a small, finite lattice (uninitialized / initialized / uncertain).

---

## 4. Grammar Implications

### 4.1 Verified Fact: `&` Is Available as a Prefix Marker

Confirmed by direct inspection of `Grammar2.cpp` (master branch):

The `AMPERSAND` token appears in exactly one substantive grammar production:

```cpp
CALL_EXPR_ADDR = as(Production::CALL_EXPR_ADDR, AMPERSAND);
```

This is used as a postfix operator within `CALL_EXPR_SUFFIX`, producing an address-of-expression result (e.g., `someValue&`). The prefix form `&name` is **not currently used** anywhere in the grammar. It is therefore available for repurposing as a parameter-mode marker without conflict.

### 4.2 The Apostrophe `'` Today

Also confirmed by inspection of `Grammar2.cpp`:

The apostrophe (TYPE_ARG_WRITEABLE) appears **only in `TYPE_CMDEXPR_ARG`** — i.e., on argument types within command-type-expressions (`:<Type', ...>`, `?<Type', ...>`, `!<Type', ...>`). It does **not** currently appear in the `DEF_CMD_PARM` production used for actual command parameter definitions.

This means the writeability marker for command *definitions* is effectively a clean slate at the grammar level. Prior intent treated `'` as the writeable marker uniformly, but the grammar implementation has not yet caught up to that intent. This is fortunate, because the two-flavor distinction this checkpoint introduces means the marker design needs revisiting anyway.

### 4.3 Proposed Marker Assignment

The recommended assignment is:

- **No marker** — IN parameter (call-by-value).
- **`'` (apostrophe, suffix on type)** — Productive writeable parameter (mandatory-write; callee must write on every successful path; caller may pass uninitialized).
- **`&` (ampersand, prefix on parameter name)** — Reference writeable parameter (optional-write; callee may read/write/either; caller must pass initialized).

The rationale for the placement asymmetry:

- `'` on the type position fits the existing `TYPE_CMDEXPR_ARG` convention and reads naturally as "this *type* is writeable in the productive sense — the slot will be filled with a value of this type."
- `&` on the name position fits C++-style reference semantics and reads naturally as "this *name* is bound to an existing slot — I'm taking a reference to your value."

The two markers can coexist on the same parameter declaration without ambiguity (though typically only one is used). If both placements turn out awkward in practice, an alternative is to put both on the type side (`Type'` for productive, `Type&` for reference). Choice of final syntax is open; the semantic distinction is the load-bearing decision.

### 4.4 Grammar Changes Required

The following productions need extension:

- **`DEF_CMD_PARM`** must accept the new marker(s). Currently it is `all(DEF_CMD_PARM_TYPE, DEF_CMD_PARM_NAME)`. The extended form needs to capture both productive and reference modes. Possible shape: `all(DEF_CMD_PARM_TYPE, maybe(as(Production::DEF_CMD_PARM_PRODUCTIVE, APOSTROPHE)), maybe(as(Production::DEF_CMD_PARM_REFERENCE, AMPERSAND)), DEF_CMD_PARM_NAME)` — though the exact placement and ordering of markers should be settled via syntactic experimentation.

- **`TYPE_CMDEXPR_ARG`** must distinguish productive from reference mode in command-type-expressions. The current `TYPE_ARG_WRITEABLE` collapses both into one — it now needs to split into `TYPE_ARG_PRODUCTIVE` and `TYPE_ARG_REFERENCE`, with corresponding marker assignments.

- **`DEF_CMD_RECEIVER`** may need similar treatment, depending on whether receivers can be productive vs. reference. (In the current model, receivers in v-commands are typically reference — the caller has an existing object whose method is being invoked. But constructors take their target as productive, since they're filling an empty slot.)

These are mechanical changes once the marker assignment is settled. The hard work is in the typechecker, not the parser.

---

## 5. Examples

### 5.1 Bare Introduction Followed by Productive Initialization

```
.cmd doWork: Int32 input =
    # result                                   ; result is uninitialized
    add: result, input, 1                      ; productive: writes result on success
    log: result                                ; safe: result is initialized on the success path
```

The typechecker:
1. Marks `result` uninitialized after `# result`.
2. At `add: result, input, 1`, requires `add`'s first parameter to be productive `'`. On success, marks `result` initialized.
3. At `log: result`, requires `result` to be initialized. The success-path analysis says yes. ✓

If `add` were not productive (or if `add`'s body didn't satisfy its definite-assignment obligation), this program would be rejected.

### 5.2 Sugar: `result <- add: ...` Equivalent to `add: result, ...`

```
.cmd doWork: Int32 input =
    # result <- (add: input, 1)                ; sugar for: # result; add: result, input, 1
    log: result
```

This is the expression-position sugar form. It desugars to the prior example exactly. Both are well-typed under the same analysis.

### 5.3 Failure Path Leaves Slot Uninitialized

```
.cmd doWork: Int32 input =
    # result <- (?divide: input, 2)            ; ?divide is may-fail
    log: result                                ; ERROR — result might be uninitialized here
```

The typechecker:
1. After `# result`, `result` is uninitialized.
2. After the `?divide` call, on the success path `result` is initialized; on the failure path `result` is still uninitialized.
3. Without a recovery block, both paths flow into `log: result`. The intersection-of-paths state is "uninitialized" (by being uncertain). ✗

The fix is to handle the failure:

```
.cmd doWork: Int32 input =
    # result <- (?divide: input, 2)
    | result <- (?divide: input, 1)            ; recover by reinitializing
    log: result                                ; safe now
```

After the `|` block, both paths converge with `result` initialized.

### 5.4 Reference Parameter for a Statechart Method

```
.cmd handleEvent: &State current, Event e =
    ?: equals: e, e.Quit
        Terminated: current                    ; reassign current to a Terminated state
    ?: equals: e, e.Tick
        ; do nothing — leave current unchanged
    ; default: do nothing
```

The receiver-pointer parameter `&State current` is reference, not productive. The typechecker:

- Requires `current` to be initialized at the call site (because the method may read it via comparison or otherwise).
- Imposes no definite-assignment obligation on the method body. The "do nothing" branches are well-typed.
- Permits writes to `current` where they occur.

If the receiver were declared `State' current` (productive), the typechecker would reject the method body because there are paths that don't write `current`.

### 5.5 Constructor Pattern

```
.cmd buildPoint: Float32' result, Float32 x, Float32 y =
    result.x <- x                              ; assumes field-level tracking; see §8.2
    result.y <- y
```

The receiver `result` is productive `'`. The typechecker requires every successful return path to fully initialize `result`. If the language commits to field-level tracking (open question), this analysis verifies that both `result.x` and `result.y` are written. Without field-level tracking, the language requires a single whole-slot write, achieved by some other mechanism (e.g., a record-literal-initializer; see §8.4).

### 5.6 Pointer Has No Default

```
.cmd loseTrack =
    # p : ^Int32                               ; p is uninitialized; there's no null
    log: p^                                    ; ERROR — reading uninitialized p
```

```
.cmd keepTrack =
    # x <- (?something)
    | x <- (constant: 0)
    # p : ^Int32 <- (addressOf: x)             ; p is initialized
    log: p^                                    ; safe
```

There's no null pointer. There's no way to *get* a null pointer. The slot is either initialized to a valid pointer or it's not initialized at all, and the latter is statically forbidden from being read.

### 5.7 Branching Initialization

```
.cmd routeByCondition: Int32 input =
    # decision
    ? lessThan: input, 10
        decision <- (constant: 0)
    - decision <- (constant: 1)
    log: decision                              ; safe
```

The typechecker:
- After `# decision`, uninitialized.
- After the `?` block's body, on the engaged path, `decision` is initialized.
- After the `-` block's body, on the engaged path, `decision` is initialized.
- The two branches are mutually exclusive (one always engages). At the join, `decision` is initialized on every path. ✓

If the `-` branch were missing, the analysis would reject `log: decision` because the path where the `?` guard fails leaves `decision` uninitialized.

---

## 6. Interaction with Existing Material

### 6.1 OQ-3 (`-> name` on Non-Writeable Parameters) Reframed

The new model gives this question a sharper answer:

- **`-> name` where `name` is productive `'`:** standard expression-position usage. On success, the caller's slot is initialized with the produced value. On failure, the slot retains its pre-call state.

- **`-> name` where `name` is reference `&`:** the caller's slot must be pre-initialized; the call may modify it in place. The expression-position result is the slot's value at successful return time. On failure, the slot retains its pre-call value (which was already initialized).

- **`-> name` where `name` is IN (no marker):** the caller's slot is unchanged by the call regardless of outcome. The expression-position result is the slot's pre-call value, echoed back. This is potentially useful for combinator-shaped commands but produces no new information; reading `name` directly would suffice. **Recommended treatment: this case is permitted but discouraged, with the result being the slot's pre-call value.** It can serve as an idiom for "use `cmd` as an expression that produces its own input."

OQ-3 is therefore largely resolved, though the IN-parameter case remains weakly motivated and might be restricted in a later revision.

### 6.2 OQ-4 (Default Initialization) Resolved

This checkpoint resolves OQ-4 by eliminating the question. There are no defaults; types do not declare defaults; the typechecker tracks initialization state. All the sub-questions of OQ-4 (sub-(a) through sub-(d)) cease to apply.

The concern in OQ-4 sub-(c) — "domains with invariants like `Positive: [4]` — is fully addressed: a `Positive` slot is uninitialized until written by a successful productive call, and any successful productive call to a command producing `Positive` must produce a value that satisfies the `Positive` invariant. Domain authors may rely on the invariant unconditionally; no invalid `Positive` value can exist.

### 6.3 Lifecycle and Object Construction (Refining Checkpoint 002 §3)

Object lifecycle from checkpoint 002 integrates cleanly with the new model. Checkpoint 002 §3.3 described the writeable `^Object` parameter as having three cases (success-with-new-object, failure-before-write, failure-after-write). Under the new model, that parameter is a productive `'` parameter — the constructor or factory commits to producing a valid object on every successful path, and failure paths leave the caller's slot in its pre-call state (uninitialized or initialized-to-prior-object, as the case may be).

Stack-exit hooks and `@`/`@!` blocks are unaffected by this checkpoint. The frame model from checkpoint 002 plus the Q-D/Q-E refinements (commands and `@`/`@!` introduce frames; flow-control constructs do not) continue to apply.

### 6.4 The `_` Underscore Empty Parameter

The grammar has `CALL_PARM_EMPTY = as(Production::CALL_PARM_EMPTY, UNDERSCORE)`, allowing `_` as a placeholder for an unused parameter at a call site. Under the new model, `_` should be permitted only for parameters whose value is not actually consumed — i.e., productive parameters where the caller doesn't care about the produced value, or possibly IN parameters with declared defaults at the *call-site signature* level (not the type level). This is a small extension to think about as the grammar is updated; for now, treat `_` as a placeholder that the typechecker will validate per-parameter according to mode.

---

## 7. Failure-Mode Integration: A Preview

This checkpoint commits the language to a more sophisticated typechecker than was previously envisioned. It is worth being explicit about how the new analysis composes with the failure-mode story (the natural next major thread).

The two analyses share a control-flow-graph walk. At each program point, the typechecker tracks:

- The **failure mode** of each command-typed value being invoked (`:` never-fails, `?` may-fail, `!` must-fail), and the path-discriminated outcomes (success path vs. failure path), with recovery contexts as path-merge points.
- The **initialization state** of each slot, transitioning per the rules in §3.

Both analyses are forward-flow, both branch at fail-points, and both rejoin at recovery points. They naturally share machinery. The implementation strategy is to walk the CFG once, maintaining a per-slot state vector per program point that captures both the failure-mode information and the initialization information.

This sharing is important. Without it, the typechecker would be doing two independent passes, each with its own CFG-walking machinery. With it, the analysis is roughly one pass, and the two questions ("is this read safe?" and "does this path's failure mode satisfy the never-fails-must-handle rule?") are answered together.

The rules for how recovery contexts modify both the failure-mode state and the initialization state are sketched here (§3.7) and need full development in the typechecker thread.

---

## 8. Open Questions and Drill-Downs

### OQ-9: Field-Level vs. Whole-Slot Initialization Tracking

The basic analysis described above tracks initialization at the slot level: `# r : Point` gives a single uninitialized `r`; one write to `r` initializes the whole thing.

Field-level tracking would allow piecemeal initialization: `# r : Point; r.x <- 1.0; r.y <- 2.0; log: r` where each field-write transitions just that field's state. This is useful for record/object construction patterns but adds complexity:

- The state per slot becomes a tree (or a set of paths) rather than a single bit.
- Reads from `r` directly require all fields to be initialized; reads from `r.x` require just `x` to be initialized.
- Aliasing through pointers and references becomes a concern (does `^r.x` refer to a field state or the whole record?).

Open sub-questions:

- (a) Is field-level tracking needed at all, or can record/object initialization be expressed via single-shot constructors that take all field values at once?
- (b) If field-level tracking is supported, does it extend to nested aggregates (`r.inner.x <- ...`)?
- (c) Does field-level tracking interact with reference parameters (does passing `&r.x` to a callee work)?

The answer affects how object/record constructors are written. Without field-level tracking, constructors must initialize their target in a single conceptual step (e.g., via a record-literal-style initialization). With field-level tracking, constructors can write fields in any order, with the typechecker proving full coverage.

### OQ-10: Composite Initializers (`=` vs. `<-`)

The dialog raised the possibility of distinguishing `=` from `<-` to support compound-type initialization. One natural reading:

- `<-` is the existing call-result form: `slot <- (call-or-expression)` performs a productive call (or sequence thereof) writing into `slot`.
- `=` is structural initialization: `slot = { field1: val1, field2: val2 }` provides a record/object literal that initializes all fields atomically.

This would satisfy OQ-9(a) without requiring field-level tracking — the structural initializer is a single conceptual write that initializes the whole slot, and the typechecker treats it as one operation.

The exact syntax and semantics of `=` are not committed by this checkpoint. The motivation is real but the design has not been worked out. This is now OQ-10.

### OQ-11: Marker Syntax — Final Choice

This checkpoint recommends `'` for productive (suffix on type) and `&` for reference (prefix on name), as the two-character split that fits existing usage and unused syntax space. Alternatives include:

- Both markers on the type side: `Type'` productive, `Type&` reference.
- Both on the name side: `'name` productive, `&name` reference.
- Different symbols entirely.

The decision should be settled before grammar changes are implemented. The choice does not affect the semantic model, only the surface syntax.

### OQ-12: Receivers and Mode

V-command and constructor receivers need a mode. Constructors are clearly productive (they fill the receiver). V-command method receivers are typically reference (the method may modify the receiver but isn't obligated to). At-stack-exit methods (`DEF_CMD_RECEIVER_ATSTACK`, `DEF_CMD_RECEIVER_ATSTACK_FAIL`) operate on receivers whose mode is unclear under the new model — probably reference, since the cleanup may inspect or modify but isn't constructing.

The default mode for receivers in different command-signature shapes needs to be specified explicitly. Possibilities:

- Make the mode explicit on every receiver, with no default.
- Default by signature shape (constructors → productive; v-commands → reference; at-stack → reference).
- Default to one mode, with explicit override.

Recommendation pending; this should be addressed alongside OQ-11.

### OQ-13: Implicit Context Parameters and Initialization

Implicit context parameters (after `/` in the signature) are resolved at call sites by uniqueness-of-type from the caller's lexical scope. The new model raises a question: must an implicit-resolved value be initialized at the resolution site?

The natural answer is yes — the implicit mechanism is plumbing, not magic; if the caller's scope has an in-scope value of the right type, that value is used, and the typechecker requires it to be initialized at the call site like any other read. But this should be confirmed and any edge cases (uninitialized values that nonetheless match by type) should be specified.

---

## 9. Summary of Status Changes to Earlier OQs

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Unchanged. Union discriminator representation. |
| OQ-2 | Open | Unchanged. Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved | Productive and reference cases now have clean semantics. IN-parameter-as-result-designator case remains weakly motivated but permitted. |
| OQ-4 | **Resolved** | No defaults; flow-sensitive initialization analysis instead. |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Open | Partial application beyond receiver-only. |
| OQ-7 | Open | Block-quote macro/fexpr semantics. |
| OQ-8 | Mostly resolved | At-stack registration mechanics. One sub-question remains. |
| OQ-9 | **New** | Field-level vs. whole-slot initialization tracking. |
| OQ-10 | **New** | Composite initializers (`=` vs. `<-`). |
| OQ-11 | **New** | Final marker syntax for productive vs. reference. |
| OQ-12 | **New** | Default mode for receivers in different signature shapes. |
| OQ-13 | **New** | Implicit context parameters and initialization. |

---

## 10. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27.

**Source materials read for this checkpoint:** intent-checkpoint-001.md, intent-checkpoint-002.md, intent-checkpoint-003.md, and direct inspection of `Grammar2.cpp` (master branch, 499 lines, retrieved 2026-04-27) to verify the availability of `&` as a prefix marker and the current placement of the apostrophe `'`.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-004.md`). The natural next intent thread is the full failure-mode integration with the typechecker, building on §3 and §7 of this checkpoint and the operational rules from checkpoint 002 §2. After that, the typechecker design is sufficiently well-specified to begin spec-driven implementation.
