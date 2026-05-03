# Basis Language — Reference: Type System and Modes

**Status:** Topic-organized authoritative reference for the Basis type system and the parameter-mode system as a type-level feature. Consolidates material from CP001 (foundational type-system overview), CP003 (nominal typing), CP004 (parameter-mode mechanics, initialization analysis), CP005 (marker syntax, R1+R2 receiver rules, OQ-11 and OQ-12 resolutions, downcast as named topic), CP006 (the buffer-backed principle made structural; OQ-9 resolution), CP008 (marker placement in nameless command-type-expressions; capture-list mode rules), CP009 (write-once productive sharpening; bare-identifier `<-`), CP013 (READ/PRODUCE/REFERENCE rename and the transitive read-only contract), and CP014 (the OQ-3 resolution as it bears on type-side `-> name` interpretation). Cross-references the failure-system and operational-semantics references for material best owned there; defers all construction-pattern material to the planned construction-and-initialization reference.
**Date:** 2026-05-02
**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` and continued with `reference-operational-semantics.md`.

**Authority statement.** Where this reference differs from the source checkpoints on a covered topic, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative. Where this reference and an *earlier* reference differ on overlapping material, this reference is authoritative on the type-side aspects and the earlier reference is authoritative on the operational/failure-side aspects, with the bridge notes already in place flagging the boundary.

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]*. Section-level connective tissue carries footer citations identifying the section spans drawn upon. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints — typically resolving an ambiguity, recording a post-checkpoint correction, or filling a gap in the source material. **Cross-references** to the failure-system and operational-semantics references take the form *[failure-system §x]* and *[op-sem §x]* and point to where related aspects are treated; **forward references** to planned but undrafted references are flagged as such.

**Standing design principle: frame-ownership.** A pervasive lens that this reference applies throughout the parameter-mode treatment, and that bears on the type-system at multiple points: the slot a value lives in is owned by *some* frame; "return" in the conventional sense does not exist in Basis (output flows through writeable parameter slots owned by the caller); reasoning about value semantics, mode contracts, aliasing, and lifetime ceilings proceeds by asking *which frame owns this slot*. Each command body's static analysis (initialization, failure-mode, access-path taint) is local to its own frame and operates on its own parameter modes; there is no cross-frame propagation of these properties. Region-style memory reclamation feasibility is a downstream concern that this lens preserves; the reference does not argue for region reclamation but is careful to not preclude it. *[Reconciliation: the frame-ownership lens is articulated in this reference for the first time as a standing principle. It is implicit in CP001's "no return values; output via writeable parameters" stance, in CP002's object-lifetime-ceiling rule, and in CP013's transitive READ contract; it is made explicit here because subsequent design hinges on it.]*

---

## 1. The Buffer-Backed Principle

### 1.1 The Two-Layer Split

Every type in Basis falls into exactly one of two categories. *[CP006 §1.1]*

**Buffer-backed types.** Types whose representation reduces — transitively — to bytes. They are value-like, byte-copyable, slice-able into byte-aggregates, and may sit inside other byte-aggregates. The category includes:

- The bracket forms `[N]` and `[]` (the buffer primitive — fixed-size and unbounded byte storage).
- The bracket forms `[N]T` and `[]T` (typed views — buffers laid out as sequences of `T`-values, where `T` is itself buffer-backed).
- Plain domains (named refinements over a buffer or another buffer-backed type).
- Records (named refinements over a buffer with named field-offset structure).
- Unions (byte-level overlays of candidate domains; see §2.6 for the discriminator question).

**Non-buffer types.** Types whose representation includes references, identity, dispatch information, or other non-byte semantics. The category includes:

- Pointers (`^T` for any `T`) — references to slots; carry target identity, not a byte aggregate.
- Command-typed values (`:<...>`, `?<...>`, `!<...>`) — first-class commands carrying dispatch metadata and (for block-quote forms) capture information.
- Objects — identity-bearing aggregates with potentially non-contiguous storage; their fields may transitively contain non-buffer fields.
- Variants — reference-semantics tagged sums whose candidates may be arbitrary types; see §3.4 for the discriminator-and-witness slot.

The split is fundamental. It governs which types may appear in which positions, how each kind of type is initialized, and how each kind of type interacts with the storage model. *[CP006 §1.1]*

### 1.2 The Containment Rule

The buffer-backed principle imposes a strict containment rule that gives the principle its teeth: **a buffer-backed type may contain only buffer-backed types.** *[CP006 §1.2]*

Specifically:

- A record's fields must all be buffer-backed types. A record may not contain a pointer, a command-typed value, an object, or a variant. To refer to other storage indirectly, a record stores **offsets, not pointers**: an offset is just bytes; a pointer is a reference. Only the former is buffer-acceptable.
- A union's candidates must all be buffer-backed types.
- A typed buffer `[N]T` requires `T` to be buffer-backed.
- A domain's parent must be buffer-backed (already required: the parent must transitively reduce to a buffer or another buffer-backed type).

Non-buffer types may appear only at top-level positions (slots introduced via `#`, parameters, receivers) or as fields of objects. An object's fields are unrestricted as to category — they may hold buffer-backed *or* non-buffer types. Variant candidates are similarly unrestricted. *[CP006 §1.2]*

The containment rule is what makes the buffer-backed principle structural rather than aspirational: every record's bytes are unambiguously a byte-aggregate, every typed buffer's elements are byte-aggregates, every union's overlay is a byte-overlay, and any non-byte semantics is confined to top-level slots and object fields.

### 1.3 Grammar Already Enforces This

Direct inspection of `Grammar2.cpp` (master branch) confirms that the buffer-backed containment rule is enforced at the grammar level. *[CP006 §1.3]*

- `DEF_RECORD_FIELD` uses `TYPE_EXPR_DOMAIN` for field types.
- `DEF_UNION_CANDIDATE` uses `TYPE_EXPR_DOMAIN` for candidate types.
- `TYPE_EXPR_DOMAIN` admits only `DEF_INLINE_RECORD`, `DEF_INLINE_UNION`, `TYPE_NAME_Q` (named domain or record), and `TYPE_EXPR_VECTOR_FIXED` followed optionally by another `TYPE_EXPR_DOMAIN`.
- `TYPE_EXPR_DOMAIN` does **not** admit `TYPE_EXPR_PTR`, `TYPE_EXPR_CMD`, `DEF_INLINE_OBJECT`, or `DEF_INLINE_VARIANT`.

Object fields and variant candidates use `TYPE_EXPR` (which admits everything). The grammar's two-tier `TYPE_EXPR_DOMAIN` / `TYPE_EXPR` distinction corresponds exactly to the buffer-backed / non-buffer split. No grammar changes are needed to enforce the principle.

### 1.4 Records Are A Form Of Domain

A consequence of §1.1 and §1.2: **records and domains are the same fundamental concept, surfaced under different declaration forms.** *[CP006 §1.4]*

A domain is a value-like nominal type over a buffer. A record is a value-like nominal type over a buffer with named field-offset structure. The grammar's separate `DOMAIN`, `RECORD`, and `UNION` declarations are surface conveniences for declaring different shapes of buffer-backed types:

- `DOMAIN` declares a buffer-backed type with no field structure (or whose structure is whatever the parent provides).
- `RECORD` declares a buffer-backed type with named field-offset structure.
- `UNION` declares a buffer-backed type as a byte-level overlay of candidates.

All three produce buffer-backed types with nominal identity (§5.1 below). All three may serve as record fields, union candidates, or domain parents. The unity is the load-bearing fact; the surface declarations are conveniences. This reference uses **"buffer-backed types"** as the umbrella term and the specific declaration form (domain, record, union) where the surface distinction matters.

### 1.5 Why This Matters

The buffer-backed principle and its containment rule are not technical fastidiousness. They are what make several other language commitments operational:

- **Mutation either succeeds fully or fails fully.** Buffer-backed types copy as bytes; copy-restore at the call boundary writes back the byte-image atomically on success and not at all on failure (§4.4 below). A record-with-pointers would punch a hole in this: the pointer copies, the pointee doesn't, and partial-failure semantics drift. The containment rule eliminates this concern at the type level.

- **Whole-slot initialization tracking.** OQ-9 was resolved (CP006 §2) in favor of whole-slot tracking with atomic compound construction, rather than per-field state. The buffer-backed principle is what makes this enough: a buffer-backed slot is one byte-aggregate, not a graph; tracking it as a unit is sound because no field can have non-byte semantics. *[CP006 §2; OQ-9 closed.]*

- **Region-style memory reclamation feasibility.** Buffer-backed values do not own non-local storage; they live in their owning frame's bytes and are reclaimable at frame retirement without traversal. Non-buffer types are confined to positions where ownership and lifetime are explicit (top-level slots, object fields with object-lifetime ceiling, frame-bound parameters and receivers). The lattice that the language can reclaim by frame is the buffer-backed lattice; the non-buffer lattice has explicit ownership at every join.

- **The frame-ownership lens applies cleanly.** Buffer-backed values move with their owning frame; non-buffer types either *are* references (pointers) or have explicit lifetime ceilings tied to frames (objects, variants).

*Sources for §1: CP006 §§1, 2, 4.1; CP001 §2 (the underlying type-form descriptions, structurally refined by CP006); CP003 §1 (nominal typing for buffer-backed types).*

---

## 2. Buffer-Backed Types

### 2.1 Buffers and Ranges

The bracket form `[N]` denotes an `N`-byte buffer. The form `[]` denotes an unbounded byte buffer. The forms `[N]T` and `[]T` denote buffers laid out as a sequence of `T`-values, sized either to `N` or unbounded. Buffers are the substrate over which all value-like types are interpreted. *[CP001 §2.1]*

A `[4]` is four bytes — nothing more. What those four bytes *mean* (a 32-bit integer, an RGBA pixel, a packed pair of `Int16`, a Unicode code point) is determined by the **domain** layered on top of it (§2.2). There is no privileged interpretation of any byte width.

**Indexing into a buffer-shaped value uses the suffix `[index]` syntax.** Indexing is failable: out-of-bounds is a first-class failure, not undefined behavior. Indexing on a domain works because domains are themselves buffer-backed; indexing is not, however, *guaranteed* to be meaningful for every domain — domain-specific operations may forbid it. *[CP001 §2.1]*

C-style pointer arithmetic is not supported. Stepping through buffer contents requires `[i]`. This restriction is also necessary for the handle-implementation of pointers (§3.1) to be valid, since handles relocate. *[CP001 §2.6]*

### 2.2 Domains

A domain is a **new type** declared in terms of a parent type, where the parent must reduce (transitively, through any aliases) to a buffer-backed type. Pointers, command-typed values, objects, and variants cannot be domain parents because the purpose of a domain is to give a refined interpretation to a definite chunk of bytes. *[CP001 §2.2]*

Domains form a parent–child hierarchy with **one-directional implicit upcasting**: a value of a child domain is implicitly accepted wherever the parent domain is expected, but a value of the parent domain is *not* implicitly a value of any specific child. So an `Inches` is implicitly an `Int32`, but an `Int32` is not implicitly an `Inches`. Sibling domains (e.g., `Inches` and `Centimeters` both parented at `Int32`) do not implicitly convert to each other. The treatment of class dispatch under this hierarchy — and the question of whether implicit upcast loses domain identity at parameter boundaries — is the subject of OQ-5(c) and is forwarded to the class-system reference; the **type-side** rule recorded here is that the upcast is a typing-acceptance rule, not a value-rewriting rule. The concrete value passing across an upcast boundary retains its declared domain identity for purposes of dispatch. *[CP001 §2.2; OQ-5(c) forwarded.]*

Domains are first-class types: they may be parameters, fields, receivers of class instances, expression-position results via `-> name`, and so on. *[CP001 §2.2]*

**Notable consequence.** Because there are no privileged primitive types, the standard library defines `Int32`, `UInt32`, `Float32`, `Int8`, etc. as domains over `[N]` for the appropriate `N`, with associated intrinsics for arithmetic and comparison. User-defined domains use the same mechanism — there is no conceptual distinction between standard-library domains and user domains. *[CP001 §2.2]*

The parent-child hierarchy described in this section is the *narrow-sense* domain hierarchy (introduced via the `domain` declaration form). The same hierarchy mechanism applies more broadly to all buffer-backed types — records, unions, and the bracket forms also participate, per §1.4's records-are-domains unification. The full subsumption picture is in §5.2.

### 2.3 Aliases

An alias is a **synonym** — purely a human-ergonomics tool. The alias name and the right-hand side are interchangeable in both directions in all contexts; aliases erase entirely from the type system's perspective. They introduce no new type identity. *[CP001 §2.3]*

This makes aliases useful as **type-level abstraction barriers**: a module exposes `alias UserId: Int64`, downstream code uses `UserId` everywhere, and a later implementation change to (say) `[16]` (a UUID) requires changing only the alias declaration, assuming the change is otherwise compatible.

Aliases compose with domains: a domain's parent may be an alias (which transparently resolves to the underlying type). This would be an edge case but is permitted.

The line between alias and domain is the line between *no new type identity* and *new type identity with implicit-upcast to the parent*: an alias erases; a domain with the same right-hand side does not. The choice between them is the choice between a renaming-only abstraction and a typing-distinct refinement.

### 2.4 Records

A **record** is a contiguous, byte-addressable buffer with named field offsets. Value-like semantics: copyable as bytes, no identity beyond byte-content, fields laid out at deterministic offsets. *[CP001 §2.4; CP006 §1.4 unifying with domains.]*

Record fields are constrained to buffer-backed types (the `TYPE_EXPR_DOMAIN` form in the grammar; §1.3 above), reflecting the requirement that every field have a definite byte width and offset. A record's total byte size is the sum of its fields' sizes plus any padding the implementation introduces for alignment; the surface specification does not currently pin down padding rules, and this is recorded as a minor open question (§7.3).

Records are nominally typed: two `record` declarations with identical field structure produce two distinct types. Values of one are not interchangeable with values of the other (subject to alias erasure if either is declared as an alias of the other). *[CP003 §1.]*

**Record-in-record composition.** A record may have a field whose type is another record; the inner record's bytes lay out within the outer record's bytes contiguously at the inner record's offset. The two records remain nominally distinct types (per the nominal-typing rule above), and the outer record's namespace introduces the inner field by name only — accessing the inner record's *own* fields requires walking through the outer field's name, using the scope operator `::` (§3.2) for field access:

```
.record Pair :
    x : Float32
    y : Float32

.record Frame :
    label : Int32
    p : Pair
    flag : Int32

# f : Frame <- ...
f::p             ; the Pair value (8 bytes; can be passed as a Pair, or as [8], or as [])
f::p::x          ; the x component of the contained Pair
```

The `::p::x` chain is mandatory under this regular form: `f::x` does not resolve, because `Frame` does not have its own `x` field — it has a `Pair`-typed `p`.

**The `.inline` modifier.** A field-declaration modifier `.inline` indicates that the included record's field names are added to the outer record's namespace *as if those fields were declared directly*. The byte layout is unchanged from the regular form (the inner record's bytes still lay out at its offset within the outer record); what changes is the namespace.

```
.record Frame :
    label : Int32
    .inline p : Pair
    flag : Int32

# f : Frame <- ...
f::p             ; the Pair value, still accessible by its field name
f::p::x          ; the contained Pair's x, still accessible through the chain
f::x             ; ALSO works under .inline — direct access to the Pair's x
f::y             ; ALSO works under .inline
```

Both surfaces — `f::x` directly and `f::p::x` through the field — refer to the same byte offset within `f`; there is no implementation-level distinction between the two access paths. The `.inline` modifier is purely a namespace convenience.

**`.inline` retains record-typed access.** Critically, `.inline p : Pair` does *not* erase `p` as a name; the Pair-typed handle is preserved in both flavors. This is essential when the inner record needs to be passed as a `Pair` value (or as a buffer per the buffer-backed hierarchy of §5.2): `f::p` is a `Pair` — and therefore implicitly acceptable wherever `[8]` or `[]` is expected — regardless of whether `p` was declared regular or `.inline`. The `.inline` form is *additive* on namespace, not destructive: the field-name access through `p` still works, and the flattened names additionally work.

**Naming collisions under `.inline` are a compile-time error.** Because `.inline` flattens the inner record's field names into the outer record's namespace, any collision between an inner field name and an outer field name (or between two distinct inlined records' field names) is a compile-time error, detected at the outer record's declaration site. To resolve, the user renames a field, drops the `.inline` modifier on the colliding inclusion, or introduces an alias. The same rule extends transitively: if an `.inline`'d record itself contains an `.inline`'d sub-record, the flattened-into-the-outer namespace must remain collision-free across all levels.

**Grammar implication.** The `.inline` keyword is added to the record-field-declaration grammar as an optional modifier on a field whose type is itself a record. The exact surface placement (before the field name, before the type, or as a separate annotation form) is a minor grammar question forwarded to the implementation thread alongside OQ-23 (§7.11); the semantic rules above are independent of the surface placement. *[Reconciliation: this section is added per the user's 2026-05-02 review. Record-in-record composition was previously summarized in a single paragraph; this expansion makes the field-naming convention via `::`, the `.inline` modifier, and the collision rule explicit at the type-side.]*

### 2.5 Records vs. Objects: The Parallel Split

Records and objects share a parallel surface syntax (named, field-based aggregates) but are categorically distinct in ways that propagate through the rest of the type system. *[CP001 §2.4; CP006 §1.1.]*

The parallel matches the buffer-backed / non-buffer split exactly:

- **Record** — buffer-backed, value-like, field types restricted to buffer-backed (the `TYPE_EXPR_DOMAIN` grammar). Fits inside other byte-aggregates; copies as bytes; has no identity beyond byte-content.
- **Object** — non-buffer, identity-bearing, field types unrestricted. Cannot fit inside a buffer-backed type; manipulated through fat pointers; has identity that survives byte-content equivalence.

Object treatment proper is in §3.3. The parallel is recorded here so that the union/variant parallel (§2.6 / §3.4) reads against it.

### 2.6 Unions (OQ-1, Buffer-Backed Half: Resolved)

A **union** is a byte-level overlay of candidate domains. The union's storage is `max(candidate-sizes)` bytes; assigning a candidate value writes that candidate's bytes into the overlay. The union's identity is value-like: two union values with the same active-candidate interpretation and the same byte content are the same value. *[CP001 §2.5]*

A union may appear in two roles: **as a field of a containing record** (the typical embedded-overlay case, where the user-managed discriminator usually lives in an adjacent field of the same record), or **as a standalone buffer-backed type in its own right** (a top-level slot whose type is a named or inline union). Both roles produce buffer-backed types subject to the same rules; the standalone form is just a union taken on its own merits as a domain-shaped value. *[Reconciliation: the standalone-union role was implicit in CP001 §2.5 and made explicit in the user's 2026-05-02 review; updated here to reflect both roles.]*

Union candidates must be buffer-backed (per §1.2) — the candidate-types grammar constraint is `TYPE_EXPR_DOMAIN`. This is what makes the byte-overlay coherent: every candidate has a definite byte-width, and the overlay is the maximum.

**OQ-1, union case: resolved as untagged byte overlay.** A union is *just bytes*; the language carries no language-level discriminator inside the union slot. Discrimination is the user's responsibility, typically by storing an enumeration value alongside the union in a containing record, or by deriving the active-candidate from contextual information already implicit in the program. *[CP001 §2.5; OQ-1; resolved here. Reconciliation: the failure-system reference §7.7 recorded OQ-1 as "general union-discriminator question remains" and suggested the failure-tag subcase did not constrain the general question. This reference resolves the buffer-backed-union half of the general question, leaving the variant half to §3.4. The user-driven-discriminator choice is the buffer-backed-side answer; the variant-side answer is a 3-word slot, structurally analogous to typed failures.]*

**Operational consequence: interpretive casting.** Reading a candidate value out of a union slot is an interpretive cast — a reinterpretation of bytes at a candidate-typed slot view. The cast operator's surface syntax is part of the `<-`-family of construction-and-initialization operators (along with the dynamic-narrowing `-<` operator from §3.5 below) and forwards to the construction-and-initialization reference. The type-side commitment recorded here is that interpretive casting is the read mechanism for unions, and the typechecker enforces only that the cast's target type is one of the union's declared candidates; it does not verify which candidate is actually active.

**Why the untagged-overlay choice.** A language-level discriminator on every union would force a discriminator-byte allocation cost on every union and would commit the language to a witness mechanism inside the buffer-backed side of the type system. Both costs are inappropriate for the lower-level coding role unions occupy; programs that want safe tagged sums use variants (§3.4). Programs that want byte-overlay efficiency at the cost of programmer-managed discrimination use unions. The two surfaces serve different needs, and the distinction is preserved by giving each its own discrimination model. *[Reconciliation: this rationale is post-CP001. CP001 §2.5 recorded the discriminator concern as "OQ-1" without resolution; the post-CP012 dialog and the present reference settle it on the union side with this rationale.]*

The grammar accepts the untagged shape directly: a `DEF_UNION_CANDIDATE` lists candidate types without admitting a discriminator-field declaration. No grammar change is needed.

### 2.7 Inline vs. Named Forms

For each of the buffer-backed compound forms (record, union), the grammar admits both **named declarations** (introducing a new nominal type into the surrounding scope) and **inline forms** (anonymous, used directly in a field or candidate position). *[CP001 §2.5 (implicit); grammar inspection.]*

The inline forms produce nominally distinct types per declaration site (so an inline record nested inside two separate outer records is two distinct types, even if their field structures are identical). The named forms produce a single nominal type usable at multiple sites. The choice between them is an ergonomics-and-reuse choice; the type system treats both uniformly under the nominal-typing rule (§5.1).

*Sources for §2: CP001 §§2.1–2.5; CP006 §§1, 2; CP003 §1; OQ-1 (union case) resolved here.*

---

## 3. Non-Buffer Types

### 3.1 Pointers

Pointer types are written `^T`. The caret is a type-prefix and may stack: `^^T` is a pointer to a pointer to `T`. A pointer's *expression-position* operators are the `^` suffix (dereference) and the `&` suffix (address-of). *[CP001 §2.6]*

The language commits to **abstracted pointer semantics**. The user-visible meaning of `^T` is "pointer to `T`." Whether the runtime implementation is a thin pointer, a fat pointer (with embedded type tag and dispatch metadata), or a handle (a pointer to a pointer, supporting relocation) is an implementation choice, made per type by the compiler, and not user-visible. *[CP001 §2.6]*

The implementation latitude is what permits the language to choose, on a per-`T` basis, the representation that best serves `T`'s actual needs:

- A pointer to a buffer-backed `T` of small fixed size may be a thin pointer (no RTTI; the static type is enough).
- A pointer to an object may be a fat pointer (carrying the dispatch dictionary's address alongside the storage address, supporting class dispatch and the `^` deref-then-method-call patterns).
- A pointer to anything in a relocation-supporting allocator may be a handle.

User-visible operations on `^T` are uniform across these choices: dereference, address-of, indexing into a buffer-typed pointee, member access for object pointees, dispatch via `::`. The user reasons about pointers in the abstract; the implementation chooses the concrete shape.

**No C-style pointer arithmetic.** Stepping through buffer contents requires the indexing syntax `p[i]`. Incrementing a pointer to advance through an array is not supported. Indexing produces a first-class failure on out-of-range access; pointer arithmetic would not. The restriction is also necessary for the handle implementation to be valid (a handle's underlying address may move; an arithmetic offset against it would alias the wrong target after relocation). *[CP001 §2.6]*

**`^Object` is essentially a double-indirection.** Object-typed values are themselves indirections (objects are stack/heap-allocated with non-contiguous fields, accessed via fat pointers). A `^Object` parameter therefore points to a *slot* containing an object reference, and a writeable `^Object` parameter allows the callee to point that slot at a different object on success. The double-indirection structure is what lets writeable-`^Object` parameters serve as object-yielding result slots in the no-return-values world. *[CP001 §2.6]*

**Pointers and the no-non-local-state principle.** Reading from `^T` and writing to `^T` do not, in themselves, violate the no-non-local-state principle: the pointer is itself a parameter (or transitively reachable from one through the provision chain), and operations *through* the pointer touch storage that arrived by explicit provision. The provision-chain rule from CP007 is preserved. *[CP001 §2.6; op-sem §3.3.]*

The CP013 transitive READ contract refines this rule for the READ case: along an access path rooted at a READ parameter's binding, writes through a `^T` reached by that path are forbidden. The pointer-write operation itself is unchanged; what changes is which pointers admit it. *[CP013 §3.4; superseding CP001 §2.6 along READ-rooted paths only. Bridge note: the op-sem reference §5.2 records the same supersession on the operational side.]*

### 3.2 Command-Typed Values

Commands are first-class values. The type forms `:<...>`, `?<...>`, and `!<...>` denote command-typed values of the corresponding failure mark, with the parameter-type list inside the angle brackets. *[CP001 §3.4; failure-system §1 for the marks.]*

Examples:

- `:<Int32, Int32>` — a never-fails command taking two `Int32` arguments.
- `?<Int32', String>` — a may-fail command taking a productive `Int32` parameter and an `Int32`-shaped (READ) `String` parameter.
- `!<Int32&>` — a must-fail command taking a reference-mode `Int32` parameter.

The angle-bracketed list is a **`TYPE_CMDEXPR_ARG`** sequence — parameter types only, no parameter names, since at the type level there is nothing to refer to a name as. The mode markers in this list are suffix-on-type:

- `Type` — READ (no marker).
- `Type'` — PRODUCE.
- `Type&` — REFERENCE.

This is the nameless-context placement; the named-context placement is identifier-shape (§4.1 below). *[CP005 §1.4 (productive suffix-on-type); CP008 §9.3 (reference suffix-on-type, resolving OQ-19).]*

Command-typed values support every operation a value supports: binding to slots, passing as parameters, storing in fields (within objects, since command-typed values are non-buffer), capture by lambdas and fexprs, partial application (§3.2.1 below), and direct invocation. *[CP001 §3.4]*

The **scope operator** `::` produces a command-typed value with the receiver(s) baked in when used in the class-resolution role; its full mechanism is operational and is treated in op-sem §5.6, with the type-side observation being that `(receiver :: name)` has the type the class declared for `name` minus the receiver position. *[CP001 §4.5; op-sem §5.6.]*

The scope operator's name reflects that `::` serves multiple roles in the language — class-method resolution on a receiver (the case named here), field member access on aggregates, namespace and module resolution, and the partial-application `::`-bake-in pattern. The single token operates uniformly as a *scope-into-a-namespace* operation across these roles, with the surrounding context (left-hand operand's type, identifier on the right) determining which role applies. *[Reconciliation: this naming is settled in the user's 2026-05-02 review; the operator's name was implicit but unwritten in earlier checkpoints. The name "scope operator" is canonical going forward.]*

**3.2.1 Partial application (type-side).** Resolving a class command on a receiver — `(receiver :: name)` — yields a command-typed value with the receiver baked in. Partial application beyond this case (partially applying a v-command on a subset of its receivers, or partially applying a regular command on a subset of its non-receiver parameters) is intended to use the same conceptual mechanism but the design has not been worked out at the type level. *Open question: OQ-6.* See §7. *[CP001 §3.4, OQ-6; class-system reference owns the full design.]*

The type-side concern with partial application is what the resulting command-typed value's type *is* — specifically, which parameter positions are elided in the resulting type and which mode markers travel with the elided positions versus remaining at the surface. This is recorded as the type-side aspect of OQ-6 to be addressed when the class-system reference takes up partial application in full.

### 3.3 Objects

An object is a stack-or-heap-allocated, identity-bearing aggregate. Its fields may be discontiguously laid out. Object-typed values are reference-semantics in the sense that they are normally manipulated through fat pointers to the object rather than as bytes. Object fields can be arbitrary types — buffer-backed *or* non-buffer — including pointers, command-typed values, other objects, and variants. *[CP001 §2.4]*

The object/record split is the core split of the buffer-backed/non-buffer division (§1.1, §2.5). Records *cannot* contain pointers, command-typed values, objects, or variants; objects can contain anything. Records have no identity beyond byte-content; objects have identity. The two are not a graded distinction — they are categorically separated, and the choice between them is the choice between byte-aggregate semantics and identity-bearing-aggregate semantics.

**Object lifetime ceiling.** The frame in which an object's storage is introduced is the object's lifetime ceiling: no mechanism in the language allows an object to outlive the frame that introduced it, except via transitive containment in another object whose ceiling is already higher. *[CP002 §3.1; op-sem §6.5 in full.]* The type-side rule recorded here is that an object's *type* does not entail its lifetime — object types are first-class types, but every object *value* is bound to an introducing frame. This distinction is what makes object-value-flow tractable in the no-return-values world: a value flows by being initialized into a slot the consumer already owns, never by being "returned upward."

The frame-ownership lens makes the rule concrete. An object lives in a frame's storage; a `^Object` parameter passed downward gives a callee access to it, but the callee does not become the owner. A writeable `^Object` parameter lets the callee swap which object a caller-owned slot points at, but the new object is allocated into the *caller's* frame on successful copy-restore (per CP002 §§3.3–3.4 / op-sem §6.5). The lifetime ceiling is whichever frame ends up holding the slot at the binding moment. *[Reconciliation: the frame-ownership lens articulation here is post-CP002 in vocabulary; the underlying mechanism is CP002 §§3.3–3.4 in full.]*

**Object types and class dispatch.** An object type may have class instances declared for it; the scope operator `::` (§3.2) works on object-typed values (or on `^Object` values) using the object's fat pointer's dispatch information. The full dispatch mechanism is in the class-system reference; the type-side observation is that an object's class membership is a property of the object's *type*, not of any particular object value, and is determined at the type-declaration site (or at instance-declaration sites for the relevant class). *[CP001 §4; class-system reference.]*

### 3.4 Variants (OQ-1, Non-Buffer Half: Resolved)

A **variant** is a non-buffer tagged sum. Its candidates may be arbitrary types (buffer-backed or non-buffer) — pointers, objects, command types, other variants, records, domains. The variant itself has reference semantics: variant values are normally manipulated through fat pointers to the variant's storage, parallel to objects. *[CP001 §2.5; CP006 §1.1.]*

The Record:Object :: Union:Variant parallel is exact at the buffer-backed/non-buffer axis: records and unions are buffer-backed; objects and variants are non-buffer. *[CP006 §4.1.]*

**OQ-1, variant case: resolved as 3-word slot — tag, candidate-pointer, class-witness.** The variant value is represented as a slot triple: *[Reconciliation: this resolution is new in this reference. The structure is structurally analogous to the failure-system slot (failure-system §5.1: tag + payload-pointer + class-witness), reflecting the user's preference (CP012 dialog) that subtype and sum typing play together cleanly. The 3-word choice is the (a) option from the post-CP014 OQ-1 dialog of 2026-05-02; the alternatives (b) pointee-header tag and (c) vtable-as-tag were considered and rejected — (b) loses the witness and forces an indirection per dispatch, (c) unifies tag and witness but loses the small-integer-tag handle that hierarchical comparison wants.]*

- **Tag identifier:** a small-integer identifier for the active candidate. Slot-sized (32 bits comfortable, with spare bits available for occupancy or other small flags). Identifies which candidate the variant is currently storing.
- **Candidate pointer:** pointer-sized; references the candidate value's storage. The storage is laid out per the candidate's type (a candidate that is a record carries that record's bytes; a candidate that is an object carries that object's identity-bearing storage; a candidate that is a pointer carries a pointer-shaped value at the indirection's other end; etc.).
- **Class witness:** pointer-sized; the typeclass dictionary for the active candidate's type with respect to the variant's "spans" class (the class that all candidates must satisfy if the variant declaration imposes one), or null when no class binding is in force.

The structural parallel with the failure-system slot is intentional: it lets the language reuse the same dispatch and propagation patterns across both surfaces. The class-witness component is what makes class dispatch on a variant's currently-active value coherent without the consumer knowing the concrete candidate type — the witness rides with the variant slot, and operations that need the candidate's class instance dispatch through it. *[Reconciliation: this is the variant-side symmetry with the typed-failure design from failure-system §§4.3, 5.1; recorded here as a deliberate language-design parallel rather than as duplication.]*

**Why this resolution.** The Expression Problem motivation is load-bearing: Basis aims to make subtype hierarchies (object-and-class) and sum types (variant) play together cleanly, which requires that sum-type dispatch be at least as expressive as subtype dispatch. The 3-word slot supports both at the variant surface: hierarchical tag comparison via the small-integer tag (parallel to the failure-system's hierarchy structure if the language adopts hierarchical variant tags later) and class dispatch via the witness. Neither expressive avenue is foreclosed.

**Variant declaration, surface form (sketch).** The grammar already admits a `DEF_INLINE_VARIANT` and (via `DEF_VARIANT`) named variant declarations. The full surface syntax for declaring a variant's candidate set, optional spanning-class, and any hierarchy structure is forwarded to the construction-and-initialization reference for treatment alongside variant construction and the dynamic-narrowing operator (§3.5). The type-side commitment here is the slot shape; the surface declaration form follows.

**Variant containment.** A variant value is non-buffer and may not appear in a buffer-backed position. It may appear in a top-level slot, in an object field, or as a candidate of another variant. *[CP006 §1.2.]*

**The absent state.** Every variant slot inherently admits a state in which *no candidate is currently present* — the **absent** state. A variant slot is therefore in one of *N+1* states, where *N* is the number of declared candidates: any of the *N* active-candidate states, or absent. The absent state is canonically represented by a null candidate-pointer in the 3-word slot (with the tag and witness words taking conforming values; the precise byte-pattern is implementation-decided but the user-visible semantics is "no candidate present"). *[Reconciliation: this is settled in the user's 2026-05-02 review. The absent state was implicit in the 3-word slot design but not articulated; it is now an explicit type-system property of every variant.]*

This refines CP006 §3.2 for variants specifically: variants now have a meaningful zero-default — the absent state — produced by zero-fill of the 3-word slot. The bare `# x : SomeVariant` introduction is well-formed and produces an absent-state variant slot. *[Reconciliation: CP006 §3.2 grouped variants with objects, pointers, and command-typed values as types whose bare `# x : T` is rejected for lack of zero-default. With the absent-state recognition, variants are *separated from this group*; bare `# x : SomeVariant` is now permitted. CP006 §3.2's framing remains valid for objects, pointers, and command-typed values.]*

The user-visible consequence: every analysis that consumes a variant must handle the absent case as a possibility, alongside whatever candidate-cases the variant declares. The pattern-matching surface (the form covering full case analysis on a variant; surface forms forwarded to the construction-and-initialization reference) requires absent-case coverage for exhaustiveness checking. The dynamic-narrowing operator `-<` (§3.5) produces a failure on absent-state input, distinguished by its own failure tag from the wrong-candidate-tag-mismatch case.

**Single-candidate variants are optionals.** A variant declaration with exactly one candidate, `variant V<T>` (surface syntax forwarded to the construction-and-initialization reference), produces an *optional `T`* — a slot that holds either a `T` value or the absent state. This subsumes what other languages call `T?`, `Option<T>`, `Maybe<T>`, or the nullable-pointer pattern. *[Reconciliation: this introduction is settled in the user's 2026-05-02 review.]*

The mechanism is intentional: it provides null-pointer-inclusive data structures without polluting the type system with a NULL type. There is no `null` value or null-typed slot; the absence is an inherent property of the variant slot itself, available uniformly across all candidate types (buffer-backed and non-buffer alike) by virtue of the 3-word slot design. A pointer-typed candidate's absent variant looks like a "null pointer" at the use site, but the type system never sees a null `^T` — it sees a variant in the absent state, which is a coherent typed value that pattern-matching and `-<` know how to consume.

**Variants are the only exception to "the type says what it contains."** For every type in Basis other than variant (and aliases denoting a variant), a value of that type contains what the type declaration says: an `Int32` slot holds an `Int32`; a record-typed slot holds bytes interpreted as that record; an object-typed slot holds a fat pointer to an object of that type; a pointer-typed slot holds a pointer to a `T`. None of these has an "absent" sub-state at the type level. Variants alone admit the absent state as part of their semantics, as a deliberate language-design choice that gives optionality first-class type-system support without compromising the cleanness of every other type. *[Reconciliation: this is the user's 2026-05-02 framing of why variants are special on this dimension.]*

The corollary for unions: unions cannot have an absent state, because for a buffer-backed type whose storage is `max(candidate-sizes)` bytes always reserved in the containing record, there is never "nothing there" — the bytes are always *some* interpretation. Union-discrimination is which-candidate, never whether-present. (A user may simulate optionality by adding an explicit "none" candidate domain to the union, paired with the user-managed enumeration; this is a user-level pattern, not a language-level construct.) *[Reconciliation: the user's 2026-05-02 framing of why optional-of-union is not a language-level concept.]*

### 3.5 The `-<` Dynamic-Narrowing Operator (OQ-15: Resolved)

The dynamic-narrowing operator `-<` performs a runtime type check and, on success, binds the narrowed value to the lhs slot. On mismatch, it produces a propagating failure — the same surface-form as the rest of the failure system. *[Reconciliation: this operator is new in this reference, resolving OQ-15. The user (post-CP014 dialog of 2026-05-02) directed that downcasting be an orthogonal alternative to a dedicated intrinsic, modeled on the `<-` polymorphic-RHS family. The `-<` form is parallel to `<-` syntactically and contrasts on the static-vs-dynamic axis. CP005 §3 originally registered the downcast intrinsic as a future thread (OQ-15); this resolution closes OQ-15 in favor of the operator approach.]*

Surface form (sketch; full surface forwarded to construction-and-initialization reference):

```
'narrow <- ‹rhs producing a wider value›
?- ‹narrow_target› -< ‹narrow›
    ; failure path: type-check failed
```

Read: produce the wider value, then attempt to narrow into a target slot whose declared type is a more-specific candidate or descendant. The narrowing is a `?`-call producing a propagating failure on type-mismatch.

The operator's applicability:


- **Object class hierarchies.** `'narrow -< ‹object_or_object_pointer›` checks whether the runtime type of the object (read from the fat pointer's RTTI) is at-or-below the `narrow` slot's declared object type. On match, the narrowed pointer is bound; on mismatch, a propagating failure is produced.
- **Pointer-to-non-buffer.** `'narrow -< ‹^T›` for pointer types whose `T` admits class-hierarchy narrowing follows the object rule.
- **Unions.** `-<` does *not* apply to union slots. Union narrowing is interpretive casting (§2.6), which is not failable in the type-system sense — the user has asserted, via the surrounding discrimination machinery (typically an enum-and-match), that the cast is correct. The two surfaces are deliberately different: variants and object hierarchies have language-tracked discrimination, so `-<` can do a runtime check; unions have user-tracked discrimination, so the cast is a user-asserted reinterpretation. *[Reconciliation: this scope decision follows from the OQ-1 split (§2.6 / §3.4): unions are *just bytes* with user discrimination, so a language-level dynamic check has nothing to check; variants carry a tag, so a check is meaningful.]*

The operator's failure-set is "tag-mismatch" — a single failure tag the language emits for `-<` mismatches, integrating with typed failures (failure-system §4) per the standard machinery. The exact tag identity is forwarded to standard-library design.

**The orthogonality property.** The `-<` operator does not require a downcast intrinsic, a downcast keyword, or any new compiler-emitted accessor commands. It uses existing failure-handling machinery (`?-`, `|`-with-spec, the propagating-set surface) and existing slot-binding semantics (parallel to `<-`). The only new mechanism is the operator itself, which is a small surface addition. The standard-library is not asked to provide downcast helpers; user code is not asked to wrap variant-narrowing in custom predicates. The orthogonal alternative the user requested (and the design principle of keeping language and standard-library orthogonal) is preserved. *[Reconciliation: this is the user's design preference per the 2026-05-02 dialog, recorded here as the design rationale for `-<`.]*

The grammar update needed for `-<` is small: an additional infix operator parallel to `<-` in the construction-and-initialization grammar. The operational semantics is forwarded to the construction-and-initialization reference; the type-system commitment recorded here is the operator's role and applicability.

*Sources for §3: CP001 §§2.4, 2.5, 2.6, 3.4, 4; CP002 §3.1 (object lifetime ceiling); CP006 §§1.1, 1.2, 4.1; OQ-1 (variant case) and OQ-15 resolved here; OQ-5(c) and OQ-6 forwarded to class-system reference.*

---

## 4. The Parameter-Mode System

### 4.1 The Three Modes

A parameter or receiver in Basis carries one of three modes, which together determine the contract between caller and callee at that position. *[CP013 §1.1, §4.2; renamed from CP004's IN/productive/reference vocabulary.]*

**READ.** The bare-name form, no marker. The callee may read through any storage path reachable from the parameter; it may not write through any such path (the *transitive* read-only contract — §4.6 below). The caller's slot is unchanged after the call regardless of outcome. *[CP013 §1.2; supersedes CP004's IN-mode "callee-copy isolation only" framing.]*

**PRODUCE.** Marked with `'`. The callee is statically obligated to write the parameter's slot exactly once on every successful return path — the **write-once** rule. The caller may pass either an initialized or an uninitialized slot; the callee will produce the value. Per copy-restore (§4.4), the produced value is committed to the caller's slot on success and not at all on failure. *[CP004 §1; CP009 §2 (write-once sharpening); CP013 §1.3 (rename).]*

**REFERENCE.** Marked with `&`. The callee may read, may write, or may do neither — no obligation in either direction. The caller must pass an already-initialized slot, since the callee may read and reading uninitialized is forbidden. *[CP004 §1; CP013 §1.4 (rename).]*

The three modes are *roles*, not *directions*. Each describes what the callee is permitted or obligated to do; the caller's side is derivable from the callee's contract. The names READ / PRODUCE / REFERENCE were chosen to make the permissions/obligations character explicit, retiring the calling-convention vocabulary that the prior IN/productive/reference names implied. *[CP013 §4.1.]*

### 4.2 Marker Syntax

The markers `'` and `&` are **part of the identifier itself**, not separate marker tokens. The identifier `'r` is a different identifier from `r`; the identifier `&x` is a different identifier from `x`. Every read and every write in the command body uses the marked form, so the marker is visible at every use site, not only at the declaration. *[CP005 §1; OQ-11 resolved.]*

The placement convention varies by context, depending on whether names are available to carry the marker:

**Named contexts** (parameter declarations, receivers, lambda invoke-method parameters, capture-list entries — places where the parameter/binding has a name): markers travel with names, identifier-shape.

- `'name` — PRODUCE.
- `&name` — REFERENCE.
- `name` — READ (bare).

The full type-bearing form is `Type 'name` or `Type &name` or `Type name`, with the type preceding the (marked) name.

**Nameless contexts** (command-type-expressions `:<...>`, `?<...>`, `!<...>`, where parameter types are listed without names): markers attach to the type as suffix, leaving the type-prefix position free for the pointer marker `^`.

- `Type'` — PRODUCE.
- `Type&` — REFERENCE. *[CP008 §9.3; OQ-19 resolved.]*
- `Type` — READ (bare).

Pointer-to-`Int32` as a reference parameter is `^Int32&`, with `^` as prefix and `&` as suffix on opposite sides — visually distinct, no blob. *[CP008 §9.3.]*

The two placements agree on what the markers mean (PRODUCE-writeable, REFERENCE-writeable) but differ on placement to suit the surrounding syntax. *[CP005 §1.4.]*

**Capture-list constraint.** In capture-list positions of lambda and fexpr forms (`:<args / captures>{body}`), only READ and REFERENCE modes are admitted; PRODUCE is forbidden. Capturing a productive obligation across a closure boundary is not meaningful — the obligation is to write at a specific call-site, not to defer indefinitely. *[CP008 §4.4; §9.2.]*

### 4.3 The Same-Scope Rule (OQ-14: Resolved)

Within any scope, **two-or-more identifiers that differ only by mode-character marking must not coexist.** The rule is symmetric across all three pairs: `x` and `'x` are mutually exclusive; `x` and `&x` are mutually exclusive; `'x` and `&x` are mutually exclusive. The rule applies both to identifiers introduced into a scope by binding (parameters, receivers, captures, local slots) *and* to command-parameter-list declarations: a command may not be defined with two parameters whose names collide modulo marker. *[CP005 §1.2 for the original `'x`/`x` rule; OQ-14 resolved here, broadened to all three pairs and to declarations as well as bindings. Reconciliation: CP005 §1.2 stated the `'x`/`x` rule and flagged the symmetric extension as OQ-14; the user (post-CP014 dialog of 2026-05-02) confirmed the symmetric extension and added the declaration-side rule explicitly. The resolution recorded here is broader than the literal CP005 §1.2 statement.]*

The motivation is reader confidence. Two textually-similar names referring to genuinely different slots in the same context produce a class of subtle reader-bugs ("did this read from the productive or the reference?"). Forbidding the coexistence costs a rename; the benefit is that scope-local reasoning never has to disambiguate near-identical names.

**A separate concern: passing a single slot at multiple modes in one call.** A caller passing a value `x` into a callee may legitimately pass `x` at one parameter position and `'x` (or `&x`) at another in the same call. The same-scope rule does not forbid this — the call-site argument list is not a scope, and the modes refer to roles at the call boundary, not to identifiers in the caller's scope. The aliasing concern this raises (the same caller-slot bound under two mode contracts simultaneously) is an aliasing question, not a same-scope question, and the typechecker handles it by the standard aliasing rules (specifically, the rule that disallows binding a single slot at PRODUCE and REFERENCE positions in the same call, since the resulting aliasing would defeat copy-restore failure-atomicity). *[Reconciliation: the same-scope-vs-same-call distinction is the user's clarification post-CP014; it sharpens the OQ-14 framing. The aliasing-of-a-single-slot question is orthogonal to OQ-14 and is the typechecker's existing concern.]*

### 4.4 Call-Boundary Mechanics: Copy-By-Value and Copy-Restore

The operational mechanics of parameter passing are detailed in op-sem §5.2; the type-side rules recorded here are the type-level commitments that those mechanics realize. *[op-sem §5.2; CP001 §3.2; CP004 §1.]*

**READ parameters** are passed by value at the observable level. The callee receives a copy. The caller's slot is bit-identical to its pre-call state after the call (success or failure). Implementation latitude: the implementation may pass READ parameters by reference under the hood, since the transitive READ contract (§4.6) precludes the writes that would make by-reference observable; the language commits to by-value at the *observable* level only. *[CP013 §7.3 (legacy OQ-2(a) largely settled); op-sem §5.2.]*

**PRODUCE and REFERENCE parameters** are passed by **call-by-copy-restore**: the callee receives a copy of the caller's slot value, operates on the copy, and on *successful* return the copy is written back to the caller's slot. On *failure*, no write-back occurs; the caller's slot is bit-identical to its pre-call state. *[CP001 §3.2; CP004 §1.]*

The copy-restore mechanism is the operational realization of the language's "mutation either succeeds fully or fails fully" principle. Failure-atomicity falls out for free, with no transactional machinery and no rollback code in user programs. *[CP001 §1.]*

**The pointer case composes cleanly.** For a parameter of pointer type `^T`, the *pointer value* itself is what's copied (READ) or copy-restored (PRODUCE / REFERENCE), not the pointee. A writeable `^T` (i.e., `^T'` or `^T&`) lets the callee swap which `T` the caller's slot points at; a non-writeable `^T` does not. The pointee's storage is reached *through* the pointer; reads and writes through the pointer touch storage that is, by the no-non-local-state principle's provision-chain rule, reached by explicit provision. *[CP001 §2.6; CP013 §3.4 for the READ-rooted-path refinement.]*

### 4.5 Parameter-Mode Invariance Under Mark Subsumption

The failure-mode marks (`:` / `?` / `!`) form a small partial order with `:` and `!` as specializations of `?` (failure-system §1.2). Parameter-mode markers are **invariant** under this subsumption: a `:<Int32'>` value is not interchangeable with `:<Int32>` or `:<Int32&>`; a `?<Int32'>` value is not interchangeable with `?<Int32>` or `?<Int32&>`. The subsumption relation is solely on the failure mark. *[CP011 §5.2; failure-system §1.3.]*

The invariance is essential for soundness: a productive parameter discharges a productive obligation and a reference parameter requires its slot initialized at the call site; substituting one mode for another would break the per-mode static rules even if the underlying type and failure-mode were compatible. The type-side rule recorded here is that `Type X` and `Type Y` (for distinct mode markings X, Y) are distinct types that do not stand in any subsumption relation, on either side of the failure-mode-mark axis.

This is a type-system rule that has its enforcement in the failure-mode-conformance and parameter-mode static analyses; both are CFG-walking analyses jointly maintained per op-sem §5.2 and failure-system §3. The type-system records the invariance; the analyses enforce it.

### 4.6 The Transitive READ Contract

A READ parameter introduces a contract: the callee may not write through any access path **reached from** the parameter. The path is rooted at the parameter's binding and extends through dereference, field access, indexing, and other operations that yield further slot-views; every slot-view on that path inherits the no-write-through restriction. *[CP013 §3.1; corrected here per the user's 2026-05-02 review: the contract is on the access path, not on the storage.]*

**The taint is on access paths, not on storage.** This distinction is load-bearing. A storage location may be reached at runtime by multiple access paths simultaneously — for example, a callee may receive the same underlying object both through a READ parameter and through a separate REFERENCE parameter, with no way for the callee to know they alias. Writes through the READ-rooted path are forbidden; writes through the REFERENCE-rooted path are permitted. The same storage, two access paths, two distinct write-permissions. The language does not attempt to detect or forbid this aliasing — it would be unsound to do so given that aliasing is a runtime property the type system cannot generally detect. The contract is what the callee promises *not to do via the READ-rooted path*; whatever the callee does via other paths is governed by those paths' own permissions. *[Reconciliation: the access-path framing was the agreed model in the 2026-05-02 dialog. CP013 §3.1 wrote "reachable from the parameter" with storage-reachability connotations; this reference corrects to access-path rooting throughout.]*

The contract is a callee-side promise verified at type-checking. A callee whose body would write through a READ-rooted path fails to type-check. The supersession of CP001 §2.6 along READ-rooted paths is recorded in §3.1 above and in op-sem §5.2's bridge note: writes through a `^T` reached from a READ-rooted access path are forbidden, even though writes through the same `^T` reached via a writeable-rooted path remain valid. *[CP013 §3.4.]*

**Operational form: access-path taint flow.** The rule is enforced via a flow-sensitive *taint* property on **access paths** (slot-views), walking jointly with the failure-mode analysis (failure-system §3) and the initialization analysis (CP004 §3 / op-sem §5.2). *[CP013 §3.2; corrected per the access-path framing.]*

- A READ parameter introduces a tainted access path at the callee's frame entry: the parameter's name, as an access path, is tainted.
- Operations that derive new access paths from a tainted access path — dereference of a tainted pointer, field access on a tainted aggregate, indexing into a tainted buffer — yield tainted access paths. The slot-view *is* an access path rooted at the tainted source.
- An attempt to write through a tainted access path (assignment to a tainted slot-view; pointer-write through a tainted `^T`-typed path) is a compile-time error.
- A buffer-backed value read through a tainted access path and copied (via `<-`) into a wholly separate slot produces an untainted destination. The buffer-backed containment rule (§1.2) makes this trivially safe: a buffer-backed value is pure bytes with no reference fields; the bytes were copied; the destination's access path is fresh and reaches only its own storage.

The access-path-taint model is **storage isn't tainted; access paths are**. Within a single frame's analysis, the rule above is operational; precise treatment of every type-former is the typechecker-implementation thread's. *[Reconciliation: an earlier draft of this section logged this as "OQ-2.2" with worked-out cases involving records-with-pointer-fields and similar. The user's 2026-05-02 review corrected the framing: records cannot contain pointer types (§1.2), so the entire "buffer-backed value with embedded references" case does not exist. The remaining within-frame implementation details belong to the typechecker thread, not to a logged OQ.]*

**Frame-locality of the analysis.** The transitive READ contract is purely a property of a single frame's analysis on its own parameter modes. Each command body has its own access-path-taint analysis, rooted at its own parameter declarations. **Read-only taint does not cross frame boundaries.** A PRODUCE or REFERENCE output of a downstream call is, in the caller's frame, a freshly-bound local; its access path is rooted at the caller's local, not at any of the caller's parameters. Whether the caller may write through that local is governed by the caller's own analysis — which sees a fresh local with whatever permissions the caller's own bindings give it. *[Reconciliation: this is the standing frame-ownership principle from the prelude, applied to the READ contract. CP013 §5.4 took an over-strict line that suggested cross-frame propagation; the bridge note in op-sem §5.2 corrected this to "aliasing materializes in the caller's frame and is the caller's responsibility" — i.e., not language-enforced. This reference aligns with the bridge note's framing.]*

**Worked example.** A callee with signature `getField: READ o: ^MyObj, 'fp: ^FieldT` is well-formed: the callee writes a pointer-into-`o`'s-field into `'fp`, which copy-restore lifts to the caller. The callee's body satisfies the READ contract on `o` (no writes through paths rooted at `o`). The caller, after the call, holds a fresh local of type `^FieldT`; that local's access path is rooted at the local itself. Whatever the caller does with that local is governed by the caller's own analysis. The example contradicts CP013 §5.4 as written, in favor of the bridge note's framing.

**Composition with lambdas and partial application.** Each lambda or fexpr body is its own command body, with its own access-path analysis. The taint of bindings inside the body — invoke-method parameters and captures alike — is determined entirely by **the binding's own mode marker**, per §4.1: a READ-marked binding (no marker, the default) is tainted; a REFERENCE `&` binding is not; a PRODUCE `'` binding is not. There is no "inherit-the-taint-of-the-captured-value-at-the-enclosing-scope" mechanism. *[Reconciliation: this revises an earlier draft of this section. The user's 2026-05-02 review clarified that taint inside a lambda body is controlled by the parameter/capture's own mode marker, not by the enclosing-scope taint of the captured value. This subsection is rewritten accordingly.]*

The same rule applies to partial application. The bound positions of a partially-applied command carry whatever mode markers the underlying command declared for them; inside the resulting command's body, those bindings are tainted-or-not per their declared modes. The enclosing-scope taint of the values bound at the partial-application site does not flow inward.

What the enclosing-scope taint *does* govern is **capture-site validity**: a capture's mode marker may not promote a value's permissions beyond what the enclosing scope's analysis allowed. Capturing an enclosing-READ-rooted name as `&`-REFERENCE is forbidden, because the resulting binding inside the lambda would permit writes that the original READ contract precluded. Capturing the same name without a marker (READ inside the lambda, READ in the enclosing scope) is fine; capturing with `&` when the enclosing-scope binding is itself REFERENCE or PRODUCE is also fine. Capture-site validity is the access-path-taint analysis's gatekeeper at lambda-construction time; *inside the body*, taint is purely a function of the body's own bindings' modes. *[CP008 §4.4 for the original capture-mode constraints; rule restated here per the access-path-taint model and the user's 2026-05-02 clarification.]*

### 4.7 V-Command Receivers: The R1+R2 Rules

A v-command takes a tuple of receivers: `(r1, r2, ...) :: cmd-name : args`. Each receiver carries an explicit mode marker. There are no implicit defaults: every receiver in every signature shape carries a marker, even where the language permits only a single mode at that shape. *[CP005 §2.]*

The rules governing v-command receiver modes are R1 and R2:

**R1 (call-site obligation).** At a v-command call site, every receiver must be initialized — independent of the receiver's mode. Dispatch fundamentally requires the receiver to exist as a real value at runtime: the dispatch mechanism resolves a method-bearing value from the receiver's type and invokes it on the receiver's value. Dispatching on an uninitialized slot is meaningless. *[CP005 §2.1.]*

**R2 (callee-body obligation).** A v-command's body, with respect to a receiver of mode `M`, has the same callee-side obligations as a parameter of mode `M` would, per §4.1:

- **PRODUCE `'`** — must write on every successful return path.
- **REFERENCE `&`** — no write obligation; may read, may write, may do neither.
- **READ** (no marker) — may read through any access path rooted at the receiver; may not write through any such access path (the transitive READ contract of §4.6). *[CP005 §2.1.]*

R1 lifts the caller-side obligation uniformly across all receiver modes (always initialized at the call site). R2 keeps the callee-side variation that the writeability marker is for. The two rules together say: dispatch always operates on a real receiver, and the marker tells the callee what it commits to doing with that receiver.

### 4.8 Receiver Modes by Signature Shape

Each signature shape restricts the set of valid receiver modes to those that are semantically meaningful for that shape. *[CP005 §2.5; OQ-12 resolved.]*

| Signature shape | Valid receiver modes | Forbidden modes |
| --- | --- | --- |
| Constructor (`DEF_CMD_CTOR`) | PRODUCE `'` only | READ, REFERENCE |
| V-command (`DEF_CMD_VCOMMAND`) | PRODUCE `'`, REFERENCE `&`, READ | none |
| At-stack `@` (`DEF_CMD_RECEIVER_ATSTACK`) | REFERENCE `&`, READ | PRODUCE |
| At-stack `@!` (`DEF_CMD_RECEIVER_ATSTACK_FAIL`) | REFERENCE `&`, READ | PRODUCE |

The marker placement is identifier-shape per §4.2.

The reasoning behind the per-shape restrictions:

**Constructor receivers must be PRODUCE.** A constructor's job is to fill the receiver slot. A reference-mode constructor receiver would mean "construct drawing on the receiver's existing state" — which is in-place modification, not construction; the v-command-with-REFERENCE-receiver covers that case. A READ constructor receiver would mean "construct a thing the caller cannot observe" — forcibly producing an inaccessible object, which has no purpose in the language. *[CP005 §2.3.]*

**At-stack receivers cannot be PRODUCE.** At-stack methods run at frame exit on objects that exist; productive mode would mean "construct the object as part of cleanup," which is meaningless — the object's existence is the precondition for cleanup, not its outcome. READ at-stack receivers are useful for observation-only cleanup ("log this object's final state at frame exit"); REFERENCE at-stack receivers are the typical case for resource-managing types. *[CP005 §2.4.]*

**V-command receivers admit all three modes.** Each mode has a distinctive idiomatic use:

- **READ receiver: externalized effect.** The method operates *through* the receiver without modifying it. Classic case: logging — `logger :: log: message` writes a log entry; the logger's state is unchanged but the world (the log file, the stream) is changed. The receiver mediates an effect external to itself. The pattern preserves the no-non-local-state principle: the receiver is a parameter, so accessing its connection-state is local; the external effect happens through some lower-level command that itself takes the connection-state as a parameter. *[CP005 §2.2.]*

- **REFERENCE receiver: modify the receiver in place.** The method may read the receiver's current state and may modify it. State transitions on objects, in-place updates, the "modify if needed" idiom. *[CP005 §2.2.]*

- **PRODUCE receiver: re-initialize the receiver.** The method commits to writing the receiver on every successful return path. Combined with R1 (must be initialized pre-call), the receiver is overwritten on success — a "factory method" or "complete reset" operation that happens to dispatch on the receiver's existing type. Unusual but coherent. *[CP005 §2.2.]*

### 4.9 The `-> name` Result Designator (Type-Side Reference)

The `-> name` clause's full operational semantics are covered in op-sem §5.3 and finalized by CP014. The type-side rule recorded here is the well-formedness rule and the type the result has in expression position. *[CP001 §3.3; CP014 §2.]*

The `-> name` clause is well-formed when `name` is a parameter or receiver of the command. It is not required to be writeable. *[CP001 §3.3.]*

The type the expression evaluates to depends on the parameter's mode:

- **PRODUCE `'name`** — expression value's type is the parameter's declared type; the value is the post-write-back value. *[CP014 §2.5.]*
- **REFERENCE `&name`** — expression value's type is the parameter's declared type; the value is the post-call slot value. *[CP014 §2.4.]*
- **READ `name`** — expression value's type is the parameter's declared type; the value is the *initial* value the caller passed in (CP014 reading (a)). *[CP014 §2.3.]*

In all three cases, failure of the call propagates as expression failure, and the pre-call state of `name` in the caller's frame is preserved. *[CP014 §2.6; CP001 §3.2; CP004 §1.]*

The implicit form (no explicit `-> name`) — used when the command has exactly one writeable parameter — applies the same rule with the implicit selection: the writeable parameter's mode determines the result type per the table above. *[CP001 §3.3.]*

The READ-with-`-> name` case enables the predicate-with-passthrough idiom — a may-fail check whose success makes the input value available to the next stage in an expression chain (`(validate: x -> x)`). The chain's next stage receives `x` only if validation succeeded. *[CP014 §2.3.]*

*Sources for §4: CP001 §§3.1–3.5 (signature shape, parameter passing); CP004 §1 (mode mechanics, original); CP005 §§1, 2 (marker syntax, R1+R2, receiver mode tables; OQ-11/OQ-12 resolved); CP008 §9.3 (OQ-19 resolved, suffix `Type&`); CP009 §2 (write-once productive sharpening); CP011 §5.2 (parameter-mode invariance under mark subsumption, integrated by reference); CP013 §§1, 3, 4 (READ/PRODUCE/REFERENCE rename, transitive READ contract, access-path-taint framing); CP014 §2 (`-> name` unified meaning, OQ-3 resolved); op-sem §5.2 bridge note for the frame-locality reading.*

---

## 5. Type Relations and Identity

### 5.1 Nominal Typing

All nominally-declared types in Basis — domains, records, unions, objects, variants — are nominally typed: two declarations with structurally identical content produce two distinct types. Values of one are not interchangeable with values of the other on the basis of structural similarity alone. *[CP003 §1.]*

The nominal-typing rule applies uniformly across the buffer-backed and non-buffer halves:

- Two `record` declarations with the same field-types-and-offsets are distinct types.
- Two `domain` declarations with the same parent are distinct types.
- Two `union` declarations with the same candidate set are distinct types.
- Two `object` declarations with the same field structure are distinct types.
- Two `variant` declarations with the same candidate set are distinct types.

Inline forms (anonymous declarations within a field or candidate position; §2.7) produce nominally distinct types per declaration site, parallel to named forms.

The single exception is **aliases** (§2.3), which erase: an alias declaration introduces no new type identity, and the alias name and the right-hand side are interchangeable in both directions in all contexts. The alias is a synonym, not a refinement.

The nominal-typing rule and the alias erasure rule together are what give the language its abstraction story. A module exposes named types; downstream code uses them by name; structural similarity to other types does not silently compromise the abstraction. The alias mechanism is the single carve-out, and its erasure is its purpose.

### 5.2 The Buffer-Backed Hierarchy and Implicit Upcasting

The single language-level subsumption relation between distinct types is the **buffer-backed parent-to-child upcast**: a value of a buffer-backed type is implicitly accepted wherever its parent (or any transitive ancestor) is expected; the converse is not implicit. *[CP001 §2.2; CP006 §1.4 unifying records/unions/named-domains under the buffer-backed umbrella.]*

**The hierarchy spans all buffer-backed types uniformly.** Per CP006 §1.4, records and named-domain declarations are surface forms of the same underlying concept — value-like nominal types over a buffer. Unions extend the same pattern with byte-level overlay structure. The buffer-backed hierarchy therefore includes all of them: every record, every named domain, every union, and the bracket forms `[N]`, `[N]T`, `[]`, `[]T`. Each buffer-backed type has a parent that is itself buffer-backed (recursively), and the parent chain ultimately reaches the bracket forms `[N]` or `[]` at the root. *[Reconciliation: this generalization of "domain-parent-to-child upcast" to all buffer-backed types was implicit in CP006 §1.4 but not surfaced in the subsumption discussion; the user's 2026-05-02 review made it explicit. Without this rule, a record could not be passed where a `[]` is expected, which would render the bracket-form universality unusable.]*

A record `Point` declared with two `Float32` fields is a buffer-backed type whose parent is the appropriately-sized bracket form (e.g., `[8]` modulo padding); the chain continues `[8]` → `[]`. A `Point` value is therefore implicitly acceptable where a `[8]` is expected, and where a `[]` is expected. The same applies to unions, to named domains, and to typed-view buffers `[N]T` (which subsume to `[]T` and to the appropriately-sized untyped bracket forms; precise lattice details are minor and forwarded with §7.3).

**The upcast is type-acceptance, not value-rewriting.** The runtime value's nominal identity is preserved across an upcast — what changes is which type the static analysis is willing to accept the value as. This is what permits class dispatch on a buffer-backed type to resolve to the type's instance even when the value has been passed through a parent-typed parameter: the runtime value is still a `Point`, even when held in a `[]`-typed slot. *[CP001 §2.2; OQ-5(c) for the subtle dispatch-after-upcast question; class-system reference owns the full treatment.]*

**Sibling buffer-backed types do not implicitly convert.** Two distinct buffer-backed types that share an ancestor but where neither is the other's ancestor are siblings; they subsume separately to the shared ancestor but do not implicitly convert peer-to-peer. The classic case is `Inches` and `Centimeters` both parented at `Int32`: incomparable for implicit conversion despite shared ancestry. The same rule covers two distinct records over `[8]` (sibling records, sharing the `[8]` ancestor), two distinct unions over the same byte-width (sibling unions), and any other two distinct buffer-backed types at the same level of the hierarchy. An explicit conversion — a constructor invocation, an interpretive cast — is required to move between siblings. *[Reconciliation: extending the sibling-domain rule to records and unions was implicit in CP006 §1.4 and made explicit in the 2026-05-02 review.]*

**The hierarchy is open at the child end.** A downstream module may declare a child of an imported buffer-backed type, extending the hierarchy. The implicit-upcast relation is structurally stable across this extension because the upcast is one-directional (child → parent) and child declarations don't widen the upcast set for any existing type. The cross-module openness is treated in §5.3 below.

### 5.3 Subsumption Limits Outside the Buffer-Backed Hierarchy

The buffer-backed parent-to-child upcast (§5.2) and alias erasure (§2.3) are the implicit-conversion relations the language admits. Outside these, no implicit conversion exists. In particular:

- **Sibling buffer-backed types do not subsume to each other.** Two distinct records over `[8]` subsume separately to `[8]` (and to `[]`), but do not implicitly convert peer-to-peer — they are siblings, analogous to `Inches` and `Centimeters` under `Int32`. The same holds for sibling unions, sibling named-domains, and any combination across the buffer-backed kinds. An explicit conversion (a constructor invocation; an interpretive cast where appropriate) is required to move between siblings.
- **Object-to-object structural similarity does not produce subsumption.** Objects are non-buffer; their identities are nominal; structural overlap of fields does not imply implicit-acceptance compatibility. Class membership *is* a relation that admits dispatch (the scope operator `::` against a class-declared method), but it is not a value-substitution relation: a parameter expecting `Foo` is not satisfied by a value of an unrelated object type that happens to have a `Foo`-like field shape.
- **Variants do not subsume by structure.** Variants are non-buffer; structural similarity of candidate sets does not produce subsumption between distinct variant declarations.
- **Pointers are invariant in their pointee type.** `^T` and `^U` are unrelated for distinct `T` and `U`, regardless of any buffer-backed subsumption between `T` and `U`. (The pointer's own type identity is fixed by the type it was declared as; no pointee-type-substitution is implicit.)
- **Command-typed values are invariant in parameter types and modes** (per §4.5). They admit subsumption only on the failure mark — the small partial order (`:`, `!`) ⊑ `?` from CP011 §5.2 / failure-system §1.3. Different parameter shapes or different mode markers produce distinct command-types that do not subsume.

The narrowness of the implicit-conversion story outside the buffer-backed hierarchy is a design commitment, not an oversight. Implicit conversions are a routine source of reasoning errors in languages that admit them; Basis admits the buffer-backed parent-to-child upcast (which is what makes the bracket-form universality usable — passing a record where a `[]` is expected is an everyday move that the language has to support) and otherwise forces every type-crossing to be explicit. The dynamic-narrowing operator `-<` (§3.5) handles the runtime-checked cases for variants and class hierarchies; constructors handle the rest. *[Reconciliation: this section was previously a flat "no other subsumption" enumeration; the user's 2026-05-02 review clarified that the right framing is "the buffer-backed hierarchy is uniform across records, unions, named domains; siblings within it do not convert; only the non-buffer side has no subsumption at all." Recast accordingly.]*

### 5.4 Buffer-Backed Hierarchy Extension and the Cross-Module Question

A downstream module may declare a child of an imported buffer-backed type — the buffer-backed hierarchy is open for extension at the child end, uniformly across records, unions, and named-domain declarations. This is symmetric to how class instances may be declared in modules other than the class's home module (subject to OQ-5(b)).

The cross-module openness has not been observed to cause the same kind of surprise that the closed-by-CP012 failure-tag hierarchy is designed to prevent. The reasons are structural:

- A handler that catches `|: SomeRootTag` widens silently when a downstream module adds new descendants of `SomeRootTag` (the failure-system concern, CP012 §2.4 / failure-system §4.11).
- A parameter declared at type `SomeParent` (a buffer-backed type) does *not* admit broader behavior just because `SomeChild` was added downstream; the parameter's static type is unchanged, and any caller passing a `SomeChild` value already had to declare or import that type.

The asymmetry is recorded here as the rationale for not extending CP012 §2.4's closure rule to the buffer-backed hierarchy. The buffer-backed hierarchy remains open for child extension; the failure-tag hierarchy is closed (subject to the open question OQ-28 in failure-system §7.6, with a specific Liskov-style opening direction proposed in OQ-29 below — §7.9). *[Reconciliation: this asymmetry analysis is post-CP012 and was implicit in the OQ-28 framing; recorded explicitly here for the type-side. The OQ-29 reference reflects the user's 2026-05-02 review proposing that the failure-tag hierarchy could be opened under a payload-covariance constraint preserving Liskov substitution.]*

### 5.5 Type Equivalence at the Slot Level

Two slots are of the same type iff they were declared with the same nominal type (modulo alias erasure). Two values are of the same type iff they were constructed at the same nominal type or transitively domain-narrowed to it via constructor or interpretive cast.

The slot-level equivalence is what the typechecker uses for binding compatibility, parameter-passing compatibility, copy-restore validity, and the rest of the static analysis. The CP004 / op-sem §5.2 / failure-system §3 / CP013 §3 analyses all operate over this slot-level type, augmented by the failure-mode mark, the initialization state, and the taint flag respectively. The state-vector at each program point is the product of these dimensions; the joins and transfer functions are component-wise, with cross-component interactions only at transfer functions (e.g., a successful `?`-call's failure-edge produces `failing(?)` *and* leaves PRODUCE-parameter init-state unchanged on that edge). *[Synthesis of CP004 §3, CP011 §7, CP013 §6.1.]*

*Sources for §5: CP003 §1 (nominal typing for buffer-backed types, generalized to all nominal types here); CP001 §§2.2, 2.3 (named-domain hierarchy and alias erasure); CP006 §1.4 (records-and-unions-and-domains unified under the buffer-backed umbrella, generalizing the hierarchy); CP013 §6 (composition with existing analyses); failure-system §3, op-sem §5.2 cross-references.*

---

## 6. The No-Defaults Discipline at the Type Level

### 6.1 The Discipline

Basis adopts a **no-defaults** model: there is no language-imposed default value for any type, and there is no convention by which a slot acquires a value implicitly. Every slot's value arrives via explicit construction at every use site. *[CP004 §3 (initialization analysis); CP006 §3 (the discipline as a type-level consequence).]*

The discipline has type-level consequences:

- A bare `# x : T` introduction produces an *uninitialized* slot, which the static analysis tracks (CP004 §3 / op-sem §5.2). Reading from an uninitialized slot is forbidden.
- A `# x : T <- ‹rhs›` introduction produces an initialized slot whose value is the result of `‹rhs›`'s evaluation.
- A `# x : T = ‹default›` declaration form (CP009 §5.1) is the explicit-default form for buffer-backed types whose every-byte-zero is not a meaningful or invariant-preserving value.

The third bullet is what closes the gap between zero-fill convenience and invariant safety. Buffer-backed types whose bytes have a meaningful zero (most numeric domains, plain-numeric records, etc.) admit a zero-default by composition. Buffer-backed types whose invariants forbid zero (e.g., a `domain Positive: Int32` whose semantic constraint excludes zero) declare an explicit default via `=`. Non-buffer types in the *no-zero-default* group — pointers, command-typed values, objects — cannot have zero-defaults because their representations include non-byte semantics with no meaningful zero state; bare `# x : T` is rejected at the type level for these.

**Variants are the exception in the non-buffer category.** The 3-word slot's zero-fill is the absent state (§3.4), which is a meaningful and well-defined state of the variant. Bare `# x : SomeVariant` is therefore *well-formed* and produces an absent-state variant slot. Subsequent uses of `x` must handle the absent case alongside any candidate cases. *[Reconciliation: this refines CP006 §3.2's grouping of variants with the no-zero-default types. Per §3.4, the absent state is the variant's natural zero-default. Pointers, command-typed values, and objects remain in the no-zero-default group; variants are separated.]*

### 6.2 R1 and R2 as Type-Level Consequences

The R1 and R2 rules from §4.7 are not isolated rules of the v-command surface; they are the v-command shape of a broader pair of disciplines that the no-defaults model imposes on every parameter and receiver position:

- **R1 (caller-side initialization for class dispatch).** Specific to v-command receivers, but symmetric to the broader rule that REFERENCE parameters require their slot initialized at the call site. The rationale is the same: an operation that needs to read uninitialized state is meaningless; the caller's obligation is to provide initialized state where reading is permitted.

- **R2 (callee-side mode obligation).** Generalizes to "every parameter and receiver of mode `M` has the callee-side obligation associated with `M`." For PRODUCE, write-once on every successful path (§4.1); for REFERENCE, no obligation; for READ, no write through any access path rooted at the parameter (§4.6).

The R-pair is the type-level shape of "no defaults — the caller initializes where reading is needed; the callee fulfills its declared role." Construction patterns (forwarded to the construction-and-initialization reference) realize the shape at every initialization site. *[Reconciliation: the framing of R1+R2 as the v-command-specific instance of a broader pattern is recorded here for the first time; CP005 §2 stated R1+R2 for v-commands without surfacing the generalization.]*

### 6.3 Composite Initializers and Atomic Construction

Whole-slot initialization tracking (CP006 §2 / OQ-9 resolved) requires that compound construction be atomic: a record, object, union, or variant slot is initialized in a single conceptual step, not field-by-field. The composite-initializer mechanism (OQ-10 in CP006 §3.3, fleshed out by CP009 §3) is the construction-side surface for this; the type-side commitment is that compound types do not admit per-field initialization tracking and therefore must be constructed-as-a-whole. *[CP006 §2; CP009 §3.]*

The type-side rule recorded here is that a compound type's well-formedness as a slot-typed entity requires the construction surface to support whole-slot initialization at every introduction site. The mechanism (Aggregate and Sequence literals from CP009 §3, the `=` declarative form from CP009 §5, constructor invocations) is forwarded to the construction-and-initialization reference. The type system's role is to verify that every reachable use of a compound slot is preceded by an atomic initialization on every path; the failure of such verification is a type error (per the joint init-and-failure analysis from op-sem §5.2 and failure-system §3).

### 6.4 The `-<` Operator at the Type Level

The dynamic-narrowing operator `-<` (§3.5) is an initialization operator at the type level: its lhs is a slot whose declared type is a more-specific candidate or descendant of the rhs's type, and on success the lhs is initialized at its declared type. On failure, no initialization occurs (failure-atomicity preserved); the lhs slot retains its pre-operation state.

The type-side commitments of `-<`:

- The lhs must be a slot of a type that is a candidate of the rhs's variant type, or a descendant in the rhs's class hierarchy (for object hierarchies).
- The rhs is consumed at its declared type; its slot is unaffected (the operator does not move out of the rhs).
- The success edge initializes the lhs; the failure edge produces a propagating failure with the tag-mismatch tag (§3.5).

The operator integrates with the joint init-and-failure analysis as a `?`-call that produces an init-transition on its success edge: the lhs becomes `init` on success, remains in its prior state on failure. *[Reconciliation: this integration with the static analyses is the type-side specification; the operational mechanics are forwarded to the construction-and-initialization reference, which will treat `-<` alongside `<-` and the rest of the construction surface.]*

*Sources for §6: CP004 §3 (the analysis); CP005 §2 (R1+R2); CP006 §§2, 3 (no-defaults discipline at the type level, OQ-9 resolved); CP009 §§3, 5 (Aggregate/Sequence literals, defaults declaration); §§3.5, 4.7 of this reference.*

---

## 7. Open Questions

This section catalogs the type-system-and-modes open questions. Resolved questions covered by this reference are noted at the section where they are settled (with cross-references here for completeness); genuinely open questions are described in full.

### 7.1 OQ-1 (Variant Half) — Resolved Here (§3.4)

The variant case of OQ-1 is resolved as a 3-word slot — tag, candidate-pointer, class-witness — structurally analogous to the failure-system slot (failure-system §5.1). The rationale is the Expression-Problem motivation: subtype hierarchies (object-and-class) and sum types (variant) play together cleanly when sum-type dispatch is at least as expressive as subtype dispatch. The 3-word slot supports both.

### 7.2 OQ-1 (Union Half) — Resolved Here (§2.6)

The union case of OQ-1 is resolved as untagged byte overlay. Unions are *just bytes*; the language carries no language-level discriminator inside the union slot. Discrimination is the user's responsibility, typically via an enumeration alongside the union in a containing record or via contextual information already implicit in the program. Reading a candidate value out of a union is interpretive casting (forwarded to the construction-and-initialization reference for surface form).

### 7.3 Record Padding and Layout — Open (Minor)

The surface specification does not currently pin down record padding rules. Implementation latitude on field offsets (alignment-based padding, packing modes, explicit `.packed` annotations) is one possible direction; a fully-specified deterministic layout is another. The choice affects ABI stability across compiler versions and the validity of programs that read record bytes through `[N]`-typed views. *[Reconciliation: this question is registered here as a minor open type-system question. CP001 §2.4 noted "fields laid out at deterministic offsets" without specifying the determinism mechanism; this reference flags the question without resolving it. Forwarded to the typechecker-implementation thread or to a future grammar-and-layout thread.]*

### 7.4 OQ-5 — Single-Class Instance Coherence — Forwarded

OQ-5 has four sub-questions (CP001 §6, OQ-5):

- (a) Are duplicate instances for the same `(class, type)` pair a static error globally, or are they ranked?
- (b) Is there an orphan-instance restriction limiting where instances can be declared?
- (c) Domain-specific dispatch — class dispatch resolves on the most specific known type at the call site; the user-intent commitment is that implicit upcast does not lose domain identity for dispatch purposes.
- (d) Cross-module overriding instances follow Julia's "more specialized module wins" pragmatics.

The type-side aspects of OQ-5(c) are recorded in §5.2 above (implicit upcast is type-acceptance, not value-rewriting; runtime value retains domain identity). The remaining sub-questions are class-system territory and are forwarded to the class-system reference. *[CP001 §6, OQ-5; type-side aspects partially covered here.]*

### 7.5 OQ-6 — Partial Application Beyond Receiver-Only — Forwarded

OQ-6 asks how partial application beyond receiver-elision (the `(receiver :: name)` case) should work. The type-side concern is what the resulting command-typed value's type *is* — which parameter positions are elided in the resulting type, and which mode markers and reachability properties travel with the elided positions. *[CP001 §6, OQ-6.]*

The full design — surface syntax, the resulting type's shape, interaction with mode markers, interaction with the transitive READ contract for values baked into the partial application — is forwarded to the class-system reference. The type-system records the question and the type-side concerns; the resolution requires class-system work to complete.

### 7.6 OQ-13 — Implicit Context Parameters and Initialization — Forwarded

The interaction between implicit context parameters (the Scala-implicit-style mechanism, op-sem §5.5) and the initialization analysis (CP004 §3 / op-sem §5.2). Specifically: when an implicit parameter is supplied automatically from the caller's lexical scope, what initialization-state checking applies, and how does the supplied value's mode and taint propagate? *[CP001 §6, OQ-13; op-sem §5.5 forwarded.]*

The type-side aspect: an implicit parameter is, structurally, a parameter; it carries a mode, a type, and a taint level. The implicit-resolution machinery selects a value from lexical scope and supplies it; the supplied value's properties propagate per the standard rules. The full mechanism — including the resolution rule, ambiguity handling, and the interaction with class instances — is forwarded to the construction-and-initialization reference (which owns context-parameter mechanics in detail) and to the class-system reference (which owns the parallel instance-resolution machinery).

### 7.7 OQ-15 — Resolved Here (§3.5)

OQ-15 (the downcast intrinsic) is resolved as the dynamic-narrowing operator `-<` (§3.5), which uses existing failure-handling machinery rather than introducing a new intrinsic. The standard library is not asked to provide downcast helpers; user code is not asked to wrap variant-narrowing in custom predicates. The orthogonality property — keeping language and standard library distinct — is preserved.

The remaining surface-syntax details (precedence, exact infix layout, integration with the broader `<-`-family of construction operators) are forwarded to the construction-and-initialization reference.

### 7.8 OQ-16 — Overloading Restriction on Dynamically-Dispatched Commands — Forwarded

OQ-16 concerns whether overloading is permitted on commands subject to class dispatch, and if so under what restrictions. The question is fundamentally a class-system question (how the class-method-by-name resolution composes with overloading); the type-side aspect is what types admit overloaded resolution. Forwarded to the class-system reference. *[CP001 §6, OQ-16.]*

### 7.9 OQ-29 — Liskov-Style Opening of the Failure-Tag Hierarchy — Forwarded

The §5.4 asymmetry argument (the buffer-backed hierarchy is open at the child end; the failure-tag hierarchy is closed under CP012 §2.4 / OQ-28 in failure-system §7.6) was framed as structurally motivated. A specific direction for opening the failure-tag hierarchy is recorded here: permit cross-module extension *under a Liskov substitution constraint* — wherever failure tag A is a subtype of tag B, any handling keyed at B must remain sound when invoked with an A-typed payload. *[Reconciliation: this OQ is registered per the user's 2026-05-02 review. It refines OQ-28 by proposing a specific controlled-extension condition rather than a flat closure-vs-openness choice.]*

The proposed concrete constraint: if A extends B, then A's payload type must not be wider than B's payload type — i.e., A's payload must be a subtype of B's payload (in the buffer-backed parent-to-child sense of §5.2), or the same type. Stated as covariance of payload with tag: as the tag narrows (B → A), the payload narrows too (or stays the same). A handler keyed at B receives B's payload type at the static interface; if invoked at runtime with an A-tagged failure (A <: B in the tag hierarchy), the payload is a value of A's payload type, which by assumption is acceptable through buffer-backed implicit upcast wherever B's payload type is expected. Liskov is preserved.

The design is forwarded to the class-system reference for substantive treatment alongside OQ-5(b) (orphan instances) and the related cross-module-coherence questions, since the structural similarity to instance-coherence concerns suggests they should be settled together.

Sub-questions for resolution:

- (a) Is the payload-covariance rule sufficient for soundness, or are additional conditions required (e.g., handler-side conformance checking, witness propagation, coherence-of-tag-extension across import paths)?
- (b) Does the rule extend cleanly to typed-failure recovery (`|: Tag` blocks; failure-system §4.10 narrowing), or does the recovery-narrowing's interaction with cross-module additions need its own analysis?
- (c) What declaration shape opts a tag hierarchy in to cross-module openness — explicit "open" annotation at the root tag's declaration, an entirely separate declaration form, or a default-open / default-closed choice (with the matching default for the buffer-backed hierarchy)?
- (d) How does the Liskov-style openness interact with the failure-tag class-witness mechanism (failure-system §5.1) — is a witness mechanism needed for the payload-covariance check at the cross-module boundary?

This OQ is concretely shaped as a refinement of OQ-28, not a replacement: failure-system §7.6's OQ-28 entry would be updated to point at this OQ once the design is settled. A future consolidated pass can collapse the two entries into one if the design lands cleanly.

### 7.10 OQ-22 — Parameterized Literal Types in `.implicit` — Forwarded

OQ-22 was provisionally forwarded to type-and-modes per the OQ catalog, but the substantive question (how `.implicit` constructors handle parameterized literal types) is construction-side: `.implicit` is the literal-type construction mechanism (CP009 §4), and the parameterization concerns the constructor's signature shape and the literal's matching rule. Forwarded onward to the construction-and-initialization reference. *[Reconciliation: the forwarding decision is post-CP014 dialog of 2026-05-02 and was confirmed in the user's reply: OQ-22 belongs to construction.]*

### 7.11 OQ-23 — Lexer Disambiguation Rules — Forwarded

OQ-23 collects small grammar-level disambiguation questions between `'`, `&`, and adjacent tokens. Tracked here for visibility but not type-system territory; the resolution will collect with other lexer/grammar implementation matters when that work is undertaken. *[Reconciliation: per the user's 2026-05-02 dialog, OQ-23 is implementation-thread material to be tracked so that all lexer/grammar work can be collected in one document and implemented together. No type-side resolution attempted here.]*

### 7.12 OQ-25 — Capture-Shadowing — Forwarded

OQ-25 concerns the rules for capture-shadowing in lambda and fexpr forms. Type-side aspect: a capture binds a name in the closure body that may shadow an outer name; the type and mode of the shadowed name and the captured name need not coincide, and the rules for resolution at use sites within the body need specification. The full treatment is owned by the lambda-and-fexpr reference. *[CP008 §IN-25; OQ-25 forwarded.]*

---

## 8. Provenance

**Authored:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, on 2026-05-02, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` and `reference-operational-semantics.md`. Scope and section structure were settled with the user in advance of drafting (the 2026-05-02 dialog), with five points pinned: the frame-ownership lens as standing principle; CP013 carve-in via the bridge note's reading; the OQ-1 split with (a)-shape variants and (α)-shape unions; the `-<` operator as the OQ-15 resolution; the broadened OQ-14 same-scope rule.

**Source materials:** intent-checkpoint-001.md (foundational type-system overview, command shape, classes and dispatch overview); intent-checkpoint-002.md (object lifetime ceiling cross-reference); intent-checkpoint-003.md (nominal typing); intent-checkpoint-004.md (parameter-mode mechanics, initialization analysis); intent-checkpoint-005.md (marker syntax, R1+R2 receiver rules; OQ-11 and OQ-12 resolutions; downcast as a named topic); intent-checkpoint-006.md (the buffer-backed principle made structural; OQ-9 resolution; records-are-domains unification); intent-checkpoint-008.md (marker placement OQ-19 resolved; capture-list mode constraints); intent-checkpoint-009.md (write-once productive sharpening; bare-identifier `<-` value-copy primitive; defaults declaration syntax); intent-checkpoint-013.md (READ/PRODUCE/REFERENCE rename; transitive READ contract; access-path-taint framing); intent-checkpoint-014.md (OQ-3 resolution as it bears on the type-side `-> name` interpretation). Secondary references consulted for cross-cutting context: `reference-failure-system.md` (mark-subsumption invariance integration; failure-system slot structure as analog for the variant slot); `reference-operational-semantics.md` (parameter-mode operational mechanics; bridge note alignment).

**Resolutions recorded in this reference:**

- OQ-1 (variant half) resolved as 3-word slot per §3.4.
- OQ-1 (union half) resolved as untagged byte overlay per §2.6.
- OQ-14 broadened and resolved per §4.3.
- OQ-15 resolved via the `-<` operator per §3.5.
- **OQ-2.1 closed without action.** CP013's framing of OQ-2.1 as "caller-side taint propagation across call boundaries" presupposed cross-frame taint flow. Under the standing frame-ownership principle (prelude) and the access-path-not-storage taint model (§4.6), read-only taint does not cross frame boundaries: each frame has its own access-path analysis, driven by its own parameter modes. There is no cross-frame question to answer. *[Reconciliation: this closure is settled in the user's 2026-05-02 review.]*
- **OQ-2.2 closed without action.** CP013's framing of OQ-2.2 as "value-disjoint vs. slot-view discrimination" implicitly assumed the existence of buffer-backed values containing reference fields whose copy-into-fresh-locals would raise the discrimination question. Under the buffer-backed containment rule (§1.2), no buffer-backed type may contain pointers, command-typed values, objects, or variants. The complicated case does not exist. The within-frame access-path-taint rule is operational at §4.6 (access paths derived via slot-views are tainted; buffer-backed bytes copied to fresh locals produce untainted destinations); precise treatment of remaining type-formers is the typechecker-implementation thread's, not a logged OQ. *[Reconciliation: this closure is settled in the user's 2026-05-02 review. The user observed that the cases I had previously listed for OQ-2.2 ("records with embedded pointer fields," "buffer indexing where the element type contains references") were against rules already stated in §1.2 of this very document.]*

**Forwarded sub-questions:**

- OQ-5 (instance coherence), OQ-6 (partial application), OQ-16 (overloading on dispatched commands), and **OQ-29 (Liskov-style opening of the failure-tag hierarchy under a payload-covariance constraint)** forwarded to the class-system reference. OQ-29 is registered new in this reference (per the 2026-05-02 review) as a refinement of OQ-28 (failure-system §7.6); see §7.9.
- OQ-13 (implicit context parameters and init) forwarded to construction and class-system jointly.
- OQ-22 (parameterized literal types in `.implicit`) forwarded to construction (per the 2026-05-02 dialog reassignment).
- OQ-23 (lexer disambiguation) forwarded to the implementation thread; this includes the surface placement of the `.inline` modifier on record fields (§2.4).
- OQ-25 (capture-shadowing) forwarded to the lambda-and-fexpr reference.
- Record padding and layout registered as a new minor open question (§7.3).

**New material recorded in this reference (no prior checkpoint coverage):**

- **Record-in-record composition and the `.inline` modifier (§2.4).** The user's 2026-05-02 review made explicit a set of rules previously summarized in a single sentence: nested record fields are accessed via the scope operator (`f::p::x`); the `.inline` modifier flattens an inner record's field names into the outer record's namespace as if declared directly; `.inline` retains the inner record's type-name handle (so the inner value remains passable as a record / buffer); naming collisions under `.inline` are a compile-time error, transitive across nested `.inline` levels. This is new authoritative content; no source checkpoint covered it.

**Reference-document impact and pending consolidations:**

- The operational-semantics reference §5.2 already carries a bridge note flagging CP013 supersession; the bridge note's "aliasing materializes in the caller's frame and is the caller's responsibility" framing is canonical and aligns with §4.6 of this reference. The fulcrum-at-frame-boundary phrasing in the bridge note is to be read as *the caller's own analysis of its own frame catches whatever it catches — the language does not propagate taint across frames*.
- The operational-semantics reference §5.3 (the `-> name` mechanism) and §8.2 (OQ-3) require revision to fold in CP014 in a future consolidated pass; this reference's §4.9 records the type-side rule for the same mechanism using the CP014 resolution from inception.
- The operational-semantics reference §8.3 (OQ-15) will require a touch-up note in a future consolidated pass to cross-reference the `-<` operator resolution recorded in §3.5 here.
- The failure-system reference §7.7 (OQ-1 in constrained form) framed the variant and union halves as remaining open; this reference resolves both, and a future consolidated pass on the failure-system reference may update §7.7 to point at §§3.4 and 2.6 here.
- The failure-system reference §7.6 (OQ-28 — closure of the failure-tag hierarchy) will require a touch-up note in a future consolidated pass to cross-reference OQ-29 (§7.9 here), which proposes a specific Liskov-style opening direction. The two OQs may collapse into a single entry once the design lands cleanly in the class-system reference.

**Recommended next step:** User review section-by-section, revisions in place. After this reference is settled, the next topic-organized reference per the agreed split is **Construction and Initialization** (#4), covering the `<-` polymorphic-RHS mechanism, the `-<` dynamic-narrowing surface, Aggregate and Sequence literals, the `.implicit` mechanism (with OQ-22), the `=` defaults declaration form, atomic compound construction, the OQ-10 composite-initializer mechanism, and the construction-side aspects of context parameters (OQ-13). Many type-side commitments recorded here will reach their full operational specification in that reference. The full sequence is in the failure-system reference's handoff §10.
