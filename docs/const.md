# Const evaluation

There are two important keywords when it comes to compile time code evaluation: `const` and `eval`.
`const` is a prefix-qualifier that may be applied to declarations; top level declarations, block level declarations
and function parameters.
`eval` forces the expression after it to be evaluated at compile time.
All values in the expression must be known at compile time.

The values of `const` qualified declarations must be known at compile time.
At compile time all their occurences are replaced with the value that they hold.
