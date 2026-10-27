# Optional types, errors and results

For optional types and result types you need a convenient way to unwrap them.

Rust solves this by taking a functional programming approach and using pattern matching.
While I like pattern matching, I don't think it is a good match for Blu.

Zig solves this by using "captures".
For optional types you can also use the `orelse` keyword.
`orelse` is similar to `??` in Javascript.
`orelse` is lazy/short circuiting like `and` and `or`.

```zig
const x: ?i32 = null;

if (x) |y| {
    // y is i32
}

const y = x orelse return;
```

```blu
x : ?i32 = nil

if x != nil {
  // this requires flow typing
  // x is known to be a i32
  // you cannot change the type of x to i32, because then that would disallow writing nil to it.
  // I guess you could split reading and writing from and to x:
  // write: x is ?i32
  // read: x is i32
  // but what if you take a pointer to x?
  // I think it should not change the type of x, but this check is required to allow unwrapping with `x.?`
  // that is tedious however. you already checked and now you also need to unwrap it.
  // maybe allow coercion to i32
  x.? += x.?
}

y := if x != nil do x.? else return

y := if x |x| x else return;

y := x orelse return;

if x |*y| {
  y.*
}

```

## Errors and results

The result type 'operator' is binary, because it combines an `error` with a 'good' type.

If you want to do something if it is an error, use `catch`.
If you want to do something if it is not an error and rethrow if it is, use `try`.
If you want to do something custom in either case, use `if-else` with captures.

```zig
const x = parse_int(str) catch 0;

const x = parse_int(str) catch |e| return e;
const x = try parse_int(str);

if (parse_int(str)) |i| {
  ...
} else |e| {
  ...
};

errdefer _; // errdefer only runs if you exit the function with an error
```

```blu

x := parse_int(str) catch 0

x := parse_int(str) catch |e| return e
x := try parse_int(str)

x := parse_int(str)

if x |x| std.println(x)

```

Zig also uses captures in the switch for union(enum) (tagged unions) data types.
Basically, Zig uses a form of pattern matching.
This makes me think that maybe Blu should have pattern matching and let optionals and results be user code.
Although, this might be tricky for the error results.

```blu
Option := |T: type|: type {
  union {
    none,
    some: T,
  }
}

x := .none

y := x match { .some y => y, _ => 0 };
```
