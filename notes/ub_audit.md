# UB / memory-safety audit (2026-11-18)

This audit read the code by hand and mutation-fuzzed the snippet corpus against the ASan/UBSan build. TODO paths were ignored. The following were fixed in the same pass:

- A format-argument mismatch in the parser.
- Nil types from instructions that fold to values. Every handler now records `inst_types`, and value consumers use `expect_some_sref_type`, which reports a nil type as "Expected a value".
- Nil values reaching `#debug`.
- An uninitialized read from `if` without `else`. Its type unified to the `then` type, so the false path left the result slot unwritten. It now has type `nil`; see "Nil, never and unknown" in `notes/MEMORY.md`.

The open findings follow.

## Logic bugs that corrupt compiler state

- `compile()` in `compiler.c` has two problems in its duplicate-declaration paths:
  - The duplicate-`decl` path calls `continue` without `offset += 1`. The same declaration is re-read for every remaining sibling, so later siblings are never registered and their `source->decl_keys` entries stay unwritten. Repro: `a := 1; a := 2; b := 3; main := ...` reports the duplicate 3 times, and `b` and `main` are never registered.
  - The path for a mod that collides with a non-mod (e.g. `mod i32`) adds `child_count` without the +1 for the mod itself. It also pops the parent frame instead of skipping the mod's children.

## Latent memory-safety holes (not reachable yet)

- **Array layout truncation.** In `types.c`, `types_value_layout` computes `u32 size = stride * array.size`, which silently truncates a `u64`. `Value.size` is also `u32`. `[4294967296]u8` gets size 0, while the bounds checks in `interpret.c` (`IIR_index`) and `specialize.c` (`SIR_index`) compare against the full `u64` length. This becomes an out-of-bounds read/write as soon as a value of such a type can be built. Reject sizes that don't fit, or use checked multiplication.
- **`write_type` format string.** `write_type` in `types.c` prints the `u64` array size with `"[%u]"`, which is UB; `[4294967296]u8` prints as `[0]u8`. The slice and pointer cases also pass an unused `array.size` argument.
- **Fixed-size interpreter stacks guarded only by `Assert`.**
  - The call stack has 64 frames (`run_main` in `compiler.c`), so recursion deeper than 64 aborts.
  - The per-function scope stack is `MAX_SCOPE_DEPTH` 64 (`interpret.c`), so more than 64 nested blocks abort.

  With `NDEBUG` both write past the array. They should produce a diagnostic.
- **Broken free-list helpers.** `freelist_alloc` and `freelist_free` in `toteload.h` are unused and wrong:
  - `freelist_alloc` sets `*freelist = *freelist`, so it never advances.
  - `freelist_free` writes `**freelist` where it should write `*freelist`.

  Fix them or delete them before first use.
- **Unbounded recursion in the parser.** About 100k nested `{` overflows the C stack. Codegen and printing likely behave the same.

## Aborts on user input that should be diagnostics

- `gen_code_for_ptr` in `codegen.c` hits `default: Panic()` when an assignment target is not an lvalue, e.g. `= comptime x`.
- `SIR_type` in `specialize.c` uses `Assert(ok)` after `expect_type_value_or_nil` for function types. It is unreachable today because the parser rejects non-type expressions in type position.

## Structural observations

- **Nil keys are not safe to look up.** Reserved slot 0 makes `types_get(0)` and `values_get(0)` return `NULL` instead of trapping. Adding `ASSERT_SAFE(key != 0)` in those getters would make any regression fail at the source.
- **`Assert` is plain `assert`.** Guards on paths that user input can reach disappear under `NDEBUG`. Use diagnostics or `ASSERT_SAFE` for those.
- **The comptime arena depends on LIFO order.** `compiler->memory` snapshots are only correct while declaration resolution is stack-shaped. It is today: the primary stack is LIFO and function bodies are deferred. Anything that resumes a suspended entry out of order would silently overwrite live comptime locals. Consider asserting in `arena_scope_end` that `snapshot.at <= arena->at`.
- **Comptime has no step limit.** A comptime infinite loop hangs the compiler.
- **Invariant: every executed SIR instruction records its type in `inst_types`.** The type is nil only when the instruction produced no value. `store_inst_comptime` stores a comptime value together with its type. `SIR_func` gets its type when its signature scope is finalized.
