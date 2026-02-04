# Functions

Functions are the primary building blocks of Stride programs. They are declared using the `fn` keyword.

## Declaration and Invocation

A function declaration consists of the `fn` keyword, followed by the function name, a parenthesized list of parameters, and a return type after a colon.

```stride
// Function declaration
fn add(a: i32, b: i32): i32 {
    return a + b
}

// Function with no return value (void)
fn greet(name: string): void {
    printf("Hello, %s!\n", name)
}

fn main(): void {
    // Invoking functions
    const result: i32 = add(10, 5)
    printf("10 + 5 = %d\n", result)
    
    greet("Stride")
}
```

## Function Parameters

Parameters are passed by value by default. Each parameter must have a name and a type.

## Return Values

Functions return values using the `return` keyword. If a function is declared with a `void` return type, it does not need a `return` statement, or it can use `return` without a value.

## The `main` Function

The `main` function is the entry point of every Stride program. It should have a `void` return type.

```stride
fn main(): void {
    // Your code starts here
}
```

## External Functions

You can also declare externally linked functions (e.g., from C libraries) using the `extern` keyword. These declarations do not have a body and must end with a semicolon.

```stride
// Declare printf from the C standard library
extern fn printf(format: string, ...): i32;

// Declare other external functions
extern fn system_time_ns(): u64;
extern fn exit(code: i32): void;

fn main(): void {
    printf("System time: %llu ns\n", system_time_ns())
}
```
