## The plan

Build an AST interpreter first.
Performance is not that important.
It is not compiled ahead of time.
All checks are performed at runtime.
You will still have to write the logic for what the language does, it will "just" be done at a different time compared to an AOT compiler.

### Basic

- [x] `test\basic\main.blu`
- [x] `test\basic\if_else.blu`
- [x] `test\basic\arithmetic.blu` 27-04-2026
- [x] `test\basic\slice.blu` 30-04-2026
  - Includes array type and string literal.
  - Includes coercion of sequence literals to typed slices.
  - Includes indexing of slices.
  - Includes coercion of array to slice.
- [-] `test\basic\print.blu`
  - Add `#print` as a builtin function. This builtin is not meant to stay, but can be used during development for debugging and getting some output.
- [ ] `test\basic\defer.blu`

### Errors

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.

### Metaprogramming

- [ ] Add `#run` builtin.

### Misc

- [-] Properly cast the literal int to i32 which is the return type of main. This happens e.g. in `test/basic/main.blu`.
- [ ] Properly free values that are no longer used in the interpreter.
- [ ] Rework values to be more memory accurate.
  - For example, currently if you have an array of u8, then the payload will be an array of ValuePayload.
    So each u8 will be 'boxed' as a ValuePayload.
    What I want is that the payload will be an array of u8. No boxing.
- [x] Verify program before interpreting it. At the moment it encounters "compile" errors during execution.

---

- [ ] `for` loop

---

- [ ] Floating point type
- [ ] Pointer type. Is not allowed to be nil
- [ ] struct type

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Compile time code execution.

- [ ] Pattern matching (?)
