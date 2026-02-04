# Functions

Functions are declared using the `fn` keyword.

## Declaration and Invocation

Functions are declared with a name, a list of parameters, and a return type.

```stride
fn add(a: i32, b: i32): i32 {
    return a + b
}

// Invoking a function
const result: i32 = add(10, 5)
```

## External Functions

You can also create externally linked functions if they are defined in another library using the `extern` keyword.

```stride
extern fn some_c_function(x: i32): i32;
```
