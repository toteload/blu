# Currently doing

- [ ] Declaration resolution rework

## Other

- [ ] Add `return` expression.
- [ ] Reuse deallocated values! And add generation check.
- [ ] SIR code generation can be moved to `Source`.

## Some things to keep in mind
- If a type has multiple method sets that are active, which ones are actually used?
  Just all the ones that match and if they have overlap it's an error?
- Are anonymous structs nominal or structural?

## Bugs/issues found by Claude (may be out of date)

# Low / nits

- **N2 — `arena_push` can commit past the reservation** (`src/toteload.c:108-117`): near the end of
  the reserved range it commits `Max(512KiB, needed)` rounded up, without clamping to `reserve_end`,
  so `vmem_commit` could run past the mapping and trip the `Assert`. Only reachable with near-full
  arenas.
- **N3 — hashmap rehash can theoretically drop a bucket** (`src/hashmap.h:321-349`): if the chained
  relocation `for j` loop runs `cap` iterations without placing the displaced bucket, it falls
  through to `next` and the in-hand bucket is lost. Very low probability, but silent data loss
  rather than a `return False` retry.
- **N4 — `segment_list` `_copy_to_array` does `memcpy(dst, NULL, 0)`** (`src/segment_list.h:142`)
  when `len` lands exactly on a segment boundary (the last segment pointer is still `NULL`).
  Harmless in practice but technically UB.
- **N5 — little-endian assumption** in `read_unsigned_integer_extend` / `eval_cast_int`
  (`src/eval.c:24-29`) and the 8-byte-pointer assumptions in `types_size_info`. Fine for current
  targets; worth a comment.
- **N6 — `read_file` uses `i32` for `ftell`** (2 GB cap) and `arena_init` doesn't check for `mmap`'s
  `MAP_FAILED` (`src/toteload.c:56`, `src/source_file.c:79`).

## Comptime

- [ ] Add `eval` keyword to evaluate expressions at compile time. (or just use `comptime` as the name)
- [ ] Add `comptime` qualifier to function parameters.
- [ ] Add `comptime` qualifier to declarations. (Maybe not a qualifier, just use `eval`)
  - This means that the declaration only exists at compile time.
    All uses of the declaration are replaced with its value.


## General

- [ ] Type casting and coercion information is kinda scattered.
      There is `eval_unify`, `eval_coerce`, `is_type_castable_to` and `is_type_coercible_to`.
      For example, if `is_type_coercible_to` then `eval_coerce` should succeed for the same types.
      So these functions need to be kept in sync which is error prone/annoying.
      See if this can be improved.
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

- [ ] Add tests to check that integer overflows are caught at runtime.
- [ ] Add tests to check that out of range integer casts are caught at runtime.
- [ ] Properly free values that are no longer used in the interpreter.
- [ ] Find a more natural(?) way of organizing the headers. the lsp gets confused about the current
      header structure. I am also not completely happy with it.


## Errors

Maybe not implement this in the language itself.
Maybe DO implement this in the language itself.

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.


---

- [ ] Bounds checking
- [ ] Integer overflow checks
