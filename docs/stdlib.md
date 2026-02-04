# Standard Library

The Stride standard library provides essential functions and utilities for common tasks. While still in early development, it already includes key modules for I/O, math, and system operations.

## Available Functions

Many standard library functions are currently available through the C runtime. You can use them by declaring them with the `extern` keyword.

### Input / Output (`stdio`)

```stride
extern fn printf(format: string, ...): i32;
extern fn scanf(format: string, ...): i32;
```

### Math (`math.h`)

```stride
extern fn powl(x: f64, y: f64): f64;
extern fn sqrt(x: f64): f64;
extern fn floor(x: f64): f64;
extern fn ceil(x: f64): f64;
```

### String Utilities (`string.h`)

```stride
extern fn strlen(s: string): i32;
extern fn strcmp(s1: string, s2: string): i32;
```

### System Operations

```stride
extern fn system_time_ns(): u64;
extern fn exit(code: i32): void;
```

## Planned Module System

A full module system is currently being implemented. In the future, you will be able to import standard library modules directly:

```stride
import std::{ Math, IO };

fn main(): void {
    let x = Math::sqrt(16.0D);
    IO::println("Square root is: " + (x as string));
}
```
