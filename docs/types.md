# Types

## Array

The syntax for an array is `[N]T` where `N` is the size of the array and `T` is the type of each element.
Arrays can be initialized with sequence literals.

## Slice

The syntax for a slice is `[]T` where T is the type of the elements.

## Sequence literal

A sequence literal is written like `.{ a, b, c, }`.
The values in a sequence literal must all have the same type.
These literals can be used to initialize arrays and slices.

## Type coercion (implicit casts)

Sometimes you have to convert data of one type to another type.
Usually you have to be explicit and insert a `cast`, but some conversions are implicit.
These implicit casts are also called coercions.

Type coercion is performed between integer types when you are casting to an integer type with more bits and the same signedness.

`const_int` may be coerced to any integer type, and since its value is known at compile time it also checks if the value is a valid value for the destination type.
