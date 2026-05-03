# Basis Language — Intent Checkpoint 003

**Status:** Small working checkpoint. Builds on intent-checkpoint-001.md and intent-checkpoint-002.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7), captured 2026-04-27. This checkpoint is intentionally narrow: it records two clarifications that emerged during a short follow-up discussion and that are worth pinning down before they're lost. Further discussion is expected; this is not a comprehensive update.

**Authority statement.** Where this document differs from intent-checkpoint-002.md or intent-checkpoint-001.md, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. `%`-Blocks as Compound Guards

This section refines the treatment of `%` (DO_BLOCK) from checkpoint 002 §2.8 and the guard-block discussions in §§2.2–2.5.

### 1.1 The Mechanism

A `%`-block has no control-flow semantics of its own — its body executes as ordinary sequential statements, and any unrecovered failure inside propagates out. This makes `%` directly usable as the **guard** of a `?`, `?-`, or `?:` block: when a `%`-block stands in guard position, the entire `%`-body must succeed for the guard to succeed.

This gives the language a logical-conjunction primitive for guards without introducing any new construct: the `%`-body is a sequence, and the sequence succeeds only when every statement in it succeeds. The first failing statement aborts the rest of the `%`-body and propagates its failure out — which is precisely the failure the enclosing `?`-family construct consumes.

### 1.2 Disjunctions Inside the Conjunction

Within a `%`-guard, `|`-chains supply disjunction. Per checkpoint 002 §2.9, a `|`-block engages on a propagating failure from preceding siblings at the same indentation level, and consumes that failure on engagement. So a sequence like:

```
% testA: ...
  | testB: ...
  testC: ...
  | testD: ...
  | testE: ...
```

means: (testA OR testB) AND (testC OR testD OR testE). Each `|`-chain locally resolves a disjunction: if any alternative in the chain succeeds, the failure cascade stops there and execution continues to the next conjunct. If all alternatives in a chain fail, the final failure propagates out of the `%`-block, the `%`-guard fails, and the enclosing construct sees that failure.

`|`-with-spec compositions inside the `%`-body work the same way, with the additional filtering and binding semantics from checkpoint 002 §2.10.

### 1.3 Failure Visibility From Outside the `%`-Guard

The OR-resolution inside a `%`-guard is invisible from outside the `%`-block. Intermediate failures that get consumed by a `|`-chain never reach the enclosing `?`-family construct; only the failure that ultimately escapes the `%`-body — i.e., the one from a conjunct whose disjunctive alternatives were all exhausted — is what the outer `?` (or `??`, when elevated) sees and consumes.

The failure *value* surfaced to the outside is therefore the failure of the last-tried alternative in whichever conjunct gave up. This is the most informative failure available at that point, and is the value that an enclosing `|`-with-spec (in elevated form via `??`) would filter and bind.

### 1.4 Composition With `??` and `?-`

`??` pairs with `?` (or `?-`) regardless of whether the guard is a simple statement or a `%`-block. The canonical compound-guard loop pattern is:

```
?? ? % conjunct1: ...
       | alternative1
       conjunct2: ...
       | alternative2
        loopBody
   ^ continueCondition: ...
     adjustIndex: i
```

The `?? ?` pairing is unchanged; the `%` simply occupies guard position. `??`'s elevation behavior is unaffected by the guard's internal structure.

Under `?-` with a `%`-guard, the body runs when the conjunction fails, which by ordinary De Morgan reading means "at least one of the conjuncts failed" (where each conjunct itself may have been a disjunction over `|`-chained alternatives). The `?-` consumes the fact of the conjunction's failure; the failure value is discarded as usual for `?-` (per checkpoint 002 §2.3).

### 1.5 Summary

The `%`-block has no control-flow semantics of its own, but its sequential-execution-with-failure-propagation behavior makes it the natural site for compound guards. Combined with `|`-chains for disjunction, `%`-guards give Basis full propositional-logic expressiveness for `?`-family conditionals — without adding any new marker or construct, and without breaking any of the composition rules established in checkpoint 002 §2.11.

---

## 2. Eager Cleanup Optimization: Asymmetry Between `@` and `@!`

This section refines IN-1 from checkpoint 002 §5. The original note is split into two implementation observations whose preconditions differ meaningfully.

### IN-1a: Eager `@`-Method Firing for Unreachable Objects

When an object becomes provably unreachable from any user-visible slot, its `@`-tagged at-stack-exit method (`DEF_CMD_RECEIVER_ATSTACK`) may be fired immediately rather than deferred to the registering frame's exit coda.

The precondition for this optimization is **unreachability alone**. `@`-methods run on any frame exit — success or failure — so their firing is not conditional on the eventual exit status of the registering frame. As soon as the optimizer can prove the object is dead, it may fire the `@`-method without waiting to see how the frame ends.

The Case 3 scenario from checkpoint 002 §3.3 (writeable `^Object` parameter swapped to a new B, then call fails) is one instance where unreachability is provable: B is by construction unreachable from any user-visible slot for the rest of the program's lifetime. But the optimization's applicability is not limited to Case 3 — any moment of provable unreachability suffices.

This optimization reduces per-frame pending-cleanup-list pressure. The user-visible semantics are unchanged: the `@`-method runs exactly once, before the program could observably depend on its having run.

### IN-1b: Eager `@!`-Method Firing — Restricted

When an object becomes provably unreachable *and* the registering frame's exit status is also already determined to be a failure exit, its `@!`-tagged at-stack-exit method (`DEF_CMD_RECEIVER_ATSTACK_FAIL`) may be fired immediately.

Both preconditions are required. `@!`-methods run only on failure exit, so eager firing is unsound unless the failure-exit outcome is committed. The optimizer cannot fire an `@!`-method speculatively: if the frame ultimately exits successfully, the `@!`-method must not run, and a speculative early firing cannot be undone.

The Case 3 scenario satisfies both preconditions simultaneously — the swap-then-fail pattern makes B unreachable *at the same moment* that the frame is committed to a failure path. So in Case 3, IN-1b does license eager firing of B's `@!`-method. But the optimization does not generalize to "any time an object becomes unreachable," because in most such cases the eventual exit status is not yet known.

### User-Facing Guidance

The asymmetry between IN-1a and IN-1b has a practical consequence for code that uses `@!`-methods:

- An `@`-method's *eventual* firing is guaranteed by the language; its *timing* may be earlier than frame-exit when the optimizer can prove unreachability.
- An `@!`-method's *eventual* firing on failure is guaranteed by the language; its *timing* is generally less predictable than `@`'s, because eager firing requires both unreachability and committed failure-exit, and most points of unreachability do not coincide with committed failure-exit.

Code that depends on an `@!`-method running *promptly* (rather than just eventually-on-failure) needs to be written with that in mind. If prompt cleanup matters and the cleanup logic is failure-conditional, the programmer should consider whether `@`-with-an-explicit-conditional-body, manual invocation at a known-failure point, or restructuring into a shorter-lived frame (so frame-exit happens sooner) would be more reliable than relying on `@!`-eager-firing.

`@!`-methods remain correct and useful; they are simply less predictable in their *firing time*, and load-bearing assumptions about that time should be avoided.

---

## 3. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27.

**Recommended next step:** Commit alongside intent-checkpoint-001.md and intent-checkpoint-002.md (suggested path: `docs/intent-checkpoint-003.md`).
