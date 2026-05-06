# Basis Language — Intent Checkpoint 017

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-016.md; supersedes them where they conflict. This checkpoint records the 2026-05-04 scope-confirmation and clarification dialog for the Class System reference (the sixth and final topic-organized reference). The dialog produced substantive resolutions of OQ-5 (all four sub-questions), OQ-6 (broader scope), OQ-16 (overloading on dispatched commands), OQ-29 (Liskov-style opening of the failure-tag hierarchy under covariance), and the class-method fexpr-typed parameter sub-item of OQ-32. Beyond OQ resolution, the dialog produced several substantive new design commitments that go beyond merely answering the registered questions: the admission of class-as-existential and bounded-parametric-polymorphism class-typed parameter forms (Cases A and B); the 3-word slot pattern as the standard runtime representation for class-typed values; the elevation of `-<` to a multi-role dynamic type coercion operator with a unifying reading; the unification of cross-module coherence rules into a single bright line spanning instance declarations and failure-tag extensions; the co-location rule for instance implementations; and the formalization of fexpr-relevance as a second taint axis parallel to the READ contract. The dialog also produced material revisions to several prior commitments — CP016's "command references cannot bind fexpr-typed values" was overruled in favor of admission via the taint axis; CP016's "variant-with-fexpr-candidate permitted in local-slot positions" was tightened to disallowed entirely in v1; CP015 §5.5's "No `-<` on Unions" was superseded by admission of `-<` on unions under the existential candidate-or-parent rule. The class-system reference itself was *not* drafted in this session; per OPS §2.8 (honesty about capacity), the dialog's substantive output exceeded the context budget for combining scope confirmation with drafting in a single session. Drafting will resume in a fresh session with this checkpoint, the operating-principles document, the work plan, and the existing references in scope.

**Date of this checkpoint:** 2026-05-04 (continuation; the lambda-and-fexpr scope dialog of CP016 was earlier the same date).

**Provenance:** Distilled from the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-05-04, in the class-system-reference scope-confirmation thread following the 2026-05-04 completion of the Lambda and Fexpr reference. The dialog walked the seven scope-confirmation questions of work plan §3.4 in order; each question's resolution surfaced one or more substantive design moves beyond the question's narrow framing, several of which warranted explicit treatment. The dialog also corrected a recurring miscalibration — the conflation of "buffer-backed type" with "concrete static type" when reasoning about the dynamic narrowing operator's behavior — and tightened earlier proposed framings (notably OQ-29's payload-covariance constraint, originally drafted in buffer-backed-subsumption terms but correctly stated in class-instance terms).

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

**Note on source faithfulness.** This checkpoint is reconstructed from the dialog's turn-by-turn record on 2026-05-04. Where commitments were sharpened or revised mid-dialog (notably for OQ-29's class-instance reframe; for the union-narrowing rule under `-<`; for the no-payload-to-typed widening rule), the final form is what is recorded here; rejected intermediate framings are noted in the relevant section's provenance trail. Quoted user phrasings are preserved where they carry design content (e.g., the bright-line preference, the orthogonality preference); paraphrasing is used elsewhere.

---

## 1. Decision Summary

This checkpoint lands the following resolutions, registrations, and revisions:

1. **Section structure for the class-system reference confirmed (15 sections).** Prelude; Classes; Instances; Witness-slot model and dictionary passing; Single-class dispatch; V-commands and multiple dispatch (with R1+R2 receiver-mode rules captured in this reference); Partial application beyond receiver-elision; Instance coherence (OQ-5); Overloading on dispatched commands (OQ-16); Liskov-style opening of the failure-tag hierarchy (OQ-29); Class-method fexpr-typed parameters (OQ-32 sub-item); Boundary against context variables (cross-reference construction §13); Variant class-witness slot (with type-system §3.4 cross-referencing here for details); Open Questions; Provenance. *(§2)*

2. **Class-as-existential parameters admitted (Case B).** A class name in parameter position denotes an existentially-quantified type — the parameter accepts any value whose runtime type satisfies the class, with the class witness traveling with the value. Symmetric in input/output positions: a productive parameter typed as a class admits the command picking some satisfying type and producing a value with witness attached for the caller to dispatch through. *(§3)*

3. **Type-variable-bound parameters admitted (Case A) with the `(T:Class) name` surface.** A type variable introduced in a parameter declaration is constrained to instances of a specified class, and may be referenced by subsequent parameters to bind them to the same concrete type. The witness flows once as a hidden parameter for all `T`-typed slots in the signature. The type-variable's scope is the entire command (whole-signature scope, regardless of source-position order). *(§3)*

4. **Multi-class constraint via user-defined combined classes only (Case A scope-narrowing).** A type variable may be constrained by a single class only; multi-class needs are met by user-defined combined classes per CP001 §4.2's `class CombinedC: C1, C2, ...` form. Future relaxation possible. *(§3.3)*

5. **The `(T:Class) name` surface extends to v-command receivers, with whole-signature type-variable scope.** Type variables introduced at receiver positions are in scope for non-receiver parameters in the same signature; this admits binding regular command parameters to receiver-introduced type variables. Grammar extension required, deferred to OQ-23's implementation-thread batch. *(§3.4)*

6. **Mode marker syntax confirmed: prefix on identifier, not suffix.** `'name` is productive; `&name` is reference. The `(T:Class)` qualifier sits in its own parens before the parameter name; mode marker prefixes the name as usual. *(§3.5)*

7. **`name : Type` is not Basis syntax outside the class-constraint case.** The colon in `(T:Class)` is reserved for type-variable qualification; regular parameter declarations use `Type 'name` order. The corrected mental model: `Type 'name`, `(T:Class) 'name`, `Showable 'name`, never `name : Type`. *(§3.6)*

8. **The 3-word slot pattern as the standard runtime representation for class-typed values.** Tag (RTTI) / value-pointer / class-witness, parallel to variants (type-system §3.4) and failures (failure-system §5.1). Applies specifically to Case B existentials where each slot independently carries witness; Case A type-variable-bound parameters use natural representation with hidden-parameter witness because the typechecker statically knows the binding. The tag enables RTTI but is implementation-internal — not programmer-visible directly. *(§4)*

9. **`-<` is the dynamic type coercion operator, multi-role.** The form `TargetType name -< SourceExpr` attempts to coerce `SourceExpr` to `TargetType` and binds `name` on success. The semantic analyzer admits the operator across any pair of types; program shape is valid even when no coercion path exists. Coercion behavior is runtime-checked semantically; the compiler may elide checks as an optimization where observationally indistinguishable from the runtime version, and may emit unreachability warnings when consequences make code unreachable. The unifying reading parallel to OPS §1.7's `::` framing: "coerce dynamically where valid; fail explicitly where not." *(§5)*

10. **`-<` admitted on unions, governed by the existential candidate-or-parent rule.** Supersedes CP015 §5.5's "No `-<` on Unions" commitment and the corresponding type-system §3.5 exclusion. The union case is essentially the explicit form of what implicit `<-` does at construction §10; the static existential rule is the gatekeeper, with no runtime check needed beyond the static admission. *(§5.4)*

11. **`-<` failure produces a propagating failure under a unified failure-tag direction.** The failure tag `CoercionFailure` is registered as a forward direction (a future "standard failure tag library" thread); concrete carving deferred. *(§5.5)*

12. **OQ-5 (a)+(d) joint framing.** Intra-module duplicate instances for the same `(class, type)` pair are a static error; cross-module duplicates are resolved by Julia's "more specialized module wins" specificity ranking. *(§6.1)*

13. **OQ-5 (b) — orphan-instance policy: permissive with import-time competition warning.** Any module may declare any instance for any (class, type) pair regardless of where the class or type is defined. Conflicts at use sites resolve by (d)'s ranking; ties are static errors. The import-time warning is an improvement over Julia's pragmatics: when an import introduces an instance that competes with an already-visible one for an `(class, type)` pair already used in the importing module, the static analyzer flags the new competition at import-time rather than letting it surface at use-sites. *(§6.2)*

14. **OQ-5 (c) — domain-specific dispatch on runtime identity codified.** Dispatch resolves on the runtime domain identity, not on the static type the value was implicitly upcast to. The type-system §5.2 commitment (implicit upcast is type-acceptance, not value-rewriting) flows through naturally. *(§6.3)*

15. **OQ-5 (d) — Julia's "more specialized module wins" pragmatics retained.** Specificity ranking by module hierarchy; ties at the use site are static errors. *(§6.4)*

16. **Single bright-line rule across class instances and failure-tag children.** *Any module may declare any well-formed extension — class instance, failure-tag child, or other declaration that extends a parameterized type contract. Where multiple visible declarations compete at a use site, specificity ranks the winner; genuine ties are static errors. Per-extension well-formedness constraints apply: instances need intra-module uniqueness; failure-tag children need payload covariance (OQ-29).* *(§6)*

17. **Co-location rule for instance implementations.** Every `.instance T: C` declaration must result in a complete dictionary for `C`'s methods on `T` after the instance is fully assembled. Implementations not otherwise in scope (via `C`'s defaults, methods on `T`, delegation per CP001 §4.2, or future mechanisms that contribute through visibility) must be supplied locally in the instance declaration. Methods left without any provider after assembly cause compilation to fail. The hedge "or some other means not currently identified" preserves design space for future contribution mechanisms. *(§7)*

18. **OQ-6 broader scope resolved.** Partial application is a command-reference (the bare `{...}` form) phenomenon. Receivers are always specified, never deferred. Non-receiver parameters may be applied or `_`-deferred. The resulting command-typed value's type covers only the deferred parameters in declaration order. Mode-marker filter: PRODUCE parameters can only be deferred (never applied — the directions don't match); REFERENCE parameters when applied capture the reference and engage ceiling-tracking; READ/IN parameters are applied or deferred freely. *(§8)*

19. **OQ-16 — overloading admitted across all three layers.** Within-class overloading, cross-class same-name methods, and non-dispatched command overloading are all admitted. Resolution is most-specific-wins by joint receiver and argument-type matching; ambiguity is a compile error including at partial-application sites. *(§9)*

20. **`{Class::method}` curly-brace form for class qualification disambiguation.** When a type satisfies multiple classes with same-named methods, `t :: {C::process}` qualifies which class's method is intended. The curly-brace mindspace aligns with the four-form taxonomy (CP016) — `{C::process}` reads as "the command reference for `process` from class `C`." Argument-shape disambiguation comes from call-site argument types alone; no syntactic shape qualifier (`{C::method[shape]}`) is admitted. Ambiguities not resolvable by class qualification + argument types are compile errors. *(§9.3)*

21. **OQ-29 reframed in class-instance terms; same-class equality as the current concrete rule.** Type-system §7.9's prior framing in buffer-backed-subsumption terms was incorrect — buffer-backed subsumption is not a subtyping mechanism for failure propagation. The correct framing: a failure tag's payload declaration binds a class; supplied payloads are values whose concrete types have instances of that class. The covariance principle: as a failure tag narrows down a hierarchy, the payload class narrows or stays the same. The current concrete rule (under flat class structure) is **same-class equality** — A's payload class equals B's payload class for any A extending B. The general "covariance" framing is preserved in the specification language as a forward-compatibility hedge for future class-hierarchy support, at which point the rule relaxes to "A's payload class implies B's payload class." *(§10)*

22. **Failure-tag hierarchy default inheritance from ancestry.** A child tag's payload specification is inherited from its parent unless the child explicitly overrides. Inheritance chains transitively up the hierarchy; "no payload" is itself a specification that propagates the same way. Three sub-rules govern explicit override: (a) widening from "no payload" at any ancestor to a typed payload at a descendant is allowed — parent-keyed handlers don't bind, so no contract violation; (b) typed-payload-to-typed-payload variation is constrained by covariance (currently same-class equality); (c) typed-payload-to-no-payload narrowing is forbidden — it removes binding capability that ancestor handlers' contracts may rely on. *(§10.2)*

23. **Fexpr-relevance as a second taint axis (substantive new design move).** Parallel to the READ contract (OPS §1.3): access-path-based, per-frame-propagated, no cross-frame flow. A fexpr-relevance-tainted access path is ceiling-bound to the fexpr's defining frame `D`; cannot escape via stores into longer-lived slots, PRODUCE outputs to outer frames, or capture by command-references whose own ceiling exceeds `D`. The taint flows from direct fexpr-typed values, from composite values containing fexpr-relevant content (variant candidates, command-reference bindings), and from captures of fexpr-relevant values. Per-frame analysis enforces the ceiling discipline uniformly across mechanisms. Substantial OPS principle to be added in §6.5 batch. *(§11)*

24. **OQ-32 (class-method fexpr-typed parameter sub-item) substantively resolved.** Class methods with fexpr-typed parameters are admitted. Partial application can apply fexpr-typed parameters under the fexpr-tainting machinery (the resulting command reference is ceiling-bound). Default implementations are incompatible with fexpr-typed parameters — compile-time error, since defaults have no determinate `D`. Failure-set declarations on class methods that invoke fexpr-typed parameters wrap the fexpr's failures in a single standard failure tag (`FexprFailure`, registered as a forward direction parallel to `CoercionFailure`); specificity is sacrificed to keep failure-sets clean and orthogonal. *(§12)*

25. **CP016's "command references cannot reference or bind fexpr-typed values" superseded.** With fexpr-tainting machinery in place, command references can bind fexpr-typed values; the bound reference inherits fexpr-relevance taint and is ceiling-bound to the fexpr's `D`. This also resolves the OQ-32 "command-reference relaxation possibilities" sub-item as a side effect. *(§11, §12)*

26. **CP016's "variant-with-fexpr-candidate permitted in local-slot positions" tightened.** In v1, variant candidates may not have fexpr type at all — even in local slots. The local-slot exception from CP016 §6.5 is removed. Future relaxation intended once fexpr-tainting machinery matures and the typechecker can enforce ceiling-tracking on variant-borne fexprs. *(§12.4)*

27. **Witness flows with captures in lambdas and fexprs.** A class-typed (Case B) capture brings its witness into the captured environment; a type-variable-bound (Case A) capture brings the hidden-parameter witness as part of the closure environment. Touch-up to lambda-and-fexpr §§5, 6 noted in §6.6 batch. *(§4.5)*

28. **Boundaries against op-sem §§5.6–5.7 confirmed.** Op-sem owns operational/runtime mechanism; this reference owns static rules and dispatch tables. New touch-up: op-sem §5.7's R1+R2 receiver-mode forwarding redirects from type-and-modes to class-system §6 (since this reference absorbs R1+R2 per Q1). *(§13)*

29. **OQ-33 newly registered.** Variant-passed-as-class-typed-parameter slot reuse-vs-wrap question: when a variant value is passed to a class-typed parameter, does the variant's existing 3-word slot get reinterpreted (reuse-when-class-membership-permits), or is a new 3-word slot allocated (always-wrap)? Resolution requires future design dialog. *(§14)*

30. **Three pending consolidations expand to substantial revision packages.** Type-system §6.3, construction §6.4, lambda-and-fexpr §6.6 (newly added), and OPS §6.5 each receive multiple touch-ups. The pending-consolidation work has grown enough that batched application after the class-system reference completes is the recommended path. *(§15)*

The class-system reference (the sixth and final topic-organized reference) will encode all of the above from inception when drafted in a fresh session.

---

## 2. Section Structure of the Class-System Reference

The 15-section structure agreed in the dialog:

1. **Prelude** — status, date, provenance, authority statement, citation conventions, standing principles carried in
2. **Classes** — declaration surface; single-parameter constraint; declared-vs-defaulted members; no-final discipline (per CP001 §4.1)
3. **Instances** — declaration surface; delegation (per CP001 §4.2); the `(class, type)` pair as the dispatch key; the co-location rule (§7 of this checkpoint); boundary against context variables (cross-reference construction §13)
4. **Witness-slot model and dictionary passing** — runtime representation; byte-faithful domain commitment; the polymorphic-command hidden-parameter mechanism (CP001 §4.3); the 3-word slot pattern for Case B class-typed values (§4 of this checkpoint); witness flow under captures
5. **Single-class dispatch** — call-resolution sequence; relationship to op-sem §5.6's surface treatment of `::`
6. **V-commands and multiple dispatch (static side)** — well-formedness rules; receiver-mode treatment with R1+R2 from CP005 §2 carved here in full; cross-reference to op-sem §5.7 for operational composition
7. **Partial application beyond receiver-elision** — OQ-6 broader resolution (§8 of this checkpoint)
8. **Instance coherence (OQ-5)** — duplicates, orphans, domain-specific dispatch, cross-module overriding; cross-module bright-line rule (§6 of this checkpoint); import-time competition warning
9. **Overloading on dispatched commands** — OQ-16 resolution (§9 of this checkpoint); `{C::method}` disambiguation
10. **Liskov-style opening of the failure-tag hierarchy** — OQ-29 resolution (§10 of this checkpoint); cross-reference failure-system §7.6
11. **Class-method fexpr-typed parameters** — OQ-32 sub-item resolution (§12 of this checkpoint); fexpr-tainting axis (§11 of this checkpoint)
12. **Boundary against context variables** — cross-reference construction §13; the parallel-but-distinct mechanisms (instance dispatch ≠ context-parameter resolution)
13. **Variant class-witness slot** — cross-reference type-system §3.4; population, propagation, consumption (this reference owns the details; type-system §3.4 declares the slot shape)
14. **Open Questions** — resolved-here / forwarded / newly-registered status for each touched OQ
15. **Provenance**

The structure is calibrated for a moderate-to-large reference; calibrating against completed references (533–1248 lines), the class-system reference is expected to land at 800–1200 lines depending on how compactly the OQ-related sections are written.

---

## 3. Class-Typed Parameters: Cases A and B

### 3.1 The Substantive Distinction

The dialog admitted two structurally different uses of class names in parameter declarations, distinguished here:

- **Case A — class as a constraint on a type variable.** A type variable `T` is introduced and constrained to instances of a class. Parameters typed as `T` accept any concrete type satisfying the class; multiple `T`-typed slots in a signature are bound to the *same* concrete type. The witness flows once as a hidden parameter (per CP001 §4.3's polymorphic-command convention); slot representation is the natural representation of the bound type. *Surface:* `(T:Class) 'name`, with subsequent parameters referencing `T` by name.

- **Case B — class as an existential type at parameter position.** A parameter typed *as* a class accepts any value whose concrete type satisfies the class; each slot independently carries its witness in the slot itself. The runtime representation is the 3-word slot pattern (§4). *Surface:* `Class 'name`, with `Class` being the class name appearing where a type would.

Both cases coexist and compose. A signature may freely mix `(T:Class) 'a, T 'b, OtherClass 'c` — type-variable-bound `a` and `b` share concrete type `T`; existential `c` is independently bound.

### 3.2 The Witness-Mechanism Difference

| Case | Witness location | Witness selection time |
|---|---|---|
| A | Hidden parameter alongside the call | Call site (typechecker selects based on visible instances) |
| B | In the slot itself (3-word) | Construction site (where the value is wrapped into the class-typed slot) |

The user-visible behavior is identical in both cases — `(value :: method)` dispatches through the appropriate witness. The cost difference (Case A pays once per call; Case B pays per slot) is invisible to the author.

The user's clarification on naming: classes in Basis are Haskell-style typeclasses (contracts), not Java-style concrete types. A class name is not itself a type whose values can be instantiated; it is a contract specifying operations a type must provide. "Passing a class into a parameter of class type" is nonsensical; one passes arguments that are *instances* of that class.

### 3.3 Multi-Class Constraints (Scope Narrowing for v1)

A type variable may be constrained by a *single* class only. Multi-class needs are met by user-defined combined classes per CP001 §4.2:

```
class CombinedC: C1, C2
```

A parameter `(T:CombinedC) 'name` then constrains `T` to types implementing both `C1` and `C2`. Future relaxation possible (admitting comma-separated multi-class constraints inline at the parameter declaration), but for v1 the single-class rule is the bright line.

### 3.4 Type-Variable Scope

Type variables introduced in a parameter declaration are in scope for the *entire signature*, regardless of source-position order. A signature like `T 'a, (T:Point) 'b` is well-formed (T is introduced at `b`'s declaration but is also referenced by `a`'s declaration). The typechecker must perform a pre-pass to gather type-variable introductions before binding parameter types; this is an implementation concern that does not affect the surface design.

### 3.5 V-Command Receiver Tuples

Type variables introduced at v-command receiver positions are in scope for non-receiver parameters in the same signature. This admits patterns like:

```
.cmd doSomething: (T:Point) 'r, T y = ...
```

where `r` is a productive receiver of some `Point`-class instance type `T`, and `y` is a non-receiver parameter constrained to the same `T`. The grammar extension (admitting `(T:Class)` qualifier in receiver positions) is added to OQ-23's implementation-thread batch.

### 3.6 Mode Markers and the `name : Type` Confusion

A recurring miscalibration in the dialog (corrected several times across prior sessions per the user) is the conflation of Basis parameter syntax with C/Pascal/Haskell-style `name : Type` annotations. Outside the class-constraint case `(T:Class)`, the colon form is not Basis syntax. The correct order is **type-then-name**, with mode marker as a *prefix* on the name:

| Form | Surface |
|---|---|
| Regular parameter (concrete type) | `Container 'r` (productive `r` of type `Container`) |
| Existential (Case B) | `Showable 'r` |
| Type-variable-bound (Case A) | `(T:Point) 'r` |

Mode markers (`'` productive, `&` reference, etc.) prefix the parameter name. The class-constraint form `(T:Class)` is the only place `:` appears as a binding-and-constraint operator on a type variable.

### 3.7 Bidirectional Existentials

A particular coherence point worth carving in: when the slot type is a class, the existential pattern applies symmetrically in both directions:

- **Input-side existential.** A caller provides a value of some type satisfying the class; command receives it with witness attached; command dispatches through the carried witness.
- **Output-side existential.** The command picks some type satisfying the class, fills the productive slot with a value of that type, attaches the witness; caller receives both and dispatches through the carried witness.

The witness flows in whichever direction the data flows, in lockstep. This was a refinement that emerged mid-dialog when the original framing of "Container 'r" as a stand-in for a concrete type was challenged — when the slot type is a class, the productive direction is itself an existential operation.

---

## 4. The 3-Word Slot Pattern as Standard for Class-Typed Values

### 4.1 The Commitment

Class-typed (Case B existential) values use the 3-word slot pattern as their standard runtime representation:

| Slot word | Content |
|---|---|
| Tag identifier | Small-integer RTTI tag identifying the runtime concrete type |
| Value pointer | Pointer to the value's bytes (or pointer-shaped value, for pointer-typed candidates) |
| Class witness | Pointer to the class dictionary (witness) for the runtime type |

This is structurally parallel to the variant 3-word slot (type-system §3.4) and the failure-system slot (failure-system §5.1). The reuse of the slot shape is intentional language-design parallelism — it allows the same dispatch and propagation patterns across variants, failures, and class-typed values.

### 4.2 Scope: Case B Only

The 3-word slot is used for Case B existentials specifically. Case A type-variable-bound parameters do not require the 3-word slot because all `T`-typed slots in a signature are the same concrete type (statically known to the typechecker); a single hidden-parameter witness covers all of them, and the values themselves can be in their natural representation.

The user's framing: "all the needed information is present" in Case A through the hidden-parameter witness; the 3-word slot is *availability*, not *mandatory repetition*. Where the witness is already available alongside the value through other means, the 3-word slot is unnecessary.

### 4.3 RTTI: Implementation-Internal, Not Programmer-Visible

The tag identifier supports RTTI (Runtime Type Information) at the implementation level — the language implementation can determine the concrete type of a class-typed value at runtime and use this for dispatch and for the dynamic narrowing operator. **The tag is not programmer-visible directly**: there is no surface form for reading the tag, no `tagOf` builtin, no determinism requirement on the tag-space (the implementation may assign tags as it sees fit, opaque to the programmer).

The user's intent: RTTI exists specifically to support `-<` and class dispatch. Programmer-facing surfaces interact with RTTI through `-<` only.

### 4.4 Variant-Class-Typed Interaction (OQ-33)

Open question: when a variant value is passed as a class-typed parameter, does the variant's existing 3-word slot get reinterpreted (since the variant slot's witness happens to be the right witness for the class-typed parameter, when class-membership permits), or is a new 3-word slot allocated?

Reinterpretation is more efficient but requires the variant's spans-class to *be* (or be a parent of) the class the parameter expects. Wrapping always works.

The dialog registered this as **OQ-33** (variant-class-typed-slot reuse-vs-wrap) and deferred resolution. Recorded in work plan §5.4.

### 4.5 Witness Flow With Captures

A class-typed (Case B) capture in a lambda or fexpr brings its witness into the captured environment. A type-variable-bound (Case A) capture brings the hidden-parameter witness as part of the closure environment. This is an extension of the lambda-and-fexpr capture machinery; not currently explicit in lambda-and-fexpr §§5–6 but implicit. Touch-up registered in §15.

---

## 5. The `-<` Operator as Multi-Role Dynamic Type Coercion

### 5.1 The Unifying Reading

`-<` is the **dynamic type coercion operator**. The form `TargetType name -< SourceExpr` attempts to coerce `SourceExpr` to `TargetType`, binding `name` on success and producing a propagating failure on mismatch.

The semantic analyzer admits the operator across any pair of types — program shape is valid even when no coercion path exists between LHS and RHS (such cases compile to "always-fails" operations, per the `if 0` analogy). Coercion behavior is runtime-checked semantically; the compiler may elide checks as an optimization where observationally indistinguishable from the runtime version, and may emit unreachability warnings when consequences make code unreachable.

The unifying reading parallel to OPS §1.7's `::` framing: "coerce dynamically where valid; fail explicitly where not." Validity is determined by the type relationship — implicit subsumption chains, variant candidate matching, class membership via runtime tag, union byte-reinterpretation under the existential rule.

### 5.2 Roles Enumerated

Cases of `-<`, by type relationship between LHS and RHS:

| LHS-RHS relationship | Behavior |
|---|---|
| Both buffer-backed concrete types, valid subsumption path | Statically resolved as success (subsumption-equivalent) |
| Both buffer-backed concrete types, no path (e.g., siblings) | Statically resolved as always-fail; compiler may emit unreachability warning |
| Variant LHS, candidate-typed RHS | Runtime-checked via tag; binds when candidate matches |
| Concrete type LHS, class-typed (existential) RHS | Runtime-checked via the carried tag; binds when concrete type satisfies LHS |
| Variant type LHS, class-typed (existential) RHS | Runtime-checked via tag; binds when runtime type is on the variant's candidate list |
| Union → candidate-or-parent | Statically resolved per the existential rule (§5.4) |

### 5.3 The Static-vs-Runtime Decision Rule

The static-vs-runtime distinction depends on whether the source's *static type* has a polymorphism layer (class-typed existential, variant, type-variable bound), not on whether the underlying value happens to be buffer-backed. Specifically:

- **Concrete static types on both sides, coercion path exists:** statically resolved as success (subsumption-equivalent or no-op).
- **Concrete static types on both sides, no coercion path:** statically resolved as always-fail; compiler may emit unreachability warning for downstream code.
- **Polymorphism layer on the RHS** (most common interesting case): runtime-checked via the carried tag; the typechecker knows neither succeed nor fail until runtime.

The user's clarification surfaced an important coherence point: a `Centimeters` value passed through a `Showable c` parameter has *static* type `Showable` (polymorphism-layered), not `Centimeters`, so a `Inches i -< c` at the receiving site is runtime-checked, not statically determinable.

### 5.4 Unions Under `-<` (Supersedes CP015 §5.5)

CP015 §5.5 ("No `-<` on Unions") and the corresponding type-system §3.5 exclusion are superseded. `-<` is admitted on unions, governed by the existential candidate-or-parent chain rule:

- The typechecker checks that the target type is on the buffer-backed subsumption chain of *at least one* declared candidate.
- If yes, `-<` is admitted and produces a byte-reinterpretation at runtime — the same byte-level work the implicit `<-` form does at construction §10.
- If no, the typechecker rejects (statically determinable always-fail with no path).

The union case is essentially the explicit form of what implicit `<-` does at construction §10; the existential rule serves as the static gatekeeper. No runtime check is needed — once the existential rule holds, the operation always succeeds at runtime (the bytes are interpreted as the target type).

### 5.5 Failure-Mode Uniformity

Every `-<` site carries the `?` failure mark uniformly — failure is semantically always possible. The compiler optimizes the failure-handling machinery away when it can prove always-succeeds, but the source program writes the failure mark consistently.

When `-<` fails at runtime, it fires a propagating failure under a unified failure-tag direction. The proposed direction is a `CoercionFailure` parent tag with sub-tags for specific case kinds (variant-mismatch, class-tag-mismatch, etc.); the user has not yet thought through standard failure tags fully, so this is a forward direction registered alongside `FexprFailure` (§12) for a future "standard failure tag library" thread.

### 5.6 Surface Uniformity

Uniform `value -< target` surface across all roles. The only divergence from uniformity is that the `_` markers (per CP015's variant `-<` extensions: `v -< _` always-succeeds clear; `_ -< v` may-fail non-absent test) are admitted only for variant operands. Classes and unions have no absent state; the `_` markers are not meaningful for them.

### 5.7 Cross-Reference Distribution

The `-<` operator's authoritative case-enumeration source becomes construction §11 (substantially revised from its current variant-only treatment):

- **OPS** declares the unifying reading as a new principle (§6.5 batch).
- **Construction §11** owns the operator's general formulation, the case-analysis decision rule, and enumerates the cases.
- **Class-system §4 (this reference)** contributes the class-typed-value tag-check and cross-references §11.
- **Type-system §3.5** cross-references §11; the prior union-exclusion is superseded.

---

## 6. Cross-Module Coherence: Instances and Failure-Tag Children

### 6.1 The Bright-Line Rule

The dialog committed to a single bright-line rule across class instance declarations and failure-tag hierarchy extensions:

> *Any module may declare any well-formed extension — class instance, failure-tag child, or other declaration that extends a parameterized type contract. Where multiple visible declarations compete at a use site, specificity ranks the winner; genuine ties are static errors. Per-extension well-formedness constraints apply: instances need intra-module uniqueness (intra-module duplicates for the same `(class, type)` pair are static errors); failure-tag children need payload covariance (OQ-29).*

This unifies what would otherwise be two parallel cross-module-coherence rules.

### 6.2 Orphan Instances (OQ-5 (b))

An **orphan instance** is `instance C T` declared in a module that defined neither `C` nor `T`. Three positions in the literature: Haskell-style restrictive (orphans forbidden, instances declarable only in C's or T's defining module); Rust-style (Haskell-style with newtype escape hatch — Basis's domain mechanism is the equivalent); permissive (Julia/Scala-style, orphans allowed, conflicts resolved by ranking or import-scope).

The dialog selected **permissive**: any module may declare any `instance C T`. Conflicts at use sites resolve by Q2(d)'s specificity ranking; ties are static errors.

### 6.3 Improvement Over Julia: Import-Time Competition Warning

Julia's pragmatics can produce silent surprises when a newly-imported module changes which instance dispatches under one's nose. Basis's improvement: at the static-analysis level, **warn at import-time** when a new instance competes with an existing one for an already-used `(class, type)` pair within the importing module. The warning surfaces the conflict at import rather than at use-site, restoring local predictability without restricting what can be declared.

### 6.4 Specificity Ranking (OQ-5 (d))

Cross-module overriding instances follow Julia's "more specialized module wins" pragmatics, with the user explicitly preferring this over alternatives where possible: *"keep Julia's pragmatics unless we see an opportunity where Basis can improve without losing Julia's flexibility."* The import-time warning is the improvement; the core specificity-ranking rule is preserved.

### 6.5 Witness-Mechanism Interaction with Permissive Orphans

A noticed coherence point: in Case B (class-as-existential), the witness is selected at the *construction site* — where the concrete value is first wrapped in a class-typed slot. Whichever orphan's dictionary is visible at construction is the one captured in the value's witness. Once attached, the witness travels; downstream sites use the captured dictionary regardless of what orphans they see.

This is actually a *feature* of the permissive policy: it localizes the orphan-selection decision to the construction site, reducing "action at a distance" worry. Whoever builds the value decides which orphan they're using; consumers receive the witness already chosen.

For Case A (type-variable-bound), the witness flows as a hidden parameter and is selected at the *call site*, so the call site's visible orphans determine which is used. Same locality principle, different selection point.

---

## 7. Co-Location Rule for Instance Implementations

### 7.1 The Rule

> *Every `.instance T: C` declaration must result in a complete dictionary for `C`'s methods on `T` after the instance is fully assembled. Implementations not otherwise in scope (via `C`'s defaults, methods supplied by `T`, delegation per CP001 §4.2, or any future mechanism that contributes implementations through visibility) must be supplied locally in the instance declaration. Methods left without any provider after assembly cause compilation to fail.*

The rule emerged from the dialog as a refinement: the original framing required all implementations to live in the instance-declaration module, but the user clarified that the responsibility is to *complete* the dictionary, not to *redundantly re-provide* what is already available through other channels.

### 7.2 What Counts as "In Scope"

"In scope by means of inclusion of T and of C" reads as: by importing the module that declares `T` and the module that declares `C`, the instance-declaration module has visibility into both. Defaults from `C` and any methods on `T` become available because of the imports, not by some special mechanism. This matches the standard module-import discipline.

### 7.3 Delegation Routes Through

When an instance uses `(delegate fieldName)`, the delegate's existing instance provides the method bodies. Those bodies, in turn, might rely on defaults or on chained delegation. The "missing" determination walks the full chain at compile time: a method is missing only if no provider exists anywhere along the resolution chain.

### 7.4 Forward-Compatibility Hedge

The user's wording "or some other means not currently identified" is a deliberate hedge — Basis preserves the option to admit future mechanisms (trait-style composition, deriving, automatic implementations from related classes, etc.) without re-litigating the assembly rule. The reference will record this hedge so future design dialogs know the door was intentionally left open.

### 7.5 Class-Evolution Implication

This rule has a structural consequence for class evolution: adding a *new declared method without a default* to an existing class breaks all existing instances at the source level — they would have to be amended in their declaring modules. Adding a new method *with* a default is backward-compatible. This is the standard typeclass-evolution trade-off; recorded in §3 of the reference because it's a load-bearing fact for module authors planning class versioning.

---

## 8. OQ-6 — Partial Application Beyond Receiver-Elision

### 8.1 The Resolution

Partial application is a **command-reference** (the bare `{...}` form) phenomenon. Other forms — command literals, lambdas, fexprs — do not engage in partial application; they have separate semantics (closure, body capture, etc.).

**Receivers must always be specified, never deferred.** For both single-receiver dispatched commands and v-commands, every receiver position in a partial application is bound at the partial-application site. This resolves dispatch (selects the witness) at that moment and captures it in the command-reference's closure environment.

**Non-receiver parameters may be applied or deferred.** Deferred parameters are marked with `_`; the resulting command-reference takes them as its remaining parameters at invocation time.

**The resulting command-typed value's type covers only the deferred parameters,** in their declaration order.

### 8.2 Surface Examples

```
{r :: cmd: x, _}              ; receiver r applied; x applied; last param deferred
{(r1, r2) :: vcmd: _, y, _}   ; v-command both receivers applied; first and third deferred
{cmd: x, _, z}                ; non-dispatched command; second param deferred
```

The v-command tuple form for multiple receivers requires grammar-extension treatment in OQ-23's batch.

### 8.3 Mode-Marker Filter

PRODUCE parameters cannot be applied; only deferred. A productive parameter is a slot the callee writes to, owned by the eventual caller of the invocation. Binding a productive parameter at partial-application time would be capturing a slot to write to later — closer to a lambda's `&` capture than partial application; crosses the four-form taxonomy boundary.

REFERENCE parameters, if applied, capture the reference and engage ceiling-tracking (lambda-and-fexpr §8). The command-reference's identity carries a captured reference; the ceiling rules apply.

READ / IN parameters may be applied or deferred freely.

### 8.4 Why Receivers Must Be Applied

The user's reasoning (mid-dialog after an initial unrestricted proposal): admitting deferred receivers would force the typechecker to track deferred dispatch — a form whose dispatch resolution awaits a future invocation when the receiver is supplied. *"In a completely dynamically typed language that would be quite useful, but I think this would be insane for Basis's typechecker to deal with."* Requiring receivers to be specified resolves dispatch at partial-application time and captures the witness, leaving only static-type elision for the deferred parameters.

### 8.5 `_` Token Consistency

The `_` deferred-parameter marker joins existing uses of `_`:

- Variant absent-state introduction in Aggregate literals (per CP015): `${kind <- _}`
- Variant operand sides of `-<` (per CP015): `v -< _` clears, `_ -< v` tests non-absent
- Deferred parameter in command-reference partial application (this resolution)

The unifying reading: `_` means "no value here / position is empty / placeholder for absence-or-deferred." Consistent across all three uses; recorded in the §3 prose of the reference.

### 8.6 Sub-Question Resolution Map

OQ-6 resolves all three CP001 §6 sub-questions:

| Sub-question | Resolution |
|---|---|
| (a) Surface | `_` deferral in command-reference form, with receivers always applied |
| (b) Resulting type | Covers only deferred parameters in declaration order |
| (c) Mode-marker interaction | PRODUCE deferred-only; REFERENCE applied captures with ceiling-tracking; READ/IN flexible |

---

## 9. OQ-16 — Overloading on Dispatched Commands

### 9.1 The Direction Selected

Despite the recommendation toward prohibiting overloading (in keeping with bright-line preferences), the user selected the opposite direction: *"given how generally useful overloading is, my inclination is to draw the bright line in the other direction: overloading is supported, and any unresolved ambiguity is an error."*

**Three layers of overloading are admitted:**

- **Within-class.** A class may declare multiple methods sharing a name with different parameter shapes.
- **Cross-class.** Multiple classes may declare methods sharing a name; types satisfying multiple of these classes can use any.
- **Non-dispatched.** Regular non-dispatched commands may share names with different signatures.

### 9.2 The Resolution Rule

**Most-specific-candidate wins by joint matching across receiver and argument types.** Genuine ties — multiple candidates equally specific — are compile errors. This parallels Q2(d)'s Julia-specificity-ranking framing applied to overload resolution; same bright-line, different domain.

### 9.3 Disambiguation: `{C::method}` Form

When a type satisfies multiple classes with same-named methods, ambiguity at the use site is an error unless the user disambiguates via the curly-brace class-qualification form:

```
t :: {C::process}                 ; explicit qualification: C's process
t :: {C::process}: arg1, arg2     ; with arguments
```

The form `{C::process}` is itself a command reference per the four-form taxonomy (CP016) — "the unbound command reference for `process` from class `C`." Combining with `t ::` produces "apply that command reference on receiver `t`." The double `::` (one for class-member access in `C::process`; one for class-dispatch in `t :: {...}`) is internally consistent with OPS §1.7's multi-role framing.

**Argument-shape disambiguation is by call-site argument types alone.** No syntactic shape qualifier (a `{C::method[shape]}` form) is admitted. The user's framing: *"let the disambiguation come from the call site in the number and type of supplied parameters, and if any ambiguity results, then the partial application fails at compile time."* Maintaining a single disambiguation form (class qualification) keeps the bright line.

### 9.4 Partial-Application Ambiguity Is a Compile Error

In partial application, deferred parameters have unknown values but known static types. Overload resolution at partial-application time uses the static types of *all* parameters (deferred and applied) to pick the overload. If multiple overloads match, ambiguity error. The user must disambiguate by:

- Applying more parameters (narrowing the set of matching overloads)
- Renaming one of the colliding methods
- Using the `{C::method}` form for cross-class collisions

### 9.5 Cross-Class Same-Name Collision

When type `T` satisfies multiple classes with same-named methods, `(t :: process)` is ambiguous and rejected; the user qualifies via `t :: {C::process}` to pick a specific class. Without qualification, the compiler reports "ambiguous: process from C₁, C₂, ..." with a list of visible candidates.

### 9.6 Receiver-Type Overloading IS Already Class Dispatch

A class method can have different bodies for different receiver types — that's how typeclasses work. So "overloading on receiver type" is already the standard mechanism; what's being added by OQ-16's resolution is overloading on *non-receiver argument* types (within a single class) and on cross-class collisions.

### 9.7 v-Command Interaction

Two v-commands with the same name and different receiver-tuple shapes? The same overload-resolution rule applies — most-specific-receiver-tuple-match wins; ties are errors. v-commands aren't class members in the same way as single-receiver methods, but the overload discipline is uniform across both.

---

## 10. OQ-29 — Liskov-Style Opening of the Failure-Tag Hierarchy

### 10.1 Reframe From Buffer-Backed-Subsumption to Class-Instance Terms

A recurring miscalibration in the dialog: the prior framing of OQ-29's payload-covariance constraint in buffer-backed-subsumption terms (per type-system §7.9) is *incorrect*. Buffer-backed subsumption is not a subtyping mechanism for failure propagation. The correct framing follows from failure-system §4.3 (already correctly stated there):

- A payload-bearing failure tag binds to a *class* (a Haskell-style typeclass), not a concrete type.
- A `.fail Tag: value` site supplies a value whose concrete type has an instance of that class.
- The class witness flows in the failure slot's third word.
- The handler `|: Tag t` operates on `t` through the bound class's operations only; concrete type is opaque.

Buffer-backed values can be supplied as payloads only if their type has an instance of the bound class — instance-of, not subsumption.

### 10.2 The Constraint: Same-Class Equality (Current); Class-Implication (Future)

Under the "covariance as the principle, same-class as the current concrete rule" framing the user requested:

- **Current rule:** When tag A extends tag B, A's payload class **equals** B's payload class. The hierarchy narrows only on the tag; payload classes don't vary. Liskov-trivially-preserved.
- **Forward-extension hedge:** When the language admits class-level hierarchies (superclass constraints, combined-class composition, or another mechanism for expressing "class C_A implies class C_B"), the rule relaxes to "A's payload class implies B's payload class." Payload variation down the failure-tag hierarchy becomes admissible at that point.

The covariance phrasing is preserved in the specification language as a forward-compatibility hedge, separating the *principle* (covariance) from the *current rule* (same-class equality, the trivially-covariant case).

### 10.3 Inheritance Default From Ancestry

A child tag's payload specification is inherited from its parent unless the child explicitly overrides. Inheritance chains transitively up the hierarchy; "no payload" is itself a specification that propagates. A payload-less root produces a payload-less hierarchy by default — declarations don't repeat the parent's payload class.

### 10.4 Three Sub-Rules for Explicit Override

**(a) Widening from "no payload" to a typed payload at a descendant: admissible.** Parent-keyed handlers don't bind, so no contract is violated. The user's reasoning: *"binding of payloads in a recovery handler is always optional irrespective of whether the tag admits a payload. Thus, widening from `_` to C is harmless, because a well-formed parent tag bearing no payload would not have a payload binding in its recovery specification."*

**(b) Typed-payload-to-typed-payload variation: constrained by covariance.** Currently realized as same-class equality; future relaxation to class-implication.

**(c) Typed-payload-to-no-payload narrowing: forbidden.** Removes binding capability that ancestor handlers' contracts may rely on. The user's confirmation: *"(i) is ABSOLUTELY the only correct answer here."* The asymmetry with (a) is rooted in the optional-binding property: optional-binding means handlers can choose not to bind, but if they do bind, the binding must be honorable.

### 10.5 Composition With Q2(c) Domain-Specific Dispatch

A noted coherence point: the witness flow through the failure slot composes cleanly with the runtime-domain-identity dispatch rule from OQ-5(c). When A extends B and a handler keyed at B catches an A-tagged failure:

- Static type of `t`: B's payload type (the class).
- Runtime value of `t`: an A-payload value (whose concrete type satisfies the same class, currently per same-class equality).
- Witness in the failure slot: for the runtime concrete type's instance of the shared class.
- Class dispatch on `t`: goes through the witness, dispatching to the runtime concrete type's instance.

The handler sees `t` as the class statically but dispatches on the actual runtime instance. Q2(c)'s runtime-domain-identity rule and OQ-29's covariance rule are two faces of the same coherence requirement.

### 10.6 Failure-Tag-Class-Witness Slot Closure

The failure-tag's class witness binding is separate from the spans-class concept used for variants (where the spans-class constrains all candidates). Failure tags don't have a "spans class" in the variant sense; each tag binds its own payload class (subject to the covariance rule down the hierarchy).

### 10.7 Terminology Forward-Pointer

The user noted (parenthetically): *"need better terminology than 'tag' at some point."* The overload of "tag" across:

- Failure-tag declarations and hierarchy
- Variant slot's tag identifier (the small integer identifying the active candidate)
- Failure slot's tag identifier
- Class-typed-value tag (RTTI, per §4 of this checkpoint)

is real and worth resolving in a future terminology pass. Recording this as a forward-pointer in the class-system reference's prose; resolution can come during a future consolidation pass.

---

## 11. Fexpr-Relevance as a Second Taint Axis

### 11.1 The Substantive Design Move

The user surfaced this as an organizing idea after the fexpr-related edge cases of Q6 (class-method fexpr-typed parameters) raised typechecking complexity concerns: *"we might want to consider fexpr-relevance to be a second type of taint to track, for ease of typechecking."*

Adopted as a substantive design move (the user's confirmation: *"Yes, let's formalize fexpr-tainting as the substantive design move."*). It supersedes ad-hoc per-case fexpr rules with a uniform discipline.

### 11.2 The Principle (Drafting for OPS §1.X)

> *Fexpr-relevance is a second taint axis, parallel to the READ contract (OPS §1.3). Taint sits on access paths, propagates per-frame, has no cross-frame propagation. A fexpr-relevance-tainted access path is ceiling-bound to the fexpr's defining frame `D`: it cannot escape via stores into longer-lived slots, via PRODUCE outputs to outer frames, or via capture by command-references whose own ceiling exceeds `D`. The taint flows from direct fexpr-typed values, from composite values containing fexpr-relevant content (variant candidates, command-reference bindings), and from captures of fexpr-relevant values. Per-frame analysis enforces the ceiling discipline uniformly across mechanisms.*

### 11.3 Sources of Taint

- Direct fexpr-typed values (parameters or locals with `<*>` types).
- Composite values containing fexpr-relevant content via structural composition: variants with fexpr candidates (currently disallowed per §12.4; future relaxation activates this), records or aggregates with fexpr-typed fields if such structures are admitted, captures of fexpr-relevant values in lambdas.
- Command references with bound fexpr-typed arguments (per §12.3).
- Anything else whose lifetime is tied to a defining frame `D` because of fexpr containment.

### 11.4 Enforcement

A fexpr-relevance-tainted access path:

- Cannot be stored into a slot whose lifetime exceeds `D`.
- Cannot be passed to a parameter slot whose ceiling is unbounded (e.g., escape via PRODUCE outputs to outer frames).
- Cannot be captured by a command reference that itself escapes `D`.
- Can be passed to other parameter slots within `D` and below, where the receiving frame's ceiling can accommodate it.
- Can be invoked (the fexpr fires, producing values that may or may not themselves be fexpr-relevance-tainted depending on the body's productive output).
- Can be stored in a local slot whose lifetime is bounded by `D`.

### 11.5 Coherence With Existing Principles

- **OPS §1.2 (each frame's analysis is local):** fexpr-relevance taint is per-frame propagated; no cross-frame escape, just like READ taint. The discipline migrates to the frame with context.
- **OPS §1.3 (access paths, not storage):** taint sits on access paths, not on storage. Aliasing through different access paths is the user's responsibility (or, more precisely, the typechecker's per-path tracking handles it).
- **The two taints are orthogonal axes.** A path can be untainted; READ-tainted; fexpr-relevance-tainted; both. Each is enforced independently.

### 11.6 Implications for Open Issues

- **Command references binding fexprs (§12.3):** the resulting command-reference is fexpr-relevance-tainted; ceiling-tracking enforced through the taint.
- **Defaults can't admit fexprs (§12.2):** a default body has no determinate `D` to bind the taint to; the typechecker rejects.
- **Variants with fexpr candidates (§12.4):** the variant becomes fexpr-relevance-tainted whenever a fexpr candidate is involved. Disallowing in v1 means the typechecker rejects with a "tainted-component-in-non-local-shape" error.
- **Class-method failure-set with fexpr invocations (§12.5):** the standard failure tag `FexprFailure` wraps fexpr invocations uniformly; specificity is sacrificed to keep failure-sets clean and orthogonal.

---

## 12. OQ-32 Sub-Item: Class-Method Fexpr-Typed Parameter Edge Cases

### 12.1 Class Methods With Fexpr-Typed Parameters Are Admitted

Class-method parameters are slots; fexpr-typed slots are admitted generally per CP016 / lambda-and-fexpr §6. No new mechanism needed for the basic admission; the structural composition is direct.

### 12.2 Defaults Are Incompatible With Fexpr-Typed Parameters

A class method declared with a fexpr-typed parameter cannot have a default implementation. The user's framing: *"default implementations should not admit fexprs; that should be a compile-time error."*

The fexpr-tainting machinery makes this naturally enforceable: a default body has no determinate `D` to bind the fexpr-relevance taint to. Instances inheriting the default would face an undefined frame relation; the only coherent rule is to forbid the combination. Compile-time error if a class declaration combines a fexpr-typed parameter with a default body.

### 12.3 Partial Application Of Class Methods On Fexpr-Typed Parameters

The user's mid-dialog correction overruled my initial "deferred-only" framing: *"You might have a 'do _something_ X number of times' partial application where you bind the something to the fexpr and leave the number of times to do it unbound. So long as the fexpr's hard-ceiling rules are respected, I don't see a reason to preclude this."*

So fexpr-typed parameters in partial application **may be applied** (bound at the partial-application site), with the resulting command reference inheriting fexpr-relevance taint and being ceiling-bound to the fexpr's `D`. Or they may be deferred (`_`) for later supply.

This supersedes CP016's commitment "command references cannot reference or bind fexpr-typed values" — the conservative-by-default rule is replaced by the fexpr-tainting machinery, which makes the binding tractable.

### 12.4 Variants With Fexpr Candidates: Tightened to Disallowed in v1

CP016 §6.5 admitted variants with fexpr candidates "permitted only when the variant inhabits a local slot." The user tightened this in the dialog: *"in theory, the fexpr candidate of the variant (which is sensible if the variant will be monomorphic) could come from a read parameter, but I'm concerned about the typechecking effort required to maintain the ceiling rules. Let's disallow fexprs in this version, with a note that support is intended later."*

So in v1: variants may not have fexpr candidates at all, regardless of slot location. Future relaxation intended once the fexpr-tainting machinery matures and the typechecker can enforce ceiling-tracking on variant-borne fexprs.

### 12.5 Failure-Set Treatment With Fexpr Invocations

A class method's declared failure set must accommodate fexpr invocations within its body, but the fexpr's specific failure profile is determined by the caller's code, not by the class declaration. The user selected the simple resolution: *"a simple standard 'fexpr failed' failure should suffice... I want to keep failure-sets clean and orthogonal."*

So a class-method body's invocation of a fexpr-typed parameter wraps the fexpr's failures in a single standard failure tag — `FexprFailure`. The actual underlying failure is opaque to consumers; specificity is sacrificed for clean failure-set declarations.

`FexprFailure` is registered as a forward direction (a future "standard failure tag library" thread) parallel to `CoercionFailure` from §5.5. Both are placeholder names for actual failure tags whose precise hierarchical position and payload-class-binding will be settled in that thread.

### 12.6 Other OQ-32 Sub-Items Remain Open

- Fexpr-typed return-from-call patterns.
- Transitive variant-fexpr-candidate containment rules (now subsumed under the disallow-entirely rule of §12.4 for v1; the rules become live again when relaxation happens).
- `-<` operator's interaction with fexpr-typed slots (now in fexpr-tainting context, but the precise semantics are deferred).

These remain forwarded to future threads.

---

## 13. Boundaries Against Op-Sem §§5.6–5.7

### 13.1 The Boundary

Op-sem §§5.6–5.7 own the *operational/runtime* mechanism for class dispatch:

- §5.6 introduces `::` as the class-resolution operator and the receiver-elision case `(receiver :: name)`.
- §5.7 covers v-commands and multiple dispatch operationally as composition of single-class dispatches.

This reference (class-system) owns the *static rules* and *dispatch tables*:

- The dispatch-table-lookup mechanism (which dictionary is consulted).
- The well-formedness rules for instance declarations.
- The receiver-mode rules (R1+R2 from CP005), captured here in §6 in full.
- The interaction with the witness-slot model.

### 13.2 R1+R2 Forwarding Redirection

A subtle but consequential touch-up: op-sem §5.7 currently forwards v-command receiver-mode treatment "to the type-and-modes reference for full treatment." The type-and-modes reference doesn't actually consolidate R1+R2 explicitly. With this reference's §6 absorbing R1+R2 in full per Q1's structural decision, op-sem §5.7's R1+R2 forwarding should redirect to class-system §6 instead of to type-and-modes.

This is a new touch-up to op-sem §5.7, added to work plan §6.2.

### 13.3 The `::` Multi-Role Usage

Per OPS §1.7 and §9.3, `::` appears in multiple roles in class-system contexts:

- Receiver-elision dispatch: `receiver :: method`
- Class-member access for disambiguation: `{C::process}`
- Doubled use: `t :: {C::process}` (receiver-dispatch outside, class-member-access inside)

The reference's §6 prose explicitly enumerates which role applies in each surface position so readers don't conflate them.

### 13.4 §3 Distinction Between Instance Dispatch and Context-Variable Resolution

Per Q2's resolution of OQ-13 (resolved in construction §13; class-system uninvolved), the class-system reference's §3 (Instances) needs a brief paragraph distinguishing:

- **Instance dispatch:** this reference's territory; resolution by `(class, type)` lookup; explicit at the call site via `::` or class-constrained type-variable.
- **Context-variable resolution:** construction reference's territory; resolution by uniqueness-of-type from the lexical scope; implicit at the call site.

The two mechanisms are conceptually parallel but distinct, and a careful boundary statement prevents conflation.

---

## 14. New Open Questions Registered

### 14.1 OQ-33 — Variant-Class-Typed-Parameter Slot Reuse-vs-Wrap

When a variant value is passed to a class-typed parameter, does the variant's existing 3-word slot get reinterpreted (since the variant slot's witness happens to be the right witness for the class-typed parameter, when class-membership permits), or is a new 3-word slot allocated?

Reinterpretation is more efficient but requires the variant's spans-class to *be* (or be a parent of) the class the parameter expects. Wrapping always works.

Resolution requires future design dialog. Recorded in work plan §5.4 (genuinely-open).

### 14.2 Forward Note on OQ-29 Relaxation Activation

The OQ-29 covariance rule's relaxation from same-class-equality to class-implication activates when the class system admits a way to express "class C_A implies class C_B" as a first-class relationship — superclass constraints, combined-class composition, or another mechanism. Until then, same-class is the rule.

Recorded as a note on OQ-29's entry in the OQ registry rather than as a separate OQ-34, since it's coupled to whatever class-hierarchy mechanism is eventually designed.

---

## 15. Pending Cross-Document Consolidations From This Dialog

These touch-up items are added to work plan §6 as a result of the 2026-05-04 class-system scope dialog:

### 15.1 Type-System Reference (work plan §6.3, expanded)

| Where | What |
|---|---|
| §3 (subsumption-relations) | Note that parameter-position admits class-name-as-existential as sugar for `∃a. Class a` (Case B); full treatment in class-system §3-§4. |
| §3.4 (variant 3-word slot) | Brief cross-reference to class-system §13 acknowledging that the 3-word slot pattern generalizes to class-typed values; class-system owns the class-typed-value details. |
| §3.5 (`-<` operator domain) | Revise to admit unions in `-<`'s domain (the prior exclusion based on no-runtime-tag reasoning is superseded by the existential-rule framing). Cross-reference construction §11. |
| §4 (parameter-mode system) | New sub-section on the second taint axis (fexpr-tainting), parallel to the READ contract treatment. |
| §7.5 (OQ-6 forwarding) | Mark OQ-6 broader-scope resolved in class-system §7; redirect. |
| §7.6 (OQ-5 forwarding) | Mark OQ-5 sub-questions (a)+(b)+(c)+(d) resolved in class-system §8; redirect. |
| §7.9 (OQ-29 forwarding) | Rewrite the OQ-29 statement in class-instance terms, not buffer-backed-subsumption terms; phrase as covariance principle with same-class as current concrete rule; note the forward-extension hedge. Cross-reference class-system §10. |
| §7.10 (OQ-16 forwarding) | Mark OQ-16 resolved in class-system §9; redirect. |

### 15.2 Failure-System Reference (work plan §6.1, expanded)

| Where | What |
|---|---|
| §4.2 (Tag Declaration Semantics) | Add paragraph on the inheritance default from ancestry; the three sub-rules for explicit override (widening allowed; typed-payload-variation under covariance; typed-to-no-payload narrowing forbidden). |
| §4.3 (Class-Bound Payloads) | Confirm framing as already correct; cross-reference class-system §10 for the cross-module-extension treatment. |
| §7.6 (OQ-28) | Close OQ-28 (open under OQ-29's covariance constraint); reframe in class-instance terms (not buffer-backed subsumption); cross-reference class-system §10. |
| §7 (new entry or appendix) | Register `FexprFailure` and `CoercionFailure` as forward items for a future "standard failure tag library" design thread. |

### 15.3 Op-Sem Reference (work plan §6.2, expanded)

| Where | What |
|---|---|
| §5.7 (R1+R2 forwarding) | Redirect R1+R2 receiver-mode forwarding from type-and-modes to class-system §6 (new touch-up from this dialog). |

(The §5.5 OQ-13 redirect and §7 supersession are already on the §6.2 batch from CP015 and CP016.)

### 15.4 Construction Reference (work plan §6.4, expanded)

| Where | What |
|---|---|
| §3.5 (variant-fexpr-candidate constraint) | Update to disallowed-entirely (tightening from CP016's local-slot exception). |
| §10 (union → candidate-or-parent reinterpretation) | Note that the explicit `-<` form is also admitted; cross-reference §11 for the unified treatment. |
| §11 (`-<` operator) | Substantial revision: full general formulation as dynamic type coercion; case-analysis decision rule; case enumeration (variant; class-typed; union; concrete-types); cross-references to class-system §4 and class-system §11; supersession of the "command-typed-value cases don't apply to fexpr-typed values" verification (now subsumed by the fexpr-tainting machinery). |

(The §6, §13 touch-ups are already on the batch from CP016.)

### 15.5 Lambda-and-Fexpr Reference (work plan §6.6, newly added)

| Where | What |
|---|---|
| §3 (command references) | Admit fexpr-typed bindings via fexpr-tainting machinery; supersede CP016 "cannot bind" commitment with a reconciliation marker citing this checkpoint. |
| §5 (lambda captures), §6 (fexpr capture-by-free-name) | Class-typed (Case B) captures carry both witness and tag; type-variable-bound (Case A) captures bring hidden-parameter witness as part of closure environment. |
| §6.5 (variant-with-fexpr-candidate) | Tighten to disallowed-entirely; remove the local-slot exception. |
| §8 (ceiling tracking) | Re-express ceiling rules as instances of fexpr-tainting flow rather than per-form ad-hoc rules. |
| §10 (forwarded items) | Update OQ-32 status: class-method fexpr-typed parameter sub-item resolved in class-system §11; command-reference relaxation sub-item resolved as side effect. |

### 15.6 Operating-Principles Document (work plan §6.5, expanded)

Two new pending updates beyond the five already batched:

| Update | Source |
|---|---|
| §1.X new — `-<` as multi-role dynamic type coercion operator | This dialog (parallel to §1.7's `::` framing). |
| §1.X new — Fexpr-relevance as a second taint axis | This dialog (parallel to §1.3's READ-contract taint). |

Plus a forward-pointer in prose (no formal §) on terminology disambiguation across "tag" uses (failure, variant, RTTI). Resolution in a future terminology pass.

---

## 16. Decision Counts and OQ Status Changes

| | Before this dialog | After this dialog |
|---|---|---|
| Resolved OQs (total) | 22 | 28 |
| Newly resolved here | n/a | 6 (OQ-5 a/b/c/d, OQ-6, OQ-16, OQ-29, OQ-32 partial) |
| Genuinely-open OQs | 4 (OQ-30, OQ-31, OQ-32, record padding) | 5 (OQ-30, OQ-31, OQ-32 reduced, record padding, **OQ-33 newly registered**) |
| Forwarded OQs | 8 | 1 (just OQ-32 remaining sub-items) |
| OPS pending updates | 5 (3 from CP015 carved + 2 awaiting content from CP016) | 7 (5 prior + `-<` principle + fexpr-tainting principle) |
| Cross-document consolidation batches | 5 (§§6.1–6.5) | 6 (§§6.1–6.5 + new §6.6 for lambda-and-fexpr) |

OQ-32 was resolved partially: the class-method-fexpr-typed-parameter sub-item resolves here, and the command-reference-relaxation sub-item resolves as side effect. Three remaining sub-items continue forwarded.

---

## 17. Provenance

**Authored:** Distilled by Claude (Opus 4.7) on 2026-05-04 from the turn-by-turn dialog with JimDesu in the class-system-reference scope-confirmation thread, following the 2026-05-04 completion of the Lambda and Fexpr reference (CP016) and the 2026-05-03 completion of the Construction and Initialization reference (CP015). The class-system reference itself was *not* drafted in this session; per OPS §2.8 (honesty about capacity), the dialog produced substantially more carved-in commitments than typical scope-confirmation dialogs, and combining drafting with this volume of scope work would have pushed context budget tight. Drafting will resume in a fresh session.

**Source materials read for this checkpoint:** `project-operating-principles.md` in full (the lens for all design reasoning); `project-work-plan.md` §3 (the seven scope-confirmation questions and their context); `intent-checkpoint-001.md` §§4 (classes, instances, dispatch), §6 (OQ-5, OQ-6, OQ-13, OQ-16 framings); `intent-checkpoint-007.md` §7 (OQ-16 candidate restriction framing); `intent-checkpoint-012.md` §§1.2, 3.1 (class-bound payloads for failures); `reference-failure-system.md` §§4.1, 4.2, 4.3, 4.4, 4.11, 4.12, 5.1 (failure-tag class-payload mechanics); `reference-operational-semantics.md` §§5.5, 5.6, 5.7 (boundary points for op-sem); `reference-type-system-and-modes.md` §§3.4, 3.5, 5.2, 7.5, 7.6, 7.7, 7.8, 7.9, 7.10 (variant 3-word slot, dynamic narrowing, OQ forwardings); `reference-construction-and-initialization.md` §§3.5, 5.4, 5.5, 10, 11 (variant construction, `.implicit`, union byte-reinterpretation, `-<` operator); `reference-lambda-and-fexpr.md` §§3, 5, 6, 8, 10.7, 10.8, 11 (four-form taxonomy, captures, ceiling tracking, OQ-32 framing).

**Grammar changes implied:**

- New `(T:Class)` form for class-constrained type-variable introduction in parameter declarations. Grammar extension; added to OQ-23's implementation-thread batch.
- The same `(T:Class)` form admitted at v-command receiver positions; grammar extension.
- New `{C::method}` form for class qualification disambiguation in class-method-resolution contexts; the curly-brace form is consistent with the four-form taxonomy's command-reference reading.
- `_` token use extended to deferred parameters in command-reference partial application (joining its existing roles in variant absent-state and `-<` discard markers).
- No new operators or block markers.

**Reference-document impact:**

- The class-system reference (the sixth and final topic-organized reference) will encode all of the above from inception when drafted in a fresh session.
- Type-system reference §§3, 3.4, 3.5, 4, 7.5, 7.6, 7.9, 7.10 receive substantial touch-ups (work plan §6.3).
- Failure-system reference §§4.2, 4.3, 7.6 plus a new entry receive touch-ups (work plan §6.1).
- Op-sem reference §5.7 receives a new touch-up (work plan §6.2).
- Construction reference §§3.5, 10, 11 receive substantial touch-ups (work plan §6.4).
- Lambda-and-fexpr reference §§3, 5, 6, 6.5, 8, 10 receive substantial touch-ups (work plan §6.6 newly added).
- Operating-principles document receives two new pending updates (`-<` principle, fexpr-tainting principle), batched alongside the existing five (work plan §6.5).
- Work plan §5 OQ registry adds OQ-33 (§5.4 genuinely-open) and moves OQ-5 (a/b/c/d), OQ-6, OQ-16, OQ-29 to §5.1 resolved (with class-system §§ as the resolution sites once drafted).

**Recommended next step.** Resume topic-organized reference work in a fresh session with full context budget. The next session drafts the **Class System** reference (#6), the last item in the agreed sequence, encoding all commitments from this checkpoint from inception with reconciliation markers. The drafting session should follow operating-principles §2.1 (scope confirmation before drafting, already complete via this dialog) and §2.8 (honesty about capacity); the reference is calibrated for 800–1200 lines based on the volume of carved-in commitments.

After the class-system reference is complete, the topic-organized consolidation work has covered the agreed six-reference sequence. The remaining work is the consolidated pass to apply all batched cross-document touch-ups (work plan §§6.1–6.6), then carving in any awaited operating-principles content from JimDesu (§§2.1 sharpening, §2.9 new from CP016, plus the two new principles from this dialog). After that pass, the project's authoritative documents are fully current.

Concurrently, JimDesu may consider when to schedule design dialogs for the still-forwarded items: OQ-30 (heap allocation), OQ-31 (tuple-positional access), OQ-32 remaining sub-items (fexpr-typing ramifications), OQ-33 (variant-class-typed-slot reuse-vs-wrap), and the standard-failure-tag library thread (`FexprFailure`, `CoercionFailure`, and others). These can be sequenced as desired; they're independent of the topic-organized reference work.
