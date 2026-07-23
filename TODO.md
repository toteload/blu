# Currently doing

I am trying to figure out how to generate code that when ran produces the partial evaluation of the
function; basically all the comptime stuff in the original function has been replaced with constants.

- [ ] Interpret the IR to produce "runtime IR"
- [ ] Interpret runtime IR to execute the program!

- [ ] Update IR_declaration to use positional arguments. So, the declared type follows right after
      the IR_declaration instruction and the declared type is followed by the declared value.
      Problem: declared type is optional. Solution: introduce IR_declaration_with_type.

## Some things to keep in mind
- If a type has multiple method sets that are active, which ones are actually used?
  Just all the ones that match and if they have overlap it's an error?
- Are anonymous structs nominal or structural?

## Bugs/issues found by Claude (may be out of date)

# M3 — Message formatting is unimplemented and the vararg path is UB

`src/messages.c:69-95`, `src/messages.h:29-61`, `src/source_file.c:6-30`, `src/compiler.c:61-83`

`print_message` prints the raw format string (`{tok}`, `{str}` placeholders appear literally) and
never consumes the collected `args`. Meanwhile the sinks read each argument with
`va_arg(vl, MessageArg)`, where `MessageArg` is a 4-byte union — but callers pass a 16-byte
`String` for `{str}` (e.g. `source_read_file`). Reading a `String` as a 4-byte union is undefined
and would desync any message with multiple args. The `{tok}`/`{type}` cases happen to work only
because a promoted `int` and a 4-byte union pass identically on the ABI. The whole arg-collection
path is effectively dead until formatting is implemented — but it's live UB in the meantime.

---

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

### Types

### Control flow

### Comptime

### Modules


## Comptime

- [ ] Update declaration syntax to allow optional omission of declaration type.
- [ ] Add `eval` keyword to evaluate expressions at compile time.
- [ ] Add `comptime` qualifier to function parameters.
- [ ] Add `comptime` qualifier to declarations.
  - This means that the declaration only exists at compile time.
    All uses of the declaration are replaced with its value.

## General

- [ ] `test\basic\slice.blu`
  - Includes array type and string literal.
  - Includes coercion of sequence literals to typed slices.
  - Includes indexing of slices.
  - Includes coercion of array to slice.
- [ ] `test\basic\print.blu`
  - Add `#print` as a builtin function. This builtin is not meant to stay, but can be used during development for debugging and getting some output.
- [ ] `test\basic\defer.blu`
- [ ] Add assign of variables.


## Code organization

- [ ] Some form of encapsulation of declarations, think packages or modules.
  - Use a toplevel package declaration to say what package you are part of.


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
- [ ] Add coercion of `[1]T`, `[1][1]T`, ... to `T`.
  - This has the benefit of having ASCII character literals for free by using the string literals.
    The type of `"A"` is `[1]u8`

- pointer to member of a struct. `PtrY := *{struct { x: i32, y: i32 }}.y` this type is only allowed to point to y member.


## Metaprogramming

- [ ] Type introspection / reflection.


## Misc

- [ ] Add tests to check that integer overflows are caught at runtime.
- [ ] Add tests to check that out of range integer casts are caught at runtime.
- [ ] Properly free values that are no longer used in the interpreter.


## Errors

Maybe not implement this in the language itself.
Maybe DO implement this in the language itself.

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.


---

- [ ] Floating point type

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Pattern matching (?)
