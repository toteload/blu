- [ ] Add user error reporting on failure for SIR_unify in specialize.c
- [ ] Compile time evaluation of binary operators in specialize.c
- [ ] Add type inference on declarations (`x := 5`). The type part of a declaration is optional. `case Ast_declaration` in codegen.c
- [ ] Add location information to interpreter error reporting
- [ ] The symbol table doesn't work correctly for nested functions. you keep access to variables in the enclosing function.
- [ ] Typecheck main.main before running it

## Comptime

- [ ] Add `eval` keyword to evaluate expressions at compile time. (or just use `comptime` as the name)
- [ ] Add `comptime` qualifier to function parameters.
- [ ] Add `comptime` qualifier to declarations. (Maybe not a qualifier, just use `eval`)
  - This means that the declaration only exists at compile time.
    All uses of the declaration are replaced with its value.


## General

- [ ] `test\basic\slice.blu`
  - Includes array type and string literal.
  - Includes coercion of sequence literals to typed slices.
  - Includes indexing of slices.
  - Includes coercion of array to slice.
- [ ] `test\basic\defer.blu`
- [ ] Add assign of variables.


## Control flow

- [ ] `break`/`continue`
- [ ] Add `return` expression.
- [ ] `for` loop
- [ ] `while` loop
- [ ] `match` expression.
  - Can match on integer values.
  - Can match on enums.


## Types

- [ ] `struct` type
- [ ] optional types.
  - Analyze the type which is made optional for niche values.
- [ ] Add `distinct` qualifier.
- [ ] Aribtrary size integers
- [ ] Integer range.
  - `a..b` exclusive range. `a..=b` inclusive range.
- [ ] Refinement
  - For integer types you could use `int(<values>)` syntax.
- [ ] `[*]T` 'multi'-pointer.
- [ ] `*T` pointer.
- [ ] `enum` definitions.
  - I think something like `Direction := enum { north, east, south, west }` is good.
  - Effectively, the enum values are named integer constants. So `Direction` does not have a size.
    - Then what is the type of `Direction`? I am guessing `distinct enum`?
      - I guess that would mean that enums are nominal. Does that make sense?
  - You can 'size' an enum through integer refinement, e.g. `u32(Direction) dir`.
- [ ] Add coercion of `bool` to integer types.
- [ ] Floating point type
- [ ] Add flow typing

- pointer to member of a struct. `PtrY := *{struct { x: i32, y: i32 }}.y` this type is only allowed to point to y member.


## Metaprogramming

- [ ] Type introspection / reflection.


## Misc

- [ ] Find a more natural(?) way of organizing the headers. the lsp gets confused about the current
      header structure. I am also not completely happy with it.


## Errors

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.


---

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

- [ ] Bounds checking
- [ ] Integer overflow checks
