# Declaration Resolution Rework

Sep 29, 2026, updated Sep 30, 2026 · @David Bos

## Summary

Declaration resolution loses its separate type phase. Each declaration gets one SIR block that computes its typed value: the declared type (if any) is passed down as the type destination, and at each leaf of the value expression the value is determined and its type unified at the same time. The resolver runs that block and sets the declaration's value from the result. There is no stub state: function bodies are specialized after declarations, so a function's value already exists when its body (including a recursive call) refers to it. Residual (runtime) functions get a single compiler-wide home in `Compiler.functions`.

Sources: the description of commit `dfde357` ("WIP (see description)"), the code changes in `5f58ad4`, `dda8d75`, `dfde357`, `6dfa85f` and `c738f3e` on the `resolution-rework` branch, and design discussion on Sep 30.

## How resolution works on `main`

On `main`, the resolver drives each declaration through two SIR blocks and owns most of the bookkeeping.

- **Codegen** emits two `SIR_comptime_block`s per declaration: `block_type` (evaluates the declared type, if any) and `block_val` (evaluates the value, starting with `SIR_lookup_decl_type` on itself).
- **Resolver** (`resolve_declarations` in `compiler.c`) keeps a `resolve_stack` of `ResolveEntry`s, each with its own `RunState` and a `min_required_resolve_status`.
- **Statuses:** `unresolved` → `resolving_type` → `type_resolved` → `resolving_value` → `fully_resolved`, plus `error`.
- **Suspension:** `SIR_lookup_decl_type` suspends the running entry until the target is `type_resolved`; `SIR_lookup_decl_value` suspends until the target is at least a stub.
- **Stubs:** when a value lookup hits a declaration whose type is known but whose value isn't, the resolver allocates a `ValueDeclarationStub` and marks it `stub_value`. This is what lets recursive functions reference themselves.
- **Result extraction:** after each block finishes, the resolver reads the block's result out of the frame's `inst_map` and writes `decl.type` or `decl.val`.
- **Cycles:** re-entering a declaration that is already `resolving_type` or `resolving_value` without a pending request reports "Encountered circular declaration".

## Why change it

The split into type and value phases doesn't match how Blu declarations work now that the declared type is optional (`5f58ad4`). A declaration's type often only becomes known while evaluating its value.

- With `f := ...` there's no type block worth running; the type falls out of the value.
- Stubs exist so a function can refer to itself before its value is done. Deferring function bodies until after declarations gives the same result without a separate status or stub value.
- `SIR_lookup_decl_type` exists mainly so function calls can check argument types. Function signatures are always written out, so a function value has its final type as soon as it's created, and a value lookup covers the same need.
- Residual IIR function bodies live inside `ValueFunc`s, with no single place that owns them. Values that point at runtime functions need a stable handle.

## Planned design

### Codegen: one block per declaration

`generate_code` emits one outer `SIR_comptime_block` per declaration, containing a comptime block for the declared type and one for the value.

- The declared type's block result (nil if there's no declared type) is the `type_destination` for the value expression.
- At each leaf, codegen emits `SIR_as` against the type destination, so the value is determined and coerced/typed in one step. A function literal emits `SIR_unify` of the destination with the literal's function type.
- The declaration's type is the type of its value. `Declaration.data.decl` holds `source`, `tree_idx`, `chunk` and `val`; there is no separate type field.
- The resolver sets the value: when the block finishes, it reads the result from `inst_map[0]` and writes `decl.data.decl.val`. There is no SIR instruction for setting a declaration's value or type.

### SIR opcode changes

| Opcode | Change | Purpose |
| --- | --- | --- |
| `SIR_lookup_decl_value` → `SIR_get_decl_value` | Renamed | Suspend until the target is `fully_resolved`, then produce a pointer to its value |
| `SIR_lookup_decl_type` | Removed | Function signatures are explicit, so the value's type is final when the value exists |

### Resolve statuses

The type steps and the stub go away. The sequence is `unresolved` → `resolving_value` → `fully_resolved`, plus `error`.

### Lookup semantics

`SIR_get_decl_value` behaves the same in comptime and runtime contexts:

- If the target is a `Declaration_decl` and no resolution has been requested yet, it sets `requested_resolution` and returns `Step_resolve_declaration_value`. The resolver pushes the target onto the resolve stack.
- When the entry resumes, the target is `fully_resolved`. The lookup produces a `ValuePointer` to the declaration's value, typed as a pointer to the value's type. Reads go through `SIR_load`.
- A lookup of a declaration that is `resolving_value` without a pending request is a cycle ("Encountered circular declaration").

Recursive functions don't need a stub: the function body is specialized after the declaration is `fully_resolved` (see below), so the recursive lookup finds a finished value.

### Resolver and specializer state

- The resolver lives in `src/resolver.c`. `src/resolver.h` exposes only `ResolveContext` and `resolve_declarations`.
- `ResolveEntry` is `{ SpecializerState state; ArenaSnapshot snapshot; }`, allocated on a fixed stack of `MAX_RESOLVE_DEPTH` (64). Each entry's state is allocated in a scratch arena scope that ends when the entry is popped.
- `SpecializerState` (formerly `RunState`) holds `requested_resolution`, a `Frame`, the `decl`, and an optional `function`. The `function` field is set when the state specializes a function body.
- `Frame` holds the `chunk`, `inst_map`, `inst_types`, `comptime_depth` and a stack of `ScopeSpan`s. Scope kinds are `Scope_block` and `Scope_comptime`.
- There is no IIR builder stack. Each `ResidualFunction` owns its `IIrBuilder`. A declaration's state has no builder: declarations are evaluated entirely at comptime, and residual code only comes from function bodies.
- There is no `EvalStack`, so there are no comptime calls: `SIR_call` always emits `IIR_call`. Comptime calls can be added later by giving a `SpecializerState` a stack of frames.

### Residual functions

`ResidualFunction { status; builder; chunk; decl; instruction; }` is stored in `Compiler.functions`, a `ResidualFunctionList` segment list. Every residual function must be stored there, and values reference functions through it (`ValueFunc` holds a `ResidualFunctionKey`).

Function return types must always be written out; they are never inferred. A function's type is therefore known as soon as its `SIR_func` runs, before its body has been specialized.

### Scheduling declarations and function bodies

A declaration that contains a function does not specialize the function's body. `SIR_func` allocates a `ResidualFunction`, sets the value to point at it, and the declaration keeps going. The body is specialized later.

`scrap.txt` needs this:

```
f := {
  g := ||: i32 { f() + h() }
  ||: i32 { g() }
}

h := ||: i32 { 1 }
```

If `g`'s body were specialized when `g` is created, it would look up `f` while `f` is still `resolving_value`, which is a false "circular declaration". Deferred, `f` resolves to its `() i32` function value first, and `g`'s body can then call `f()` and `h()` normally.

Work is split by whether anything is waiting on it, not by declaration vs. function:

- **Resolve stack.** One stack of entries, each a declaration or a function body (`SpecializerState.function` tells them apart). Whatever is blocked pushes what it needs on top. A function body that looks up a declaration pushes that declaration.
- **Deferred bodies.** `Compiler.functions` is the queue of bodies nobody has asked for yet. After the declaration loop, a second loop runs `for (i = 0; i < functions.len; i++)` and specializes every function that isn't `finished`, re-reading `len` so newly created functions are picked up.

With explicit return types and no comptime calls, nothing ever waits on a function body, so function bodies only enter the resolve stack from the second loop. Once comptime calls exist, a declaration like `n := g()` will need `g`'s body and push it as a dependency; the single stack handles that without changes.

Why not one stack for declarations and one for functions, with declarations first:

- A declaration blocked on a function body (once comptime calls exist) would be picked again forever.
- A cycle through both stacks (`f` → `g` body → `f`) is invisible to a check that only looks at the top of one stack.
- Entries allocate their state with `arena_scope_begin/end` on the scratch arena. That only works if entries finish in LIFO order, which two independent stacks don't guarantee.

Deferred bodies only run when the resolve stack is empty, so declarations still go first; the priority is just the order of the two loops.

### Captures

A function body can use comptime values from the scope it was created in. In `scrap.txt`, the inner lambda calls `g`, a local of `f`'s comptime block. Because bodies are specialized after the declaration's frame is gone, the function needs its own copy of those values.

A capture is a copy taken when the function is created. Keeping the enclosing frame alive instead doesn't work: a lambda created in a comptime loop would see the last iteration's values, frames would have to be kept alive by other function frames, and each kept frame holds its whole `SpecializerState` (~36 KB of `ScopeSpan`s).

Locals compile to `SIR_alloc` + `SIR_store`, and reads are `SIR_load` of the alloc, so the outer frame holds a pointer to a slot, not the value. The capture has to load the value when the function is created, and the body has to read the copy:

- `SIrFunc` gets `capture_count` followed by the outer alloc instructions to capture. When `SIR_func` runs, the specializer loads each one and stores it on the `ResidualFunction` (in `perm`).
- Inside the body, a read of a captured local becomes `SIR_capture i`, returning `captures[i]`. Codegen gives each captured name one index per function.
- Codegen detects captures with the scope stack: a local found below the current function's `ScopeEntry_block` marker is a capture. Globals go through `SIR_get_decl_value` and are never captured.
- Nested functions: if an inner lambda captures `x` from two functions out, every function in between captures `x` as well.
- Capturing an enclosing function's parameter is always a runtime value, so codegen rejects it (currently a `Todo()` in the `Ast_identifier` case of `gen_code_for_ptr`). Whether a local in a runtime body is comptime-known is only known to the specializer, which reports "cannot capture runtime value" at `SIR_func`.
- Mutating a comptime variable after a lambda captured it doesn't affect the lambda. This is a language rule.

Each residual function keeps a flat capture array. Sharing capture blocks between nested functions (linked environments) was rejected: reads would need a depth and an index, each block needs a parent pointer, and blocks would still have to be snapshots per function creation.

### Values that outlive a frame

The frame owns its values: `dealloc_scope_values` frees the payload of every value in a scope's `inst_map` when the scope exits. So a capture needs `values_copy`, not just the `ValueIndex`.

- Nested captures don't copy the payload twice. A residual function's captures are permanent, so when an inner lambda captures what the middle function already captured, `SIR_func` copies only the `ValueIndex`. `dealloc_scope_values` must then skip `SIR_capture` slots, or `SIR_capture` must put a copy in `inst_map`.
- `values_copy` is shallow. It copies the payload, but a `ValuePointer` inside it still refers to a `ValueIndex` the frame owns, and a `ValueSlice`'s `data` still points into frame memory. For now, captures whose type contains a pointer or slice are rejected. Integers, types and functions (`ValueFunc` is a `ResidualFunctionKey`) are fine.

General direction, following Zig: values that can outlive a frame are permanent and immutable, and a pointer into frame memory that escapes is a compile error. In Zig, a `const` that holds a pointer to a `comptime var` is an error, and a cyclic structure is written as a declaration that takes the address of its own elements (`const nodes = [_]Node{ .{ .next = &nodes[1] }, .{ .next = &nodes[0] } };`). Blu can't express that yet: without stubs, a declaration that looks up itself while `resolving_value` is a cycle.

The alternative is copying the whole reachable graph when a value escapes, with an old → new `ValueIndex` map to handle sharing and cycles, walking each payload by its type. That can be added later without changing the language. Either way, pointers must be relocatable: `ValuePointer { val, offset }` is, but `ValueSlice { len, data }` holds a raw pointer and should become `{ val, offset, len }`. The IR → C backend needs this too, to emit permanent values as globals and pointers as `&global + offset`.

## Progress and remaining work

The branch builds at `c738f3e`, but function bodies aren't specialized yet: `SIR_func` and `specializer_state_init_function` are `Todo()`.

| Date | Commit | What landed |
| --- | --- | --- |
| 2026-09-29 | `c738f3e` WIP | Resolver extracted to `src/resolver.c` (`ResolveContext`, one stack of `ResolveEntry { SpecializerState, ArenaSnapshot }`). `ResidualFunction` tracked in `Compiler.functions`. Declaration interner moved to `src/declaration_interner.h`. `SIR_func` is meant to allocate a residual function and return `Step_resolve_function_body` |
| 2026-09-24 | `6dfa85f` Add resolver.h | Moved the `Resolver` types into `src/resolver.h`. Moved `builders` into `RunState` |
| 2026-09-24 | `dfde357` WIP (see description) | Plan written. Removed the type statuses and the `block_type`/`block_val` fields. Added `ResidualFunctionList`, renamed opcodes in `ir.h`, and added `Frame`/`EvalScope` |
| 2026-09-09 | `dda8d75` WIP | Added `ResolveStatus_stub_value` (since removed) and `docs/ARCHITECTURE.md`. Added `test.blu`, where a declaration's value is a recursive function returned from a comptime `if` |
| earlier | `5f58ad4` Make declared type optional | Codegen and specializer changes that let `f := ...` omit the type |

Remaining steps:

- [ ] Specializer: `SIR_func` allocates a `ResidualFunction`, copies captures, and returns without specializing the body
- [ ] Specializer: implement `specializer_state_init_function` (function scope over the `SIR_func` range, captures available)
- [ ] Resolver: allow function-body entries on the resolve stack, with cycle detection for both kinds
- [ ] Resolver: after the declaration loop, specialize remaining bodies from `Compiler.functions`
- [ ] Codegen: `capture_count` + capture list on `SIrFunc`, `SIR_capture i` for captured reads, captures propagated through nested functions
- [ ] Reject captures of runtime values, and of values containing pointers or slices
- [ ] Change `ValueSlice` to `{ val, offset, len }`
- [ ] Check that each declaration's type is fully defined once it's resolved
- [ ] Get `test.blu` and `scrap.txt` running and update `docs/ARCHITECTURE.md`
- [ ] Later: comptime calls (a stack of frames per `SpecializerState`)

## Open questions

- **Where does the "type fully defined" check run?** The plan flags it ("there must be a check to ensure the type of a declaration is fully defined at some point"). The natural place is where the resolver sets the declaration's value.
- **Self-referential data declarations.** A struct like `Node` holding a pointer to `Node`, or a constant that takes its own address, needs a declaration's address or type before its value is done. That was a use for stubs; with them gone, it needs another mechanism once aggregates are in the language.
- **Deep copy or escape error for comptime pointers?** Starting with the Zig rule (escaping pointer into frame memory is an error). Revisit if that's too restrictive.
