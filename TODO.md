# Currently doing

I am trying to figure out how to generate code that when ran produces the partial evaluation of the
function; basically all the comptime stuff in the original function has been replaced with constants.

- [ ] Make sure that `0` for IrRef means nil
- [ ] Add IR pretty printer
- [ ] Debug IR generation (if necessary)
- [ ] Interpret the IR to produce "runtime IR"
- [ ] Interpret runtime IR to execute the program!

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
  - Sized booleans? `u32(bool)`. `bool := enum { false, true }`
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
