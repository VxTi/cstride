# Variables and Types

Stride is a statically typed language.

## Creating variables

Variables are created using `let` for constants and `mut` for mutable variables. By default, variables are constant.

```stride
// Creating a mutable 32 bit int variable
mut x: i32 = 10

// Creating a constant variable
let y: f64 = 3.0D
```

## Types

### Integer Types

Stride supports several integer types:
- Signed: `i8`, `i16`, `i32`, `i64`
- Unsigned: `u8`, `u16`, `u32`, `u64`

### Integer Literals

Stride supports various integer literal formats, including decimal and hexadecimal.
For example, `0x12345` is a hexadecimal integer literal.
64-bit integer literals can be suffixed with an 'L':
```stride
let large_int: i64 = 12345678901234567890L
```

### Floating Point Types

- `f32`: Single precision
- `f64`: Double precision

Floating point literals can be suffixed with 'F' for single precision or 'D' for double precision.
For example, `3.14F` is a single precision float, and `2.71828D` is a double precision float.

### Other Types

- `bool`: Boolean type (`true`, `false`)
- `char`: Character type
- `string`: String type
- `void`: Void type

## Operators

Stride uses traditional arithmetic and comparison operators.

```stride
let x: i32 = 10
let y: f64 = 3.0D

// Arithmetic operators
let sum: f64 = x + y
let difference: f64 = x - y
let product: f64 = x * y
let quotient: f64 = x / y
let remainder: f64 = x % y

// Comparison operators
let is_equal: bool = x == y
let is_not_equal: bool = x != y
let is_less_than: bool = x < y
let is_greater_than: bool = x > y
let is_less_than_or_equal: bool = x <= y
let is_greater_than_or_equal: bool = x >= y
```
