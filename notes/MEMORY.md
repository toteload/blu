# Project memory

Design decisions and preferences that the code does not show on its own.

## IR design

- Keep the opcode set minimal and orthogonal. Every opcode must express something the others cannot. Do not add convenience or fused opcodes unless asked.
- Split an opcode when the split carries real semantics, such as "can fail" vs "cannot fail" (`int_extend` vs `int_cast_check`).
- IIR should lower cleanly to C/LLVM.
- Present new IR formats for review before writing lowering or interpreter code.

## Interpreter

- The interpreter runs IIR directly. The separate bytecode layer was dropped on purpose; being slower than a bytecode interpreter is accepted. Do not reintroduce a bytecode layer.
- Memory should be reclaimed at scope boundaries, not held in compiler-lifetime arenas.
- See `notes/iir_plan.md`.

## Values and refs

- Refs share a 2-bit tag in the top bits: ValueKey, ValueOffset or InstructionOffset. IRef, SRef and ValueRef can be combined.
- A runtime pointer is `ValuePointer { base, offset }`. Runtime memory uses the same byte representation as interned values. Do not reintroduce per-key constant buffers or `void *` pointers.
- Interned values hold canonical bytes, so a pointer inside an interned value is always a ValueKey, never an offset.
- `mut` globals live in a preallocated region. The specializer records emitted `IIR_global` instructions, and after specialization `compiler.c` allocates only the referenced `mut` globals and patches the offsets.
- Writing through a slice requires `[]mut T`. A literal copied into a `[]mut` local gets fresh memory, so it is valid.

## Comptime

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

## Naming

- `TokenIndex` and `InstructionIndex` are plain offsets and never optional. Never treat 0 as "no location".
- Every other u32 handle is a `*Key` type from `key.h`, with 0 as nil. Use the `Key` suffix for new opaque handles.
