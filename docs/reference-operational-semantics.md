# Basis Language — Reference: Operational Semantics and Block Markers

**Status:** Topic-organized authoritative reference for the operational semantics of Basis and the structural/operational behavior of the eleven block markers. Consolidates material from CP001 (foundational principles, execution model, command shape), CP002 (block markers, lifecycle), CP003 (compound guards), CP004 (parameter-mode mechanics), CP005 (marker syntax, receiver modes), and CP007 (provision chain rule, brace-quote surface distinction). Cross-references the failure-system reference for failure-flow material rather than duplicating it; defers type-system, construction, lambda/fexpr, and class-system material to their respective planned references.
**Date:** 2026-04-29
**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, in continuation of the topic-organized consolidation begun with `reference-failure-system.md`.

**Authority statement.** Where this reference differs from the source checkpoints on a covered topic, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative.

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]*; section-level connective tissue carries footer citations. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints or resolves an ambiguity. **Cross-references** to the failure-system reference take the form *[failure-system §x]* and point to where failure-related aspects of the same topic are treated; **forward references** to planned but undrafted references are flagged as such.

---

## 1. The Execution Model

### 1.1 Commands as the Unit of Execution

Basis has no statement-versus-function dichotomy. The single unit of execution is the **command**: a named, parameterized operation that runs to one of two outcomes. Commands take parameters, may produce output through writeable parameters, may fail with a failure value, and compose. There are no statements and no functions; there are commands. *[CP001 §1, §3]*

This is more than a terminological choice. The collapse of statement/function into command means that everything that *does* something is in the same syntactic and semantic category, and the distinctions other languages make between "expressions that evaluate to values" and "statements that produce effects" do not exist in Basis. There are commands and there are effects; there is nothing else at the level of execution. *[CP001 §1.3]*

### 1.2 Success and Failure as the Two Outcomes

Every command terminates in exactly one of two outcomes: success or failure. There is no third category, no "void" return, no "exception" parallel to the success/failure axis. A command that produces no observable effect on success still produces a success outcome; a command that fails carries a failure value. *[CP001 §1]*

The failure-system reference covers failure modes (the `:` / `?` / `!` marks declared in command signatures), the structural rules for failure propagation, the static analysis that verifies bodies conform to their declared marks, typed failures, and the payload lifecycle. *[failure-system §§1, 3, 4, 5]* For the rest of this reference, failure flow appears as a peer to success flow at the operational level — both are first-class outcomes of every command — without re-deriving the failure-side material.

### 1.3 Single-Shot Semantics; Icon at the Combinator Level Only

Each command call is **single-shot**: it produces exactly one outcome, with no notion of "the next value" to demand. There is no backtracking, no resumption, no multi-valued evaluation. A command that runs runs once; either it succeeds or it fails, and that outcome is final for that call. *[CP002 §1.1]*

Basis's Icon influence shows up at the *combinator* level — the `?`-family block markers compose single-shot outcomes in expressive ways — but not at the *call* level. Constructs may internally re-invoke the same command-typed value (the rewind block `^` re-invokes its preceding sibling on each iteration), but this is structural recurrence, not generator resumption. The language has no generators, no coroutines, no continuation-passing semantics. *[CP002 §1.1]*

### 1.4 Expressions as Syntactic Sugar

There are no true expressions in Basis. Constructs that *look* like expressions in surface syntax — for instance, the right-hand side of `# r <- (...)` — are syntactic sugaring over command sequences. The semantic model has no evaluation-to-a-value step: every command succeeds or fails; results are the *effects* of command execution (in particular, writeable-parameter mutations); there is no expression-yielding-a-value primitive. *[CP002 §1.3]*

This is a meaningful divergence from Icon, which has expressions yielding values (which Icon then treats as commands when their value is unused). Basis collapses the distinction entirely: there are commands, and there are effects.

The expression-shaped surface forms — `<-` with a call as RHS, parenthesized command invocations in argument position, and so on — desugar into command sequences whose effects produce what the surface form names. The construction-and-initialization reference will cover the desugaring rules in detail; the operational point here is that no expression-level evaluation primitive exists at the bottom of the stack.

### 1.5 The Operational State Tuple ⟨V, Φ, Σ⟩

Basis's operational semantics is given over a state tuple `⟨V, Φ, Σ⟩`, where:

- **V** is the value environment — the mapping from in-scope names to slot identities and values.
- **Φ** is the failure register — when a failure is propagating, Φ carries the failure value (the tag identifier, payload pointer, and class witness, per *[failure-system §5.1]*); when no failure is propagating, Φ is empty.
- **Σ** is the store — the underlying memory in which slots are realized.

The operational reduction rules over this tuple are not duplicated in this reference; they appear in the README and are refined in the operational sections of the relevant checkpoints. The reference's role is to specify the *intent* of the operational rules, not to restate them in formal notation. *[CP001 §1, "Formal operational semantics"]*

The Φ component is the operational handle by which failure flow is realized. Successful execution leaves Φ empty; a failing command populates Φ; recovery contexts consume Φ; propagation copies Φ-carrying state up the call stack. The detailed mechanics — slot-copy propagation, deferred-retirement, holding-frame moves at binding and re-fail — are specified in the failure-system reference. *[failure-system §5]*

*Sources for §1: CP001 §1 (foundational principles); CP002 §1 (Icon influence, single-shot, expressions as sugar); failure-system reference for Φ-handling details.*

---

## 2. Surface Structure

### 2.1 Syntactically Significant Whitespace

Layout is part of the grammar. The lexer's bracket/brace/indent stacks bind tokens to their closing partners, which is what allows the combinator-based parser's `bound` / `boundedGroup` / `exclusiveGroup` constructs to operate. Indentation determines nesting; sibling relationships between blocks at the same indentation level are how composite structures like `?:` chains, `??`-pairings, `|`-cascades, and `?`/`-` if/else are recognized. *[CP001 §1, "Syntactically significant whitespace"]*

The user reading source can determine block structure by inspection of indentation. There is no separate brace-or-keyword block-ender; the indentation level *is* the block-ender. This is a load-bearing property because most of the block-marker composition rules (§4 below) depend on identifying siblings at the same indentation level, and indentation is the language's only carrier of that information.

### 2.2 Brackets, Braces, and Indent Stacks

The lexer maintains three coordinated stacks: one for square brackets `[ ]`, one for curly braces `{ }`, and one for indentation. A token is bound to its opening partner via the stack appropriate to its kind: a closing bracket pops the bracket stack, a closing brace pops the brace stack, and a dedent pops the indentation stack. The parser's combinator constructs (`bound`, `boundedGroup`, `exclusiveGroup`) consume token streams in awareness of this stack structure. *[CP001 §1; structural property of the lexer per the source code.]*

The user-visible consequence is that all three structural features — brackets, braces, indentation — are first-class, and constructs may interleave them naturally. A block at some indentation level may contain a bracket-delimited buffer expression, which may contain a brace-delimited block-quote, which may contain its own indented content; the lexer keeps these straight and the parser sees a well-structured token stream.

### 2.3 The Block, the Sibling, the Indentation Level

A **block** in Basis is a sequence of statements at a common indentation level, introduced by a block-marker token (or the body of a command, which introduces an implicit block) and terminated by a dedent. Within a block, statements are either *siblings* (at the block's own indentation level) or *children* of one of those siblings (at deeper indentation). *[CP002 §2 throughout, structural rules implicit.]*

Sibling-recognition is the key structural primitive. Many of the eleven block-marker constructs derive their meaning from sibling relationships: `?:` chains are recognized purely by adjacency of consecutive `?:` siblings; `|` engages on a propagating failure from preceding siblings at the same indentation level; `??` pairs with a `?`-family block as its first executable inner construct, and a `^` rewinds to its preceding sibling. None of these relationships is marked by an explicit syntactic wrapper — they are read off the indentation structure directly.

### 2.4 Adjacency and Chain Recognition

**Adjacency** means "consecutive siblings at the same indentation level." Constructs whose meaning depends on adjacency:

- A `?:` chain extends as long as consecutive siblings are `?:`-blocks. The first non-`?:` sibling ends the chain (and serves as the chain's default arm). *[CP002 §2.4]*
- A `|`-cascade engages on propagating failures from preceding siblings; the first matching `|` (with-spec or bare) consumes the failure. *[CP002 §§2.9–2.10]*
- A `-` block attaches only to its immediately preceding `?` or `?-` sibling. *[CP002 §2.7]*
- A `^` rewinds to its preceding sibling. *[CP002 §2.6]*

Adjacency is sensitive to indentation: a sibling at a *different* indentation level is in a different block and does not participate. The user controls block structure by indenting, and indentation structure determines which constructs see which adjacent material.

*Sources for §2: CP001 §1; CP002 §§2 (block-marker structural rules implicit throughout); structural details per the lexer/parser source code.*

---

## 3. The Two Foundational Principles

Two principles together shape what code in Basis is *able* to do. Both are stated as commitments the language makes; the operational semantics is structured to enforce them, and the type system tracks what can be inferred from them.

### 3.1 No Hidden Control Flow

Failures propagate, but only along structurally visible paths. There are no exceptions in the C++/Java sense. Indirection through dispatch is explicit (the `::` syntax) or visible at the call site. The programmer can read source code and know what executes when, and can reason about effects from signatures alone. *[CP001 §1, "No hidden control flow"]*

This principle has structural consequences:

- Every failure flow is marked by a block-marker construct in the source. There is no `try`/`catch`/`finally` parallel; recovery is at sibling positions in the source structure (`|`, `|`-with-spec) or at guard positions (`?`, `?-`, `?:`, `^`).
- Class dispatch is syntactically marked at the call site by `::`, so the reader can identify dispatched calls without inspecting types.
- No construct fires implicitly except the at-stack-exit mechanism (§3.2 below).

The principle is what makes signature-based reasoning possible. A reader looking at a `:`-marked command signature knows it returns successfully on every reachable path; a `?`-marked command may fail; a `!`-marked command always fails. The reader does not have to trace through an arbitrary depth of dynamic dispatch and runtime exception flow to determine what may happen — the signature carries the answer. *[CP001 §1; failure-system §1 for the marks themselves.]*

### 3.2 The Single Intentional Exception: At-Stack Mechanism

The at-stack-exit mechanism (`DEF_CMD_RECEIVER_ATSTACK` and `DEF_CMD_RECEIVER_ATSTACK_FAIL`) is the single intentional exception to no-hidden-control-flow. It fires implicitly at object lifetime ceilings — when a frame retires, registered at-stack methods on objects introduced in that frame execute, *as if* an `@` block (for `ATSTACK`) or `@!` block (for `ATSTACK_FAIL`) had been written manually at the object's introduction site. *[CP002 §1.4, §3.2]*

The trade is deliberate: one specific category of implicit invocation, in exchange for ergonomic resource management. The alternative — requiring users to manually register `@`/`@!` blocks for every object with cleanup needs — would be sufficiently awkward as to undermine the language's usability for resource-managing code. The mechanism is local to object construction, predictable from the object's type, and visible at the type level (the object's class declares whether it has at-stack methods).

The operational details of the mechanism — registration, firing schedule, manual versus automatic invocation, override via class instances — are covered in §6 below. The point for the no-hidden-control-flow principle is that this is the *only* implicit invocation; everything else is structurally visible in the source.

### 3.3 No Non-Local State (Reframed via Provision Chain)

The principle is *not* that a command can only access state passed as a parameter. It *is* that a command cannot reach for state that lies off the call stack — module-level mutable globals, ambient singletons, hidden caches, thread-local storage, and the like. State that physically resides on the call stack remains in principle reachable, with the further restriction that such reach must be licensed by an explicit **provision chain**. *[CP007 §1.1; reframes CP001's earlier articulation.]*

State reachable from inside a command body must arrive there by an explicit chain of provision. There are three kinds of provision: *[CP007 §1.2]*

- **Parameters and receivers.** A frame's caller passes state as named arguments at the call site. The state is locally bound to the parameter (or receiver) names within the body.
- **Context parameters.** The Scala-implicit-style mechanism (§5.5 below): a frame's caller has a value of matching type in lexical scope, automatically supplied at the call site. The provision is implicit at the syntactic level but not at the semantic level — the value must actually exist in the caller's lexical scope.
- **Captured-and-traveling state.** A value (such as a fexpr) carries with it references to slots in some frame, established at the moment the value was constructed. When the value is later invoked, those references are part of the value's identity. The provision was made at construction time, not at invocation time. Lambda values capture by-copy and so carry *values* rather than slot references; both fit the provision-chain shape (the lambda's hidden fields are provisioned at construction time). *[CP007 §1.4; lambda/fexpr details deferred to the lambda-and-fexpr reference.]*

Absence of any of these means absence of reach. A frame six levels above the current one is on the call stack, but if no explicit provision chain reaches up to it, its slots are unreachable from the current command. *[CP007 §1.2]*

The principle has the same shape as no-hidden-control-flow: both restrict what a command can do *implicitly*. Control flow must be structurally visible; state reach must be provisionally licensed. Together they make the language signatures-as-documentation work: a command's signature names everything the command can affect (provision-chain) and the marks plus block structure describe everything that can happen control-flow-wise.

The earlier articulation of no-non-local-state — "a command's parameter list is a complete inventory of its potential effects on the universe" — is a *useful description of the typical case*, where the typical case is a regular command whose effects flow exclusively through its parameters. It is not the universal rule; mechanisms exist where a value carries with it a reference to state in a frame above the current one, and those references are themselves a form of provision. The provision-chain rule is the universal rule. *[CP007 §1.1]*

### 3.4 The `.test` and `.program` Exceptions

The principles relax at `.test` and `.program` directives, which are the language's entry points and lie outside command scope. These directives may use language features that ordinary commands cannot — at minimum, they are the only contexts in which top-level failures are terminally consumed *(failure-system §6)*. The precise rules for what `.test` and `.program` may do that ordinary commands cannot have not been worked through in the source checkpoints. *[CP001 §1; CP002 §6 ("Topics Not Yet Covered" — tests and programs).]*

The static checking story for `.test` and `.program` bodies, and the precise relaxations of the no-non-local-state and no-hidden-control-flow principles, are open. The failure-system reference settled the terminal-failure-handling aspect *(failure-system §§6.2–6.3)*; the broader question of what these directives can do remains for future intent dialogs.

*Sources for §3: CP001 §1; CP002 §1.4; CP007 §1.*

---

## 4. Block Markers — Structural and Operational Roles

This section covers the eleven block-marker constructs in their structural and operational behavior. The failure-flow aspects — which construct consumes which failure on what edge, what the lattice transition is at each node, what the conformance implications are — are covered in the failure-system reference *[failure-system §§2, 3.4]*. The material here is the *non-failure* operational behavior plus the structural composition rules. Where a marker has substantive failure-flow behavior, the section here points forward and does not repeat.

### 4.1 The Eleven Markers

The grammar defines eleven block-marker productions: `DO_WHEN` (`?`), `DO_WHEN_MULTI` (`??`), `DO_WHEN_FAIL` (`?-`), `DO_WHEN_SELECT` (`?:`), `DO_ELSE` (`-`), `DO_BLOCK` (`%`), `DO_REWIND` (`^`), `DO_RECOVER` (`|`), `DO_RECOVER_SPEC` (`|`-with-spec), `DO_ON_EXIT` (`@`), and `DO_ON_EXIT_FAIL` (`@!`). The full categorization by failure-handling role appears in the failure-system reference *[failure-system §2.1]*. Here the markers are organized by their structural shapes: which require guards, which engage by sibling adjacency, which pair with other markers, which operate as standalone blocks.

### 4.2 Guard-Bearing Markers: `?`, `?-`, `?:`

The three guard-bearing markers share a structural shape: the first statement of the block is the **guard**, and the remaining statements form the **body**. The construct's behavior is determined by the guard's outcome.

For `?`, the body runs iff the guard succeeds. For `?-`, the body runs iff the guard fails. For `?:`, the body runs iff the guard succeeds; additionally, `?:` participates in chains formed by adjacency with consecutive `?:` siblings. *[CP002 §§2.2–2.4]*

Structurally, a guard may itself be a complex construct. A `%`-block standing in guard position acts as a compound guard: the `%`-body's success-or-failure becomes the guard's outcome. *[CP003 §1.1]* This is how the language expresses logical conjunction in guard position without a dedicated syntax — the `%`-body is a sequence, the sequence succeeds only when every statement in it succeeds. Disjunctions inside the conjunction are written using `|`-chains within the `%`-body. *[CP003 §1.2]*

The detailed failure-flow rules for guard-bearing markers — what state each engaged or skipped path produces, how the failure is consumed, what the post-block lattice state is — appear in *[failure-system §§2.2, 3.4]*.

### 4.3 The Branch: `-`

`-` attaches *only* to a preceding `?` or `?-` block at the same indentation level. It supplies the alternative branch. *[CP002 §2.7]*

The pairing is fixed by adjacency. A `-` whose preceding sibling is not a `?` or `?-` is a static error. `-` does not chain — there is no `? ... - ... -` form for n-ary branching; for that, use `?:`. `-` does not attach to `?:` (which has its own chain-recognition mechanism), to `^`, to `|`, to `%`, to `@`, to `@!`, or to `??` (which has no predecessor branch to alternate with). *[CP002 §§2.7, 2.11]*

The body of `-` is *not* a recovery context; failures within it propagate per the surrounding rules. *[CP002 §2.7]*

### 4.4 The Escape Elevator: `??`

`??` is the language's most unusual block marker. It is a meta-marker: it pairs with a `?`-family block (a `?` or `?-`) as its first executable inner construct, and modifies the structural destination at which the inner block's guard-failure recovery resumes execution. *[CP002 §2.5]*

Structurally:

- A `??`-block must contain a `?` or `?-` as its first executable inner construct. Bare `??` (with no inner guard-bearing block) is a static semantic error. *[CP002 §2.5]*
- The inner construct is generally on the same line as `??` for visual clarity, but layout-wise it need only be the first executable inner item.
- `??` does not combine with `?:` (no meaningful semantics) and does not combine with `%` (which has no failure-escape to elevate). It does pair with both `?` and `?-` (a `?? ?-` chain is well-formed). *[CP002 §2.5]*
- `??` operates at a fixed elevation of one level. There is no `???` or further-elevating construct; if multiple levels of elevation are needed, restructure the code. There is no fused `??-` syntax; a `??` paired with `?-` is written as adjacent markers. *[CP002 §2.5]*

The operational effect on failure flow — that the inner guard's failure resumes at the next sibling of the `??`-block rather than the next sibling of the inner block — is detailed in *[failure-system §§2.5, 3.4]*.

The canonical loop pattern, `?? ?` paired with a sibling `^`, exercises the elevation feature: the inner guard's failure terminates the loop by resuming past the `^`-block, while the `^`-body's failure terminates the loop by falling through normally. The two failure paths reach the same destination via different mechanisms by design. *[CP002 §2.5; failure-system §2.5.]*

### 4.5 The Rewind: `^`

`^` is a sibling block whose body controls whether execution loops back to the preceding sibling. The body executes; on success, control jumps back to the preceding sibling at the same indentation level (loop continues); on failure, the failure is consumed, the body terminates, and control falls through to the statement following the `^`-block (loop terminates). *[CP002 §2.6]*

Structural rules:

- `^` requires a preceding sibling at the same indentation level — *something to rewind to*. A `^` with no preceding sibling is a static semantic error reported at parse or AST-construction time. *[CP011 §10.1, refines CP002 §2.6.]*
- The `^`-body is **optional**. A bodiless `^` is treated as if its body had succeeded — control unconditionally rewinds to the preceding sibling. The bodiless form is the cleanest expression of "loop until the guard fails" idioms and pairs naturally with `?? ?`. *[CP011 §10.1.]*
- `^` is itself a sibling rather than an inner construct. It does not combine with `??` directly (the canonical loop pattern is `?? ?` with a sibling `^`, not `?? ^`). It does not pair with `-`. *[CP002 §2.11]*

The full-body recovery semantics — every body statement's failure is consumed by the construct, the body is a unified test-and-adjust region — is specified in *[failure-system §§2.3, 3.4]*.

### 4.6 External-Failure Recovery: `|` and `|`-with-spec

`|` and `|`-with-spec are external-failure recovery markers: they engage on a propagating failure from preceding siblings at the same indentation level. Structurally, they have no guard — their engagement is triggered by the propagation state at their entry, not by an inner statement. *[CP002 §§2.9–2.10]*

Structural rules:

- The block's incoming engagement-arm is the program point at the same indentation level following the preceding siblings; the incoming non-engagement-arm is the same point on the no-failure path. *[CP002 §2.9]*
- The body is *not* a recovery context — its statements propagate failures normally per the enclosing rules. The block consumes the failure that triggered its engagement, but its body's own failures are subject to the surrounding scope's recovery contexts.
- Cascade structure: each `|` (bare or with-spec) is a recovery destination for preceding `|`-bodies as well. If `|` block N engages and its body itself fails, block N+1 (if present) can catch it. The cascade generalizes — each block is both a recovery destination and a potential recovery source. *[CP002 §2.10]*

The bare form `|` and the with-spec form differ in filtering and binding. Bare `|` engages on any propagating failure and does not bind the failure value. The with-spec form filters by the spec (post-typed-failures, by tag-at-or-below match) and binds the failure value when the spec matches. *[CP002 §2.10; failure-system §2.4 and §4.6 for the typed-failures refinement.]*

The detailed failure-flow rules — which states engage which arm, how filtering interacts with the propagating set, how cascades narrow the propagating set under typed failures — appear in *[failure-system §§2.4, 3.4, 4.10]*.

The deprecated `|:` form once used for `|`-with-spec has been retired in favor of plain `|` with structural distinction; the prior syntax should not appear in current code. *[failure-system §7.2]*

### 4.7 Plain Grouping: `%`

`%` (DO_BLOCK) is a plain grouping block. It has no control-flow semantics on its own, no recovery semantics, no failure-handling effect of its own. Its body executes as ordinary sequential statements. *[CP002 §2.8]*

Uses for `%` are visual organization; explicit scope demarcation; structural clarity in deeply-nested code; and most importantly, **standing in guard position** to act as a compound guard. *[CP003 §1.1]* When `%` stands as the guard of a `?`, `?-`, or `?:` block, the `%`-body's success-or-failure becomes the guard's outcome, giving the language a logical-conjunction primitive without a new construct.

`%` does not combine with `??` (no failure-escape to elevate) and does not pair with `-`. *[CP002 §2.11]*

### 4.8 Frame-Exit Hooks: `@` and `@!`

`@` (DO_ON_EXIT) and `@!` (DO_ON_EXIT_FAIL) register cleanup actions for the frame in which the block is encountered. They run at frame-exit time: `@` on any frame exit (success or failure); `@!` only on failure-exit. *[CP002 §3.6]*

Structurally:

- The blocks contribute *registration* to the frame; the cleanup body is associated with the frame, not with any particular value.
- The cleanup body executes from a synthetic clear state — neither block sees nor consumes the propagating failure if any. Their bodies run as if no failure were active, even when the frame is exiting via failure. *[CP002 §3.6]*
- Internal failures of an `@`-body or `@!`-body propagate within the cleanup flow but do not return to the body's normal CFG; the cleanup machinery handles them per its own rules.
- Neither block combines with `??`, with `-`, or with itself in any structural composition beyond ordinary sibling sequencing.

The full operational behavior — including registration mechanics, the firing schedule, manual versus automatic invocation, override via class instances, and the interaction with object lifetime ceilings — is the subject of §6 below.

### 4.9 Composition Summary

The composition rules from CP002 §2.11, retained as the operational truth and unchanged across the failure-system reference and this one:

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

### 4.10 Indentation and Adjacency Are the Composition Substrate

A recurring theme across the eleven markers: composition is by *adjacency at the same indentation level*. There are no syntactic wrapping constructs for chains, no surrounding brackets for sibling-engagement, no explicit delimiters for branch-attachment. The indentation level is the block boundary; siblings at that level are siblings; markers that engage by sibling relationship engage by being siblings.

The user reading source determines block structure by inspection. The grammar enforces it via the lexer's indent stack and the parser's bound/boundedGroup constructs. The operational semantics consumes the resulting AST tree as the structural truth. There is no structural information at the operational level that is not visible in the source's indentation.

This property is what makes block-marker composition reasoning local: a programmer determining whether a `|` will engage on some failure inspects the source for preceding siblings at the relevant indentation level; a programmer reading a `?:` chain identifies it by the run of consecutive `?:` siblings ending at the first non-`?:` sibling. The structural relationships are mechanical to read.

*Sources for §4: CP002 §2 throughout (operational rules per marker); CP002 §2.11 (composition table); CP003 §1 (compound guards via `%`); CP011 §10.1 (`^`-body-optional refinement); failure-system §§2, 3.4 cross-referenced for failure-flow material.*

---

## 5. Commands and Invocation

### 5.1 Signature Shape

A regular command signature is `cmd-name : parm-list / implicit-parm-list -> result-name`. The result-name is the optional `-> name` clause; the `/` separates regular parameters from implicit (context) parameters; the failure-mode mark (`?` or `!` prefix on the name; absence means `:`) declares the command's failure mode. *[CP001 §3.1]*

A v-command signature has receivers in front: `(recv1, recv2, ...) :: cmd-name : parms / iparms -> result`. The receivers participate in class dispatch (§5.7 below); the rest of the signature has the same shape as a regular command's. *[CP001 §3.1]*

Constructors and destructors use receiver-bearing forms with the `@`/`@!` markers in the signature itself: a constructor signature is the form that binds an at-stack-introduction handler; an at-stack destructor uses `@` or `@!` in the receiver position to declare an at-stack method. The full constructor/destructor signature shapes are detailed in CP005 §§2.3–2.4 and are forwarded to the class-system reference for full treatment. *[CP005 §§2.3–2.4; full coverage deferred to the class-system reference.]*

### 5.2 Parameter Modes (Operational Behavior)

Parameters are classified by mode. The full type-and-mode system — the syntactic markers (`'` apostrophe and `&` ampersand as identifier shapes per *[CP005 §1]*), the type-level rules, the same-scope rule — is the subject of the planned type-and-modes reference. The operational behavior at call boundaries appears here.

**IN parameters** (no marker) are passed by value. The callee receives a copy. Mutations to the callee's local are not visible to the caller. After the call returns (success or failure), the caller's slot is bit-identical to its pre-call state. *[CP001 §3.2]*

**Productive parameters** (`'` marker) are writeable parameters with an additional callee-side obligation: the callee is statically obligated to write the parameter exactly once on every successful return path. The caller may pass either an initialized or an uninitialized slot; the callee will produce the value. *[CP004 §1; CP009 §2 sharpens to write-once.]*

**Reference parameters** (`&` marker) are writeable parameters without the productive obligation: the callee may read, write, or do neither. The caller must pass an already-initialized slot, since the callee may read and reading uninitialized is forbidden. *[CP004 §1]*

Both productive and reference parameters are passed by **call-by-copy-restore**: the callee receives a copy of the caller's slot value (productive parameters' uninitialized inputs are passed as a fresh slot; the operational details depend on the type system and are forwarded to the type-and-modes reference). On *successful* return, the callee's (possibly-modified) copy is written back to the caller's slot. On *failure*, no write-back occurs; the caller's slot is bit-identical to its pre-call state. *[CP001 §3.2; CP004 §1]*

This is the mechanism by which "mutation either succeeds fully or fails fully" is realized. Failure-atomicity falls out of copy-restore for free, with no separate transactional machinery and no rollback code in user programs. *[CP001 §1, "Mutation either succeeds fully or fails fully"]*

The pointer case composes cleanly: the *pointer value* itself is what's copied (IN) or copy-restored (productive/reference), not the pointee. A writeable `^T` lets the callee swap which T the caller's slot points at; a non-writeable `^T` does not (though the callee can still read from and write through the pointed-to storage during the call, subject to type-system constraints). *[CP001 §3.2]*

Implementation latitude: the language semantics commit to call-by-value for IN and call-by-copy-restore for writeable parameters at the *observable* level. The implementation may diverge where divergence is unobservable. *Open question: OQ-2.* See §8. *[CP001 §3.2; CP004; OQ-2 from CP001 §6.]*

### 5.3 The `-> name` Result Designator

A command's *true* return is its success/failure status (with failures carrying their value per *[failure-system §5]*). Commands do not have value-typed return slots in the C/Haskell sense; output flows through writeable parameters. *[CP001 §3.3]*

The `-> name` clause is **syntactic sugar** to make commands usable in expression-shaped position without forcing the caller to introduce named temporaries. When `do: x, y -> r` is in scope and the caller writes `# r <- (do: x, y)`, the desugarer rewrites to:

1. Introduce `r` as a slot of the appropriate type (in the no-defaults model from CP004, this introduces an uninitialized slot).
2. Pass `r` as the writeable parameter selected by `-> name`.
3. Execute the call.
4. On success, copy-restore writes back into `r`. On failure, no restore; `r` retains its pre-call state (uninitialized in the typical fresh-introduction case). *[CP001 §3.3; refined by CP004 §§1, 5.]*

The well-formedness rule is that the name on the right of `->` must be one of the command's parameters or receivers — full stop. It is *not* required to be writeable. *Open question: OQ-3* (the semantic meaning of designating a non-writeable parameter). See §8. *[CP001 §3.3, OQ-3.]*

**Implicit `-> name`:** if a command has no explicit `-> name` clause but has *exactly one* writeable parameter, that parameter is implicitly the expression-position result. Commands with no writeable parameters, or with multiple writeable parameters and no explicit `-> name`, cannot be used in expression-shaped position; the caller must use the explicit invocation form. *[CP001 §3.3]*

### 5.4 Commands as First-Class Values

Commands are values. They can be bound to variables, passed as parameters, stored in fields, captured by lambdas and fexprs, returned from other commands (through writeable parameters via the `-> name` mechanism), and partially applied. The resolution of a class-method or v-command dispatch produces a command-typed value with the receiver(s) baked in. *[CP001 §3.4, §2.7]*

The first-class-ness of commands enables a programmer-visible optimization for tight loops: a class-method dispatch can be hoisted out of the loop by binding the resolved command to a local variable.

```
# logFn <- (logger :: log)        ; one dictionary lookup
... loop ...
   logFn: "iteration " + i        ; direct invocation, no dispatch
... end loop ...
```

The dispatch occurs once when `logFn` is bound; thereafter `logFn` is invoked directly. The compiler may perform this hoisting as an optimization, but the language gives the programmer the explicit lever. The pattern is safe when the receiver doesn't change in a way that affects dispatch over the loop's lifetime; the programmer holds the discipline; the language enables the move. *[CP001 §3.4]*

The full design of partial application beyond receiver-elision is open. *Open question: OQ-6.* The class-system reference will treat partial application in full; the operational point here is that command-typed values exist, are first-class, and support the receiver-elision pattern via `::`. *[CP001 §3.4, OQ-6.]*

### 5.5 Implicit (Context) Parameters

The `/` separator in a command signature divides regular parameters from **implicit context parameters**, which behave like Scala's implicit parameters. At a call site, an implicit parameter is automatically supplied from the caller's lexical scope when *exactly one* in-scope value matches the parameter's type. Ambiguity (multiple in-scope matches) and absence (no match) are both call-site errors that the user can resolve by passing the value explicitly. *[CP001 §3.5]*

This preserves the no-non-local-state principle (§3.3 above): implicit resolution is doing *plumbing*, not *reaching*. The value must actually exist in the caller's lexical scope to be eligible. The principle's invariant — that a command's parameter list is a complete inventory of its dependencies — is preserved at the *signature* level; the implicit mechanism reduces the *syntactic noise at call sites* without weakening the semantic guarantee. *[CP001 §3.5; CP007 §1.2 for the provision-chain framing.]*

The implicit-parameter mechanism is conceptually parallel to but distinct from instance delegation (the class-system mechanism for instances satisfying classes by deferring to a named field): implicits resolve at call sites by uniqueness-of-type; instance delegation resolves at type-class lookup by explicit naming. *[CP001 §§3.5, 4.2]*

The interaction between implicit context parameters and partial application, and the interaction with capture lists in lambda and fexpr forms, are open. *Open questions: OQ-13, OQ-21.* These are forwarded to the class-system and lambda-and-fexpr references respectively.

### 5.6 The Class-Resolution Operator `::`

The expression `(receiver :: name)` resolves the class method `name` on `receiver` and produces a command-typed value with the receiver baked in. The result's type matches the class's declaration of `name`, with the receiver position elided. So if `Loggable :: log: String` is declared, then `(myLogger :: log)` has type `:<String>` — receiver-baked-in, ready to be invoked with just the string argument. *[CP001 §4.5]*

The `::` syntax marks indirection at the source. The reader sees `::` and knows that dispatch is occurring at this position — that the actual command invoked depends on the receiver's type and is resolved through the class's instance machinery. This visibility is what makes class dispatch consistent with the no-hidden-control-flow principle (§3.1 above). *[CP001 §1, "No hidden control flow"]*

The detailed dispatch mechanism — Haskell-style dictionary passing, single-class dispatch composed across v-command receivers, the dictionary as a hidden parameter to polymorphic commands — is forwarded to the class-system reference. The operational point here is that class dispatch *occurs* and is *visible*; the implementation mechanics live in the class-system treatment.

### 5.7 V-Commands and Multiple Dispatch (Operational)

A v-command takes a tuple of receivers: `(r1, r2, ...) :: cmd-name : args`. The user-visible behavior is multiple dispatch — the receivers' types together determine the command's behavior, in the spirit of Julia's open multiple dispatch. *[CP001 §4.4]*

The implementation is composition of single-class dispatches, not joint-instance dispatch. A v-command's body is authored against the classes its receivers individually satisfy. The body invokes class methods on each receiver as needed; each such invocation is an ordinary single-class dispatch using the receiver's own class dictionary. The combined behavior of a v-command call is the product of its receivers' types, but no jointly-keyed dictionary exists. *[CP001 §4.4]*

V-command receiver modes (productive, reference, IN) are governed by the R1+R2 rules from CP005 §2 and forwarded to the type-and-modes reference for full treatment. The operational point relevant here is that v-commands compose multiple single-class dispatches — the call site sees a single invocation, but the dispatch resolution touches multiple class dictionaries. *[CP005 §2; class-system reference for full treatment.]*

*Sources for §5: CP001 §§3 (signatures, parameter passing, `-> name`, first-class commands, implicit parameters), §4 (classes and dispatch); CP004 §1 (parameter mode mechanics); CP005 §1 (marker syntax) and §2 (R1+R2 receiver rules); CP009 §2 (write-once productive sharpening).*

---

## 6. Frame-Exit and Lifecycle (Operational)

This section treats the operational behavior of the frame-exit and lifecycle machinery: the registration mechanism for `@`/`@!`, the firing schedule, the at-stack-exit mechanism for objects, and object lifetime ceilings. The failure-system reference covers failure-related aspects — particularly the at-stack handler discipline for failure payloads, which is more subtle than the general object case *(failure-system §§5.8, 5.10)*. The general operational rules appear here.

### 6.1 `@` and `@!` Operationally

`@` and `@!` blocks register cleanup actions for the frame in which the block is encountered. The blocks are not control-flow constructs in the body's normal execution; they are registrations. When control reaches an `@` or `@!` block during normal flow, the body is *registered* with the frame; the body does not execute at that point. *[CP002 §3.6]*

At frame exit:

- Registered `@` cleanups run on any frame exit (success or failure), in reverse order of registration.
- Registered `@!` cleanups run only on failure exit, in reverse order of registration.
- All cleanup bodies execute from a synthetic clear state; they neither see nor consume the propagating failure if any. *[CP002 §3.6]*

The reverse-registration-order rule reflects the standard "construct in order, destruct in reverse order" pattern: later-registered cleanups depend (potentially) on earlier-registered state, so later cleanups run first.

Internal failures of `@` or `@!` cleanup bodies propagate within the cleanup flow but do not return to the body's normal CFG. The exact propagation rules within the cleanup flow are subject to the same operational rules as ordinary command bodies; multiple cleanup bodies' failures compose per the cleanup machinery's own rules. *[CP002 §3.6; the precise handling of cleanup-internal failures is partially open.]*

### 6.2 The Hook Is a Property of the Call, Not the Value

`@`/`@!` are **fundamentally different from C++ destructors**. They fire on *call/scope exit*, not on *value destruction*. The hook is a property of the registering call's frame, not of any value that the cleanup body may mention. *[CP001 §5]*

Exit hooks do **not** fire when:

- A value is overwritten (assignment is not a destructor trigger).
- A value is copied (no double-destruction concerns).
- A value is moved or rebound.
- A pointer to a value goes out of scope (the value is not "owned" by the pointer).
- A `^Object` is rebound to a different object (the old object is not destructor-called by the rebinding).

The hook fires when the *registering frame* exits. If the frame retires successfully and an `@`-registered cleanup mentions an object that has since been swapped out and is no longer reachable, the cleanup runs anyway — with whatever the registering scope has at that point. The discipline is the programmer's; the language fires registered cleanups on schedule.

The natural pattern for resource management is: the command that *acquires* the resource also registers its release. *[CP001 §5]*

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

### 6.3 Resources Cannot Be Value-Owned Across Command Boundaries

Because `@`/`@!` fire on call exit, not on value destruction, resources cannot meaningfully be "owned" by data structures in the C++ RAII sense. A file handle stored in a record does not get released when the record goes out of scope; there is no "going out of scope" event for values in Basis. *[CP001 §5]*

Resources are tied to the *lifetime of the call that acquired them*. If a resource needs to outlive the acquiring command, the acquiring command must not register an exit hook for it, and release responsibility moves elsewhere — typically to a containing frame that registered the cleanup before delegating acquisition. The language does not track this; it is a discipline. *[CP001 §5]*

The language's at-stack-exit mechanism (§6.4 below) is the structured solution for resources whose cleanup is naturally tied to an *object's* lifetime ceiling rather than the acquiring frame's exit. The two mechanisms — frame-exit hooks for call-bound cleanup, at-stack-exit methods for object-bound cleanup — partition the cleanup design space.

### 6.4 The At-Stack-Exit Mechanism (Operational)

At-stack-exit methods are the language's structured mechanism for object cleanup. When an object type defines a `DEF_CMD_RECEIVER_ATSTACK` method (the `@` form on a receiver in the signature), that method is automatically invoked when the object's introducing frame exits — *as if* an `@ method: theObject` block had been written manually at the introduction site. Symmetrically, `DEF_CMD_RECEIVER_ATSTACK_FAIL` (the `@!` form) defines a method automatically invoked on failure-exit, paralleling `@!`. *[CP002 §3.2]*

Operationally, when an object is introduced into a frame, its type's at-stack methods (if any) are registered with the frame's exit machinery. At frame exit, the at-stack methods fire alongside any explicit `@`/`@!` cleanups registered in the frame, on the same schedule.

**Manual versus automatic invocation.** At-stack methods may also be invoked manually by user code. The two modes have different failure-handling semantics: *[CP002 §3.2]*

- **Manual invocation:** subject to ordinary failure-handling rules. The method's `?` / `:` / `!` mark applies; failures must be handled per the conformance rules.
- **Automatic invocation:** the failure status of the method is silently ignored. An automatically-invoked at-stack method can neither propagate a new failure nor recover an existing one. Failures inside automatic invocation are absorbed by the frame-exit mechanism. This is consistent with the semantics of `@!` blocks (which do not see or consume the propagating failure).

**Override via class instances.** At-stack methods are virtual in the class-dispatch sense. An instance declaration may override an at-stack method, and overriding is sometimes necessary to ensure proper cleanup for the implementing type. Dispatch of the implicit at-stack invocation goes through the type's class dictionary in the usual way. *[CP002 §3.2]*

### 6.5 Object Lifetime Ceiling

**The frame in which an object's storage is introduced is its lifetime ceiling.** No mechanism in the language allows an object to outlive the frame that introduced it, except via transitive containment in another object whose ceiling is already higher. *[CP002 §3.1]*

Structural consequences:

- A command cannot return an object to its caller. There is no return-value mechanism for objects (or for anything else; output is via writeable parameters only).
- A command can populate (initialize) an object whose storage was introduced by the caller via a writeable `^Object` parameter. The constructor doesn't *create-and-return* an object; it *initializes* an object slot the caller already owns.
- Object containment via fields propagates the ceiling upward: a field-of-object's lifecycle is at least as long as its container's lifecycle, which is bounded by its container's introducing frame. By transitive closure, every reachable object has a ceiling that is at most the introducing frame of the outermost containing object. *[CP002 §3.1]*

**Consequence: objects cannot escape upward.** A command may pass an object reference downward (to commands it calls) but cannot pass an object reference upward (since there are no return values). The object's reachability is fully contained within the call stack at-or-below the introducing frame. This is not a discipline; it is a structural guarantee enforced by the language. *[CP002 §3.1]*

The detailed mechanics of writeable `^Object` parameters — the three lifecycle cases (success with new object, failure before any write, failure after writing), registration migration on successful copy-restore — appear in CP002 §§3.3–3.4 and are forwarded to the construction-and-initialization reference for full treatment alongside other construction-related material. The point relevant here is that the operational lifetime ceiling is a structural property of the language enforced by the absence of object-return mechanisms.

### 6.6 Cross-Reference: At-Stack Handlers on Failure Payloads

The at-stack handler discipline for *failure payloads* — the values carried by propagating failures under typed failures — is more subtle than the general object case. Failure payloads move across frames at binding (binding-as-move) and at re-fail (re-fail-as-move), and the at-stack handler must fire when the value's *actual lifetime* ends, not at intermediate moves. The full discipline is specified in *[failure-system §§5.5–5.10]*.

The general principle from the operational rules of this section — "the at-stack handler fires when the value's home retires" — applies in both cases. For ordinary objects, the home is the introducing frame; for failure payloads, the home is the holding frame, which can change across moves. The failure-system reference is the source of truth for the holding-frame semantics; this reference covers the general object case.

*Sources for §6: CP001 §5 (frame-exit hooks fundamentally distinct from destructors); CP002 §§3.1–3.2, 3.6 (object lifetime ceiling, at-stack-exit mechanism, frame-exit hooks operational); failure-system §§5.5–5.10 cross-referenced for payload-specific at-stack discipline.*

---

## 7. Command Literals and Block Quotes (Surface Form)

This section is a placeholder pointer. The full semantics of command literals, lambdas, and fexprs belong to the planned lambda-and-fexpr reference; the operational role of this section is to record the surface distinction at the level visible from the operational semantics.

### 7.1 The Surface Distinction

The grammar admits two related but distinct families of brace-quote forms:

- **Block quotes** with a failure-mode prefix marker: `:{...}`, `?{...}`, `!{...}`. These are **fexprs** — slot-reference-bearing values whose body executes in the context of the capture frame, with captures by free name and a lifetime ceiling tracked through the type system. *[CP007 §2.1; CP010.]*
- **Bare brace quotes** without a failure-mode prefix: `{...}`. These are **lambdas** — objects with implicit fields, capturing values by copy at construction time, with no lifetime ceiling beyond the ordinary object ceiling. *[CP007 §§2.1, 3; CP008.]*
- **Command literals** with explicit parameter lists: `:<...>{...}`, `?<...>{...}`, `!<...>{...}`. These are eagerly-evaluated command values — distinct from both lambdas and fexprs in their evaluation semantics, and not subject to the lambda/fexpr distinction. *[CP007 §2.1.]*

The user reading source can distinguish the three families at sight. A bare `{...}` is a lambda; a failure-marked `:{...}`, `?{...}`, or `!{...}` is a fexpr; an angle-bracket form `:<...>{...}` is a command literal.

### 7.2 What This Reference Records

The surface distinction's existence and its three categories are recorded here so that the operational semantics — which sees command-typed values being constructed, stored, captured, and invoked — has names for what the user is constructing.

The full design of each category is the subject of the planned lambda-and-fexpr reference:

- Lambda mechanism details: by-value capture, hidden fields, invoke method, lifecycle inheritance, partial application restrictions, capture lists *[CP008]*.
- Fexpr mechanism details: implicit capture by free name, the invocation frame, the locality rule, body semantics, re-entry and nesting *[CP010]*.
- Command literal evaluation: how parameter passing is treated, how the eagerly-evaluated value is constructed *[CP007 §2.1; partially open per OQ-2.]*.

The interaction of these mechanisms with implicit context parameters, with capture-shadowing rules, and with the no-non-local-state provision-chain rule is also forwarded to the lambda-and-fexpr reference.

*Sources for §7: CP007 §2 (the syntactic distinction); CP008, CP010 (full semantics, deferred).*

---

## 8. Open Questions (Operational)

This section catalogs the operational/structural open questions covered by this reference's scope. Open questions about types-as-types, construction, lambda/fexpr details, and class-system specifics are forwarded to their respective planned references.

### 8.1 OQ-2 — Implementation Latitude for IN Parameter Passing — Open

The language semantics commit to call-by-value for IN parameters and call-by-copy-restore for writeable parameters at the *observable* level. The implementation may diverge where divergence is unobservable. The precise rules — the conditions under which large IN parameters may be passed by reference under the hood, how this interacts with the macro/fexpr-like semantics of block-quote forms, the correctness criterion for "unobservable divergence" in a language with first-class commands and writeable parameters — are open. *[CP001 §3.2, §6 OQ-2.]*

Sub-questions noted by CP001 §6:

- (a) Conditions under which large IN parameters may be passed by reference under the hood (no aliasing through other parameters, no escape of the parameter address, no observation of mutation through the reference).
- (b) Interaction with macro/fexpr-like semantics: the `:{...}` block-quote and `:<...>{...}` command-literal forms are believed to need different parameter-passing treatment from each other; the design has not been finalized.
- (c) Correctness criterion: precisely what "unobservable divergence" means in a language with first-class commands and writeable parameters.

Resolution must specify both the observable semantics for each call form and the optimizations the implementation is permitted to make.

### 8.2 OQ-3 — `-> name` Result Designator on Non-Writeable Parameters — Open (mostly resolved per CP004)

The well-formedness rule for `-> name` requires that name be a parameter or receiver but does not require writeability. The semantic meaning of designating a non-writeable parameter as the expression-position result — given that non-writeable parameters are call-by-value and the callee's copy evaporates on return — needs characterization. *[CP001 §6 OQ-3; reframed by CP004 §6.1.]*

Possible readings from CP001 §6:

- (a) The result is the *initial* value of the parameter (echoing back what the caller passed in; useful for combinator/identity-shaped commands).
- (b) The result is the value at the moment of return *as observed inside the callee*, with the callee's local lifted out as the expression value at the call boundary.
- (c) The case is intentionally restricted to specific patterns yet to be specified.

CP004 §6.1 partially resolved this by reframing the question in terms of the productive/reference distinction: under the no-defaults model, a non-writeable parameter cannot be the productive output of the call (the callee's local copy doesn't write back), so the surface form's meaning, if any, must be one of (a) or (b). The full settlement remains for incremental resolution.

### 8.3 OQ-15 — Downcast Intrinsic — Open (Forwarded)

CP005 §3 introduced the downcast intrinsic as a named topic for a future dedicated thread. The operational shape — a named intrinsic that performs a runtime type check and either succeeds with the narrowed value or fails — is sketched. The full design (surface form, interaction with class dispatch, interaction with fat-pointer RTTI) is open. *[CP005 §3; OQ-15.]*

The downcast intrinsic is operational in that it fires runtime checks visible at the source level. Its full design will likely affect both the type system (the narrowed type the success edge produces) and the operational machinery (the runtime mechanism for the check). For the moment, the question is recorded here and forwarded to its eventual dedicated thread.

### 8.4 Forward References for Open Questions Belonging Elsewhere

Several open questions are operational-adjacent but more naturally belong to other planned references:

- **OQ-1** (union discriminator representation, CP001 §6) — type-and-modes reference. The failure-system reference established that failure tags are orthogonal to OQ-1 *(failure-system §7.7)*; the general union-discriminator question is type-system territory.
- **OQ-4** (default initialization, CP001 §6) — resolved by CP004 in favor of no-defaults plus statically-tracked initialization. The operational consequences for parameter passing appear in §5.2 above; the construction-and-initialization reference will treat the resolution in full.
- **OQ-5** (single-class instance coherence, CP001 §6) — class-system reference.
- **OQ-6** (partial application beyond receiver-only, CP001 §6) — class-system and lambda-and-fexpr references jointly.
- **OQ-13** (implicit context parameters and initialization), **OQ-14** (same-scope rule for `&x` and `x`), **OQ-16** (overloading restriction on dynamically-dispatched commands), **OQ-22** (parameterized literal types in `.implicit`), **OQ-23** (lexer disambiguation rules), **OQ-24** (`.implicit` Aggregate/Sequence phasing), **OQ-25** (capture-shadowing) — variously forwarded to type-and-modes, construction, lambda-and-fexpr, and class-system references.
- **OQ-18** (lambda visible-signature representation), **OQ-20** (slash-list internal grammar), **OQ-21** (capture-list interaction with implicit context parameters) — lambda-and-fexpr reference.

The full OQ catalog with current status appears in CP011 §13 (as of CP011) and CP012 §11 (as of CP012); the failure-system reference's §7 covers the failure-related OQs in resolved or open form. A consolidated cross-reference catalog is a candidate for future work but is not produced here.

---

## 9. Provenance

**Authored:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, on 2026-04-29, in continuation of the topic-organized consolidation begun with `reference-failure-system.md`. No per-document handoff was prepared for this reference; scope and section structure were settled with the user inline before drafting.

**Source materials:** intent-checkpoint-001.md (foundational principles, type system overview, command shape, classes and dispatch overview, lifecycle); intent-checkpoint-002.md (block markers operational rules, lifecycle hooks); intent-checkpoint-003.md (compound guards via `%`); intent-checkpoint-004.md (parameter-mode mechanics, write-once productive parameters); intent-checkpoint-005.md (marker syntax `'` and `&` as identifier shapes, R1+R2 receiver rules); intent-checkpoint-007.md (provision-chain rule, brace-quote surface distinction); intent-checkpoint-009.md (productive-parameter sharpening to write-once); intent-checkpoint-011.md (`^`-body-optional refinement). Secondary reference: `reference-failure-system.md` for failure-flow material consistently cross-referenced rather than duplicated.

**Cross-references to other planned references:** Type-and-modes reference (parameter-mode definitions, type system, OQ-1, OQ-5 partially); construction-and-initialization reference (`<-` polymorphic RHS, `.implicit`, default initialization resolution from CP004, aggregate/sequence literals); lambda-and-fexpr reference (CP007–CP008–CP010 in full, OQ-7 history, OQ-18, OQ-21, OQ-25); class-system reference (Haskell-style dictionary passing details, instance coherence, partial application).

**Scope boundary calls confirmed with user before drafting:** Parameter-mode operational behavior covered here briefly; full type-and-mode definitions deferred to the type-and-modes reference. `-> name` mechanism covered here as operational; construction reference may cross-refer back. Object lifetime ceiling covered as a general operational rule; type-side and construction-side aspects deferred. The `<-` operator and polymorphic RHS deferred entirely to the construction-and-initialization reference. *[Reconciliation: these are this-reference choices, not directly determined by the source checkpoints; they implement the topic split agreed in the failure-system reference's handoff §10.]*

**Recommended next step:** User review section-by-section, revisions in place. After this reference is settled, the next topic-organized reference per the agreed split is **Type System and Modes** (#3), covering buffers/ranges/domains/aliases/records/objects/unions/variants/pointers/command-types as types and parameter-mode markers as type-level features. The full sequence is in the failure-system reference's handoff §10.
