# Variables and Types

Stride is a statically typed language.

## Variable Declaration

Stride provides keywords for variable declaration with different mutability rules:

| Keyword | Description                            |
|---------|----------------------------------------|
| `const` | Declares a constant (immutable) value. |
| `let`   | Declares a mutable variable.           |

### Examples

```stride
// Creating a constant variable
const pi: float64 = 3.14159D

// Creating a mutable 32-bit integer variable
let x: int32 = 10
x = 20 // OK

// Mutable variables in a loop
for (let i: int32 = 0; i < 10; i++) {
    printf("%d\n", i)
}
```

## Types

### Integer Types

Stride supports several integer types:
- Signed: `int8`, `int16`, `int32`, `int64`
- Unsigned: `u8`, `u16`, `u32`, `u64`

### Integer Literals

Stride supports various integer literal formats, including decimal and hexadecimal.
For example, `0x12345` is a hexadecimal integer literal.

64-bit integer literals can be suffixed with an 'L':
```stride
let large_int: int64 = 12345678901234567890L
```

### Floating Point Types

- `float32`: Single precision
- `float64`: Double precision

Floating point literals can be suffixed with 'F' for single precision or 'D' for double precision.
For example, `3.14F` is a single precision float, and `2.71828D` is a double precision float.

### Other Types

- `bool`: Boolean type (`true`, `false`)
- `char`: Character type
- `string`: String type
- `void`: Void type (used for functions that don't return a value)

## Operators

Stride uses traditional arithmetic and comparison operators.

```stride
let x: int32 = 10
let y: float64 = 3.0D

// Arithmetic operators
let sum: float64 = (x as float64) + y
let difference: float64 = (x as float64) - y
let product: float64 = (x as float64) * y
let quotient: float64 = (x as float64) / y
let remainder: int32 = x % 3

// Comparison operators
let is_equal: bool = (x as float64) == y
let is_not_equal: bool = (x as float64) != y
let is_less_than: bool = (x as float64) < y
let is_greater_than: bool = (x as float64) > y
```

### Type Casting

Stride requires explicit casting between different types using the `as` keyword.

```stride
let integer: int32 = 10
let floating: float64 = integer as float64
```
