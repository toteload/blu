# Types

## Array

The syntax for an array is `[N]T` where `N` is the size of the array and `T` is the type of each element.
Arrays can be initialized with sequence literals.

## Sequence literal

A sequence literal is written like `.{ a, b, c, }`.
The values in a sequence literal must all have the same type.
A sequence literal can also be written with explicit index assignment.
For example, `.{ [0] = 1, [3] = 7, }` has 1 at index 0 and 7 at index 3.
The values of at unspecified indices is undefined.
