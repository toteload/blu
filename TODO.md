- [ ] Add semantic validation and get passing `test\basic\main.blu`.
  - [ ] Validate symbol use.
  - [ ] Anotate types.
  - [ ] Type check.
- [ ] Compile HIR to C. Get it working for `test\basic\main.blu`.
- [ ] Get `test\basic\if_else.blu` to compile.
- [ ] Set up extendible program/script that runs all tests and gives a summary.
- [ ] Get `test\basic\call_function.blu` to compile.
- [ ] Get `test\basic\slice.blu` to compile.
- [ ] Get `test\error\out_of_bounds.blu` to fail compilation.

- [ ] Control flow analysis.
    - [ ] Does a function return a value?

- [ ] Array type
- [ ] Floating point type
- [ ] Pointer type. Is not allowed to be nil
- [ ] struct type

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Return error from function
    - [ ] Add `try` and `catch` for error handling.
- [ ] Add `defer`. Gets executed at end of scope.
    - [ ] Add `defer catch`. Gets executed at end of scope when an error occured.
    - [ ] Add `defer try`. Gets executed at end of scope when no error occured.

- [ ] Compile time code execution.

- [ ] Pattern matching (?)
