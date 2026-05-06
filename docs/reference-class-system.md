# Basis Language — Reference: Class System

**Status:** Topic-organized authoritative reference for the class system — classes and instances; the witness-slot model and dictionary passing; single-class dispatch and v-command multiple dispatch; partial application beyond receiver-elision; instance coherence (orphan instances, domain-specific dispatch, cross-module overriding); overloading on dispatched commands; the Liskov-style opening of the failure-tag hierarchy under payload-class covariance; class-method fexpr-typed parameters and the fexpr-relevance taint axis; the boundary against context variables; and the variant class-witness slot. Consolidates material from CP001 (foundational class design, instance and dispatch mechanics, OQ-5/OQ-6/OQ-16 framings), CP005 §2 (R1+R2 receiver-mode rules), CP007 §7 (the OQ-16 candidate-restriction framing), CP012 §§1.2 / 3.1 (class-bound payloads on failure tags), and the 2026-05-04 scope-confirmation dialog (CP017) which produced the substantive resolutions of OQ-5 (all four sub-questions), OQ-6 (broader scope), OQ-16, OQ-29 (reframed in class-instance terms), and the OQ-32 class-method-fexpr-typed-parameter sub-item; and which carved in the Cases A and B class-typed-parameter taxonomy, the 3-word slot pattern for class-typed values, the multi-role `-<` framing, the unified bright-line rule across instance declarations and failure-tag children, the co-location rule for instance implementations, and fexpr-relevance as a second taint axis parallel to the READ contract.

**Date:** 2026-05-05

**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, in continuation of the topic-organized reference consolidation begun with `reference-failure-system.md` (2026-04-29), `reference-operational-semantics.md` (2026-04-29), `reference-type-system-and-modes.md` (2026-05-02), `reference-construction-and-initialization.md` (2026-05-03), and `reference-lambda-and-fexpr.md` (2026-05-04). This is the sixth and final reference in the agreed topic-organized sequence. All commitments from CP017 are encoded from inception with reconciliation markers; rejected intermediate framings from the dialog (notably the buffer-backed-subsumption framing of OQ-29 and the deferred-receiver framing of OQ-6) are noted at point of correction.

**Authority statement.** Where this reference differs from earlier checkpoints on a covered topic, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative. Where this reference and an earlier reference overlap, this reference is authoritative on the class-system side and the earlier reference is authoritative on its own domain, with cross-references flagging the boundaries. Specifically: type-system §§3.4, 3.5, 5.2, 7.5, 7.6, 7.9, 7.10 receive substantial touch-ups (work plan §6.3); failure-system §§4.2, 4.3, 7.6 receive touch-ups (work plan §6.1); op-sem §5.7's R1+R2 forwarding redirects here from type-and-modes (work plan §6.2); construction §§3.5, 10, 11 receive substantial touch-ups (work plan §6.4); lambda-and-fexpr §§3, 5, 6, 6.5, 8, 10 receive substantial touch-ups (work plan §6.6).

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]*. Section-level connective tissue carries footer citations identifying the section spans drawn upon. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints — typically resolving an ambiguity, recording a post-checkpoint correction, filling a gap, or carving in a refinement after the 2026-05-04 dialog. **Cross-references** to earlier references take the form *[failure-system §x]*, *[op-sem §x]*, *[type-system §x]*, *[construction §x]*, *[lambda-and-fexpr §x]*.

**Standing principles carried into this reference.** Six lenses from prior references and the operating-principles document govern the treatment here:

1. **Frame-ownership** *[OPS §1.1; type-system standing principle]*. Every slot is owned by some frame; "return values" in the conventional sense do not exist. Class-typed parameter slots (Cases A and B, §3) are owned by the call site for input positions and by the call site for output positions; the witness flow is part of the slot's contents, not part of the callee's analysis context.

2. **No cross-frame analysis propagation** *[OPS §1.2]*. Each frame's static analysis is local. A polymorphic command's hidden-parameter witness is part of its parameter list from the inside; from the caller, the witness selection happens at the call site against the caller's visible instances. Witness selection does not propagate across frames.

3. **Access paths, not storage** *[OPS §1.3]*. Both established taint axes (the transitive READ contract; fexpr-relevance, §11) sit on access paths. Class-typed values are values like any other; their access paths can carry either taint.

4. **The orthogonality preference** *[OPS §1.8]*. Where a new mechanism's work can be done by composing existing language facilities, the composition is preferred. Two cases applied here: the `-<` operator is elevated to a multi-role dynamic type coercion across all type pairs (class membership, variant candidate matching, union byte-reinterpretation) rather than introducing new per-case operators; the `{C::method}` disambiguation form reuses the four-form taxonomy's command-reference reading rather than adding a new syntactic shape.

5. **Liskov substitution as a design tool** *[OPS §1.9]*. Where the language admits a subtyping relation, the design preserves Liskov substitutability. This reference's §10 reframes OQ-29 in class-instance terms (the prior buffer-backed-subsumption framing was incorrect) and lands the current concrete rule of *same-class equality* as the trivially-Liskov-preserving case, with *class-implication* as the future relaxation when a class-hierarchy mechanism is admitted.

6. **Compute-then-commit applies in witness selection** *[construction §1.4; failure-system §5.4]*. Witness construction at `.fail` sites and at construction sites for Case B class-typed values follows the same compute-then-commit pattern as the rest of the language: the witness is selected at compile time (at the construction site for the value); no runtime witness construction is required.

---

## 2. Classes

A **class** is a single-parameter type contract — structurally analogous to a Haskell single-parameter typeclass, a Java interface, or a Scala trait. A class is parameterized over *one* type (the implementing type / receiver), not over a tuple of types; there is no multi-parameter class concept. *[CP001 §4.1.]*

The class is the contract; instances of types satisfying the class are the things that have layouts. A class itself has no size, no `sizeof`, no allocation footprint. The class declaration enumerates the operations that any instance must provide; the instance declaration provides those operations for some specific implementing type.

### 2.1 Class Declarations

A class declaration introduces a class name into the surrounding scope and lists the class's command members. Each command member is either a **declaration** (no body — the instance must supply it) or a **definition** (with a body — the instance may override it but is not required to). The grammar's surface for the two cases is `declare ...` (declaration) and `command ...` (definition). *[CP001 §4.1.]*

**Declared methods must be supplied by every instance.** An instance lacking a body for a declared method causes compilation to fail (subject to the co-location rule of §3.4 — "supplied" includes inheritance via delegation, defaults from the class itself when the class supplies a default, or any other in-scope visibility mechanism).

**Definitions provide a default implementation.** An instance that does not override a default uses the class's default body. The default body is compiled in the class's defining-module scope; it can reference any name that scope makes available, including the receiver `T`-typed parameters of the class's other methods (since calling `T`-typed methods on a `T`-typed receiver is well-formed within a default body).

**There are no "final" implementations.** Every default may be overridden by an instance. The language does not provide a marker for "this default may not be overridden"; this is a deliberate simplicity choice. *[CP001 §4.1.]*

### 2.2 Single-Parameter Constraint

A class is parameterized over exactly one type. There is no `class C T1 T2` form (multi-parameter classes), no `class C T where T2 = ...` form (associated types as additional parameters), and no `class C[T<:Bound]` form (bounded parameters at the class level — the bound, if needed, lives at the use site as a type-variable constraint per §3.2 of this reference).

Multi-class needs (a parameter constrained by both `Showable` and `Hashable`, for instance) are met by the **combined-class** form per CP001 §4.2: the user declares a new class `class CombinedC: C1, C2` whose instances are required to satisfy both `C1` and `C2`. The combined class is itself a single-parameter class; the multi-parameter conceptual structure is built up from single-parameter classes by composition. *[CP001 §4.2.]*

The single-parameter constraint composes cleanly with v-command multiple dispatch (§6 of this reference): a v-command's individual receiver positions are each constrained by single-parameter classes, and the v-command's behavior is the product of those independent constraints. There is no "join" of class dispatches across receivers; each receiver's dispatch is independent.

### 2.3 No-Final Discipline

The "no final implementations" rule has a structural consequence for class evolution: **adding a new declared method without a default to an existing class breaks all existing instances at the source level.** The instance declarations would need to be amended in their declaring modules to provide the new method's body. Adding a new method *with* a default is backward-compatible — existing instances inherit the default and continue to work. *[CP017 §7.5; the typeclass-evolution trade-off codified explicitly here.]*

This is the standard typeclass-evolution trade-off, recorded here as a load-bearing fact for module authors planning class versioning. The user is responsible for class evolution discipline; the language provides the mechanisms (defaults, combined classes) but does not enforce backward compatibility automatically.

### 2.4 What a Class Is Not

A class is not a type. A class name is a contract; it is not interchangeable with a concrete type, a domain, a record, or any of the language's type-bearing forms in most contexts. The two contexts where a class name appears in a type-like position are *[CP017 §3.1]*:

- **In a parameter declaration as Case B**, where the class name denotes an existentially-quantified type — a value satisfying the class. The runtime representation is the 3-word slot pattern of §4.
- **In a type-variable constraint as Case A**, where the class name appears in the qualifier `(T:Class)` introducing a type variable `T` constrained to instances of the class. The class name is a constraint on `T`, not the type of the parameter.

In all other positions, a class name is a syntactic error. The user does not write `Showable x` to mean "a value of class Showable's type"; the user writes `(T:Showable) x` to constrain `x` to a class instance, or `Showable x` (Case B existential) to admit any class-satisfying value at that slot. *[CP017 §3.1; the dialog-clarification on naming.]*

The user's framing in CP017: classes in Basis are Haskell-style typeclasses (contracts), not Java-style concrete types. A class name is not itself a type whose values can be instantiated; it is a contract specifying operations a type must provide. "Passing a class into a parameter of class type" is nonsensical; one passes arguments that are *instances* of that class.

*Sources for §2: CP001 §4.1 (class declarations, declared/defaulted, no-final); §4.2 (combined classes); CP017 §§2 (section structure), 3.1 (class-as-existential vs. class-as-type-variable-constraint), 7.5 (class-evolution trade-off).*

---

## 3. Instances

An **instance** declaration says: "implementing type `T` satisfies class `C`," and supplies the bodies for `C`'s declared methods (and, optionally, overrides for `C`'s defaults) on `T`. The instance produces a **dictionary** for the `(C, T)` pair — a record-like value containing command-typed values for each of `C`'s members, populated per the instance declaration. *[CP001 §4.2; CP001 §4.3.]*

The dictionary is the runtime artifact through which class-dispatch flows; §4 covers its representation and the witness-passing model in detail. This section covers the surface mechanics of instance declarations, the explicit-delegation rule, the co-location rule, the dispatch-key structure, and the boundary against context-variable resolution.

### 3.1 Instance Declarations

The grammar's instance form (`instance Name: TYPENAME, TYPENAME, ...` in placeholder syntax) is *not* a tuple-keyed instance. It is "one type implementing several classes simultaneously," analogous to Haskell's `instance (C1 a, C2 a) => ...`. There is no multi-class joint-instance machinery; each class listed produces a separate dictionary for the `(class, T)` pair. *[CP001 §4.2.]*

```
.instance Color :
    .class Showable
        cmd render: String 'r = ...
    .class Hashable
        cmd hash: Int32 'r = ...
```

(Placeholder syntax pending the broader signature-surface question of OQ-26.2.) The declaration above produces two dictionaries: one for `(Showable, Color)`, one for `(Hashable, Color)`. The two dictionaries are independent; dispatching `Showable`'s methods on a `Color` value uses one, dispatching `Hashable`'s methods uses the other.

The instance body provides implementations for each listed class's declared and defaulted commands. Where a default from the class itself is acceptable, the instance may omit the override. Where a method is declared (no default), the instance must provide a body — subject to the co-location rule of §3.4.

### 3.2 Delegation

An instance may **delegate** its implementation of a class to a designated field of the implementing type. The grammar's `(delegate fieldName)` clause names the delegate explicitly. *[CP001 §4.2.]*

```
.record Wrapped :
    Backing inner

.instance Wrapped :
    .class Showable (delegate inner)        ; use inner's Showable instance
```

When `Wrapped`'s `Showable` methods are dispatched, the dispatch routes to `inner`'s `Showable` dictionary. The receiver passed to `inner`'s methods is `inner` itself (the field's value), not the wrapping `Wrapped` value.

**Delegation is always explicit.** Even if the implementing type has multiple fields whose types could provide an implementation of the class, only the *named* delegate is used. This rule overrides the type-based singleton resolution that applies elsewhere (specifically, to context parameters' uniqueness-of-type resolution, §12). In instance delegation, the explicit delegate trumps any uniqueness-of-type consideration. *[CP001 §4.2.]*

The asymmetry between context-parameter resolution (uniqueness-of-type, implicit) and instance delegation (explicit naming) is deliberate. Context parameters operate at value level where uniqueness-of-type is reliable; delegation operates at type level where multiple compatible fields could plausibly exist by accident, so explicit naming is required to preserve "no surprises": the caller always knows what implementation will be invoked.

### 3.3 The Dispatch Key Is `(class, type)`

A dispatch site specifies a class (the contract being satisfied) and a receiver value of some type (the type providing the implementation). The runtime resolution looks up the dictionary keyed on `(class, type)`. *[CP001 §4.3.]*

Two consequences worth carving in:

- **The dispatch key is not (class, value).** Two values of the same type dispatch through the same dictionary. There is no per-value implementation; the dictionary is per-type-per-class.
- **The dispatch key is not (class, parent-type).** A value whose runtime type has its own instance for a class dispatches through *its* instance, even when the value has been implicitly upcast to a parent type at the call site (the domain-specific-dispatch rule of §8.3).

The dispatch-key structure also implies that a single type may have at most one instance per class. Two `instance Showable: Color` declarations in the same module are an intra-module duplicate — a static error. Two `instance Showable: Color` declarations in *different* modules are cross-module duplicates — resolved by Julia-style specificity ranking per §8.4.

### 3.4 The Co-Location Rule

Every `.instance T: C` declaration must result in a complete dictionary for `C`'s methods on `T` after the instance is fully assembled. **Implementations not otherwise in scope (via `C`'s defaults, methods on `T`, delegation per §3.2, or any future mechanism that contributes implementations through visibility) must be supplied locally in the instance declaration. Methods left without any provider after assembly cause compilation to fail.** *[CP017 §7.1; the co-location rule.]*

The phrasing is deliberately positive — what counts as *in scope* — rather than restrictive — what must be *redundantly re-provided*. The user's framing in CP017: the responsibility is to *complete* the dictionary, not to *redundantly re-provide* what is already available through other channels.

What counts as "in scope" *[CP017 §7.2]*:

- **Defaults from the class.** If `C` declares a default body for some method, an instance for `T` need not re-supply it; the default is in scope by virtue of importing `C`.
- **Methods supplied by `T`.** If `T`'s declaration supplies a command bearing the right shape for `C`'s declaration of some method, that method is in scope by virtue of importing `T`. The instance need not re-supply it.
- **Delegation per §3.2.** If the instance declares `(delegate fieldName)`, the delegate's existing instance provides the method bodies through chained dispatch.
- **Future mechanisms.** *[Reconciliation: this is the user's deliberate hedge in CP017 §7.4. The phrasing "or some other means not currently identified" preserves design space for future contribution mechanisms — trait-style composition, deriving, automatic implementations from related classes, etc. — without re-litigating the assembly rule when such a mechanism is added. The reference records this hedge so future design dialogs know the door was intentionally left open.]*

**Delegation routes through.** When an instance uses `(delegate fieldName)`, the delegate's existing instance provides the method bodies. Those bodies, in turn, might rely on defaults or on chained delegation. The "missing" determination walks the full chain at compile time: a method is missing only if no provider exists anywhere along the resolution chain. *[CP017 §7.3.]*

### 3.5 Boundary Against Context Variables

Implicit **context variables** — supplied to commands via the `/`-separated section of the parameter list (op-sem §5.5; construction §13) — are a parallel-but-distinct mechanism from instance dispatch. The two are conceptually adjacent (both are "implicit-resolution" machinery) and easily conflated; this section draws the line.

| Mechanism | Resolution rule | Resolved at | Resolved against |
|---|---|---|---|
| **Instance dispatch** | `(class, type)` lookup | Call site, statically | The class dictionary table |
| **Context-variable resolution** | Uniqueness-of-type from lexical scope | Call site, statically | The caller's lexical scope |

The two never directly interact: instance dispatch picks a method (a behavior); context-variable resolution picks a value (a configuration object, a logger, etc.). A class method may itself take context parameters in its `/`-list, but that is a property of the *method's* signature, not of the dispatch-resolution machinery.

Construction §13 owns the full mechanics of context variables. The class-system reference's involvement is the boundary statement: if a question is "which method runs?", that's instance dispatch (this reference); if a question is "which value is passed at this slot?", that's context-variable resolution (construction §13). *[CP015 §8; construction §13.8 for the symmetric statement.]*

The two have no operational dependency on each other. A polymorphic command passing through dictionary-based class dispatch (§4) may also have context parameters; the witness is one hidden parameter, the context value is another. They flow independently.

*Sources for §3: CP001 §§4.2, 4.3 (instance declarations, delegation, dictionary representation); CP017 §§7 (co-location rule), 13.4 (boundary against context variables); construction §13 (context-variable mechanics).*

---

## 4. Witness-Slot Model and Dictionary Passing

This section covers the runtime representation of class-typed values and the mechanism through which class dispatch flows. Three layers compose:

- The **dictionary** — the per-`(class, type)` record of method implementations (§4.1).
- The **hidden-parameter witness** — the polymorphic-command convention for Case A (type-variable-bound) parameters, where a single witness travels alongside the call (§4.2).
- The **3-word slot pattern** — the runtime representation for Case B (existential) class-typed values, where each slot independently carries its witness (§4.3).

The two parameter cases (A and B) are introduced at §4.4, and the witness-flow rules under captures and through composite values are covered at §§4.5–4.6.

### 4.1 The Dictionary

A **dictionary** is a record-like value containing command-typed values for each of a class's members, populated according to a matching `instance` declaration. Default implementations from the class body fill in for any commands the instance did not override. *[CP001 §4.3.]*

The dictionary's runtime representation is implementation-internal; the language model is that, given a dispatch site, the implementation locates the appropriate dictionary and invokes the class member's command-typed value found there. The mechanism is structurally analogous to GHC's underlying representation of typeclass dispatch: each dictionary is a record of function pointers (or, more precisely, command-typed values), and dispatch is an indirect call through one of those values.

The commitment to dictionary passing has several structural consequences *[CP001 §4.3]*:

- **Domains stay byte-faithful.** An `Inches` is `[4]` bytes, period. Class dispatch information lives in the dictionary, not in the value. Adding `instance Showable: Inches` does not change the runtime size of an `Inches` value.
- **Records stay packable.** Records containing class-constrained fields are still exactly the size of their fields' bytes. No hidden alignment or padding for vtables.
- **Monomorphization is an optimization, not the semantics.** The semantic model is dictionary passing; specialization to direct calls is a compiler optimization the implementation may perform when type information is statically known. The programmer can also force this manually by binding the resolved command to a local variable (op-sem §5.4 / CP001 §3.4 — the tight-loop optimization).

The fat-pointer RTTI mechanism for object-class hierarchies (mentioned in type-system §3.3 / CP001 §2.4) is a *separate* runtime-type-info mechanism, used for downcast safety on object pointers. It is not the same as the class-dispatch dictionary. Different jobs, different mechanisms; both coexist in the runtime. *[CP001 §4.3.]*

### 4.2 The Hidden-Parameter Witness (Case A)

A polymorphic command — one whose signature contains type variables constrained by classes — receives, as a hidden parameter alongside its declared parameters, the dictionaries for the type-variable's class constraints. *[CP001 §4.3.]*

For a signature like `cmd doIt: (T:Showable) 'r, T x = ...`, the compiler emits the command as if its actual parameter list were `(SHOWABLE_DICT, 'r, x)` — the dictionary for `(Showable, T)` is supplied at the call site and travels alongside the regular arguments. Inside the body, dispatch on `x :: render` (where `render` is a `Showable` method) compiles as an indirect call through the supplied dictionary's `render` slot.

The witness is selected at the call site by the typechecker, against the visible instances at that site. For a call `doIt: 'result, (color :: as_showable)` where `color: Color` and `instance Showable: Color` is visible, the typechecker selects the dictionary for `(Showable, Color)` and passes it. The selection happens at compile time; no runtime instance lookup is required.

This is the mechanism that supports Case A type-variable-bound parameters specifically. The witness flows once per call (regardless of how many `T`-typed slots the signature has, since they all bind the same `T`) as a hidden parameter. The values themselves are in their natural representation — a `T`-typed slot is bytes-of-`T`, not a 3-word slot. *[CP017 §4.2.]*

### 4.3 The 3-Word Slot Pattern (Case B)

A Case B class-typed slot — a parameter declared `Class 'name`, where `Class` denotes "any value satisfying this class" existentially — uses the 3-word slot pattern as its runtime representation: *[CP017 §4.1.]*

| Slot word | Content |
|---|---|
| Tag identifier | Small-integer RTTI tag identifying the runtime concrete type |
| Value pointer | Pointer to the value's bytes (or pointer-shaped value, for pointer-typed candidates) |
| Class witness | Pointer to the class dictionary (witness) for the runtime type |

This is structurally parallel to the variant 3-word slot (type-system §3.4 / §13 of this reference) and the failure-system slot (failure-system §5.1). The reuse of the slot shape is intentional language-design parallelism — it allows the same dispatch and propagation patterns across variants, failures, and class-typed values. *[Reconciliation: the 3-word slot pattern's elevation to a *standard* runtime representation across three contexts (variants, failures, class-typed values) is a CP017 carve-in. Prior to CP017, the slot shape was specified independently for variants (type-system §3.4) and failures (failure-system §5.1); the class-typed-value case was open. CP017 §4.1 settles all three uses as instances of the same pattern, with Case A as the variation that uses natural representation rather than the slot.]*

The witness in the slot is selected at the **construction site** for the value — wherever the value is first wrapped into a class-typed slot. Whichever instance is visible at that construction site is the one captured in the value's witness; downstream sites use the captured witness regardless of what instances they see. (This is the basis of the witness-mechanism interaction with permissive orphan instances; see §8.5.) *[CP017 §6.5.]*

### 4.4 Cases A and B: The Substantive Distinction

The dialog admitted two structurally different uses of class names in parameter declarations: *[CP017 §3.1.]*

- **Case A — class as a constraint on a type variable.** A type variable `T` is introduced and constrained to instances of a class. Parameters typed as `T` accept any concrete type satisfying the class; multiple `T`-typed slots in a signature are bound to the *same* concrete type. The witness flows once as a hidden parameter (per §4.2); slot representation is the natural representation of the bound type. *Surface:* `(T:Class) 'name`, with subsequent parameters referencing `T` by name.

- **Case B — class as an existential type at parameter position.** A parameter typed *as* a class accepts any value whose concrete type satisfies the class; each slot independently carries its witness in the slot itself. The runtime representation is the 3-word slot pattern of §4.3. *Surface:* `Class 'name`, with `Class` being the class name appearing where a type would.

Both cases coexist and compose. A signature may freely mix `(T:Class) 'a, T 'b, OtherClass 'c` — type-variable-bound `a` and `b` share concrete type `T`; existential `c` is independently bound, carrying its own witness in its slot. *[CP017 §3.1.]*

The witness-mechanism difference, at a glance:

| Case | Witness location | Witness selection time |
|---|---|---|
| A | Hidden parameter alongside the call | Call site (typechecker selects based on visible instances) |
| B | In the slot itself (3-word) | Construction site (where the value is wrapped into the class-typed slot) |

The user-visible behavior is identical in both cases — `(value :: method)` dispatches through the appropriate witness. The cost difference (Case A pays once per call; Case B pays per slot) is invisible to the author at the dispatch site; the difference shows up only in storage and parameter-passing economics.

#### 4.4.1 Multi-Class Constraints (Scope Narrowing for v1)

A type variable may be constrained by a *single* class only. Multi-class needs are met by user-defined combined classes per CP001 §4.2's `class CombinedC: C1, C2, ...` form. *[CP017 §3.3.]*

A parameter `(T:CombinedC) 'name` then constrains `T` to types implementing both `C1` and `C2`. The combined class declares no methods of its own; its purpose is purely the conjunction of constraints. Future relaxation possible — admitting comma-separated multi-class constraints inline at the parameter declaration, e.g., `(T:C1,C2) 'name` — but for v1, the single-class rule is the bright line. *[CP017 §3.3.]*

#### 4.4.2 Type-Variable Scope

Type variables introduced in a parameter declaration are in scope for the *entire signature*, regardless of source-position order. A signature like `T 'a, (T:Point) 'b` is well-formed: `T` is introduced at `b`'s declaration but is also referenced by `a`'s declaration. The typechecker performs a pre-pass to gather type-variable introductions before binding parameter types; this is an implementation concern that does not affect the surface design. *[CP017 §3.4.]*

The whole-signature scope simplifies authoring: the user does not need to think about declaration order when introducing type variables. Any parameter may reference any type variable bound anywhere in the signature.

#### 4.4.3 V-Command Receiver Tuples

Type variables introduced at v-command receiver positions are in scope for non-receiver parameters in the same signature. This admits patterns like: *[CP017 §3.5.]*

```
.cmd doSomething: (T:Point) 'r, T y = ...
```

where `r` is a productive receiver of some `Point`-class instance type `T`, and `y` is a non-receiver parameter constrained to the same `T`. The grammar extension (admitting the `(T:Class)` qualifier in receiver positions) is added to OQ-23's implementation-thread batch.

#### 4.4.4 Mode Markers and the `name : Type` Confusion

A recurring miscalibration from the dialog (corrected several times across prior sessions per the user) is the conflation of Basis parameter syntax with C/Pascal/Haskell-style `name : Type` annotations. **Outside the class-constraint case `(T:Class)`, the colon form is not Basis syntax.** The correct order is type-then-name, with the mode marker as a *prefix* on the name: *[CP017 §3.6.]*

| Form | Surface |
|---|---|
| Regular parameter (concrete type) | `Container 'r` (productive `r` of type `Container`) |
| Existential (Case B) | `Showable 'r` |
| Type-variable-bound (Case A) | `(T:Point) 'r` |

Mode markers (`'` productive, `&` reference; or absent for IN) prefix the parameter name. The class-constraint form `(T:Class)` is the only place `:` appears as a binding-and-constraint operator on a type variable; in that form, the parens delimit the binding clause, and the mode marker still prefixes the parameter name *outside* the parens.

The mental model: `Type 'name`, `(T:Class) 'name`, `Showable 'name` — never `name : Type`. *[CP017 §3.6; recorded as a dialog-correction reconciliation point.]*

#### 4.4.5 Bidirectional Existentials

A coherence point worth carving in: when the slot type is a class (Case B), the existential pattern applies symmetrically in both directions: *[CP017 §3.7.]*

- **Input-side existential.** A caller provides a value of some type satisfying the class; the command receives it with witness attached; the command dispatches through the carried witness.
- **Output-side existential.** The command picks some type satisfying the class, fills the productive slot with a value of that type, attaches the witness; the caller receives both and dispatches through the carried witness.

The witness flows in whichever direction the data flows, in lockstep. This was a refinement that emerged mid-dialog when the original framing of "`Container 'r` as a stand-in for a concrete type" was challenged — when the slot type is a class, the productive direction is itself an existential operation. Neither direction reduces to a known concrete type; both directions carry the witness in the slot. *[CP017 §3.7.]*

The framing has implications for §13's variant class-witness slot, where the same bidirectional-existential property applies: a variant whose candidate value carries a class witness produces witness-bearing reads in either direction.

### 4.5 RTTI: Implementation-Internal, Not Programmer-Visible

The tag identifier in the 3-word slot supports **runtime type information** at the implementation level — the language implementation can determine the concrete type of a class-typed value at runtime and use this for the dynamic narrowing operator `-<` (§5) and for class dispatch on the value. *[CP017 §4.3.]*

**The tag is not programmer-visible directly.** There is no surface form for reading the tag, no `tagOf` builtin, no determinism requirement on the tag-space (the implementation may assign tags as it sees fit, opaque to the programmer). The tag exists specifically to support `-<` and class dispatch; programmer-facing surfaces interact with RTTI through `-<` only.

The user's framing in CP017: RTTI exists specifically to support `-<` and class dispatch. Programmer-facing surfaces interact with RTTI through `-<` only.

This is a deliberate scope-narrowing: the language could in principle expose the tag (a la Java's `getClass()` or C++'s `typeid`) but does not. The orthogonality preference (OPS §1.8) is the rationale: `-<` already does the work of "branch on runtime type"; a tag-introspection surface would be a parallel mechanism without doing additional work.

### 4.6 Witness Flow With Captures

A class-typed (Case B) capture in a lambda or fexpr brings its witness into the captured environment. A type-variable-bound (Case A) capture brings the hidden-parameter witness as part of the closure environment. *[CP017 §4.5.]*

For lambdas (lambda-and-fexpr §5), an IN capture of a Case B class-typed value copies the 3-word slot into the lambda's hidden field at construction time; subsequent invocations dispatch through the captured witness. A reference (`&`) capture of a Case B class-typed slot captures the slot reference; the witness is read at each invocation through the captured reference (and the standard per-invocation copy-restore preserves the captured slot's contents across failures).

For fexprs (lambda-and-fexpr §6), an implicit capture-by-free-name of a Case B class-typed slot resolves to the slot's defining-frame contents directly (no copy-restore); the witness is read at each access through the captured slot.

For Case A captures, the hidden-parameter witness is part of the closure environment of any lambda or fexpr that captures `T`-typed slots. The witness is captured once at construction time (since it's a single hidden parameter for the whole signature) and is available throughout the body's invocations.

This is an extension of the lambda-and-fexpr capture machinery to class-typed values; not currently explicit in lambda-and-fexpr §§5–6 but implicit. The touch-up to lambda-and-fexpr §§5, 6 is registered in the work plan §6.6 batch.

### 4.7 Variant-Class-Typed Interaction (OQ-33)

When a variant value (a 3-word slot per type-system §3.4) is passed as a class-typed parameter (Case B, expecting a 3-word slot per §4.3), an open question surfaces: does the variant's existing 3-word slot get **reinterpreted** in place (since the variant slot's witness happens to be the right witness for the class-typed parameter, when class-membership permits), or is a **new 3-word slot allocated** for the class-typed parameter, with the value wrapped into it? *[CP017 §4.4.]*

**Reinterpretation is more efficient** but requires the variant's spans-class to *be* (or be a parent of) the class the parameter expects. Concretely, if the variant is declared as `variant Drawable spanning Showable`, and the class-typed parameter expects `Showable`, the variant slot's witness is already a `Showable` witness for the active candidate's type — no rewrap needed.

**Wrapping always works** but pays a slot-construction cost at the call site: a fresh 3-word slot is allocated, the variant's value-pointer is copied (or re-pointed), the witness is copied (or recomputed), and the new slot is supplied as the parameter.

The dialog registered this as **OQ-33** and deferred resolution. The choice has runtime-performance and implementation-complexity implications but does not change the user-facing surface — the call is well-formed either way; the question is purely how the runtime represents it. Recorded in work plan §5.4 (genuinely-open). *[CP017 §§4.4, 14.1.]*

*Sources for §4: CP001 §§4.3 (dictionary passing, hidden-parameter witness, byte-faithful domains, monomorphization-as-optimization); CP017 §§3 (Cases A and B, type-variable scope, mode markers), 4 (3-word slot pattern, RTTI, witness flow with captures, OQ-33), 6.5 (witness selection at construction site under permissive orphans).*

---

## 5. Single-Class Dispatch

This section covers the call-resolution sequence for a single-class dispatch — the basic mechanism through which a `(receiver :: method)` expression resolves to an actual command invocation. The mechanism is structurally simple; the complexity lives in the surrounding rules (instance coherence §8, overloading §9, witness flow §4) that determine *which* dispatch is selected at a given site.

### 5.1 The Call-Resolution Sequence

For an expression of the form `(receiver :: method)`, where `receiver` is a value of some type `T` and `method` is the name of a class member: *[CP001 §4.5; op-sem §5.6.]*

1. **Identify the class.** The typechecker determines which class `method` belongs to. In the absence of overloading (§9), this is uniquely determined by `method`'s name across the visible classes. With overloading, the rules of §9 apply (most-specific-receiver wins; cross-class collisions disambiguated by `{C::method}`).
2. **Identify the dispatch type.** The typechecker determines `receiver`'s static type for dispatch purposes. For Case A (type-variable-bound), this is the type variable's bound concrete type, known at the call site. For Case B (existential), this is the runtime type carried in the slot's tag (read at runtime). For ordinary (non-class-typed) receivers, this is the receiver's declared static type, modulo the domain-specific-dispatch rule of §8.3.
3. **Look up the dictionary.** The runtime resolves the `(class, dispatch-type)` pair to a dictionary, either by hidden-parameter witness (Case A; the dictionary was supplied at the call to the enclosing polymorphic command) or by the slot's witness component (Case B; the dictionary is in the 3-word slot).
4. **Invoke the dictionary's method.** The dictionary's slot for `method` holds a command-typed value; the language emits an indirect call through it, supplying `receiver` as the receiver argument.

The sequence is the same whether the dispatch site is a free-standing call (`(myColor :: render)`), a method call inside another method's body (`(other :: hash)`), a partial-application binding (§7), or a v-command receiver-position dispatch (§6). What varies is *how the dictionary is located* — Case A vs. Case B — not the sequence's overall shape.

### 5.2 Class-Resolution Yields a Command-Typed Value

The expression `(receiver :: methodName)` resolves the class method `methodName` on `receiver` and produces a **command-typed value with the receiver baked in**. The result's type matches the class's declaration of `methodName`, with the receiver position elided. *[CP001 §4.5; op-sem §5.6.]*

For `class Loggable :: log: String 'r, String message` and a value `myLogger: ConsoleLogger` with `instance Loggable: ConsoleLogger`, the expression `(myLogger :: log)` has type `:<String 'r, String>` — the receiver position has been elided; the productive `'r` and IN `message` parameters remain. The value is invokable as if it were a regular command of that signature.

This is the receiver-elision case of partial application (§7) — the simplest form, with only the receiver bound. The mechanism that supports it is the same partial-application machinery generalized in §7. The receiver-elision case is so common that it has the dedicated `(value :: name)` surface; broader partial-application uses the command-reference form `{value :: name: arg, _}` from §7.

### 5.3 The Tight-Loop Optimization

A class-method dispatch can be hoisted out of a loop by binding the resolved command to a local variable: *[CP001 §3.4; op-sem §5.4.]*

```
# logFn <- (myLogger :: log)            ; resolve once
loop ...
    logFn: 'discarded, ("iteration " + i)        ; direct invocation, no dispatch
```

The dispatch occurs once when `logFn` is bound; thereafter `logFn` is invoked directly through its captured command-value. The compiler may perform this hoisting as an optimization, but the language gives the programmer the explicit lever: binding the resolved command to a local makes the optimization explicit.

This pattern is safe when the receiver doesn't change *in a way that affects dispatch* over the loop's lifetime. The programmer holds the discipline; the language enables the move. Concretely: the receiver's type at `(myLogger :: log)` is captured at binding; if the receiver is reassigned to a different type during the loop, the bound `logFn` continues to dispatch through the original type's instance (because the dispatch was already resolved). This is usually what the user wants; if it isn't, the user re-binds `logFn` after the receiver-changing operation.

### 5.4 Boundary Against Op-Sem §5.6

Op-sem §5.6 owns the *operational* mechanism for class dispatch — what happens at runtime when a `::` is evaluated. This reference's §5 owns the *static rules* — what dispatch site means, how the class and dictionary are identified, how the result's type is computed. The two are complementary; the static rules describe the well-formedness of dispatch sites, the operational mechanism describes their execution.

A subtle but consequential touch-up registered in CP017 §13.2: op-sem §5.7 currently forwards v-command receiver-mode treatment "to the type-and-modes reference for full treatment." The type-and-modes reference does not actually consolidate R1+R2 explicitly. With this reference's §6 absorbing R1+R2 in full per CP017's structural decision, op-sem §5.7's R1+R2 forwarding redirects to class-system §6 instead of to type-and-modes. The redirect is a touch-up to op-sem §5.7, added to work plan §6.2.

*Sources for §5: CP001 §§3.4 (tight-loop optimization), 4.5 (class-resolution yields a command-typed value); op-sem §§5.4, 5.6 (operational mechanics of `::` and first-class command-typed values); CP017 §§5 (the call-resolution sequence as the static-rules side of op-sem §5.6), 13.2 (op-sem §5.7 R1+R2 redirect).*

---

## 6. V-Commands and Multiple Dispatch (Static Side)

A **v-command** takes a tuple of receivers — `(r1, r2, ...) :: cmd-name : args` — and dispatches based on the receivers' types together. The user-visible behavior is multiple dispatch in the spirit of Julia's open multiple dispatch. The implementation is composition of single-class dispatches, not joint-instance dispatch. *[CP001 §4.4.]*

### 6.1 Dispatch Composition, Not Tuple-Keyed Tables

A v-command's body is authored against the classes its receivers individually satisfy. The body invokes class methods on each receiver as needed; each such invocation is an ordinary single-class dispatch (§5) using the receiver's own class dictionary. The combined behavior of a v-command call is the product of its receivers' types — different receiver-type combinations produce different behaviors — but no jointly-keyed dictionary exists in the system. *[CP001 §4.4.]*

The concrete example from CP001: given a v-command `(logger, severity) :: format`, calling it with `(consoleLogger, warning)` and with `(consoleLogger, error)` will produce different behaviors *because* `Warning` and `Error` are distinct domains satisfying the `Severity` class, and their `Severity`-class methods dispatch to different implementations. The v-command's body sees those different behaviors when it invokes `Severity` class methods on the second receiver.

The consequences *[CP001 §4.4]*:

- Single-receiver v-commands are class-method dispatch on that receiver.
- Multi-receiver v-commands are dispatch composed from per-receiver class lookups.
- There is no tuple-keyed dispatch table.
- Coherence is the union of single-class coherence properties (§8); no separate multi-parameter coherence theory is needed.
- Separate compilation works cleanly. Module A defining `Logger` and module B defining `Severity` need not coordinate: a v-command using both classes resolves each independently.

This design is deliberately structured so that "open multiple dispatch like Julia's" is achievable without the joint-instance complexity that breaks separate compilation and modular reasoning.

### 6.2 Receiver Modes and the R1+R2 Rules

V-command receivers are explicitly mode-marked, like all writeable parameters. There are no implicit defaults: every receiver in every signature shape carries an explicit mode marker. *[CP005 §2; carved here in full per CP017 §13.2.]*

V-command receivers follow two rules that together replace the simpler "receivers are reference parameters" instinct:

**Rule R1 (call-site obligation).** At a v-command call site, every receiver must be initialized — independent of the receiver's mode. Dispatch fundamentally requires the receiver to exist as a real value at runtime, because the dispatch mechanism resolves a method-bearing value from the receiver's type and invokes it on the receiver's value. Dispatching on an uninitialized slot is meaningless.

**Rule R2 (callee-body obligation).** A v-command's body, with respect to a receiver of mode M, has the same callee-side obligations as a parameter of mode M would, per CP004 §3:

- **Productive `'`** — must write on every successful return path.
- **Reference `&`** — no write obligation; may read and may write.
- **IN (no marker)** — may read; may not write.

R1 lifts the caller-side obligation uniformly across all receiver modes (always initialized at the call site). R2 keeps the callee-side variation that the writeability marker is for. The two rules together say: dispatch always operates on a real receiver, and the marker tells the callee what it commits to doing with that receiver. *[CP005 §2.1.]*

### 6.3 Receiver Modes and Their Idiomatic Uses

Each of the three v-command receiver modes has a distinctive idiomatic use *[CP005 §2.2]*:

**IN receiver — externalized effect system.** An IN-mode receiver is a method that operates *through* the receiver without modifying it. The classic case is logging: `logger :: log: message` writes a log entry. The logger's state is unchanged; the world (the log file, the log stream, whatever the logger is connected to) is changed. The receiver mediates an effect external to itself — the effect happens through the connection-state the receiver carries, not on the receiver itself. This pattern preserves the no-non-local-state principle: the receiver is a parameter, so accessing its connection-state is local; the external effect happens through some lower-level command that itself takes the connection-state as a parameter.

Other examples of IN receivers: network sockets used to send (the socket is the channel, the bytes are the effect on the wire); metrics emitters; event publishers; file handles in append-only mode where the handle is itself unchanged.

**Reference `&` receiver — modify the receiver in place.** The method may read the receiver's current state and may modify it. Pre-call: initialized. Post-call (success or failure): initialized, possibly with a different value. This pattern fits cleanup methods, state transitions in objects, in-place updates, and the "modify if needed" idiom.

**Productive `'` receiver — re-initialize the receiver.** The method commits to writing the receiver on every successful return path (R2 for productive). Combined with R1, the receiver must be initialized pre-call and the method overwrites it on success. This pattern is unusual — it is specifically a "factory method" or "complete reset" operation that happens to dispatch on the receiver's existing type. The pattern is preserved as a coherent option rather than ruled out, on the principle that clean rules are better than special-case wrinkles.

### 6.4 Receiver Modes by Signature Shape

The valid receiver modes vary by signature shape. The full table *[CP005 §2.5]*:

| Signature shape | Valid receiver modes | Forbidden modes |
|---|---|---|
| Constructor (`DEF_CMD_CTOR`) | productive `'` only | reference, IN |
| V-command (`DEF_CMD_VCOMMAND`) | productive `'`, reference `&`, IN | none |
| At-stack `@` (`DEF_CMD_RECEIVER_ATSTACK`) | reference `&`, IN | productive |
| At-stack `@!` (`DEF_CMD_RECEIVER_ATSTACK_FAIL`) | reference `&`, IN | productive |

In all cases the marker placement is identifier-shape (the mode marker prefixes the name; the colon in `(T:Class)` is reserved for the class-constraint form per §4.4.4) per CP005 §1.

The reasoning behind the constructor-only-productive rule *[CP005 §2.3]*:

- A reference-mode constructor receiver would mean "construct drawing on the receiver's existing state" — which is not construction; it is in-place modification, which is what reference v-command methods are for.
- An IN-mode constructor receiver would mean "construct a thing the caller cannot observe" — forcibly producing an inaccessible object, which has no purpose in the language.

The reasoning behind the at-stack-no-productive rule *[CP005 §2.4]*:

- At-stack methods run at frame exit on objects that exist (otherwise there would be nothing to clean up). Productive mode would mean "construct the object as part of cleanup," which is meaningless — the object's existence is the precondition for cleanup, not its outcome.
- IN at-stack receivers are useful for cleanup that does not modify the object — for example, "log this object's final state at frame exit." The object is observed but not changed.
- Reference `&` at-stack receivers are the typical case — cleanup that may inspect, finalize, or mark the object as released. Resource-managing types use this form.

### 6.5 V-Command Type-Variable Receivers

Type variables introduced at v-command receiver positions are in scope for non-receiver parameters in the same signature, per §4.4.3. This admits patterns where a receiver's class instance constrains a non-receiver parameter to the same concrete type:

```
.cmd combine: (T:Mergeable) 'r, T other = ...
```

The receiver `r` is a productive `T`-typed slot for some `Mergeable` instance type; the non-receiver `other` is constrained to the same `T`. The dictionary for `(Mergeable, T)` flows once as the hidden-parameter witness; both `r` and `other` are in their natural representation (since `T` is statically bound at the call site). The composition with R1+R2 is uniform: `r` (productive receiver) must be initialized at the call site (R1) and is written on every successful return path (R2); `other` (IN) is initialized at the call site and read-only in the body.

The grammar extension admitting `(T:Class)` qualifiers in receiver positions is part of OQ-23's implementation-thread batch.

### 6.6 V-Command Static-Side Coherence

V-command coherence is the union of single-class coherence properties (§8). Each receiver position's class dispatch resolves independently against that receiver's own class hierarchy and instance set; ambiguities or duplicates are diagnosed per the single-class coherence rules at each position. *[CP001 §4.4.]*

The composition has structural consequences worth carving in:

- **A v-command call is well-formed iff each receiver position's single-class dispatch is well-formed.** No additional cross-receiver well-formedness check is needed.
- **A v-command body's static analysis composes single-class dispatches per receiver.** The body's static analysis sees each `(receiver :: method)` invocation as an ordinary single-class dispatch.
- **Overloading on v-commands** (per §9.7) follows the most-specific-receiver-tuple-match rule across the receiver tuple as a whole; ties are compile errors.

The boundary against op-sem §5.7: this section's §6 owns the *static rules* governing v-command well-formedness (signature shape, receiver modes, type-variable scope, dispatch composition); op-sem §5.7 owns the *operational mechanism* (how the runtime sequences the dispatches). The redirect of op-sem §5.7's R1+R2 forwarding from type-and-modes to here is registered in work plan §6.2.

*Sources for §6: CP001 §4.4 (v-commands as composition of single-class dispatches; multiple-dispatch behavior); CP005 §2 (R1+R2, receiver modes per signature shape, idiomatic uses); CP017 §§3.5 (v-command type-variable receivers), 13 (boundary against op-sem); op-sem §5.7 (operational composition).*

---

## 7. Partial Application Beyond Receiver-Elision

OQ-6 — partial application beyond the receiver-elision case `(receiver :: name)` — is resolved in this section. Partial application is a **command-reference** phenomenon (the bare `{...}` form per lambda-and-fexpr §3); other forms (command literals, lambdas, fexprs) do not engage in partial application; they have separate semantics (closure, body capture). *[CP017 §8.1.]*

### 7.1 The Resolution

**Receivers are always specified, never deferred.** For both single-receiver dispatched commands and v-commands, every receiver position in a partial application is bound at the partial-application site. This resolves dispatch (selects the witness) at that moment and captures it in the command-reference's closure environment. *[CP017 §8.1.]*

**Non-receiver parameters may be applied or deferred.** Deferred parameters are marked with `_`; the resulting command-reference takes them as its remaining parameters at invocation time.

**The resulting command-typed value's type covers only the deferred parameters,** in their declaration order. Bound positions are not part of the resulting type; they are part of the value's identity (its captures or bound args).

### 7.2 Surface Examples

```
{r :: cmd: x, _}              ; receiver r applied; x applied; last param deferred
{(r1, r2) :: vcmd: _, y, _}   ; v-command both receivers applied; first and third deferred
{cmd: x, _, z}                ; non-dispatched command; second param deferred
```

The v-command tuple form for multiple receivers requires grammar-extension treatment in OQ-23's batch. *[CP017 §8.2.]*

The bare `{cmd: x, _, z}` form is the non-dispatched-command case: the underlying command has no `::` receiver, just regular parameters; the partial application binds first and third and defers the second. The resulting type covers only the second parameter.

### 7.3 Mode-Marker Filter

Mode markers on the underlying command's parameters interact with partial application as follows *[CP017 §8.3]*:

- **PRODUCE parameters cannot be applied; only deferred.** A productive parameter is a slot the callee writes to, owned by the eventual caller of the invocation. Binding a productive parameter at partial-application time would be capturing a slot to write to later — closer to a lambda's `&` capture than to partial application; it crosses the four-form taxonomy boundary (lambda-and-fexpr §3.7). The partial application discipline forbids it.

- **REFERENCE parameters, if applied, capture the reference and engage ceiling-tracking.** A `&y` parameter applied at the partial-application site captures the slot reference; the resulting command-reference is ceiling-bound to that slot's origin frame, just like a reference-bearing lambda capture (lambda-and-fexpr §5.6). The full ceiling-rules from lambda-and-fexpr §8 apply.

- **READ / IN parameters may be applied or deferred freely.** No mode-related constraint; the value is captured at partial-application time (for IN) or carried as a remaining parameter (for deferred).

The PRODUCE-deferred-only rule preserves the productive write-once-on-success contract: a deferred productive parameter is a regular productive parameter at the resulting command's invocation site, with all the standard CP013 / type-system §4.4 contract; an applied productive parameter would have indeterminate timing for its write, which the language does not admit.

### 7.4 Why Receivers Must Be Applied

The user's reasoning, mid-dialog after an initial unrestricted proposal: admitting deferred receivers would force the typechecker to track *deferred dispatch* — a form whose dispatch resolution awaits a future invocation when the receiver is supplied. *[CP017 §8.4.]*

The user's framing: "In a completely dynamically typed language that would be quite useful, but I think this would be insane for Basis's typechecker to deal with."

Requiring receivers to be specified resolves dispatch at partial-application time and captures the witness, leaving only static-type elision for the deferred parameters. This keeps the typechecker's reasoning local to the partial-application site: *which witness is captured here* is statically resolved; *what remains to be supplied* is purely a parameter-type question, identical to non-dispatched-command partial application.

The receivers-always-applied rule has a structural consequence worth recording: it ties partial application closely to the existing class-resolution mechanism. The simpler `(receiver :: name)` form (§5.2) is the receiver-only case of this broader partial application; the mechanism is uniform, with the special-cased surface for the most common case.

### 7.5 The `_` Token Across Uses

The `_` deferred-parameter marker joins existing uses of `_` *[CP017 §8.5]*:

- Variant absent-state introduction in Aggregate literals (per construction §3.5 / CP015): `${kind <- _}` introduces the variant in its absent state.
- Variant operand sides of `-<` (per construction §11 / CP015): `v -< _` clears the variant; `_ -< v` tests for non-absent.
- Deferred parameter in command-reference partial application (this section).

The unifying reading: **`_` means "no value here / position is empty / placeholder for absence-or-deferred."** Consistent across all three uses; a touch-up to construction §3 (the prose introducing `_`) is registered to recapitulate this unifying reading.

### 7.6 Sub-Question Resolution Map

OQ-6 (CP001 §6) resolves all three sub-questions *[CP017 §8.6]*:

| Sub-question | Resolution |
|---|---|
| (a) Surface | `_` deferral in command-reference form, with receivers always applied |
| (b) Resulting type | Covers only deferred parameters in declaration order |
| (c) Mode-marker interaction | PRODUCE deferred-only; REFERENCE applied captures with ceiling-tracking; READ/IN flexible |

The resolution interacts with OQ-18 (lambda visible-signature representation, resolved in lambda-and-fexpr §5.11) symmetrically: both lambdas and command references hide the "what's bound" detail and surface only the "what's invokable" type. A partial-application result has the underlying command's signature with the bound positions removed; the bound positions are part of the value's identity, not part of its visible type.

*Sources for §7: CP001 §6 (OQ-6 framing); CP017 §8 (full resolution: command-reference phenomenon, receivers-applied rule, mode-marker filter, `_` token unification, sub-question resolution); lambda-and-fexpr §§3, 5.11, 8 (command-reference form, OQ-18 resolution, ceiling-tracking).*

---

## 8. Instance Coherence (OQ-5)

This section resolves the four sub-questions of OQ-5 (CP001 §6) and codifies the cross-module coherence rule for class instances. The four sub-questions:

- **(a)** Are duplicate instances for the same `(class, type)` pair a static error globally, or are they ranked?
- **(b)** Is there an orphan-instance restriction limiting where instances for a given class can be declared?
- **(c)** Domain-specific dispatch — does class dispatch resolve on the most specific known type at the call site?
- **(d)** Cross-module overriding instances — what's the resolution rule?

The resolutions form a coherent whole, organized around a single bright-line rule unified across this section's §8.6 and §10's failure-tag-hierarchy treatment.

### 8.1 (a)+(d) — Duplicate Instances and Cross-Module Resolution

**Intra-module duplicate instances for the same `(class, type)` pair are a static error.** A module declaring two `instance C: T` declarations for the same `(C, T)` pair fails to compile. *[CP017 §6.1.]*

**Cross-module duplicates are resolved by Julia's "more specialized module wins" pragmatics.** When multiple modules declare instances for the same `(class, type)` pair, the use site sees competition between them; resolution is by specificity ranking on module hierarchy. Genuine ties at the use site are static errors.

The user's standing instruction in CP017: "Keep Julia's pragmatics unless we see an opportunity where Basis can improve without losing Julia's flexibility." The improvement is the import-time competition warning of §8.7; the core specificity-ranking rule is preserved. *[CP017 §6.4.]*

The intra-module rule is the standard discipline (no `(class, type)` may have two different bodies in the same module); the cross-module rule preserves Julia's flexibility (any module may declare any instance, with specificity resolving competition). Together they joint-frame OQ-5 (a) and (d): duplicates within a module are forbidden; duplicates across modules are ranked.

### 8.2 (b) — Orphan Instances: Permissive

An **orphan instance** is `instance C: T` declared in a module that defined neither `C` nor `T`. Three positions in the literature:

- **Haskell-style restrictive** — orphans forbidden, instances declarable only in `C`'s or `T`'s defining module.
- **Rust-style** — Haskell-style with newtype escape hatch (Basis's domain mechanism is the equivalent).
- **Permissive** (Julia/Scala-style) — orphans allowed, conflicts resolved by ranking or import-scope.

The dialog selected **permissive**: any module may declare any `instance C: T`. Conflicts at use sites resolve by §8.1's specificity ranking; ties are static errors. *[CP017 §6.2.]*

The reasoning: the restrictive forms forbid common patterns (a module wanting to provide a `Showable` instance for a numeric type from a numerics library it neither defined nor controls); the newtype escape hatch is verbose; the ranking-based resolution is well-understood from Julia and Scala practice. The cost — silent surprises when two modules both declare orphans — is mitigated by the import-time warning of §8.7.

### 8.3 (c) — Domain-Specific Dispatch on Runtime Identity

Class dispatch resolves on the **runtime domain identity** of the receiver, not on the static type the value was implicitly upcast to. *[CP017 §6.3; CP001 §2.2 / §6 OQ-5(c).]*

If `Inches` is a domain over `Int32`, and both `instance Showable: Int32` and `instance Showable: Inches` exist, an `Inches` value dispatches to the `Inches` instance — even when the calling context's static expectations would have allowed implicit upcast to `Int32`. The runtime value retains its domain identity through the upcast (type-system §5.2: upcast is type-acceptance, not value-rewriting); class dispatch sees the runtime identity.

This preserves the typeclass-with-newtype pattern (analog of Haskell's `newtype Sum = Sum Int` with `instance Monoid Sum`) which is one of the principal motivations for distinct domains. The dispatch-on-runtime-identity rule is what makes the pattern work: a value flagged as `Inches` continues to dispatch to `Inches`'s instance even when passed as a parent-typed parameter.

The composition with type-system §5.2 is clean: implicit upcast is type-acceptance (the static side admits the substitution), but the runtime value retains its domain identity (the runtime side knows what it actually is). Class dispatch is a runtime decision; it consults the runtime identity. *[type-system §5.2; the symmetry recorded here as the dispatch-side completion.]*

### 8.4 Specificity Ranking (the "More Specialized Module Wins" Rule)

When multiple visible instances compete for the same `(class, type)` pair at a use site, the resolution is by specificity ranking on module hierarchy, paralleling Julia's pragmatics *[CP017 §6.4]*:

- **More specialized module wins.** If module `B` is downstream of module `A` (imports `A`, transitively), and both declare `instance C: T`, module `B`'s instance wins at uses within `B` (and transitively in modules downstream of `B`).
- **Genuine ties are static errors.** Two modules at the same level of the import graph, neither downstream of the other, both declaring `instance C: T`, with both visible at a use site, produce an ambiguity. The user resolves by import discipline (don't import both, or wrap one in a domain-newtype).

The ranking rule is what gives the permissive-orphan policy (§8.2) its predictability: a use site's resolution is determined by which module the use site is in, and the most-specialized-module-in-scope rule produces a deterministic answer.

### 8.5 Witness-Mechanism Interaction with Permissive Orphans

A noticed coherence point: in Case B (class-as-existential), the witness is selected at the **construction site** — where the concrete value is first wrapped in a class-typed slot. Whichever orphan's dictionary is visible at construction is the one captured in the value's witness. Once attached, the witness travels; downstream sites use the captured dictionary regardless of what orphans they see. *[CP017 §6.5.]*

This is a *feature* of the permissive policy: it localizes the orphan-selection decision to the construction site, reducing "action at a distance" worry. Whoever builds the value decides which orphan they're using; consumers receive the witness already chosen. The construction site's instance set is what determines the captured witness; downstream instance variations cannot retroactively change the witness.

For Case A (type-variable-bound), the witness flows as a hidden parameter and is selected at the **call site**, so the call site's visible orphans determine which is used. Same locality principle, different selection point. The Case A behavior matches Haskell's standard typeclass dispatch; the Case B behavior is novel to Basis's existential-by-default class-typed parameters.

### 8.6 The Bright-Line Rule

The dialog committed to a single bright-line rule across class instance declarations and failure-tag hierarchy extensions: *[CP017 §6.1.]*

> *Any module may declare any well-formed extension — class instance, failure-tag child, or other declaration that extends a parameterized type contract. Where multiple visible declarations compete at a use site, specificity ranks the winner; genuine ties are static errors. Per-extension well-formedness constraints apply: instances need intra-module uniqueness (intra-module duplicates for the same `(class, type)` pair are static errors); failure-tag children need payload covariance (OQ-29, §10).*

The unification matters because the two extensibility mechanisms — declaring class instances and declaring failure-tag children — solve structurally similar problems (extending a parameterized contract from a module other than the contract's home), and unifying their rules produces a coherent cross-module discipline. The discipline: extensions are permitted; competition is ranked; well-formedness is per-mechanism. *[CP017 §6.1.]*

### 8.7 Import-Time Competition Warning

Julia's pragmatics can produce silent surprises when a newly-imported module changes which instance dispatches under one's nose. Basis's improvement: at the static-analysis level, **warn at import-time** when a new instance competes with an existing one for an already-used `(class, type)` pair within the importing module. *[CP017 §6.3.]*

The warning surfaces the conflict at import rather than at use-site, restoring local predictability without restricting what can be declared. The user's framing: "preserve Julia's flexibility, improve Julia's pragmatics where possible." The mechanism keeps the permissive policy intact; the surface area for surprise is reduced.

The warning fires when:

- The importing module's source already used `(class, T)` somewhere (either through its own declarations or through a chain of imports).
- A new import brings a competing `instance C: T` (an instance for the same `(class, type)` pair from a different module).
- The new instance competes with an already-visible one (a competing instance was already in scope before the new import).

The warning's exact wording, severity (warning vs. error), and any opt-out mechanism are forwarded to the typechecker-implementation thread. The semantic point recorded here is that the language requires this warning at compile time; the implementation specifies how the warning is presented.

*Sources for §8: CP001 §§4 (instance declarations, dictionary), §6 (OQ-5 four-sub-question framing, OQ-5(c) user intent on domain-specific dispatch); CP017 §6 (full OQ-5 resolution; bright-line rule; orphan-instance permissive policy; import-time warning); type-system §5.2 (implicit upcast as type-acceptance, not value-rewriting); class-system §4 (witness construction at construction site).*

---

## 9. Overloading on Dispatched Commands (OQ-16)

OQ-16, registered in CP001 §6 and refined in CP007 §7, asks whether overloading is permitted on commands subject to class dispatch — and if so, under what restrictions. *[CP017 §9.]*

The dialog selected the maximal-permission direction: overloading is admitted across all three layers. The user's framing: "Given how generally useful overloading is, my inclination is to draw the bright line in the other direction: overloading is supported, and any unresolved ambiguity is an error." *[CP017 §9.1.]*

### 9.1 Three Layers of Overloading

**Within-class.** A class may declare multiple methods sharing a name with different parameter shapes:

```
.class Stringy
    declare format: String 'r, Int32 i
    declare format: String 'r, Float64 f
    declare format: String 'r, Bool b
```

The class declares three `format` methods, distinguished by their second parameter's type. An instance for some type `T` provides bodies for all three; dispatch at a call site selects the appropriate body by argument-type matching. *[CP017 §9.1.]*

**Cross-class.** Multiple classes may declare methods sharing a name; types satisfying multiple of these classes can use any:

```
.class Renderable
    declare process: String 'r

.class Computable
    declare process: Int32 'r
```

A type `T` with both `instance Renderable: T` and `instance Computable: T` admits both `(t :: process)` calls; the call site disambiguates by argument-shape matching, or by the explicit `{C::process}` qualification (§9.4) when the argument-shape is ambiguous. *[CP017 §9.1.]*

**Non-dispatched.** Regular (non-class-method) commands may share names with different signatures:

```
.cmd doIt: Int32 'r, Int32 x = ...
.cmd doIt: Int32 'r, Float64 x = ...
```

Overload resolution at a call site picks the appropriate body by argument-type matching, just as for within-class overloading. *[CP017 §9.1.]*

### 9.2 The Resolution Rule

**Most-specific-candidate wins by joint matching across receiver and argument types.** Genuine ties — multiple candidates equally specific — are compile errors. *[CP017 §9.2.]*

This parallels §8.4's "more specialized module wins" framing applied to overload resolution: same bright-line, different domain. The specificity ordering on types — what makes one type more specific than another — is determined by the buffer-backed subsumption hierarchy (a child domain is more specific than its parent), the class membership hierarchy (a concrete type is more specific than a class it satisfies), and structural type identity (any type is more specific than a class-typed existential).

For a call `(receiver :: methodName: arg1, arg2)`, the typechecker:

1. Identifies all visible methods named `methodName` from classes the receiver satisfies (or all visible commands named `methodName` for non-dispatched calls).
2. Filters to those whose parameter types are compatible with the actual argument types (allowing implicit upcasts per type-system §5.2).
3. Ranks the surviving candidates by specificity (a candidate `C1` is more specific than `C2` if `C1`'s parameter types are uniformly at-or-below `C2`'s under the type hierarchy).
4. Selects the most specific. If there is a unique most-specific, dispatch resolves to it; if there is a tie at the most-specific level, the call site is ambiguous — a compile error.

### 9.3 Receiver-Type Overloading Is Already Class Dispatch

A class method can have different bodies for different receiver types — that's how typeclasses work. So "overloading on receiver type" is already the standard mechanism; what's being added by OQ-16's resolution is overloading on *non-receiver argument* types (within a single class) and on cross-class collisions. *[CP017 §9.6.]*

The clarification matters because the unfamiliar reader might frame "overloading on dispatched commands" as "varying behavior by receiver type" — which is just dispatch. The novel content of OQ-16 is the additional dimension: varying behavior by *non-receiver* argument types as well, with the most-specific-match rule covering the joint matching.

### 9.4 The `{C::method}` Disambiguation Form

When a type satisfies multiple classes with same-named methods, ambiguity at the use site is an error unless the user disambiguates via the curly-brace class-qualification form: *[CP017 §9.3.]*

```
t :: {C::process}                    ; explicit qualification: C's process
t :: {C::process}: arg1, arg2        ; with arguments
```

The form `{C::process}` is itself a command reference per the four-form taxonomy (lambda-and-fexpr §3) — "the unbound command reference for `process` from class `C`." Combining with `t ::` produces "apply that command reference on receiver `t`." The double `::` (one for class-member access in `C::process`; one for class-dispatch in `t :: {...}`) is internally consistent with OPS §1.7's multi-role framing of `::`.

The form is a deliberate reuse of the four-form taxonomy: the curly braces signal "command reference"; the `C::process` inside is class-namespace-scoped name-resolution; the `t ::` outside is class-dispatch on `t`. No new syntactic mechanism; just composition of existing ones. *[CP017 §9.3.]*

### 9.5 Argument-Shape Disambiguation by Call-Site Types Alone

**No syntactic shape qualifier (`{C::method[shape]}`) is admitted.** Argument-shape disambiguation comes from the call-site argument types alone. *[CP017 §9.3.]*

The user's framing: "Let the disambiguation come from the call site in the number and type of supplied parameters, and if any ambiguity results, then the partial application fails at compile time." Maintaining a single disambiguation form (class qualification) keeps the bright line.

The rule's consequence: at a call site `(t :: {C::process}: arg1, arg2)`, the qualification picks which class's `process` is used; the argument types pick which overload of `C::process` is used. If multiple overloads of `C::process` match the supplied arguments, the call site is ambiguous unless the user supplies more arguments (narrowing the candidate set) or renames one of the colliding methods.

### 9.6 Partial Application With Overloading Is Compile-Error-on-Ambiguity

In partial application (§7), deferred parameters have unknown values but known *static types* (from the underlying command's signature). Overload resolution at partial-application time uses the static types of *all* parameters (deferred and applied) to pick the overload. *[CP017 §9.4.]*

If multiple overloads match, the partial application is ambiguous — a compile error. The user disambiguates by:

- Applying more parameters (narrowing the set of matching overloads).
- Renaming one of the colliding methods.
- Using the `{C::method}` form for cross-class collisions.

This rule is what makes overloading and partial application compose: the partial-application result has a determinate type, which means the overload is resolved at partial-application time, which means the resulting command-reference's identity is fully determined.

### 9.7 V-Command Interaction

Two v-commands with the same name and different receiver-tuple shapes? The same overload-resolution rule applies — most-specific-receiver-tuple-match wins; ties are compile errors. V-commands aren't class members in the same way as single-receiver methods, but the overload discipline is uniform across both. *[CP017 §9.7.]*

The receiver tuple `(r1, r2, ...)`'s joint specificity follows the same component-wise specificity ranking as the argument-type matching of §9.2. A v-command call `(r1, r2) :: format` with `r1: ConsoleLogger, r2: Warning` and visible v-command declarations for `(Logger, Severity) :: format` and `(ConsoleLogger, Severity) :: format` selects the second (more specific in the first receiver, equally specific in the second).

### 9.8 Boundary Against Dispatch (§5)

Overload resolution operates *before* dispatch resolution (§5.1's call-resolution sequence). The typechecker first picks which method is being called (§5.1 step 1, augmented by overload resolution per this section); then identifies the dispatch type and looks up the dictionary (steps 2–3); then invokes (step 4). The two stages are layered: overload resolution is static (compile-time), based on argument types; dispatch resolution may be static (Case A) or runtime (Case B), based on runtime witness availability. *[Reconciliation: the layering of overload resolution before dispatch resolution is implicit in the §5.1 call-resolution sequence but worth recording explicitly to avoid the temptation to fuse them.]*

For Case A type-variable-bound parameters, the overload-resolved method's signature is what governs the type-variable's binding; the dispatch-type is the call site's known concrete type for `T`. For Case B existential parameters, the overload-resolved method's signature is what governs the slot's expected class; the dispatch-type is read at runtime from the slot's tag. The two stages compose without interference.

*Sources for §9: CP001 §6 (OQ-16 framing); CP007 §7 (the original candidate-restriction framing); CP017 §9 (full OQ-16 resolution: three-layer admission, most-specific-match rule, `{C::method}` disambiguation, partial-application interaction, v-command interaction); class-system §§5, 7 (dispatch and partial application).*

---

## 10. Liskov-Style Opening of the Failure-Tag Hierarchy (OQ-29)

OQ-29, originally registered in type-system §7.9 (and in failure-system §7.6 as OQ-28's refined form), asks whether the failure-tag hierarchy can be opened for cross-module extension under a covariance constraint that preserves Liskov substitutability. *[CP012 §2.4 closed the hierarchy as the initial conservative position; OQ-28 / OQ-29 propose opening it.]*

This section resolves OQ-29 by reframing it in class-instance terms (the prior buffer-backed-subsumption framing was incorrect), landing the current concrete rule of *same-class equality*, and recording the forward-extension hedge for *class-implication* once a class-hierarchy mechanism is admitted.

### 10.1 Reframe From Buffer-Backed-Subsumption to Class-Instance Terms

A recurring miscalibration in the dialog (corrected mid-session): the prior framing of OQ-29's payload-covariance constraint in buffer-backed-subsumption terms (per type-system §7.9) is **incorrect**. *[CP017 §10.1.]*

Buffer-backed subsumption is a relation between buffer-backed types (records subsuming to `[N]`, domains subsuming to their parents); it is not a subtyping mechanism for failure propagation. The correct framing follows from failure-system §4.3 (already correctly stated there): *[CP017 §10.1; failure-system §4.3.]*

- A payload-bearing failure tag binds to a **class** (a Haskell-style typeclass), not a concrete type.
- A `.fail Tag: value` site supplies a value whose concrete type has an instance of that class.
- The class witness flows in the failure slot's third word.
- The handler `|: Tag t` operates on `t` through the bound class's operations only; concrete type is opaque.

Buffer-backed values can be supplied as payloads only if their type has an instance of the bound class — *instance-of, not subsumption*.

The reframe matters because the buffer-backed-subsumption framing accidentally suggested a different mechanism: that a child failure tag's payload could be a subtype (in the buffer-backed sense) of the parent's payload. That framing is wrong on two fronts: failure-tag payloads are not buffer-backed values in general (they are class-bound, with concrete types varying per `.fail` site), and Liskov substitutability for failure propagation is governed by class-membership, not by buffer-backed subsumption. *[CP017 §10.1; correction from prior framing.]*

### 10.2 The Constraint: Same-Class Equality (Current); Class-Implication (Future)

Under the "covariance as the principle, same-class as the current concrete rule" framing the user requested *[CP017 §10.2]*:

**Current rule.** When tag `A` extends tag `B`, `A`'s payload class **equals** `B`'s payload class. The hierarchy narrows only on the tag; payload classes don't vary. This is Liskov-trivially-preserved: a handler keyed at `B` operates on the payload through `B`'s payload class's operations; an `A`-tagged failure carries a payload satisfying the same class; the handler's operations are well-defined on that payload.

**Forward-extension hedge.** When the language admits class-level hierarchies — superclass constraints (e.g., "class `D` requires instances to also satisfy class `C`"), combined-class composition, or another mechanism for expressing "class C_A implies class C_B" — the rule relaxes to "A's payload class implies B's payload class." Payload variation down the failure-tag hierarchy becomes admissible at that point. *[CP017 §10.2.]*

The covariance phrasing is preserved in the specification language as a forward-compatibility hedge, separating the *principle* (covariance) from the *current rule* (same-class equality, the trivially-covariant case). When the future class-hierarchy mechanism arrives, the rule changes from same-class to class-implication; the principle and the well-formedness check remain the same shape.

### 10.3 Inheritance Default From Ancestry

A child tag's payload specification is **inherited from its parent unless the child explicitly overrides**. Inheritance chains transitively up the hierarchy; "no payload" is itself a specification that propagates the same way. A payload-less root produces a payload-less hierarchy by default — declarations don't repeat the parent's payload class. *[CP017 §10.3.]*

The inheritance default reduces declaration burden: a hierarchy with a single payload class at the root and many leaf tags doesn't require each leaf to re-declare the payload class. The user writes the payload class once at the root; descendants inherit it.

### 10.4 Three Sub-Rules for Explicit Override

When a child explicitly overrides the inherited payload specification, three sub-rules govern whether the override is well-formed *[CP017 §10.4]*:

**(a) Widening from "no payload" to a typed payload at a descendant: admissible.** Parent-keyed handlers don't bind, so no contract is violated. The user's reasoning: *"binding of payloads in a recovery handler is always optional irrespective of whether the tag admits a payload. Thus, widening from `_` to `C` is harmless, because a well-formed parent tag bearing no payload would not have a payload binding in its recovery specification."*

A handler `|: ParentTag` (no binding) catching an `A`-tagged failure (where `A` is a descendant of `ParentTag` with payload class `C`) sees no `t` to bind; the handler operates as the no-payload form. The widening at `A` is invisible to `ParentTag`-keyed handlers.

**(b) Typed-payload-to-typed-payload variation: constrained by covariance.** Currently realized as same-class equality; future relaxation to class-implication. A child tag declaring a different payload class must have the child class implying the parent class (under the future relaxation); under the current rule, they must be equal.

**(c) Typed-payload-to-no-payload narrowing: forbidden.** Removes binding capability that ancestor handlers' contracts may rely on. The user's confirmation: *"(i) is ABSOLUTELY the only correct answer here."* The asymmetry with (a) is rooted in the optional-binding property: optional-binding means handlers can choose not to bind, but if they do bind, the binding must be honorable.

A handler `|: ParentTag t` (with binding) catching an `A`-tagged failure (where `A` overrides to no-payload) would see no value to bind to `t`, breaking the handler's contract. The narrowing at `A` is forbidden to preserve handler well-formedness.

### 10.5 Composition With OQ-5(c) Domain-Specific Dispatch

A noted coherence point: the witness flow through the failure slot composes cleanly with the runtime-domain-identity dispatch rule from §8.3 (OQ-5(c)). When `A` extends `B` and a handler keyed at `B` catches an `A`-tagged failure *[CP017 §10.5]*:

- **Static type of `t`:** `B`'s payload type (the class).
- **Runtime value of `t`:** an `A`-payload value (whose concrete type satisfies the same class, currently per same-class equality).
- **Witness in the failure slot:** for the runtime concrete type's instance of the shared class.
- **Class dispatch on `t`:** goes through the witness, dispatching to the runtime concrete type's instance.

The handler sees `t` as the class statically but dispatches on the actual runtime instance. §8.3's runtime-domain-identity rule and OQ-29's covariance rule are two faces of the same coherence requirement: the language uses witness-based dispatch in both cases, with the static-type-as-class providing the contract surface and the runtime-witness providing the actual dispatch target.

### 10.6 Failure-Tag-Class-Witness Slot Closure

The failure-tag's class witness binding is **separate** from the spans-class concept used for variants (§13, where the spans-class constrains all candidates). Failure tags don't have a "spans class" in the variant sense; each tag binds its own payload class (subject to the covariance rule down the hierarchy). *[CP017 §10.6.]*

The structural difference: a variant's spans-class is a property of the *variant declaration* (a single class that all candidates must satisfy); a failure tag's payload class is a property of the *tag declaration* (the class for that specific tag's payload, with inheritance and covariance down the tag hierarchy). The two mechanisms use the same 3-word slot pattern (§4.3, §13.1) but different binding disciplines.

### 10.7 Terminology Forward-Pointer

The user noted (parenthetically in CP017): *"need better terminology than 'tag' at some point."* The overload of "tag" across multiple uses *[CP017 §10.7]*:

- Failure-tag declarations and hierarchy
- Variant slot's tag identifier (the small integer identifying the active candidate)
- Failure slot's tag identifier
- Class-typed-value tag (RTTI, per §4.5)

is real and worth resolving in a future terminology pass. Recording this as a forward-pointer in this section's prose; resolution can come during a future consolidation pass when the terminology question is addressed.

### 10.8 Cross-Reference Distribution

The OQ-29 resolution authoritative source becomes this section. The pending touch-ups *[CP017 §15.1, §15.2]*:

- **Failure-system §7.6** — close OQ-28 (open under OQ-29's covariance constraint); reframe in class-instance terms (not buffer-backed subsumption); cross-reference here.
- **Failure-system §4.2** — add paragraph on the inheritance default from ancestry; the three sub-rules for explicit override.
- **Failure-system §4.3** — confirm framing as already correct; cross-reference here for the cross-module-extension treatment.
- **Type-system §7.9** — rewrite the OQ-29 statement in class-instance terms, not buffer-backed-subsumption terms; phrase as covariance principle with same-class as current concrete rule; note the forward-extension hedge.

These are batched in the work plan for application after this reference is settled.

*Sources for §10: CP012 §§1.2 (class-bound payloads, original framing), 2.4 (closed hierarchy, original conservative position), 3.1 (class-binding mechanics); failure-system §§4.2, 4.3, 7.6 (typed-failure design, class-bound payloads, OQ-28 framing); type-system §7.9 (prior buffer-backed framing of OQ-29; this reference's reframe supersedes); CP017 §10 (full OQ-29 resolution: class-instance reframe, same-class-equality current rule, three sub-rules for override, witness-flow composition with OQ-5(c)).*

---

## 11. Class-Method Fexpr-Typed Parameters and the Fexpr-Relevance Taint Axis

This section resolves the class-method-fexpr-typed-parameter sub-item of OQ-32 (lambda-and-fexpr §10.7) and codifies the **fexpr-relevance taint axis** as a substantive new design move parallel to the READ contract (OPS §1.3 / type-system §4.6).

The substantive design move emerged mid-dialog as an organizing idea after the fexpr-related edge cases of OQ-32 raised typechecking complexity concerns. The user's confirmation: *"Yes, let's formalize fexpr-tainting as the substantive design move."* It supersedes ad-hoc per-case fexpr rules with a uniform discipline. *[CP017 §11.1.]*

### 11.1 The Principle (Drafting for OPS §1.X)

The principle to be added to the operating-principles document:

> *Fexpr-relevance is a second taint axis, parallel to the READ contract (OPS §1.3). Taint sits on access paths, propagates per-frame, has no cross-frame propagation. A fexpr-relevance-tainted access path is ceiling-bound to the fexpr's defining frame `D`: it cannot escape via stores into longer-lived slots, via PRODUCE outputs to outer frames, or via capture by command-references whose own ceiling exceeds `D`. The taint flows from direct fexpr-typed values, from composite values containing fexpr-relevant content (variant candidates, command-reference bindings), and from captures of fexpr-relevant values. Per-frame analysis enforces the ceiling discipline uniformly across mechanisms.*

The principle is added to the operating-principles document as a new §1.X (placement to be settled when the OPS update is done; CP017 §15.6 batches the addition alongside the `-<` multi-role principle from §5 of CP017).

### 11.2 Sources of Taint

A fexpr-relevance taint flows from several sources *[CP017 §11.3]*:

- **Direct fexpr-typed values.** Parameters or locals with `<*>` types (lambda-and-fexpr §6.7).
- **Composite values containing fexpr-relevant content via structural composition.** Variants with fexpr candidates (currently disallowed per §11.5; future relaxation activates this), records or aggregates with fexpr-typed fields if such structures are admitted, captures of fexpr-relevant values in lambdas.
- **Command references with bound fexpr-typed arguments** (per §11.4).
- **Anything else whose lifetime is tied to a defining frame `D` because of fexpr containment.**

The taint propagates through the access-path machinery (parallel to the READ contract): an access path rooted at a fexpr-relevant value is itself fexpr-relevance-tainted; subsequent operations along that path inherit the taint. The discipline is uniform across mechanisms.

### 11.3 Enforcement

A fexpr-relevance-tainted access path *[CP017 §11.4]*:

- **Cannot be stored into a slot whose lifetime exceeds `D`.** Object fields, record fields embedded in long-lived structures, and similar are forbidden destinations.
- **Cannot be passed to a parameter slot whose ceiling is unbounded** (e.g., escape via PRODUCE outputs to outer frames). Productive parameters of the defining frame's caller are forbidden destinations.
- **Cannot be captured by a command reference that itself escapes `D`.** The command-reference's own ceiling must be at-or-below `D`.
- **Can be passed to other parameter slots within `D` and below**, where the receiving frame's ceiling can accommodate it. Sub-frame parameter slots are valid destinations.
- **Can be invoked** (the fexpr fires, producing values that may or may not themselves be fexpr-relevance-tainted depending on the body's productive output).
- **Can be stored in a local slot whose lifetime is bounded by `D`.** Local slots are valid destinations.

The enforcement composes with the READ contract uniformly: a path can be untainted, READ-tainted, fexpr-relevance-tainted, or both. Each taint is enforced independently; both must be respected.

### 11.4 Class Methods With Fexpr-Typed Parameters Are Admitted

Class-method parameters are slots; fexpr-typed slots are admitted generally per CP016 / lambda-and-fexpr §6. No new mechanism needed for the basic admission; the structural composition is direct. *[CP017 §12.1.]*

A class method declaring a fexpr-typed parameter has the standard fexpr-typed-parameter contract (lambda-and-fexpr §6 for fexpr semantics; lambda-and-fexpr §6.5 Restriction A: only IN mode permitted on fexpr-typed parameters). The instance's body for that method receives a fexpr-typed value whose `D` is the *caller's frame at the call site* — the frame in which the fexpr literal was written, which is the frame from which the class-method was invoked.

The instance's body cannot store the fexpr in a longer-lived slot, cannot pass it back to the caller via a PRODUCE output, cannot capture it in a command-reference that escapes the body's frame; the fexpr-relevance-taint enforces these constraints uniformly. Within the body's frame and any sub-frames, the fexpr can be invoked, passed downward, and stored in body-local slots — all the ordinary operations a fexpr-typed value supports.

### 11.5 Defaults Are Incompatible With Fexpr-Typed Parameters

A class method declared with a fexpr-typed parameter cannot have a default implementation. The user's framing: *"default implementations should not admit fexprs; that should be a compile-time error."* *[CP017 §12.2.]*

The fexpr-tainting machinery makes this naturally enforceable: a default body has no determinate `D` to bind the fexpr-relevance taint to. Instances inheriting the default would face an undefined frame relation; the only coherent rule is to forbid the combination. **Compile-time error if a class declaration combines a fexpr-typed parameter with a default body.** *[CP017 §12.2.]*

The rule's structural underpinning: every fexpr-relevance-tainted access path is ceiling-bound to *some* `D`. A default body has no determinate `D` because it is not associated with a specific call site; it is a template body that any instance may inherit. Without `D`, the ceiling-tracking discipline cannot be enforced; the language rejects the combination at the source level rather than admit a body whose fexpr handling is undefined.

The rule has implications for class evolution: a class that wants to admit fexpr-typed parameters in some methods cannot provide defaults for those methods. The methods must be `declare`-only, requiring every instance to supply a body. This is a stricter contract on instance authors but is the only coherent rule under fexpr-tainting.

### 11.6 Partial Application of Class Methods on Fexpr-Typed Parameters

The user's mid-dialog correction overruled an initial "deferred-only" framing: *"You might have a 'do _something_ X number of times' partial application where you bind the something to the fexpr and leave the number of times to do it unbound. So long as the fexpr's hard-ceiling rules are respected, I don't see a reason to preclude this."* *[CP017 §12.3.]*

So fexpr-typed parameters in partial application **may be applied** (bound at the partial-application site), with the resulting command reference inheriting fexpr-relevance taint and being ceiling-bound to the fexpr's `D`. Or they may be deferred (`_`) for later supply.

This supersedes CP016's commitment "command references cannot reference or bind fexpr-typed values" (lambda-and-fexpr §3.7) — the conservative-by-default rule is replaced by the fexpr-tainting machinery, which makes the binding tractable. *[CP017 §11.6, §12.3; lambda-and-fexpr §3.7 superseded.]*

The pending touch-up to lambda-and-fexpr §3.7 is registered in work plan §6.6: the section's text needs to be revised to admit fexpr-typed bindings via fexpr-tainting machinery, with a reconciliation marker citing CP017.

### 11.7 Variants With Fexpr Candidates: Tightened to Disallowed in v1

CP016 §6.5 (lambda-and-fexpr §6.5 Restriction C) admitted variants with fexpr candidates "permitted only when the variant inhabits a local slot." The user tightened this in the dialog: *"in theory, the fexpr candidate of the variant (which is sensible if the variant will be monomorphic) could come from a read parameter, but I'm concerned about the typechecking effort required to maintain the ceiling rules. Let's disallow fexprs in this version, with a note that support is intended later."* *[CP017 §12.4.]*

So in v1: **variants may not have fexpr candidates at all, regardless of slot location.** Future relaxation intended once the fexpr-tainting machinery matures and the typechecker can enforce ceiling-tracking on variant-borne fexprs.

The pending touch-up to lambda-and-fexpr §6.5 Restriction C is registered in work plan §6.6: tightening from the local-slot exception to the fully-disallowed rule, with a reconciliation marker citing CP017.

### 11.8 Failure-Set Treatment With Fexpr Invocations

A class method's declared failure set must accommodate fexpr invocations within its body, but the fexpr's specific failure profile is determined by the caller's code, not by the class declaration. The user selected the simple resolution: *"a simple standard 'fexpr failed' failure should suffice... I want to keep failure-sets clean and orthogonal."* *[CP017 §12.5.]*

So a class-method body's invocation of a fexpr-typed parameter wraps the fexpr's failures in a single standard failure tag — `FexprFailure`. The actual underlying failure is opaque to consumers; specificity is sacrificed for clean failure-set declarations.

`FexprFailure` is registered as a forward direction (a future "standard failure tag library" thread) parallel to `CoercionFailure` from §5.5 of CP017 (the `-<` operator's failure tag). Both are placeholder names for actual failure tags whose precise hierarchical position and payload-class-binding will be settled in that thread. *[CP017 §12.5.]*

### 11.9 OQ-32 Status

OQ-32 was registered in lambda-and-fexpr §10.7 with five sub-items:

- Fexpr-typed return-from-call edge channels.
- Class-method fexpr-typed parameter edge cases.
- Command-reference relaxation possibilities.
- Transitive variant-fexpr-candidate containment rules.
- `-<` operator's interaction with fexpr-typed slots.

The class-method-fexpr-typed-parameter sub-item is **resolved here** (this §11). The command-reference-relaxation sub-item is **resolved as a side effect** of the fexpr-tainting axis (§11.6 admits the binding under taint). The other three sub-items remain forwarded to future threads. *[CP017 §12.6.]*

### 11.10 Coherence With Existing Principles

The fexpr-tainting axis composes cleanly with existing principles *[CP017 §11.5]*:

- **OPS §1.2 (each frame's analysis is local):** fexpr-relevance taint is per-frame propagated; no cross-frame escape, just like READ taint. The discipline migrates to the frame with context.
- **OPS §1.3 (access paths, not storage):** taint sits on access paths, not on storage. Aliasing through different access paths is the user's responsibility (or, more precisely, the typechecker's per-path tracking handles it).
- **The two taints are orthogonal axes.** A path can be untainted; READ-tainted; fexpr-relevance-tainted; both. Each is enforced independently.

The orthogonality matters for the typechecker's implementation: the two taint analyses are independent passes over the same access-path representation; the joint analysis is component-wise. This is the same composition pattern as the failure-mode-analysis × init-analysis × READ-analysis composition recorded in type-system §5.5.

*Sources for §11: lambda-and-fexpr §§6 (fexpr semantics, capture-by-free-name, the `D`/`I`/`F` frame model), 6.5 (fexpr restrictions including Restriction C variant-with-fexpr-candidate), 6.7 (the `<*>` typing surface), 10.7 (OQ-32 registration); CP016 (the lambda-and-fexpr scope dialog that admitted variant-with-fexpr-candidate in local slots and forbade command references binding fexprs); CP017 §§11 (fexpr-tainting axis as substantive new design move), 12 (OQ-32 sub-item resolution: admission, defaults-incompatible, partial-application admission, variant-tightening, failure-set treatment).*

---

## 12. Boundary Against Context Variables

This section codifies the boundary between class-system mechanisms (this reference) and context-variable mechanisms (construction §13). The two mechanisms are conceptually parallel — both are "implicit-resolution" machinery that supplies values to call sites without explicit syntax — but they operate in different layers and resolve by different rules.

### 12.1 The Two Mechanisms

The summary table from §3.5 (recapitulated and expanded):

| Mechanism | Resolution rule | Resolved at | Resolved against |
|---|---|---|---|
| **Instance dispatch** (this reference) | `(class, type)` lookup | Call site, statically (Case A) or runtime (Case B) | The class dictionary table |
| **Context-variable resolution** (construction §13) | Uniqueness-of-type from lexical scope | Call site, statically | The caller's lexical scope |

Both mechanisms are "implicit" in the sense that they supply values without an explicit syntactic argument at the call site. But what they supply, and how they pick what to supply, is structurally different. Conflating them is a recurring mental hazard the dialog corrected several times. *[CP017 §13.4; construction §13.8.]*

### 12.2 The Distinction

**Instance dispatch picks a method (a behavior).** Given a class `C` and a receiver type `T`, the dispatch machinery looks up the dictionary for `(C, T)` and invokes the relevant method. The dispatch is keyed by *type*; the value at the call site (the receiver) is the dispatcher.

**Context-variable resolution picks a value (a configuration object, a logger, etc.).** Given a parameter declared in the `/`-section of a command's signature, the resolution machinery searches the caller's lexical scope for a value whose type uniquely matches; if found, that value is supplied as the parameter. The resolution is keyed by *type*; the value at the call site is what is being supplied.

Both look up by type, but they look up *different things from different tables*: instance dispatch looks up a dictionary from the class-instance table; context-variable resolution looks up a value from the caller's lexical scope. The two never directly interact: instance dispatch picks a method (not a value); context-variable resolution picks a value (not a method).

### 12.3 The Two Compose Without Interference

A polymorphic command may have both class-constrained type-variable parameters (Case A, requiring witnesses via the hidden-parameter mechanism) and context parameters (in the `/`-section, requiring values via the context-variable resolution). The two are independent hidden-parameter slots in the underlying compiled signature: *[CP017 §13.4.]*

```
.cmd doIt: (T:Showable) 'r, T x / Logger logger = ...
```

The signature has:

- A regular productive parameter `'r` of type `T`.
- A regular IN parameter `x` of type `T`.
- A hidden-parameter witness for `(Showable, T)` — selected at the call site by the typechecker, supplied as a hidden first argument.
- A hidden-parameter `logger` of type `Logger` — selected at the call site by uniqueness-of-type from the caller's lexical scope, supplied as a hidden second argument.

The two hidden parameters are independent. The typechecker resolves the witness against the visible class instances; the resolver finds the context value in the caller's lexical scope. Either failing — no visible instance for `(Showable, T)`, or no in-scope `Logger` value, or multiple in-scope `Logger` values — produces a call-site error. Both must succeed for the call to be well-formed.

### 12.4 Class-Method Methods With Context Parameters

A class method may itself have context parameters in its signature's `/`-section. The class declaration includes the context parameters:

```
.class Loggable
    declare logIt: String 'r / Logger logger
```

An instance for some type `T` with `instance Loggable: T` provides bodies for `logIt` whose signature includes the same `/ Logger logger` context parameter. At a call site `(t :: logIt: 'result)`, the receiver `t` is supplied (per dispatch), and the context parameter `logger` is resolved from the caller's lexical scope by uniqueness-of-type.

This composition is regular: the class-method dispatch picks the body; the context-variable resolution supplies the `/`-section parameter; the body executes with both. The dispatch and the context-resolution happen at the same call site but through independent mechanisms.

### 12.5 Construction §13 Owns the Mechanics

Construction §13 owns the full mechanics of context variables — the resolution rule, the position-1 vs. position-2 framing, the `.implicit` exclusion, the defaults handling, the lambda-capture forwarding to lambda-and-fexpr §5.4. The class-system reference's involvement is purely the boundary statement: where the dispatch ends and the context-resolution begins.

The construction-side cross-reference (construction §13.8) explicitly states "the class-system reference has no involvement with OQ-13." OQ-13 (implicit context parameters and initialization) is fully resolved in construction §13. The class-system reference is *uninvolved* — it neither owns mechanics nor has hooks into them. *[construction §13.8.]*

The op-sem cross-reference (op-sem §5.5) currently forwards OQ-13 to "the class-system reference"; this is an artifact of an earlier stage of the work plan and will be redirected to construction §13 in the consolidated touch-up pass (work plan §6.2).

*Sources for §12: construction §13 (full context-variable mechanics, OQ-13 resolution); CP015 §§8.3, 8.5 (Position 1 decision; class-system uninvolvement); CP017 §13.4 (boundary statement, recapitulated here); class-system §3.5 (the boundary first introduced in §3); op-sem §5.5 (operational mechanics of context resolution; forwarding redirect pending).*

---

## 13. Variant Class-Witness Slot

A variant value's runtime representation is the 3-word slot pattern (type-system §3.4): tag identifier, candidate pointer, class witness. Type-system §3.4 declares the slot shape; this section owns the **details of the class witness component** — its population, propagation, consumption, and interaction with class dispatch on variant-borne values. *[type-system §3.4; CP017 §13 (boundaries).]*

### 13.1 The Slot Shape (Cross-Reference)

The variant slot's three components, as declared in type-system §3.4:

| Slot word | Content |
|---|---|
| Tag identifier | Small-integer identifier for the active candidate |
| Candidate pointer | Pointer to the candidate value's storage |
| Class witness | Typeclass dictionary for the active candidate's type with respect to the variant's spans-class, or null when no spans-class is in force |

The class witness component is what makes class dispatch on a variant's currently-active value coherent without the consumer knowing the concrete candidate type — the witness rides with the variant slot, and operations that need the candidate's class instance dispatch through it.

### 13.2 The Spans Class

A variant declaration may specify a **spans class** — a class that all of the variant's candidates must satisfy. The spans-class is a property of the *variant declaration*, not of any individual candidate. *[type-system §3.4; the spans-class concept introduced operationally here.]*

Surface form (placeholder, pending OQ-23 grammar settlement):

```
.variant Drawable spanning Showable :
    Circle
    Rectangle
    Polygon
```

The variant `Drawable` has three candidates (`Circle`, `Rectangle`, `Polygon`); each must have an instance for `Showable`. At any moment when a `Drawable` value is non-absent, its slot's witness is the dictionary for `(Showable, runtime-candidate-type)`.

A variant declared without a spans-class has a null witness component in its slot — class dispatch on the variant's active candidate is not supported because the language doesn't know which class is in force. The user must extract the candidate explicitly (via `-<` or pattern matching) and dispatch on the extracted concrete value. *[Reconciliation: the spans-class-or-null distinction is implicit in type-system §3.4's "or null when no class binding is in force" phrasing; sharpened here as an explicit property of the variant declaration.]*

### 13.3 Witness Population at Construction

When a variant value is constructed at one of its candidate types — `${Circle <- 1.0}` or similar (construction §9 / CP015 §3.5) — the witness is populated from the construction site's visible instances: *[CP017 §6.5; symmetric to §4.3.]*

- The candidate's concrete type `T_c` is known at the construction site.
- The variant's spans-class `S` is known from the variant declaration.
- The construction site's visible instances determine which `(S, T_c)` dictionary applies.
- That dictionary is captured in the slot's witness component.

The witness selection follows the same locality principle as Case B class-typed values (§4.3 / §8.5): the *construction site* picks the witness from its visible instances; downstream consumers receive the witness already chosen. The orphan-instance permissive policy (§8.2) interacts with variant construction the same way it does with Case B existentials: whichever orphan is visible at construction is captured.

### 13.4 Witness Propagation

A variant value's witness propagates through the language's normal value-flow mechanisms, in lockstep with the candidate pointer and tag identifier:

- **Copy-on-assign.** When a variant value is copied to a new slot (for instance, on a parameter copy at a call site), the three slot words are copied together; the witness is preserved.
- **Pointer indirection.** When a variant is accessed through a pointer, the pointed-to slot's witness is read; no new witness is computed.
- **Captures.** When a variant value is captured by a lambda or fexpr (§4.6), the witness travels with the captured value. For IN captures, the slot is copied into the lambda's hidden field. For reference captures, the captured slot reference reads the witness on each access.

The propagation is uniform with the rest of the slot's contents; there is no separate witness-propagation mechanism. *[Reconciliation: the propagation rules are implicit in the slot's value semantics but worth recording explicitly to anchor the variant-witness model.]*

### 13.5 Witness Consumption: Class Dispatch on the Active Candidate

A variant value `v: Drawable spanning Showable` admits class dispatch through the carried witness:

```
v :: render: 'output      ; dispatch Showable's render method on v's active candidate
```

The dispatch reads the variant slot's witness component; the witness is the dictionary for `(Showable, runtime-candidate-type)`; the indirect call invokes the appropriate `render` body for that candidate type. The user code is written against the spans-class `Showable`; the runtime dispatch resolves to the candidate-type-specific implementation through the witness. *[Reconciliation: this dispatch surface is implicit in type-system §3.4's commitment that "class dispatch on a variant's currently-active value [is] coherent... the witness rides with the variant slot, and operations that need the candidate's class instance dispatch through it"; codified here as the consumption rule.]*

The dispatch is well-defined only when the variant is non-absent. A variant in its absent state has a null candidate pointer and (consequentially) an undefined witness state; class dispatch on an absent variant is a runtime error in the same way that dispatching on an uninitialized receiver would be (R1 of §6.2 violated). The user's discipline is to test for absent before dispatching, typically via the `?- _ -< v` form (construction §11.4) or via pattern matching with absent-case coverage (construction §9.5).

### 13.6 Spans-Class as a Class-System Concept

The spans-class is a class-system concept, not a variant-specific concept. The language reuses its standard class machinery — class declarations, instance declarations, dictionary passing — to populate and consume the variant's witness. Three points worth carving in *[Reconciliation: the spans-class-as-class-system-concept characterization is implicit in the design but recorded here as the framing the class-system reference owns]*:

- **The spans-class can be any class**, including user-defined ones, including combined classes. The variant declaration constrains its candidates by the spans-class; the candidates are all required to have an instance for that class.
- **The spans-class's instances determine the candidate set's well-formedness**, not the candidate types' relationship to each other. Two unrelated types `Circle` and `Polygon` can both be candidates of `Drawable` because both have `Showable` instances; their unrelatedness is irrelevant.
- **The spans-class's methods are the operations available on the variant** (without explicit narrowing). `(v :: render)` dispatches through the spans-class; operations not in the spans-class require explicit narrowing (`-<`) or pattern matching.

### 13.7 Variant-Class-Typed Interaction (OQ-33 Forward Pointer)

When a variant value is passed to a class-typed parameter (Case B existential, §4.3), the interaction raises a question about slot reuse: does the variant's existing 3-word slot get reinterpreted as the class-typed slot (since both are 3-word patterns), or is a fresh class-typed slot allocated?

The reinterpretation case requires the variant's spans-class to be (or be a parent of) the class-typed parameter's expected class. The wrap case always works.

This is registered as OQ-33 (CP017 §§4.4, 14.1) and forwarded to a future design dialog. The class-system reference records the question but does not resolve it; the resolution is part of the runtime-representation work the implementation thread will eventually do.

### 13.8 Cross-Reference Distribution

Type-system §3.4 declares the variant slot shape; this section owns the witness population, propagation, and consumption details. Pending touch-up to type-system §3.4 (work plan §6.3): brief cross-reference to here acknowledging that the 3-word slot pattern generalizes to class-typed values; class-system §13 owns the witness-component details.

The op-sem reference's coverage of variant operations is at op-sem §5 (commands and invocation, where dispatch is introduced); the operational mechanics of variant dispatch follow the standard `::` mechanism (op-sem §5.6) augmented by the witness flow recorded in this section.

*Sources for §13: type-system §3.4 (variant 3-word slot shape; the witness commitment); CP017 §§13 (boundary against op-sem; section structure of this reference, item 13); CP001 §4.3 (dictionary passing as the underlying mechanism); construction §§9, 11 (variant construction surface; `-<` operator with variant operands).*

---

## 14. Open Questions

This section catalogs the class-system-related open questions. Resolutions covered by this reference are noted at the section where they are settled (with cross-references here for completeness); forwarded sub-questions are recorded; genuinely open items registered by this reference are described in full.

### 14.1 OQ-5 — Resolved Here (§8)

OQ-5's four sub-questions are all resolved in §8 *[CP001 §6, OQ-5; CP017 §6 full resolution]*:

| Sub-question | Resolution | Where |
|---|---|---|
| (a) Duplicate instances | Intra-module duplicates are static errors; cross-module duplicates resolved by specificity ranking | §8.1 |
| (b) Orphan instances | Permissive: any module may declare any `instance C: T`; conflicts ranked at use sites; ties are static errors | §8.2 |
| (c) Domain-specific dispatch | Dispatch resolves on runtime domain identity, not on the static type of an implicit upcast | §8.3 |
| (d) Cross-module overriding | Julia's "more specialized module wins" pragmatics, with import-time competition warning improvement | §§8.4, 8.7 |

The resolutions form a coherent whole, organized around the bright-line rule of §8.6 unifying the cross-module coherence story across class instances and failure-tag children.

### 14.2 OQ-6 — Resolved Here (§7)

OQ-6 (partial application beyond receiver-elision) is resolved in §7. Partial application is a command-reference phenomenon; receivers must always be applied (never deferred); non-receiver parameters may be applied or `_`-deferred; mode-marker filter (PRODUCE deferred-only; REFERENCE applied captures with ceiling-tracking; READ/IN flexible); resulting type covers only deferred parameters in declaration order. *[CP001 §6, OQ-6; CP017 §8 full resolution.]*

### 14.3 OQ-16 — Resolved Here (§9)

OQ-16 (overloading on dispatched commands) is resolved in §9. Three layers of overloading admitted (within-class, cross-class, non-dispatched); most-specific-candidate-wins by joint matching across receiver and argument types; ties are compile errors; `{C::method}` form for cross-class disambiguation; argument-shape disambiguation by call-site types alone. *[CP001 §6, OQ-16; CP007 §7; CP017 §9 full resolution.]*

### 14.4 OQ-29 — Resolved Here (§10)

OQ-29 (Liskov-style opening of the failure-tag hierarchy under payload-class covariance) is resolved in §10. Reframed in class-instance terms (not buffer-backed-subsumption); current concrete rule is same-class equality (the trivially-covariant case); forward-extension hedge for class-implication once a class-hierarchy mechanism is admitted. Three sub-rules for explicit override: widening-from-no-payload allowed, typed-payload-variation under covariance, typed-to-no-payload narrowing forbidden. *[type-system §7.9; failure-system §7.6 (OQ-28); CP017 §10 full resolution.]*

### 14.5 OQ-32 — Class-Method Fexpr-Typed Parameter Sub-Item Resolved Here (§11)

OQ-32 was registered in lambda-and-fexpr §10.7 with five sub-items. Two are resolved in §11 of this reference *[CP017 §12.6]*:

| Sub-item | Status |
|---|---|
| Class-method fexpr-typed parameter edge cases | **Resolved** in §11.4–§11.8 |
| Command-reference relaxation possibilities | **Resolved as side effect** of fexpr-tainting axis (§11.6) |

The other three sub-items remain forwarded to future threads:

- Fexpr-typed return-from-call edge channels.
- Transitive variant-fexpr-candidate containment rules (now subsumed under the disallow-entirely rule of §11.7 for v1; the rules become live again when relaxation happens).
- `-<` operator's interaction with fexpr-typed slots (now in fexpr-tainting context per §11.10, but the precise semantics are deferred).

### 14.6 OQ-13 — Uninvolved Here

OQ-13 (implicit context parameters and initialization) is fully resolved in construction §13. The class-system reference is uninvolved. The boundary statement is recorded in §12 of this reference. *[construction §13.8.]*

### 14.7 OQ-28 — Closed Here (Under OQ-29)

OQ-28 (closure of the failure-tag hierarchy) is registered in failure-system §7.6 as a refinement of CP012 §2.4's flat-closure rule. With OQ-29 resolved here in §10 (Liskov-style opening under same-class-equality covariance), OQ-28's question is answered: the hierarchy is opened under the same-class constraint. The pending touch-up to failure-system §7.6 is registered in work plan §6.1: close OQ-28 with cross-reference to class-system §10.

### 14.8 OQ-33 — Variant-Class-Typed-Parameter Slot Reuse-vs-Wrap (Newly Registered)

When a variant value is passed to a class-typed parameter (Case B existential, §4.3), the interaction raises a representation question: does the variant's existing 3-word slot get reinterpreted in place (since both variant and class-typed slots are 3-word patterns), or is a fresh class-typed slot allocated and the value wrapped into it? *[CP017 §§4.4, 14.1.]*

Reinterpretation is more efficient but requires the variant's spans-class to *be* (or be a parent of) the class the parameter expects. Wrapping always works.

The choice has runtime-performance and implementation-complexity implications but does not change the user-facing surface — the call is well-formed either way; the question is purely how the runtime represents it. Resolution requires future design dialog. *[CP017 §14.1.]*

### 14.9 Forward Note on OQ-29 Relaxation Activation

The OQ-29 covariance rule's relaxation from same-class-equality to class-implication activates when the class system admits a way to express "class C_A implies class C_B" as a first-class relationship — superclass constraints, combined-class composition extended, or another mechanism. Until then, same-class is the rule. *[CP017 §14.2.]*

This is recorded as a note on OQ-29's entry in the OQ registry rather than as a separate OQ-34, since it's coupled to whatever class-hierarchy mechanism is eventually designed.

### 14.10 Forwarded Items Originating Elsewhere

The class-system reference touches several OQs settled or maintained elsewhere:

| OQ | Owner | Where settled / pending |
|---|---|---|
| OQ-23 | Implementation thread | Grammar items added: `(T:Class)` in receiver positions; `{C::method}` form |
| OQ-26.2 | Failure-system / implementation | Surface syntax for class-instance declarations (placeholder used here) |
| OQ-30 | Construction | Heap allocation language-level mechanism (genuinely open) |
| OQ-31 | Construction | Tuple-style positional access via `Bar::N` (genuinely open) |
| OQ-32 (remaining) | Future thread | Three sub-items still forwarded per §14.5 |

The standard-failure-tag library thread (forward direction registered) will eventually settle:

- `FexprFailure` (§11.8) — the class-method-fexpr-invocation failure tag.
- `CoercionFailure` (CP017 §5.5) — the `-<` operator's failure tag (originating from construction §11; cross-referenced here for completeness).

These are placeholder names whose exact hierarchical position and payload-class binding will be carved when the standard-library design thread is opened.

---

## 15. Provenance

**Authored:** Distilled by Claude (Opus 4.7) on 2026-05-05 from the Basis intent-dialog corpus, in continuation of the topic-organized reference consolidation begun with `reference-failure-system.md` (2026-04-29), `reference-operational-semantics.md` (2026-04-29), `reference-type-system-and-modes.md` (2026-05-02), `reference-construction-and-initialization.md` (2026-05-03), and `reference-lambda-and-fexpr.md` (2026-05-04). This is the **sixth and final reference** in the agreed topic-organized sequence; with this reference, the consolidation work covers the full six-reference plan.

**Source materials read for this reference:**

- `project-operating-principles.md` in full (the lens for all design reasoning).
- `project-work-plan.md` §3 (recommended next-reference scope), §5 (OQ registry), §6 (pending consolidations).
- `intent-checkpoint-001.md` §§4 (classes, instances, dispatch — foundational), §6 (OQ-5, OQ-6, OQ-13, OQ-16 framings).
- `intent-checkpoint-005.md` §2 (R1+R2 receiver-mode rules — carved into §6 of this reference).
- `intent-checkpoint-007.md` §7 (the OQ-16 candidate-restriction framing — superseded by §9 of this reference).
- `intent-checkpoint-012.md` §§1.2, 3.1 (class-bound payloads on failure tags — carried into §10 of this reference).
- `intent-checkpoint-017.md` in full (the 2026-05-04 scope-confirmation and substantive-design dialog; the primary source for this reference's commitments).
- `reference-failure-system.md` §§4.1, 4.2, 4.3, 4.4, 4.11, 4.12, 5.1, 7.6 (failure-tag class-payload mechanics; OQ-28 framing).
- `reference-operational-semantics.md` §§5.5, 5.6, 5.7 (boundary points for op-sem; the redirect from §5.7 to here).
- `reference-type-system-and-modes.md` §§3.4, 3.5, 5.2, 7.5, 7.6, 7.7, 7.8, 7.9, 7.10 (variant 3-word slot, dynamic narrowing, OQ forwardings).
- `reference-construction-and-initialization.md` §§3.5, 5.4, 5.5, 9, 10, 11, 13 (variant construction, `.implicit`, union byte-reinterpretation, `-<` operator; context-variable mechanics).
- `reference-lambda-and-fexpr.md` §§3, 5, 6, 6.5, 6.7, 8, 10.7, 10.8, 11 (four-form taxonomy, captures, ceiling tracking, OQ-32 framing, fexpr-typing).

**Carried-in commitments encoded from inception (not from prior reference's framing):**

- **Cases A and B class-typed-parameter taxonomy** *[CP017 §3]*: type-variable-bound (Case A) with `(T:Class) 'name` surface; existential (Case B) with `Class 'name` surface; the witness-mechanism difference (hidden parameter vs. 3-word slot); whole-signature type-variable scope; multi-class via combined classes only in v1; v-command receiver type-variable extension; mode-marker prefix discipline; bidirectional existentials. Encoded throughout §§3, 4, 6 from inception.

- **3-word slot pattern as standard runtime representation for class-typed values** *[CP017 §4]*: tag identifier / value pointer / class witness; structural parallel to variants and failures; Case B only (Case A uses natural representation with hidden-parameter witness); RTTI implementation-internal not programmer-visible; witness flow with captures. Encoded in §4 from inception.

- **Single bright-line rule across class instances and failure-tag children** *[CP017 §6.1]*: any module may declare any well-formed extension; specificity ranks at competition; ties are static errors; per-mechanism well-formedness constraints. Encoded in §§8.6, 10 from inception.

- **Co-location rule for instance implementations** *[CP017 §7.1]*: every `.instance T: C` must result in a complete dictionary; in-scope visibility (defaults, methods on `T`, delegation, future mechanisms) counts; missing-after-assembly is a compile error; forward-compatibility hedge. Encoded in §3.4 from inception.

- **OQ-29 reframed in class-instance terms** *[CP017 §10.1]*: prior buffer-backed-subsumption framing was incorrect; correct framing is class-binding with covariance principle; current concrete rule is same-class equality; forward-extension hedge for class-implication. Encoded in §10 from inception.

- **Fexpr-relevance as second taint axis** *[CP017 §11]*: parallel to READ contract (OPS §1.3); access-path-based; per-frame propagation; ceiling-bound to fexpr's `D`; sources of taint (direct fexpr-typed values, composite values, command-reference bindings, captures); enforcement (no escape via long-lived stores, no PRODUCE outputs to outer frames, no escape via command-references with greater ceiling). Encoded in §11 from inception.

- **CP016 commitments revised** *[CP017 §§11.6, 11.7]*: command references *can* bind fexpr-typed values under fexpr-tainting (supersedes CP016 / lambda-and-fexpr §3.7); variants with fexpr candidates fully disallowed in v1 (tightens CP016 / lambda-and-fexpr §6.5 Restriction C from local-slot-permitted to disallowed-entirely). Reconciliation markers cite CP017 in lambda-and-fexpr touch-up batch.

- **CP015 §5.5 superseded for `-<` on unions** *[CP017 §5.4]*: the prior "No `-<` on Unions" commitment is replaced by admission under the existential candidate-or-parent rule. The construction reference's §11 will be revised; class-system §4.5 (RTTI via `-<`) and class-system §13.7 (variant-class-typed interaction) are written assuming the multi-role `-<` framing.

**Resolutions recorded:**

- OQ-5 (a/b/c/d) — full resolution in §8 across four sub-questions.
- OQ-6 — broader-scope resolution in §7.
- OQ-16 — overloading admitted across all three layers in §9.
- OQ-29 — Liskov-style opening with same-class equality (current) and class-implication (future) in §10.
- OQ-32 (class-method-fexpr sub-item) — admission, defaults-incompatibility, partial-application admission, variant-tightening, `FexprFailure` wrapping in §11.
- OQ-32 (command-reference relaxation sub-item) — resolved as side effect of fexpr-tainting axis in §11.6.

**Newly registered open questions:**

- OQ-33 — variant-class-typed-parameter slot reuse-vs-wrap. Recorded in §14.8.

**Reference-document impact (pending consolidations).** The following revisions to other references are required as a consequence of the commitments in this reference; collected in the work plan's §6 for a future consolidated pass rather than applied here:

- **Failure-system §4.2** — add paragraph on inheritance default from ancestry; the three sub-rules for explicit override (widening allowed; typed-payload-variation under covariance; typed-to-no-payload narrowing forbidden). *[work plan §6.1.]*
- **Failure-system §4.3** — confirm framing as already correct; cross-reference class-system §10 for cross-module-extension treatment. *[work plan §6.1.]*
- **Failure-system §7.6 (OQ-28)** — close OQ-28 (open under OQ-29's covariance constraint); reframe in class-instance terms; cross-reference class-system §10. *[work plan §6.1.]*
- **Failure-system new entry or appendix** — register `FexprFailure` and `CoercionFailure` as forward items for a future "standard failure tag library" design thread. *[work plan §6.1.]*
- **Op-sem §5.7** — redirect R1+R2 receiver-mode forwarding from type-and-modes to class-system §6. *[work plan §6.2.]*
- **Op-sem §5.5** — redirect OQ-13 forwarding from "class-system reference" to construction §13. *[work plan §6.2.]*
- **Type-system §3** — note that parameter-position admits class-name-as-existential as Case B sugar; full treatment in class-system §§3, 4. *[work plan §6.3.]*
- **Type-system §3.4** — brief cross-reference to class-system §13 acknowledging that the 3-word slot pattern generalizes to class-typed values; class-system owns class-typed-value details. *[work plan §6.3.]*
- **Type-system §3.5 (`-<` operator domain)** — revise to admit unions in `-<`'s domain (the prior exclusion based on no-runtime-tag reasoning is superseded by the existential-rule framing); cross-reference construction §11. *[work plan §6.3.]*
- **Type-system §4** — new sub-section on the second taint axis (fexpr-tainting), parallel to the READ contract treatment. *[work plan §6.3.]*
- **Type-system §7.5 (OQ-6)** — mark resolved in class-system §7; redirect. *[work plan §6.3.]*
- **Type-system §7.6 (OQ-5)** — mark all four sub-questions resolved in class-system §8; redirect. *[work plan §6.3.]*
- **Type-system §7.9 (OQ-29)** — rewrite in class-instance terms; covariance principle with same-class as current concrete rule; forward-extension hedge; cross-reference class-system §10. *[work plan §6.3.]*
- **Type-system §7.10 (OQ-16)** — mark resolved in class-system §9; redirect. *[work plan §6.3.]*
- **Construction §3.5 (variant-fexpr-candidate)** — update to disallowed-entirely (tightening from CP016's local-slot exception). *[work plan §6.4.]*
- **Construction §10 (union → candidate-or-parent reinterpretation)** — note that the explicit `-<` form is also admitted; cross-reference construction §11 for unified treatment. *[work plan §6.4.]*
- **Construction §11 (`-<` operator)** — substantial revision: full general formulation as dynamic type coercion; case-analysis decision rule; case enumeration (variant; class-typed; union; concrete-types); cross-references to class-system §4 and class-system §11. *[work plan §6.4.]*
- **Lambda-and-fexpr §3 (command references)** — admit fexpr-typed bindings via fexpr-tainting machinery; supersede CP016 "cannot bind" commitment with reconciliation marker citing CP017. *[work plan §6.6.]*
- **Lambda-and-fexpr §§5, 6** — class-typed (Case B) captures carry both witness and tag; type-variable-bound (Case A) captures bring hidden-parameter witness as part of closure environment. *[work plan §6.6.]*
- **Lambda-and-fexpr §6.5** — variant-with-fexpr-candidate tighten to disallowed-entirely; remove the local-slot exception. *[work plan §6.6.]*
- **Lambda-and-fexpr §8** — re-express ceiling rules as instances of fexpr-tainting flow rather than per-form ad-hoc rules. *[work plan §6.6.]*
- **Lambda-and-fexpr §10** — update OQ-32 status: class-method fexpr-typed parameter sub-item resolved in class-system §11; command-reference relaxation sub-item resolved as side effect. *[work plan §6.6.]*
- **Operating-principles document** — two new pending updates: `-<` as multi-role dynamic type coercion operator (parallel to §1.7's `::` framing); fexpr-relevance as second taint axis (parallel to §1.3's READ-contract taint). Plus a forward-pointer in prose on terminology disambiguation across "tag" uses. *[work plan §6.5.]*

**Authority statement (recapitulated).** Where this reference and an earlier reference overlap, this reference is authoritative on the class-system side and the earlier reference is authoritative on its own domain. The overlap touch-ups above are scheduled for batched application; until they are applied, the canonical reading of the overlapping material is what this reference states. Earlier references' provisional or stale framings are superseded by this reference's commitments where this reference covers the topic.

**Recommended next step.** With this reference complete, the topic-organized consolidation work has covered the full six-reference sequence (failure-system, op-sem, type-system, construction, lambda-and-fexpr, class-system). The remaining work is the **consolidated cross-document touch-up pass** — applying all the batched revisions enumerated above (work plan §§6.1–6.6) to the earlier references and the operating-principles document. This pass updates earlier references to reflect later resolutions (notably: type-system §7's OQ-forwardings collapse to "resolved in class-system §X"; failure-system §7.6's OQ-28 closes; op-sem §§5.5, 5.7's forwardings redirect; construction §11's `-<` becomes the multi-role authoritative source; lambda-and-fexpr §3.7's "cannot bind fexpr" is superseded; the operating-principles document gains two new principles).

After the consolidated touch-up pass, the project's authoritative documents are fully current: each reference is consistent with the latest resolutions; no batched touch-ups remain pending; the OQ registry reflects the final state. The work plan should be updated to reflect this state — moving the completed-references count from 5 to 6, retiring the §§6.1–6.6 batches, updating the OQ registry's resolved/forwarded/open partition, and noting the consolidated pass as the next task.

Concurrently, JimDesu may consider scheduling design dialogs for the still-forwarded items: OQ-30 (heap allocation), OQ-31 (tuple-positional access), OQ-32 remaining sub-items (fexpr-typing ramifications), OQ-33 (variant-class-typed-slot reuse-vs-wrap), and the standard-failure-tag library thread (`FexprFailure`, `CoercionFailure`, and any others). These can be sequenced as desired; they are independent of the topic-organized reference work and the consolidated touch-up pass.

A grammar item warrants explicit registration with the implementation thread (work plan / OQ-23 batch): the `(T:Class)` form admitted at v-command receiver positions (per §4.4.3), and the `{C::method}` curly-brace form for class qualification disambiguation (per §9.4). Both are small additions to the existing grammar; their formal grammar specifications belong with OQ-23's other lexer-and-grammar items.
