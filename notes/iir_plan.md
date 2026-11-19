# IIR

IIR used to do two jobs: be the backend-neutral target that could later go to C or LLVM, and be the format the interpreter runs.
An earlier version of this plan split those apart with a separate interpreter bytecode. That was dropped: the interpreter runs IIR directly.
It will be slower than a bytecode interpreter, and that is fine.

```
SIR -> specializer -> IIR -> interpreter
                         \-> (later) C / LLVM
```

## Decisions

- **IIR is the backend-neutral IR.**
  - It is typed (a `TypeKey` per instruction) and keeps structured `block`/`loop` control flow.
  - It has no size information.
  - An IIR ref (`IRef`) is either an instruction or a `ValueKey`. A chunk has no constants array, and constants are never copied into the IIR.
  - Comptime-only types (`type`, `comptime_int`) must not appear in IIR. This is not enforced yet.
  - An IIR ref is never nil. `br` and `ret` with no value carry the zero-size `VAL_NIL_NIL`; there are no void variants.
  - `int_cast` is split into `int_extend`, a sign or zero extension that cannot fail, and `int_cast_check`, which checks that the value fits.
- **The interpreter runs IIR.** No IIR -> bytecode lowering.
  - It is runtime-only. Comptime code never reaches it.
  - It gets sizes from instruction types.
  - Each instruction result gets a slot (`value_offsets`); block scopes are popped with `scopes_pop_to`, so memory is reclaimed at scope boundaries.
- **Comptime function calls run in the specializer**, not through the interpreter.
  - The specializer is needed anyway for calls whose signatures involve comptime-only types.
  - Two execution engines for comptime would have to agree on every semantic detail.
  - Consequence: the interpreter only runs after specialization has finished, when all stubs are resolved.
  - Comptime stays target-independent and safe: values use the target layout and `ValuePointer`, and bad comptime code produces a diagnostic instead of crashing the compiler.
  - Comptime never calls C.
  - Comptime function calls are not implemented yet; `SIR_call` in a comptime frame reports an error. See Planned.
- **Interned Values keep `ValuePointer { ValueKey, offset }`.** This is the comptime and canonical representation.
- **The index of `a[i]` / `s[i]` is usize by the time it reaches IIR.**
- **A pointer into a popped scope or frame dangles**, as in C. This is fine for now.
- **Comptime coercion never happens at runtime.** Comptime-only types do not appear in IIR, so the runtime path of `coerce()` treats `Coercion_comptime` as `Unreachable()`.
- **Array to slice is not a runtime coercion.** At runtime the specializer emits a slice built from a pointer to the array and the array's length as a constant `usize`. IIR has no instruction yet to take an address or to build a slice; both are needed for this.
- **The specializer's `inst_map` keeps using `IRef`.** It only needs its own ref type once comptime stack offsets from the ValueRef plan are added as a third case.

## Done

- Removed `size_infos`, `CRef`, the `IIrChunk*` mirror structs, `IIrConstant`, `iir_chunk_convert_refs`, and the specializer's `ConstantFlattener`.
- The IIR printer prints a `ValueKey` ref directly from the Value.
- The interpreter runs IIR directly (see Decisions).
- `int_cast` was split into `int_extend` and `int_cast_check`.
- The `SIR_cast` arm switches on `classify_type_cast`:
  - coercions go through `coerce()`;
  - `CAST_INT_RANGE_CHECK` either evaluates the cast at comptime or emits `int_cast_check`.
- The bytecode draft (`src/bytecode.h`, `src/x_bytecode.h`) was removed.

## Planned

Decided, but not implemented yet.

- **Runtime memory uses host addresses.** Pointers in runtime memory become 8-byte host addresses instead of `ValuePointer`, so runtime memory has exactly C's layout. Runtime memory regions are reserved up front and never move.
- **Only constants with pointers are materialized.** A `ValueKey` operand without pointers is used in place as the interned Value. One with pointers is materialized once per key into runtime memory, with its pointers relocated to addresses. A C backend would emit the same thing as static data with relocations.
- **C calls through libffi.** Arguments already have C layout, so their pointers go straight to `ffi_call`. An extern call is an `IIR_call` whose callee is an extern function; there is no separate opcode. The `extern` declaration syntax and its SIR/IIR form still need to be designed and reviewed.
- **Comptime function calls in the specializer.** They push onto the specializer's explicit frame stack, not C recursion.
  - Memoize comptime calls by function plus argument `ValueKey`s, only when every argument is interned.
  - An evaluation quota so infinite comptime loops become errors.
- **Compiler builtins for comptime access to the outside world**, since comptime never calls C.
- **Possibly a build program** (like Zig's `build.zig`): an ordinary Blu program run by the interpreter, which can call C, producing files the main compile reads. Preferred over C calls at comptime.

## Open questions

- **`bit_not`:** today the interpreter implements it as boolean not (`!*v`). Should it become `bool_not`, or real bitwise not?

## Loose ends and findings

- **Missing language features block runtime array tests.** Array type syntax (`[5]u8`) hits a `Todo()` in `specialize.c`, and untyped locals (`a := ...`) assert in `codegen.c`. Until they work, no program can create a runtime array, so the array path of indexing can't be tested end to end.
- **Interpreter gaps:**
  - `interpret.c` still lowers every constant to raw bytes (`constant_data`, `materialize`, `Interpreter.constants`). Once runtime memory uses host addresses (see Planned), only constants that contain pointers should be materialized.
  - `bit_or` and `bit_xor` are `Todo()`.
- **`Unreachable()` vs `Todo()`.** In non-safe builds `Unreachable()` is `__builtin_unreachable()`, which is undefined behavior if it is reached. Use `Todo()` for cases that are merely not implemented yet. `ttld_panic_handler` is now `TTLD_NORETURN`, so switches that end in a panic no longer cause "may be used uninitialized" warnings.
- **`types.h` static asserts warn.** The `TTLD_STATIC_ASSERT` lines comparing `CoercionResult` with `CastClassification` trigger `-Wenum-compare` in every file that includes `types.h`. Casting both sides to `u32` fixes that.
- **`parse.c` still uses `_Static_assert`** directly, in 13 places, instead of `TTLD_STATIC_ASSERT`.
- **Cast classification semantics.** `classify_type_cast` tries a coercion first, so `CAST_INT_RANGE_CHECK` always means "needs a range check". A `comptime_int` source is always a coercion, so its range error comes from `coerce()`: "Value of comptime_int does not fit in the destination type %type".
- **Hash-consing merges equal constants.** Two distinct comptime values with identical contents share a `ValueKey`, and therefore one address. This is fine for immutable data, but a C backend would inherit it.
