# Code review findings

Review of the `tidying` branch at `aa8c761`, plus the uncommitted changes to `src/types.c` and `TODO.md`.
Findings are sorted by severity.

**How the findings were checked**

- **Verified**: I reproduced the problem.
  Every program crashes at startup (C1), so the other repros used a scratch copy with only that one assert relaxed.
  The repository itself was not modified.
- **Inspection**: found by reading the code, not executed.

Exit code `0xC0000374` is `STATUS_HEAP_CORRUPTION` and `0xC0000005` is an access violation.

---

## High

### H5. `run_main` trusts that `main.main` is a function (Verified)

- `src/compiler.c:830-846` casts `v->data` to `ValueFunc*` without checking the type (there is a `TODO`).
- Repro: `main : i32 = 5` → exit `0xC0000005`.
- The return buffer is a fixed `u8 buf[16]`.
  A `main` returning anything larger (for example an array) would overflow it in `IIR_ret`'s `memcpy`.

---

## Medium

### M2. `if` without `else` whose then-branch yields a value panics (Verified)

- `src/specialize.c:96-100` (`end_residual_block`) calls `Todo()` when break types differ.
- The then-branch `br` has the type of its last expression, but the implicit else `br` has type 0.
- Repro: `if x > 0 { #debug(x) }` → `TODO: panic in end_residual_block`.
- Branches with genuinely different types also hit this `Todo` instead of a diagnostic.

### M4. Top-level declarations crash on many ordinary initialisers (Verified)

- `b : bool = true and false`: any block, `if`, `and`/`or`, or `while` hits `SIR_block` inside a `Scope_eval_block` → `Todo` at `specialize.c:440`.
- `x : i32 = { 5 }` panics in codegen: `gen_code_for_declaration_type` has `Ast_block: Todo()` (`codegen.c:1046`).

### M10. Integer cast/coercion paths read `data.integer` from non-integer types (Inspection)

- `SIR_cast` (`specialize.c:716-720`), the runtime `SIR_as` path (`specialize.c:661`, which always emits `IIR_int_cast`) and `IIR_int_cast` (`interpret.c:309`) all treat `type->data.integer` as valid.
- That is true for every type that `is_type_castable_to`/`classify_type_coercion` accepts, including `bool` and array→slice.
- `bool` and `usize` work only because `compiler_init` happens to fill `.data.integer` for them (`compiler.c:191`, `196`).
- Array→slice reads the array's `base_type` bytes as `signedness`/`bitwidth`.
- The same assumption appears in `eval_coerce` for `usize`/`isize` destinations (`eval.c:354`).
- No `isize` value with integer data is ever created, so an `isize` would reach `Unreachable()`.

---

## Low

### L1. Diagnostics and printing

- **Last line of a file loses its final character in diagnostics** (Verified):
  - `tokens_find_line_info` (`tokenize.c:514-518`) computes the last line's length without a newline, but `get_position_info` subtracts 1 for the newline (`messages.c:366`).
    Repro: an error on the last line `main := ||: i32 { xyz }` shows `{ xyz ` with the `}` missing.
  - If the last line is empty, `line_len - 1` underflows.
    `%.*s` then receives a negative precision and reads past the text buffer until a NUL.
- **Unterminated string or label errors point to the wrong place** (Verified): they use `tokenizer->end` as the offset (`tokenize.c:250`, `262`, `387`, `415`), but the line lookup returns the line where the string started.
  The repro printed `3:26` on a 16-character line, followed by the rest of the file.
- **`message_format`'s `bufsize` is never updated** (`messages.c:207`).
  Every size check is therefore always true, and the function works only because each `arena_push` is contiguous with `buf`.
  Either track the size or drop the checks.
- **`write_type` problems** (`types.c`):
  - `"[%u]"` is passed a `u64` (`types.c:461`).
  - The slice and pointer cases read through `data.array` (`469-476`).
  - `Type_function` is `Todo()` (`466`), so any message with `%type` on a function type panics.
  - There is no return after the switch.
- **`arena_printf` in `write_type`** relies on rewinding `arena->at` by 1 to drop the NUL (`types.c:426`).
  That is fine, but it is the same contiguity assumption as above.

### L2. Lexer and parser

- `u8""` evaluates to `34` (the closing quote).
  `s.len - 4 > 1` is false for length 4, so `s.str[3]` is read (`codegen.c:590-597`). Verified.
- **Label error offsets**: see L1.

### L3. Memory and lifetime

- `freelist_alloc`/`freelist_free` in `toteload.h:265-274` are wrong; they are currently unused.
  - `freelist_alloc` does `*freelist = *(void***)freelist`, which is a no-op, so the head is never advanced.
  - `freelist_free` links the node to `head->next` instead of `head`, and dereferences `NULL` on an empty list.
- `frame_push` (`specialize.c:142-148`) takes `.snapshot = arena_scope_begin(arena)` inside the same compound literal as the two `arena_push_array` calls.
  The evaluation order of initialisers is unspecified in C11, so the snapshot position isn't guaranteed.
- `eval_coerce` for array→slice aliases the array's payload (`eval.c:380`, there is a `TODO`).
  Once the source array value is freed by `dealloc_scope_values`, the slice dangles.
  This is currently hard to reach because array types are broken (H3).
- String literal values set `data_size = s.len` (including the quotes) while the type size is `len` (`codegen.c:716`).
- **Fixed-size stacks without checks**:
  - The segment list has no check against `SEGMENTLIST_SEGMENT_COUNT` (`segment_list.h:370`).
  - The specializer's `builders`, scopes and call stack use `stack_push_ptr`, which only asserts.
  - `scope_add_break_or_return` panics after 16 entries.
- `iir_flex_array_size` uses `sizeof(SRef)` for an `IRef` array (`ir.c:226`). Harmless today because both are a `u32`.
- `cstd_alloc_fn` passes `realloc(NULL, 0)` through for zero-sized values (for example `nil`), which is implementation-defined.
  `memcpy` with a possibly `NULL` destination follows in `values_copy`.

### L4. Portability

- `vmem_*` is only implemented for Windows and macOS (`toteload.c:299-355`), so Linux fails to link.
- `arena_init` doesn't check for `NULL`/`MAP_FAILED`.
- `read_file` uses `i32` for `ftell` (`source_file.c:290`).
  Note: these last two were in the `TODO.md` section deleted in the working tree, so they are no longer tracked anywhere.

---

## Tests, tooling and docs

- **The end-to-end snippet suite isn't part of the build, and is currently all red (C1).**
  `test/snippet_test.py` is the only test that exercises codegen, the specializer and the interpreter.
  Worth running from `build.py` or CI.
- **Stale snapshot**: `test/snippet/error/index_out_of_bounds_runtime.blu.snap` expects `Index is out of bounds`.
  The interpreter now prints `… index = 9, bounds = 0 .. 2` (`771efba`).
- **Hashmap and segment-list tests are not built**:
  - `test/hashmap.test.c` and `test/segment_list.test.c` aren't in `add_test_suite` (`build.py:26-33`) or registered in `main_test.c`.
  - `test/hashmap.fuzz.c` no longer compiles: 2-argument compare function, `Is_null`, and no `HASHMAP_OUTPUT_TYPES`.
  - I ran a fresh randomized insert/remove/find fuzz (50 rounds × 200k ops) against the current `hashmap.h`, including the in-place rehash and realloc grow paths, and it passed.
    The recent optimization looks correct; it is just untested in-tree.
- **`CLAUDE.md` is out of date**:
  - The CLI flags are `--print-sir` / `--print-iir`, not `--print-decl-ir` / `--print-residual`.
  - The unit test binary is `out/blu_unit_test(.exe)`, not `out/blu.test`.
  - Sanitizers are **off** (`USE_SANITIZERS = False`), not on by default.
  - Message placeholders are `%tokenkind` / `%string` / `%type` / `%u64`, not `{tok}` / `{str}`.
