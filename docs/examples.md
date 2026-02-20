# Examples

This page provides various examples of Stride programs to help you understand the language syntax and features.

## 1. Hello World
The most basic program in Stride. It demonstrates how to use `printf` from the standard library.

```stride
fn main(): void {
    printf("Hello, Stride!\n")
}
```

## 2. FizzBuzz
A classic programming challenge that demonstrates loops and conditional logic.

```stride
fn main(): void {
    let limit: int32 = 20
    
    for (let i: int32 = 1; i <= limit; i++) {
        if (i % 3 == 0 && i % 5 == 0) {
            printf("FizzBuzz\n")
        } else if (i % 3 == 0) {
            printf("Fizz\n")
        } else if (i % 5 == 0) {
            printf("Buzz\n")
        } else {
            printf("%d\n", i)
        }
    }
}
```

## 3. Mathematical Approximation (e^x)

This example calculates an approximation of $e^x$ using a Taylor series. It demonstrates function definition, loops, variables, and calling external functions.

```stride
extern fn powl(base: float64, exp: float64): float64;
extern fn system_time_ns(): u64;
extern fn printf(format: string, ...): int32;

/**
 * In this example, we'll try to calculate an approximation of e^x using a taylor series.
 * This shows parts of the language;
 * - Function invocation
 * - External function calls
 * - The significant speed : )
 */
fn main(): void {
    const pow: float64 = 3.5D
    
    const start: u64 = system_time_ns()
    const approx: float64 = e_to_the_x(pow)
    const end: u64 = system_time_ns()
    
    const elapsed: u64 = end - start

    printf("Elapsed time: %llu ns; e^%.2f: %.15f\n", elapsed, pow, approx)
}


// Here we try to calculate e^x by using a taylor series
// In this case, we get an accuracy of 7 digits with 20 iterations.
fn e_to_the_x(x: float64): float64 {
    let approximation: float64 = 0.0D
    const iterations: int32 = 20

    for (let n: float64 = 0.0D; n < iterations; n++) {
        approximation += powl(x, n) / factorial(n)
    }

    return approximation
}

// A basic recursive factorial function
fn factorial(n: float64): float64 {
    if (n <= 1.0D) {
        return 1.0D
    }

    // Recursive step: n * factorial of (n-1)
    return n * factorial(n - 1.0D)
}
```

## 4. Recursive Factorial (Integer)

A classic example of recursion to calculate the factorial of a number using 64-bit integers.

```stride
fn main(): void {
    let number: int64 = 10L
    let result: int64 = factorial(number)
    printf("Factorial of %lld is %lld\n", number, result)
}

fn factorial(n: int64): int64 {
    if (n <= 1L) {
        return 1L
    }
    return n * factorial(n - 1L)
}
```

## 5. Fibonacci Sequence

Generating Fibonacci numbers using an iterative approach.

```stride
extern fn printf(format: string, ...): int32;

fn main(): void {
    print_fibonacci(10)
}

fn print_fibonacci(n: int32): void {
    let a = 0L
    let b = 1L
    
    printf("Fibonacci sequence up to %d iterations:\n", n)

    let i: int32 = 0
    for (; i < n; i++) {
        printf("%lld ", a)
        let temp: int64 = a
        a = b
        b = temp + b
    }
    printf("\n")
}
```

