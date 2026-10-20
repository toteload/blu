- [ ] Add user error reporting on failure for SIR_unify in specialize.c
- [ ] Compile time evaluation of binary operators in specialize.c
- [ ] Add type inference on declarations (`x := 5`). The type part of a declaration is optional. `case Ast_declaration` in codegen.c
- [ ] Add location information to interpreter error reporting
- [ ] The symbol table doesn't work correctly for nested functions. you keep access to variables in the enclosing function.

## Some things to keep in mind
- If a type has multiple method sets that are active, which ones are actually used?
  Just all the ones that match and if they have overlap it's an error?
- Are anonymous structs nominal or structural?

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

- [ ] Add tests to check that integer overflows are caught at runtime.
- [ ] Add tests to check that out of range integer casts are caught at runtime.
- [ ] Properly free values that are no longer used in the interpreter.
- [ ] Find a more natural(?) way of organizing the headers. the lsp gets confused about the current
      header structure. I am also not completely happy with it.


## Errors

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.


---

- [ ] Bounds checking
- [ ] Integer overflow checks
