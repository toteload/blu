# Const evaluation

## Valid uses of `const`

- Declarations, the declaration will only be available at compile time.
- Identifier, `const x` is valid as long as `x` is a `const` declaration and in scope.
  This is superfluous.

## Illegal uses of `const`

- Assignment (`=`), variables may not be reassigned at compile time even if the variable was declared with `const`.
  This does not mean that you cannot use variables at all at compile time.
  For example, variables declared in a `const` block may be reassigned in the same block.
- `const defer` is only valid inside a `const` block, similar to assignment.
