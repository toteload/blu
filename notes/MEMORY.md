# Project memory

Design decisions and preferences that the code does not show on its own.
Anything not implemented yet is marked as planned. Never describe planned work as if it already exists.

## IR design

- Keep the opcode set minimal and orthogonal. Every opcode must express something the others cannot. Do not add convenience or fused opcodes unless asked.
- Split an opcode when the split carries real semantics, such as "can fail" vs "cannot fail" (`int_extend` vs `int_cast_check`).
- IIR should lower cleanly to C/LLVM.
- Present new IR formats for review before writing lowering or interpreter code.

## Interpreter

- The interpreter runs IIR directly. The separate bytecode layer was dropped on purpose; being slower than a bytecode interpreter is accepted. Do not reintroduce a bytecode layer.
- The interpreter is runtime-only. It never runs comptime code.
- Memory should be reclaimed at scope boundaries, not held in compiler-lifetime arenas.
- Planned, not implemented yet:
  - Runtime memory uses 8-byte host addresses instead of `ValuePointer`, so its layout matches C exactly. Runtime memory regions must never move once addresses into them exist.
  - Calling C functions through libffi (`dlopen`/`dlsym`, `LoadLibraryW`/`GetProcAddress` on Windows). The goal is to call any C function, including structs by value, so do not restrict the signatures that can cross the boundary.
- See `notes/iir_plan.md`.

## Values and refs

- Refs share a 2-bit tag in the top bits: ValueKey, ValueOffset or InstructionOffset. IRef, SRef and ValueRef can be combined.
- A pointer is `ValuePointer { base, offset }`, in comptime, interned and (for now) runtime memory. Comptime and interned memory keep it permanently, which keeps comptime target-independent and safe; do not use `void *` pointers there. Runtime memory is planned to switch to host addresses (see Interpreter).
- Interned values hold canonical bytes, so a pointer inside an interned value is always a ValueKey, never an offset or an address.
- Planned, not implemented yet: once runtime memory uses host addresses, an interned value without pointers is used in place at runtime, and one with pointers is materialized once into runtime memory with its pointers relocated to addresses, like a linker does for static data. These will be the only per-key copies.
- `mut` globals live in a preallocated region. The specializer records emitted `IIR_global` instructions, and after specialization `compiler.c` allocates only the referenced `mut` globals and patches the offsets.
- Writing through a slice requires `[]mut T`. A literal copied into a `[]mut` local gets fresh memory, so it is valid.

## Comptime

- The specializer is the comptime engine. Comptime code is never specialized to IIR and then interpreted.
- Comptime code never calls C. Comptime access to the outside world will go through explicit compiler builtins; none exist yet.
- Comptime function calls are not implemented yet (they report an error). When added, they run in the specializer and push onto its explicit frame stack, never C recursion, so deep comptime recursion cannot crash the compiler.
- Comptime locals live in `compiler->memory` as offset refs. Each comptime scope snapshots and restores that memory.
- Values are interned only when they escape, and escaping memory is frozen (deep-interned). Escape detection does not exist yet because there is no address-of syntax.
- There are no top-level comptime mutable variables.
- Immutable locals are bindings, like params, and whether they are comptime is inferred.
- A `mut` local of a comptime-only type at runtime is an error. Explicit `comptime mut` is planned for later.
- Do not default `comptime_int` to `i64`; this was explicitly rejected.

## Nil, never and unknown

- `TypeKey` 0 means "unknown, still to be inferred" (omitted annotations, unification). It never means "nothing".
- "Nothing" is the type `TYPE_NIL`, and its only value is `VAL_NIL_NIL`. `VAL_TYPE_NIL` is the type `nil` as a value of type `type`; do not use it to mean "no value".
- A function's return type is always a real type after its signature is finalized. A function that returns nothing has return type `nil`.
- A block with no breaks never falls through, so its type is `never`. `never` coerces to any type, so it also unifies with any type.
- `br` and `repeat` have type `never`. An assignment expression has the value `VAL_NIL_NIL`.
- An `if` without `else` has type `nil`; the `then` value is discarded.
- Every executed SIR instruction records its type in `inst_types`. `get_sref_type` asserts on a missing type.
- Every emitted IIR instruction has a type. `condbr`, `br` and `repeat` have type `never`. The interpreter's `layout_of` asserts on a missing type.

## Naming

- `TokenIndex` and `InstructionIndex` are plain offsets and never optional. Never treat 0 as "no location".
- Every other u32 handle is a `*Key` type from `key.h`, with 0 as nil. Use the `Key` suffix for new opaque handles.
