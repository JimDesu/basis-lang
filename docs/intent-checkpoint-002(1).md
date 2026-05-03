# Basis Language — Intent Checkpoint 002

**Status:** Working draft. Builds on intent-checkpoint-001.md; supersedes it where they conflict, and supersedes the repository README throughout.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) begun on 2026-04-26 and continued in the same session. This checkpoint captures the major thread on **control flow and the failure model**, plus several substantial refinements to material from checkpoint 001 (notably object lifecycle).

**Authority statement.** Where this document and intent-checkpoint-001.md differ, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Refined Foundational Principles

The principles in checkpoint 001 are restated here only where the dialog has refined them. New principles introduced in this thread are stated in full.

### 1.1 Icon's Failure Model Without Icon's Generator Model

Basis adopts the **failure semantics** of Icon — every command either succeeds or fails, with no truth-valued type — but **not** Icon's generator semantics. A command call is single-shot: it produces exactly one outcome, success or failure, with no notion of "the next value" to demand. There is no backtracking, no resumption, no multi-valued evaluation.

The Icon influence shows up at the *combinator* level: the `?`-family control-flow markers compose single-shot outcomes in expressive ways. The Icon influence does *not* show up at the *call* level: each command is a discrete, single-outcome operation. Constructs may internally re-invoke the same command-typed value (e.g., a `^`-rewind block re-invokes its preceding sibling on each loop iteration), but this is structural recurrence, not generator resumption.

### 1.2 No Boolean Type

There is no Boolean type in Basis. Comparisons, predicates, conditional tests, and similar operations are *commands that may fail*: they succeed (with whatever effect their semantics specifies) when the test "holds," and fail when the test "does not hold." `equal: x, y` is a `?`-marked command that succeeds when x equals y and fails otherwise. The `?`-family markers consume these failures as their control-flow signal.

This means **every conditional construct in Basis is a recovery context for some failable test**. Without Booleans, control-flow branching has nothing to dispatch on except success/failure status. The language is consistent throughout: there is no "boolean expression" hiding underneath the syntax — only commands and their failure outcomes.

### 1.3 Expressions as Pure Syntactic Sugar

There are no true expressions in Basis. Constructs that *look* like expressions in surface syntax (e.g., the right-hand side of `# r <- (...)`) are syntactic sugaring over command sequences. The semantic model is: every command succeeds or fails; results are the *effects* of command execution (writeable parameter mutations); there is no evaluation-to-a-value step in the operational model.

This is a meaningful divergence from Icon, which has expressions that yield values (and which Icon then treats as commands when their value is unused). Basis collapses the distinction entirely: there are commands, and there are effects.

### 1.4 No Invisible Control Flow — With One Specific Exception

The principle of no hidden control flow remains in force. The single, intentional exception is the at-stack-exit mechanism (`DEF_CMD_RECEIVER_ATSTACK` and `DEF_CMD_RECEIVER_ATSTACK_FAIL`), which fires implicitly at object lifetime ceilings. This exception is documented in Section 4.2 below. It exists because the alternative — requiring users to manually register `@`/`@!` blocks for every object with cleanup needs — would be sufficiently awkward as to undermine the language's usability for resource-managing code. The trade is: one specific category of implicit invocation, in exchange for ergonomic resource management.

---

## 2. Control Flow: The `?`-Family Markers

This section covers the eleven block-marker productions in the grammar (`DO_WHEN`, `DO_WHEN_MULTI`, `DO_WHEN_FAIL`, `DO_WHEN_SELECT`, `DO_ELSE`, `DO_BLOCK`, `DO_REWIND`, `DO_RECOVER`, `DO_RECOVER_SPEC`, `DO_ON_EXIT`, `DO_ON_EXIT_FAIL`). The lifecycle hooks `@` and `@!` were introduced in checkpoint 001 and are revisited in Section 3 below; this section focuses on the failure-and-control markers.

### 2.1 Categorization

The markers split cleanly into roles based on what they recover and how they affect control flow.

**Guard-only recovery markers** (`?`, `?-`, `?:`): each block has a privileged first statement (the *guard*) whose failure is consumed by the construct itself. Statements after the guard are *not* recovered; their failures propagate normally per the surrounding context's rules.

**Full-body recovery marker** (`^`): the entire body is a recovered region. Any failure in any body statement is consumed by the construct, terminates the body, and signals loop termination.

**External-failure recovery markers** (`|`, `|`-with-spec): these have no guard. They engage when a failure is propagating from preceding siblings at the same indentation level. The `|`'s body is *not* recovered — its statements propagate failures normally.

**Escape-elevator marker** (`??`): does not directly recover anything; modifies the structural destination at which a paired guard's recovery resumes.

**Branch marker** (`-`): the else-companion of `?` or `?-`. Recovers nothing; provides the alternative branch.

**Grouping marker** (`%`): plain grouping with no control-flow effect; visual/structural organization.

**Lifecycle hook markers** (`@`, `@!`): frame-exit hooks; do not consume failures, neither see them nor recover them. Detailed in Section 3.

### 2.2 `?` (DO_WHEN) — Guard Block

A `?`-block has a guard (the first statement) and a body (statements 2..N). Operationally:

1. Run the guard.
2. If the guard succeeds: run statements 2..N as ordinary code.
3. If the guard fails: skip statements 2..N. The guard's failure is consumed.
4. Either way, control proceeds to the statement following the `?`-block.

The guard's `?`-mark (if any) is satisfied by being a guard — the typechecker treats the `?`-block as the recovery context for the guard. Statements 2..N have no special recovery treatment; their `?`-marks must be handled by the surrounding rules (the never-fails-must-handle rule applies if the enclosing command is never-fails).

**Idiomatic shape:** `?` followed by a guard test followed by conditional work. The classic `if guard then body` pattern, with the guard's possible failure being the engagement signal.

### 2.3 `?-` (DO_WHEN_FAIL) — Inverse Guard Block

Mirror of `?`. The first statement is the guard; statements 2..N run iff the guard *fails*. The guard's failure is consumed regardless of which path engages. The body's failure value is *not* available to statements 2..N — `?-` consumes the *fact* of failure, not the *value*. (For value-bearing recovery, use `|`-with-spec.)

**Idiomatic shape:** "try to do this normally; if it doesn't work, do this fallback."

**Re-fail pattern.** A common composition is `?-` followed by `fail somethingDomainSpecific`, which converts a low-level failure to a domain-meaningful failure without leaking implementation details:

```
?- lowLevelOp: ...
    fail: domainSpecificFailure
```

The original failure value is discarded; the new domain failure replaces it. This is the Basis equivalent of exception-translation patterns.

### 2.4 `?:` (DO_WHEN_SELECT) — Case-When Chain

A `?:` block on its own is similar to a `?` block — first statement is guard, body is recovered only at the guard. The defining feature is *chaining*: a sequence of consecutive `?:` siblings at the same indentation level forms a coherent multi-branch construct, analogous to `cond` in Lisp or `switch` in C (without fallthrough).

**Chain semantics:**

1. Run the first `?:` block's guard.
2. If guard succeeds: run that block's statements 2..N. Then **skip the remainder of the chain entirely** (including any default arm). Control proceeds to the statement following the chain.
3. If guard fails: control moves to the next sibling.
   - If that sibling is another `?:`: repeat with its guard.
   - If that sibling is *not* a `?:`: it is the **default arm**. The chain has been "exited from below" by guard exhaustion. The default arm runs as ordinary statements (potentially complex — it can include further control flow). After the default arm, control proceeds to the statement following the chain.

The chain has no syntactic wrapping construct. It is recognized purely by adjacency: consecutive `?:` siblings extend the chain; the first non-`?:` sibling ends it. There is no `default:` keyword; the default arm is just whatever non-`?:` statements come after the final `?:` in the chain (or empty, if the chain is the last thing in its enclosing block).

**No fallthrough.** A successful `?:` body does not flow into the next `?:` block; it skips the rest of the chain.

**No exhaustion-as-failure.** A `?:` chain whose guards all fail does not propagate a failure from the chain itself; it falls through to the default arm (which may be empty). The chain itself is therefore not a failable construct from outside.

**Body-statement failures.** Within an engaged `?:`-body, statements 2..N follow normal failure rules. A failure in those statements is *not* recovered by the chain or by the default arm — it propagates outward, escaping past the entire chain. Recovery contexts at the chain's enclosing scope (or beyond) can catch such failures, but the chain itself does no help. This is consistent with `?`'s rule (only the guard is recovered).

**Stylistic note.** Single `?:` (with default arm) and `? ... -` ... are functionally equivalent for binary branching. The expected idiomatic distinction:
- `? ... -` ... reads as "if/else" — two branches of equal status.
- Single `?:` reads as "short-circuit this operation, otherwise continue with the default."

### 2.5 `??` (DO_WHEN_MULTI) — Escape Elevator

`??` is the language's most unusual control-flow construct. It is a **meta-marker** that modifies the structural destination at which the paired inner construct's guard-failure recovery *resumes execution*.

**Pairing rule.** A `??`-block must contain a `?`-family block (a `?` or a `?-`) as its first executable inner construct. Bare `??` is a semantic error. The inner construct is generally on the same line as `??` for visual clarity, but layout-wise it need only be the first executable inner item.

`??` does *not* combine with `?:` (no meaningful semantics). It does *not* combine with `%` (which has no control flow to elevate). It does pair with both `?` and `?-` (a `?? ?-` chain is well-formed).

**Operational effect.** Without `??`, when an inner `?` (or `?-`) block's guard fails, the failure is consumed by the inner block, and execution resumes at the next sibling *of the inner block* — which is one indentation level inside the outer structure. With `??`, the failure is still consumed (by the `??` mechanism), but execution resumes at the next sibling *of the `??`-block*, which is one indent level higher.

So `??` is a **failure-consuming construct whose distinctive feature is where it places execution after consumption.** The failure does not escape `??`; the failure is fully consumed by `??` itself. The "elevation" is in the resumption position, not in the failure's reach.

**Crucial corollary.** A `|` block at the elevated level — i.e., as the next sibling after a `??`-block — does *not* engage on the inner guard's failure, because that failure has already been consumed by `??`. There is no propagating failure for `|` to catch. A `|` after `??` engages only on failures from *non-guard* operations within the `??`-block: body statements after a successful guard, or other propagated failures from inside the construct.

**Loop construction.** The canonical loop pattern uses `??` paired with `?` and a sibling `^`:

```
?? ? guardCmd: ...
        loopBody
   ^ continueCondition: ...
     adjustIndex: i
```

When the guard fails, `??` consumes the failure and resumes past the `^` — loop terminates. When the guard succeeds, the body runs, and then `^` (whose own body succeeds) loops back to the guard. When the `^`-body fails (e.g., loop counter exceeds bound), `^` consumes that failure and falls through to the next sibling — also past the loop.

The two failure paths (guard failure and `^`-body failure) terminate the loop via different mechanisms but to the same destination, by design.

**No `???`.** `??` operates at a fixed elevation of one level, and there is no further-elevating construct. If multiple levels of elevation are needed, restructure the code.

**No `??-`.** There is no fused syntax for `??` + `?-`; the two are written as separate adjacent markers (`?? ?-`).

### 2.6 `^` (DO_REWIND) — Loop Block

`^` is a sibling block whose body controls whether execution loops back to the preceding sibling. The `^`-body serves as the loop's *continuation test plus per-iteration adjustments*.

**Operational effect.**
- When control reaches `^`, run its body.
- If the body succeeds: jump back to the preceding sibling at the same indentation level. (Loop continues.)
- If the body fails: the failure is consumed by `^`. The body terminates at the failed statement. Control falls through to the statement following the `^`-block. (Loop terminates.)

**Full-body recovery.** Unlike `?` and `?-`, the `^`-body recovers *every* failure within itself, not just an "overall body outcome." Any failing statement consumes its failure into `^` and terminates the body. This makes the body a unified test-and-adjust region: comparisons (`?`-marked tests like `lessThan: i, n`) compose freely with adjustments (potentially failable arithmetic like `add: i, 1`), with the first-failure determining loop termination.

**Why this works ergonomically.** Inside an `^`-body, every `?`-call is statically satisfied — the `^`-block itself is the recovery context. The programmer writes a flat sequence of test-and-adjust operations; the first to fail terminates the loop. No nested recovery contexts are needed.

**Preceding sibling.** "Preceding sibling at the same indentation level" is the rewind target. In the canonical loop pattern (Section 2.5), `^` rewinds to the `?`-block. Without an explicit `?` or other guard, `^` would rewind to whatever statement preceded it — though such structures are unusual.

**Composition with `??`.** The standard pattern (Section 2.5) puts `??` outside the rewind structure to elevate guard-failure exit past `^`. `^` itself does not need elevation, because `^`'s own failure-consumption naturally produces fall-through at the right level.

### 2.7 `-` (DO_ELSE) — Else Branch

`-` attaches *only* to a preceding `?` or `?-` block at the same indentation level. It provides the alternative branch when the predecessor's body did not engage.

**Pairings:**
- `?` ... `-` ... — body engages on guard success; `-` body engages on guard failure. Standard if/else.
- `?-` ... `-` ... — body engages on guard failure; `-` body engages on guard success. Inverted if/else, success case in the `-` branch.

`-` does not attach to `?:`, `^`, `|`, `%`, `@`, `@!`, or `??`. There is no chained `-` (no `?` ... `-` ... `-` ... ); for n-ary branching, use `?:`. The `-` body is *not* a recovery context — failures within it propagate normally per the enclosing context.

**Composition with `??`.** A `?? ? ... -` ... or `?? ?- ... -` ... structure elevates the *guard's* failure-escape one level. The `-` branch is the success branch of the guard's outcome (or failure branch in `?-`'s case); since the guard's outcome that engages `-` is the *non-failure* path, `??` does not change `-`'s position. Only the failure path is elevated.

### 2.8 `%` (DO_BLOCK) — Plain Grouping

`%` is a plain grouping block. It has no control-flow semantics, no recovery semantics, no failure-handling effect. Its body executes as ordinary sequential statements.

**Uses:** visual organization; explicit scope demarcation; structural clarity in deeply-nested code. It is *not* combinable with `??` (which has no failure-escape to elevate).

### 2.9 `|` (DO_RECOVER) — Recovery Block, Bare Form

A `|` block is a sibling at some indentation level that engages when a failure is propagating from preceding siblings at the same level. Operationally:

1. When control reaches `|`, check whether a failure is currently propagating.
2. If no failure: skip the `|`-block entirely. Control proceeds to the next sibling.
3. If failure is propagating: the `|`-block engages. The failure is consumed immediately upon engagement.
4. The body runs as ordinary code, with no active failure.
5. The body itself is *not* a recovery context — its statements propagate failures normally per the enclosing rules.
6. After the body completes (whether normally or via an internal propagating failure), control proceeds to the next sibling of the `|`-block.

**Scope of preceding siblings.** `|` recovers failures from *all* preceding siblings at the same indentation level within the same enclosing scope, not just the immediately preceding one. In:

```
:cmd ...
   stmtA: ...
   stmtB: ...
   stmtC: ...
   | recoveryBody: ...
   stmtD: ...
```

The `|` engages on a propagating failure from stmtA, stmtB, *or* stmtC. If any propagates, the rest of the preceding siblings are skipped (failure-skip is active until recovery), `|` engages, recovery body runs. `stmtD` runs after the `|`-block completes (or runs after the preceding siblings if none failed).

**No failure: `|` has no effect.** A `|` block in source where no failure ever reaches it is a no-op at runtime, analogous to a `catch` block where no exception was thrown.

**Bare `|` recovers any failure value.** The propagating failure's value is not made available to the body. (To bind the failure value, use `|`-with-spec.)

### 2.10 `|`-with-spec (DO_RECOVER_SPEC) — Filtered/Binding Recovery

The `|`-with-spec form behaves identically to bare `|` with two additions:

**Filtering.** The spec acts as a predicate on the propagating failure's value. The block engages only if the failure matches the spec. Non-matching failures continue propagating *as if the `|`-with-spec block were not there*. This makes possible cascade-style handling chains:

```
stmtA: ...
| spec1: handler1
| spec2: handler2
| catchAllBody          ; bare | as final fallback
```

Each `|`-with-spec filters; the bare `|` at the end (or another `|`-with-spec with a matches-everything spec) catches whatever remains. This is structurally analogous to multiple `catch` clauses for one `try`.

**Binding.** When the spec matches, the spec's identifier is bound to the failure value within the body's scope. The body can then inspect or act on the failure value.

**Cascade among `|`-blocks.** Each `|` is also a potential recovery target for *preceding `|`-blocks' bodies*. If `|` block N engages and its body itself fails, the failure propagates and `|` block N+1 (if present) can catch it. This generalizes the cascade: each block is both a recovery destination and a potential recovery source.

**Multi-`|` filter ordering.** When a failure propagates through several `|`-with-spec blocks, the first whose spec matches engages. Programmers use this to put more-specific specs first and broader specs (or bare `|`) last.

### 2.11 Composition Summary

For reference, the full composition rules established:

| Marker | Combines with `??`? | Combines with `-`? | Body recovers failures? |
| --- | --- | --- | --- |
| `?` | yes (→ elevates guard escape) | yes (→ if/else) | guard only |
| `?-` | yes (→ elevates guard escape) | yes (→ inverted if/else) | guard only |
| `?:` | no | no (chain with more `?:` instead) | guard only |
| `-` | (paired only) | n/a | no |
| `%` | no | no | no |
| `^` | n/a (`^` is sibling, not inner) | no | yes (entire body) |
| `\|` | (`?? \|` at same level is meaningful — `\|` catches non-guard failures from `??`-block) | no | no |
| `\|`-spec | (same as `\|`) | no | no |
| `@` | no | no | no (runs alongside, doesn't recover) |
| `@!` | no | no | no (runs alongside, doesn't recover) |

---

## 3. Lifecycle (Refined)

This section refines the lifecycle treatment from checkpoint 001 in light of the fuller dialog about object lifetimes, copy-restore mechanics, and at-stack-exit methods.

### 3.1 Object Lifetime Ceiling

**The frame in which an object's storage is introduced is its lifetime ceiling.** No mechanism in the language allows an object to outlive the frame that introduced it, except via transitive containment in another object whose ceiling is already higher.

This rule has structural consequences:

- A command cannot return an object to its caller. There is no return-value mechanism for objects (or for anything else; output is via writeable parameters only).
- A command can populate (initialize) an object whose storage was introduced by the caller via a writeable `^Object` parameter. The constructor doesn't *create-and-return* an object; it *initializes* an object slot the caller already owns.
- Object containment via fields propagates the ceiling upward: a field-of-object's lifecycle is at least as long as its container's lifecycle, which is bounded by its container's introducing frame. By transitive closure, every reachable object has a ceiling that is at most the introducing frame of the outermost containing object.

**Consequence: objects cannot escape upward.** A command may pass an object reference downward (to commands it calls) but cannot pass an object reference upward (since there are no return values). The object's reachability is fully contained within the call stack at-or-below the introducing frame. This is not a discipline; it is a structural guarantee enforced by the language.

### 3.2 The At-Stack-Exit Mechanism

At-stack-exit methods are the **only intentional exception to the no-invisible-control-flow principle.** They exist for ergonomic resource management.

**Mechanism.** When an object type defines a `DEF_CMD_RECEIVER_ATSTACK` method, that method is automatically invoked when the object's introducing frame exits — *as if* an `@ method: theObject` block had been written manually. Symmetrically, `DEF_CMD_RECEIVER_ATSTACK_FAIL` defines a method automatically invoked when the introducing frame exits via failure propagation, paralleling `@!`.

**Manual vs automatic invocation.** At-stack methods may also be invoked manually by user code. The two modes have different failure-handling semantics:

- **Manual invocation:** subject to ordinary failure-handling rules. The method's `?` / `:` / `!` mark applies; failures must be handled per the never-fails-must-handle rule when applicable.
- **Automatic invocation:** failure status of the method is silently ignored. An automatically-invoked at-stack method can neither propagate a new failure nor recover an existing one. Failures inside automatic invocation are absorbed by the frame-exit mechanism. This is consistent with the semantics of `@!` blocks (which do not see or consume the propagating failure).

**Override via class instances.** At-stack methods are virtual in the same sense as ordinary class methods. An instance declaration may override an at-stack method, and overriding is sometimes necessary to ensure proper cleanup for the implementing type. Dispatch of the implicit at-stack invocation goes through the type's class dictionary in the usual way.

### 3.3 Writeable `^Object` Parameter Mechanics

Writeable pointer-to-object parameters use call-by-copy-restore on the *pointer*, not on the object's storage. The object's storage is never copied or moved; the pointer's value is what gets cloned and potentially written back.

**The clone protocol:**
- At call entry: the caller's pointer slot is cloned. The callee operates on its own clone. The caller's slot is unchanged during the call.
- The callee may, during execution, allocate a new object and update its clone of the pointer to point at the new object.
- The callee may also leave its clone unchanged.
- On *successful* return: the callee's clone is written back to the caller's slot. The caller's slot now holds whatever pointer the callee ended with.
- On *failed* return: no write-back occurs. The caller's slot retains its pre-call value.

**The three lifecycle cases for a writeable `^Object` parameter:**

**Case 1 — Success with new object.** The callee allocates and constructs a new object B and updates its clone to point at B. On successful return, copy-restore writes the pointer-to-B into the caller's slot. The original object A (if any) is no longer pointed-to from this slot but remains registered for cleanup against its introducing frame. The new object B's at-stack registration *migrates* from the callee's frame to the caller's frame as part of copy-restore commit. Both A and B will be cleaned up when their respective frames exit (which may be the same caller frame, in which case both fire on that one frame's exit).

**Case 2 — Failure before any write.** The callee fails before modifying its clone. The clone still points at A. No copy-restore occurs. The caller's slot remains pointing at A — unchanged. No new object exists.

**Case 3 — Failure after writing.** The callee allocated B and updated its clone to point at B, then failed. The clone now points at B; B exists and was constructed; B's at-stack registration was made when B was created. No copy-restore occurs, so the caller's slot remains pointing at A. From the user's perspective, the failure was atomic — the caller's state is identical to its pre-call state. **B exists in language-internal state**, registered against the callee's failed frame. B's at-stack-fail method (if any) will fire from the callee's frame failure-exit; otherwise B's storage is reclaimed by the frame-exit mechanism.

### 3.4 Registration Migration on Successful Copy-Restore

The at-stack registration of an object is bound to the frame currently responsible for the object's lifetime. On successful copy-restore that points a caller's slot at a new object (Case 1 above), the registration migrates from the callee's frame to the caller's frame. This is essential to the atomic-mutation guarantee: the caller, on receiving a successful return, holds a pointer to a properly-managed object whose cleanup is correctly scheduled.

**Multiple swaps in one frame.** If a command performs several writeable-parameter swaps within its body — each pointing the writeable slot at successively new objects — the registrations of all the successively-allocated objects accumulate against the calling frame. Even objects no longer reachable from any slot remain registered until the calling frame exits. This is the correct behavior (every constructed object's cleanup must run somewhere) but means programs that allocate-and-discard many objects via swap accumulate registrations within the introducing frame's pending-cleanup list.

**Programmer-level control.** A programmer who needs more granular lifecycle control structures their code to introduce shorter-lived frames, so that at-stack mechanism fires more frequently. There is no language-level "release this object now" operation that releases an object before its owning frame exits.

### 3.5 Storage Location

Object storage may live on the stack or on the heap. The language semantics are silent on which is used in any specific instance; the choice is an implementation/optimization concern. Stack allocation is permissible only when the object's lifecycle is provably contained within a single frame (no swaps that migrate registration outward). Objects that cross a writeable-parameter swap boundary fall back to heap allocation (or arena allocation, or any storage that outlives a single stack frame).

The user-visible semantics are:
- Object storage exists for the entire lifetime of the object (introducing-frame entry through introducing-frame exit, modulo migration on swap).
- At-stack methods fire at frame exit per the rules in 3.2.
- The pointer-and-registration model decouples physical storage location from cleanup responsibility.

### 3.6 Frame-Exit Hooks (Recap)

`@` (DO_ON_EXIT) and `@!` (DO_ON_EXIT_FAIL) blocks register cleanup actions for the frame in which the block is encountered. They run at frame-exit time:

- `@` runs on any frame exit (success or failure).
- `@!` runs only on failure-exit.

Neither block sees nor consumes the propagating failure. Their bodies run as if no failure were active. Internal failures of an `@`-body or `@!`-body propagate normally from the perspective of the frame-exit machinery (though see open questions for refinements).

The at-stack-exit methods (Section 3.2) are conceptually equivalent to user-written `@`/`@!` blocks that the compiler inserts implicitly at object-introducing positions.

---

## 4. Open Questions

This section consolidates open questions from both checkpoints and adds new ones from this thread. Prior questions are reproduced here in their current refined form; new questions are added at the end.

### OQ-1: Union discriminator representation

Unchanged from checkpoint 001. Unions are value-like byte-overlays whose candidates may share byte-widths and bit-patterns; some discriminator is required to prevent synthesis of incorrectly-typed pointers to a union. Unresolved: whether the discriminator lives inline with the union bytes, in the fat pointer / RTTI, or in a hybrid arrangement. The rule that must hold globally: there must be no syntactic or semantic path by which a candidate's bytes can be reinterpreted as a different candidate. Records-containing-unions push the design pressure toward an inline or hybrid solution.

### OQ-2: Implementation latitude for IN parameter passing

Refined. The language semantics commit to call-by-value for IN parameters and call-by-copy-restore for INOUT parameters. The implementation may diverge where divergence is unobservable.

The dialog has now identified an additional axis of difference between block-quotes (`:{...}` and the may-fail/must-fail variants) and command-literals (`:<...>{...}` and variants): not only do their parameter-passing conventions differ, but block-quotes are intended to play a role analogous to fexprs/macros (capturing un-evaluated source structure) while command-literals are eagerly-evaluated values. See OQ-7 for the macro/fexpr aspect specifically.

Sub-questions: conditions under which large IN parameters may be passed by reference; the macro/fexpr design's interaction with parameter passing; the correctness criterion for "unobservable divergence" in a language with first-class commands and writeable parameters.

### OQ-3: `-> name` result designator on non-writeable parameters

Unchanged. The well-formedness rule for `-> name` requires that name be a parameter or receiver but does not require writeability. The semantic meaning of a non-writeable result designator needs characterization (initial-value reading, callee-local-lifted reading, or restriction).

### OQ-4: Default initialization across the type system

Unchanged. The desugaring of `# r <- (call: ...)` and the bare `# x` introduction both require a defined default for any type used in this position. Sub-questions about how default initialization interacts with constraints (domains with invariants), constructors, and the various kinds of types remain open. The user has flagged sub-question (c) — domains with invariants like `Positive: [4]` — as already a known concern without satisfactory resolution.

### OQ-5: Single-class instance coherence

Unchanged. Class dispatch is single-parameter. Sub-questions about coherence policy (duplicate instance error vs. specificity-based ranking), orphan-instance restriction, domain-specific dispatch (sub-point (c) — user intent: most-specific-known-type wins, contingent on absence of unforeseen problems), and import-priority resolution (user preference: Julia-style "more specialized module wins", openness to rebuttals).

### OQ-6: Partial application beyond receiver-only

Unchanged. `(receiver :: name)` resolves a class command on a receiver, yielding a command-typed value with the receiver baked in. Partial application beyond this case (subsets of v-command receivers, subsets of regular command parameters) is intended to use the same conceptual mechanism but is not yet designed.

### OQ-7: Block-quote (`:{...}`) macro/fexpr semantics

The block-quote forms (`:{...}`, `?{...}`, `!{...}`) and the command-literal forms (`:<...>{...}`, `?<...>{...}`, `!<...>{...}`) differ along multiple axes. Beyond their parameter-passing differences (OQ-2), block-quotes are intended to play a role analogous to fexprs or macros — capturing un-evaluated source structure that the receiving construct can interpret on its own terms, rather than evaluating eagerly to a value the way command-literals do.

The full design — when this evaluation distinction matters, what a fexpr-like block-quote can do that a command-literal cannot, how this interacts with name resolution and hygiene, and how the typechecker reasons about block-quotes — has not been worked out and needs a dedicated intent thread.

### OQ-8: At-stack-exit method registration mechanics

Largely settled in this dialog, with one sub-question remaining.

**Settled:**
- At-stack methods may have any failure mode (`:`, `?`, `!`).
- Manually invoked at-stack methods follow ordinary failure-handling rules.
- Automatically invoked at-stack methods have their failure status silently ignored — they may neither propagate a new failure nor recover an existing one.
- At-stack methods are virtual in the class-dispatch sense; instances may override them, and overriding is sometimes necessary for proper cleanup of specific types.
- On successful copy-restore involving a writeable `^Object` parameter that swaps to a new object, the new object's at-stack registration migrates from the callee's frame to the caller's frame.
- On failure during a call that allocated a new object (Case 3), the new object remains registered with the failed callee's frame and is cleaned up by that frame's failure-exit mechanism.

**Open sub-question:**
- The precise representation and bookkeeping of registration migration (whether registrations are tracked as a list, as a graph, as part of the runtime stack frame data, etc.). This affects the implementation but should not affect user-visible semantics. The implementation must guarantee that every constructed object's at-stack methods (if any) are eventually invoked by some frame's exit, exactly once.

---

## 5. Implementation Notes

This section captures observations that are not open design questions but rather permissible optimizations, implementation strategies, or other practical guidance for the eventual compiler.

### IN-1: Eager cleanup of objects unreachable due to failure

When a writeable `^Object` parameter has been swapped to point at a new object B (Case 3 of the writeable-parameter-mechanics), and the call subsequently fails, B is by construction unreachable from any user-visible slot for the rest of the program's lifetime. The failure prevents copy-restore from migrating the pointer back to the caller, so no user-level slot can ever point at B again.

B's at-stack-fail method (and storage reclamation) may be fired immediately upon failure detection rather than deferred until normal frame-exit coda. This is sound because:

- At-stack-fail methods do not see or interact with the propagating failure (per the established `@!` semantics).
- B's reachability state is monotonic-decreasing from the moment of swap-followed-by-failure — nothing else in the program will ever point to B.

The optimization reduces per-frame pending-cleanup-list pressure in failure-heavy code paths. Implementations may apply it; the language semantics are unchanged either way (B is cleaned up by the time the failed callee's frame has finished its exit coda).

---

## 6. Topics Not Yet Covered

The remaining topics for future intent dialogs (revised in light of this checkpoint's coverage):

**Failure semantics integration with the type system, at the static-checking level.** This thread *began* in this checkpoint with the never-fails-must-handle rule (Q7) and the operational semantics of the eleven block markers (Section 2). What remains is the precise *typechecker* job: how `?` / `!` / `:` propagate through type expressions and command compositions, how recovery contexts modify the failure-mode of code regions, how command-typed values' failure-modes interact with the failure-modes of the contexts they're invoked in, and how this all integrates with the operational rules from the README. This is the natural next major thread.

**Command literals and quotes.** The grammar distinguishes `:{...}` / `?{...}` / `!{...}` block quotes from `:<...>{...}` / `?<...>{...}` / `!<...>{...}` command literals. OQ-7 captures the specific question of macro/fexpr semantics for block-quotes; the broader thread of how each form is meant to be used, how scoping and capture work, when they are equivalent, and how they fit into expression-position vs. command-position has not been substantively addressed.

**Modules, imports, scopes, and visibility.** Not yet addressed. Intersects with OQ-5 (instance coherence and import priority).

**Tests and programs as first-class entry points.** Mentioned as the only places where "no non-local state" relaxes; the precise rules for what they may do that commands cannot, and what their static checking looks like, have not been worked through.

**IR design (issue #24).** Not yet addressed.

**FFI design (issue #23).** Not yet addressed.

**Railroad diagrams (issue #8).** Presentational only.

---

## 7. Updated Glossary

Terms introduced or refined in this checkpoint, supplementing the glossary in checkpoint 001.

**Branch marker (`-`)** — the else-companion of `?` or `?-`. Body is not recovered; runs when the predecessor's body did not engage.

**Case-when chain (`?:` chain)** — a sequence of consecutive `?:` siblings at the same indentation level, ended by the first non-`?:` sibling (or end of enclosing block). Each `?:` is a guard-block; the first guard to succeed engages its body and skips the rest of the chain. The non-`?:` siblings that end the chain serve as the default arm.

**Default arm** — the non-`?:` statements that end a `?:` chain. They run if all guards in the chain fail. They have no syntactic marker; they are simply whatever comes after the chain at the same indentation level.

**Escape elevator (`??`)** — meta-marker requiring an inner `?` or `?-` block as its first executable inner item. Modifies the structural destination at which the inner guard's failure-recovery resumes execution, lifting it from inner-construct's-next-sibling to `??`-block's-next-sibling.

**External-failure recovery** — the failure-handling category of `|` and `|`-with-spec: engages on a propagating failure from preceding siblings rather than on a guard's outcome.

**Fall-through (in `?:`)** — when all `?:` guards in a chain fail and control reaches the default arm. Distinct from C-style fallthrough (which Basis does not have).

**Failure value (Φ)** — the value carried by a failure as it propagates. Bare `|` does not bind it; `|`-with-spec binds it to the spec's identifier within the recovery body. `?-` consumes the failure but does not expose its value.

**Frame-exit hook** — see checkpoint 001 glossary; reaffirmed in this checkpoint with sharper coverage of `@!` semantics (runs alongside but does not see or consume the failure).

**Full-body recovery** — the failure-handling category of `^`: every statement in the body has its failure consumed by the construct.

**Guard** — the first statement of a `?`-block, `?-`-block, or `?:`-block. Its failure is consumed by the construct.

**Guard-only recovery** — the failure-handling category of `?`, `?-`, and `?:`: only the guard's failure is recovered; statements 2..N follow normal failure rules.

**Loop (canonical pattern)** — `?? ? guard ... ^ continueTest` ... or variants. Combines guard-failure-elevation with rewind to produce a structured loop with two distinct termination modes (guard-failure and continue-test-failure) routed identically.

**Plain grouping (`%`)** — block with no semantic effect beyond establishing a sub-block.

**Recovery block (`|`, `|`-with-spec)** — sibling block engaging on propagating failure from preceding siblings. Bare form catches any failure without binding; spec'd form filters and binds.

**Re-fail pattern** — `?-` followed by `fail somethingDomainSpecific`, used to translate low-level failures to domain-specific ones without leaking implementation details.

**Rewind block (`^`)** — sibling block whose body, on success, jumps back to the preceding sibling; on failure, terminates the loop. Body has full-body recovery.

---

## 8. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-26.

**Source materials:** Same as checkpoint 001 — README (with caveats), Token.h, Lexer.h, Productions.h, Grammar2.h, Grammar2.cpp, Parsing2.h, Ast.h, AstBuilder.h, partial AstBuilder.cpp. The test files in basis_tests/ remain unreadable from the chat interface and were not consulted. The user noted during this thread that `|:` (which appears in the README's marker list) is not present in the current grammar; only `|` and `|`-with-spec exist. Grammar2.cpp is authoritative.

**Recommended next step:** Commit alongside intent-checkpoint-001.md (suggested path: `docs/intent-checkpoint-002.md`). The natural next intent thread is failure-mode integration with the typechecker — translating the operational rules of Section 2 into static rules the compiler can verify, with particular attention to how command-typed values' `:` / `?` / `!` marks compose with the failure-modes of contexts in which they're invoked.
