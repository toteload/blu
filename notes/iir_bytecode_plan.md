# IIR / bytecode split

The current IIR does two jobs: it is the backend-neutral target that could later go to C or LLVM, and it is the format the interpreter runs.
This plan splits those jobs apart.

```
SIR -> specializer -> IIR -> bytecode -> interpreter
                         \-> (later) C / LLVM
```

## Decisions

- **IIR is the backend-neutral IR.**
  - It is typed (a `TypeKey` per instruction) and keeps structured `block`/`loop` control flow.
  - It has no size information.
  - An IIR ref (`IRef`) is either an instruction or a `ValueKey`. A chunk has no constants array, and constants are never copied into the IIR.
  - Comptime-only types (`type`, `comptime_int`) must not appear in IIR. This is not enforced yet.
  - `br`/`br_void` and `ret`/`ret_void` are separate opcodes, so an IIR ref is never nil.
  - `int_cast` is split into `int_extend`, a sign or zero extension that cannot fail, and `int_cast_check`, which checks that the value fits.
- **The interpreter runs its own bytecode, not IIR.** Every operand is a byte offset.
- **Comptime function calls run in the specializer**, not through IIR -> bytecode -> Value.
  - The specializer is needed anyway for calls whose signatures involve comptime-only types.
  - Two execution engines for comptime would have to agree on every semantic detail.
  - Consequence: the interpreter only runs after specialization has finished. IIR is lowered to bytecode in a single pass at the end, by which point all stubs are resolved. No conversion from bytecode results back to Values is needed.
  - A bytecode fast path for heavy comptime calls with all-runtime signatures can be added later, if profiling shows it is needed.
- **Interned Values keep `ValuePointer { ValueKey, offset }`.** When IIR is lowered to bytecode, constants are deep-converted into raw bytes with real pointers. This is cached once per `ValueKey` for the whole compiler, so the same constant has the same address in every function.
- **The specializer's `inst_map` keeps using `IRef`.** It only needs its own ref type once comptime stack offsets from the ValueRef plan are added as a third case. That is not part of this work.

## Phase 1: reshape IIR (done)

- Removed `size_infos`, `CRef`, the `IIrChunk*` mirror structs, `IIrConstant`, `iir_chunk_convert_refs`, and the specializer's `ConstantFlattener`.
- The IIR printer prints a `ValueKey` ref directly from the Value.
- The interpreter temporarily runs IIR directly:
  - it gets sizes from instruction types;
  - it converts constants with `constant_data`, cached per `ValueKey` in `Interpreter.constants`. In Phase 2 this conversion moves into bytecode lowering.
- `int_cast` was split into `int_extend` and `int_cast_check`.
- The `SIR_cast` arm switches on `classify_type_cast`:
  - coercions go through `coerce()`;
  - `CAST_INT_RANGE_CHECK` either evaluates the cast at comptime or emits `int_cast_check`.

## Phase 2: bytecode format and lowering

The format is in `src/bytecode.h` and the opcode list in `src/x_bytecode.h`.

### Format

- **Operands.** `BcOperand` is a u32 byte offset.
  - Bit 31 clear: the offset is into the current frame.
  - Bit 31 set: the offset is into the function's constants blob.
- **Constants blob.** Each function copies the outer bytes of the constants it uses into its own blob. Pointers inside those constants point into the compiler-wide cache, so the same constant keeps one address everywhere. IIR has no "address of a constant" operation, so duplicating the outer bytes cannot be observed.
- **Code.**
  - The code is a byte stream of instruction structs, and each struct starts with `u32 op`.
  - Every field is 4 bytes wide, so the stream needs no padding. The next instruction starts at `pc + sizeof(struct)`; `call` also adds the size of its argument array.
  - `pc` and jump targets are byte offsets into the code.
- **Type information sits in the instructions**, so the interpreter never looks up a type at runtime:
  - integer ops carry a `TypeInteger`;
  - `copy`, `load` and `store` carry a size;
  - `elem_ptr` carries a stride;
  - `debug` carries a `TypeKey`, used only for printing.

### Per-function data (container not designed yet)

`BcChunk`, `BcSlot` and `BcLocation` were removed, to be redesigned. Whatever replaces them has to hold:

- **Code:** the instruction byte stream.
- **Constants blob:** the outer bytes of the constants the function uses.
- **Frame:** the frame's size and alignment. Each call pushes one frame and each return pops it. Variables are frame offsets, so an `alloc` inside a loop uses the same slot on every iteration.
- **Parameters:** each parameter's frame offset and size. The caller writes each argument there.
- **Return slot:** its frame offset and size (size 0 for void). `ret` takes no operand: the value is written into the return slot first, and on `ret` the interpreter copies it into the caller's `dst`.
- **Error locations:** a mapping from pc to source location. It is only searched when an error is reported.

### Opcodes

| Opcode | Meaning |
|---|---|
| `copy dst, src, size` | Copy bytes between two operand locations. |
| `jmp target` | Jump. |
| `jmp_if_false cond, target` | Jump if `cond` is false. |
| `load dst, ptr, size` | Copy from the address stored at `ptr`. |
| `store ptr, src, size` | Copy to the address stored at `ptr`. |
| `call dst, func, args[]` | `func` holds the bytes of a `ResidualFunctionKey`. |
| `ret` | Copy the return slot to the caller's `dst` and pop the frame. |
| `int_extend dst, src, from, to` | Sign or zero extension; cannot fail. |
| `int_cast_check dst, src, from, to` | Conversion with a range check. |
| `int_add/sub/mul/div/mod`, `int_cmp_*` | Integer ops carrying a `TypeInteger`. |
| `bit_not`, `bit_and/or/xor` | Bit ops carrying a size. |
| `addr dst, src` | Address of an operand location (needed for inline arrays). |
| `bounds_check index, len` | Error if `index >= len`. |
| `elem_ptr dst, ptr, index, stride` | `dst = *ptr + index * stride`. |
| `debug dst, src, type, size` | `#debug`. |

### How IIR lowers

| IIR | Bytecode |
|---|---|
| `block`, `loop` | Nothing is emitted. |
| `br v` | `copy` into the block's result slot, then `jmp`. |
| `br_void`, `repeat` | `jmp`. |
| `condbr` | `jmp_if_false`. |
| `ret v` | `copy` into the return slot, then `ret`. |
| `ret_void` | `ret`. |
| `alloc` | Nothing is emitted; the variable gets a frame slot. |
| `load %alloc` / `store %alloc, v` | `copy` from or to the variable's frame slot. |
| `load` / `store` through any other pointer | `load` / `store`. |
| `param` | Nothing is emitted; covered by the function's parameter offsets. |
| `#len s` | `copy dst, s+offsetof(len)`. |
| `a[i]` (array) | `addr t0, a` -> `bounds_check i, const(len)` -> `elem_ptr t1, t0, i, stride` -> `load`. |
| `s[i]` (slice) | `bounds_check i, s+offsetof(len)` -> `elem_ptr t1, s+offsetof(data), i, stride` -> `load`. |

An `alloc` whose pointer is used as a value cannot happen today, because there is no address-of operator. Lowering asserts on it.

### Lowering steps

1. Build the compiler-wide constant cache, mapping each `ValueKey` to raw bytes. Resolve stubs, and convert every `ValuePointer` inside a value into a real pointer. This is `constant_data` moved out of `interpret.c`.
2. Lay out each frame: walk the IIR and give every result and every variable a fixed frame offset.
3. Emit the code and the per-function data listed above.
4. Lower every function in one pass after specialization. A `ResidualFunction` holds both its IIR and its bytecode.

## Phase 3: interpreter on bytecode

- The interpreter runs only bytecode.
- A call pushes the callee's frame and writes the arguments to its parameter offsets. `ret` copies the return slot to the caller's `dst` and pops the frame.
- This removes `value_offsets`, the per-instruction slot allocation, the scope stack (`ScopeStack2`, `scopes_pop_to`) and the temporary `constant_data`.
- `run_main` calls into the bytecode.

## Open questions

- **Per-function container:** how to store the per-function data listed in Phase 2. `BcChunk`, `BcSlot` and `BcLocation` were removed so this can be redesigned.
- **`bit_not`:** today the interpreter implements it as boolean not (`!*v`). Should it become `bool_not`, or real bitwise not?
- **Index operand:** `bounds_check` and `elem_ptr` read `index` and `len` as usize. Is the index always usize by the time it reaches IIR?
- **Frame slot reuse:** should sibling blocks share frame space (frame size = deepest nesting), or should each instruction get its own slot (simpler, wastes memory)?
- **Escaping pointers:** a pointer into a frame dangles once that frame is popped. This is the same rule as in C, and it is a language-semantics decision.
- **Array and slice casts at runtime:** `CAST_COERCION_ARRAY_TO_SLICE` and `CAST_COERCION_COMPTIME` are `Todo()` in `coerce()`. They become reachable once array types can be written in source.

## Loose ends and findings

- **Missing language features block runtime array tests.** Array type syntax (`[5]u8`) hits a `Todo()` at `specialize.c:1054`, and untyped locals (`a := ...`) assert at `codegen.c:469`. Until they work, no program can create a runtime array, so `addr` and the array path of indexing can't be tested end to end.
- **Temporary state of the interpreter (until Phase 3):**
  - Constants are converted on first use. A constant containing an unresolved stub fails its `Assert` at runtime instead of right after specialization.
  - `bit_or` and `bit_xor` are `Todo()`.
  - With `--print-iir`, a pointer or slice constant prints the address of the interned Value's data, not the address the program sees at runtime.
- **`Unreachable()` vs `Todo()`.** In non-safe builds `Unreachable()` is `__builtin_unreachable()`, which is undefined behavior if it is reached. Use `Todo()` for cases that are merely not implemented yet. `ttld_panic_handler` is now `TTLD_NORETURN`, so switches that end in a panic no longer cause "may be used uninitialized" warnings.
- **`types.h` static asserts warn.** The `TTLD_STATIC_ASSERT` lines comparing `CoercionResult` with `CastClassification` trigger `-Wenum-compare` in every file that includes `types.h`. Casting both sides to `u32` fixes that.
- **`parse.c` still uses `_Static_assert`** directly, in 13 places, instead of `TTLD_STATIC_ASSERT`.
- **Cast classification semantics.** `classify_type_cast` tries a coercion first, so `CAST_INT_RANGE_CHECK` always means "needs a range check". A `comptime_int` source is always a coercion, so its range error comes from `coerce()`: "Value of comptime_int does not fit in the destination type %type".
- **Hash-consing merges equal constants.** Two distinct comptime values with identical contents share a `ValueKey`, and therefore one address. This is fine for immutable data, but a C backend would inherit it.
