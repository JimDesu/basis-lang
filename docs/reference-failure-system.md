# Basis Language — Reference: Failure System

**Status:** Topic-organized authoritative reference for the failure-handling design. Consolidates the failure-related material across intent-checkpoint-002, intent-checkpoint-003, intent-checkpoint-011, and intent-checkpoint-012, integrates post-CP012 refinements identified during the OQ-26 dialog, and supersedes the corresponding sections of those checkpoints where the reference differs.
**Date:** 2026-04-29
**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, with post-CP012 refinements per the typed-failures conversation that followed CP012's commit.

**Authority statement.** Where this reference differs from CP002, CP003, CP011, or CP012 on a failure-handling matter, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative.

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]* identifying the source checkpoint sections that established them. Section-level connective tissue carries footer citations identifying the section spans drawn upon. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints — typically resolving an ambiguity, recording a post-CP012 correction, or filling a gap in the source material. These markers are the user's primary review handle and are flagged explicitly.

---

## 1. Failure Modes and Marks

### 1.1 The Three Marks

Every command in Basis carries one of three failure-mode marks, declared as part of the command's signature:

The `:` mark — the **never-fails** mark — denotes a command that always succeeds. The body must reach its exit on a non-failing path on every reachable execution.

The `?` mark — the **may-fail** mark — denotes a command that may succeed or may produce a propagating failure. The body's exits may be in any state; the caller must be prepared to handle either outcome.

The `!` mark — the **must-fail** mark — denotes a command that always fails. The body must reach its exit on a propagating-failure path on every reachable execution; no successful return is possible.

The marks classify command-typed values uniformly across all the language's command-bearing constructs: named commands declared with `.cmd`, slots of command type, class methods, v-commands, lambdas, and fexprs all carry one of the three marks as part of their visible signature. *[CP011 §5.6–§5.8]*

### 1.2 Subsumption on the Mark

The marks form a small partial order: `: <: ?` and `! <: ?`, with `:` and `!` mutually incomparable. A `:`-marked or `!`-marked value may stand wherever a `?`-marked value is expected; a `?`-marked value may not stand for either of the more specific marks. *[CP011 §5.2]*

The order has a natural reading: `?` is "may-or-may-not," `:` and `!` are the two "definitely" specializations of that. A `:`-value is more useful at a `?`-call site than a generic `?`-value (the failure edge is statically dead) but the typechecker's analysis at the call site is still that of a `?`-call: subsumption permits the substitution but does not narrow the static analysis. The corollary is that signatures opaquely upcast — the caller of a `?`-marked command sees the `?` two-edge pattern even when the callee's body internally produces only `failing(!)`. *[CP011 §5.2]*

### 1.3 Parameter-Mode Invariance

Parameter modes (`'` productive, `&` reference, plain IN) and parameter types are **invariant** under mark subsumption. A `:<Int32'>` value is not interchangeable with `:<Int32>` or `:<Int32&>`; a `?<Int32'>` value is not interchangeable with `?<Int32>` or `?<Int32&>`. The subsumption relation is solely on the failure mark. *[CP011 §5.2]*

Invariance is essential for soundness: a `'r` parameter discharges a productive obligation and a `&r` parameter requires `r` initialized at the call site; substituting one mode for another would break the per-mode static rules even if the underlying type and failure-mode were compatible.

### 1.4 Never-Fails-Must-Handle as Corollary

The "never-fails-must-handle" rule from CP002 §1.4 — that a `:`-marked command body must handle every internal `?`-call's possible failure — is not a separate rule of the language. It is a corollary of `:`-conformance under the static analysis (§3 below). *[CP011 §8.1]*

A `:`-marked body's exits must all be in state `clear`. Every internal `?`- or `!`-call produces a failure edge with state `failing(?)` or `failing(!)`. For that failure not to contribute to an exit-edge state of `failing(M)` or `mixed(M)`, it must be consumed by some recovery context within the body — `?`'s guard recovery, `?-`'s guard recovery, `?:`'s chain recoveries, `^`'s full-body recovery, `|` (bare or with-spec) on a propagating-failure path, or `??`'s rewiring of the inner-construct's guard failure. If no recovery context catches the failure, the failure edge propagates to the body's exit and `:`-conformance fails.

### 1.5 The Dual Rule for `!`-Bodies

The conformance rules for `:` and `!` are exact duals: `:` rejects every exit-edge state containing a propagating-failure contribution; `!` rejects every exit-edge state containing a successful-path contribution. *[CP011 §6.2]*

The dual of must-handle is therefore the "must-fail" rule: every reachable path through a `!`-marked body must produce a propagating failure that reaches the exit. A path that fully recovers a propagating failure within the body (so the post-recovery state is `clear`) and then reaches the exit is a `!`-conformance failure. The fix is to fire `.fail` on that path, or to restructure so the failure is not fully recovered. *[CP011 §8.2]*

The `?` middle accepts everything (subject to the reachability rider in §3.7 below).

*Sources for §1: CP002 §1.4 (never-fails-must-handle, original statement); CP011 §§5.2, 6.1–6.2, 8.1–8.2.*

---

## 2. Block Markers for Failure Handling

This section covers the eleven block-marker constructs as they pertain to failure flow. Operational details independent of failure (visual organization, default-arm semantics for `?:`, etc.) appear in CP002 §2 and are not duplicated here; this reference focuses on the failure-handling role of each marker.

### 2.1 Categorization

The eleven markers split into roles by what they recover and how they affect failure flow. *[CP002 §2.1]*

**Guard-only recovery markers** — `?` (DO_WHEN), `?-` (DO_WHEN_FAIL), and `?:` (DO_WHEN_SELECT) — each have a privileged first statement (the *guard*) whose failure is consumed by the construct itself. Statements after the guard run as ordinary code; their failures propagate per the surrounding context's rules.

**Full-body recovery marker** — `^` (DO_REWIND) — the entire body is a recovered region. Any failure in any body statement is consumed by the construct, terminates the body, and signals loop termination.

**External-failure recovery markers** — `|` (DO_RECOVER) and `|`-with-spec (DO_RECOVER_SPEC) — engage on a failure propagating from preceding siblings at the same indentation level. Their bodies are *not* recovered; statements within them propagate failures normally.

**Escape-elevator marker** — `??` (DO_WHEN_MULTI) — does not directly recover anything; modifies the structural destination at which a paired guard's recovery resumes.

**Branch marker** — `-` (DO_ELSE) — the else-companion of `?` or `?-`. Recovers nothing; provides the alternative branch.

**Plain grouping** — `%` (DO_BLOCK) — has no control-flow or recovery semantics of its own. Its sequential-execution-with-failure-propagation behavior makes it usable as a *compound guard* when it stands in guard position. See §2.7.

**Lifecycle hook markers** — `@` (DO_ON_EXIT) and `@!` (DO_ON_EXIT_FAIL) — frame-exit hooks. They neither see nor consume the propagating failure. See §2.8.

### 2.2 Guard-Only Recovery: `?`, `?-`, `?:`

A `?`-block runs its guard; if the guard succeeds, the body runs; if the guard fails, the body is skipped and the guard's failure is consumed. Either way, control proceeds to the statement following the `?`-block. *[CP002 §2.2]*

A `?-`-block is the mirror image: the body runs *iff* the guard fails, and the guard's failure is consumed regardless of which path engaged. Critically, **`?-` consumes the fact of failure, not the value**; the propagating failure value is not surfaced to the body. Programs that need the value should use `|`-with-spec instead. *[CP002 §2.3]*

A `?:`-block on its own is similar to `?`. The defining feature is *chaining*: a sequence of consecutive `?:` siblings at the same indentation level forms a coherent multi-branch construct. The first guard to succeed engages its body and skips the rest of the chain; if all guards fail, the non-`?:` siblings following the chain serve as the default arm. The chain has no syntactic wrapping construct — it is recognized purely by adjacency. *[CP002 §2.4]*

In all three cases, only the guard's failure is recovered. Statements after the guard within the body have their failures propagate per the surrounding context, not consumed by the construct. *[CP002 §§2.2, 2.4]*

### 2.3 Full-Body Recovery: `^` (Including Body-Optional Refinement)

The `^`-block is a sibling whose body controls whether execution loops back to the preceding sibling. When control reaches `^`, the body runs. On body success, control jumps back to the preceding sibling at the same indentation level (loop continues). On body failure, the failure is consumed, the body terminates at the failed statement, and control falls through to the statement following the `^`-block (loop terminates). *[CP002 §2.6]*

Unlike `?` and `?-`, the `^`-body recovers *every* failure within itself, not just an "overall body outcome." Every failing statement consumes its failure into `^` and terminates the body. This makes the body a unified test-and-adjust region: comparisons compose freely with adjustments, with the first-failure determining loop termination. Inside an `^`-body, every `?`-call is statically satisfied — `^` is the recovery context.

**Body is optional.** A bodiless `^` is treated as if its body had succeeded, producing an unconditional rewind to the preceding sibling. The bodiless form is the cleanest expression of "loop until the guard fails" idioms and pairs naturally with `?? ?` to form the canonical loop. *[CP011 §10.1; refines CP002 §2.6, which implicitly assumed the body was always present.]*

**Preceding sibling is required.** A `^` with no preceding sibling at the same indentation level is a static semantic error reported at parse or AST-construction time. *[CP011 §10.1]*

### 2.4 External-Failure Recovery: `|` and `|`-with-spec

A bare `|`-block engages when a failure is propagating from any preceding sibling at the same indentation level. If no failure is propagating, the `|`-block is skipped. On engagement, the failure is consumed immediately; the body runs from no-active-failure. The body is not itself a recovery context — its statements propagate failures normally. *[CP002 §2.9]*

The bare form does not bind the failure value; the body has no access to it. To inspect or act on the failure value, the `|`-with-spec form is required.

A `|`-with-spec block adds two refinements to the bare form: filtering and binding. The spec acts as a predicate on the propagating failure's value; the block engages only if the failure matches the spec. Non-matching failures continue propagating *as if the block were not there*. When the spec matches, the spec's identifier is bound to the failure value within the body's scope. *[CP002 §2.10]*

The combination of bare `|` and `|`-with-spec supports cascade-style handling chains: a sequence of `|`-with-spec blocks each filtering for some subset of failures, optionally followed by a bare `|` as catch-all. The first whose spec matches engages; programmers put more-specific specs first and broader specs (or bare `|`) last. *[CP002 §2.10]*

Each `|`-block is also a recovery target for *preceding `|`-blocks' bodies*. If `|` block N engages and its body itself fails, block N+1 (if present) can catch it. The cascade generalizes: each block is both a recovery destination and a potential recovery source. *[CP002 §2.10]*

Under typed failures (§4 below), the `|`-with-spec spec is refined to a tag specification: `|: Tag t` engages on a propagating failure whose tag is at-or-below `Tag` in the relevant hierarchy, binding `t` at the tag's bound class (or with no binding for payload-less tags). The cascade structure is unchanged. *[CP012 §3.4]*

### 2.5 The Escape Elevator: `??`

`??` is a meta-marker. It pairs with a `?`-family block (a `?` or a `?-`) as its first executable inner construct, and modifies *where* the inner block's guard-failure recovery resumes execution. Without `??`, the inner guard's failure is consumed by the inner block and execution resumes at the next sibling of the inner block. With `??`, the failure is still consumed (by `??` itself), but execution resumes at the next sibling of the `??`-block — one indentation level higher. *[CP002 §2.5]*

`??` is therefore a failure-consuming construct whose distinctive feature is *where it places execution after consumption*. The failure does not escape `??`; the failure is fully consumed by `??` itself. Bare `??` (with no inner `?` or `?-`) is a static semantic error.

**Crucial corollary.** A `|`-block placed as the next sibling after a `??`-block does *not* engage on the inner guard's failure, because that failure has already been consumed by `??`. There is no propagating failure for `|` to catch. A `|` after `??` engages only on failures from non-guard operations within the `??`-block — body statements after a successful guard, or other propagated failures from inside the construct. *[CP002 §2.5]*

`??` operates at a fixed elevation of one level. There is no `???` or further-elevating construct; if multiple levels of elevation are needed, the code must be restructured.

### 2.6 The Branch: `-`

`-` attaches only to a preceding `?` or `?-` block at the same indentation level. With `?`, the body engages on guard success and `-` engages on guard failure (standard if/else); with `?-`, the body engages on guard failure and `-` engages on guard success (inverted if/else, success case in the `-` branch). *[CP002 §2.7]*

The `-`-body is *not* a recovery context. Failures within it propagate per the enclosing rules. The branch consumes nothing; it merely supplies the alternative path for the predecessor's outcome.

`-` does not chain (no `? ... - ... -` ...); for n-ary branching, use `?:`.

### 2.7 Compound Guards via `%`

`%` on its own has no control-flow effect: its body executes as ordinary sequential statements, and any unrecovered failure inside propagates out. This makes `%` directly usable as the **guard** of a `?`, `?-`, or `?:` block: when a `%`-block stands in guard position, the entire `%`-body must succeed for the guard to succeed. *[CP003 §1.1]*

The `%`-body provides logical conjunction without a new construct. Within a `%`-guard, `|`-chains supply disjunction: a `|` engaging on a propagating failure from the immediately preceding statement consumes that failure on engagement, so a sequence like

```
% testA: ...
  | testB: ...
  testC: ...
  | testD: ...
  | testE: ...
```

means `(testA OR testB) AND (testC OR (testD OR testE))`. Each `|`-chain locally resolves a disjunction; the OR-resolution is invisible from outside the `%`-block. Only the failure that ultimately escapes — from a conjunct whose disjunctive alternatives were all exhausted — is what the enclosing `?`-family construct sees and consumes. *[CP003 §1.2–§1.3]*

The failure value surfaced to the outside is the failure of the last-tried alternative in whichever conjunct gave up; under typed failures (§4), this also determines the tag of the propagating failure. *[CP003 §1.3; refines under §4 below.]*

`%` does not combine with `??` (no failure-escape to elevate) and does not pair with `-`. *[CP002 §2.11]*

### 2.8 Frame-Exit Hooks: `@` and `@!`

`@` (DO_ON_EXIT) registers a cleanup body that runs at frame-exit, regardless of whether the frame exits via success or failure. `@!` (DO_ON_EXIT_FAIL) registers a cleanup body that runs only on failure-exit. Neither block sees nor consumes the propagating failure; their bodies execute as if no failure were active, even when the frame is exiting via failure. *[CP002 §3.6]*

Internal failures of an `@`-body or `@!`-body propagate within the cleanup flow but do not return to the body's normal CFG; the cleanup machinery handles them per its own rules.

The at-stack-exit mechanism for object cleanup (`DEF_CMD_RECEIVER_ATSTACK` and `DEF_CMD_RECEIVER_ATSTACK_FAIL`) is the language's only intentional exception to the no-invisible-control-flow principle. Conceptually, an at-stack method is a `@`/`@!` block the compiler inserts implicitly at object-introducing positions. The static analysis of `@` and `@!` (§3.4 below) treats both forms uniformly. *[CP002 §§1.4, 3.2, 3.6]*

The implementation's eager-firing optimizations differ between `@` and `@!`: `@` may fire eagerly on provable unreachability alone, since `@`-methods run on any frame exit; `@!` requires both unreachability and committed-failure-exit, so eager firing is more restricted. This affects timing predictability for `@!`-method workloads but not user-facing semantics. *[CP003 §IN-1a, §IN-1b]*

### 2.9 Re-Fail

A common composition is `?-` followed by `.fail`, which converts a low-level failure to a domain-meaningful failure without leaking implementation details:

```
?- lowLevelOp: ...
    .fail: domainSpecificFailure
```

The original failure value is discarded by `?-` (which consumes the *fact* of failure but not the *value*); the new domain-specific failure replaces it. *[CP002 §2.3]*

Under typed failures (§4 and §5 below), the new `.fail` is an ordinary `.fail` originating from the handler's frame — there is no re-fail-specific machinery in the language. The original failure is consumed by `?-`'s mechanism; the new failure has its own originating frame, its own slot population, its own payload-storage. *[CP012 §6.7]*

When the new `.fail`'s payload value is the bound name from a preceding `|`-with-spec — i.e., when the handler is forwarding the recovered failure value into a freshly-tagged failure — the bound value moves into the new payload position. The handling of this move and the timing of at-stack handlers on the moved value is the subject of §5.5–§5.9 below. *[Reconciliation: this framing draws on the post-CP012 dialog; CP012 §6.7 sketches the same scenario but in pre-correction language. See §5.5 for the corrected discipline.]*

### 2.10 Composition Summary

The composition table from CP002 §2.11, retained as-is and unchanged by this reference:

| Marker | Combines with `??`? | Combines with `-`? | Body recovers failures? |
| --- | --- | --- | --- |
| `?` | yes (→ elevates guard escape) | yes (→ if/else) | guard only |
| `?-` | yes (→ elevates guard escape) | yes (→ inverted if/else) | guard only |
| `?:` | no | no (chain with more `?:` instead) | guard only |
| `-` | (paired only) | n/a | no |
| `%` | no | no | no (but body propagates; usable as compound guard) |
| `^` | n/a (`^` is sibling, not inner) | no | yes (entire body) |
| `\|` | yes (`?? \|` at same level — `\|` catches non-guard failures from `??`-block) | no | no |
| `\|`-spec | (same as `\|`) | no | no |
| `@` | no | no | no (runs alongside, doesn't recover) |
| `@!` | no | no | no (runs alongside, doesn't recover) |

*Sources for §2: CP002 §§2.1–2.11, 3.6; CP003 §1; CP011 §10.1.*

---

## 3. Failure-Mode Static Analysis

### 3.1 The Analysis at a Glance

The Basis typechecker performs a **forward-flow analysis** over the control-flow graph of each command body. At each program point the analysis maintains a state with two components: a failure-state lattice value (this section) and a per-slot initialization-state vector (CP004 §3, integrated here). The two components share the CFG walk and the same join points but join independently and interact only at transfer functions. *[CP011 §1.1, §2.3, §7]*

The analysis reads the operational rules from §2 above and translates them into static state transitions. The conformance check at the body's exit edges uses both components together to decide whether the body matches its declared mark and obeys the initialization discipline.

CFG and forward-flow-analysis vocabulary follow the standard compiler-construction definitions; CP011 §2 is the canonical introduction, not duplicated here.

### 3.2 The Six-State Lattice

The failure-state lattice has six elements: *[CP011 §3.1]*

| State | Meaning |
| --- | --- |
| `clear` | No failure is propagating at this program point. |
| `failing(?)` | A `?`-mode failure is propagating. |
| `failing(!)` | A `!`-mode failure is propagating. |
| `mixed(?)` | Some incoming paths reach this point clear, others with `failing(?)` (possibly widened from `failing(!)`). |
| `mixed(!)` | Some incoming paths reach this point clear, others with `failing(!)`. |
| `unreachable` | No incoming path reaches this program point. |

The states are mutually exclusive: at any program point under the analysis, the state is exactly one of these six.

### 3.3 The Partial Order and Join Table

`unreachable` is the lattice's bottom and the identity for join. Above it sit `clear`, `failing(!)`, and `failing(?)` as pairwise-incomparable peers. `mixed(!)` is the join of `clear` with `failing(!)`. `mixed(?)` is the supremum of every pair-of-states that includes a `?`-flavored failure or that has a propagating-failure arm and a clear arm. There is no element above `mixed(?)`. *[CP011 §3.2–§3.3]*

| ∨ | `unreach` | `clear` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| --- | --- | --- | --- | --- | --- | --- |
| `unreach` | `unreach` | `clear` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| `clear` | `clear` | `clear` | `mixed(!)` | `mixed(?)` | `mixed(!)` | `mixed(?)` |
| `failing(!)` | `failing(!)` | `mixed(!)` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| `failing(?)` | `failing(?)` | `mixed(?)` | `failing(?)` | `failing(?)` | `mixed(?)` | `mixed(?)` |
| `mixed(!)` | `mixed(!)` | `mixed(!)` | `mixed(!)` | `mixed(?)` | `mixed(!)` | `mixed(?)` |
| `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` |

Two reading rules summarize the table. **Widening of `!` to `?`** — whenever a `!`-flavored state is joined with a `?`-flavored state, the result is `?`-flavored. The `?` mark is the supremum: a context that joins a `!`-failure path with a `?`-failure path can only conservatively claim "a `?`-mode failure may be propagating." **`clear` plus failing produces `mixed`** — a node with one incoming clear path and one incoming failing path is reachable on both; downstream code must consider both possibilities. *[CP011 §3.3]*

The `unreachable` state propagates through transfer functions: if a node's incoming state is `unreachable`, every outgoing edge has `unreachable` as its outgoing state. This handles dead-code regions cleanly — code reachable only after a `!`-call's success edge (which doesn't exist) is `unreachable`, and the analysis correctly identifies it as such. *[CP011 §3.5]*

### 3.4 Transfer Functions for Block Markers

Each of the eleven block markers has a transfer function that translates its operational behavior into state transitions over the lattice. CP011 §4 specifies all eleven in full; the load-bearing structures are summarized here.

For **guard-only recoveries** (`?`, `?-`, `?:`), the guard-failure edge produces state `clear` at the post-block program point (the failure is consumed by the construct), and the body's exit state contributes whatever the body produced. The post-block join is the join of these. *[CP011 §§4.1–4.3]*

For `^` **with body**, every body statement's failure edge is consumed. The outgoing rewind edge has state `clear` (body succeeded — no failure occurred); the outgoing post-`^` edge has state `clear` (body failed — failure consumed). For `^` **without body**, the outgoing rewind edge has state `S` (the entry state passes through unchanged); there is no post-`^` outgoing edge from the `^`-block itself. *[CP011 §4.6]*

For **`|`** (bare), entry state `clear` skips the block; entry state `failing(M)` engages, body executes from `clear`, outgoing state is the body's exit; entry state `mixed(M)` joins the engaged path's body-exit with the skipped path's `clear`. *[CP011 §4.7]*

For **`|`-with-spec** (pre-typed-failures), the conservative analysis assumes both engagement and non-engagement are possible on a `failing(M)` entry: the engagement-path outgoing state is the body's exit; the non-engagement-path outgoing state is `failing(M)` (propagating past the block). The combined outgoing state is the join. Under typed failures (§4), this conservative treatment can be sharpened — see §3.9 below. *[CP011 §4.8]*

For `??`, the transfer function is structural: the inner `?`-block's guard-failure edge is rewired to the post-`??` program point. This is a CFG topology change, not a state transition; downstream of the rewiring, edges propagate normally. The crucial corollary remains: a `|` placed at the `??`-elevated level catches no inner-guard failures (those are consumed by `??`), only failures from non-guard operations within the `??`-block. *[CP011 §4.9]*

`@` and `@!` have identity transfer functions in the body's normal CFG — their effect is registration for a separate cleanup-flow, with its own CFG and its own analysis. *[CP011 §§4.10–4.11]*

`-`, `%` are unsurprising: `-` propagates `clear` to its body-entry on the engaged edge; `%` is identity through its body's statements. *[CP011 §§4.4–4.5]*

### 3.5 Transfer Functions for Call Sites

A call site invokes a value of command type. The mark of that value as declared at the call site — *not* what it happens to be at runtime — determines the transfer function. *[CP011 §5.1]*

A `:`-invocation produces one outgoing edge: the success edge with state `clear`. A `?`-invocation produces two: the success edge with `clear` and the failure edge with `failing(?)`. A `!`-invocation produces one: the failure edge with `failing(!)`; the next ordinary statement is reachable from the call only via the failure edge (i.e., not at all, unless caught by a recovery context).

Subsumption at the call site reads the slot's mark, not the runtime value. A `?`-typed slot holding a `:`-marked value still produces the `?` two-edge pattern at the call site. The analysis is conservative; programs that need tighter analysis should declare slots more tightly. *[CP011 §5.2]*

The `.fail` keyword is a `!`-call producing a failure value. Its post-call state is `failing(!)`; its operand (under typed failures, the tag plus optional payload) determines the failure value's identity. *[CP011 §5.9; refined under §4 below.]*

Productive parameter discharge happens on the success edge of the call. A `:`-call's productive parameters always discharge; a `?`-call's discharge conditionally on the success edge; a `!`-call's productive parameters never discharge (the success edge does not exist). Reference parameters require their slot to be `init` at the call site, regardless of edge. *[CP011 §§5.3–5.4]*

### 3.6 Body-Conformance: The Three Rules

A command body is **conformant** with its declared mark when its CFG analysis exit edges satisfy the rule for that mark. *[CP011 §6.1]*

A `:`-marked body is well-formed when every reachable exit edge has state `clear` and at least one exit edge is reachable. Any exit in `failing(M)` or `mixed(M)` fails conformance — a propagating failure has reached the body's exit, contradicting the `:` declaration.

A `?`-marked body is well-formed when at least one exit edge is reachable. There is no constraint on the *state* of the exit edges: they may be `clear`, `failing(?)`, `failing(!)`, `mixed(?)`, or `mixed(!)`.

A `!`-marked body is well-formed when every reachable exit edge has state `failing(?)` or `failing(!)` and at least one exit edge is reachable. Any exit in `clear` or `mixed(M)` fails conformance: `clear` because the path is purely successful; `mixed(M)` because some path (the clear arm) is successful, even if other paths are failing.

The duality from §1.5 surfaces directly here: `:` rejects every state containing a propagating-failure contribution; `!` rejects every state containing a successful-path contribution. The lattice was designed so that these checks are simple state-membership tests. *[CP011 §6.2]*

### 3.7 The Reachability Rider

A body whose every exit edge is in `unreachable` state has no reachable exit. This is dead-code-equivalent: the body has no way to reach its end. The reachability rider in each conformance rule rejects this case as a typecheck failure rather than just a warning, because a body that cannot complete is not a well-defined command. *[CP011 §6.3]*

The distinction between "exit edge is reachable in state `failing(M)`" (well-formed for `?` and `!`) and "exit edge is in `unreachable` state because no incoming edge to the exit was reachable at all" is subtle but important. In the former, control reaches the exit, just along a failing path. In the latter, control does not reach the exit. The former corresponds to the body fulfilling its (failing) contract; the latter corresponds to dead-code-equivalence.

### 3.8 Integration With Initialization Analysis

The full state at each program point is a pair (failure-state, init-state-vector), where the failure-state is one of the six values from §3.2 and the init-state-vector assigns each in-scope slot one of `init` / `uninit` / `uncertain` per CP004 §3. The product lattice's join is component-wise. *[CP011 §7.1]*

The two components do not interact at the join level. They *do* interact at the transfer-function level: the failure-state determines which edges are reachable, and the init-state transitions happen on those reachable edges. Productive parameter discharge updates the init-state on the success edge of a call (whose failure-state is `clear` for any call with a success edge); a `?`-call's failure edge does not transition any productive parameter slots. Reference parameters require their slots `init` at the call site, checked on the joined incoming state regardless of subsequent edge. *[CP011 §7.2]*

The single CFG walk produces both analyses. The implementation maintains one work-list, one state-vector per node, and applies the joint transfer function at each visit. The conformance check at the body's exit uses both components together. *[CP011 §§7.5, IN-19]*

### 3.9 Anticipated Refinement Under Typed Failures

The CP011 lattice is type-agnostic: a `failing(M)` state names the failure mode but not the failure type. Under typed failures (§4 below), the lattice can be refined to track *which set of tags* may be propagating at each program point. *[CP012 §9]*

The refinement direction: each `failing(M)` state carries an associated failure-set component `T`, becoming `failing(M, T)`; each `mixed(M)` becomes `mixed(M, T)`; `clear` and `unreachable` are unchanged. The join refines by taking the mode-join of `M` and the set-union of `T`. Transfer functions refine analogously: a call to a command with declared failure set `T` produces an outgoing state with that set on the failure edge; `.fail Tag` produces a state with set `{Tag}`; `|: Tag` consumes failures whose tags are at-or-below `Tag` (with the propagating set narrowing accordingly, per §4.10's punt-or-not choice); bare `|` consumes the entire propagating set. *[CP012 §9]*

The refinement is compatible with the CP011 lattice: dropping the failure-set component recovers the CP011 lattice exactly. Implementations may begin with the CP011 lattice and add the failure-set component incrementally. The exhaustiveness analysis for `|`-with-spec chains, deferred in CP011 §4.8 due to the absence of typed failures, becomes feasible under the refinement.

Full specification of the refined lattice and its transfer functions is the subject of the typechecker-implementation thread. The reference here records the direction; details are not pinned down. *[CP012 §9; specification deferred per CP012's recommendation.]*

*Sources for §3: CP011 §§2–8, 10.2, IN-19–IN-22; CP012 §9; CP004 §3 (init analysis, integrated by reference).*

---

## 4. Typed Failures

### 4.1 The Forest, Not the Tree

Failure types are **hierarchical tags** arranged in user-declared **forests** — a collection of independent hierarchies, each rooted at a user-declared root tag. There is no language-imposed Top tag; no universal ancestor analogous to Java's `Exception`. Each module declares its own roots; hierarchies do not span modules implicitly. *[CP012 §1.1, §2.1]*

The reasoning behind no-Top: a universal ancestor invites code to catch-and-bind generically, which is precisely the practice that makes failure-handling sloppy in languages that have one. Bare `|` already provides catch-anything-without-binding, which suffices for clean-up-and-propagate patterns. Code that wants to *bind* a payload must commit to which hierarchies it understands. The forest structure also preserves the language's hexagonal-architecture-friendly properties: nominal identity, no implicit shared state across modules, no hidden dependencies among unrelated subsystems. *[CP012 §2.1]*

### 4.2 Tag Declaration Semantics (Surface Syntax Open)

A failure tag declaration introduces a name into the surrounding scope, fixes the tag's position in some hierarchy (either as a new root or as a child of a named parent), and optionally binds the tag to a class as its payload contract. Declarations may be syntactically nested (parent tag's declaration containing children) or flat (each child naming its parent); the choice is a surface-syntax matter. *[CP012 §2.2]*

The surface syntax for tag declarations is not pinned down. *Open question: OQ-26.2.* See §7.

### 4.3 Class-Bound Payloads

Every tag-with-payload binds to a class. **The class is a Haskell-style typeclass — a constraint on types asserting the availability of a set of operations, not a concrete type with a layout.** *[CP012 §1.2, §3.1]*

The class has no size, no `sizeof`, no allocation footprint. Instances of types satisfying the class are the things that have layouts; the bound class on a tag is the *contract* — the operations a recovery handler may invoke on the bound value, and the only operations it may invoke. The concrete type of any particular payload value is per-`.fail`-site detail not visible to consumers. *[CP012 §3.1]*

The corollary: different `.fail` sites for the same tag may pass values of different concrete types, all satisfying the bound class. Refactoring the concrete type at one `.fail` site (changing it to a different type that also satisfies the class) is non-breaking; refactoring the class (changing its operations) is breaking. *[CP012 §1.3, §3.1]*

The class binding to tags is therefore a refinement of the language's class machinery applied to failure values: the same dictionary-passing mechanism the language uses for ordinary class dispatch is sliced through the failure machinery. The class witness travels with the propagating failure (see §5); the consumer dispatches class operations through that witness without ever knowing the concrete type. *[CP012 §3.1, §6.5]*

### 4.4 Tags Without Payloads

A tag may be declared without a payload class. In this case `.fail` for that tag carries no value, and a recovery-spec consumer for that tag binds nothing. The tag retains nominal identity, hierarchy position, and full participation in set inclusion. *[CP012 §3.2]*

A single hierarchy may freely mix payload-bearing and payload-less tags. The typechecker enforces the appropriate `.fail` and recovery-spec shapes per-tag; nothing in the hierarchy mechanism requires uniformity.

### 4.5 The `.fail` Site

`.fail Tag: ‹value›` — using the placeholder syntax pending OQ-26.2 — requires that `Tag` is a declared failure tag in scope, and that if `Tag` has a bound class `C`, the `‹value›` expression has a type satisfying `C`. If `Tag` has no bound class, `‹value›` is grammatically absent. The post-`.fail` lattice state is `failing(!)` carrying the propagating set `{Tag}`. *[CP012 §3.3]*

`Tag` must be in the failure set declared by the enclosing command's signature, subject to the resolution of OQ-26.1 (implicit failure sets — see §7). *[CP012 §3.3]*

### 4.6 The Recovery-Spec Site

`|: Tag t ‹body›` — placeholder syntax — engages on a propagating failure whose tag is at-or-below `Tag` in the relevant hierarchy. Failures whose tags are not at-or-below `Tag` continue propagating per §2.4 above. If `Tag` has a bound class `C`, the identifier `t` is introduced into the body's scope at type "any value satisfying `C`"; concretely, `t` is usable wherever `C`'s operations are required. If `Tag` has no bound class, `t` is grammatically absent. *[CP012 §3.4]*

When the block engages and binds `t`, the failure's status as in-flight ends at that moment of binding. *[CP012 §3.4, §6.4]* The handler body executes from no-active-failure. The mechanics of how the value reaches the recovery body — the question of what physically happens to the payload at binding — is the subject of §5. The user-facing semantics is that `t` is in scope throughout the handler body and may be used (including as a payload in a re-fail) until that body exits.

### 4.7 Failure Sets and Structural Set Inclusion

A **failure set** is a set of failure tags. A command signature's failure-set component is the set of tags the command may emit — equivalently, the union of tags appearing at `.fail` sites in the body, including those produced by called commands' propagating failures. *[CP012 §4.1]*

A set may be expressed as an explicit list of tags or as the closure-at-or-below of a higher-level tag. By convention, a set listing tag `T` denotes "any tag at-or-below `T` in `T`'s hierarchy"; a set listing several tags denotes the union of their respective at-or-below closures.

A set `A` **subsumes** a set `B` (notation: `B ⊆ A`) iff for every tag `t` in `B`, there exists a tag `t'` in `A` such that `t` is at-or-below `t'` in `t'`'s hierarchy. Tags in `B` whose hierarchies have no representative in `A` violate the subsumption. *[CP012 §4.2]*

This is set-theoretic inclusion lifted to the hierarchy. It is reflexive, transitive, and antisymmetric (modulo at-or-below equivalence within a single hierarchy, which is trivial). Tags from different hierarchies are mutually incomparable; nothing in one hierarchy subsumes anything in another. *[CP012 §2.3, §4.2]*

A caller invoking a command with declared failure set `D` is responsible for ensuring its own enclosing signature's set `E` satisfies `D ⊆ E` (modulo any sets consumed by recovery within the caller). A caller may *widen* (`E` a superset of `D`) at the cost of precision; a caller may *not* narrow at a call site. *[CP012 §4.3]*

A `|`-block consumes failures of certain tags; after consumption, the propagating set narrows. A bare `|` consumes the entire set (post-`|` propagating set is empty, modulo failures fired *by* the handler body itself). A `|: Tag` consumes failures whose tag is at-or-below `Tag`; the post-block propagating set is the original set minus the at-or-below closure of `Tag`. *[CP012 §4.4]*

### 4.8 Signature Surface

The primary signature surface for failure sets is an inline list of tag names, surface syntax open per OQ-26.2. *[CP012 §5.1]*

A module may declare named aliases for failure sets — `failureset MyFailures = {Net, Parse.Malformed}` in placeholder syntax — and then reference the alias by name in command signatures. Aliases are pure sugar over the inline form; subsumption operates on the expanded set. The motivation is refactoring economy: when several commands share a failure profile, the alias allows changes to that profile to propagate by editing the alias declaration rather than every command's signature. *[CP012 §5.2]*

A higher-order command — one taking a callable as parameter — currently has no surface form to express "my failure set is the union of my callees' failure sets, plus whatever I add directly." Without such a form, every higher-order command must list every failure its callees might emit, with the same combinatorial pressure that plagues Java's `throws`. The propagation form is desirable but not designed; the design space is shared with the fexpr-signature thread (refined OQ-7). *Open question: OQ-26.4.* See §7. *[CP012 §5.3]*

### 4.9 Body Conformance Under Typed Failures

The CP011 conformance rules sharpen under typed failures. *[CP012 §4.5]*

A `:`-marked body's exit edges must have empty propagating set. This is implied by the CP011 rule that exits be `clear`; under typed failures, "empty propagating set" is the typed analog.

A `?`-marked body's exit edges' union of propagating sets must be a subset of the body's declared failure set. This is the new dimension: the failure-set declaration is enforced at conformance check.

A `!`-marked body's exit edges must have non-empty propagating set, and the union must be a subset of the body's declared failure set. The reachability rider remains.

The CP011 mode-conformance rules and the new failure-set-conformance rules compose by intersection: both must hold.

### 4.10 Recovery Narrowing — The Punt

For initial implementation, the typechecker may treat `|: Tag` as not narrowing the propagating set — the post-block set is the same as the pre-block set. This is conservative (it overstates the set, never understates) and acceptable for first-cut soundness. Refinement to track narrowing precisely — post-block set = pre-block set minus at-or-below closure of `Tag` — is a typechecker-implementation matter and may be added as an optimization once the basic mechanism is in place. *[CP012 §4.6]*

*Open question: OQ-26.3.* See §7.

### 4.11 Cross-Module Hierarchy Extension

A downstream module cannot extend the hierarchy of an imported tag — it cannot add new children to a foreign tag. The set of tags at-or-below a given tag is fixed at the tag's declaration site. *[CP012 §2.4]*

This prevents a class of cross-module surprises: a `|: SomeRootTag` handler in module `A` cannot be silently widened by module `B` adding new descendants. The closure semantics are stable across imports.

The closure-by-current-decision is intentional but provisional. *Open question: OQ-28 — whether to permit cross-module hierarchy extension under controlled conditions, e.g., explicit "open hierarchy" declarations, or whether to retain the closed rule indefinitely.* See §7. *[Reconciliation: CP012 §2.4 asserts closure as a flat decision; the post-CP012 dialog flagged it as worth revisiting and registered OQ-28 to capture that. The reference reflects "closed by current decision; see OQ-28 for revisit."]*

### 4.12 Class Dispatch and Failure Sets

A class method's failure set must be uniform across all instances. At a class-dispatched call, the typechecker knows the class and the method but not the instance; therefore the failure set must be a property of the (class, method) pair and not of the instance. *[CP012 §8]*

A class declaration that includes a `?`- or `!`-marked method must declare that method's failure set at the class level (or arrange for the failure set to be a class-level property by some other mechanism). An instance's implementation may emit any subset of the class-declared failure set; emitting tags outside the class-declared set is a static error. The class's declared set is what callers observe.

A class with multiple `?`- or `!`-marked methods may declare a separate failure set per method, or share an alias across methods — both are pure surface conveniences.

This composes cleanly with §§4.7–4.8. The remaining detail is the surface syntax for class-level failure-set declarations, which is part of the broader signature-surface question (OQ-26.2).

*Sources for §4: CP012 §§1.1–1.5, 2, 3, 4, 5, 8.*

---

## 5. Payload Lifecycle Implementation

This section is the most error-prone part of the failure-system specification and is the one most worth reading slowly. The post-CP012 dialog corrected an earlier draft's at-stack discipline; this section reflects the corrected understanding throughout.

### 5.1 The Slot

Each command frame carries a fixed-size **failure slot** holding three words on a 64-bit target: *[CP012 §1.6, §6.1]*

- A **tag identifier**: a small integer (32 bits comfortable, with spare bits available for occupancy or other small flags).
- A **payload pointer**: a pointer-sized value, pointing to the payload value (or null for payload-less tags).
- A **class witness**: a pointer-sized value (typically the address of the typeclass dictionary for the payload's concrete type with respect to the bound class), or null for payload-less tags.

The slot is part of the activation record, allocated at frame entry, occupied only when a failure is in flight. **The slot is identity-of-failure, not contents-of-failure.** Propagation copies the slot's three words; it does not copy or move the payload value itself.

### 5.2 The Originating Frame

When `.fail Tag: ‹value›` fires: *[CP012 §6.2]*

1. `‹value›` is evaluated normally. Evaluation may itself fail; in that case the new failure replaces the intended one (the `.fail` does not fire, the evaluation's failure propagates).
2. The evaluated value is written into a payload-storage area in the current frame. This frame becomes the **originating frame** for this failure.
3. The current frame's slot is populated with the tag, a pointer to the payload-storage area, and the witness for the value's concrete type with respect to the tag's bound class.
4. The frame's normal exit is suppressed in favor of the failure-exit path; the failure-state lattice transitions from `clear` to `failing(!)` with propagating set `{Tag}`.

The payload-storage area's size in any given frame is bounded by the maximum payload size *for that frame's `.fail` sites* — not by the full failure set the frame's command may emit. A frame with two `.fail` sites needs storage for the larger of the two; a frame that propagates failures from called commands needs no payload storage of its own (the payload lives in the callee's originating frame, which is somewhere down the call stack).

### 5.3 Propagation by Slot-Copy

A failure propagating from a callee to a caller does **not** copy or move the payload. The callee's slot is propagated by copying its three words (tag, pointer, witness) into the caller's slot. The pointer continues to reference the payload value at its current home; that home does not change during propagation. *[CP012 §6.3]*

This is the structural reason the slot is fixed-size and the payload is not. The slot communicates *what is failing* up the stack; the payload — *what the failure carries* — stays where it was created.

### 5.4 Witness Construction

The class witness in the slot is the typeclass dictionary corresponding to (concrete-type-of-payload, bound-class-of-tag). At the `.fail` site, the typechecker has both: the concrete type of the evaluated `‹value›` expression, and the bound class from the tag's declaration. The dictionary is selected at compile time and emitted as a witness pointer; no runtime witness construction is required, no lookup at the consumer site. *[CP012 §6.5]*

The consumer side, at a `|: Tag t` block, uses the witness to dispatch class operations on `t`. The witness's identity — which concrete type's dictionary it is — is opaque to the consumer, which sees only the class's operations.

### 5.5 The Holding-Frame Model

The payload value lives in a particular frame at any given time — its **holding frame**. Across the lifetime of a single payload value, the holding frame can change; it is not invariant.

Initially, the holding frame is the originating frame (per §5.2). Propagation up the stack does not change the holding frame: the slot is copied, but the payload remains where it was. The holding frame may, however, change at two events: **binding** (when a `|`-with-spec engages and binds the payload — see §5.6) and **re-fail** (when a recovery handler fires a new `.fail` whose payload is the bound value — see §5.9). Both events are *moves*: the payload value's identity is preserved, but its holding frame changes.

The model matters because the at-stack handler discipline (§5.8) keys on the value's actual lifetime end, not on intermediate moves. Reasoning about at-stack timing requires reasoning about which frame holds the value at any given moment. *[Reconciliation: the "holding-frame" terminology is post-CP012 and not literally used in CP012 §6, which describes the same semantics in flatter language. The post-CP012 dialog adopted this term to make the at-stack reasoning tractable. See §5.8 for the discipline this terminology supports.]*

### 5.6 Binding-as-Move

When a `|`-with-spec engages and binds the failure value, the payload moves from the originating frame's payload-storage area into the recovery (handler) frame's binding slot. This is interpretation **I-a** of OQ-26.5: bound names from a recovered failure are copied into the handler frame at binding. *[Reconciliation: CP012 §11.1 lists OQ-26.5 as open, with I-a and I-b sketched as alternatives in CP012 §6.7. The post-CP012 dialog resolved OQ-26.5 in favor of I-a, per the user's commit, on the grounds that the move-at-binding model makes the at-stack discipline (§5.8) coherent under re-fail. CP012 §6.4 and §6.6 as written use language consistent with a different reading; this reference uses I-a throughout.]*

At the moment of binding:

1. The payload value moves from the originating frame's payload-storage area into the recovery frame.
2. The originating frame's payload-storage area becomes available for reclamation.
3. The originating frame's *slot* is no longer relevant — the failure is no longer in flight.
4. The handler body begins execution from no-active-failure, with the bound name in scope referring to the value at its new holding frame (the recovery frame).

The value's identity is preserved across the move; only its location changes. The class witness travels with the value (it is still the dictionary for the same concrete type with respect to the bound class). The bound name `t` in the handler body is a name for the value at its new home.

### 5.7 Consumption Ends the Failure-in-Flight; the Value Survives

"Consumption" and "end of value lifetime" are distinct events. Consumption happens at the moment of binding (§5.6); it ends the failure's status as in-flight, frees the originating frame's slot, and unblocks the handler body's execution. The *value*, however, survives the consumption: it has moved to the recovery frame and may be referenced by the bound name throughout the handler body's execution.

The value's lifetime continues until one of the following events: *[Reconciliation: the explicit list of value-lifetime endings is a synthesis of CP012 §6.4, §6.6, and the post-CP012 at-stack correction. CP012 §6.6 as written says the at-stack handler fires at originating-frame retirement; the corrected discipline ties at-stack firing to the value's actual lifetime end, which §5.8 below specifies precisely.]*

- The handler body completes without re-failing using the bound value: the value's lifetime ends at handler-body-exit.
- The handler body re-fails using the bound value (or a value derived from it): the value moves again (§5.9), and the new holding frame's lifetime governs.
- The program terminates while the value is still held somewhere: the value's lifetime ends at program termination.

The distinction between consumption and end-of-lifetime is the load-bearing one. Conflating them is what produces the at-stack-fires-at-binding bug; keeping them distinct is what makes re-fail semantics coherent.

### 5.8 At-Stack Handler Discipline (Corrected)

The payload value, like any other Basis value, may have an at-stack handler. **The handler fires when the value's actual lifetime ends — at handler-body-exit if the body completes without re-failing, at the eventual final consumption of the chain if the body re-fails, or at program termination otherwise. The handler does not fire at intermediate moves, including at binding.** *[Reconciliation: CP012 §6.4 reads "[at consumption] its at-stack handlers fire on the normal schedule" and §6.6 reads "the handler fires once, at the moment the originating frame retires (consumption-at-binding, or termination)." Both readings, taken literally, place the handler firing at binding. The post-CP012 dialog identified this as a bug via the re-fail counterexample: if the at-stack handler fires at binding, it may mutate the payload before the body has a chance to re-fail using it. The corrected discipline, recorded here, ties at-stack firing to the value's actual lifetime end, which is at-or-after handler-body-exit. The originating frame's payload-storage reclamation can still happen at binding (per the move-at-binding model of §5.6); what changes under the correction is that the at-stack handler is associated with the value, not the originating frame, and fires when the value actually ends its life.]*

The motivating counterexample, in concrete terms: suppose a payload value's class includes a `cleanup` operation registered as the at-stack handler — for instance, releasing an underlying resource. A recovery handler binds the value, inspects it, and decides to re-fail using it (passing the bound value forward as the payload of a new tag). If `cleanup` had fired at binding, the bound value would be in a post-cleanup state at the moment the handler tries to re-fail it; the re-fail's payload would be a corpse. The corrected discipline avoids this: `cleanup` fires at the actual end of the value's life, which is post-handler-body-exit (or post-final-consumption-of-the-re-fail-chain), so the value remains usable throughout the handler body and through any re-fail that propagates it.

The corrected discipline composes cleanly with the rest of the language's at-stack rules: the general principle remains "the at-stack handler fires when the value's home retires." Under typed failures, the home is the holding frame, and the holding frame retires when no further moves are pending.

### 5.9 Re-Fail-as-Move

When a recovery handler binds a failure value and subsequently fires `.fail` from its body, this is an ordinary `.fail` originating from the handler's frame. There is no re-fail-specific machinery; the language sees a fresh failure with a fresh originating frame. *[CP012 §1.9, §6.7]*

If the new `.fail`'s payload value is `t` (the bound name from the preceding `|`-with-spec), the value moves again: from the recovery frame (where it was held after binding) into the new originating frame's payload-storage area. The new originating frame is the handler's frame; the new propagating slot carries the new tag (whatever tag the re-fail names), the new pointer (to the payload's new home), and the witness for `t`'s class with respect to the new tag's bound class. *[Reconciliation: the re-fail-as-move framing is the post-CP012 dialog's terminology. CP012 §6.7 describes the same scenario but in language that did not yet distinguish move-from-origin from move-from-recovery; the corrected framing makes the chain explicit.]*

The witness may need to change at re-fail. If the original tag's bound class and the new tag's bound class differ, the typechecker requires that `t`'s concrete type satisfies *both* classes, and the new witness is the dictionary for `t`'s concrete type with respect to the *new* tag's bound class. The witness is selected at the re-fail site, at compile time. (If `t`'s concrete type does not satisfy the new tag's class, the re-fail is a static error at the re-fail site.)

The bound name `t` ceases to refer to anything reachable after the re-fail; the value has moved into the new originating frame and is no longer visible at its old name. The handler body's execution continues only along the failure-skip path of the new `.fail` (which, as a `!`-call, has no success edge), reaching the body's exit on the failure path.

The chain may continue: if the new failure is caught by a further recovery handler that binds and re-fails again, the value moves again, and the at-stack handler's actual firing waits for the eventual final consumption of the chain. *[Reconciliation: the "eventual final consumption of the chain" formulation is post-CP012; the chain semantics are implied by CP012 §6.7 but not explicitly worked out there.]*

### 5.10 Frame Retirement on the Failure Path (Deferred)

A frame that has fired a failure does not retire on the normal "command returned" schedule. It enters a **deferred-retirement state**: still allocated, payload-storage area still valid, but no longer executing. The deferred-retirement state ends at consumption (when a `|`-block somewhere up the stack engages and the value moves out — §5.6) or at program termination. *[CP012 §6.4]*

After consumption, the originating frame's payload-storage area is reclaimable (the value has moved). Other resources held by the frame — local values, registrations not yet retired — are released at the same moment, per the normal frame-retirement schedule. The single difference between deferred retirement and ordinary retirement is the timing: deferred retirement waits for consumption; ordinary retirement happens at command-returned. The mechanism is the same.

The at-stack handler on the *payload value*, however, is not part of the frame's retirement (per the correction in §5.8). It travels with the value, fires at the value's actual lifetime end, and is independent of when the originating frame retires.

### 5.11 At Most One In-Flight Failure Per Thread

The Basis runtime maintains the invariant that a thread has at most one in-flight failure at any moment. As a consequence, at most one deferred-retirement frame exists per thread: the originating frame of the in-flight failure. Multi-thread Basis programs may have one per thread; this is consistent with thread-local activation records. *[CP012 §6.4]*

The single-in-flight invariant is what permits the simple slot-per-frame representation (§5.1) to suffice; if multiple in-flight failures could coexist, the slot would need to be replaced by a per-failure structure with its own discriminator. The invariant is preserved by the language's semantics: `.fail` is only valid when no failure is in flight at the firing point (a propagating failure would have failure-skipped past the `.fail` site), and `|`-blocks consume the in-flight failure before any new one can be fired from the handler body.

*Sources for §5: CP012 §§1.6–1.9, 6.1–6.7. Post-CP012 corrections: at-stack discipline (§5.8), holding-frame terminology (§5.5), OQ-26.5 resolution (§5.6), re-fail-as-move framing (§5.9). All reconciliation markers as inline.*

---

## 6. Top-Level Handling

### 6.1 What's Settled

Failures may propagate up to the body of `.program` or `.test`; nothing propagates further. These directives are the lattice's top frame. The bodies of `.program` and `.test` admit normal recovery constructs (`|`, `|`-with-spec, `?`, `?-`, `?:`, `^`, `%`, `??`, `@`, `@!`) per §2 above; a failure propagating within such a body can be caught by recovery siblings at the body's level (with or without enclosing `%`-block organization). *[CP012 §7.1]*

The deferred-retirement rule from §5.10 composes naturally with whatever terminal rule is chosen for unhandled-at-top-level failures: the originating frame retires as part of program-end teardown, with at-stack handlers on the *frame's* registered values firing as part of that teardown. The at-stack handler on the *payload value* (per the corrected discipline of §5.8) fires when the value's lifetime ends; if the failure is unhandled at termination, that ending is at termination itself.

The OQ-27 question is uniquely a top-level question because the rest of the language requires every failure to be either consumed by a recovery construct or propagated to a frame that *will* recover it (modulo conformance rules that statically prevent unhandled-at-`:`-frame-exit). The top-level frame has no parent to propagate to; the rule has to terminate somewhere, and that somewhere is `.program`/`.test`. *[CP012 §7.3]*

### 6.2 What's Open as OQ-27

The precise terminal-handling semantics for failures that escape the entire `.program` or `.test` body is not resolved. *Open question: OQ-27.* See §7. *[CP012 §7.2, §11.2]*

The candidate rules from CP012 §11.2 are summarized in §7 below; the choice has implications for diagnostic reporting, test-framework integration, and the standard library's failure-class hierarchy. Resolution is downstream of standard-library design (specifically, of what operations the standard failure-payload class hierarchy mandates — particularly diagnostic rendering).

*Sources for §6: CP012 §§7, 11.2.*

---

## 7. Open Questions

This section catalogs the failure-system's open questions. Each entry summarizes the question, references its source checkpoints, and notes any current tentative direction.

### 7.1 OQ-26.1 — Implicit Failure Sets

When a command signature has a `?` or `!` mark but no explicit failure set, what does the typechecker assume? Candidates: implicit-empty (restrictive), implicit-any (permissive but defeats static checking), inferred-from-body (convenient but interacts badly with separate compilation), required-explicit (strictest, most clearly sound). *[CP012 §5.4, §11.1]*

Tentative direction: **required-explicit at module boundaries, with intra-module inference permitted** — a callable defined and used within a single module need not annotate its failure set, but anything exported must do so. Final resolution is left to the typechecker-implementation thread.

### 7.2 OQ-26.2 — Surface Syntax

Surface syntax for tag declarations, failure-set aliases, signature failure-set positions, `.fail` with payload, and the recovery-spec form. These are several related grammar questions sharing design constraints; they should be settled together. The current grammar's recovery-spec form is acknowledged as requiring alteration. *[CP012 §11.1, §10]*

### 7.3 OQ-26.3 — Recovery Narrowing

Whether `|: Tag` narrows the propagating set precisely (set minus at-or-below closure) or conservatively (no narrowing). The simple precise form is implementable; the conservative form is initially acceptable. Choice can be deferred to typechecker implementation. *[CP012 §4.6, §11.1]*

### 7.4 OQ-26.4 — Higher-Order Propagation Form

A surface for "my failure set is the union of my callees', plus what I add directly," paralleling the fexpr-signature thread. Should be designed jointly with any future refinement of OQ-7. *[CP012 §5.3, §11.1]*

### 7.5 OQ-27 — Top-Level Failure Handling

Precise terminal-handling semantics for failures that escape the entire `.program` or `.test` body. Candidate rules: *[CP012 §11.2]*

- **Strict termination**: the program (or test) terminates immediately on the unhandled failure; the originating frame retires as part of termination teardown; the payload class's standard rendering produces diagnostic output. For `.test`, this counts as a test failure.
- **Implicit top-level recovery**: the runtime provides an implicit `|` at the top of every `.program`/`.test`, catching anything unhandled, running a standard rendering, and terminating. Structurally equivalent to strict termination but framed as recovery.
- **Expected-failure declarations for `.test`**: a `.test` block intended to verify a specific failure may declare that failure as expected; reaching the top with that failure is a *pass*. Test-framework territory.
- **`.program`/`.test` failure-set declarations**: allow these directives to declare the failure set they are willing to terminate on, as partial static documentation. Probably overkill for `.program`; potentially useful for `.test`.

Resolution is independent of OQ-26 in design but downstream in dependency: OQ-27 cannot be fully resolved until OQ-26's class-binding mechanism is in use, and should be coordinated with whatever standard-library design pins down the failure-payload class hierarchy.

### 7.6 OQ-28 — Cross-Module Hierarchy Extension (New, Post-CP012)

Whether to permit cross-module hierarchy extension — a downstream module adding new children to a foreign tag — under controlled conditions (e.g., explicit "open hierarchy" declarations on the originating tag), or to retain CP012 §2.4's closed-hierarchy rule indefinitely.

The closed rule prevents cross-module surprises: a `|: SomeRootTag` handler in module `A` cannot be silently widened by module `B` adding new descendants. The motivation for revisiting the rule is the symmetric concern: legitimate cases (e.g., a library exposing an extensible hierarchy of error tags for downstream classification) are forbidden by the closed rule. The right balance is not yet settled. *[Reconciliation: OQ-28 is new in this reference. It corresponds to the post-CP012 dialog's recognition that CP012 §2.4's flat closure is provisional. CP012 itself does not register this as a sub-question; the reference does so to make the provisional status explicit.]*

### 7.7 OQ-1 in Constrained Form

The general union-discriminator question (OQ-1, from CP001) — how a discriminator is represented and where it lives for value-like byte-overlay unions — remains open. The failure-tag discriminator subcase is settled by the slot's tag-identifier component (§5.1): tags are small integers, slot-sized, and serve as discriminators for failure values uniformly. *[CP012 §11, OQ-1 entry]*

The settled subcase does not constrain the general question. Other union-bearing positions (in particular, value-like byte-overlay unions in record fields) may use different discriminator strategies; the failure-system's choice is local to the failure machinery.

---

## 8. Provenance

**Authored:** Distilled by Claude (Opus 4.7) from CP002, CP003, CP011, and CP012, with post-CP012 corrections from the OQ-26 dialog of 2026-04-29.

**Source materials:** intent-checkpoint-002.md, intent-checkpoint-003.md, intent-checkpoint-011.md, intent-checkpoint-012.md. Secondary references consulted for cross-cutting context: CP004 §3 (initialization analysis, integrated by reference in §3.8), CP008 §10 / FI-1 (region-style cleanup, background for at-stack-handler timing), CP001 (failure-related glossary entries). Other checkpoints (CP005–CP010) consulted only as needed for terminology; nothing in those checkpoints is load-bearing for the failure system.

**Post-CP012 corrections recorded as reconciliation markers:** at-stack handler timing (§5.8), holding-frame terminology (§5.5), OQ-26.5 resolution in favor of I-a (§5.6), re-fail-as-move framing (§5.9), OQ-28 registered as new (§4.11, §7.6). All marked inline at point of use.

**Recommended next step:** Review section-by-section with the user. Revisions in place. After this reference is settled, the next topic-organized reference per the agreed split is **Operational Semantics and Block Markers** (foundational, cited by most others); the full sequence is in the task handoff document. Each consolidation is its own focused thread; do not start the next without explicit direction.
