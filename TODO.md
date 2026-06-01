## In progress

- [ ] Allow type annotations on params of function literals and a return type annotation.

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
- [ ] Update declaration syntax to allow optional omission of declaration type.
- [ ] Add `const` qualifier to declarations.
  - This means that the declaration only exists at compile time.
    All uses of the declaration are replaced with its value.
- [ ] Add `const` qualifier to function parameters.


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
- [ ] refinement
- [ ] `enum` definitions.
  - I think something like `Direction := enum { north, east, south, west }` is good.
  - Effectively, the enum values are named integer constants. So `Direction` does not have a size.
    - Then what is the type of `Direction`? I am guessing `distinct enum`.
  - You can 'size' an enum through integer refinement, e.g. `u32(Direction) dir`.
  - Sized booleans? `u32(bool)`. `bool := enum { false, true }` 
- [ ] Add coercion of `bool` to integer types. 
- [ ] Add coercion of `[1]T`, `[1][1]T`, ... to `T`.
  - This has the benefit of having ASCII character literals for free by using the string literals.
    The type of `"A"` is `[1]u8`
- [ ] Add `const` qualifier to types.
  - Values of a `const` type may only exist at compile time.
    Declarations or parameters that have a `const` type are implicitly `const` themselves.


## Metaprogramming

- [ ] Add `eval` keyword to evaluate expressions at compile time.
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
- [ ] Pointer type. Is not allowed to be nil

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Pattern matching (?)
